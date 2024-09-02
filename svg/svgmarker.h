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

	struct SVGMarkerElement : public SVGGraphicsElement
	{
		static void registerFactory()
		{
			gSVGGraphicsElementCreation["marker"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGMarkerElement>(groot);
				node->loadFromXmlIterator(iter, groot);

				return node;
				};
		}

		// Fields
		SVGViewbox fViewbox{};
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

		SVGMarkerElement(IAmGroot* root)
			: SVGGraphicsElement(root)
		{
			isStructural(false);
		}

		const SVGOrient& orientation() const { return fOrientation; }


		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{
			auto preserveAspectRatio = getAttribute("preserveAspectRatio");


			if (getAttribute("viewBox")) {
				fViewbox.loadFromChunk(getAttribute("viewBox"));
			}

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
			else if (fViewbox.isSet())
				fMarkerWidth = fViewbox.width();

			if (fDimMarkerHeight.isSet())
				fMarkerHeight = fDimMarkerHeight.calculatePixels();
			else if (fViewbox.isSet())
				fMarkerHeight = fViewbox.height();

			if (fDimRefX.isSet())
				fRefX = fDimRefX.calculatePixels();
			else if (fViewbox.isSet())
				fRefX = fViewbox.x();
		}

		void applyAttributes(IRenderSVG* ctx, IAmGroot* groot) override
		{
			SVGGraphicsElement::applyAttributes(ctx, groot);

			double sWidth = ctx->strokeWidth();
			double scaleX = 1.0;
			double scaleY = 1.0;

			if (fMarkerUnits == StrokeWidth)
			{
				// scale == strokeWidth
				scaleX = sWidth;
				scaleY = sWidth;
				ctx->scale(scaleX, scaleY);
			}
			else
			{
				if (fViewbox.isSet())
				{
					scaleX = fMarkerWidth / fViewbox.width();
					scaleY = fMarkerHeight / fViewbox.height();
					ctx->scale(scaleX, scaleY);
				}
				// No scaling
			}


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


