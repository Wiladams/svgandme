#pragma once

//
// Support for SVGGradientElement
// http://www.w3.org/TR/SVG11/feature#Gradient
// linearGradient, radialGradient, conicGradient
//

#include <functional>
#include <unordered_map>

#include "svgattributes.h"
#include "svgstructuretypes.h"


namespace waavs {

	//============================================================
	//	SVGGradient
	//  gradientUnits ("userSpaceOnUse" or "objectBoundingBox")
	//============================================================
	enum GradientUnits {
		objectBoundingBox = 0,
		userSpaceOnUse = 1
	};

	// Default values
	// offset == 0
	// color == black
	// opacity == 1.0
	struct SVGStopNode : public SVGObject
	{
		double fOffset = 0;
		double fOpacity = 1;
		BLRgba32 fColor{ 0xff000000 };

		SVGStopNode(IAmGroot* groot) :SVGObject() {}

		double offset() const { return fOffset; }
		double opacity() const { return fOpacity; }
		BLRgba32 color() const { return fColor; }

		void loadFromXmlElement(const XmlElement& elem, IAmGroot* groot)
		{
			// Get the attributes from the element
			ByteSpan attrSpan = elem.data();
			XmlAttributeCollection attrs{};
			attrs.scanAttributes(attrSpan);
			
			// If there's a style attribute, then add those to the collection
			auto style = attrs.getAttribute("style");
			if (style)
			{
				// If we have a style attribute, assume both the stop-color
				// and the stop-opacity are in there
				parseStyleAttribute(style, attrs);
			}

			// Get the offset
			SVGDimension dim{};
			dim.loadFromChunk(attrs.getAttribute("offset"));
			if (dim.isSet())
			{
				fOffset = dim.calculatePixels(1);
			}

			SVGDimension dimOpacity{};
			SVGPaint paint(groot);

			// Get the stop color
			if (attrs.getAttribute("stop-color"))
			{
				paint.loadFromChunk(attrs.getAttribute("stop-color"));
			}
			else
			{
				// Default color is black
				paint.loadFromChunk("black");
			}
			
			if (attrs.getAttribute("stop-opacity"))
			{
				dimOpacity.loadFromChunk(attrs.getAttribute("stop-opacity"));
			}
			else
			{
				// Default opacity is 1.0
				dimOpacity.loadFromChunk("1.0");
			}
			
			if (dimOpacity.isSet())
			{
				fOpacity = dimOpacity.calculatePixels(1);
			}

			paint.setOpacity(fOpacity);

			

			BLVar aVar = paint.getVariant();
			uint32_t colorValue = 0;
			auto res = blVarToRgba32(&aVar, &colorValue);
			fColor.value = colorValue;

		}
	};
	
	//============================================================
	// SVGGradient
	// Base class for other gradient types
	//============================================================
	struct SVGGradient : public SVGGraphicsElement
	{
		bool fHasGradientTransform = false;
		BLMatrix2D fGradientTransform{};

		BLGradient fGradient{};
		BLVar fGradientVar{};
		GradientUnits fGradientUnits{ objectBoundingBox };
		ByteSpan fTemplateReference{};


		// Constructor
		SVGGradient(IAmGroot* aroot)
			:SVGGraphicsElement(aroot)
		{
			fGradient.setExtendMode(BL_EXTEND_MODE_PAD);
			isStructural(false);
			needsBinding(true);
		}
		SVGGradient(const SVGGradient& other) = delete;

		SVGGradient operator=(const SVGGradient& other) = delete;

		const BLVar getVariant() noexcept override
		{
			return fGradientVar;
		}

		// Load a URL Reference
		void resolveReference(IAmGroot* groot, SVGViewable* container)
		{
			// if there's no manager
			// or there's no template to look
			// return immediately
			//if (refManager == nullptr || !fTemplateReference)
			//	return;

			if (fTemplateReference)
			{
				auto idValue = chunk_trim(fTemplateReference, xmlwsp);

				// The first character could be '.' or '#'
				// so we need to skip past that
				if (*idValue == '.' || *idValue == '#')
					idValue++;

				if (!idValue)
					return;

				// lookup the thing we're referencing
				auto node = groot->getElementById(idValue);


				if (node != nullptr)
				{
					// That node itself might need to be bound
					//if (node->needsBinding())
					//	node->bindToGroot(groot);
					node->bindToGroot(groot, container);

					const BLVar& aVar = node->getVariant();

					if (aVar.isGradient())
					{
						// BUGBUG - we need to take care here to assign
						// the right stuff based on what kind of gradient
						// we are.
						// Start with just assigning the stops
						const BLGradient& tmpGradient = aVar.as<BLGradient>();

						//fGradient = tmpGradient;
						//if (fHasGradientTransform)
						//{
						//	fGradient.setTransform(fGradientTransform);
						//}
					
						///*
						// assign stops from tmpGradient
						fGradient.assignStops(tmpGradient.stops(), tmpGradient.size());

						// transform matrix if it already exists and
						// we had set a new one as well
						// otherwise, just set the new one
						if (tmpGradient.hasTransform())
						{
							if (fHasGradientTransform)
							{
								BLMatrix2D tmpMatrix = tmpGradient.transform();
								tmpMatrix.transform(fGradientTransform);
								fGradient.setTransform(tmpMatrix);
							}
							else
							{
								fGradient.setTransform(tmpGradient.transform());
							}
						}
						else if (fHasGradientTransform)
						{
							fGradient.setTransform(fGradientTransform);
						}
						//*/

					}
				}
			}
			else if (fHasGradientTransform) {
				fGradient.setTransform(fGradientTransform);
			}

			fGradientVar = fGradient;

		}


