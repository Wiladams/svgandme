#pragma once

#include "svgstructuretypes.h"
#include "svgportal.h"

namespace waavs {
    //
    // SVGContainer
    //
    // A base class element for all SVG elements that can contain other elements.
    // The container helps facilitate the proper creation of a coordinate space.
    // This should be applied to the following containers that support 'viewBox'
    // svg
    // symbol
    // marker
    // pattern
    // view
    //

    struct SVGContainer : public SVGGraphicsElement
    {
        SVGPortal fPortal;


        SVGContainer()
            : SVGGraphicsElement()
        {
            needsBinding(true);
        }

        BLRect frame() const override
        {
            return fPortal.getBBox();
        }

        BLRect getBBox() const override
        {
            return fPortal.getBBox();
        }

        virtual void fixupSelfStyleAttributes(IRenderSVG*, IAmGroot*)
        {
            // printf("fixupSelfStyleAttributes\n");
            fPortal.loadFromAttributes(fAttributes);

        }
        
        //void fixupStyleAttributes(IRenderSVG* ctx, IAmGroot* groot) override
        //{
        //    SVGGraphicsElement::fixupStyleAttributes(ctx, groot);

            // First, allow all the other attributes to be fixed up
            // Then, load the portal attributes
        //    fViewport.loadFromAttributes(fAttributes);
        //}

        virtual void bindSelfToContext(IRenderSVG *ctx, IAmGroot *groot) 
        {
            fPortal.bindToContext(ctx, groot);
        }

        //
        // drawSelf()
        // 
        // This is called before the child nodes are drawn.  
        // We apply the transform, ensuring the coordinate system
        // is properly established.
        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            // Clipping doesn't quite work out, because it's a non-transformed
            // rectangle on the context, and it's only a rectangle, not a shape
            // it will not transform along with the context
            //ctx->clip(getBBox());

            // We do an 'applyTransform' instead of 'setTransform'
            // because there might already be a transform on the context
            // and we want to build upon that, rather than replace it.
            //ctx->setTransform(fViewport.sceneToSurfaceTransform());
            ctx->applyTransform(fPortal.viewBoxToViewportTransform());
            ctx->setViewport(getBBox());
        }

    };
}

