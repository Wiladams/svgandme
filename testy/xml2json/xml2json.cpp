

#include "appmodule.h"
#include "app/mappedfile.h"
#include "app/xmlutil.h"

#include "xmliter.h"
#include "wsenum.h"
#include "xmltojson.h"


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
        printf(" = ");
        printChunk(value);
    }
}

static void testTokenizer(const ByteSpan& src)
{
    XmlTokenState tokState = { src, false };
    XmlToken tok;

    while (nextXmlToken(tokState, tok))
    {
        printf("Token %d: [%.*s]\n", tok.type, (int)tok.value.size(), tok.value.data());
    }
}

static void testXmlToJson(const ByteSpan& src)
{
    // convert the XML to JSON
    printXmlToJson(src, stdout, true);
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
        printf("Usage: xml2json <xml file>  [output file]\n");
        return 1;
    }

    // create an mmap for the specified file
    const char* filename = argv[1];
    auto mapped = MappedFile::create_shared(filename);

    if (nullptr == mapped)
        return 0;

    waavs::ByteSpan s(mapped->data(), mapped->size());


    testXmlToJson(s);



    // close the mapped file
    mapped->close();


    return 0;
}
