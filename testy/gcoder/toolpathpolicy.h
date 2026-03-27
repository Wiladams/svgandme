#pragma once

#include <cmath>

#include "gcodeprogram_builder.h"

namespace waavs
{
    struct ToolPathState
    {
        bool fHasCurrent{ false };
        bool fToolActive{ false };

        double fCurX{ 0.0 };
        double fCurY{ 0.0 };

        double fStartX{ 0.0 };
        double fStartY{ 0.0 };

        void resetPath() noexcept
        {
            fHasCurrent = false;
            fToolActive = false;
            fCurX = 0.0;
            fCurY = 0.0;
            fStartX = 0.0;
            fStartY = 0.0;
        }
    };


    // ToolPathPolicy is the bridge between geometry and machine motion.
    //
    // PathProgram describes geometry.
    // ToolPathPolicy decides what that geometry means operationally.
    // GCodeProgram stores the resulting machine-motion IR.
    //
    // Typical responsibilities of a policy:
    //  - setup modal state at program start
    //  - decide what moveTo means
    //  - decide what lineTo means
    //  - decide how closePath is handled
    //  - manage tool-active state
    //  - emit shutdown / cleanup at program end
    //
    // First implementations can assume flattened geometry, so only:
    //  - moveTo
    //  - lineTo
    //  - closePath
    // are required.
    struct ToolPathPolicy
    {
        GCodeProgramBuilder* fBuilder{ nullptr };
        ToolPathState fState{};

        ToolPathPolicy() noexcept = default;

        explicit ToolPathPolicy(GCodeProgramBuilder& builder) noexcept
            : fBuilder(&builder)
        {
        }

        virtual ~ToolPathPolicy() = default;

        void resetBuilder(GCodeProgramBuilder& builder) noexcept
        {
            fBuilder = &builder;
            fState.resetPath();
        }

        virtual bool beginProgram() noexcept = 0;
        virtual bool endProgram() noexcept = 0;

        virtual bool beginPath() noexcept
        {
            fState.resetPath();
            return true;
        }

        virtual bool moveTo(double x, double y) noexcept = 0;
        virtual bool lineTo(double x, double y) noexcept = 0;
        virtual bool closePath() noexcept = 0;

        virtual bool endPath() noexcept
        {
            return true;
        }

    protected:
        static bool nearlyEqual(double a, double b) noexcept
        {
            const double eps = 1e-9;
            return std::abs(a - b) <= eps;
        }
    };
}


