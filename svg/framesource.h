#pragma once

//#include "blend2d.h"
#include "bspan.h"
#include "pixelaccessor.h"
#include "svgdatatypes.h"


namespace waavs 
{
    static INLINE double bindNumberOrPercent(const SVGNumberOrPercent& norp, const double range, const double fallback) noexcept
    {
        if (norp.isSet())
        {
            if (norp.isPercent())
                return norp.calculatedValue() * range;

            return norp.calculatedValue();
        }

        return fallback;
    }

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


    struct IFrameSource {
        virtual ~IFrameSource() = default;

        virtual bool reset(const FrameSourceDesc& desc) noexcept = 0;
        virtual bool update() noexcept = 0;

        virtual PixelArray& pixels() noexcept = 0;
        virtual const PixelArray& pixels() const noexcept = 0;

    };

} // namespace waavs
