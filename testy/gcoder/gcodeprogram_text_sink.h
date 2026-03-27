#pragma once

#include <string>
#include <sstream>
#include <iomanip>

#include "gcodeprogram_dispatch.h"

namespace waavs
{
    struct GCodeTextSink
    {
        std::ostringstream fOut;
        int fPrecision{ 3 };

        GCodeTextSink() noexcept
        {
            fOut << std::fixed << std::setprecision(fPrecision);
        }

        explicit GCodeTextSink(int precision) noexcept
            : fPrecision(precision)
        {
            fOut << std::fixed << std::setprecision(fPrecision);
        }

        void reset() noexcept
        {
            fOut.str("");
            fOut.clear();
            fOut << std::fixed << std::setprecision(fPrecision);
        }

        std::string str() const
        {
            return fOut.str();
        }

        bool writeProgram(const GCodeProgram& prog) noexcept
        {
            reset();
            return gcodeprogram_dispatch(prog, *this);
        }

        bool onUnitsMM() noexcept
        {
            fOut << "G21\n";
            return true;
        }

        bool onUnitsInch() noexcept
        {
            fOut << "G20\n";
            return true;
        }

        bool onAbsolute() noexcept
        {
            fOut << "G90\n";
            return true;
        }

        bool onIncremental() noexcept
        {
            fOut << "G91\n";
            return true;
        }

        bool onPlaneXY() noexcept
        {
            fOut << "G17\n";
            return true;
        }

        bool onPlaneZX() noexcept
        {
            fOut << "G18\n";
            return true;
        }

        bool onPlaneYZ() noexcept
        {
            fOut << "G19\n";
            return true;
        }

        bool onSetFeed(double v) noexcept
        {
            fOut << "F" << v << "\n";
            return true;
        }

        bool onSetSpindleRPM(double v) noexcept
        {
            fOut << "S" << v << "\n";
            return true;
        }

        bool onSpindleOnCW() noexcept
        {
            fOut << "M3\n";
            return true;
        }

        bool onSpindleOnCCW() noexcept
        {
            fOut << "M4\n";
            return true;
        }

        bool onSpindleOff() noexcept
        {
            fOut << "M5\n";
            return true;
        }

        bool onCoolantOn() noexcept
        {
            fOut << "M8\n";
            return true;
        }

        bool onCoolantOff() noexcept
        {
            fOut << "M9\n";
            return true;
        }

        bool onRapid(double x, double y, double z) noexcept
        {
            fOut << "G0";
            emitXYZ_(x, y, z);
            fOut << "\n";
            return true;
        }

        bool onLinear(double x, double y, double z) noexcept
        {
            fOut << "G1";
            emitXYZ_(x, y, z);
            fOut << "\n";
            return true;
        }

        bool onArcCW(double x, double y, double z,
            double i, double j, double k) noexcept
        {
            fOut << "G2";
            emitXYZ_(x, y, z);
            fOut << " I" << i
                << " J" << j
                << " K" << k;
            fOut << "\n";
            return true;
        }

        bool onArcCCW(double x, double y, double z,
            double i, double j, double k) noexcept
        {
            fOut << "G3";
            emitXYZ_(x, y, z);
            fOut << " I" << i
                << " J" << j
                << " K" << k;
            fOut << "\n";
            return true;
        }

        bool onDwell(double seconds) noexcept
        {
            fOut << "G4 P" << seconds << "\n";
            return true;
        }

        bool onOptionalStop() noexcept
        {
            fOut << "M1\n";
            return true;
        }

        bool onStop() noexcept
        {
            fOut << "M0\n";
            return true;
        }

        bool onEndProgram() noexcept
        {
            fOut << "M2\n";
            return true;
        }

    private:
        void emitXYZ_(double x, double y, double z) noexcept
        {
            fOut << " X" << x
                << " Y" << y
                << " Z" << z;
        }
    };
}

