#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

#include "gcodeprogram_dispatch.h"

namespace waavs
{
    struct GCodeTextSinkBase
    {
        std::ostringstream fOut;
        int fPrecision{ 3 };

        bool fHasLastXYZ{ false };
        double fLastX{ 0.0 };
        double fLastY{ 0.0 };
        double fLastZ{ 0.0 };

        bool fHasFeed{ false };
        double fFeed{ 0.0 };

        bool fHasSpindleRPM{ false };
        double fSpindleRPM{ 0.0 };

        GCodeTextSinkBase() noexcept
        {
            configureStream();
        }

        explicit GCodeTextSinkBase(int precision) noexcept
            : fPrecision(precision)
        {
            configureStream();
        }

        virtual ~GCodeTextSinkBase() = default;

        void reset() noexcept
        {
            fOut.str("");
            fOut.clear();

            fHasLastXYZ = false;
            fLastX = 0.0;
            fLastY = 0.0;
            fLastZ = 0.0;

            fHasFeed = false;
            fFeed = 0.0;

            fHasSpindleRPM = false;
            fSpindleRPM = 0.0;

            configureStream();
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

        // Default modal/state handlers.
        // Derived sinks can override only what they need.

        virtual bool onUnitsMM() noexcept
        {
            line0("G21");
            return true;
        }

        virtual bool onUnitsInch() noexcept
        {
            line0("G20");
            return true;
        }

        virtual bool onAbsolute() noexcept
        {
            line0("G90");
            return true;
        }

        virtual bool onIncremental() noexcept
        {
            line0("G91");
            return true;
        }

        virtual bool onPlaneXY() noexcept
        {
            line0("G17");
            return true;
        }

        virtual bool onPlaneZX() noexcept
        {
            line0("G18");
            return true;
        }

        virtual bool onPlaneYZ() noexcept
        {
            line0("G19");
            return true;
        }

        virtual bool onSetFeed(double v) noexcept
        {
            fFeed = v;
            fHasFeed = true;
            line1("F", v);
            return true;
        }

        virtual bool onSetSpindleRPM(double v) noexcept
        {
            fSpindleRPM = v;
            fHasSpindleRPM = true;
            line1("S", v);
            return true;
        }

        virtual bool onSpindleOnCW() noexcept
        {
            line0("M3");
            return true;
        }

        virtual bool onSpindleOnCCW() noexcept
        {
            line0("M4");
            return true;
        }

        virtual bool onSpindleOff() noexcept
        {
            line0("M5");
            return true;
        }

        virtual bool onCoolantOn() noexcept
        {
            line0("M8");
            return true;
        }

        virtual bool onCoolantOff() noexcept
        {
            line0("M9");
            return true;
        }

        virtual bool onRapid(double x, double y, double z) noexcept
        {
            beginLine("G0");
            emitXYZAll(x, y, z);
            endLine();
            rememberXYZ(x, y, z);
            return true;
        }

        virtual bool onLinear(double x, double y, double z) noexcept
        {
            beginLine("G1");
            emitXYZAll(x, y, z);
            endLine();
            rememberXYZ(x, y, z);
            return true;
        }

        virtual bool onArcCW(double x, double y, double z,
            double i, double j, double k) noexcept
        {
            beginLine("G2");
            emitXYZAll(x, y, z);
            emitWord('I', i);
            emitWord('J', j);
            emitWord('K', k);
            endLine();
            rememberXYZ(x, y, z);
            return true;
        }

        virtual bool onArcCCW(double x, double y, double z,
            double i, double j, double k) noexcept
        {
            beginLine("G3");
            emitXYZAll(x, y, z);
            emitWord('I', i);
            emitWord('J', j);
            emitWord('K', k);
            endLine();
            rememberXYZ(x, y, z);
            return true;
        }

        virtual bool onDwell(double seconds) noexcept
        {
            beginLine("G4");
            emitWord('P', seconds);
            endLine();
            return true;
        }

        virtual bool onOptionalStop() noexcept
        {
            line0("M1");
            return true;
        }

        virtual bool onStop() noexcept
        {
            line0("M0");
            return true;
        }

        virtual bool onEndProgram() noexcept
        {
            line0("M2");
            return true;
        }

    protected:
        void configureStream() noexcept
        {
            fOut << std::fixed << std::setprecision(fPrecision);
        }

        static bool nearlyEqual(double a, double b) noexcept
        {
            const double eps = 1e-9;
            return std::abs(a - b) <= eps;
        }

        void beginLine(const char* word) noexcept
        {
            fOut << word;
        }

        void endLine() noexcept
        {
            fOut << "\n";
        }

        void line0(const char* word) noexcept
        {
            beginLine(word);
            endLine();
        }

        void line1(const char* word, double v) noexcept
        {
            beginLine(word);
            fOut << v;
            endLine();
        }

        void emitWord(char prefix, double v) noexcept
        {
            fOut << " " << prefix << v;
        }

        void emitXYZAll(double x, double y, double z) noexcept
        {
            emitWord('X', x);
            emitWord('Y', y);
            emitWord('Z', z);
        }

        void emitXYZChanged(double x, double y, double z) noexcept
        {
            if (!fHasLastXYZ || !nearlyEqual(x, fLastX))
                emitWord('X', x);

            if (!fHasLastXYZ || !nearlyEqual(y, fLastY))
                emitWord('Y', y);

            if (!fHasLastXYZ || !nearlyEqual(z, fLastZ))
                emitWord('Z', z);
        }

        void rememberXYZ(double x, double y, double z) noexcept
        {
            fLastX = x;
            fLastY = y;
            fLastZ = z;
            fHasLastXYZ = true;
        }
    };
}