		void resolvePosition(IAmGroot* groot, SVGViewable* container)
		{
			// See if we have a template reference
			if (getAttribute("href"))
				fTemplateReference = getAttribute("href");
			else if (getAttribute("xlink:href"))
				fTemplateReference = getAttribute("xlink:href");

			// Whether we've loaded a template or not
			// load the common attributes for gradients
			BLExtendMode extendMode{ BL_EXTEND_MODE_PAD };
			if (parseSpreadMethod(getAttribute("spreadMethod"), extendMode))
			{
				fGradient.setExtendMode(extendMode);
			}


			// read the gradientUnits
			if (getAttribute("gradientUnits"))
			{
				auto units = getAttribute("gradientUnits");
				if (units == "userSpaceOnUse")
					fGradientUnits = userSpaceOnUse;
				else if (units == "objectBoundingBox")
					fGradientUnits = objectBoundingBox;
			}

			// Get the transform
			// This will get applied in the resolveReferences in case
			// the template has its own matrix
			if (getAttribute("gradientTransform"))
			{
				fHasGradientTransform = parseTransform(getAttribute("gradientTransform"), fGradientTransform);
			}


		}

		//
		// The only nodes here should be stop nodes
		//
		void loadSelfClosingNode(const XmlElement& elem, IAmGroot* groot) override
		{
			//printf("SVGGradientNode::loadSelfClosingNode()\n");
			//printXmlElement(elem);
			if (elem.name() != "stop")
			{
				return;
			}

			SVGStopNode stop(groot);
			stop.loadFromXmlElement(elem, groot);

			auto offset = stop.offset();
			auto acolor = stop.color();

			fGradient.addStop(offset, acolor);

		}

	};

	//=======================================
	// SVGLinearGradient
	//
	struct SVGLinearGradient : public SVGGradient
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["linearGradient"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGLinearGradient>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["linearGradient"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGLinearGradient>(groot);
				node->loadFromXmlIterator(iter, groot);

				return node;
				};

			registerSingularNode();
		}

		SVGLinearGradient(IAmGroot* aroot) :SVGGradient(aroot)
		{
			fGradient.setType(BL_GRADIENT_TYPE_LINEAR);
		}

		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{
			SVGGradient::resolvePosition(groot, container);

			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}


			// Get current values, in case we've already
			// loaded from a template
			BLLinearGradientValues values{ 0,0,0,1 };// = fGradient.linear();

			SVGDimension fX1{ 0,SVG_UNITS_USER };
			SVGDimension fY1{ 0,SVG_UNITS_USER };
			SVGDimension fX2{ 100,SVG_UNITS_PERCENT };
			SVGDimension fY2{ 0,SVG_UNITS_PERCENT };
			
			fX1.loadFromChunk(getAttribute("x1"));
			fY1.loadFromChunk(getAttribute("y1"));
			fX2.loadFromChunk(getAttribute("x2"));
			fY2.loadFromChunk(getAttribute("y2"));

			
			if (fGradientUnits == userSpaceOnUse)
			{
				if (fX1.isSet())
					values.x0 = fX1.calculatePixels(w, 0, dpi);
				if (fY1.isSet())
					values.y0 = fY1.calculatePixels(h, 0, dpi);
				if (fX2.isSet())
					values.x1 = fX2.calculatePixels(w, 0, dpi);
				if (fY2.isSet())
					values.y1 = fY2.calculatePixels(h, 0, dpi);
			}
			else
			{
				if (fX1.isSet())
					values.x0 = fX1.calculatePixels(w, 0, dpi);
				if (fY1.isSet())
					values.y0 = fY1.calculatePixels(h, 0, dpi);
				if (fX2.isSet())
					values.x1 = fX2.calculatePixels(w, 0, dpi);
				if (fY2.isSet())
					values.y1 = fY2.calculatePixels(h, 0, dpi);
			}

			resolveReference(groot, container);
			
