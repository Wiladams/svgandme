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
		BLRgba32 fColor{ 0xff000000 };

		SVGStopNode() :SVGObject() {}

		double offset() const { return fOffset; }
		//double opacity() const { return fOpacity; }
		BLRgba32 color() const { return fColor; }

		void loadFromXmlElement(const XmlElement& elem, IAmGroot* groot)
		{
			// Get the attributes from the element
			ByteSpan attrSpan = elem.data();
			XmlAttributeCollection attrs{};
            scanAttributes(attrs, attrSpan);
			
            ByteSpan styleAttr{}, offsetAttr{}, stopColorAttr{}, stopOpacityAttr{};


			// If there's a style attribute, then add those to the collection
			if (attrs.getValue(svgattr::style(), styleAttr))
			{
				// If we have a style attribute, assume both the stop-color
				// and the stop-opacity are in there
				parseStyleAttribute(styleAttr, attrs);
			}

			// Get the offset
			SVGDimension dim{};
			if (attrs.getValue(svgattr::offset(), offsetAttr))
			{
				dim.loadFromChunk(offsetAttr);
				if (dim.isSet())
				{
					fOffset = dim.calculatePixels(1);
				}
			}


			SVGDimension dimOpacity{};
			SVGPaint paint(groot);

			// Get the stop color
			if (attrs.getValue(svgattr::stop_color(), stopColorAttr))
			{
				paint.loadFromChunk(stopColorAttr);
			}
			else
			{
				// Default color is black
				paint.loadFromChunk("black");
			}
			
			if (attrs.getValue(svgattr::stop_opacity(), stopOpacityAttr))
			{
				dimOpacity.loadFromChunk(stopOpacityAttr);
			}
			else
			{
				// Default opacity is 1.0
				dimOpacity.loadFromChunk("1.0");
			}
			
			double opacity = 1;

			if (dimOpacity.isSet())
			{
				opacity = dimOpacity.calculatePixels(1);
			}

			paint.setOpacity(opacity);

			

			BLVar aVar = paint.getVariant(nullptr, groot);
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
		ByteSpan fTemplateReference{};

		// Some common attributes
		BLExtendMode fSpreadMethod{ BL_EXTEND_MODE_PAD };
		SpaceUnitsKind fGradientUnits{ SVG_SPACE_OBJECT };


		// Constructor
		SVGGradient(IAmGroot* )
			:SVGGraphicsElement()
		{
			fGradient.setExtendMode(BL_EXTEND_MODE_PAD);
			setIsStructural(false);
			setNeedsBinding(true);
		}
		SVGGradient(const SVGGradient& other) = delete;

		SVGGradient operator=(const SVGGradient& other) = delete;

		BLGradientType gradientType() const { return fGradient.type(); }

		// getVariant()
		//
		// Whomever is using us for paint is calling in here to get
		// our paint variant.  This is the place to construct
		// the thing.
		const BLVar getVariant(IRenderSVG *ctx, IAmGroot *groot) noexcept override
		{
			bindToContext(ctx, groot);
			setNeedsBinding(true);
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
		virtual void inheritSameKindProperties(const SVGGradient* elem)
		{
			;
		}
		
		// Inherit the general properties that are always
		// inherited
		//   stops
		//   


		virtual void inheritProperties(const SVGGradient* elem)
		{
			if (!elem)
				return;
			/*
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
			*/

			// If we already have some stops, don't take stops
			// from the referred to gradient
			// If we don't have stops, and the referred to gradient does
			// copy the gradient stops from the referred to gradient
			if (fGradient.size() == 0)
			{
				auto stopsView = elem->fGradient.stopsView();

				if (stopsView.size > 0)
				{
					fGradient.assignStops(stopsView.data, stopsView.size);
				}
			}

			// inherity type specific properties
			if (elem->gradientType() == gradientType())
			{
				inheritSameKindProperties(elem);
			}
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
			if (getAttributeByName("href"))
				fTemplateReference = getAttributeByName("href");
			else if (getAttributeByName("xlink:href"))
				fTemplateReference = getAttributeByName("xlink:href");

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
			registerSVGSingularNodeByName("linearGradient", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGLinearGradient>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("linearGradient",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGLinearGradient>(groot);
					node->loadFromXmlPull(iter, groot);

					return node;
				});

			registerSingularNode();
		}

		SVGLinearGradient(IAmGroot* aroot) :SVGGradient(aroot)
		{
			fGradient.setType(BL_GRADIENT_TYPE_LINEAR);
		}

		// Attributes to inherit from the template if
		// it's also linearGradient
		// Values that are set in this instance override any values
		// that might have been inherited.
		// x1, y1
		// x2, y2
		virtual void inheritSameKindProperties(const SVGGradient* elem) override
		{
			if (!elem)
				return;

			
			if (!getAttributeByName("x1"))
			{
				if (elem->getAttributeByName("x1"))
					setAttribute("x1", elem->getAttributeByName("x1"));
			}

			if (!getAttributeByName("y1"))
			{
				if (elem->getAttributeByName("y1"))
					setAttribute("y1", elem->getAttributeByName("y1"));
			}
			
			if (!getAttributeByName("x2"))
			{
				if (elem->getAttributeByName("x2"))
					setAttribute("x2", elem->getAttributeByName("x2"));
			}

			if (!getAttributeByName("y2"))
			{
				if (elem->getAttributeByName("y2"))
					setAttribute("y2", elem->getAttributeByName("y2"));
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
			
			fX1.loadFromChunk(getAttributeByName("x1"));
			fY1.loadFromChunk(getAttributeByName("y1"));
			fX2.loadFromChunk(getAttributeByName("x2"));
			fY2.loadFromChunk(getAttributeByName("y2"));

			
			// Before we go any further, get our current gradientUnits
			// this should override whatever was set if we had referred to a template
			// if there is NOT a gradientUnits attribute, then we'll either
			// be using the one that came from a template (if there was one)
			// or we'll just be sticking with the default value
			if (getEnumValue(SVGSpreadMethod, getAttributeByName("spreadMethod"), (uint32_t&)fSpreadMethod))
			{
				fGradient.setExtendMode((BLExtendMode)fSpreadMethod);
			}

			getEnumValue(SVGSpaceUnits, getAttributeByName("gradientUnits"), (uint32_t&)fGradientUnits);

			fHasGradientTransform = parseTransform(getAttributeByName("gradientTransform"), fGradientTransform);



			
			if (fGradientUnits == SVG_SPACE_OBJECT )
			{
				BLRect oFrame = ctx->getObjectFrame();
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
			registerSVGSingularNodeByName("radialGradient", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGRadialGradient>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("radialGradient",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGRadialGradient>(groot);
					node->loadFromXmlPull(iter, groot);
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
		virtual void inheritSameKindProperties(const SVGGradient* elem) override
		{
			if (!elem)
				return;
			
			if (!getAttributeByName("cx"))
			{
				if (elem->getAttributeByName("cx"))
					setAttribute("cx", elem->getAttributeByName("cx"));
			}

			if (!getAttributeByName("cy"))
			{
				if (elem->getAttributeByName("cy"))
					setAttribute("cy", elem->getAttributeByName("cy"));
			}

			if (!getAttributeByName("r"))
			{
				if (elem->getAttributeByName("r"))
					setAttribute("r", elem->getAttributeByName("r"));
			}

			if (!getAttributeByName("fx"))
			{
				if (elem->getAttributeByName("fx"))
					setAttribute("fx", elem->getAttributeByName("fx"));
			}

			if (!getAttributeByName("fy"))
			{
				if (elem->getAttributeByName("fy"))
					setAttribute("fy", elem->getAttributeByName("fy"));
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
			
			fCx.loadFromChunk(getAttributeByName("cx"));
			fCy.loadFromChunk(getAttributeByName("cy"));
			fR.loadFromChunk(getAttributeByName("r"));
			fFx.loadFromChunk(getAttributeByName("fx"));
			fFy.loadFromChunk(getAttributeByName("fy"));
			

			// Before we go any further, get our current gradientUnits
			// this should override whatever was set if we had referred to a template
			// if there is NOT a gradientUnits attribute, then we'll either
			// be using the one that came from a template (if there was one)
			// or we'll just be sticking with the default value
			if (getEnumValue(SVGSpreadMethod, getAttributeByName("spreadMethod"), (uint32_t&)fSpreadMethod))
			{
				fGradient.setExtendMode((BLExtendMode)fSpreadMethod);
			}

			getEnumValue(SVGSpaceUnits, getAttributeByName("gradientUnits"), (uint32_t&)fGradientUnits);

			fHasGradientTransform = parseTransform(getAttributeByName("gradientTransform"), fGradientTransform);

			// If the gradientUnits are ObjectBoundingBox, then 
			// the parameters are relative to the size of that box
			if (fGradientUnits == SVG_SPACE_OBJECT)
			{
				BLRect oFrame = ctx->getObjectFrame();
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
			registerSVGSingularNodeByName("conicGradient", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGConicGradient>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		static void registerFactory()
		{
			registerContainerNodeByName("conicGradient",
				[](IAmGroot* groot, XmlPull& iter) {
					auto node = std::make_shared<SVGConicGradient>(groot);
					node->loadFromXmlPull(iter, groot);

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
		virtual void inheritSameKindProperties(const SVGGradient* elem) override
		{
			if (!elem)
				return;

			if (!getAttributeByName("x1"))
			{
				if (elem->getAttributeByName("x1"))
					setAttribute("x1", elem->getAttributeByName("x1"));
			}

			if (!getAttributeByName("y1"))
			{
				if (elem->getAttributeByName("y1"))
					setAttribute("y1", elem->getAttributeByName("y1"));
			}

			if (!getAttributeByName("angle"))
			{
				if (elem->getAttributeByName("angle"))
					setAttribute("angle", elem->getAttributeByName("angle"));
			}
		
			if (!getAttributeByName("repeat"))
			{
				if (elem->getAttributeByName("repeat"))
					setAttribute("repeat", elem->getAttributeByName("repeat"));
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
				x0.loadFromChunk(getAttributeByName("x1"));
				values.x0 = x0.calculatePixels(w, 0, dpi);
			}

			if (hasAttribute("y1")) {
				y0.loadFromChunk(getAttributeByName("y1"));
				values.y0 = y0.calculatePixels(h, 0, dpi);
			}

			if (hasAttribute("angle")) {
				// treat the angle as an angle type
				ByteSpan angAttr = getAttributeByName("angle");
				if (angAttr)
				{
					SVGAngleUnits units;
					parseAngle(angAttr, values.angle, units);
				}
			}

			if (hasAttribute("repeat")) {
				repeat.loadFromChunk(getAttributeByName("repeat"));
				values.repeat = repeat.calculatePixels(1.0, 0, dpi);
			}
			else if (values.repeat == 0)
				values.repeat = 1.0;
			
			fHasGradientTransform = parseTransform(getAttributeByName("gradientTransform"), fGradientTransform);

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
			registerSVGSingularNodeByName("solidColor", [](IAmGroot* groot, const XmlElement& elem) {
				auto node = std::make_shared<SVGSolidColorElement>(groot);
				node->loadFromXmlElement(elem, groot);

				return node;
				});
		}

		SVGPaint fPaint{ nullptr };

		SVGSolidColorElement(IAmGroot* ) :SVGGraphicsElement() {}


		const BLVar getVariant(IRenderSVG* ctx, IAmGroot* groot) noexcept override
		{
			return fPaint.getVariant(ctx, groot);
		}

		void bindSelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
		{
			fPaint.loadFromChunk(getAttributeByName("solid-color"));

			auto solidopa = getAttributeByName("solid-opacity");

			double opa{ 0 };
			if (readNumber(solidopa, opa))
			{
				fPaint.setOpacity(opa);
			}
		}

	};

}

