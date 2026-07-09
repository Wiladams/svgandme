

#include "appmodule.h"
#include "app_mappedfile.h"
#include "app/xmlutil.h"


#include "wsenum.h"
#include "xml_scan_element.h"
#include "xml_pull.h"
#include "svgatoms.h"
#include "xml_scan_token.h"

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
    if (elem.nameAtom())
    {
        printf(" - %s", elem.nameAtom());
        //writeChunk(elem.name());
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
    while (xmlattribute_read_next(src, key, value))
    {
        printf("  ");
        writeChunk(key);
        printf(" = ");
        printChunk(value);
    }
}

/*
static void printToken(const XmlToken& tok)
{
	ByteSpan kindname{};
	getEnumKey(XML_TOKEN_TYPE_WSEnum, tok.type, kindname);
	printf("Token [%d]  %.*s : [%.*s]\n",
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
*/

static void testXmlPull(const ByteSpan& s)
{
    XmlPull iter(s);

    while (iter.next())
    {
        waavs::printXmlElement(*iter);
    }
}

static void testXmlElementScan(const ByteSpan& s)
{
    ByteSpan src = s;
    XmlIteratorParams params{};
    XmlElement elem;

    while (read_next_xml_element_direct(params, src, elem))
    {
        waavs::printXmlElement(elem);
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



static void test_xml_attributes_and_collection()
{
    printf("\n==== test_xml_attributes_and_collection ====\n");

    static const char* xml = R"||(
<rect
    id="r1"
    class="primary hot"
    x="10"
    y="20"
    width="300"
    height="150"
    fill="red"
    stroke="black"
    stroke-width="2"
    opacity="0.5"
    transform="translate(10,20)"
    style="fill:blue; opacity:0.75; stroke-width:4"
/>
)||";

    XmlPull pull(xml);

    if (!pull.next())
    {
        printf("FAIL: could not read first XML element\n");
        return;
    }

    const XmlElement& elem = *pull;

    if (!elem.isSelfClosing())
    {
        printf("FAIL: expected self-closing element\n");
        return;
    }

    printf("Element name: %s\n", elem.nameAtom());

    ByteSpan value{};

    if (elem.getElementAttribute(svgattr::id(), value))
    {
        printf("id = ");
        printChunk(value);
    }
    else
    {
        printf("FAIL: id not found\n");
    }

    if (elem.getElementAttribute(svgattr::klass(), value))
    {
        printf("class = ");
        printChunk(value);
    }
    else
    {
        printf("FAIL: class not found\n");
    }

    if (elem.getElementAttribute(svgattr::fill(), value))
    {
        printf("raw fill = ");
        printChunk(value);
    }
    else
    {
        printf("FAIL: fill not found\n");
    }

    if (elem.getElementAttribute(svgattr::style(), value))
    {
        printf("style = ");
        printChunk(value);
    }
    else
    {
        printf("FAIL: style not found\n");
    }

    printf("\n-- scan raw attributes into XmlAttributeCollection --\n");

    XmlAttributeCollection presentationAttrs{};

    ByteSpan src = elem.data();
    ByteSpan attrName{};
    ByteSpan attrValue{};

    while (xmlattribute_read_next(src, attrName, attrValue))
    {
        if (attrName == "style")
            continue;

        presentationAttrs.addValueBySpan(attrName, attrValue);
    }

    printXmlAttributeCollection(presentationAttrs);

    printf("\n-- verify collection lookup --\n");

    value.reset();
    if (presentationAttrs.getValue(svgattr::width(), value))
    {
        printf("width = ");
        printChunk(value);
    }
    else
    {
        printf("FAIL: width not found in collection\n");
    }

    value.reset();
    if (presentationAttrs.getValue(svgattr::stroke_width(), value))
    {
        printf("stroke-width before overwrite = ");
        printChunk(value);
    }
    else
    {
        printf("FAIL: stroke-width not found in collection\n");
    }

    printf("\n-- overwrite existing attribute --\n");

    presentationAttrs.addValue(svgattr::fill(), ByteSpan("green"));
    presentationAttrs.addValue(svgattr::stroke_width(), ByteSpan("5"));

    value.reset();
    if (presentationAttrs.getValue(svgattr::fill(), value))
    {
        printf("fill after overwrite = ");
        printChunk(value);
    }

    value.reset();
    if (presentationAttrs.getValue(svgattr::stroke_width(), value))
    {
        printf("stroke-width after overwrite = ");
        printChunk(value);
    }

    printf("\n-- merge CSS/style attributes --\n");

    XmlAttributeCollection styleAttrs{};
    styleAttrs.addValue(svgattr::fill(), ByteSpan("blue"));
    styleAttrs.addValue(svgattr::opacity(), ByteSpan("0.75"));
    styleAttrs.addValue(svgattr::stroke_width(), ByteSpan("4"));

    presentationAttrs.mergeAttributes(styleAttrs);

    printXmlAttributeCollection(presentationAttrs);

    printf("\n-- verify merge precedence --\n");

    value.reset();
    if (presentationAttrs.getValue(svgattr::fill(), value))
    {
        printf("merged fill = ");
        printChunk(value);
    }

    value.reset();
    if (presentationAttrs.getValue(svgattr::opacity(), value))
    {
        printf("merged opacity = ");
        printChunk(value);
    }

    value.reset();
    if (presentationAttrs.getValue(svgattr::stroke_width(), value))
    {
        printf("merged stroke-width = ");
        printChunk(value);
    }

    printf("==== end test_xml_attributes_and_collection ====\n");
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
    
    //testTokenizer("<svg><rect width='100' height='150' /></svg>");
	//testTokenGenerator(s);

    //testXmlElementScan("<svg><!-- comment --> <rect width='100' height='150' /></svg>");
    testXmlElementScan(s);
    //test_xml_attributes_and_collection();



    // close the mapped file
    mapped->close();


    return 0;
}
