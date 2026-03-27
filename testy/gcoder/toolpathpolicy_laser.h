#pragma once

#include "toolpathpolicy.h"

namespace waavs
{
    struct LaserToolPathOptions
    {
        bool metric{ true };

        double workZ{ 0.0 };

        double travelFeed{ 3000.0 };
        double cutFeed{ 1200.0 };

        double power{ 1000.0 };
        bool useDynamicPowerMode{ false };

        bool closePathWithLine{ true };
        bool emitLaserPower{ true };
    };


    // LaserToolPathPolicy lowers 2D path geometry into a laser toolpath:
    //
    //  - moveTo: laser off, rapid in XY at workZ
    //  - first lineTo in a subpath: laser on, then cut move
    //  - subsequent lineTo: continue cut moves
    //  - closePath: optional line back to subpath start
    //  - endPath: laser off
    //  - endProgram: laser off, emit M2, end stream
    //
    // This policy assumes the input path has already been flattened.
    //
    // Notes:
    //  - power is carried through SET_SPINDLE_RPM as a pragmatic G-code IR proxy
    //  - M3 is constant-power mode, M4 is dynamic-power mode on controllers that support it
    struct LaserToolPathPolicy : public ToolPathPolicy
    {
        LaserToolPathOptions fOpt{};

        LaserToolPathPolicy() noexcept = default;

        explicit LaserToolPathPolicy(GCodeProgramBuilder& builder) noexcept
            : ToolPathPolicy(builder)
        {
        }

        LaserToolPathPolicy(GCodeProgramBuilder& builder,
            const LaserToolPathOptions& opt) noexcept
            : ToolPathPolicy(builder)
            , fOpt(opt)
        {
        }

        bool beginProgram() noexcept override
        {
            if (!fBuilder)
                return false;

            if (fOpt.metric) {
                if (!fBuilder->unitsMM())
                    return false;
            }
            else {
                if (!fBuilder->unitsInch())
                    return false;
            }

            if (!fBuilder->absolute())
                return false;

            if (!fBuilder->planeXY())
                return false;

            if (fOpt.emitLaserPower) {
                if (!fBuilder->setSpindleRPM(fOpt.power))
                    return false;
            }

            return true;
        }

        bool endProgram() noexcept override
        {
            if (!fBuilder)
                return false;

            if (!laserOffIfNeeded())
                return false;

            if (!fBuilder->endProgram())
                return false;

            return fBuilder->end();
        }

        bool beginPath() noexcept override
        {
            fState.resetPath();
            return true;
        }

        bool moveTo(double x, double y) noexcept override
        {
            if (!fBuilder)
                return false;

            if (!laserOffIfNeeded())
                return false;

            if (!fBuilder->setFeed(fOpt.travelFeed))
                return false;

            if (!fBuilder->rapidTo(x, y, fOpt.workZ))
                return false;

            fState.fCurX = x;
            fState.fCurY = y;
            fState.fStartX = x;
            fState.fStartY = y;
            fState.fHasCurrent = true;

            return true;
        }

        bool lineTo(double x, double y) noexcept override
        {
            if (!fBuilder || !fState.fHasCurrent)
                return false;

            if (!ensureLaserOn())
                return false;

            if (!fBuilder->setFeed(fOpt.cutFeed))
                return false;

            if (!fBuilder->lineTo(x, y, fOpt.workZ))
                return false;

            fState.fCurX = x;
            fState.fCurY = y;
            return true;
        }

        bool closePath() noexcept override
        {
            if (!fState.fHasCurrent)
                return false;

            if (!fOpt.closePathWithLine)
                return true;

            if (nearlyEqual(fState.fCurX, fState.fStartX) &&
                nearlyEqual(fState.fCurY, fState.fStartY))
                return true;

            return lineTo(fState.fStartX, fState.fStartY);
        }

        bool endPath() noexcept override
        {
            return laserOffIfNeeded();
        }

    private:
        bool ensureLaserOn() noexcept
        {
            if (fState.fToolActive)
                return true;

            if (fOpt.emitLaserPower) {
                if (!fBuilder->setSpindleRPM(fOpt.power))
                    return false;
            }

            if (fOpt.useDynamicPowerMode) {
                if (!fBuilder->spindleOnCCW())
                    return false;
            }
            else {
                if (!fBuilder->spindleOnCW())
                    return false;
            }

            fState.fToolActive = true;
            return true;
        }

        bool laserOffIfNeeded() noexcept
        {
            if (!fBuilder)
                return false;

            if (!fState.fToolActive)
                return true;

            if (!fBuilder->spindleOff())
                return false;

            fState.fToolActive = false;
            return true;
        }
    };
}

