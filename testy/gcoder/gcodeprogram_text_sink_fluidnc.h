#pragma once

#include "gcodeprogram_text_sink_grbl.h"

namespace waavs
{
    // FluidNC is close enough to GRBL that the same sink is a
    // reasonable first implementation.
    struct GCodeFluidNCTextSink : public GCodeGRBLTextSink
    {
        GCodeFluidNCTextSink() noexcept = default;

        explicit GCodeFluidNCTextSink(int precision) noexcept
            : GCodeGRBLTextSink(precision)
        {
        }
    };
}

