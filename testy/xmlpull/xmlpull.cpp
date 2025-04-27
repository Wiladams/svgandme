

#include "appmodule.h"
#include "app/mappedfile.h"
#include "app/xmlutil.h"

#include "xmliter.h"
#include "wsenum.h"


using namespace waavs;

static const WSEnum XML_ELEMENT_TYPE_WSEnum = {
    { "INVALID", XML_ELEMENT_TYPE_INVALID },
    { "XMLDECL", XML_ELEMENT_TYPE_XMLDECL },
    { "START_TAG", XML_ELEMENT_TYPE_START_TAG },
    { "END_TAG", XML_ELEMENT_TYPE_END_TAG },
    { "SELF_CLOSING", XML_ELEMENT_TYPE_SELF_CLOSING },
    { "EMPTY_TAG", XML_ELEMENT_TYPE_EMPTY_TAG },
    { "CONTENT", XML_ELEMENT_TYPE_CONTENT },
    { "COMMENT", XML_ELEMENT_TYPE_COMMENT },
    { "PROCESSING_INSTRUCTION", XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION },
    { "CDATA", XML_ELEMENT_TYPE_CDATA },
    { "DOCTYPE", XML_ELEMENT_TYPE_DOCTYPE },
    { "ENTITY", XML_ELEMENT_TYPE_ENTITY }
};


static void printXmlElementInfo(const XmlElementInfo& elem)
{
    ByteSpan kindname{};
    getEnumKey(XML_ELEMENT_TYPE_WSEnum, elem.kind(), kindname);
    
    printf("[[");
    writeChunk(kindname);
    if (elem.nameSpan())
    {
        printf(" - ");
        writeChunk(elem.nameSpan());
    }
    printf(" ]]\n");
    

    switch (elem.kind())
    {
    case XML_ELEMENT_TYPE_CONTENT:
    case XML_ELEMENT_TYPE_COMMENT:
    case XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION:
    case XML_ELEMENT_TYPE_DOCTYPE:
    case XML_ELEMENT_TYPE_CDATA:
        printChunk(elem.data());
        break;


    case XML_ELEMENT_TYPE_START_TAG:
    case XML_ELEMENT_TYPE_SELF_CLOSING:
    case XML_ELEMENT_TYPE_END_TAG:
        break;

    default:
        printf("NYI: ==> ");
        printChunk(elem.data());
        break;
    }

    // Print the attributes
    ByteSpan src = elem.data();

    ByteSpan key{};
    ByteSpan value{};
    while (readNextKeyAttribute(src, key, value))
    {
        printf("  ");
        writeChunk(key);
        printf("   = ");
        printChunk(value);
    }
}



static void testElementInfo(const ByteSpan& s)
{
	XmlElementInfoIterator iter(s);
	while (iter.next())
	{
		printXmlElementInfo(*iter);
	}

}

static void testXmlElementScan(const ByteSpan& s)
{
	XmlIteratorParams fParams{};
	XmlIteratorState fState{ XML_ITERATOR_STATE_CONTENT, s, s };
	fParams.fAutoScanAttributes = false;
    fParams.fSkipWhitespace = true;

	XmlElement elem;
	while (XmlElementGenerator(fParams, fState, elem))
	{
		waavs::printXmlElement(elem);
	}
}

static void testXmlIter(const ByteSpan& s)
{
    XmlElementIterator iter(s);

    do
    {
        const XmlElement& elem = *iter;

        printXmlElementInfo(elem);
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


/*
        case XPathTokenKind::ROOT:           printf("ROOT\n"); break;
        case XPathTokenKind::CHILD:          printf("CHILD\n"); break;
        case XPathTokenKind::DESCENDANT:     printf("DESCENDANT\n"); break;
        case XPathTokenKind::SELF:           printf("SELF\n"); break;
        case XPathTokenKind::PARENT:         printf("PARENT\n"); break;
        case XPathTokenKind::WILDCARD_NODE:  printf("WILDCARD (*)\n"); break;
*/


    static WSEnum XPathAxisEnum = 
    {
        { "ROOT", XPathTokenKind::ROOT },
        { "CHILD", XPathTokenKind::CHILD },
        { "DESCENDANT", XPathTokenKind::DESCENDANT },
        { "SELF", XPathTokenKind::SELF },
        { "PARENT", XPathTokenKind::PARENT },
        { "WILDCARD_NODE", XPathTokenKind::WILDCARD_NODE }
    };


static void printXPathExpressions(const ByteSpan& xpath)
{
    XPathExpression expr(xpath);

    if (!expr.parse(xpath))
    {
        printf("Failed to parse XPath expression: ");
        printChunk(xpath);
        return;
    }

    printf("Parsed XPath Expression: ");
	printChunk(xpath);


    for (size_t i = 0; i < expr.steps.size(); ++i)
    {
        const auto& step = expr.steps[i];

        printf("Step %zu:\n", i + 1);
        printf("  Node Name: ");
        printChunk(step.nodeName.empty() ? ByteSpan("(*)") : step.nodeName);

        printf("  Axis: ");
		ByteSpan axisname;
		getEnumKey(XPathAxisEnum, step.axis, axisname);
		if (axisname.empty())
			axisname = ByteSpan("UNKNOWN");
        printChunk(axisname);

        if (!step.attribute.empty())
        {
            printf("  Attribute: ");
            printChunk(step.attribute);
        }

        if (!step.value.empty())
        {
            printf("  Value: ");
            printChunk(step.value);
        }

        if (step.operatorType == XPathTokenKind::OPERATOR)
        {
            printf("  Operator: ");
            //printChunk(step.op);
        }

        //if (!step.predicate.empty())
        //{
        //    printf("  Predicate: ");
        //    printChunk(step.predicate);
        //}

        printf("\n");
    }
}

static void testXPathExpressions()
{
	printXPathExpressions("/root/item[@id=\"SVGID_7_\"]");
}

static void printModuleName()
{
	ByteSpan modname = getModuleFileName();
	printf("Module Name: ");
	printChunk(modname);
	printf("\n");
}

static void printCommandLine()
{
	ByteSpan cmdLine = getModuleCommandLine();
	printf("Command Line: ");
	printChunk(cmdLine);
	printf("\n");
}

int main(int argc, char** argv)
{
	printModuleName();
	printCommandLine();

    if (argc < 2)
    {
        printf("Usage: xmlpull <xml file>  [output file]\n");
        return 1;
    }

    // create an mmap for the specified file
    const char* filename = argv[1];
    auto mapped = MappedFile::create_shared(filename);

    if (nullptr == mapped)
        return 0;

    waavs::ByteSpan s(mapped->data(), mapped->size());
    

    //testElementInfo(s);
    testXmlElementScan(s);
    //testXmlIter(s);
	//testElementContainer(s);
    //testElementFilter(s);
    //testXPathFilter();
    //testXPathExpressions();

    // iterate over the elements
    // printing each one
    // There is no regard to hierarchy here, just raw
    // element output
    // Printing an element will print its attributes though
	// and printing a pure content node will print its content
	// and printing a comment will print the comment

    // Use an iterator directly
    //XmlElementIterator iter(s);
    //while (iter.next())
    //{   
    //    waavs::printXmlElement(*iter);
   // }

    // close the mapped file
    //mapped->close();


    return 0;
}
