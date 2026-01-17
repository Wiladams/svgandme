#include "xmlscan.h"

#include "app/xmlutil.h"

using namespace waavs;

// Nothing useful in this file.
// Just some potential experiments in filtering XML elements
//
static void testXmlIter(const ByteSpan& s)
{
    XmlElementIterator iter(s);

    do
    {
        const XmlElement& elem = *iter;
        if (elem.isContent())
        {
            // only print content if it is not whitespace
            ByteSpan content = elem.data();
            content.skipWhile(chrWspChars);
            if (!content.empty())
                printXmlElement(elem);
        } else
            printXmlElement(elem);

    } while (iter.next());

}

static void testElementContainer(const ByteSpan& s)
{
    // use a range loop to iterate over the elements
    // and print them out
    XmlElementContainer container(s);
    for (XmlElement  const & elem : container)
    {
        waavs::printXmlElement(elem);
    }
}

static void testElementFilter(const ByteSpan& s)
{
    // create a filter that will only include
    // elements that are start tags
    auto onlyStartTags = [](const XmlElement& elem) { return elem.isStart(); };

    // a predicate that matches elements that have a 'cx' attribute
    auto hasCxAttribute = [](const XmlElement& elem) { ByteSpan value{}; return elem.getRawAttributeValue("d", value); };

    // create a container
    XmlElementContainer container(s);

    // create a filtered container
    //XmlFilteredContainer predContainer(container, onlyStartTags);
    XmlFilteredContainer predContainer(container, hasCxAttribute);

    // iterate over the filtered container
    for (XmlElement const& elem : predContainer)
    {
        waavs::printXmlElement(elem);
    }
}