			fGradient.setValues(values);
			fGradientVar = fGradient;

		}


	};

	//==================================
	// Radial Gradient
	// The radial gradient has a center point (cx, cy), a radius (r), and a focal point (fx, fy)
	// The center point is the center of the circle that the gradient is drawn on
	// The radius is the radius of that outer circle
	// The focal point is the point within, or on the circle that the gradient is focused on
	//==================================
	struct SVGRadialGradient : public SVGGradient
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["radialGradient"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGRadialGradient>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["radialGradient"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGRadialGradient>(groot);
				node->loadFromXmlIterator(iter, groot);
				return node;
				};

			registerSingularNode();
		}



		SVGRadialGradient(IAmGroot* groot) :SVGGradient(groot)
		{
			fGradient.setType(BL_GRADIENT_TYPE_RADIAL);
		}

		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{
			SVGGradient::resolvePosition(groot, container);

			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}


			BLRadialGradientValues values = fGradient.radial();

			SVGDimension fCx{ 50, SVG_UNITS_PERCENT };
			SVGDimension fCy{ 50, SVG_UNITS_PERCENT };
			SVGDimension fR{ 50, SVG_UNITS_PERCENT };
			SVGDimension fFx{ 50, SVG_UNITS_PERCENT, false };
			SVGDimension fFy{ 50, SVG_UNITS_PERCENT, false };
			
			fCx.loadFromChunk(getAttribute("cx"));
			fCy.loadFromChunk(getAttribute("cy"));
			fR.loadFromChunk(getAttribute("r"));
			fFx.loadFromChunk(getAttribute("fx"));
			fFy.loadFromChunk(getAttribute("fy"));
			
			values.x0 = fCx.calculatePixels(w, 0, dpi);
			values.y0 = fCy.calculatePixels(h, 0, dpi);
			values.r0 = fR.calculatePixels(w, 0, dpi);

			if (fFx.isSet())
				values.x1 = fFx.calculatePixels(w, 0, dpi);
			else
				values.x1 = values.x0;
			if (fFy.isSet())
				values.y1 = fFy.calculatePixels(h, 0, dpi);
			else
				values.y1 = values.y0;

			resolveReference(groot, container);
			
			fGradient.setValues(values);
			fGradientVar = fGradient;
		}



	};


	
	//=======================================
	// SVGConicGradient
	// This is NOT SVG Standard compliant
	// The conic gradient is supported by the blend2d library
	// so here it is.
	//
	struct SVGConicGradient : public SVGGradient
	{
		static void registerSingularNode()
		{
			gShapeCreationMap["conicGradient"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGConicGradient>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			gSVGGraphicsElementCreation["conicGradient"] = [](IAmGroot* groot, XmlElementIterator& iter) {
				auto node = std::make_shared<SVGConicGradient>(groot);
				node->loadFromXmlIterator(iter, groot);

				return node;
				};

			registerSingularNode();
		}




		SVGConicGradient(IAmGroot* aroot) :SVGGradient(aroot)
		{
			fGradient.setType(BLGradientType::BL_GRADIENT_TYPE_CONIC);
		}


		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{
			SVGGradient::resolvePosition(groot, container);

			double dpi = 96;
			double w = 1.0;
			double h = 1.0;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
				w = groot->canvasWidth();
				h = groot->canvasHeight();
			}


			BLConicGradientValues values = fGradient.conic();

			SVGDimension x0{};
			SVGDimension y0{};
			SVGDimension angle{};
			SVGDimension repeat{};
			
			if (hasAttribute("x1")) {
				x0.loadFromChunk(getAttribute("x1"));
				values.x0 = x0.calculatePixels(w, 0, dpi);
			}

			if (hasAttribute("y1")) {
				y0.loadFromChunk(getAttribute("y1"));
				values.y0 = y0.calculatePixels(h, 0, dpi);
			}

			if (hasAttribute("angle")) {
				// treat the angle as an angle type
				ByteSpan angAttr = getAttribute("angle");
				if (angAttr)
				{
					SVGAngleUnits units;
					parseAngle(angAttr, values.angle, units);
				}
			}

			if (hasAttribute("repeat")) {
				repeat.loadFromChunk(getAttribute("repeat"));
				values.repeat = repeat.calculatePixels(1.0, 0, dpi);
			}
			else if (values.repeat == 0)
				values.repeat = 1.0;
			
			resolveReference(groot, container);
			
			fGradient.setValues(values);
			fGradientVar = fGradient;
		}

	};
}

namespace waavs {
	//============================================================
	// SVGColidColor
	// Represents a single solid color
	// You could represent this as a linear gradient with a single color
	// but this is the way to do it for SVG 2.0
	//============================================================
	struct SVGSolidColorElement : public SVGVisualNode
	{
		static void registerFactory() {
			gShapeCreationMap["solidColor"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGSolidColorElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		SVGPaint fPaint{ nullptr };

		SVGSolidColorElement(IAmGroot* aroot) :SVGVisualNode(aroot) {}


		const BLVar getVariant() noexcept override
		{
			return fPaint.getVariant();
		}

		void resolvePosition(IAmGroot* groot, SVGViewable* container) override
		{
			fPaint.loadFromChunk(getAttribute("solid-color"));

			auto solidopa = getAttribute("solid-opacity");

			double opa{ 0 };
			if (readNumber(solidopa, opa))
			{
				fPaint.setOpacity(opa);
			}
		}

	};

}

