#pragma once


//
// http://www.w3.org/TR/SVG11/feature#Mask
//

#include <functional>

#include "svgattributes.h"
#include "svggraphicselement.h"


namespace waavs {

    //================================================
    // SVGMaskElement
    // 'mask' element

    //================================================
    struct SVGMaskElement : public SVGGraphicsElement
    {
        enum MaskType : uint32_t
        {
            MASKTYPE_LUMINANCE,
            MASKTYPE_ALPHA
        };

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
        SVGLengthValue fX{ -10, SVG_LENGTHTYPE_PERCENTAGE, false };
        SVGLengthValue fY{ -10, SVG_LENGTHTYPE_PERCENTAGE, false };
        SVGLengthValue fWidth{ 120, SVG_LENGTHTYPE_PERCENTAGE, false };
        SVGLengthValue fHeight{ 120, SVG_LENGTHTYPE_PERCENTAGE, false };

        MaskType fMaskType{ MASKTYPE_LUMINANCE };
        SpaceUnitsKind fMaskUnits{ SpaceUnitsKind::SVG_SPACE_OBJECT };
        SpaceUnitsKind fMaskContentUnits{ SpaceUnitsKind::SVG_SPACE_USER };


        // Instance Constructor
        SVGMaskElement()
            : SVGGraphicsElement()
        {
            setIsVisible(false);
        }

        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            // parse length attributes
            ByteSpan x{}, y{}, w{}, h{};
            fAttributes.getValue(svgattr::x(), x);
            fAttributes.getValue(svgattr::y(), y);
            fAttributes.getValue(svgattr::width(), w);
            fAttributes.getValue(svgattr::height(), h);

            fX = parseLengthAttr(x);
            fY = parseLengthAttr(y);
            fWidth = parseLengthAttr(w);
            fHeight = parseLengthAttr(h);

            // parse mask-type
            ByteSpan maskTypeAttr{};
            fAttributes.getValue(svgattr::mask_type(), maskTypeAttr);
            if (maskTypeAttr)
            {
                InternedKey maskTypeKey = PSNameTable::INTERN(maskTypeAttr);
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
                InternedKey maskUnitsKey = PSNameTable::INTERN(maskUnitsAttr);
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
                InternedKey maskContentUnitsKey = PSNameTable::INTERN(maskContentUnitsAttr);
                if (maskContentUnitsKey == svgval::userSpaceOnUse())
                    fMaskContentUnits = SpaceUnitsKind::SVG_SPACE_USER;
                else if (maskContentUnitsKey == svgval::objectBoundingBox())
                    fMaskContentUnits = SpaceUnitsKind::SVG_SPACE_OBJECT;
            }


        }

        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            //The parent graphic should be stuffed into a BLPattern
            // and set as a fill style, then we can 
            // do this fillMask
            // parent->getVar()
            // BLPattern* pattern = new BLPattern(parent->getVar());
            // ctx->setFillStyle(pattern);
            //ctx->fillMask(BLPoint(0, 0), fCachedImage);

        }
    };
}