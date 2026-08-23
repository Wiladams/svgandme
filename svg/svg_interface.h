#pragma once

#include "blend2d_connect.h"

namespace waavs
{
    struct IAmGroot;            // forward declaration
    struct IDrawGraphics;
    struct FilterProgramStream;


    //    struct AnimationProgram; // forward declaration
    //    class AnimationValueContext;

        // Base class of many things.  This is just to ensure a virtual destructor
        // and binding behavior.
        // BUGBUG - I'm not sure binding needs to be represented universally at this
        // level.  I think having it at the IViewable level might be ok.
        // Having the ability to call: getVariant, is still useful, because both
        // attributes, and elements can produce paint variants, so that's still good.
        //
    struct IBindToContext
    {
    protected:
        bool fNeedsBinding{ false };

    public:
        virtual ~IBindToContext() = default;

        bool needsBinding() const noexcept { return fNeedsBinding; }
        void setNeedsBinding(bool needsIt) noexcept { fNeedsBinding = needsIt; }

        virtual void bindToContext(IDrawGraphics*, IAmGroot*) noexcept = 0;

    };

    // IServePaint
    //
    // A paint server delivers paint in a given context.  Paint is
    // represented as a BLVar, so it can be anything from a solid
    // color to a pattern, or gradient.
    // This is the general interface for serving up paint.
    struct IServePaint
    {
        virtual ~IServePaint() = default;

        virtual const BLVar getVariant(IDrawGraphics*, IAmGroot*) noexcept = 0;
    };




}

namespace waavs
{
    // BUGBUG - It's unclear if there is any value to have
    // this base class any longer.  I don't think anything depends
    // on it.
    struct SVGObject : public IBindToContext
    {
    public:

        // default and copy constructor not allowed, let's see what breaks
        SVGObject() = default;

        // want to know when a copy or assignment is happening
        // so mark these as 'delete' for now so we can catch it
        SVGObject(const SVGObject& other) = delete;
        SVGObject& operator=(const SVGObject& other) = delete;

        virtual ~SVGObject() = default;

    };

}

