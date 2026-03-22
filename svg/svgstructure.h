#pragma once

// Support for SVG structure elements
// http://www.w3.org/TR/SVG11/feature#Structure
// svg
// g
// defs
// desc
// title
// metadata


#include <array>
#include <functional>

#include "svggraphicselement.h"
#include "viewport.h"


namespace waavs {
    //====================================================
    // SVGSVGElement
    // 
    // SVGSVGElement
    // This is the root node of an entire SVG tree
    //====================================================
    struct SVGSVGElement : public SVGGraphicsElement
    {
        static void registerFactory()
        {
            registerContainerNodeByName("svg",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGSVGElement>(groot);
                    node->loadFromXmlPull(iter, groot);

                    return node;
                });
        }

        // Instance fields
        bool fIsTopLevel{ true };	// Is this a top level svg element

        // The viewport attributes as authored in the document, before resolution
        DocViewportState fDocViewport;	
        bool fHasDocViewport{ false };

        // The viewport state after resolution, which is what we use for rendering and layout
        SVGViewportState fViewportState;	// The resolved viewport state
        

        SVGSVGElement(IAmGroot* groot )
            : SVGGraphicsElement()
        {
            setNeedsBinding(true);
        }

        // setTopLevel
        //
        // A top level node is different than an SVG node which might
        // be located deeper in the DOM tree.  The top level node ignores
        // any x,y attributes, only paying attention to the dimensions
        // width, and height.
        //
        // Deeper into the tree, the x,y attributes are used to position the node
        // relative to the parent node.  The width and height attributes are used
        // to set the size of the node, but not the position.
        // And svg node is NOT top level by default, and is explicitly set
        // to be top level by the DOM constructor
        void setTopLevel(bool isTop) { fIsTopLevel = isTop; }
        bool isTopLevel() const { return fIsTopLevel; }

        const WGRectD viewPort() const noexcept
        {
            return fViewportState.fViewport;
        }

        const WGRectD objectBoundingBox() const noexcept override
        {
            return viewPort();
        }

        

        void fixupSelfStyleAttributes(IAmGroot *groot) override
        {
            // printf("fixupSelfStyleAttributes\n");
            // load the document representation of the viewport attributes
            DocViewportState vps{};
            loadDocViewportState(vps, fAttributes);

            // Try to resolve the viewport state 
            if (fIsTopLevel && groot)
            {
                const double dpi = groot->dpi();
                WGRectD canvasRect = { 0, 0, groot->canvasWidth(), groot->canvasHeight() };
                resolveViewState(canvasRect, vps, true, dpi, nullptr, fViewportState);
            }
            else {
                fDocViewport = vps;
                fHasDocViewport = true;
                fViewportState.fResolved = false;
            }

        }


        
        void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            if (!fViewportState.fResolved)
            {
                const WGRectD containingVP = ctx->viewport();
                const double dpi = groot ? groot->dpi() : 96.0;
                resolveViewState(containingVP, fDocViewport, false, dpi, nullptr, fViewportState);
            }
        }
        
        
        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            // Clipping doesn't quite work out, because it's a non-transformed
            // rectangle on the context, and it's only a rectangle, not a shape
            // it will not transform along with the context
            //ctx->clip(viewport());

            // Apply mapping from this svg's user space (viewBox) to its 
            // parent user space (viewport)
            // We do an 'applyTransform()' instead of 'transform()'
            // because we want to concatenat to whatever transform is 
            // already there, not replace it.
            ctx->applyTransform(fViewportState.viewBoxToViewportXform);

            // Establish the 'nearest viewport' for children, in THIS
            // svg's user space.  This is what percentage lengths should
            // use as their reference.
            ctx->setViewport(fViewportState.fViewBox);

            // Object frame for <svg> isn't a true geometry bbox; we'll
            // keep it simple, just so there is something.
            // We could just ignore it, and allow whatever was already
            // to remain.
            //ctx->setObjectFrame(fViewportState.fViewBox);
        }
    };
    
    //================================================
    // SVGGroupNode
    // 'g' element
    //================================================
    struct SVGGElement : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("g", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGGElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        // Static constructor to register factory method in map
        static void registerFactory()
        {
            registerContainerNodeByName("g",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGGElement>();
                    node->loadFromXmlPull(iter, groot);

                    return node;
                });
            
            registerSingularNode();
        }



        // Instance Constructor
        SVGGElement( )
            : SVGGraphicsElement()
        {
        }

        const WGRectD objectBoundingBox() const noexcept override
        {
            WGRectD pbox{};

            for (auto& node : fNodes)
            {
                if (!node || !node->isVisible()) continue;

                WGRectD nodeBox{};
                nodeBox = node->objectBoundingBox();
                wg_rectD_union(pbox, nodeBox);
            }

            return pbox;
        }

        // calculate the full extent of the painted area.  Used to calculate
        // ofscreen size for filters.
        const WGRectD getFilterRegion(IRenderSVG* ctx, IAmGroot* groot)  noexcept override
        {
            WGRectD pbox{};

            for (auto& node : fNodes)
            {
                if (!node || !node->isVisible()) continue;

                WGRectD nodeBox{};
                nodeBox = node->getFilterRegion(ctx, groot);
                wg_rectD_union(pbox, nodeBox);
            }

            return pbox;
        }
    };

}
