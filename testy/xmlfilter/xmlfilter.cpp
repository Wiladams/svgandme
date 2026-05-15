//
// This file was originally a test of containers and iterators
// for XML elements.  The whole container/iterator was abandoned
// so, these tests no longer work.

#include "xmlscan.h"
#include "app/xmlutil.h"

using namespace waavs;



static void testElementFilter(const ByteSpan& s)
{
    // create a filter that will only include
    // elements that are start tags
    auto onlyStartTags = [](const XmlElement& elem) { return elem.isStart(); };

    // a predicate that matches elements that have a 'cx' attribute
    auto hasCxAttribute = [](const XmlElement& elem) { ByteSpan value{}; return elem.getElementAttribute("d", value); };

    // create a element iterator
    XmlPull iter(s);

    while (iter.next())
    {
        if (hasCxAttribute(*iter))
            waavs::printXmlElement(*iter);
    }

}

