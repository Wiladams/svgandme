// svggradient.h
#pragma once

//
// Support for SVGGradientElement
// http://www.w3.org/TR/SVG11/feature#Gradient
// linearGradient, radialGradient, conicGradient
//

#include <functional>

#include "svgattributes.h"
#include "svggraphicselement.h"
#include "maths.h"


namespace waavs 
{

    // Helpers to resolve gradient coordinates, and to make the bbox to user transform
    // For gradientUnits="objectBoundingBox":
    // Return coordinate in bbox-space (0..1 typical, but allow outside).
    static INLINE double resolveLengthBBoxUnits(const SVGLengthValue& v, double fallback = 0.0) noexcept
    {
        if (!v.isSet())
            return fallback;

        if (v.fUnitType == SVG_LENGTHTYPE_PERCENTAGE)
            return v.fValue / 100.0;

        return v.fValue;
    }

    static INLINE double resolveGradientLength(const SVGLengthValue& v,
        const LengthResolveCtx& ctx,
        double fallback) noexcept
    {
        if (!v.isSet())
            return fallback;

        return lengthValue_resolve(v, ctx);
    }




    static INLINE WGMatrix3x3 makeBBoxToUserTransform(const WGRectD& b) noexcept
    {
        WGMatrix3x3 m = WGMatrix3x3::makeIdentity();
        m.translate(b.x, b.y);
        m.scale(b.w, b.h);

        return m;
    }

    static INLINE WGMatrix3x3 composeGradientTransformBBox(const WGRectD& b, bool hasGT, const WGMatrix3x3& GT) noexcept
    {
        WGMatrix3x3 m = makeBBoxToUserTransform(b);
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
    struct SVGStopNode //: public SVGObject
    {
        double fOffset = 0;
        BLRgba32 fColor{};  // default black

        SVGStopNode() //:SVGObject() 
        {
            fColor = BLRgba32(0xFF000000u);
        }

        double offset() const { return fOffset; }
        BLRgba32 color() const { return fColor; }

        void loadFromXmlElement(const XmlElement& elem, IAmGroot* groot)
        {
            // Get the attributes from the element
            ByteSpan attrSpan = elem.data();
            XmlAttributeCollection attrs{};
            scanAttributes(attrs, attrSpan);
            
            ByteSpan styleAttr{}, offsetAttr{};


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
                if (numberOrPercent_read(s, op))
                {
                    fOffset = op.normalizedValue();
                    if (fOffset < 0.0 || fOffset > 1.0)
                        WAAVS_ASSERT(false && "Gradient stop offset value out of range (should be between 0 and 1)");
                } else
                    WAAVS_ASSERT(false && "Gradient stop offset value not parsed properly");

            } // else, value defaults to 0
            // which is already set in the constructor

            // Now, try to read a color value
            SVGColor sColor(svgattr::stop_color(), svgattr::stop_opacity());

            if (sColor.loadFromAttributes(attrs, groot))
            {
                if (sColor.isColor())
                    fColor.value = Pixel_ARGB32_from_ColorSRGB(sColor.value());
            }
            // else, do nothing, as the default color value is black
            //    fColor.value = 0xFF000000u;
        }
        
        //void bindToContext(IDrawGraphics*, IAmGroot*) noexcept override
        //{
            // nothing to see here
        //}
    };
    
    //============================================================
    // SVGGradient
    // Base class for other gradient types
    //============================================================
    struct SVGGradient : public SVGGraphicsElement
    {
        static constexpr uint32_t kMaxGradientHrefDepth = 32;

    protected:
        // Used for gradient construction
        IDrawGraphics* fBuildCtx = nullptr;
        IAmGroot* fBuildGroot = nullptr;
        double fBuildDpi = 96.0;
        WGRectD fBuildViewport{};
        WGRectD fBuildObjectFrame{};

    public:
        WGMatrix3x3 fGradientTransform{};
        bool fHasGradientTransform = false;

        BLGradient fGradient{};

        // Some common attributes
        BLExtendMode fSpreadMethod{ BL_EXTEND_MODE_PAD };
        SpaceUnitsKind fGradientUnits{ SVG_SPACE_OBJECT };


