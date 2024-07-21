#pragma once

//
// SVGScript
// Handle the SVG script element
//
#include <map>
#include <functional>
#include <cstdint>

#include "blend2d.h"


#include "bspan.h"
#include "maths.h"


namespace waavs {
    struct SVGScriptElement : public SVGGraphicsElement
    {
        static void registerFactory()
        {
            gSVGGraphicsElementCreation["script"] = [](IAmGroot* aroot, XmlElementIterator& iter) {
                auto node = std::make_shared<SVGScriptElement>(aroot);
                node->loadFromXmlIterator(iter);
                return node;
                };
        }

        ByteSpan fScript{};

        SVGScriptElement(IAmGroot* aroot) : SVGGraphicsElement(aroot)
        {
            isStructural(false);
        }

        virtual void loadFromXmlIterator(XmlElementIterator& iter) override
        {
            SVGGraphicsElement::loadFromXmlIterator(iter);
        }

        void loadCDataNode(const XmlElement& elem) override
        {
            fScript = elem.data();
			//writeChunkToFile(fScript, "script.js");
        }
    };
}
