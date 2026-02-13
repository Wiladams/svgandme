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
    // Helpers to resolve gradient coordinates, and to make the bbox to user transform
    
    // For gradientUnits="objectBoundingBox":
    // Return coordinate in bbox-space (0..1 typical, but allow outside).

    static INLINE double resolveLengthBBoxUnits(const SVGLengthValue& L, double fallback) noexcept
    {
        if (!L.isSet())
            return fallback;

        if (L.fUnitType == SVG_LENGTHTYPE_PERCENTAGE)
            return L.fValue / 100.0;

        // For objectBoundingBox gradients, NUMBER is already bbox-space.
        // (Other unit types should generally not appear here; treat as NUMBER or reject upstream.)
        return L.fValue;
    }

    static INLINE BLMatrix2D makeBBoxToUserTransform(const BLRect& b) noexcept
    {
        BLMatrix2D m = BLMatrix2D::makeIdentity();
        m.translate(b.x, b.y);
        m.scale(b.w, b.h);
        return m;
    }

    static INLINE BLMatrix2D composeGradientTransformBBox(const BLRect& b, bool hasGT, const BLMatrix2D& GT) noexcept
    {
        BLMatrix2D m = makeBBoxToUserTransform(b);
        if (hasGT)
            m.transform(GT);   // m = m * GT  (Blend2D’s transform() post-multiplies)
        return m;
    }

}

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
        BLRgba32 color() const { return fColor; }

        void loadFromXmlElement(const XmlElement& elem, IAmGroot* groot)
        {
            // Get the attributes from the element
            ByteSpan attrSpan = elem.data();
            XmlAttributeCollection attrs{};
            scanAttributes(attrs, attrSpan);
            
            ByteSpan styleAttr{}, offsetAttr{}, stopColorAttr{};


            // If there's a style attribute, then add those to the collection
            if (attrs.getValue(svgattr::style(), styleAttr))
            {
                // If we have a style attribute, both the stop-color
                // and the stop-opacity could be in there
                parseStyleAttribute(styleAttr, attrs);
            }

            // Get the offset
            if (attrs.getValue(svgattr::offset(), offsetAttr))
            {
                SVGNumberOrPercent op{};
                ByteSpan s = offsetAttr;
                if (readSVGNumberOrPercent(s, op))
                {
                    fOffset = op.calculatedValue();
                    if (fOffset < 0.0 || fOffset > 1.0)
                        WAAVS_ASSERT(false && "Gradient stop offset value out of range (should be between 0 and 1)");
                    //fOffset = waavs::clamp(op.calculatedValue(), 0.0, 1.0);
                }

            }

            // Get the stop opacity
            double opacity = 1.0;
            ByteSpan stopOpacityAttr{};

            if (attrs.getValue(svgattr::stop_opacity(), stopOpacityAttr))
            {
                SVGNumberOrPercent op{};
                ByteSpan s = stopOpacityAttr;
                if (readSVGNumberOrPercent(s, op))
                {
                    opacity = waavs::clamp(op.calculatedValue(), 0.0, 1.0);
                }
            }

            // Get the stop color, and incorporate the opacity
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
        BLMatrix2D fGradientTransform{};
        bool fHasGradientTransform = false;

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
            setIsStructural(true);
            setIsVisible(false);
            setNeedsBinding(true);
        }

        // We want to catch when copy construction or
        // assignment are occuring
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

            BLVar tmpVar{};
            tmpVar = fGradient;

            return tmpVar;
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
        void resolveReference(IAmGroot* groot)
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

            
            //gnode->bindToContext(ctx, groot);
            inheritProperties(gnode.get());
        }



        //
        // The only nodes here should be stop nodes
        //
        void loadSelfClosingNode(const XmlElement& elem, IAmGroot* groot) override
        {
            if (elem.nameAtom() != svgtag::tag_stop())
            {
                return;
            }

            SVGStopNode stopnode{};
            stopnode.loadFromXmlElement(elem, groot);

            auto offset = stopnode.offset();
            auto acolor = stopnode.color();

            fGradient.addStop(offset, acolor);

        }

        void fixupSelfStyleAttributes(IAmGroot *groot) override
        {
            // spreadMethod / gradientUnits / gradientTransform
            if (getEnumValue(SVGSpreadMethod, getAttribute(svgattr::spreadMethod()), (uint32_t&)fSpreadMethod))
                fGradient.setExtendMode((BLExtendMode)fSpreadMethod);

            getEnumValue(SVGSpaceUnits, getAttribute(svgattr::gradientUnits()), (uint32_t&)fGradientUnits);

            fHasGradientTransform = parseTransform(getAttribute(svgattr::gradientTransform()), fGradientTransform);


            // See if we have a template reference
            if (getAttribute(svgattr::href()))
                fTemplateReference = getAttribute(svgattr::href());
            else if (getAttributeByName(svgattr::xlink_href()))
                fTemplateReference = getAttributeByName(svgattr::xlink_href());

            // we should be able resolve any inheritance
            // at this point
            resolveReference(groot);
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


        // defaults per SVG:
        // x1 = 0%, y1 = 0%,, x2 = 100%, y2 = 0%
        SVGLengthValue x1{ 0.0, SVG_LENGTHTYPE_PERCENTAGE, false };
        SVGLengthValue y1{ 0.0, SVG_LENGTHTYPE_PERCENTAGE, false };
        SVGLengthValue x2{ 100.0, SVG_LENGTHTYPE_PERCENTAGE, false };
        SVGLengthValue y2{ 0.0, SVG_LENGTHTYPE_PERCENTAGE, false };


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

            // The way the inheritance works is, if we don't currently have
            // a value for a particular attribute, but the referred to gradient does
            // then we should take that value from the referred to gradient.
            if (!getAttributeByName(svgattr::x1()))
            {
                if (elem->getAttributeByName(svgattr::x1()))
                    setAttributeByName("x1", elem->getAttributeByName(svgattr::x1()));
            }

            if (!getAttributeByName(svgattr::y1()))
            {
                if (elem->getAttributeByName(svgattr::y1()))
                    setAttributeByName("y1", elem->getAttributeByName(svgattr::y1()));
            }
            
            if (!getAttributeByName(svgattr::x2()))
            {
                if (elem->getAttributeByName(svgattr::x2()))
                    setAttributeByName("x2", elem->getAttributeByName(svgattr::x2()));
            }

            if (!getAttributeByName(svgattr::y2()))
            {
                if (elem->getAttributeByName(svgattr::y2()))
                    setAttributeByName("y2", elem->getAttributeByName(svgattr::y2()));
            }
            
        }
        

        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            // Parse attributes if present
            ByteSpan s;
            if ((s = getAttribute(svgattr::x1()))) { ByteSpan t = s; parseLengthValue(t, x1); }
            if ((s = getAttribute(svgattr::y1()))) { ByteSpan t = s; parseLengthValue(t, y1); }
            if ((s = getAttribute(svgattr::x2()))) { ByteSpan t = s; parseLengthValue(t, x2); }
            if ((s = getAttribute(svgattr::y2()))) { ByteSpan t = s; parseLengthValue(t, y2); }

            SVGGradient::fixupSelfStyleAttributes(groot);
        }


        void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
        {
            double dpi = groot ? groot->dpi() : 96.0;



            // Start setting up the gradient
            BLLinearGradientValues values{ 0,0,1,0 };
            BLMatrix2D xform = BLMatrix2D::makeIdentity();

            if (fGradientUnits == SVG_SPACE_USER)
            {
                const BLRect vp = ctx->viewport();

                LengthResolveCtx rx{ dpi, nullptr, vp.w, 0.0, SpaceUnitsKind::SVG_SPACE_USER };
                LengthResolveCtx ry{ dpi, nullptr, vp.h, 0.0, SpaceUnitsKind::SVG_SPACE_USER };

                // defaults if not set
                values.x0 = x1.isSet() ? resolveLengthUserUnits(x1, rx) : 0.0;
                values.y0 = y1.isSet() ? resolveLengthUserUnits(y1, ry) : 0.0;
                values.x1 = x2.isSet() ? resolveLengthUserUnits(x2, rx) : vp.w;
                values.y1 = y2.isSet() ? resolveLengthUserUnits(y2, ry) : 0.0;

                if (fHasGradientTransform)
                    xform = fGradientTransform;

            }
            else // SVG_SPACE_OBJECT (objectBoundingBox)
            {
                const BLRect b = ctx->getObjectFrame();

                // resolve into bbox space (0..1),
                // then use transform to map bbox -> user
                values.x0 = resolveLengthBBoxUnits(x1, 0.0);
                values.y0 = resolveLengthBBoxUnits(y1, 0.0);
                values.x1 = resolveLengthBBoxUnits(x2, 1.0);
                values.y1 = resolveLengthBBoxUnits(y2, 0.0);

                xform = composeGradientTransformBBox(b, fHasGradientTransform, fGradientTransform);
            }

            fGradient.setValues(values);
            fGradient.setTransform(xform);
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
    //
    // calculateDistance()
    // 
    // To calculate distances when using a percentage value on a radius of something
    //
    static INLINE double calculateDistance(const double percentage, const double width, const double height) noexcept
    {
        return percentage / 100.0 * std::sqrt((width * width) + (height * height));
    }


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
                    setAttributeByName("cx", elem->getAttributeByName("cx"));
            }

            if (!getAttributeByName("cy"))
            {
                if (elem->getAttributeByName("cy"))
                    setAttributeByName("cy", elem->getAttributeByName("cy"));
            }

            if (!getAttributeByName("r"))
            {
                if (elem->getAttributeByName("r"))
                    setAttributeByName("r", elem->getAttributeByName("r"));
            }

            if (!getAttributeByName("fx"))
            {
                if (elem->getAttributeByName("fx"))
                    setAttributeByName("fx", elem->getAttributeByName("fx"));
            }

            if (!getAttributeByName("fy"))
            {
                if (elem->getAttributeByName("fy"))
                    setAttributeByName("fy", elem->getAttributeByName("fy"));
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
                    setAttributeByName("x1", elem->getAttributeByName("x1"));
            }

            if (!getAttributeByName("y1"))
            {
                if (elem->getAttributeByName("y1"))
                    setAttributeByName("y1", elem->getAttributeByName("y1"));
            }

            if (!getAttributeByName("angle"))
            {
                if (elem->getAttributeByName("angle"))
                    setAttributeByName("angle", elem->getAttributeByName("angle"));
            }
        
            if (!getAttributeByName("repeat"))
            {
                if (elem->getAttributeByName("repeat"))
                    setAttributeByName("repeat", elem->getAttributeByName("repeat"));
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
            
            if (hasAttributeByName("x1")) {
                x0.loadFromChunk(getAttributeByName("x1"));
                values.x0 = x0.calculatePixels(w, 0, dpi);
            }

            if (hasAttributeByName("y1")) {
                y0.loadFromChunk(getAttributeByName("y1"));
                values.y0 = y0.calculatePixels(h, 0, dpi);
            }

            if (hasAttributeByName("angle")) {
                // treat the angle as an angle type
                ByteSpan angAttr = getAttributeByName("angle");
                if (angAttr)
                {
                    SVGAngleUnits units;
                    parseAngle(angAttr, values.angle, units);
                }
            }

            if (hasAttributeByName("repeat")) {
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