        // Constructor
        SVGGradient(BLGradientType aType)
            :SVGGraphicsElement()
        {
            fGradient.set_type(aType);
            fGradient.set_extend_mode(BL_EXTEND_MODE_PAD);

            setIsStructural(true);
            setIsVisible(false);
            setNeedsBinding(true);
        }

        // We want to catch when copy construction or
        // assignment are occuring
        SVGGradient(const SVGGradient& other) = delete;
        SVGGradient operator=(const SVGGradient& other) = delete;

        BLGradientType gradientType() const { return fGradient.type(); }

        bool hasHref() const { return !href().empty(); }
        ByteSpan href() const {
            ByteSpan svgHref{};
            svgHref = getAttribute(svgattr::href());
            if (!svgHref)
                svgHref = getAttribute(svgattr::xlink_href());    // support legacy xlink:href for compatibility

            return svgHref;
        }

    protected:

        void beginGradientBuild(IDrawGraphics* ctx, IAmGroot* groot) noexcept
        {
            fBuildCtx = ctx;
            fBuildGroot = groot;
            fBuildDpi = groot ? groot->dpi() : 96.0;

            if (ctx) {
                fBuildViewport = ctx->state().getViewport();
                fBuildObjectFrame = ctx->state().getObjectFrame();
            }
            else {
                fBuildViewport = {};
                fBuildObjectFrame = {};
            }
        }

        bool isGradientUserSpace() const noexcept { return fGradientUnits == SVG_SPACE_USER; }
        bool isGradientObjectBBox() const noexcept { return fGradientUnits == SVG_SPACE_OBJECT; }

        WGMatrix3x3 gradientFinalTransform() const noexcept
        {
            if (isGradientObjectBBox())
                return composeGradientTransformBBox(
                    fBuildObjectFrame,
                    fHasGradientTransform,
                    fGradientTransform);

            if (fHasGradientTransform)
                return fGradientTransform;

            return WGMatrix3x3::makeIdentity();
        }

        LengthResolveCtx gradientWidthCtx() const noexcept
        {
            return LengthResolveCtx{
                fBuildDpi,
                nullptr,
                fBuildViewport.w,
                0.0,
                SVG_SPACE_USER
            };
        }

        LengthResolveCtx gradientHeightCtx() const noexcept
        {
            return LengthResolveCtx{
                fBuildDpi,
                nullptr,
                fBuildViewport.h,
                0.0,
                SVG_SPACE_USER
            };
        }

        LengthResolveCtx gradientRadiusCtx() const noexcept
        {
            const double d = std::sqrt(
                fBuildViewport.w * fBuildViewport.w +
                fBuildViewport.h * fBuildViewport.h);

            return LengthResolveCtx{
                fBuildDpi,
                nullptr,
                d,
                0.0,
                SVG_SPACE_USER
            };
        }

        double resolveGradientX(const SVGLengthValue& v, double fallback) const noexcept
        {
            if (isGradientObjectBBox())
                return resolveLengthBBoxUnits(v, fallback);

            return resolveGradientLength(v, gradientWidthCtx(), fallback);
        }

        double resolveGradientY(const SVGLengthValue& v, double fallback) const noexcept
        {
            if (isGradientObjectBBox())
                return resolveLengthBBoxUnits(v, fallback);

            return resolveGradientLength(v, gradientHeightCtx(), fallback);
        }


        double resolveGradientRadius(const SVGLengthValue& v, double fallback) const noexcept
        {
            if (isGradientObjectBBox())
                return resolveLengthBBoxUnits(v, fallback);

            return resolveGradientLength(v, gradientRadiusCtx(), fallback);
        }


        bool buildCommonGradient(IDrawGraphics* ctx, IAmGroot* groot, BLGradient& grad) noexcept
        {
            beginGradientBuild(ctx, groot);

            grad.set_type(fGradient.type());
            grad.set_extend_mode(fGradient.extend_mode());
            grad.reset_stops();
            grad.assign_stops(fGradient.stops_view());

            if (!buildGradientValues(grad))
                return false;

            auto tform = gradientFinalTransform();
            grad.set_transform(blMatrix_from_WGMatrix3x3(tform));

            return true;
        }

        virtual bool buildGradientValues(BLGradient& grad) noexcept = 0;

