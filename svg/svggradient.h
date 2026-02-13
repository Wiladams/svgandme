#pragma once

//
// Support for SVGGradientElement
// http://www.w3.org/TR/SVG11/feature#Gradient
// linearGradient, radialGradient, conicGradient
//

#include <functional>

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
            (void)blVarToRgba32(&aVar, &colorValue);
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
        static constexpr uint32_t kMaxGradientHrefDepth = 32;

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

        ByteSpan href() const { return fTemplateReference; }
        bool hasHref() const { return !fTemplateReference.empty(); }

        // getVariant()
        //
        // Whomever is using us for paint is calling in here to get
        // our paint variant.  This is the place to construct the thing,
        // if it hasn't already been constructed.
        //
        // It's like a bindToContext essentially, but bindToContext is
        // only called as part of a drawing chain.
        const BLVar getVariant(IRenderSVG *ctx, IAmGroot *groot) noexcept override
        {
            bindToContext(ctx, groot);

            return fGradientVar;

            //BLVar tmpVar{};
            //tmpVar = fGradient;

            //return tmpVar;
        }

        //
        // Properties to inherit:
        // Common properties to inherit
        //   gradientUnits
        //   gradientTransform
        //   spreadMethod
        //
        void setAttributeIfAbsent(const SVGGradient* elem, InternedKey key)
        {
            if (!elem)
                return;
            if (!hasAttribute(key))
            {
                ByteSpan candidateAttr{};
                if (elem->getAttribute(key, candidateAttr))
                    setAttributeByName(key, candidateAttr);
            }
        }


        // The way the inheritance works is, if we don't currently have
        // a value for a particular attribute, but the referred to gradient does
        // then we should take that value from the referred to gradient.
        // 
        // Inherit the raw attributes that are common to all gradients,
        // if we don't already have them.
        void inheritCommonAttributesRaw(const SVGGradient* elem)
        {
            if (!elem)
                return;

            setAttributeIfAbsent(elem, svgattr::gradientUnits());
            setAttributeIfAbsent(elem, svgattr::gradientTransform());
            setAttributeIfAbsent(elem, svgattr::spreadMethod());
        }

        virtual void inheritSameKindProperties(const SVGGradient* elem)
        {
            ;
        }
        
        virtual void inheritProperties(const SVGGradient* elem)
        {
            if (!elem) return;

            // 1) stops: copy stops from referred to only if 
            // we don't have any stops already.
            if (fGradient.size() == 0)
            {
                auto stopsView = elem->fGradient.stopsView();
                if (stopsView.size > 0)
                {
                    fGradient.assignStops(stopsView.data, stopsView.size);
                }
            }

            // 2) Common raw attributes 
            inheritCommonAttributesRaw(elem);

            // 3) type-specific raw attributes, only if
            // same type of gradient.
            if (elem->gradientType() == gradientType())
            {
                inheritSameKindProperties(elem);
            }
        }
        
        // If we have an href, followed the chain of referred to
        // gradients, inheriting raw attributes that are missing 
        // along the way.
        void resolveReferenceChain(IAmGroot* groot)
        {
            if (!groot) return;
            if (!hasHref()) return;
        
            const SVGGradient* cur = this;
            ByteSpan hrefSpan = href();

            // Keep a simple visited list to detect cycles
            const SVGGradient* visited[kMaxGradientHrefDepth]{};
            uint32_t visitedCount = 0;

            // Traverse the chain of references, inheriting attributes
            // and copying stops if we don't have any.
            for (uint32_t depth = 0; depth < kMaxGradientHrefDepth; ++depth)
            {
                if (!hrefSpan) break;

                // Make sure we actually find a node associated with the href
                auto node = groot->findNodeByHref(hrefSpan);
                if (!node) break;

                // Make sure that node is a gradient
                auto gnode = std::dynamic_pointer_cast<SVGGradient>(node);
                if (!gnode) break;

                const SVGGradient* ref = gnode.get();

                // cycle detection (including self)
                bool seen = (ref == this);
                for (uint32_t i = 0; i < visitedCount && !seen; ++i)
                    if (visited[i] == ref) seen = true;

                if (seen)
                {
                    WAAVS_ASSERT(false && "Gradient href cycle detected");
                    break;
                }

                if (visitedCount < kMaxGradientHrefDepth)
                    visited[visitedCount++] = ref;

                // Merge from nearest first: direct reference wins.
                inheritProperties(ref);

                // follow next link in the chain
                hrefSpan = ref->href();   // requires ref to have captured its href in its own fixup/load
            }

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

        void fixupCommonAttributes(IAmGroot* groot)
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
        }

        /*
        void fixupSelfStyleAttributes(IAmGroot *groot) override
        {
            // we should be able resolve any inheritance
            // at this point
            resolveReferenceChain(groot);
        }
        */

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

            setAttributeIfAbsent(elem, svgattr::x1());
            setAttributeIfAbsent(elem, svgattr::y1());
            setAttributeIfAbsent(elem, svgattr::x2());
            setAttributeIfAbsent(elem, svgattr::y2());
        }
        

        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            fixupCommonAttributes(groot);

            // Parse attributes if present
            ByteSpan s;
            if ((s = getAttribute(svgattr::x1()))) { ByteSpan t = s; parseLengthValue(t, x1); }
            if ((s = getAttribute(svgattr::y1()))) { ByteSpan t = s; parseLengthValue(t, y1); }
            if ((s = getAttribute(svgattr::x2()))) { ByteSpan t = s; parseLengthValue(t, x2); }
            if ((s = getAttribute(svgattr::y2()))) { ByteSpan t = s; parseLengthValue(t, y2); }

            resolveReferenceChain(groot);
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
        void inheritSameKindProperties(const SVGGradient* elem) override
        {
            if (!elem)
                return;
            
            setAttributeIfAbsent(elem, svgattr::cx());
            setAttributeIfAbsent(elem, svgattr::cy());
            setAttributeIfAbsent(elem, svgattr::r());
            setAttributeIfAbsent(elem, svgattr::fx());
            setAttributeIfAbsent(elem, svgattr::fy());
        }

        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            fixupCommonAttributes(groot);

            // Parse attributes if present

            resolveReferenceChain(groot);
        }

        void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
        {
            
            double dpi = groot ? groot->dpi() : 96.0;

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
                            values.r0 = calculateDistance(fR.value() * 100, w, h);
                        }
                        else
                            values.r0 = fR.value();
                    }
                    else
                        values.r0 = fR.calculatePixels(w, 0, dpi);

                }
                else
                    values.r0 = (w * 0.50);	// default to half width of object

                values.x1 = values.x0;
                values.y1 = values.y0;
                values.r1 = 0;

                
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


        SVGConicGradient(IAmGroot* aroot) 
            :SVGGradient(aroot)
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

            setAttributeIfAbsent(elem, svgattr::x1());
            setAttributeIfAbsent(elem, svgattr::y1());
            setAttributeIfAbsent(elem, svgattr::angle());
            setAttributeIfAbsent(elem, svgattr::repeat());


        }
        
        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            fixupCommonAttributes(groot);

            // Parse attributes if present

            resolveReferenceChain(groot);
        }


        void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
        {
            double dpi = groot ? groot->dpi() : 96.0;
            double w = groot ? groot->canvasWidth() : 1.0;
            double h = groot ? groot->canvasHeight() : 1.0;

            BLConicGradientValues values = fGradient.conic();

            SVGDimension x0{};
            SVGDimension y0{};
            SVGDimension angle{};
            SVGDimension repeat{};
            
            ByteSpan x1Attr, y1Attr, angleAttr, repeatAttr;
            if (getAttribute(svgattr::x1(), x1Attr)) {
                x0.loadFromChunk(x1Attr);
                values.x0 = x0.calculatePixels(w, 0, dpi);
            }

            if (getAttribute(svgattr::y1(), y1Attr)) {
                y0.loadFromChunk(y1Attr);
                values.y0 = y0.calculatePixels(h, 0, dpi);
            }

            if (getAttribute(svgattr::angle(), angleAttr)) {
                // treat the angle as an angle type
                SVGAngleUnits units;
                parseAngle(angleAttr, values.angle, units);
            }

            if (getAttribute(svgattr::repeat(), repeatAttr)) {
                repeat.loadFromChunk(repeatAttr);
                values.repeat = repeat.calculatePixels(1.0, 0, dpi);
            }
            else if (values.repeat == 0)
                values.repeat = 1.0;

            
            fHasGradientTransform = parseTransform(getAttribute(svgattr::gradientTransform()), fGradientTransform);

            fGradient.setValues(values);
            if (fHasGradientTransform) {
                fGradient.setTransform(fGradientTransform);
            }
            
            fGradientVar = fGradient;
        }

    };
}


