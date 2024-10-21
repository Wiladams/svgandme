#pragma once

//
// Feature String: http://www.w3.org/TR/SVG11/feature#Marker
//
//#include <functional>
//#include <unordered_map>

#include "svgattributes.h"
#include "svgstructuretypes.h"


namespace waavs {
	//=================================================
	// SVGMarkerNode

	// Reference: https://svg-art.ru/?page_id=855
	//=================================================

	struct SVGMarkerElement : public SVGContainer
	{
		static void registerFactory()
		{
			registerContainerNode("marker",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGMarkerElement>(groot);
					node->loadFromXmlIterator(iter, groot);

					return node;
				});
			
		}

		// Fields
		SVGDimension fDimRefX{};
		SVGDimension fDimRefY{};

		SpaceUnitsKind fMarkerUnits{ SpaceUnitsKind::SVG_SPACE_STROKEWIDTH };
		SVGOrient fOrientation{ nullptr };

		// Resolved field values
		double fRefX = 0;
		double fRefY = 0;

		
		double scaleX = 1.0;
		double scaleY = 1.0;

		
		SVGMarkerElement(IAmGroot* root)
			: SVGContainer()
		{
			isStructural(false);
		}

		const SVGOrient& orientation() const { return fOrientation; }


		//
		// createPortal
		// 
		// This code establishes the coordinate space for the element, and 
		// its child nodes.
		// 
		void createPortal(IRenderSVG* ctx, IAmGroot* groot) override
		{
			double dpi = 96;
			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}

			getEnumValue(MarkerUnitEnum, getAttribute("markerUnits"), (uint32_t&)fMarkerUnits);


			double sWidth = ctx->strokeWidth();
			double w = 3;
			double h = 3;
			if (fMarkerUnits == SpaceUnitsKind::SVG_SPACE_STROKEWIDTH)
			{
				w = sWidth;
				h = sWidth;
			}
			else
			{
				BLRect cFrame = ctx->localFrame();
				w = cFrame.w;
				h = cFrame.h;
			}

				
			// Get the aspect ratio
			getEnumValue(SVGAspectRatioEnum, getAttribute("preserveAspectRatio"), (uint32_t&)fPreserveAspectRatio);


			// Load parameters for the portal
			SVGVariableSize fDimWidth{};
			SVGVariableSize fDimHeight{};

			// If these are not specified, then we use default values of '3'
			fDimWidth.loadFromChunk(getAttribute("markerWidth"));
			fDimHeight.loadFromChunk(getAttribute("markerHeight"));

			
			// Load parameters for the viewbox
			BLRect viewboxRect{};
			bool haveViewbox = parseViewBox(getAttribute("viewBox"), viewboxRect);

			BLRect fSceneFrame{};
			BLRect fSurfaceFrame{};
			
			// Set default surfaceFrame
			fSurfaceFrame = { 0,0,3.0,3.0 };

			
			if (fDimWidth.isSet() || fDimHeight.isSet())
			{	
				if (fDimWidth.isSet())
				{
					fSurfaceFrame.w = fDimWidth.calculatePixels(ctx->font(), w, 0, dpi);
				}
				if (fDimHeight.isSet())
				{
					fSurfaceFrame.h = fDimHeight.calculatePixels(ctx->font(), h, 0, dpi);
				}
			}
			else {
				// If no width and height were set, then we want the
				// surface frame to match the 'viewBox' attribute
				// and if that was not set, then we want to return immediately
				// because it's an error to not have at least one of them set
				if (haveViewbox)
					fSurfaceFrame = viewboxRect;
				else {
					visible(false);
					return;
				}
			}

			fViewport.surfaceFrame(fSurfaceFrame);

			// If the viewbox is set, then use that as the scene frame
			// if not, then use the surfaceframe as the scene frame for 
			// an identity transform
			if (haveViewbox)
			{
				fViewport.sceneFrame(viewboxRect);
			}
			else
			{
				fViewport.sceneFrame(fSurfaceFrame);
			}

		}
		
		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			createPortal(ctx, groot);
			

			if (getAttribute("refX")) {
				fDimRefX.loadFromChunk(getAttribute("refX"));
			}
			if (getAttribute("refY")) {
				fDimRefY.loadFromChunk(getAttribute("refY"));
			}

			if (getAttribute("orient")) {
				fOrientation.loadFromChunk(getAttribute("orient"));
			}
			


			
		}

		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			SVGContainer::drawSelf(ctx, groot);
			
			//ctx->scale(scaleX, scaleY);
			ctx->translate(-fDimRefX.calculatePixels(), -fDimRefY.calculatePixels());
		}

		void drawChildren(IRenderSVG* ctx, IAmGroot* groot) override
		{
			ctx->push();

			// Setup drawing state
			ctx->blendMode(BL_COMP_OP_SRC_OVER);
			ctx->fill(BLRgba32(0, 0, 0));
			ctx->noStroke();
			ctx->strokeWidth(1.0);
			ctx->setStrokeJoin(BLStrokeJoin::BL_STROKE_JOIN_MITER_BEVEL);

			SVGGraphicsElement::drawChildren(ctx, groot);

			ctx->pop();
		}


	};
}


