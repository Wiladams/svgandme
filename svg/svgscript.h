#pragma once

//
// SVGScript
// Handle the SVG script element
//
#include <map>
#include <functional>
#include <cstdint>


#include "svgattributes.h"
#include "svgstructuretypes.h"



namespace waavs {
    struct SVGScriptElement : public SVGGraphicsElement
    {
        static void registerFactory()
        {
            registerContainerNode("script",
                [](IAmGroot* groot, XmlElementIterator& iter) {
                    auto node = std::make_shared<SVGScriptElement>(groot);
                    node->loadFromXmlIterator(iter, groot);
                    return node;
                });
            
        }

        ByteSpan fScript{};

        SVGScriptElement(IAmGroot* groot) : SVGGraphicsElement(groot)
        {
            isStructural(false);
        }

        // When content is not wrapped in CDATA
        void loadContentNode(const XmlElement& elem, IAmGroot* groot) override
        {
            loadCDataNode(elem, groot);
        }
        
        // When content is wrapped in CDATA
        void loadCDataNode(const XmlElement& elem, IAmGroot* groot) override
        {
            fScript = elem.data();
			//writeChunkToFile(fScript, "script.js");
        }
    };
}
