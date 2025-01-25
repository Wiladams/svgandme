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


	// Default values
	// offset == 0
	// color == black
	// opacity == 1.0
	struct SVGStopNode : public SVGObject
	{
		double fOffset = 0;
		double fOpacity = 1;
		BLRgba32 fColor{ 0xff000000 };

		SVGStopNode() :SVGObject() {}

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

			

			BLVar aVar = paint.getVariant(nullptr, nullptr);
			uint32_t colorValue = 0;
			auto res = blVarToRgba32(&aVar, &colorValue);
			fColor.value = colorValue;

		}
		
		void bindToContext(IRenderSVG*, IAmGroot*) noexcept override
		{
			// nothing to see here, part of SVGObject abstract
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
		BLExtendMode fSpreadMethod{ BL_EXTEND_MODE_PAD };
		SpaceUnitsKind fGradientUnits{ SVG_SPACE_OBJECT };
		ByteSpan fTemplateReference{};


		// Constructor
		SVGGradient(IAmGroot* )
			:SVGGraphicsElement()
		{
			fGradient.setExtendMode(BL_EXTEND_MODE_PAD);
			isStructural(false);
			needsBinding(true);
		}
		SVGGradient(const SVGGradient& other) = delete;

		SVGGradient operator=(const SVGGradient& other) = delete;

		// getVariant()
		//
		// Whomever is using us for paint is calling in here to get
		// our paint variant.  This is the place to construct
		// the thing.
		const BLVar getVariant(IRenderSVG *ctx, IAmGroot *groot) noexcept override
		{
			bindToContext(ctx, groot);
			needsBinding(true);
			BLVar tmpVar{};
			tmpVar = fGradient;
			return tmpVar;
			
			/*
			// If we've never gone through, load the gradient
			if (needsBinding() || fGradientVar.isNull())
			{
				bindToContext(ctx, groot);
				fGradientVar = fGradient;
			}
			
			
			// Now that we know what we have
			if (fGradientUnits == SVG_SPACE_OBJECT) {
				BLVar tmpVar{};
				tmpVar = fGradient;
				needsBinding(true);
				
				return tmpVar;
			}

			return fGradientVar;
			*/
		}

		//
		// Properties to inherit:
		// Common properties to inherit
		//   gradientUnits
		//   gradientTransform
		//   spreadMethod
		//
		virtual void inheritSelfProperties(const SVGGradient* elem)
		{
			;
		}
		
		virtual void inheritProperties(const SVGGradient* elem)
		{
			if (!elem)
				return;

			if (!getAttribute("gradientUnits"))
			{
				if (elem->getAttribute("gradientUnits"))
					setAttribute("gradientUnits", elem->getAttribute("gradientUnits"));
			}

			if (!getAttribute("gradientTransform"))
			{
				if (elem->getAttribute("gradientTransform"))
					setAttribute("gradientTransform", elem->getAttribute("gradientTransform"));
			}

			if (!getAttribute("spreadMethod"))
			{
				if (elem->getAttribute("spreadMethod"))
					setAttribute("spreadMethod", elem->getAttribute("spreadMethod"));
			}

			// If we already have some stops, don't take stops
			// from the referred to gradient
			// If we don't have stops, and the referred to gradient does
			// copy the gradient stops from the referred to gradient
			if (fGradient.size() == 0)
			{
				if (elem->fGradient.size() > 0)
				{
					fGradient.reset();
					fGradient.assignStops(elem->fGradient.stops(), elem->fGradient.size());
				}
			}

			
			inheritSelfProperties(elem);
		}
		
		// resolveReference()
		// 
		// We want to copy all relevant data from the reference.
		// Most importantly, we need the color stops.
		// then we need the coordinate system (userspace or bounding box
		//
		void resolveReference(IRenderSVG* ctx, IAmGroot* groot)
		{
			// return early if we can't do a lookup
			if (!groot)
				return;

			// Quick return if there is no referred to pattern
			if (!fTemplateReference)
				return;

			// Try to find the referred to pattern
			auto node = groot->findNodeByHref(fTemplateReference);

			// Didn't find the node, so return empty handed
			if (!node)
				return;



			// try to cast to SVGGradient
			auto gnode = std::dynamic_pointer_cast<SVGGradient>(node);

			// if we can't cast, then we can't resolve the reference
			// so return immediately
			if (!gnode)
				return;

			
			gnode->bindToContext(ctx, groot);
			inheritProperties(gnode.get());

		}



		//
		// The only nodes here should be stop nodes
		//
		void loadSelfClosingNode(const XmlElement& elem, IAmGroot* groot) override
		{
			if (elem.name() != "stop")
			{
				return;
			}

			SVGStopNode stopnode{};
			stopnode.loadFromXmlElement(elem, groot);

			auto offset = stopnode.offset();
			auto acolor = stopnode.color();

			fGradient.addStop(offset, acolor);

		}

		void fixupSelfStyleAttributes(IRenderSVG *ctx, IAmGroot *groot) override
		{
			// See if we have a template reference
			if (getAttribute("href"))
				fTemplateReference = getAttribute("href");
			else if (getAttribute("xlink:href"))
				fTemplateReference = getAttribute("xlink:href");

			resolveReference(ctx, groot);
		}
		

	};

	//=======================================
	// SVGLinearGradient
	//
	struct SVGLinearGradient : public SVGGradient
	{
		static void registerSingularNode()
		{
			getSVGSingularCreationMap()["linearGradient"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGLinearGradient>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			registerContainerNode("linearGradient",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGLinearGradient>(groot);
					node->loadFromXmlIterator(iter, groot);

					return node;
				});

			registerSingularNode();
		}

		SVGLinearGradient(IAmGroot* aroot) :SVGGradient(aroot)
		{
			fGradient.setType(BL_GRADIENT_TYPE_LINEAR);
		}

		// Attributes to inherit
		// x1, y1
		// x2, y2
		virtual void inheritSelfProperties(const SVGGradient* elem) override
		{
			if (!elem)
				return;

			
			if (!getAttribute("x1"))
			{
				if (elem->getAttribute("x1"))
					setAttribute("x1", elem->getAttribute("x1"));
			}

			if (!getAttribute("y1"))
			{
				if (elem->getAttribute("y1"))
					setAttribute("y1", elem->getAttribute("y1"));
			}
			
			if (!getAttribute("x2"))
			{
				if (elem->getAttribute("x2"))
					setAttribute("x2", elem->getAttribute("x2"));
			}

			if (!getAttribute("y2"))
			{
				if (elem->getAttribute("y2"))
					setAttribute("y2", elem->getAttribute("y2"));
			}
			
		}
		
		void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
		{
			double dpi = 96;

			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}

			
			// Get current values, in case we've already
			// loaded from a template
			BLLinearGradientValues values{ 0,0,0,1 };// = fGradient.linear();

			SVGDimension fX1; //{ 0, SVG_LENGTHTYPE_PERCENTAGE };
			SVGDimension fY1; //{ 0, SVG_LENGTHTYPE_NUMBER };
			SVGDimension fX2; //{ 100, SVG_LENGTHTYPE_PERCENTAGE };
			SVGDimension fY2; //{ 0, SVG_LENGTHTYPE_PERCENTAGE };
			
			fX1.loadFromChunk(getAttribute("x1"));
			fY1.loadFromChunk(getAttribute("y1"));
			fX2.loadFromChunk(getAttribute("x2"));
			fY2.loadFromChunk(getAttribute("y2"));

			
			// Before we go any further, get our current gradientUnits
			// this should override whatever was set if we had referred to a template
			// if there is NOT a gradientUnits attribute, then we'll either
			// be using the one that came from a template (if there was one)
			// or we'll just be sticking with the default value
			if (getEnumValue(SVGSpreadMethod, getAttribute("spreadMethod"), (uint32_t&)fSpreadMethod))
			{
				fGradient.setExtendMode((BLExtendMode)fSpreadMethod);
			}

			getEnumValue(SVGSpaceUnits, getAttribute("gradientUnits"), (uint32_t&)fGradientUnits);

			fHasGradientTransform = parseTransform(getAttribute("gradientTransform"), fGradientTransform);



			
			if (fGradientUnits == SVG_SPACE_OBJECT )
			{
				BLRect oFrame = ctx->objectFrame();
				double w = oFrame.w;
				double h = oFrame.h;
				auto x = oFrame.x;
				auto y = oFrame.y;
				
				if (fX1.isSet()) {
					if (fX1.fUnits == SVG_LENGTHTYPE_NUMBER) {
						if (fX1.value() <= 1.0)
							values.x0 = x + (fX1.value() * w);
						else
							values.x0 = x + fX1.value();
					} else
						values.x0 = x + fX1.calculatePixels(w, 0, dpi);

				} else
					values.x0 = x + 0;

				if (fY1.isSet()) {
					if (fY1.fUnits == SVG_LENGTHTYPE_NUMBER) {
						if (fY1.value() <= 1.0)
							values.y0 = y + (fY1.value() * h);
						else
							values.y0 = y + fY1.value();
					} else 
						values.y0 = y + fY1.calculatePixels(h, 0, dpi);
				} else
					values.y0 = y + 0;


				if (fX2.isSet()) {
					if (fX2.fUnits == SVG_LENGTHTYPE_NUMBER) {
						if (fX2.value() <= 1.0)
							values.x1 = x + (fX2.value() * w);
						else
							values.x1 = x + fX2.value();
					} else
						values.x1 = x + fX2.calculatePixels(w, 0, dpi);
				} else
					values.x1 = x + w;
				
				if (fY2.isSet()) {
					if (fY2.fUnits == SVG_LENGTHTYPE_NUMBER) {
						if (fY2.value() <= 1.0)
							values.y1 = y + (fY2.value() * h);
						else
							values.y1 = y + fY2.value();
					} else
						values.y1 = y + fY2.calculatePixels(h, 0, dpi);
				} else
					values.y1 = y + 0;

			}
			else if (fGradientUnits == SVG_SPACE_USER)
			{
				BLRect lFrame = ctx->viewport();
				double w = lFrame.w;
				double h = lFrame.h;
				
				if (fX1.isSet())
					values.x0 = fX1.calculatePixels(w, 0, dpi);
				if (fY1.isSet())
					values.y0 = fY1.calculatePixels(h, 0, dpi);
				if (fX2.isSet())
					values.x1 = fX2.calculatePixels(w, 0, dpi);
				if (fY2.isSet())
					values.y1 = fY2.calculatePixels(h, 0, dpi);
			}


			


			


			
			fGradient.setValues(values);
			if (fHasGradientTransform) {
				fGradient.setTransform(fGradientTransform);
			}
			
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
			getSVGSingularCreationMap()["radialGradient"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGRadialGradient>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			registerContainerNode("radialGradient",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGRadialGradient>(groot);
					node->loadFromXmlIterator(iter, groot);
					return node;
				});
			


			registerSingularNode();
		}



		SVGRadialGradient(IAmGroot* groot) :SVGGradient(groot)
		{
			fGradient.setType(BL_GRADIENT_TYPE_RADIAL);
		}

		// Attributes to inherit
		// cx, cy
		// r
		// fx, fy
		virtual void inheritSelfProperties(const SVGGradient* elem) override
		{
			if (!elem)
				return;
			
			if (!getAttribute("cx"))
			{
				if (elem->getAttribute("cx"))
					setAttribute("cx", elem->getAttribute("cx"));
			}

			if (!getAttribute("cy"))
			{
				if (elem->getAttribute("cy"))
					setAttribute("cy", elem->getAttribute("cy"));
			}

			if (!getAttribute("r"))
			{
				if (elem->getAttribute("r"))
					setAttribute("r", elem->getAttribute("r"));
			}

			if (!getAttribute("fx"))
			{
				if (elem->getAttribute("fx"))
					setAttribute("fx", elem->getAttribute("fx"));
			}

			if (!getAttribute("fy"))
			{
				if (elem->getAttribute("fy"))
					setAttribute("fy", elem->getAttribute("fy"));
			}
		}

		
		void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
		{
			
			double dpi = 96;
			if (nullptr != groot)
			{
				dpi = groot->dpi();
			}


			BLRadialGradientValues values = fGradient.radial();

			SVGDimension fCx;	// { 50, SVG_LENGTHTYPE_PERCENTAGE };
			SVGDimension fCy;	//  { 50, SVG_LENGTHTYPE_PERCENTAGE };
			SVGDimension fR;	// { 50, SVG_LENGTHTYPE_PERCENTAGE };
			SVGDimension fFx;	// { 50, SVG_LENGTHTYPE_PERCENTAGE, false };
			SVGDimension fFy;	// { 50, SVG_LENGTHTYPE_PERCENTAGE, false };
			
			fCx.loadFromChunk(getAttribute("cx"));
			fCy.loadFromChunk(getAttribute("cy"));
			fR.loadFromChunk(getAttribute("r"));
			fFx.loadFromChunk(getAttribute("fx"));
			fFy.loadFromChunk(getAttribute("fy"));
			

			// Before we go any further, get our current gradientUnits
			// this should override whatever was set if we had referred to a template
			// if there is NOT a gradientUnits attribute, then we'll either
			// be using the one that came from a template (if there was one)
			// or we'll just be sticking with the default value
			if (getEnumValue(SVGSpreadMethod, getAttribute("spreadMethod"), (uint32_t&)fSpreadMethod))
			{
				fGradient.setExtendMode((BLExtendMode)fSpreadMethod);
			}

			getEnumValue(SVGSpaceUnits, getAttribute("gradientUnits"), (uint32_t&)fGradientUnits);

			fHasGradientTransform = parseTransform(getAttribute("gradientTransform"), fGradientTransform);

			
			if (fGradientUnits == SVG_SPACE_OBJECT)
			{
				BLRect oFrame = ctx->objectFrame();
				auto w = oFrame.w;
				auto h = oFrame.h;
				auto x = oFrame.x;
				auto y = oFrame.y;
				
				if (fCx.isSet()) {
					if (fCx.fUnits == SVG_LENGTHTYPE_NUMBER) {
						if (fCx.value() <= 1.0)
							values.x0 = x + (fCx.value() * w);
						else
							values.x0 = x + fCx.value();
					}
					else
						values.x0 = x + fCx.calculatePixels(w, 0, dpi);

				}
				else
					values.x0 = x + (w*0.50);	// default to center of object

				if (fCy.isSet()) {
					if (fCy.fUnits == SVG_LENGTHTYPE_NUMBER) {
						if (fCy.value() <= 1.0)
							values.y0 = y + (fCy.value() * h);
						else
							values.y0 = y + fCy.value();
					}
					else
						values.y0 = y + fCy.calculatePixels(h, 0, dpi);

				}
				else
					values.y0 = y + (h * 0.50);	// default to center of object
				
				
				if (fR.isSet()) {
					if (fR.fUnits == SVG_LENGTHTYPE_NUMBER) {
						if (fR.value() <= 1.0) {
							values.r1 = calculateDistance(fR.value() * 100, w, h);
						}
						else
							values.r1 = fR.value();
					}
					else
						values.r1 = fR.calculatePixels(w, 0, dpi);

				}
				else
					values.r1 = (w * 0.50);	// default to center of object

				values.x1 = values.x0;
				values.y1 = values.y0;
				values.r0 = 0;

				
				if (fFx.isSet()) {
					if (fFx.fUnits == SVG_LENGTHTYPE_NUMBER) {
						if (fFx.value() <= 1.0)
							values.x1 = x + (fFx.value() * w);
						else
							values.x1 = x + fFx.value();
					}
					else
						values.x1 = x + fFx.calculatePixels(w, 0, dpi);
				}
				else
					values.x1 = values.x0;

				
				if (fFy.isSet()) {
					if (fFy.fUnits == SVG_LENGTHTYPE_NUMBER) {
						if (fFy.value() <= 1.0)
							values.y1 = y + (fFy.value() * h);
						else
							values.y1 = y + fFy.value();
					}
					else
						values.y1 = y + fFy.calculatePixels(h, 0, dpi);

				}
				else
					values.y1 = values.y0;

			}
			else if (fGradientUnits == SVG_SPACE_USER)
			{
				BLRect lFrame = ctx->viewport();
				double w = lFrame.w;
				double h = lFrame.h;
				
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

			}
			

			
			fGradient.setValues(values);
			
			if (fHasGradientTransform) {
				fGradient.setTransform(fGradientTransform);
			}
			
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
			getSVGSingularCreationMap()["conicGradient"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGConicGradient>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		static void registerFactory()
		{
			registerContainerNode("conicGradient",
				[](IAmGroot* groot, XmlElementIterator& iter) {
					auto node = std::make_shared<SVGConicGradient>(groot);
					node->loadFromXmlIterator(iter, groot);

					return node;
				});
			

			registerSingularNode();
		}




		SVGConicGradient(IAmGroot* aroot) :SVGGradient(aroot)
		{
			fGradient.setType(BLGradientType::BL_GRADIENT_TYPE_CONIC);
		}

		// Attributes to inherit
		// x1, y1
		// angle, repeat
		//
		virtual void inheritSelfProperties(const SVGGradient* elem) override
		{
			if (!elem)
				return;

			if (!getAttribute("x1"))
			{
				if (elem->getAttribute("x1"))
					setAttribute("x1", elem->getAttribute("x1"));
			}

			if (!getAttribute("y1"))
			{
				if (elem->getAttribute("y1"))
					setAttribute("y1", elem->getAttribute("y1"));
			}

			if (!getAttribute("angle"))
			{
				if (elem->getAttribute("angle"))
					setAttribute("angle", elem->getAttribute("angle"));
			}
		
			if (!getAttribute("repeat"))
			{
				if (elem->getAttribute("repeat"))
					setAttribute("repeat", elem->getAttribute("repeat"));
			}
		}
		
		void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
		{

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
			
			fHasGradientTransform = parseTransform(getAttribute("gradientTransform"), fGradientTransform);

			fGradient.setValues(values);
			if (fHasGradientTransform) {
				fGradient.setTransform(fGradientTransform);
			}
			
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
	struct SVGSolidColorElement : public SVGGraphicsElement
	{
		static void registerFactory() {
			getSVGSingularCreationMap()["solidColor"] = [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGSolidColorElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				};
		}

		SVGPaint fPaint{ nullptr };

		SVGSolidColorElement(IAmGroot* ) :SVGGraphicsElement() {}


		const BLVar getVariant(IRenderSVG* ctx, IAmGroot* groot) noexcept override
		{
			return fPaint.getVariant(ctx, groot);
		}

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
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

