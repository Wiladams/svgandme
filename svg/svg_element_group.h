#pragma once



#include <functional>

#include "svggraphicselement.h"
#include "svg_attribute_viewport.h"


namespace waavs 
{


    //================================================
    // SVGGroupNode
    // 'g' element
    //================================================
    struct SVGGElement : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("g", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGGElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        // Static constructor to register factory method in map
        static void registerFactory()
        {
            registerContainerNodeByName("g",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGGElement>();
                    node->loadFromXmlPull(iter, groot);

                    return node;
                });

            registerSingularNode();
        }



        // Instance Constructor
        SVGGElement()
            : SVGGraphicsElement()
        {
        }

        const WGRectD objectBoundingBox() const noexcept override
        {
            WGRectD pbox{};

            for (auto& node : fRenderNodes)
            {
                if (!node || !node->isVisible()) continue;

                WGRectD nodeBox{};
                nodeBox = node->objectBoundingBox();
                wg_rectD_union(pbox, nodeBox);
            }

            return pbox;
        }

        // calculate the full extent of the painted area.  Used to calculate
        // ofscreen size for filters.
        const WGRectD getObjectBoundingBox(IDrawGraphics* ctx, IAmGroot* groot)  noexcept override
        {
            WGRectD pbox{};

            for (auto& node : fRenderNodes)
            {
                if (!node || !node->isVisible()) continue;

                WGRectD nodeBox{};
                nodeBox = node->getObjectBoundingBox(ctx, groot);
                wg_rectD_union(pbox, nodeBox);
            }

            return pbox;
        }
    };

}
