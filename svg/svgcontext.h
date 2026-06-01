// svgcontext.h

#pragma once

#include "wggeometry.h"
#include "surface.h"
#include "svgobject.h"
#include "svgenums.h"


namespace waavs
{
    struct SVGStyleState
    {
        IServePaint* fill = nullptr;
        IServePaint* stroke = nullptr;

        double fillOpacity = 1.0;
        double strokeOpacity = 1.0;
        double opacity = 1.0;

        StrokeState strokeState{};
        FillState fillState{};
        FontState fontState{};

        WGCompositeOp compositeOp = WG_COMP_SRC_OVER;
        uint32_t paintOrder = SVG_PAINT_ORDER_NORMAL;
    };

    struct SVGSemanticFrame
    {
        WGMatrix3x3 userToCanvas = WGMatrix3x3::makeIdentity();

        WGRectD viewportUS{};
        WGRectD objectBBoxUS{};

        // Current SVG element coordinate context.
        SpaceUnitsKind currentUnits = SVG_SPACE_USER;
    };

    struct SVGSemanticFrame
    {
        WGMatrix3x3 userToCanvas = WGMatrix3x3::makeIdentity();

        WGRectD viewportUS{};
        WGRectD objectBBoxUS{};

        // Current SVG element coordinate context.
        SpaceUnitsKind currentUnits = SVG_SPACE_USER;
    };

    struct SVGTargetFrame
    {
        Surface target{};

        // Maps semantic SVG user space into the currently attached target.
        WGMatrix3x3 userToTarget = WGMatrix3x3::makeIdentity();

        WGRectD targetRectUS{};
        WGRectI targetRectPX{};

        // Real clip object, not just rectangle.
        IClipSource* clip = nullptr;
    };

    struct ISVGContext
    {
        virtual SVGStyleState& style() noexcept = 0;
        virtual SVGSemanticFrame& semantic() noexcept = 0;
        virtual SVGTargetFrame& target() noexcept = 0;

        virtual void pushStyle() = 0;
        virtual void popStyle() = 0;

        virtual void pushSemantic() = 0;
        virtual void popSemantic() = 0;

        virtual void pushTarget() = 0;
        virtual void popTarget() = 0;

        virtual void applyCurrentState() = 0;
    };
}