#pragma once

#include "svgstructuretypes.h"
#include "svgenums.h"


namespace waavs {
    
    struct SVGPreserveAspectRatio
    {
		AspectRatioAlingKind fAlignment = AspectRatioAlingKind::SVG_ASPECT_RATIO_XMIDYMID;
		AspectRatioMeetOrSliceKind fMeetOrSlice = AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_MEET;

        // Load the data type from a single ByteSpan
        bool loadFromChunk(const ByteSpan& inChunk)
        {
            ByteSpan s = chunk_trim(inChunk, xmlwsp);
            if (s.empty())
                return false;

            
            // Get first token, which should be alignment
            ByteSpan align = chunk_token(s, xmlwsp);
            
            if (!align)
                return false;

            // We have an alignment token, convert to numeric value
			getEnumValue(SVGAspectRatioAlignEnum, align, (uint32_t &)fAlignment);

            // Now, see if there is a slice value
			chunk_ltrim(s, xmlwsp);

            if (s.empty())
                return false;

			getEnumValue(SVGAspectRatioMeetOrSliceEnum, s, (uint32_t&)fMeetOrSlice);
            
            return true;
        }
        

    };
    
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
        
        SVGPreserveAspectRatio fPreserveAspectRatio{};
        

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
			fPreserveAspectRatio.loadFromChunk(attrs.getAttribute("preserveAspectRatio"));
            
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
