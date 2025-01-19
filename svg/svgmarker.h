#pragma once

//
// Feature String: http://www.w3.org/TR/SVG11/feature#Marker
//
//#include <functional>
//#include <unordered_map>

#include "svgattributes.h"
#include "svgstructuretypes.h"
#include "svgportal.h"

namespace waavs {
	//=================================================
	// SVGMarkerNode

	// Reference: https://svg-art.ru/?page_id=855
	//=================================================

	struct SVGMarkerElement : public SVGGraphicsElement
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
		SVGPortal fPortal{};
		SVGVariableSize fDimRefX{};
		SVGVariableSize fDimRefY{};

		SpaceUnitsKind fMarkerUnits{ SpaceUnitsKind::SVG_SPACE_STROKEWIDTH };
		SVGOrient fOrientation{ nullptr };

		// Resolved field values
		double fRefX = 0;
		double fRefY = 0;

		
		BLPoint fMarkerContentScale{ 1,1 };
		BLPoint fMarkerTranslation{ 0,0 };
		BLRect fMarkerBoundingBox{ 0,0,3,3 };
		BLRect fViewbox{};
		
		
		SVGMarkerElement(IAmGroot* root)
			: SVGGraphicsElement()
		{
			isStructural(false);
		}

		BLRect frame() const override
		{
			return fPortal.getBBox();
		}
		
		BLRect getBBox() const override
		{
			return fPortal.getBBox();
		}
		

		const SVGOrient& orientation() const { return fOrientation; }


		//
		// createPortal
		// 
		// This code establishes the coordinate space for the element, and 
		// its child nodes.
		// 
		
		void createPortal(IRenderSVG *ctx, IAmGroot *groot)
		{
			// First, we want to know the size of the object we're going to be
			// rendered into
			// We also want to know the size of the container the object is in
			BLRect objectBoundingBox = ctx->objectFrame();
			BLRect containerBoundingBox = ctx->viewport();
			
			
			double dpi = 96;
			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}

			// Load parameters for the portal
			SVGDimension fDimWidth{};
			SVGDimension fDimHeight{};

			// If these are not specified, then we use default values of '3'
			fDimWidth.loadFromChunk(getAttribute("markerWidth"));
			fDimHeight.loadFromChunk(getAttribute("markerHeight"));
			bool haveViewbox = parseViewBox(getAttribute("viewBox"), fViewbox);
			fPortal.loadFromAttributes(fAttributes);
			
			double sWidth = ctx->getStrokeWidth();

			// First, we setup the marker bounding box
			// This is determined based on the markerWidth, and markerHeight
			// attributes.
			if (fMarkerUnits == SpaceUnitsKind::SVG_SPACE_STROKEWIDTH)
			{
				// We use the strokeWidth to scale the marker
				// If the dimension type is percentage, then calculate
				// based on a percentage of the stroke width
				// If the dimension is a number, then use that number, and 
				// multiply it by the strokeWidth
				if (fDimWidth.isSet())
				{
					if (fDimWidth.isPercentage())
					{
						fMarkerBoundingBox.w = fDimWidth.calculatePixels(sWidth);
					}
					else
					{
						fMarkerBoundingBox.w = fDimWidth.calculatePixels() * sWidth;
					}
				}
				
				if (fDimHeight.isSet())
				{
					if (fDimWidth.isPercentage())
					{
						fMarkerBoundingBox.h = fDimHeight.calculatePixels(sWidth);
					}
					else {
						fMarkerBoundingBox.h = fDimHeight.calculatePixels() * sWidth;
					}
				}
			}
			else if (fMarkerUnits == SpaceUnitsKind::SVG_SPACE_USER)
			{
				if (fDimWidth.isSet())
				{
					fMarkerBoundingBox.w = fDimWidth.calculatePixels(containerBoundingBox.w)*sWidth;
				}

				if (fDimHeight.isSet())
				{
					fMarkerBoundingBox.h = fDimHeight.calculatePixels(containerBoundingBox.h)*sWidth;
				}
			}
			
			// Now that we have the markerBoundingBox settled, we need to calculate an additional 
			// scaling for the content area if a viewBox is specified
			
			if (haveViewbox) {
				fMarkerContentScale.x = fMarkerBoundingBox.w / fViewbox.w;
				fMarkerContentScale.y = fMarkerBoundingBox.h / fViewbox.h;
			}
			
			fDimRefX.parseValue(fMarkerTranslation.x, ctx->font(), fMarkerBoundingBox.w, 0, dpi);
			fDimRefY.parseValue(fMarkerTranslation.y, ctx->font(), fMarkerBoundingBox.h, 0, dpi);


			fPortal.viewportFrame(fMarkerBoundingBox);



		}
		
		virtual void fixupSelfStyleAttributes(IRenderSVG*, IAmGroot*)
		{
			// printf("fixupSelfStyleAttributes\n");
			getEnumValue(MarkerUnitEnum, getAttribute("markerUnits"), (uint32_t&)fMarkerUnits);
			//getEnumValue(SVGAspectRatioEnum, getAttribute("preserveAspectRatio"), (uint32_t&)fPreserveAspectRatio);

			fDimRefX.loadFromChunk(getAttribute("refX"));
			fDimRefY.loadFromChunk(getAttribute("refY"));
			fOrientation.loadFromChunk(getAttribute("orient"));

		}

		
		void bindSelfToContext(IRenderSVG *ctx, IAmGroot *groot) override 
		{
			createPortal(ctx, groot);

		}


		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			//createPortal(ctx, groot);
			
			ctx->scale(fMarkerContentScale.x, fMarkerContentScale.y);
			ctx->translate(-fMarkerTranslation.x, -fMarkerTranslation.y);

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


