#pragma once

#include "xmlscan.h"

// NOTE: Use XmlPull structure for most uses
// this one is a bit experimental, and adds more overhead
// than is necessary for simple uses.
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

