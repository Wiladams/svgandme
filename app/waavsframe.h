#pragma once

#include <memory>
#include <vector>



#include "converters.h"

#include "svggraphicselement.h"
#include "screensnapshot.h"
#include "frames_blend2d_source.h"
#include "frames_ffmpeg_source.h"

namespace waavs
{
    //
// <frames> Source Scheme Resolution
// ----------------------------------
//
// The `src` attribute may optionally include a URI-style scheme prefix:
//
//     scheme:rest
//
// If no explicit scheme is present, the source kind is inferred from
// the presence of "://", or from the file extension.
//
// ---------------------------------------------------------------------
// Explicitly Supported Schemes
// ---------------------------------------------------------------------
//
//   monitor:MONITOR_NAME
//       -> FramesSourceKind::Screen
//       Uses platform screen capture backend.
//       Example: "screen:default" or "screen:\\\\.\\DISPLAY1"
// 
//   ffmpeg:ANYTHING
//       -> FramesSourceKind::FFmpeg
//       Forces FFmpeg backend for file, device, or stream.
//
//   file:PATH
//       -> Inferred from file extension (Blend2DImage or FFmpeg).
//
//   rtsp://...
//   http://...
//   https://...
//   udp://...
//   tcp://...
//   srt://...
//       -> FramesSourceKind::FFmpeg
//       Network / streaming protocols handled by FFmpeg.
//
// ---------------------------------------------------------------------
// Implicit Behavior (No Explicit Scheme)
// ---------------------------------------------------------------------
//
//   If "://" appears in the string
//       -> FramesSourceKind::FFmpeg
//
//   Otherwise, inference is based on file extension:
//
//       Image formats  -> FramesSourceKind::Blend2DImage
//           png, apng, gif, jpg, jpeg, bmp, webp
//
//       Video formats  -> FramesSourceKind::FFmpeg
//           mp4, mkv, mov, avi, webm, ts, m3u8
//
//       Unknown extension
//           -> Defaults to FramesSourceKind::Blend2DImage
//
// ---------------------------------------------------------------------
// Notes
// ---------------------------------------------------------------------
//
//   * Scheme detection follows URI-style grammar:
//         ALPHA *( ALNUM / "+" / "-" / "." )
//
//   * Windows drive paths (e.g., "C:\file.mp4") are NOT treated as schemes.
//
//   * All scheme and extension comparisons use InternedKey
//     (PSNameTable::INTERN) for fast dispatch.
//
//   * The <frames> element is intentionally separate from <image>,
//     and supports animated images, video files, streams, and screen capture.
//

    static constexpr charset kSchemeFirst = chrAlphaChars;
    static constexpr charset kSchemeRest = chrAlphaChars + chrDecDigits + "+-.";


    enum class FramesSourceKind : uint8_t
    {
        None = 0,
        MONITOR,
        Blend2DImage,      // static or animated via BLImageCodec
        FFmpeg
    };

    static bool parseFrameSourceScheme(const ByteSpan& src, ByteSpan& scheme, ByteSpan& rest)
    {
        scheme.reset();
        rest.reset();

        if (src.empty())
            return false;

        const uint8_t* ptr = src.begin();
        const uint8_t* endPtr = src.end();

        // Find first ':' but abort if we see '/' or '\' first
        // and treat that as a path instead
        const uint8_t* colon = nullptr;
        for (const uint8_t* it = ptr; it < endPtr; ++it)
        {
            const uint8_t c = *it;
            if (c == ':') { colon = it; break; }
            if (c == '/' || c == '\\') { return false; }
        }

        if (!colon)
            return false; // No colon found, so no scheme

        const size_t schemeLen = (size_t)(colon - ptr);
        if (schemeLen == 0)
            return false; // Empty scheme is not valid

        // Reject Windows drive letter: "C:\path\to\file"
        if (schemeLen == 1 && kSchemeFirst(ptr[0]))
            return false;

        // Validate first character of scheme
        if (!kSchemeFirst(ptr[0]))
            return false;

        // Validate remaining characters of scheme
        if (schemeLen > 1)
        {
            ByteSpan tail = ByteSpan::fromPointers(ptr + 1, colon);
            if (!isAll(tail, kSchemeRest))
                return false;
        }

        // Now we have enough to actually return the scheme and rest
        scheme = ByteSpan::fromPointers(ptr, colon);
        rest = ByteSpan::fromPointers(colon + 1, endPtr);

        return true;
    }


