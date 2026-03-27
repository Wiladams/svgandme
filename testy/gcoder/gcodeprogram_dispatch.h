#pragma once

#include "gcodeprogram.h"

namespace waavs
{
    template <typename Sink>
    static INLINE bool gcodeprogram_dispatch(const GCodeProgram& prog, Sink& sink) noexcept
    {
        size_t ip = 0;
        size_t ap = 0;

        while (ip < prog.ops.size()) {
            const uint8_t op = prog.ops[ip++];

            if (op > GOP_END_PROGRAM)
                return false;

            if (op == GOP_END)
                return true;

            const size_t arity = kGCodeOpArity[op];
            if (ap + arity > prog.args.size())
                return false;

            const double* a = prog.args.data() + ap;
            ap += arity;

            switch ((GCodeOp)op) {
            case GOP_UNITS_MM:
                if (!sink.onUnitsMM())
                    return false;
                break;

            case GOP_UNITS_INCH:
                if (!sink.onUnitsInch())
                    return false;
                break;

            case GOP_ABSOLUTE:
                if (!sink.onAbsolute())
                    return false;
                break;

            case GOP_INCREMENTAL:
                if (!sink.onIncremental())
                    return false;
                break;

            case GOP_PLANE_XY:
                if (!sink.onPlaneXY())
                    return false;
                break;

            case GOP_PLANE_ZX:
                if (!sink.onPlaneZX())
                    return false;
                break;

            case GOP_PLANE_YZ:
                if (!sink.onPlaneYZ())
                    return false;
                break;

            case GOP_SET_FEED:
                if (!sink.onSetFeed(a[0]))
                    return false;
                break;

            case GOP_SET_SPINDLE_RPM:
                if (!sink.onSetSpindleRPM(a[0]))
                    return false;
                break;

            case GOP_SPINDLE_ON_CW:
                if (!sink.onSpindleOnCW())
                    return false;
                break;

            case GOP_SPINDLE_ON_CCW:
                if (!sink.onSpindleOnCCW())
                    return false;
                break;

            case GOP_SPINDLE_OFF:
                if (!sink.onSpindleOff())
                    return false;
                break;

            case GOP_COOLANT_ON:
                if (!sink.onCoolantOn())
                    return false;
                break;

            case GOP_COOLANT_OFF:
                if (!sink.onCoolantOff())
                    return false;
                break;

            case GOP_RAPID:
                if (!sink.onRapid(a[0], a[1], a[2]))
                    return false;
                break;

            case GOP_LINEAR:
                if (!sink.onLinear(a[0], a[1], a[2]))
                    return false;
                break;

            case GOP_ARC_CW:
                if (!sink.onArcCW(a[0], a[1], a[2], a[3], a[4], a[5]))
                    return false;
                break;

            case GOP_ARC_CCW:
                if (!sink.onArcCCW(a[0], a[1], a[2], a[3], a[4], a[5]))
                    return false;
                break;

            case GOP_DWELL:
                if (!sink.onDwell(a[0]))
                    return false;
                break;

            case GOP_OPTIONAL_STOP:
                if (!sink.onOptionalStop())
                    return false;
                break;

            case GOP_STOP:
                if (!sink.onStop())
                    return false;
                break;

            case GOP_END_PROGRAM:
                if (!sink.onEndProgram())
                    return false;
                break;

            default:
                return false;
            }
        }

        return true;
    }
}

