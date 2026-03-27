#pragma once

#include "gcodeprogram_text_sink_base.h"

namespace waavs
{
    // Debug sink emits one straightforward line per op.
    // It does not try to suppress repeated values or compact output.
    struct GCodeDebugTextSink : public GCodeTextSinkBase
    {
        GCodeDebugTextSink() noexcept = default;

        explicit GCodeDebugTextSink(int precision) noexcept
            : GCodeTextSinkBase(precision)
        {
        }
    };
}
