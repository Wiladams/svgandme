#pragma once


//
// http://www.w3.org/TR/SVG11/feature#Mask
//

#include <functional>

#include "svgattributes.h"
#include "svggraphicselement.h"
#include "pixeling_mask.h"

namespace waavs 
{

    //================================================
    // SVGMaskElement
    // 'mask' element

    //================================================
    struct SVGMaskElement : public SVGGraphicsElement
    {

        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("mask", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGMaskElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        // Static constructor to register factory method in map
        static void registerFactory()
        {
            registerContainerNodeByName("mask",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGMaskElement>();
                    node->loadFromXmlPull(iter, groot);

                    return node;
                });
            
            registerSingularNode();
        }

        // Attributes:
        SVGLengthValue fX{ -10, SVG_LENGTHTYPE_PERCENTAGE, true };
        SVGLengthValue fY{ -10, SVG_LENGTHTYPE_PERCENTAGE, true };
        SVGLengthValue fWidth{ 120, SVG_LENGTHTYPE_PERCENTAGE, true };
        SVGLengthValue fHeight{ 120, SVG_LENGTHTYPE_PERCENTAGE, true };

        MaskTypeKind fMaskType{ MASKTYPE_LUMINANCE };
        SpaceUnitsKind fMaskUnits{ SpaceUnitsKind::SVG_SPACE_OBJECT };
        SpaceUnitsKind fMaskContentUnits{ SpaceUnitsKind::SVG_SPACE_USER };


        // Instance Constructor
        SVGMaskElement()
            : SVGGraphicsElement()
        {
            setIsVisible(false);
        }

        MaskTypeKind maskType() const noexcept { return fMaskType; }

        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            // parse length attributes
            //ByteSpan x{}, y{}, w{}, h{};
            //fAttributes.getValue(svgattr::x(), x);
            //fAttributes.getValue(svgattr::y(), y);
            //fAttributes.getValue(svgattr::width(), w);
            //fAttributes.getValue(svgattr::height(), h);

            lengthValue_parse(getAttribute(svgattr::x()), fX);
            lengthValue_parse(getAttribute(svgattr::y()), fY);
            lengthValue_parse(getAttribute(svgattr::width()), fWidth);
            lengthValue_parse(getAttribute(svgattr::height()), fHeight);

            //fX = parseLengthAttr(x);
            //fY = parseLengthAttr(y);
            //fWidth = parseLengthAttr(w);
            //fHeight = parseLengthAttr(h);

            // parse mask-type
            ByteSpan maskTypeAttr{};
            fAttributes.getValue(svgattr::mask_type(), maskTypeAttr);
            if (maskTypeAttr)
            {
                InternedKey maskTypeKey = WSNameSet::INTERN(maskTypeAttr);
                if (maskTypeKey == svgval::luminance())
                    fMaskType = MASKTYPE_LUMINANCE;
                else if (maskTypeKey == svgval::alpha())
                    fMaskType = MASKTYPE_ALPHA;
            }

            // parse maskUnits
            ByteSpan maskUnitsAttr{};
            fAttributes.getValue(svgattr::mask_units(), maskUnitsAttr);
            if (maskUnitsAttr)
            {
                InternedKey maskUnitsKey = WSNameSet::INTERN(maskUnitsAttr);
                if (maskUnitsKey == svgval::userSpaceOnUse())
                    fMaskUnits = SpaceUnitsKind::SVG_SPACE_USER;
                else if (maskUnitsKey == svgval::objectBoundingBox())
                    fMaskUnits = SpaceUnitsKind::SVG_SPACE_OBJECT;
            }

            // parse maskContentUnits
            ByteSpan maskContentUnitsAttr{};
            fAttributes.getValue(svgattr::mask_content_units(), maskContentUnitsAttr);
            if (maskContentUnitsAttr)
            {
                InternedKey maskContentUnitsKey = WSNameSet::INTERN(maskContentUnitsAttr);
                if (maskContentUnitsKey == svgval::userSpaceOnUse())
                    fMaskContentUnits = SpaceUnitsKind::SVG_SPACE_USER;
                else if (maskContentUnitsKey == svgval::objectBoundingBox())
                    fMaskContentUnits = SpaceUnitsKind::SVG_SPACE_OBJECT;
            }


        }

        /*
        bool renderMaskSurface(
            IAmGroot* groot,
            const IsolatedRenderPlan& plan,
            Surface& outMask) noexcept
        {
            if (!groot)
                return false;

            WGMatrix3x3 maskCtm = plan.ctm;

            if (fMaskContentUnits == SpaceUnitsKind::SVG_SPACE_OBJECT)
            {
                WGMatrix3x3 obb = WGMatrix3x3::makeIdentity();
                obb.translate(plan.objectBBoxUS.x, plan.objectBBoxUS.y);
                obb.scale(plan.objectBBoxUS.w, plan.objectBBoxUS.h);

                maskCtm.transform(obb);
                //maskCtm.postTransform(obb);
            }

            IsolatedSubtreeRequest req{};
            //SVGDrawigState* ds = ctx->getDrawingState();
            //if (ds)
            //    req.drawingState = *ds;
            req.userRect = plan.effectRectUS;
            req.pixelRect = plan.pixelRect;
            req.ctm = maskCtm;
            req.objectBBoxUS = plan.objectBBoxUS;
            req.renderMode = RF_Content;
            req.clear = true;

            return renderSubtreeToSurface(groot, this, req, outMask);
        }


        bool applyMaskToSurface(
            IAmGroot* groot,
            const IsolatedRenderPlan& plan,
            Surface& result) noexcept
        {
            if (!groot || result.empty())
                return false;

            Surface maskSurface{};
            if (!renderMaskSurface(groot, plan, maskSurface)) {
                //result.clear();
                return false;
            }

            Surface_ARGB32 maskView = maskSurface.info();
            Surface_ARGB32 resultView = result.info();
            wg_surface_mask(resultView, maskView, maskType());

            return true;
        }
        */
    };


}