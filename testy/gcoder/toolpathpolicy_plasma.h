#pragma once

#include "toolpathpolicy.h"

namespace waavs
{
    struct PlasmaToolPathOptions
    {
        bool metric{ true };

        double safeZ{ 5.0 };
        double pierceZ{ 1.5 };
        double cutZ{ 1.0 };

        double travelFeed{ 3000.0 };
        double cutFeed{ 1000.0 };

        double pierceDelaySeconds{ 0.5 };

        bool closePathWithLine{ true };
        bool emitTorchPower{ false };
        double torchPower{ 1000.0 };
    };


    // PlasmaToolPathPolicy lowers 2D path geometry into a simple plasma path:
    //
    //  - moveTo: torch off, retract if needed, rapid in XY at safeZ
    //  - first lineTo in a subpath:
    //      rapid / move to pierceZ at current XY
    //      torch on
    //      dwell for pierce delay
    //      move to cutZ
    //  - subsequent lineTo: cut in XY at cutZ
    //  - closePath: optional line back to subpath start
    //  - endPath: torch off, retract to safeZ
    //  - endProgram: torch off, retract, emit M2, end stream
    //
    // This policy assumes the input path has already been flattened.
    //
    // Notes:
    //  - torch on/off is represented through spindle on/off ops as a pragmatic G-code IR proxy
    struct PlasmaToolPathPolicy : public ToolPathPolicy
    {
        PlasmaToolPathOptions fOpt{};

        PlasmaToolPathPolicy() noexcept = default;

        explicit PlasmaToolPathPolicy(GCodeProgramBuilder& builder) noexcept
            : ToolPathPolicy(builder)
        {
        }

        PlasmaToolPathPolicy(GCodeProgramBuilder& builder,
            const PlasmaToolPathOptions& opt) noexcept
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

            if (fOpt.emitTorchPower) {
                if (!fBuilder->setSpindleRPM(fOpt.torchPower))
                    return false;
            }

            return true;
        }

        bool endProgram() noexcept override
        {
            if (!fBuilder)
                return false;

            if (!torchOffAndRetractIfNeeded())
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

            if (!torchOffAndRetractIfNeeded())
                return false;

            if (!fBuilder->setFeed(fOpt.travelFeed))
                return false;

            if (!fBuilder->rapidTo(x, y, fOpt.safeZ))
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

            if (!ensureTorchCutting())
                return false;

            if (!fBuilder->setFeed(fOpt.cutFeed))
                return false;

            if (!fBuilder->lineTo(x, y, fOpt.cutZ))
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
            return torchOffAndRetractIfNeeded();
        }

    private:
        bool ensureTorchCutting() noexcept
        {
            if (fState.fToolActive)
                return true;

            if (fBuilder == nullptr)
                return false;

            if (!fBuilder->setFeed(fOpt.travelFeed))
                return false;

            if (!fBuilder->lineTo(fState.fCurX, fState.fCurY, fOpt.pierceZ))
                return false;

            if (fOpt.emitTorchPower) {
                if (!fBuilder->setSpindleRPM(fOpt.torchPower))
                    return false;
            }

            if (!fBuilder->spindleOnCW())
                return false;

            if (fOpt.pierceDelaySeconds > 0.0) {
                if (!fBuilder->dwell(fOpt.pierceDelaySeconds))
                    return false;
            }

            if (!fBuilder->lineTo(fState.fCurX, fState.fCurY, fOpt.cutZ))
                return false;

            fState.fToolActive = true;
            return true;
        }

        bool torchOffAndRetractIfNeeded() noexcept
        {
            if (!fBuilder)
                return false;

            if (!fState.fToolActive)
                return true;

            if (!fBuilder->spindleOff())
                return false;

            if (!fBuilder->setFeed(fOpt.travelFeed))
                return false;

            if (!fBuilder->rapidTo(fState.fCurX, fState.fCurY, fOpt.safeZ))
                return false;

            fState.fToolActive = false;
            return true;
        }
    };
}

