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
//
// Reference: 
// https://github.com/leandromoreira/ffmpeg-libav-tutorial
// https://github.com/leandromoreira/ffmpeg-libav-tutorial?tab=readme-ov-file#chapter-0---the-infamous-hello-world
//

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

    enum FFMpegDecodeResult : uint32_t
    {
        None = 0,
        FrameReady,
        FrameReadyStructureChanged,
        Rewound,
        EndOfStream,
        Failed
    };


    // Representation of a piece of media opened with ffmpeg.
    // This is meant to be a simple wrapper around the ffmpeg APIs, 
    // to keep things organized.
    // You can do the open(), then ask for frames
    // Note:  It might be nice to do pub/sub for those who
    // want to be notified.  There are two conditions that are interesting
    // 
    // 1) Receiving a new frame
    // 2) Receiving a new frame with a different size/format 
    //      (which may require renderer reconfig).

    struct FFMpegFrameFormat
    {
        AVPixelFormat format{ AV_PIX_FMT_NONE };
        int width{};
        int height{};

        void reset(AVPixelFormat fmt, int w, int h) noexcept
        {
            format = fmt;
            width = w;
            height = h;
        }

        bool equals(const FFMpegFrameFormat& other) const noexcept
        {
            return format == other.format &&
                width == other.width &&
                height == other.height;
        }
    };

    struct FFMpegMedia
    {
        InternedKey fSourceKey{ nullptr };
    
        AVDictionary* fOpts = nullptr;
        AVFormatContext* fFormatContext = nullptr;

        const AVCodec* fCodec{ nullptr };
        AVCodecContext* fCodecContext = nullptr;
        
        int fVideoStreamIndex = -1;
        AVStream* fVideoStream{ nullptr };
        AVRational fTimeBase{ 1, 1 };

        // Media structure, as reported when originally opened.  
        FFMpegFrameFormat fCurrentStructure{};


        // Frame handling
        AVPacket* fPkt{ nullptr };
        AVFrame* fFrame{ nullptr };

        // Configurable behavior
        bool fAutoLoop{ false };

        FFMpegMedia() = default;


        ~FFMpegMedia()
        {
            reset();
        }

        void reset()
        {
            if (fFormatContext) {
                avformat_close_input(&fFormatContext);
                fFormatContext = nullptr;
            }

            if (fCodecContext) {
                avcodec_free_context(&fCodecContext);
                fCodecContext = nullptr;
            }

            if (fFrame) {
                av_frame_free(&fFrame);
                fFrame = nullptr;
            }

            if (fPkt) {
                av_packet_free(&fPkt);
                fPkt = nullptr;
            }
        }

        void setAutoLoop(bool loop) noexcept {fAutoLoop = loop;}
        bool isAutoLoop() const noexcept {return fAutoLoop;}   

        // How the caller can get a hold of the last read frame
        const AVFrame* currentFrame() const { return fFrame;}
        const FFMpegFrameFormat& currentStructure() const { return fCurrentStructure; }
        
        
        // open()
        //
        // Opens the source and prepares for decoding.  
        // This is where we first learn what the media 
        // structure is (size/format).
        // 
        // Return: 0 on success, negative on failure.
        //
        int open(const ByteSpan &src)
        {
            // Intern the key in case we're storing in a library
            fSourceKey = PSNameTable::INTERN(src);

            int err = avformat_open_input(&fFormatContext, fSourceKey, nullptr, &fOpts);
            av_dict_free(&fOpts);

            if (err < 0 || !fFormatContext)
                return err;

            err = avformat_find_stream_info(fFormatContext, nullptr);
            if (err < 0)
                return err;

            // Find best video stream
            err = av_find_best_stream(fFormatContext, 
                AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
            if (err < 0)
                return err;

            fVideoStreamIndex = err;
            fVideoStream = fFormatContext->streams[fVideoStreamIndex];
            if (!fVideoStream)
                return -1;

            fTimeBase = fVideoStream->time_base;

            // Create decoder
            const AVCodecParameters* par = fVideoStream->codecpar;
            if (!par)
                return -1;

            fCodec = avcodec_find_decoder(par->codec_id);
            if (!fCodec)
                return -1;

            fCodecContext = avcodec_alloc_context3(fCodec);
            if (!fCodecContext)
                return -1;

            err = avcodec_parameters_to_context(fCodecContext, par);
            if (err < 0)
                return err;

            err = avcodec_open2(fCodecContext, fCodec, nullptr);
            if (err < 0)
                return err;

            fFrame = av_frame_alloc();
            fPkt = av_packet_alloc();
            if (!fFrame || !fPkt)
                return -1;

            // Prefer coded size if set; 
            // otherwise use codec context.
            fCurrentStructure.reset((AVPixelFormat)fCodecContext->pix_fmt,
                fCodecContext->coded_width > 0 ? fCodecContext->coded_width : fCodecContext->width,
                fCodecContext->coded_height > 0 ? fCodecContext->coded_height : fCodecContext->height);


            if (fCurrentStructure.width <= 0 || fCurrentStructure.height <= 0)
                return -1;

            // decode one frame to get the real size/format, 
            // and to prime the decoder for streaming.
            // Then the caller can get an initial size for the backing
            // surface, and can subscribe to changes if they want.
            FFMpegDecodeResult decoderr = decodeOneFrame();
            if ((decoderr != FFMpegDecodeResult::FrameReady)  &&
                (decoderr != FFMpegDecodeResult::FrameReadyStructureChanged))
                return 
                
                FFMpegDecodeResult::Failed;

            return 0;
        }

        int tryRewind() const
        {
            // EOF or error. If file-like, loop to start.
            // For live streams this may block/fail; you can refine policy later.
            if (av_seek_frame(fFormatContext, fVideoStreamIndex, 0, AVSEEK_FLAG_BACKWARD) >= 0)
            {
                avcodec_flush_buffers(fCodecContext);
                return 0;
            }

            return -1;
        }

        FFMpegDecodeResult decodeOneFrame() noexcept
        {
            // Read packets until we can output one frame.
            for (;;)
            {
                int err = av_read_frame(fFormatContext, fPkt);

                // If we get an error, we might be at EOF
                // If file-like, try to rewind and continue; 
                // otherwise fail.
                if (err < 0)
                {
                    if (err == AVERROR_EOF)
                    {
                        if (isAutoLoop())
                        {
                            if (tryRewind() < 0)
                                return FFMpegDecodeResult::Failed;

                            continue;
                        }
                        return FFMpegDecodeResult::EndOfStream;
                    }

                    return FFMpegDecodeResult::Failed;
                }

                if (fPkt->stream_index != fVideoStreamIndex) {
                    av_packet_unref(fPkt);
                    continue;
                }

                err = avcodec_send_packet(fCodecContext, fPkt);
                av_packet_unref(fPkt);
                if (err < 0) {
                    // Try to continue in the face of decode errors
                    continue;
                }

                err = avcodec_receive_frame(fCodecContext, fFrame);
                if (err == AVERROR(EAGAIN)) {
                    // if we see EAGAIN, it means the decoder needs more 
                    // packets before it can output a frame.
                    // so continue
                    continue;
                }

                // Any other error is a failure.
                if (err < 0) {
                    return FFMpegDecodeResult::Failed;
                }

                FFMpegFrameFormat newStruct;
                newStruct.reset((AVPixelFormat)fFrame->format, fFrame->width, fFrame->height);

                if (!fCurrentStructure.equals(newStruct))
                {
                    fCurrentStructure = newStruct;
                    return FFMpegDecodeResult::FrameReadyStructureChanged;
                }

                return FFMpegDecodeResult::FrameReady;
            }
        }
    };


    // FFmpegFrameSource
    //
    // Supporting FFMPEG in the IFrameSource interface.
    // This allows us to display moving pictures from video files
    // and streams.
    class FFmpegFrameSource final : public IFrameSource
    {
        static constexpr double kDefaultFrameRate = 30.0;

        bool fIsValid = false;

        FFMpegMedia fMedia{};

        SwsContext* fSws = nullptr;

        // Decoded frame size and format
        FFMpegFrameFormat fCurrentStruct{};


        // Crop in source pixel space (resolved)
        bool fCropIsFull = true;

        // Output pixels used by renderer (crop-sized)
        Surface fCroppedPixels{};

        // Full frame of captured pixels
        Surface fPixels{};

    public:
        FFmpegFrameSource() = default;

        ~FFmpegFrameSource() override
        {
            if (fSws) {
                sws_freeContext(fSws);
                fSws = nullptr;
            }
        }


        Surface& pixels() noexcept override { return fCroppedPixels; }
        const Surface& pixels() const noexcept override { return fCroppedPixels; }

    private:
        void clampCropToBounds() noexcept
        {
            // Create a rectangle based on the crop parameters
            WGRectI cropRect{ (int)fCropX, (int)fCropY, (int)fCropWidth, (int)fCropHeight };

            // Create a rectangle based on the source dimensions
            WGRectI srcRect{ 0, 0, fCurrentStruct.width, fCurrentStruct.height };

            // Intersect the crop rectangle with the source rectangle to ensure it fits within bounds
            fCroppedRect = intersection(cropRect, srcRect);

            fCropIsFull = (fCroppedRect == srcRect);
        }

        // This should only be called when the format
        // changes
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
                fCurrentStruct.width, fCurrentStruct.height, fCurrentStruct.format,
                dstW, dstH, dstFmt,
                SWS_BILINEAR,
                nullptr, nullptr, nullptr);

            return fSws != nullptr;
        }

    public:
        bool reset(const FrameSourceDesc& desc) noexcept override
        {
            fIsValid = false;

            if (desc.src.empty())
                return false;

            // Throttle
            if (desc.maxFps > 0.0) 
                setFrameRate(desc.maxFps);
            else 
                setFrameRate(kDefaultFrameRate);

            // Open input + decoder
            int err = fMedia.open(desc.src);
            if (err < 0)
                return false;

            fMedia.setAutoLoop(true);

            // get the current size and format information
            fCurrentStruct = fMedia.currentStructure();

            // Resolve crop against decoded dimensions
            fCropX = (int64_t)resolveNumberOrPercent(desc.cropX, (double)fCurrentStruct.width, 0.0);
            fCropY = (int64_t)resolveNumberOrPercent(desc.cropY, (double)fCurrentStruct.height, 0.0);
            fCropWidth = (int64_t)resolveNumberOrPercent(desc.cropW, (double)fCurrentStruct.width, (double)fCurrentStruct.width);
            fCropHeight = (int64_t)resolveNumberOrPercent(desc.cropH, (double)fCurrentStruct.height, (double)fCurrentStruct.height);

            clampCropToBounds();
            if (fCroppedRect.w <= 0 || fCroppedRect.h <= 0)
                return false;

            // Allocate full buffer to match media size
            fPixels.reset((size_t)fCurrentStruct.width, (size_t)fCurrentStruct.height);
            fPixels.clearAll();

            // Crop buffer is a subregion of the full buffer,
            // possibly the same as the full buffer size
            if (fPixels.getSubSurface(fCroppedRect, fCroppedPixels) != WG_SUCCESS)
                return false;



            // Build sws for conversion to BGRA.
            // just decode the full size and let the cropping
            // view do what it does
            if (!ensureSws(fCurrentStruct.width, fCurrentStruct.height, AV_PIX_FMT_BGRA))
                return false;

            fLastCaptureTime = 0.0;

            // Prime with first frame
            update();

            fIsValid = true;

            return true;
        }

        bool update() noexcept override
        {
            // Throttle
            const double now = fTimer.seconds();
            if ((now - fLastCaptureTime) < fMinInterval)
                return false;

            // Decode one frame
            FFMpegDecodeResult res = fMedia.decodeOneFrame();
            if (res == FFMpegDecodeResult::Failed)
                return false;

            if (res == FFMpegDecodeResult::FrameReadyStructureChanged)
            {
                // Update our current structure information
                fCurrentStruct = fMedia.currentStructure();
                clampCropToBounds();
                if (!ensureSws(fCurrentStruct.width, fCurrentStruct.height, AV_PIX_FMT_BGRA))
                    return false;

                fPixels.reset(fCurrentStruct.width, fCurrentStruct.height);
            }
            else if (res == FFMpegDecodeResult::FrameReady)
            {
            }

            // Convert frame to BGRA.
            if (!fPixels.data())
                return false;

            uint8_t* dstData[4] = { (uint8_t*)fPixels.data(), nullptr, nullptr, nullptr };
            int dstLinesize[4] = { (int)fPixels.stride(), 0, 0, 0 };

            const AVFrame* aFrame = fMedia.currentFrame();
            const uint8_t* srcData[4] = {
                aFrame->data[0], aFrame->data[1], aFrame->data[2], aFrame->data[3]
            };
            int srcLinesize[4] = {
                aFrame->linesize[0], aFrame->linesize[1], aFrame->linesize[2], aFrame->linesize[3]
            };

            sws_scale(fSws, srcData, srcLinesize, 0, fCurrentStruct.height, dstData, dstLinesize);


            fLastCaptureTime = now;

            return true;
        }
    };

} // namespace waavs