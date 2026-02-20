#pragma once

//#include <functional>
//#include <unordered_map>

//#include "svgattributes.h"
#include "svgstructuretypes.h"

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
                auto node = std::make_shared<SVGSolidColorElement>(groot);
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        SVGPaint fPaint{ nullptr };

        SVGSolidColorElement(IAmGroot*)
            :SVGGraphicsElement()
        {
            setIsVisible(false);
        }

        const BLVar getVariant(IRenderSVG* ctx, IAmGroot* groot) noexcept override
        {
            return fPaint.getVariant(ctx, groot);
        }

        void fixupSelfStyleAttributes(IAmGroot* groot) override
        {
            // Get the color and opacity attributes, and set up the paint
            ByteSpan solidColorAttr{}, solidOpacityAttr{};

            getAttribute(svgattr::solid_color(), solidColorAttr);
            getAttribute(svgattr::solid_opacity(), solidOpacityAttr);

            fPaint.loadFromChunk(solidColorAttr);
            if (!solidOpacityAttr.empty())
            {
                double opa{ 1.0 };
                if (readNumber(solidOpacityAttr, opa))
                {
                    fPaint.setOpacity(opa);
                }
            }
        }


    };

}
