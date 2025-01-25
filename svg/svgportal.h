#pragma once

#include "svgstructuretypes.h"
#include "svgenums.h"


namespace waavs {
    

    
	// A SVGPortal represents the mapping between one 2D coordinate system and another.
    // This applies to the <svg>, and <symbol> elements.
    // This class specifically knows how to load itself
    // from xml attributes, and  can delay resolve sizing information
    struct SVGPortal : public ViewportTransformer, public SVGObject
    {        
        SVGVariableSize fDimX{};
        SVGVariableSize fDimY{};
        SVGVariableSize fDimWidth{};
        SVGVariableSize fDimHeight{};
        
        bool fHasViewbox{ false };


        BLRect getBBox() const
        {
            return viewportFrame();
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
            
            // preserveAspectRatio
            fPreserveAspectRatio.loadFromChunk(attrs.getAttribute("preserveAspectRatio"));

			// viewBox
            fHasViewbox = parseViewBox(attrs.getAttribute("viewBox"), fViewBoxFrame);


            // We can parse these here, but we can't resolve them 
            // until we bind to a context
            // x, y, width, height
            fDimX.loadFromChunk(attrs.getAttribute("x"));
            fDimY.loadFromChunk(attrs.getAttribute("y"));
            fDimWidth.loadFromChunk(attrs.getAttribute("width"));
            fDimHeight.loadFromChunk(attrs.getAttribute("height"));

            
            return true;
        }

        // Here's where we can resolve what the values actually mean
        // We need to deal with a couple of complex cases where one or the
		// other of the dimensions are not specified.
        // If either of width or height  
        void bindToContext(IRenderSVG *ctx, IAmGroot *groot) noexcept override
        {
            double origin = 0;
            BLRect viewport = ctx->viewport();
            double dpi = 96;
            if (groot != nullptr)
                dpi = groot->dpi();

            // Resolve the bounding box first
            // Start with it being the containing frame
            // alter only the parts that are specified
            BLRect srfFrame = viewport;


            fDimX.parseValue(srfFrame.x, ctx->font(), viewport.w, origin, dpi, SpaceUnitsKind::SVG_SPACE_USER);
            fDimY.parseValue(srfFrame.y, ctx->font(), viewport.h, origin, dpi, SpaceUnitsKind::SVG_SPACE_USER);
            fDimWidth.parseValue(srfFrame.w, ctx->font(), viewport.w, origin, dpi, SpaceUnitsKind::SVG_SPACE_USER);
            fDimHeight.parseValue(srfFrame.h, ctx->font(), viewport.h, origin, dpi, SpaceUnitsKind::SVG_SPACE_USER);

            
			// If a viewbox was set, then we use that to create the transformation
			// matrix between the scene, and the surface
            BLRect scnFrame = srfFrame;
			if (!fHasViewbox)
			{
                fViewBoxFrame = srfFrame;
                //viewBoxFrame(srfFrame);
            }
            else {
                //viewBoxFrame(fViewBoxFrame);
            }

            viewportFrame(srfFrame);
            
            //updateTransformMatrix();

        }

    };
}
