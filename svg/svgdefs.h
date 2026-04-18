#pragma once

#include <functional>


#include "svggraphicselement.h"

namespace waavs
{
    //============================================================
    // 	SVGDefsNode
    // This node is here to hold onto definitions of other nodes
    //============================================================
    struct SVGDefsNode : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("defs", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGDefsNode>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        // Static constructor to register factory method in map
        static void registerFactory()
        {
            registerContainerNodeByName(svgtag::tag_defs(),
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGDefsNode>();
                    node->loadFromXmlPull(iter, groot);

                    return node;
                });


            registerSingularNode();
        }

        // Instance Constructor
        SVGDefsNode()
            : SVGGraphicsElement()
        {
            setIsStructural(true);
            setIsVisible(false);
        }

        // We override this here, because we want to add all nodes to our 
        // sub-tree, not just the ones that are structural.
        bool addNodeToSubtree(std::shared_ptr < IViewable > node, IAmGroot* groot) override
        {
            if (node == nullptr || groot == nullptr)
                return false;

            fNodes.push_back(node);

            return true;
        }


    };
}