    // Find the file extension (without '.') in `src`, bounded by path separators.
    // Returns empty ByteSpan if no extension present.
    static INLINE ByteSpan extractExtension(const ByteSpan& src) noexcept
    {
        if (!src) return {};

        const uint8_t* p = src.begin();
        const uint8_t* e = src.end();

        // Scan backward:
        // - stop if we hit '/' or '\'
        // - if we hit '.', extension starts after it
        for (const uint8_t* it = e; it > p; )
        {
            --it;
            const uint8_t c = *it;

            if (c == '/' || c == '\\')
                break;

            if (c == '.')
            {
                const uint8_t* extStart = it + 1;
                if (extStart >= e)
                    return {};
                return ByteSpan::fromPointers(extStart, e);
            }
        }

        return {};
    }


    // Lowercase an extension into a small stack buffer and intern it.
    // Returns 0 if ext is empty or too long for the buffer.
    static INLINE InternedKey internLowerExt(const ByteSpan& ext) noexcept
    {
        if (!ext) return 0;

        // Extensions are short; cap to something reasonable.
        // Adjust if you expect longer (e.g., "somethingreallylong").
        static constexpr size_t kMaxExt = 16;
        if (ext.size() > kMaxExt)
            return nullptr;

        char buf[kMaxExt + 1];
        for (size_t i = 0; i < ext.size(); ++i)
            buf[i] = (char)to_lower(ext[i]);
        buf[ext.size()] = 0;

        return PSNameTable::INTERN(ByteSpan(buf));
    }

    static INLINE FramesSourceKind kindFromExtension(const ByteSpan& src) noexcept
    {
        ByteSpan ext = extractExtension(src);
        if (!ext)
            return FramesSourceKind::Blend2DImage; // default: cheap and safe

        const InternedKey ek = internLowerExt(ext);
        if (!ek)
            return FramesSourceKind::Blend2DImage;

        // Interned extension keys (lowercase)
        static const InternedKey kPng = PSNameTable::INTERN("png");
        static const InternedKey kApng = PSNameTable::INTERN("apng");
        static const InternedKey kGif = PSNameTable::INTERN("gif");
        static const InternedKey kJpg = PSNameTable::INTERN("jpg");
        static const InternedKey kJpeg = PSNameTable::INTERN("jpeg");
        static const InternedKey kBmp = PSNameTable::INTERN("bmp");
        static const InternedKey kWebp = PSNameTable::INTERN("webp");
        static const InternedKey kQoi = PSNameTable::INTERN("qoi");

        static const InternedKey kMp4 = PSNameTable::INTERN("mp4");
        static const InternedKey kMkv = PSNameTable::INTERN("mkv");
        static const InternedKey kMov = PSNameTable::INTERN("mov");
        static const InternedKey kAvi = PSNameTable::INTERN("avi");
        static const InternedKey kWebm = PSNameTable::INTERN("webm");
        static const InternedKey kTs = PSNameTable::INTERN("ts");
        static const InternedKey kM3u8 = PSNameTable::INTERN("m3u8");

        // Blend2D image formats (static or animated)
        if (ek == kPng || ek == kApng || ek == kGif || ek == kJpg || ek == kJpeg || ek == kBmp || ek == kWebp || ek == kQoi)
            return FramesSourceKind::Blend2DImage;

        // FFmpeg-ish containers / playlists
        if (ek == kMp4 || ek == kMkv || ek == kMov || ek == kAvi || ek == kWebm || ek == kTs || ek == kM3u8)
            return FramesSourceKind::FFmpeg;

        // Unknown extension: default policy
        return FramesSourceKind::Blend2DImage;
    }


    // Hybrid resolver:
    // - explicit scheme dispatch via InternedKey
    // - if no scheme and contains "://", treat as FFmpeg URL
    // - else infer by file extension
    static INLINE FramesSourceKind resolveFrameSourceKind(const ByteSpan &srcAuth, ByteSpan &rest) noexcept
    {
        ByteSpan src = chunk_trim(srcAuth, chrWspChars);
        if (!src)
            return FramesSourceKind::None;

        ByteSpan scheme{};

        if (parseFrameSourceScheme(src, scheme, rest))
        {
            const InternedKey sk = PSNameTable::INTERN(scheme);

            // Interned scheme keys
            static const InternedKey kMonitor = PSNameTable::INTERN("monitor");
            static const InternedKey kScreen = PSNameTable::INTERN("screen");
            static const InternedKey kFfmpeg = PSNameTable::INTERN("ffmpeg");
            static const InternedKey kFile = PSNameTable::INTERN("file");

            // Common URL schemes -> FFmpeg
            static const InternedKey kRtsp = PSNameTable::INTERN("rtsp");
            static const InternedKey kHttp = PSNameTable::INTERN("http");
            static const InternedKey kHttps = PSNameTable::INTERN("https");
            static const InternedKey kUdp = PSNameTable::INTERN("udp");
            static const InternedKey kTcp = PSNameTable::INTERN("tcp");
            static const InternedKey kSrt = PSNameTable::INTERN("srt");

            if (sk == kMonitor)
                return FramesSourceKind::MONITOR;

            if (sk == kFfmpeg)
                return FramesSourceKind::FFmpeg;

            if (sk == kRtsp || sk == kHttp || sk == kHttps || sk == kUdp || sk == kTcp || sk == kSrt)
                return FramesSourceKind::FFmpeg;

            // file:... => infer from the remainder as a path
            if (sk == kFile)
                return kindFromExtension(rest);

            // Unknown scheme: tighten policy (force author to fix)
            return FramesSourceKind::None;
        }

        // No explicit scheme: if it contains "://", treat as FFmpeg URL
        // (chunk_find returns a matched span; we don't need it, just the bool)
        ByteSpan found{};
        if (chunk_find(src, ByteSpan("://"), found))
            return FramesSourceKind::FFmpeg;

        // Otherwise infer from file extension
        return kindFromExtension(src);
    }
}



