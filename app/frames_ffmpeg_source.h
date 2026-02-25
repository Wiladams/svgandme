#pragma once

// FFmpeg frame source for <frames>
// - Decodes video / streams via libavformat + libavcodec
// - Converts to BGRA via libswscale and writes into PixelArray (BL_FORMAT_PRGB32)
//
// Notes:
// - BL_FORMAT_PRGB32 expects premultiplied alpha. Most video is opaque.
//   If you expect alpha-bearing sources, consider premultiplying after conversion.
// - Cropping is expressed in authored SVGNumberOrPercent and resolved against
//   decoded frame dimensions.
// - This implementation loops files on EOF by seeking back to start.
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

#include <memory>
#include <cstdint>
#include <cstring>

#include "framesource.h"
#include "stopwatch.h"
#include "pixelaccessor.h"
#include "nametable.h"
#include "bspan.h"
#include "charset.h"

namespace waavs {

    static INLINE int64_t clamp_i64(int64_t v, int64_t lo, int64_t hi) noexcept
    {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    static INLINE bool isLikelyNetworkUrl(const ByteSpan& s) noexcept
    {
        // cheap check; you already have scheme parsing upstream
        ByteSpan found{};
        return chunk_find(s, ByteSpan("://"), found);
    }

    struct AvFormatDeleter {
        void operator()(AVFormatContext* ctx) const noexcept {
            if (ctx) avformat_close_input(&ctx);
        }
    };

    struct AvCodecDeleter {
        void operator()(AVCodecContext* ctx) const noexcept {
            if (ctx) avcodec_free_context(&ctx);
        }
    };

    struct AvFrameDeleter {
        void operator()(AVFrame* f) const noexcept {
            if (f) av_frame_free(&f);
        }
    };

    struct AvPacketDeleter {
        void operator()(AVPacket* p) const noexcept {
            if (p) av_packet_free(&p);
        }
    };

    class FFmpegFrameSource final : public IFrameSource
    {
        static constexpr double kDefaultFrameRate = 30.0;

        bool fIsValid = false;

        // Source identity
        InternedKey fSourceKey{ nullptr };

        // FFmpeg core
        std::unique_ptr<AVFormatContext, AvFormatDeleter> fFmt{};
        std::unique_ptr<AVCodecContext, AvCodecDeleter>   fCodec{};
        std::unique_ptr<AVFrame, AvFrameDeleter>          fFrame{};
        std::unique_ptr<AVPacket, AvPacketDeleter>        fPkt{};
        SwsContext* fSws = nullptr;

        int fVideoStream = -1;
        AVRational fTimeBase{ 1, 1 };

        // Decoded frame size (source)
        int fSrcW = 0;
        int fSrcH = 0;
        AVPixelFormat fSrcFmt = AV_PIX_FMT_NONE;

        // Crop in source pixel space (resolved)
        int64_t fCropX = 0;
        int64_t fCropY = 0;
        int64_t fCropW = 0;
        int64_t fCropH = 0;
        bool fCropIsFull = true;

        // Output pixels used by renderer (crop-sized)
        PixelArray fPixels{};

        // Optional full-frame staging buffer (only used when crop is not full)
        PixelArray fFullPixels{};

        // Throttling
        StopWatch fTimer;
        double fMinInterval = 0.0;
        double fLastTime = 0.0;

    public:
        FFmpegFrameSource() = default;

        ~FFmpegFrameSource() override
        {
            if (fSws) {
                sws_freeContext(fSws);
                fSws = nullptr;
            }
        }

        void setMaxFrameRate(double fps) noexcept
        {
            fMinInterval = (fps > 0.0) ? (1.0 / fps) : 0.0;
        }

        PixelArray& pixels() noexcept override { return fPixels; }
        const PixelArray& pixels() const noexcept override { return fPixels; }

    private:
        void clampCropToBounds() noexcept
        {
            const int64_t sw = (int64_t)fSrcW;
            const int64_t sh = (int64_t)fSrcH;

            if (fCropX < 0) fCropX = 0;
            if (fCropY < 0) fCropY = 0;
            if (fCropW < 0) fCropW = 0;
            if (fCropH < 0) fCropH = 0;

            if (fCropX > sw) fCropX = sw;
            if (fCropY > sh) fCropY = sh;

            if (fCropX + fCropW > sw) fCropW = sw - fCropX;
            if (fCropY + fCropH > sh) fCropH = sh - fCropY;

            fCropIsFull = (fCropX == 0 && fCropY == 0 && fCropW == sw && fCropH == sh);
        }

        bool openInput(const ByteSpan& src) noexcept
        {
            // Convert ByteSpan to null-terminated C string for ffmpeg.
            // We rely on PSNameTable::INTERN for a stable c-string.
            fSourceKey = PSNameTable::INTERN(src);
            if (!fSourceKey)
                return false;

            AVFormatContext* rawFmt = nullptr;

            // Optional: tune for low latency / networking. Keep minimal here.
            AVDictionary* opts = nullptr;

            // If you want to add options later:
            // av_dict_set(&opts, "rtsp_transport", "tcp", 0);
            // av_dict_set(&opts, "fflags", "nobuffer", 0);

            int err = avformat_open_input(&rawFmt, fSourceKey, nullptr, &opts);
            av_dict_free(&opts);

            if (err < 0 || !rawFmt)
                return false;

            fFmt.reset(rawFmt);

            err = avformat_find_stream_info(fFmt.get(), nullptr);
            if (err < 0)
                return false;

            // Find best video stream
            err = av_find_best_stream(fFmt.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
            if (err < 0)
                return false;

            fVideoStream = err;
            AVStream* st = fFmt->streams[fVideoStream];
            if (!st)
                return false;

            fTimeBase = st->time_base;

            // Create decoder
            const AVCodecParameters* par = st->codecpar;
            if (!par)
                return false;

            const AVCodec* dec = avcodec_find_decoder(par->codec_id);
            if (!dec)
                return false;

            AVCodecContext* rawCodec = avcodec_alloc_context3(dec);
            if (!rawCodec)
                return false;

            fCodec.reset(rawCodec);

            err = avcodec_parameters_to_context(fCodec.get(), par);
            if (err < 0)
                return false;

            err = avcodec_open2(fCodec.get(), dec, nullptr);
            if (err < 0)
                return false;

            fFrame.reset(av_frame_alloc());
            fPkt.reset(av_packet_alloc());
            if (!fFrame || !fPkt)
                return false;

            // Prefer coded size if set; otherwise use codec context.
            fSrcW = fCodec->width;
            fSrcH = fCodec->height;
            fSrcFmt = (AVPixelFormat)fCodec->pix_fmt;

            if (fSrcW <= 0 || fSrcH <= 0)
                return false;

            return true;
        }

        bool ensureSws(int dstW, int dstH, AVPixelFormat dstFmt) noexcept
        {
            // Rebuild sws if parameters changed.
            // (For simplicity: always rebuild on reset; for streams that change,
            // rebuild when frame size or format changes.)
            if (fSws) {
                sws_freeContext(fSws);
                fSws = nullptr;
            }

            fSws = sws_getContext(
                fSrcW, fSrcH, fSrcFmt,
                dstW, dstH, dstFmt,
                SWS_BILINEAR,
                nullptr, nullptr, nullptr);

            return fSws != nullptr;
        }

        bool decodeOneFrame() noexcept
        {
            // Read packets until we can output one frame.
            for (;;)
            {
                int err = av_read_frame(fFmt.get(), fPkt.get());
                if (err < 0)
                {
                    // EOF or error. If file-like, loop to start.
                    // For live streams this may block/fail; you can refine policy later.
                    if (av_seek_frame(fFmt.get(), fVideoStream, 0, AVSEEK_FLAG_BACKWARD) >= 0) {
                        avcodec_flush_buffers(fCodec.get());
                        continue;
                    }
                    return false;
                }

                if (fPkt->stream_index != fVideoStream) {
                    av_packet_unref(fPkt.get());
                    continue;
                }

                err = avcodec_send_packet(fCodec.get(), fPkt.get());
                av_packet_unref(fPkt.get());
                if (err < 0) {
                    // Try to continue on decode errors
                    continue;
                }

                err = avcodec_receive_frame(fCodec.get(), fFrame.get());
                if (err == AVERROR(EAGAIN)) {
                    continue;
                }
                if (err < 0) {
                    return false;
                }

                // Frame received.
                // Update source size/format if stream changes mid-flight.
                if (fFrame->width > 0 && fFrame->height > 0) {
                    if (fFrame->width != fSrcW || fFrame->height != fSrcH || (AVPixelFormat)fFrame->format != fSrcFmt) {
                        fSrcW = fFrame->width;
                        fSrcH = fFrame->height;
                        fSrcFmt = (AVPixelFormat)fFrame->format;
                        // Sws will be rebuilt by caller if needed.
                    }
                }

                return true;
            }
        }

        static INLINE void blitCropBGRA(const PixelArray& src, PixelArray& dst, int64_t x, int64_t y, int64_t w, int64_t h) noexcept
        {
            // Both are BGRA 4 bytes per pixel in a BLImage backing store.
            const int64_t bpp = 4;
            const int64_t rowBytes = w * bpp;

            for (int64_t row = 0; row < h; ++row)
            {
                const uint8_t* s = src.rowPointer((size_t)(y + row)) + (size_t)(x * bpp);
                uint8_t* d = dst.rowPointer((size_t)row);
                std::memcpy(d, s, (size_t)rowBytes);
            }
        }

    public:
        bool reset(const FrameSourceDesc& desc) noexcept override
        {
            fIsValid = false;

            if (desc.src.empty())
                return false;

            // Throttle
            if (desc.maxFps > 0.0) setMaxFrameRate(desc.maxFps);
            else setMaxFrameRate(kDefaultFrameRate);

            // Open input + decoder
            if (!openInput(desc.src))
                return false;

            // Resolve crop against decoded dimensions
            fCropX = (int64_t)bindNumberOrPercent(desc.cropX, (double)fSrcW, 0.0);
            fCropY = (int64_t)bindNumberOrPercent(desc.cropY, (double)fSrcH, 0.0);
            fCropW = (int64_t)bindNumberOrPercent(desc.cropW, (double)fSrcW, (double)fSrcW);
            fCropH = (int64_t)bindNumberOrPercent(desc.cropH, (double)fSrcH, (double)fSrcH);

            clampCropToBounds();
            if (fCropW <= 0 || fCropH <= 0)
                return false;

            // Output buffer is crop size
            if (!fPixels.reset((size_t)fCropW, (size_t)fCropH))
                return false;

            // If crop is not full, allocate a full-frame staging buffer
            if (!fCropIsFull)
            {
                if (!fFullPixels.reset((size_t)fSrcW, (size_t)fSrcH))
                    return false;
            }
            else
            {
                // Ensure staging buffer is cleared
                fFullPixels = PixelArray{};
            }

            // Build sws for conversion to BGRA.
            // We convert into full-size or crop-size depending on path.
            const int dstW = fCropIsFull ? fSrcW : fSrcW;
            const int dstH = fCropIsFull ? fSrcH : fSrcH;
            if (!ensureSws(dstW, dstH, AV_PIX_FMT_BGRA))
                return false;

            fLastTime = 0.0;

            // Prime with first frame
            if (!update())
                return false;

            fIsValid = true;
            return true;
        }

        bool update() noexcept override
        {
            // Throttle
            const double now = fTimer.seconds();
            if ((now - fLastTime) < fMinInterval)
                return false;

            if (!fFmt || !fCodec || !fFrame || !fPkt)
                return false;

            // Decode one frame
            if (!decodeOneFrame())
                return false;

            // If stream changed size/format, rebuild sws + buffers
            if (fFrame->width != fSrcW || fFrame->height != fSrcH || (AVPixelFormat)fFrame->format != fSrcFmt)
            {
                fSrcW = fFrame->width;
                fSrcH = fFrame->height;
                fSrcFmt = (AVPixelFormat)fFrame->format;

                clampCropToBounds();
                if (fCropW <= 0 || fCropH <= 0)
                    return false;

                if (!fPixels.reset((size_t)fCropW, (size_t)fCropH))
                    return false;

                if (!fCropIsFull) {
                    if (!fFullPixels.reset((size_t)fSrcW, (size_t)fSrcH))
                        return false;
                }
                else {
                    fFullPixels = PixelArray{};
                }

                if (!ensureSws(fSrcW, fSrcH, AV_PIX_FMT_BGRA))
                    return false;
            }

            // Convert frame to BGRA.
            // Use fast path when crop is full-frame: write directly into fPixels.
            if (fCropIsFull)
            {
                BLImageData out{};
                if (fPixels.image().getData(&out) != BL_SUCCESS)
                    return false;

                uint8_t* dstData[4] = { (uint8_t*)out.pixelData, nullptr, nullptr, nullptr };
                int dstLinesize[4] = { (int)out.stride, 0, 0, 0 };

                const uint8_t* srcData[4] = {
                    fFrame->data[0], fFrame->data[1], fFrame->data[2], fFrame->data[3]
                };
                int srcLinesize[4] = {
                    fFrame->linesize[0], fFrame->linesize[1], fFrame->linesize[2], fFrame->linesize[3]
                };

                sws_scale(fSws, srcData, srcLinesize, 0, fSrcH, dstData, dstLinesize);
            }
            else
            {
                // Convert full frame into fFullPixels, then crop-copy into fPixels.
                BLImageData full{};
                if (fFullPixels.image().getData(&full) != BL_SUCCESS)
                    return false;

                uint8_t* dstData[4] = { (uint8_t*)full.pixelData, nullptr, nullptr, nullptr };
                int dstLinesize[4] = { (int)full.stride, 0, 0, 0 };

                const uint8_t* srcData[4] = {
                    fFrame->data[0], fFrame->data[1], fFrame->data[2], fFrame->data[3]
                };
                int srcLinesize[4] = {
                    fFrame->linesize[0], fFrame->linesize[1], fFrame->linesize[2], fFrame->linesize[3]
                };

                sws_scale(fSws, srcData, srcLinesize, 0, fSrcH, dstData, dstLinesize);

                blitCropBGRA(fFullPixels, fPixels, fCropX, fCropY, fCropW, fCropH);
            }

            fLastTime = now;
            return true;
        }
    };

} // namespace waavs