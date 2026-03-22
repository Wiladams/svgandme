#pragma once

#include "bspan.h"
#include "surface.h"
#include "svgdatatypes.h"
#include "stopwatch.h"

namespace waavs 
{


    // “Source” can be a device name, filename, URL, etc.
    struct FrameSourceDesc {
        ByteSpan src{};     // device name, filename, URL, etc.

        // Optional crop in “source-space” (for capture devices / video cropping)
        SVGNumberOrPercent cropX{ 0 };
        SVGNumberOrPercent cropY{ 0 };
        SVGNumberOrPercent cropW{ 0 };
        SVGNumberOrPercent cropH{ 0 };
        bool hasCrop{ false };

        // Optional throttling (<= 0 no throttling)
        double maxFps{ 0.0 };
    };

    //
    // Interface for a source of pixel frames.
    // This could be a video file, a camera, a screen capture, etc.
    //
    struct IFrameSource {
        // Name of thing we're displaying
        InternedKey fSourceKey{ nullptr };

        WGRectI fCroppedRect{}; // Calculated crop rectangle in source-space (pixels)
        int64_t fCropX = 0;
        int64_t fCropY = 0;
        int64_t fCropWidth = 0;
        int64_t fCropHeight = 0;

        // Capture throttling
        StopWatch fTimer;
        double fFrameRate{ 15.0 };
        double fMinInterval = 0.0;
        double fLastCaptureTime = 0;

        virtual ~IFrameSource() = default;

        virtual void setFrameRate(double fps) noexcept
        {
            fFrameRate = fps;
            fMinInterval = (fps > 0.0) ? (1.0 / fps) : 0.0;
        }
        double frameRate() const noexcept { return fFrameRate; }

        virtual bool reset(const FrameSourceDesc& desc) noexcept = 0;
        virtual bool update() noexcept = 0;

        virtual Surface& pixels() noexcept = 0;
        virtual const Surface& pixels() const noexcept = 0;

    };


} // namespace waavs
