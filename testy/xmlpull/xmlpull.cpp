

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

static const WSEnum XML_TOKEN_TYPE_WSEnum = 
{
    {"INVALID", XML_TOKEN_INVALID},
    {"<", XML_TOKEN_LT},
    {">", XML_TOKEN_GT},
	{"/", XML_TOKEN_SLASH},
	{"=", XML_TOKEN_EQ},
	{"?", XML_TOKEN_QMARK},
	{"!", XML_TOKEN_BANG},
	{"NAME", XML_TOKEN_NAME},
	{"STRING", XML_TOKEN_STRING},
	{"TEXT", XML_TOKEN_TEXT}
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
        printf(" = ");
        printChunk(value);
    }
}

static void printToken(const XmlToken& tok)
{
	ByteSpan kindname{};
	getEnumKey(XML_TOKEN_TYPE_WSEnum, tok.type, kindname);
	printf("Token [%d] %.*s : [%.*s]\n",
		tok.type, (int)kindname.size(), kindname.data(),
		(int)tok.value.size(), tok.value.data());
}

static void testTokenizer(const ByteSpan& src)
{
    XmlTokenState tokState = { src, false };
    XmlToken tok;

    while (nextXmlToken(tokState, tok)) 
    {
        printToken(tok);
    }
}


static void testTokenGenerator(const ByteSpan& src)
{
    XmlTokenGenerator gen(src);

    XmlToken tok;
    while (gen.next(tok))
    {
        printToken(tok);
    }
}


static void testXmlElementScan(const ByteSpan& s)
{
	XmlIteratorParams fParams{};
	XmlIteratorState fState{ s};
	fParams.fAutoScanAttributes = false;
    fParams.fSkipWhitespace = true;

	XmlElement elem;
	while (nextXmlElement(fParams, fState, elem))
	{
		waavs::printXmlElement(elem);
	}
}

static void testXmlElementGenerator(const ByteSpan& s)
{
	XmlElementGenerator gen(s);
	XmlElement elem;
	while (gen.next(elem))
	{
		waavs::printXmlElement(elem);
	}
}

/*
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
                printXmlElementInfo(elem);
        } else 
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
*/



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
    
    //testTokenizer(s);
	//testTokenGenerator(s);

    //testXmlElementScan(s);
    testXmlElementGenerator(s);

    //testXmlIter(s);
	//testElementContainer(s);
    //testElementFilter(s);


    // close the mapped file
    mapped->close();


    return 0;
}
