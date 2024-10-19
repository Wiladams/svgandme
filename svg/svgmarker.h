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
	enum MarkerUnits {
		UserSpaceOnUse = 0,
		StrokeWidth = 1
	};

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
		//SVGViewbox fViewbox{};
		SVGDimension fDimRefX{};
		SVGDimension fDimRefY{};
		SVGDimension fDimMarkerWidth{};
		SVGDimension fDimMarkerHeight{};
		uint32_t fMarkerUnits{ StrokeWidth };
		SVGOrient fOrientation{ nullptr };

		// Resolved field values
		double fRefX = 0;
		double fRefY = 0;
		double fMarkerWidth{ 3 };
		double fMarkerHeight{ 3 };
		
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

			// Get the current container frame
			BLRect cFrame = ctx->localFrame();
			double w = cFrame.w;
			double h = cFrame.h;

			BLRect fSceneFrame{};
			BLRect fSurfaceFrame{};


			// Get the aspect ratio
			getEnumValue(SVGAspectRatioEnum, getAttribute("preserveAspectRatio"), (uint32_t&)fPreserveAspectRatio);


			// Load parameters for the portal
			SVGVariableSize fDimX{};
			SVGVariableSize fDimY{};
			SVGVariableSize fDimWidth{};
			SVGVariableSize fDimHeight{};

			fDimX.loadFromChunk(getAttribute("x"));
			fDimY.loadFromChunk(getAttribute("y"));
			fDimWidth.loadFromChunk(getAttribute("width"));
			fDimHeight.loadFromChunk(getAttribute("height"));

			// Load parameters for the viewbox
			BLRect viewboxRect{};
			bool haveViewbox = parseViewBox(getAttribute("viewBox"), viewboxRect);



			if (fDimWidth.isSet() || fDimHeight.isSet())
			{
				// When calculating these, if the attribute was not set, the 'length' will 
				// be the value returned.
				fSurfaceFrame.w = fDimWidth.calculatePixels(ctx->font(), w, 0, dpi);
				fSurfaceFrame.h = fDimHeight.calculatePixels(ctx->font(), h, 0, dpi);

				// For these, we want to use a default value of '0' if they were not
				// explicitly set.  
				// For these, we start with default values of '0', and only
				// change them if they were explicitly set.
				if (fDimX.isSet())
					fSurfaceFrame.x = fDimX.calculatePixels(ctx->font(), w, 0, dpi);

				if (fDimY.isSet())
					fSurfaceFrame.y = fDimY.calculatePixels(ctx->font(), h, 0, dpi);


			}
			else {
				// If no width and height were set, then we want the
				// surface frame to match the 'viewBox' attribute
				// and if that was not set, then we want to return immediately
				// because it's an error to not have at least one of them set
				if (haveViewbox)
					fSurfaceFrame = viewboxRect;
				else
					return;

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
			
			BLRect viewboxRect{};
			bool haveViewbox = parseViewBox(getAttribute("viewBox"), viewboxRect);
			

			if (getAttribute("refX")) {
				fDimRefX.loadFromChunk(getAttribute("refX"));
			}
			if (getAttribute("refY")) {
				fDimRefY.loadFromChunk(getAttribute("refY"));
			}

			if (getAttribute("markerWidth")) {
				fDimMarkerWidth.loadFromChunk(getAttribute("markerWidth"));
			}

			if (getAttribute("markerHeight")) {
				fDimMarkerHeight.loadFromChunk(getAttribute("markerHeight"));
			}

			if (getAttribute("markerUnits")) {
				auto units = getAttribute("markerUnits");
				if (units == "strokeWidth") {
					fMarkerUnits = StrokeWidth;
				}
				else if (units == "userSpaceOnUse") {
					fMarkerUnits = UserSpaceOnUse;
				}
			}

			if (getAttribute("orient")) {
				fOrientation.loadFromChunk(getAttribute("orient"));
			}
			
			
			if (fDimMarkerWidth.isSet())
				fMarkerWidth = fDimMarkerWidth.calculatePixels();
			else if (haveViewbox)
				fMarkerWidth = viewboxRect.w;

			if (fDimMarkerHeight.isSet())
				fMarkerHeight = fDimMarkerHeight.calculatePixels();
			else if (haveViewbox)
				fMarkerHeight = viewboxRect.h;

			if (fDimRefX.isSet())
				fRefX = fDimRefX.calculatePixels();
			else if (haveViewbox)
				fRefX = viewboxRect.x;

			if (fDimRefY.isSet())
				fRefY = fDimRefY.calculatePixels();
			else if (haveViewbox)
				fRefY = viewboxRect.y;


			double sWidth = ctx->strokeWidth();
			
			if (fMarkerUnits == StrokeWidth)
			{
				scaleX = sWidth;
				scaleY = sWidth;
			}
			else
			{
				if (haveViewbox)
				{
					scaleX = fMarkerWidth / viewboxRect.w;
					scaleY = fMarkerHeight / viewboxRect.h;
				}
				// No scaling
			}
			
		}

		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			SVGContainer::drawSelf(ctx, groot);
			
			ctx->scale(scaleX, scaleY);




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


