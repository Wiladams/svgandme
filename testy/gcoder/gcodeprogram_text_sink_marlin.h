#pragma once

#include "gcodeprogram_text_sink_base.h"

namespace waavs
{
    // Marlin-oriented sink:
    //  - keeps feed deferred and appends it to motion commands
    //  - keeps spindle/power deferred and emits with M3/M4 when present
    //  - uses changed-axis emission for more compact motion lines
    //  - dwell uses P by default in this first implementation
    struct GCodeMarlinTextSink : public GCodeTextSinkBase
    {
        GCodeMarlinTextSink() noexcept = default;

        explicit GCodeMarlinTextSink(int precision) noexcept
            : GCodeTextSinkBase(precision)
        {
        }

        bool onSetFeed(double v) noexcept override
        {
            fFeed = v;
            fHasFeed = true;
            return true;
        }

        bool onSetSpindleRPM(double v) noexcept override
        {
            fSpindleRPM = v;
            fHasSpindleRPM = true;
            return true;
        }

        bool onSpindleOnCW() noexcept override
        {
            beginLine("M3");
            if (fHasSpindleRPM)
                emitWord('S', fSpindleRPM);
            endLine();
            return true;
        }

        bool onSpindleOnCCW() noexcept override
        {
            beginLine("M4");
            if (fHasSpindleRPM)
                emitWord('S', fSpindleRPM);
            endLine();
            return true;
        }

        bool onRapid(double x, double y, double z) noexcept override
        {
            beginLine("G0");
            emitXYZChanged(x, y, z);
            if (fHasFeed)
                emitWord('F', fFeed);
            endLine();
            rememberXYZ(x, y, z);
            return true;
        }

        bool onLinear(double x, double y, double z) noexcept override
        {
            beginLine("G1");
            emitXYZChanged(x, y, z);
            if (fHasFeed)
                emitWord('F', fFeed);
            endLine();
            rememberXYZ(x, y, z);
            return true;
        }

        bool onArcCW(double x, double y, double z,
            double i, double j, double k) noexcept override
        {
            beginLine("G2");
            emitXYZChanged(x, y, z);
            emitWord('I', i);
            emitWord('J', j);
            emitWord('K', k);
            if (fHasFeed)
                emitWord('F', fFeed);
            endLine();
            rememberXYZ(x, y, z);
            return true;
        }

        bool onArcCCW(double x, double y, double z,
            double i, double j, double k) noexcept override
        {
            beginLine("G3");
            emitXYZChanged(x, y, z);
            emitWord('I', i);
            emitWord('J', j);
            emitWord('K', k);
            if (fHasFeed)
                emitWord('F', fFeed);
            endLine();
            rememberXYZ(x, y, z);
            return true;
        }

        bool onDwell(double seconds) noexcept override
        {
            beginLine("G4");
            emitWord('P', seconds);
            endLine();
            return true;
        }
    };
}

