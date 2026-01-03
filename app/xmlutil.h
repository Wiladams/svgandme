#ifndef XMLUTIL_H_INCLUDED
#define XMLUTIL_H_INCLUDED




#include <map>

#include "xmlscan.h"

// Forward declarations
namespace waavs 
{
    static void printXmlAttributes(const XmlElement& elem);
    static void printXmlAttributeCollection(const XmlAttributeCollection& attrColl);
    static void printXmlElement(const XmlElement& elem);

}


namespace waavs {

    static void printXmlAttributes(const XmlElement& elem)
    {
        ByteSpan src = elem.data();
        ByteSpan key{};
        ByteSpan value{};

        printf("ATTRIBUTES:\n");
        //printChunk(elem.data());

        while (readNextKeyAttribute(src, key, value))
        {
            printf("  ");
            writeChunk(key);
            printf("   = ");
            printChunk(value);
        }
    }

    static void printXmlAttributeCollection(const XmlAttributeCollection& attrColl)
    {

        for (const auto& attr : attrColl.attributes())
        {
            printf("  ");
            writeChunk(attr.first);
            printf("   = ");
            printChunk(attr.second);
        }
    }

    // printXmlElement
    // Print some basic information about an XML element
    // The name, they type, and the attribute values
    static void printXmlElement(const XmlElement& elem)
    {
        /*
		static WSEnum xmlTypeMap = {
			{ "ELEMENT", 0 },
			{ "ATTRIBUTE", 1 },
			{ "TEXT", 2 },
			{ "CDATA", 3 },
			{ "ENTITY_REF", 4 },
			{ "ENTITY", 5 },
			{ "PI", 6 },
			{ "COMMENT", 7 },
			{ "DOC", 8 },
			{ "DOC_TYPE", 9 },
			{ "DOC_FRAG", 10 },
			{ "NOTATION", 11 },
			{ "WHITESPACE", 12 },
			{ "SIGNIFICANT_WHITESPACE", 13 },
			{ "END_ELEMENT", 14 },
			{ "END_ENTITY", 15 },
			{ "XML_DECL", 16 }
		};
        */
        static std::map<int, std::string> elemTypeNames = {
         {XML_ELEMENT_TYPE_INVALID, "INVALID"}
        ,{XML_ELEMENT_TYPE_CONTENT, "CONTENT"}
        ,{XML_ELEMENT_TYPE_SELF_CLOSING, "SELF_CLOSING"}
        ,{XML_ELEMENT_TYPE_START_TAG, "START_TAG"}
        ,{XML_ELEMENT_TYPE_END_TAG, "END_TAG"}
        ,{XML_ELEMENT_TYPE_COMMENT, "COMMENT"}
        ,{XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION, "PROCESSING_INSTRUCTION"}
        ,{XML_ELEMENT_TYPE_CDATA, "CDATA"}
        ,{XML_ELEMENT_TYPE_XMLDECL, "XMLDECL"}
        ,{XML_ELEMENT_TYPE_DOCTYPE, "DOCTYPE"}
        };


        printf("[[");

        switch (elem.kind())
        {
        case XML_ELEMENT_TYPE_CONTENT:
        case XML_ELEMENT_TYPE_COMMENT:
        case XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION:
        case XML_ELEMENT_TYPE_DOCTYPE:
        case XML_ELEMENT_TYPE_CDATA:
            printf("%s: \n", elemTypeNames[elem.kind()].c_str());
            printChunk(elem.data());
            break;


        case XML_ELEMENT_TYPE_START_TAG:
            printf("START_TAG: ");
            printChunk(elem.nameSpan());
            if (elem.data())
                printXmlAttributes(elem);
            break;

        case XML_ELEMENT_TYPE_SELF_CLOSING:
            printf("SELF_CLOSING: ");
            printChunk(elem.nameSpan());
            if (elem.data())
                printXmlAttributes(elem);
            break;

        case XML_ELEMENT_TYPE_END_TAG:
            printf("END_TAG: ");
            printChunk(elem.nameSpan());
            break;

        default:
            printf("NYI: %s\n", elemTypeNames[elem.kind()].c_str());
            printChunk(elem.data());
            break;
        }



    }


}

#endif // XMLUTIL_H_INCLUDED