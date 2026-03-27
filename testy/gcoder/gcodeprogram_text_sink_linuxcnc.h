#pragma once

#include "gcodeprogram_text_sink_base.h"

namespace waavs
{
    // LinuxCNC-oriented sink:
    //  - verbose and explicit
    //  - emits feed as separate line when set
    //  - emits spindle RPM as separate line when set
    //  - keeps motion lines straightforward
    //
    // This is intentionally close to the debug sink, but kept separate
    // so LinuxCNC-specific behavior can diverge later without disturbing
    // the generic debug form.
    struct GCodeLinuxCNCTextSink : public GCodeTextSinkBase
    {
        GCodeLinuxCNCTextSink() noexcept = default;

        explicit GCodeLinuxCNCTextSink(int precision) noexcept
            : GCodeTextSinkBase(precision)
        {
        }
    };
}

