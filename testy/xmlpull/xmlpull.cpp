

#include "appmodule.h"
#include "app/mappedfile.h"
#include "app/xmlutil.h"


#include "wsenum.h"
#include "xmltokengen.h"
#include "xmlscan.h"


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


static void printXmlElementInfo(const XmlElement& elem)
{
    ByteSpan kindname{};
    getEnumKey(XML_ELEMENT_TYPE_WSEnum, elem.kind(), kindname);
    
    printf("[[");
    writeChunk(kindname);
    if (elem.name())
    {
        printf(" - ");
        writeChunk(elem.name());
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
	XmlPull iter( s);
	//iter.fParams.fAutoScanAttributes = false;
    //iter.fParams.fSkipWhitespace = true;

	while (iter.next())
	{
		waavs::printXmlElement(*iter);
	}
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

static double round_half_up(double x) { return floor(x + 0.49999999999999994); }

void test_round()
{
    double t1 = round_half_up(2.5); // 3
    double t2 = round_half_up(2.4); // 2
    double t3 = round_half_up(-2.5); // -2
    double t4 = round_half_up(-2.6); // -3

    printf("round_half_up(2.5) = %f\n", t1);
    printf("round_half_up(2.4) = %f\n", t2);
    printf("round_half_up(-2.5) = %f\n", t3);
    printf("round_half_up(-2.6) = %f\n", t4);
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

    waavs::ByteSpan s;
    s.resetFromSize(mapped->data(), mapped->size());
    
    //testTokenizer(s);
	//testTokenGenerator(s);

    testXmlElementScan(s);
    //testXmlElementGenerator(s);

    //testXmlIter(s);
	//testElementContainer(s);
    //testElementFilter(s);


    // close the mapped file
    mapped->close();


    return 0;
}
