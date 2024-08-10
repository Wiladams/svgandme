#pragma once



#include <map>

#include "xmlscan.h"
#include "collections.h"

namespace waavs {

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

    static void printXmlElement(const XmlElement& elem)
    {
        
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
            break;

        case XML_ELEMENT_TYPE_SELF_CLOSING:
            printf("SELF_CLOSING: ");
            printChunk(elem.nameSpan());
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

        // Print the attributes
        ByteSpan src = elem.data();

        ByteSpan key{};
        ByteSpan value{};
        while (nextAttributeKeyValue(src, key, value))
        {
            printf("  ");
            writeChunk(key);
            printf("   = ");
            printChunk(value);
        }
    }

	static void printXmlAttributes(const XmlAttributeCollection& attrColl)
	{

		for (const auto& attr : attrColl.attributes())
		{
			printf("  ");
			writeChunk(attr.first);
			printf("   = ");
			printChunk(attr.second);
		}
	}
}

