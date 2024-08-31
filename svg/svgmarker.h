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

		void loadVisualProperties(const XmlAttributeCollection& attrs, IAmGroot* groot) override
		{
			SVGGraphicsElement::loadVisualProperties(attrs, groot);

			auto preserveAspectRatio = attrs.getAttribute("preserveAspectRatio");


			if (attrs.getAttribute("viewBox")) {
				fViewbox.loadFromChunk(attrs.getAttribute("viewBox"));
			}

			if (attrs.getAttribute("refX")) {
				fDimRefX.loadFromChunk(attrs.getAttribute("refX"));
			}
			if (attrs.getAttribute("refY")) {
				fDimRefY.loadFromChunk(attrs.getAttribute("refY"));
			}

			if (attrs.getAttribute("markerWidth")) {
				fDimMarkerWidth.loadFromChunk(attrs.getAttribute("markerWidth"));
			}

			if (attrs.getAttribute("markerHeight")) {
				fDimMarkerHeight.loadFromChunk(attrs.getAttribute("markerHeight"));
			}

			if (attrs.getAttribute("markerUnits")) {
				auto units = attrs.getAttribute("markerUnits");
				if (units == "strokeWidth") {
					fMarkerUnits = StrokeWidth;
				}
				else if (units == "userSpaceOnUse") {
					fMarkerUnits = UserSpaceOnUse;
				}
			}

			if (attrs.getAttribute("orient")) {
				fOrientation.loadFromChunk(attrs.getAttribute("orient"));
			}

		}


	};
}


