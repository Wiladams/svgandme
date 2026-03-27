#pragma once

#include "toolpathpolicy.h"

namespace waavs
{
    struct RouterToolPathOptions
    {
        bool metric{ true };

        double safeZ{ 5.0 };
        double cutZ{ 0.0 };

        double plungeFeed{ 200.0 };
        double cutFeed{ 800.0 };

        double spindleRPM{ 10000.0 };
        double spindleDwellSeconds{ 1.0 };

        bool emitSpindle{ true };
        bool emitCoolant{ false };
        bool closePathWithLine{ true };
    };


    // RouterToolPathPolicy lowers 2D path geometry into a simple
    // router-style 2.5D toolpath:
    //
    //  - moveTo: retract if needed, then rapid in XY at safe Z
    //  - first lineTo in a subpath: plunge at current XY to cut Z
    //  - subsequent lineTo: cut in XY at cut Z
    //  - closePath: optional line back to subpath start
    //  - endPath: retract to safe Z
    //  - endProgram: retract, stop coolant/spindle, emit M2, end stream
    //
    // This policy assumes the input path has already been flattened.
    struct RouterToolPathPolicy : public ToolPathPolicy
    {
        RouterToolPathOptions fOpt{};

        RouterToolPathPolicy() noexcept = default;

        explicit RouterToolPathPolicy(GCodeProgramBuilder& builder) noexcept
            : ToolPathPolicy(builder)
        {
        }

        RouterToolPathPolicy(GCodeProgramBuilder& builder,
            const RouterToolPathOptions& opt) noexcept
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

            if (fOpt.emitSpindle) {
                if (!fBuilder->setSpindleRPM(fOpt.spindleRPM))
                    return false;

                if (!fBuilder->spindleOnCW())
                    return false;

                if (fOpt.spindleDwellSeconds > 0.0) {
                    if (!fBuilder->dwell(fOpt.spindleDwellSeconds))
                        return false;
                }
            }

            if (fOpt.emitCoolant) {
                if (!fBuilder->coolantOn())
                    return false;
            }

            return true;
        }

        bool endProgram() noexcept override
        {
            if (!fBuilder)
                return false;

            if (!retractIfNeeded())
                return false;

            if (fOpt.emitCoolant) {
                if (!fBuilder->coolantOff())
                    return false;
            }

            if (fOpt.emitSpindle) {
                if (!fBuilder->spindleOff())
                    return false;
            }

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

            if (!retractIfNeeded())
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

            if (!ensureCutting())
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
            return retractIfNeeded();
        }

    private:
        bool ensureCutting() noexcept
        {
            if (fState.fToolActive)
                return true;

            if (!fBuilder->setFeed(fOpt.plungeFeed))
                return false;

            if (!fBuilder->lineTo(fState.fCurX, fState.fCurY, fOpt.cutZ))
                return false;

            fState.fToolActive = true;
            return true;
        }

        bool retractIfNeeded() noexcept
        {
            if (!fBuilder)
                return false;

            if (!fState.fToolActive)
                return true;

            if (!fBuilder->rapidTo(fState.fCurX, fState.fCurY, fOpt.safeZ))
                return false;

            fState.fToolActive = false;
            return true;
        }
    };
}