namespace waavs 
{
    //
    // DisplayCaptureElement
    // This is a base for video sequence frames.
    // It can be used standalone, like SVGImage, and also as
    // a paint source for fills and patterns (support getVariant())
    //
    struct SVGFramesElement : public SVGGraphicsElement
    {
        static void registerFactory() {
            registerSVGSingularNodeByName("frames",
                [](IAmGroot* groot, const XmlElement& elem) {
                    auto node = std::make_shared<SVGFramesElement>(groot);
                    node->loadFromXmlElement(elem, groot);
                    return node;
                });
        }


        // Stuff that must be resolved at bindToContext time
        // 
        // The intended destination
        SVGLengthValue fDimX{};
        SVGLengthValue fDimY{};
        SVGLengthValue fDimWidth{};
        SVGLengthValue fDimHeight{};

        // Stuff that can be resolved at fixupStyle time
        //ScreenSnapper fSnapper{};
        std::unique_ptr<IFrameSource> fSource;
        ByteSpan fSrcSpan{};
        FramesSourceKind fSourceKind{ FramesSourceKind::None };
        ByteSpan fSourceRest{}; // The portion of the src after the scheme (if any)

        // The parameters for the capture area
        // if the width or height are zero, then it
        // will capture nothing, and not draw
        
        SVGNumberOrPercent fCropXAuth{};
        SVGNumberOrPercent fCropYAuth{};
        SVGNumberOrPercent fCropWidthAuth{};
        SVGNumberOrPercent fCropHeightAuth{};


        bool fHasCapture = false;
        double fFrameRate{ 15.0 }; // default to 15 fps if not specified

        // Resolved location where we are drawing
        double fX{ 0 };
        double fY{ 0 };
        double fWidth{ 0 };
        double fHeight{ 0 };
        SpaceUnitsKind fDisplayUnits{ SpaceUnitsKind::SVG_SPACE_USER };

        BLImage fPatternImage{};
        BLPattern fPatternForVariant{};



        SVGFramesElement(IAmGroot*)
            :SVGGraphicsElement()
        {
        }


        const BLVar getVariant(IRenderSVG* ctx, IAmGroot* groot) noexcept override
        {
            if (needsBinding())
            {
                bindToContext(ctx, groot);
            }

            return fVar;
        }


        void fixupSelfStyleAttributes(IAmGroot*) override
        {
            // If no 'src' then don't do any setup
            fSrcSpan = getAttribute(svgattr::src());
            if (fSrcSpan.empty())
                return;


            // Selecting something to handle the source should
            // happen right here.  Parse the source attribute, and
            // Hold onto the information so that we can use it later
            // when we want to bind.
            fSourceKind = resolveFrameSourceKind(fSrcSpan, fSourceRest);
            if (fSourceKind == FramesSourceKind::None)
                return;

            parseNumberOrPercent(getAttribute(svgattr::cropX()), fCropXAuth);
            parseNumberOrPercent(getAttribute(svgattr::cropY()), fCropYAuth);
            parseNumberOrPercent(getAttribute(svgattr::cropWidth()), fCropWidthAuth);
            parseNumberOrPercent(getAttribute(svgattr::cropHeight()), fCropHeightAuth);

            
            ByteSpan fpsA;
            if (fAttributes.getValue(svgattr::frame_rate(), fpsA)) {
                double v{};
                if (parseNumber(fpsA, v))
                    fFrameRate = v;
            }

            ByteSpan duA;
            if (fAttributes.getValue(svgattr::displayUnits(), duA)) {
                uint32_t v{};
                if (getEnumValue(SVGSpaceUnits, duA, v))
                    fDisplayUnits = (SpaceUnitsKind)v;
            }

        }


