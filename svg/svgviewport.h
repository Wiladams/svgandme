#pragma once

#include "svgstructuretypes.h"


namespace waavs {
    
	// A SVGViewport represents the mapping between one 2D coordinate system and another.
    // This applies to the <svg>, and <symbol> elements.
    struct SVGViewport : public SVGObject
    {
        ViewPort fViewport;     // The workhorse
        
        SVGVariableSize fDimX{};
        SVGVariableSize fDimY{};
        SVGVariableSize fDimWidth{};
        SVGVariableSize fDimHeight{};
        
        bool fHasViewbox{ false };
		BLRect fViewBox{};
        
        AspectRatioKind fPreserveAspectRatio{ AspectRatioKind::SVG_ASPECT_RATIO_XMIDYMID };
        

        BLRect getBBox() const
        {
            return fViewport.surfaceFrame();
        }

		ViewPort& getViewport()
        {
			return fViewport;
		}
        
        // loadFromAttributes()
        //
        // Everything we need to establish the viewport should be in the attributes
        // so load it up, and establish the coordinate system
		// x, y, width, height, viewBox, preserveAspectRatio
        //
		// Load the non-bound attribute values here, for processing later
        // when we bind.
        bool loadFromAttributes(const XmlAttributeCollection& attrs)
        {
			// x, y, width, height
            fDimX.loadFromChunk(attrs.getAttribute("x"));
            fDimY.loadFromChunk(attrs.getAttribute("y"));
			fDimWidth.loadFromChunk(attrs.getAttribute("width"));
			fDimHeight.loadFromChunk(attrs.getAttribute("height"));

			// viewBox
			fHasViewbox = parseViewBox(attrs.getAttribute("viewBox"), fViewBox);

			// preserveAspectRatio
            getEnumValue(SVGAspectRatioEnum, attrs.getAttribute("preserveAspectRatio"), (uint32_t&)fPreserveAspectRatio);

            return true;
        }

        // Here's where we can resolve what the values actually mean
        // We need to deal with a couple of complex cases where one or the
		// other of the dimensions are not specified.
        // If either of width or height  
        void bindToContext(IRenderSVG *ctx, IAmGroot *groot) noexcept override
        {
            double origin = 0;
            BLRect containerFrame = ctx->localFrame();
            double dpi = 96;
            if (groot != nullptr)
                dpi = groot->dpi();

            // Resolve the bounding box first
            // Start with it being the containing frame
            // alter only the parts that are specified
            BLRect surfaceFrame = containerFrame;


            fDimX.parseValue(surfaceFrame.x, ctx->font(), containerFrame.w, origin, dpi, SpaceUnitsKind::SVG_SPACE_USER);
            fDimY.parseValue(surfaceFrame.y, ctx->font(), containerFrame.h, origin, dpi, SpaceUnitsKind::SVG_SPACE_USER);
            fDimWidth.parseValue(surfaceFrame.w, ctx->font(), containerFrame.w, origin, dpi, SpaceUnitsKind::SVG_SPACE_USER);
            fDimHeight.parseValue(surfaceFrame.h, ctx->font(), containerFrame.h, origin, dpi, SpaceUnitsKind::SVG_SPACE_USER);

            
			// If a viewbox was set, then we use that to create the transformation
			// matrix between the scene, and the surface
            BLRect sceneFrame = surfaceFrame;
			if (fHasViewbox)
			{
				sceneFrame = fViewBox;
            }

            fViewport.surfaceFrame(surfaceFrame);
            fViewport.sceneFrame(sceneFrame);
            

			// Now the preserveAspectRatio
			//fViewport.setPreserveAspectRatio(fPreserveAspectRatio);
        }

    };
}
