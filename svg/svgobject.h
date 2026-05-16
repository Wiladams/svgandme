#pragma once

#include <blend2d/blend2d.h>
#include "blend2d_connect.h"

namespace waavs
{
    struct IAmGroot;            // forward declaration
    struct IRenderSVG;
//    struct AnimationProgram; // forward declaration
//    class AnimationValueContext;
//    struct CSSStyleSheet;

    // Base class of many things.  This is just to ensure a virtual destructor
    // and binding behavior.
    // BUGBUG - I'm not sure binding needs to be represented universally at this
    // level.  I think having it at the IViewable level might be ok.
    // Having the ability to call: getVariant, is still useful, because both
    // attributes, and elements can produce paint variants, so that's still good.
    //
    struct SVGObject
    {
    protected:
        bool fNeedsBinding{ false };

    public:

        // default and copy constructor not allowed, let's see what breaks
        SVGObject() = default;

        // want to know when a copy or assignment is happening
        // so mark these as 'delete' for now so we can catch it
        SVGObject(const SVGObject& other) = delete;
        SVGObject& operator=(const SVGObject& other) = delete;

        virtual ~SVGObject() = default;


        bool needsBinding() const noexcept { return fNeedsBinding; }
        void setNeedsBinding(bool needsIt) noexcept { fNeedsBinding = needsIt; }


        virtual void bindToContext(IRenderSVG*, IAmGroot*) noexcept = 0;

    };

    struct IServePaint
    {
        virtual ~IServePaint() = default;

        virtual const BLVar getVariant(IRenderSVG*, IAmGroot*) noexcept = 0;
    };
}
