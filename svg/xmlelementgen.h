#pragma once

#include "xmlscan.h"

namespace waavs
{
    // Objectified XML token generator
    struct XmlElementGenerator : IProduce<XmlElement>
    {
        XmlIterator iter{};

        XmlElementGenerator(const ByteSpan& src)
            :iter{ src }
        {
        }

        bool next(XmlElement& elem) override
        {
            return nextXmlElement(iter, elem);
        }
    };
}

