#pragma once

#include "toolpathpolicy.h"

namespace waavs
{
    struct PenToolPathOptions
    {
        bool metric{ true };

        double penUpZ{ 5.0 };
        double penDownZ{ 0.0 };

        double travelFeed{ 3000.0 };
        double drawFeed{ 800.0 };

        bool closePathWithLine{ true };
    };


    // PenToolPathPolicy lowers 2D path geometry into a simple pen plotter path:
    //
    //  - moveTo: raise pen if needed, then rapid in XY at penUpZ
    //  - first lineTo in a subpath: lower pen at current XY
    //  - subsequent lineTo: draw in XY at penDownZ
    //  - closePath: optional line back to subpath start
    //  - endPath: raise pen
    //  - endProgram: raise pen, emit M2, end stream
    //
    // This policy assumes the input path has already been flattened.
    struct PenToolPathPolicy : public ToolPathPolicy
    {
        PenToolPathOptions fOpt{};

        PenToolPathPolicy() noexcept = default;

        explicit PenToolPathPolicy(GCodeProgramBuilder& builder) noexcept
            : ToolPathPolicy(builder)
        {
        }

        PenToolPathPolicy(GCodeProgramBuilder& builder,
            const PenToolPathOptions& opt) noexcept
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

            return true;
        }

        bool endProgram() noexcept override
        {
            if (!fBuilder)
                return false;

            if (!raisePenIfNeeded())
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

            if (!raisePenIfNeeded())
                return false;

            if (!fBuilder->setFeed(fOpt.travelFeed))
                return false;

            if (!fBuilder->rapidTo(x, y, fOpt.penUpZ))
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

            if (!ensurePenDown())
                return false;

            if (!fBuilder->setFeed(fOpt.drawFeed))
                return false;

            if (!fBuilder->lineTo(x, y, fOpt.penDownZ))
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
            return raisePenIfNeeded();
        }

    private:
        bool ensurePenDown() noexcept
        {
            if (fState.fToolActive)
                return true;

            if (!fBuilder->setFeed(fOpt.drawFeed))
                return false;

            if (!fBuilder->lineTo(fState.fCurX, fState.fCurY, fOpt.penDownZ))
                return false;

            fState.fToolActive = true;
            return true;
        }

        bool raisePenIfNeeded() noexcept
        {
            if (!fBuilder)
                return false;

            if (!fState.fToolActive)
                return true;

            if (!fBuilder->setFeed(fOpt.travelFeed))
                return false;

            if (!fBuilder->rapidTo(fState.fCurX, fState.fCurY, fOpt.penUpZ))
                return false;

            fState.fToolActive = false;
            return true;
        }
    };
}

