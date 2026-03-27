#pragma once

#include "toolpathpolicy.h"

namespace waavs
{
    struct DragKnifeToolPathOptions
    {
        bool metric{ true };

        double knifeUpZ{ 5.0 };
        double knifeDownZ{ 0.0 };

        double travelFeed{ 3000.0 };
        double cutFeed{ 800.0 };

        bool closePathWithLine{ true };
    };


    // DragKnifeToolPathPolicy lowers 2D path geometry into a simple
    // drag-knife machine path.
    //
    // This policy does not perform blade-offset compensation.
    // It assumes any drag-knife-specific geometry compensation has
    // already been applied in the PathProgram domain.
    //
    // Behavior:
    //  - moveTo: raise knife if needed, then rapid in XY at knifeUpZ
    //  - first lineTo in a subpath: lower knife at current XY
    //  - subsequent lineTo: cut in XY at knifeDownZ
    //  - closePath: optional line back to subpath start
    //  - endPath: raise knife
    //  - endProgram: raise knife, emit M2, end stream
    //
    // This policy assumes the input path has already been flattened.
    struct DragKnifeToolPathPolicy : public ToolPathPolicy
    {
        DragKnifeToolPathOptions fOpt{};

        DragKnifeToolPathPolicy() noexcept = default;

        explicit DragKnifeToolPathPolicy(GCodeProgramBuilder& builder) noexcept
            : ToolPathPolicy(builder)
        {
        }

        DragKnifeToolPathPolicy(GCodeProgramBuilder& builder,
            const DragKnifeToolPathOptions& opt) noexcept
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

            if (!raiseKnifeIfNeeded())
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

            if (!raiseKnifeIfNeeded())
                return false;

            if (!fBuilder->setFeed(fOpt.travelFeed))
                return false;

            if (!fBuilder->rapidTo(x, y, fOpt.knifeUpZ))
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

            if (!ensureKnifeDown())
                return false;

            if (!fBuilder->setFeed(fOpt.cutFeed))
                return false;

            if (!fBuilder->lineTo(x, y, fOpt.knifeDownZ))
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
            return raiseKnifeIfNeeded();
        }

    private:
        bool ensureKnifeDown() noexcept
        {
            if (fState.fToolActive)
                return true;

            if (!fBuilder->setFeed(fOpt.cutFeed))
                return false;

            if (!fBuilder->lineTo(fState.fCurX, fState.fCurY, fOpt.knifeDownZ))
                return false;

            fState.fToolActive = true;
            return true;
        }

        bool raiseKnifeIfNeeded() noexcept
        {
            if (!fBuilder)
                return false;

            if (!fState.fToolActive)
                return true;

            if (!fBuilder->setFeed(fOpt.travelFeed))
                return false;

            if (!fBuilder->rapidTo(fState.fCurX, fState.fCurY, fOpt.knifeUpZ))
                return false;

            fState.fToolActive = false;
            return true;
        }
    };
}
