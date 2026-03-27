#pragma once

#include "gcodeprogram.h"

namespace waavs
{
    struct GCodeProgramBuilder
    {
        GCodeProgram* fProgram{ nullptr };
        bool fEnded{ false };

        GCodeProgramBuilder() noexcept = default;

        explicit GCodeProgramBuilder(GCodeProgram& prog) noexcept
            : fProgram(&prog)
        {
        }

        void reset(GCodeProgram& prog) noexcept
        {
            fProgram = &prog;
            fEnded = false;
        }

        void clear() noexcept
        {
            if (!fProgram)
                return;

            fProgram->clear();
            fEnded = false;
        }

        bool good() const noexcept
        {
            return (fProgram != nullptr) && !fEnded;
        }

        bool emit0(GCodeOp op) noexcept
        {
            if (!fProgram || fEnded)
                return false;

            fProgram->ops.push_back((uint8_t)op);

            if (op == GOP_END)
                fEnded = true;

            return true;
        }

        bool emit1(GCodeOp op, double a0) noexcept
        {
            if (!fProgram || fEnded)
                return false;

            fProgram->ops.push_back((uint8_t)op);
            fProgram->args.push_back(a0);
            return true;
        }

        bool emit3(GCodeOp op, double a0, double a1, double a2) noexcept
        {
            if (!fProgram || fEnded)
                return false;

            fProgram->ops.push_back((uint8_t)op);
            fProgram->args.push_back(a0);
            fProgram->args.push_back(a1);
            fProgram->args.push_back(a2);
            return true;
        }

        bool emit6(GCodeOp op,
            double a0, double a1, double a2,
            double a3, double a4, double a5) noexcept
        {
            if (!fProgram || fEnded)
                return false;

            fProgram->ops.push_back((uint8_t)op);
            fProgram->args.push_back(a0);
            fProgram->args.push_back(a1);
            fProgram->args.push_back(a2);
            fProgram->args.push_back(a3);
            fProgram->args.push_back(a4);
            fProgram->args.push_back(a5);
            return true;
        }

        // Modal setup

        bool unitsMM() noexcept
        {
            return emit0(GOP_UNITS_MM);
        }

        bool unitsInch() noexcept
        {
            return emit0(GOP_UNITS_INCH);
        }

        bool absolute() noexcept
        {
            return emit0(GOP_ABSOLUTE);
        }

        bool incremental() noexcept
        {
            return emit0(GOP_INCREMENTAL);
        }

        bool planeXY() noexcept
        {
            return emit0(GOP_PLANE_XY);
        }

        bool planeZX() noexcept
        {
            return emit0(GOP_PLANE_ZX);
        }

        bool planeYZ() noexcept
        {
            return emit0(GOP_PLANE_YZ);
        }

        // Feed / spindle / coolant

        bool setFeed(double feed) noexcept
        {
            return emit1(GOP_SET_FEED, feed);
        }

        bool setSpindleRPM(double rpm) noexcept
        {
            return emit1(GOP_SET_SPINDLE_RPM, rpm);
        }

        bool spindleOnCW() noexcept
        {
            return emit0(GOP_SPINDLE_ON_CW);
        }

        bool spindleOnCCW() noexcept
        {
            return emit0(GOP_SPINDLE_ON_CCW);
        }

        bool spindleOff() noexcept
        {
            return emit0(GOP_SPINDLE_OFF);
        }

        bool coolantOn() noexcept
        {
            return emit0(GOP_COOLANT_ON);
        }

        bool coolantOff() noexcept
        {
            return emit0(GOP_COOLANT_OFF);
        }

        // Motion

        bool rapidTo(double x, double y, double z) noexcept
        {
            return emit3(GOP_RAPID, x, y, z);
        }

        bool lineTo(double x, double y, double z) noexcept
        {
            return emit3(GOP_LINEAR, x, y, z);
        }

        bool arcCWTo(double x, double y, double z,
            double i, double j, double k) noexcept
        {
            return emit6(GOP_ARC_CW, x, y, z, i, j, k);
        }

        bool arcCCWTo(double x, double y, double z,
            double i, double j, double k) noexcept
        {
            return emit6(GOP_ARC_CCW, x, y, z, i, j, k);
        }

        bool dwell(double seconds) noexcept
        {
            return emit1(GOP_DWELL, seconds);
        }

        // Program control

        bool optionalStop() noexcept
        {
            return emit0(GOP_OPTIONAL_STOP);
        }

        bool stop() noexcept
        {
            return emit0(GOP_STOP);
        }

        bool endProgram() noexcept
        {
            return emit0(GOP_END_PROGRAM);
        }

        bool end() noexcept
        {
            return emit0(GOP_END);
        }
    };
}
