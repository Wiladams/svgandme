#pragma once

//#include <functional>
//#include <unordered_map>

//#include "svgattributes.h"
#include "svggraphicselement.h"

namespace waavs {
    //============================================================
    // SVGSolidColor
    // Represents a single solid color
    // You could represent this as a linear gradient with a single color
    // but this is the way to do it for SVG 2.0
    //============================================================
    struct SVGSolidColorElement : public SVGGraphicsElement
    {
        static void registerFactory() {
            registerSVGSingularNodeByName("solidColor", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGSolidColorElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        SVGColor fColor;


        SVGSolidColorElement()
            :SVGGraphicsElement()
            ,fColor(svgattr::solid_color(), svgattr::solid_opacity())
        {
            setIsVisible(false);
        }

        const BLVar getVariant(IRenderSVG* ctx, IAmGroot* groot) noexcept override
        {
            BLVar tmpVar;
            tmpVar = BLRgba32_premultiplied_from_ColorSRGB(fColor.value());

            return tmpVar;
        }

        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            (void)groot;

            fColor.loadFromAttributes(fAttributes);
        }

    };

}