        // Here's where we create the backend to deal with the source.
        void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            if (fSourceKind == FramesSourceKind::None)
                return;

            // 1) Select a backend to handle the source, and attempt to initialize it.
            fSource.reset();
            switch (fSourceKind)
            {
                case FramesSourceKind::None:
                break;
                
                case FramesSourceKind::MONITOR:
                    fSource = std::make_unique<ScreenSnapper>();
                break;
            
                case FramesSourceKind::Blend2DImage:
                    fSource = std::make_unique<Blend2DImageSource>();
                break;
            
                case FramesSourceKind::FFmpeg:
                    fSource = std::make_unique<FFmpegFrameSource>();
                break;
                
                default:
                break;
            }

            // Could not initialize?  Just return, we won't be drawing anything
            if (!fSource)
                return;

            fSource->setFrameRate(fFrameRate);

            // 2) Prepare descripto using srcRest + authored crop values
            // if no scheme, rest is the whole thing
            ByteSpan srcRest = fSourceRest ? fSourceRest : fSrcSpan; 

            FrameSourceDesc desc{};
            desc.src = srcRest;
            desc.cropX = fCropXAuth;
            desc.cropY = fCropYAuth;
            desc.cropW = fCropWidthAuth;
            desc.cropH = fCropHeightAuth;
            desc.maxFps = fFrameRate;

            //desc.hasCrop = fCropWidthAuth.isSet() && fCropHeightAuth.isSet();

            // if we define maxFps as an attribute, we'll pass that as well
            // desc.maxFps = fMaxFpsAuth.isSet() ? fMaxFpsAuth.value() : 0.0;
            
            // 3) Let the backend resolve crop and initialize itself.  
            // If it fails, just return and don't draw anything.
            if (!fSource->reset(desc))
                return;

            fHasCapture = true;


            // 4) Layout now uses the backend's pixel dimensions (post-crop)
            auto& px = fSource->pixels();
            if (px.width() <= 0 || px.height() <= 0)
            {
                fHasCapture = false;
                return;
            }


            // Default display size starts out the same as backend pixel dimensions
            double w = (double)px.width();
            double h = (double)px.height();

            BLMatrix2D pattMatrix = BLMatrix2D::make_identity();


            // displayUnits - Object bounding box, user space, or physical pixels (default)
            //if (fDisplayUnits == SpaceUnitsKind::SVG_SPACE_USER)
            //{
                // do nothing, we're already set
                // it should perhaps be relative to the viewport
                // as that's how other things operate
            //}
            //else 
            if (fDisplayUnits == SpaceUnitsKind::SVG_SPACE_OBJECT)
            {
                // We're drawing in objectFrame units
                // so get current object frame
                WGRectD objFrame = ctx->getObjectFrame();
                w = objFrame.w;
                h = objFrame.h;

                double sx = objFrame.w / (double)px.width();
                double sy = objFrame.h / (double)px.height();

                pattMatrix.translate(objFrame.x, objFrame.y);
                pattMatrix.scale(sx, sy);
            }


            double dpi = groot ? groot->dpi() : 96.0;
            const BLFont* fontOpt = groot ? &ctx->getFont() : nullptr;

            LengthResolveCtx wCtx = makeLengthCtxUser(w, 0, dpi, fontOpt);
            LengthResolveCtx hCtx = makeLengthCtxUser(h, 0, dpi, fontOpt);

            parseLengthValue(getAttribute(svgattr::x()), fDimX);
            parseLengthValue(getAttribute(svgattr::y()), fDimY);
            parseLengthValue(getAttribute(svgattr::width()), fDimWidth);
            parseLengthValue(getAttribute(svgattr::height()), fDimHeight);


            fX = resolveLengthOr(fDimX, wCtx, 0);
            fY = resolveLengthOr(fDimY, hCtx, 0);
            fWidth = resolveLengthOr(fDimWidth, wCtx, w);
            fHeight = resolveLengthOr(fDimHeight, hCtx, h);


            // 5) - Provide variant pattern
            // set a pattern transform
            // to accomodate where it's going to be displayed
            fPatternImage = blImageFromSurface(fSource->pixels());
            fPatternForVariant.set_image(fPatternImage);
            fPatternForVariant.set_transform(pattMatrix);

            fVar = fPatternForVariant;
        }


        void updateSelf(IAmGroot*) override
        {
            if (!fSource)
                return;

            fSource->update();
        }

        void drawSelf(IRenderSVG* ctx, IAmGroot*) override
        {
            if (!fHasCapture)
                return;

            auto& px = fSource->pixels();

            ctx->scaleImage(px, 0, 0, (int)px.width(), (int)px.height(), fX, fY, fWidth, fHeight);
            ctx->flush();
        }

    };
}