    public:
        // getVariant()
        //
        // Whomever is using us for paint is calling in here to get
        // our paint variant.  This is the place to construct the thing,
        // if it hasn't already been constructed.
        //
        // It's like a bindToContext essentially, but bindToContext is
        // only called as part of a drawing chain.

        const BLVar getVariant(IDrawGraphics* ctx, IAmGroot* groot) noexcept override
        {
            BLGradient grad{};

            if (!buildCommonGradient(ctx, groot, grad))
                return BLVar::null();

            BLVar out{};
            out = grad;
            return out;
        }

        // Inherit the raw attributes that are common to all gradients,
        // if we don't already have them.
        // Properties to inherit:
        // Common properties to inherit
        //   gradientUnits
        //   gradientTransform
        //   spreadMethod
        //

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
        
        // inheritProperties
        //
        // Inherit properties from the given element
        virtual void inheritProperties(const SVGGradient* elem)
        {
            if (!elem) 
                return;

            // 1) stops: copy stops from referred to only if 
            // we don't have any stops already.
            if (fGradient.size() == 0)
            {
                auto stopsView = elem->fGradient.stops_view();
                if (stopsView.size > 0)
                {
                    fGradient.assign_stops(stopsView.data, stopsView.size);
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

                // Make sure the referredTo gradient has already
                // resolved it's attributes first
                gnode->resolveStyleSubtree(groot);

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

            fGradient.add_stop(offset, acolor);

        }

        void fixupCommonAttributes(IAmGroot* groot)
        {
            // spreadMethod / gradientUnits / gradientTransform
            if (getEnumValue(SVGSpreadMethod, getAttribute(svgattr::spreadMethod()), (uint32_t&)fSpreadMethod))
                fGradient.set_extend_mode((BLExtendMode)fSpreadMethod);

            getEnumValue(SVGSpaceUnits, getAttribute(svgattr::gradientUnits()), (uint32_t&)fGradientUnits);

            fHasGradientTransform = parseTransform(getAttribute(svgattr::gradientTransform()), fGradientTransform);

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
                auto node = std::make_shared<SVGLinearGradient>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("linearGradient",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGLinearGradient>();
                    node->loadFromXmlPull(iter, groot);

                    return node;
                });

            registerSingularNode();
        }


        SVGLengthValue x1;
        SVGLengthValue y1;
        SVGLengthValue x2;
        SVGLengthValue y2;


        SVGLinearGradient() 
            :SVGGradient(BLGradientType::BL_GRADIENT_TYPE_LINEAR)
        {
        }

        bool buildGradientValues(BLGradient& grad) noexcept override
        {
            SVGLengthValue x1{ 0.0, SVG_LENGTHTYPE_PERCENTAGE, false };
            SVGLengthValue y1{ 0.0, SVG_LENGTHTYPE_PERCENTAGE, false };
            SVGLengthValue x2{ 100.0, SVG_LENGTHTYPE_PERCENTAGE, false };
            SVGLengthValue y2{ 0.0, SVG_LENGTHTYPE_PERCENTAGE, false };

            ByteSpan s;
            if ((s = getAttribute(svgattr::x1()))) { ByteSpan t = s; lengthValue_parse(t, x1); }
            if ((s = getAttribute(svgattr::y1()))) { ByteSpan t = s; lengthValue_parse(t, y1); }
            if ((s = getAttribute(svgattr::x2()))) { ByteSpan t = s; lengthValue_parse(t, x2); }
            if ((s = getAttribute(svgattr::y2()))) { ByteSpan t = s; lengthValue_parse(t, y2); }

            BLLinearGradientValues values{};
            values.x0 = resolveGradientX(x1, 0.0);
            values.y0 = resolveGradientY(y1, 0.0);
            values.x1 = resolveGradientX(x2, isGradientObjectBBox() ? 1.0 : fBuildViewport.w);
            values.y1 = resolveGradientY(y2, 0.0);

            grad.set_values(values);

            return true;
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
            // We have our own attributes already set, so 
            // try to inherit any missing attributes from the
            // chain of references, if we have an href.
            resolveReferenceChain(groot);
         
            // Convert the non-variant attributes
            // spreadMethod / gradientUnits / gradientTransform
            fixupCommonAttributes(groot);
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
        // SVG spec says that percentage values for radial gradient radius should 
        // be interpreted as a fraction of the distance from the center to the 
        // hypotenuse of the objectBoundingBox.  So we calculate that distance here.
        //return fraction  * std::sqrt((width * width) + (height * height));
        
        // But, what the browsers do, is to treat percentage values for radial 
        // gradient radius as a fraction of the width or height (unit 1)
        return fraction * width;
    }


    struct SVGRadialGradient : public SVGGradient
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("radialGradient", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGRadialGradient>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("radialGradient",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGRadialGradient>();
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


        SVGRadialGradient() 
            :SVGGradient(BLGradientType::BL_GRADIENT_TYPE_RADIAL)
        {
        }

        bool buildGradientValues(BLGradient& grad) noexcept override
        {
            BLRadialGradientValues values{};

            values.x0 = resolveGradientX(fCx, 0.5);
            values.y0 = resolveGradientY(fCy, 0.5);
            values.r0 = resolveGradientRadius(fR, 0.5);

            values.x1 = resolveGradientX(fFx, values.x0);
            values.y1 = resolveGradientY(fFy, values.y0);
            values.r1 = resolveGradientRadius(fFr, 0.0);

            grad.set_values(values);
            return true;
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

            resolveReferenceChain(groot);

            fixupCommonAttributes(groot);

            // Parse our own attributes after they've been 
            // inherited and resolved.
            lengthValue_parse(getAttribute(svgattr::cx()), fCx);
            lengthValue_parse(getAttribute(svgattr::cy()), fCy);
            lengthValue_parse(getAttribute(svgattr::r()), fR);
            
            lengthValue_parse(getAttribute(svgattr::fx()), fFx);
            lengthValue_parse(getAttribute(svgattr::fy()), fFy);
            lengthValue_parse(getAttribute(svgattr::fr()), fFr);
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
                auto node = std::make_shared<SVGConicGradient>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("conicGradient",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGConicGradient>();
                    node->loadFromXmlPull(iter, groot);

                    return node;
                });
            
            registerSingularNode();
        }

        SVGLengthValue fCx{ };
        SVGLengthValue fCy{};
        SVGLengthValue fRepeat{};
        double fAngle{};    // in radians

        SVGConicGradient() 
            :SVGGradient(BLGradientType::BL_GRADIENT_TYPE_CONIC)
        {
        }

        bool buildGradientValues(BLGradient& grad) noexcept override
        {
            BLConicGradientValues values{};

            values.x0 = resolveGradientX(fCx, 0.5);
            values.y0 = resolveGradientY(fCy, 0.5);
            if (fRepeat.isSet()) {
                LengthResolveCtx repeatCtx{fBuildDpi,nullptr, 1.0, 0.0, SVG_SPACE_USER};
                values.repeat = lengthValue_resolve(fRepeat, repeatCtx);
            }

            // blend2d's conic gradient doesn't support a repeat value of 0, 
            // but SVG allows that to mean "no repeat", so treat 0 as 1 here.
            if (values.repeat == 0.0)
                values.repeat = 1.0;

            values.angle = fAngle;

            grad.set_values(values);

            return true;
        }

        // Attributes to inherit
        // x1, y1
        // angle, repeat
        //
        virtual void inheritSameKindProperties(const SVGGradient* elem) override
        {
            if (!elem)
                return;

            setAttributeIfAbsent(elem, svgattr::cx());
            setAttributeIfAbsent(elem, svgattr::cy());
            setAttributeIfAbsent(elem, svgattr::angle());
            setAttributeIfAbsent(elem, svgattr::repeat());
        }
        
        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            resolveReferenceChain(groot);
            fixupCommonAttributes(groot);

            // Parse our own attributes after they've been 
            // inherited and resolved.
            lengthValue_parse(getAttribute(svgattr::cx()), fCx);
            lengthValue_parse(getAttribute(svgattr::cy()), fCy);

            lengthValue_parse(getAttribute(svgattr::repeat()), fRepeat);

            ByteSpan angleSpan{};
            if (getAttribute(svgattr::angle(), angleSpan))
            {
                SVGAngleUnits units{};
                parseAngle(angleSpan, fAngle, units);
            }
        }


    };
}


