#pragma once

#include "svggraphicselement.h"

// Miscellaneous node types that don't provide much 
// value, typically meta information.
//
namespace waavs
{
    //=================================================
    //
    // SVGDescNode
    // https://www.w3.org/TR/SVG11/struct.html#DescElement
    // 
    struct SVGDescNode : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("desc", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGDescNode>(groot);
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        // Static constructor to register factory method in map
        static void registerFactory()
        {
            registerContainerNodeByName("desc",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGDescNode>(groot);
                    node->loadFromXmlPull(iter, groot);

                    return node;
                });


            registerSingularNode();
        }

        ByteSpan fContent{};

        // Instance Constructor
        SVGDescNode(IAmGroot*)
            : SVGGraphicsElement()
        {
            setIsVisible(false);
        }

        const ByteSpan& content() const { return fContent; }

        // Load the text content if it exists
        void loadContentNode(const XmlElement& elem, IAmGroot* groot) override
        {
            fContent = elem.data();
        }
    };



    //=================================================
    // SVGTitleNode
    // 	   https://www.w3.org/TR/SVG11/struct.html#TitleElement
    // Capture the title of the SVG document
    //=================================================
    struct SVGTitleNode : public SVGGraphicsElement
    {
        static void registerSingularNode()
        {
            registerSVGSingularNodeByName("title", [](IAmGroot* groot, const XmlElement& elem) {
                auto node = std::make_shared<SVGTitleNode>(groot);
                node->loadFromXmlElement(elem, groot);

                return node;
                });
        }

        // Static constructor to register factory method in map
        static void registerFactory()
        {
            registerContainerNodeByName("title",
                [](IAmGroot* groot, XmlPull& iter) {
                    auto node = std::make_shared<SVGTitleNode>(groot);
                    node->loadFromXmlPull(iter, groot);

                    return node;
                });

            registerSingularNode();
        }

        ByteSpan fContent{};

        // Instance Constructor
        SVGTitleNode(IAmGroot*)
            : SVGGraphicsElement()
        {
            setIsVisible(false);
        }

        const ByteSpan& content() const { return fContent; }

        // Load the text content if it exists
        void loadContentNode(const XmlElement& elem, IAmGroot* groot) override
        {
            fContent = elem.data();
        }
    };
}