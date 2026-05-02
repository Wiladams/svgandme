#pragma once

#include "svggraphicselement.h"

// -------------------------------------
// sub-components, which are used inside 
// feDiffuseLighting and feSpecularLighting
// These are not full filter primitives on their own, 
// but they still need to be parsed and represented as 
// nodes in the filter DOM.
//
// BUGBUG - Since these just serve as attribute containers
// with no behavior, we should create a simple way to create
// such nodes without using all the boilerplate.
// Maybe it's just SVGNode
// -------------------------------------

namespace waavs
{
    //
    // feDistantLight
    // This is a sub-component of other lighting components
    // so don't emit a program for it.
    //
    struct SVGFeDistantLightElement : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName(filter::feDistantLight(), [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeDistantLightElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName(filter::feDistantLight(), [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeDistantLightElement>();
                node->loadFromXmlPull(iter, groot);

                return node;
                });

            registerSingularNode();
        }


        SVGFeDistantLightElement()
            : SVGGraphicsElement()
        {
        }


    };

    // fePointLight
    // This is a sub-component of other lighting components
    // so don't emit a program for it.
    //
    struct SVGFePointLightElement : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName(filter::fePointLight(), [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFePointLightElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName(filter::fePointLight(), [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFePointLightElement>();
                node->loadFromXmlPull(iter, groot);

                return node;
                });

            registerSingularNode();
        }

        SVGFePointLightElement()
            : SVGGraphicsElement()
        {
        }

    };


    // feSpotLight
    // 
    // This is a sub-component of other lighting components
    // so don't emit a program for it.
    //
    struct SVGFeSpotLightElement : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("feSpotLight", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGFeSpotLightElement>();
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        static void registerFactory()
        {
            registerContainerNodeByName("feSpotLight", [](IAmGroot* groot, XmlPull& iter) {
                auto node = std::make_shared<SVGFeSpotLightElement>();
                node->loadFromXmlPull(iter, groot);

                return node;
                });

            registerSingularNode();
        }

        SVGFeSpotLightElement()
            : SVGGraphicsElement()
        {
        }

    };
}
