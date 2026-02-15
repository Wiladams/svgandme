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

            // If a stop-opacity is specified, then apply that to the paint
            // regardless of how it was constructed.
            ByteSpan stopOpacityAttr{};
            if (attrs.getValue(svgattr::stop_opacity(), stopOpacityAttr))
            {
                stopOpacityAttr = chunk_ltrim(stopOpacityAttr, chrWspChars);
                if (!stopOpacityAttr.empty())
                {
                    double opacity = 1.0;
                    SVGNumberOrPercent op{};
                    ByteSpan s = stopOpacityAttr;
                    if (readSVGNumberOrPercent(s, op))
                    {
                        opacity = waavs::clamp(op.calculatedValue(), 0.0, 1.0);
                    }

                    paint.setOpacity(opacity);
                }
            }

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

            setAttributeIfAbsent(elem, svgattr::x1());
            setAttributeIfAbsent(elem, svgattr::y1());
            setAttributeIfAbsent(elem, svgattr::x2());
            setAttributeIfAbsent(elem, svgattr::y2());
        }
        

        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            fixupCommonAttributes(groot);

            resolveReferenceChain(groot);
        }


        void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
        {
            double dpi = groot ? groot->dpi() : 96.0;

            // Start setting up the gradient
            BLLinearGradientValues values{ 0,0,1,0 };
            BLMatrix2D xform = BLMatrix2D::makeIdentity();

            // Parse attributes if present
            // defaults per SVG:
            // x1 = 0%, y1 = 0%,, x2 = 100%, y2 = 0%
            SVGLengthValue x1{ 0.0, SVG_LENGTHTYPE_PERCENTAGE, false };
            SVGLengthValue y1{ 0.0, SVG_LENGTHTYPE_PERCENTAGE, false };
            SVGLengthValue x2{ 100.0, SVG_LENGTHTYPE_PERCENTAGE, false };
            SVGLengthValue y2{ 0.0, SVG_LENGTHTYPE_PERCENTAGE, false };


            ByteSpan s;
            if ((s = getAttribute(svgattr::x1()))) { ByteSpan t = s; parseLengthValue(t, x1); }
            if ((s = getAttribute(svgattr::y1()))) { ByteSpan t = s; parseLengthValue(t, y1); }
            if ((s = getAttribute(svgattr::x2()))) { ByteSpan t = s; parseLengthValue(t, x2); }
            if ((s = getAttribute(svgattr::y2()))) { ByteSpan t = s; parseLengthValue(t, y2); }



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
                const BLRect objFrame = ctx->getObjectFrame();

                // resolve into bbox space (0..1),
                // then use transform to map bbox -> user
                values.x0 = resolveLengthBBoxUnits(x1, 0.0);
                values.y0 = resolveLengthBBoxUnits(y1, 0.0);
                values.x1 = resolveLengthBBoxUnits(x2, 1.0);
                values.y1 = resolveLengthBBoxUnits(y2, 0.0);

                xform = composeGradientTransformBBox(objFrame, fHasGradientTransform, fGradientTransform);
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
    static INLINE double calculateDistance(const double fraction, const double width, const double height) noexcept
    {
        return fraction  * std::sqrt((width * width) + (height * height));
        
        // What the browsers do
        //return fraction * width;
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

        // Attributes as authored
        SVGLengthValue fCx{ };
        SVGLengthValue fCy{};
        SVGLengthValue fR{};
        SVGLengthValue fFx{};
        SVGLengthValue fFy{};
        SVGLengthValue fFr{};


        SVGRadialGradient(IAmGroot* groot) :SVGGradient(groot)
        {
            fGradient.setType(BL_GRADIENT_TYPE_RADIAL);
        }

        // Attributes to inherit
        // cx, cy, r
        // fx, fy, focal-radius
        //
        void inheritSameKindProperties(const SVGGradient* elem) override
        {
            if (!elem)
                return;
            
            setAttributeIfAbsent(elem, svgattr::cx());
            setAttributeIfAbsent(elem, svgattr::cy());
            setAttributeIfAbsent(elem, svgattr::r());
            setAttributeIfAbsent(elem, svgattr::fx());
            setAttributeIfAbsent(elem, svgattr::fy());
            setAttributeIfAbsent(elem, svgattr::fr());
        }

        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            fixupCommonAttributes(groot);

            resolveReferenceChain(groot);

            // Parse our own attributes after they've been 
            // inherited and resolved.
            parseLengthValue(getAttribute(svgattr::cx()), fCx);
            parseLengthValue(getAttribute(svgattr::cy()), fCy);
            parseLengthValue(getAttribute(svgattr::r()), fR);
            
            parseLengthValue(getAttribute(svgattr::fx()), fFx);
            parseLengthValue(getAttribute(svgattr::fy()), fFy);
            parseLengthValue(getAttribute(svgattr::fr()), fFr);
        }

        //Available if we want to be spec 1.1 compliant
        // The focal point will be clamped to the outer circle

        static INLINE void clampFocalPointToOuterCircle(BLRadialGradientValues& v) noexcept
        {
            // Only makes sense if r0 is positive.
            if (!(v.r0 > 0.0))
                return;

            const double dx = v.x1 - v.x0;
            const double dy = v.y1 - v.y0;
            const double d2 = dx * dx + dy * dy;
            const double r2 = v.r0 * v.r0;

            // If focal is outside outer circle, clamp it onto the circle boundary.
            if (d2 > r2) {
                const double d = std::sqrt(d2);
                // d can't be 0 here because d2 > r2 and r0 > 0
                const double s = v.r0 / d;
                v.x1 = v.x0 + dx * s;
                v.y1 = v.y0 + dy * s;
            }
        }

        void bindSelfToContext(IRenderSVG *ctx, IAmGroot* groot) override
        {
            double dpi = groot ? groot->dpi() : 96.0;

            BLRadialGradientValues values = fGradient.radial();
            
            // Before we go any further, get our current gradientUnits
            // default is objectBoundingBox
            getEnumValue(SVGSpaceUnits, getAttributeByName("gradientUnits"), (uint32_t&)fGradientUnits);

            if (getEnumValue(SVGSpreadMethod, getAttributeByName("spreadMethod"), (uint32_t&)fSpreadMethod))
            {
                fGradient.setExtendMode((BLExtendMode)fSpreadMethod);
            }

            fHasGradientTransform = parseTransform(getAttributeByName("gradientTransform"), fGradientTransform);

            // If the gradientUnits are ObjectBoundingBox, then 
            // the parameters are relative to the size of that box
            if (fGradientUnits == SVG_SPACE_OBJECT)
            {
                BLRect objFrame = ctx->getObjectFrame();
                auto w = objFrame.w;
                auto h = objFrame.h;
                auto x = objFrame.x;
                auto y = objFrame.y;
                
                const double cxN = resolveLengthBBoxUnits(fCx, 0.5);
                const double cyN = resolveLengthBBoxUnits(fCy, 0.5);
                const double crN = resolveLengthBBoxUnits(fR, 0.5);

                const double fxN = resolveLengthBBoxUnits(fFx, cxN);
                const double fyN = resolveLengthBBoxUnits(fFy, cyN);
                const double frN = resolveLengthBBoxUnits(fFr, 0.0);

                values.x0 = x + cxN * w;
                values.y0 = y + cyN* h;
                values.r0 = calculateDistance(crN, w, h);


                values.x1 = x + fxN * w;
                values.y1 = y + fyN * h;
                values.r1 = calculateDistance(frN, w, h);

            }
            else if (fGradientUnits == SVG_SPACE_USER)
            {
                BLRect lFrame = ctx->viewport();
                double w = lFrame.w;
                double h = lFrame.h;
                
                LengthResolveCtx wCtx{ dpi, nullptr, w, 0.0, SpaceUnitsKind::SVG_SPACE_USER };
                LengthResolveCtx hCtx{ dpi, nullptr, h, 0.0, SpaceUnitsKind::SVG_SPACE_USER };
                LengthResolveCtx rCtx{ dpi, nullptr, calculateDistance(1.0, w, h), 0.0, SpaceUnitsKind::SVG_SPACE_USER };

                values.x0 = resolveLengthUserUnits(fCx, wCtx);
                values.y0 = resolveLengthUserUnits(fCy, hCtx);
                values.r0 = resolveLengthUserUnits(fR,  rCtx);
                
                values.x1 = values.x0;
                values.y1 = values.y0;
                values.r1 = 0;

                LengthResolveCtx fwCtx{ dpi, nullptr, w, 0.0, SpaceUnitsKind::SVG_SPACE_USER };
                LengthResolveCtx fhCtx{ dpi, nullptr, h, 0.0, SpaceUnitsKind::SVG_SPACE_USER };
                LengthResolveCtx frCtx{ dpi, nullptr, calculateDistance(1.0, w, h), 0.0, SpaceUnitsKind::SVG_SPACE_USER };

                values.x1 = resolveLengthOr(fFx, fwCtx,values.x0);
                values.y1 = resolveLengthOr(fFy, fhCtx, values.y0);
                values.r1 = resolveLengthOr(fFr, frCtx, 0.0);
                
                //clampFocalPointToOuterCircle(values);
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


