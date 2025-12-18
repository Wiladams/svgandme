#pragma once


#include <cstdint>

#include "bspan.h"

enum XML_ELEMENT_TYPE {
    XML_ELEMENT_TYPE_INVALID = 0
    , XML_ELEMENT_TYPE_XMLDECL                      // <?xml version="1.0" encoding="UTF-8" standalone="yes"?>
    , XML_ELEMENT_TYPE_START_TAG                    // <tag>
    , XML_ELEMENT_TYPE_END_TAG                      // </tag>
    , XML_ELEMENT_TYPE_SELF_CLOSING                 // <tag/>
    , XML_ELEMENT_TYPE_EMPTY_TAG                    // <br>
    , XML_ELEMENT_TYPE_CONTENT                      // <tag>content</tag>
    , XML_ELEMENT_TYPE_COMMENT                      // <!-- comment -->
    , XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION       // <?target data?>
    , XML_ELEMENT_TYPE_CDATA                        // <![CDATA[<greeting>Hello, world!</greeting>]]>
    , XML_ELEMENT_TYPE_DOCTYPE                      // <!DOCTYPE greeting SYSTEM "hello.dtd">
    , XML_ELEMENT_TYPE_ENTITY                       // <!ENTITY hello "Hello">
};

namespace waavs {


    // XmlElementInfo
    // 
    // This data structure contains the raw scanned information for an XML node
    // The data is separated into the name component, the kind of node,
    // and the attributes are separated from the rest, but not 'parsed'.
    // 
    // In the case of a start, or self-closing tag, the name is separated
    // and the attributes are in the fData field
    // 
    // For other node types, the fData represents the content of the node
    struct XmlElementInfo
    {
        uint8_t fElementKind{ XML_ELEMENT_TYPE_INVALID };
        ByteSpan fNameSpan{};
        ByteSpan fData{};

        XmlElementInfo() = default;
		XmlElementInfo(const XmlElementInfo& other) noexcept
			: fElementKind(other.fElementKind)
			, fNameSpan(other.fNameSpan)
			, fData(other.fData)
		{
		}


        void reset()
        {
            fElementKind = XML_ELEMENT_TYPE_INVALID;
            fNameSpan.reset();
            fData.reset();
        }

        void reset(int kind, const ByteSpan& data)
        {
            fNameSpan = {};
            fElementKind = kind;
            fData = data;
        }

        void reset(int kind, const ByteSpan& data, const ByteSpan& name)
        {
            fElementKind = kind;
            fNameSpan = name;
            fData = data;
        }

        constexpr bool empty() const noexcept { return fElementKind == XML_ELEMENT_TYPE_INVALID; }
        explicit operator bool() const noexcept { return !empty(); }

        uint32_t kind() const noexcept { return fElementKind; }
        void setKind(const uint32_t kind) { fElementKind = kind; }

        ByteSpan nameSpan() const noexcept { return fNameSpan; }
        ByteSpan data() const noexcept { return fData; }

        // You can get the bytespan that represents a specific attribute value. 
        // If the attribute is not found, the function returns false
        // If it is found, true is returned, and the value is in the 'value' parameter
        // The attribute value is not parsed in any way.
        bool getRawAttributeValue(const ByteSpan& key, ByteSpan& value) const
        {
            value.reset();
            if (!isStart() && !isSelfClosing())
                return false;

            if (getKeyValue(data(), key, value))
                return true;

            return false;
        }



        // Convenience for what kind of tag it is
		constexpr bool isElementKind(uint32_t kind) const { return fElementKind == kind; }

        constexpr bool isXmlDecl() const { return fElementKind == XML_ELEMENT_TYPE_XMLDECL; }
        constexpr bool isStart() const { return (fElementKind == XML_ELEMENT_TYPE_START_TAG); }
        constexpr bool isSelfClosing() const { return fElementKind == XML_ELEMENT_TYPE_SELF_CLOSING; }
        constexpr bool isEnd() const { return fElementKind == XML_ELEMENT_TYPE_END_TAG; }
        constexpr bool isComment() const { return fElementKind == XML_ELEMENT_TYPE_COMMENT; }
        constexpr bool isProcessingInstruction() const { return fElementKind == XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION; }
        constexpr bool isContent() const { return fElementKind == XML_ELEMENT_TYPE_CONTENT; }
        constexpr bool isCData() const { return fElementKind == XML_ELEMENT_TYPE_CDATA; }
        constexpr bool isDoctype() const { return fElementKind == XML_ELEMENT_TYPE_DOCTYPE; }
        constexpr bool isEntityDeclaration() const { return fElementKind == XML_ELEMENT_TYPE_ENTITY; }

    };
}

namespace waavs {
struct XmlName {
        ByteSpan fQName{};
        ByteSpan fNamespace{};
        ByteSpan fName{};

        constexpr XmlName() noexcept = default;

        XmlName(const ByteSpan& src)
            :fQName(src)
        {
            reset(src);
        }

        XmlName(const XmlName& other) noexcept
        {
            reset(other.fQName);
        }

        XmlName(XmlName&& other) noexcept
            : fQName(other.fQName)
            , fNamespace(other.fNamespace)
            , fName(other.fName)
        {
            other.clear();
        }


        void clear()
        {
            fQName.reset();
            fNamespace.reset();
            fName.reset();
        }

        // Re-initialize this from the given span
        XmlName& reset(const ByteSpan& src)
        {
            fQName = src;
            fName = src;
            fNamespace = chunk_token(fName, charset(':'));
            if (fName.size() < 1)
            {
                fName = fNamespace;
                fNamespace = {};
            }
            return *this;
        }

        // Move assignment
        XmlName& operator=(XmlName&& other) noexcept
        {
            if (this != &other)
            {
                fQName = other.fQName;
                fNamespace = other.fNamespace;
                fName = other.fName;
                other.clear();
            }
            return *this;
        }

        // copy assignment
        XmlName& operator =(const XmlName& rhs)
        {
            if (this != &rhs)
            {
                fQName = rhs.fQName;  // Copy fQName
                fNamespace = rhs.fNamespace;
                fName = rhs.fName;
            }
            return *this;
        }

		bool operator==(const ByteSpan& rhs) const noexcept
		{
			return fQName == rhs;
		}

		bool operator!=(const ByteSpan& rhs) const noexcept
		{
			return fQName != rhs;
		}

        // fqname
        // Fully Qualified Name
        // This is the namespace, plus the basic name
        ByteSpan fqname() const noexcept
        {
            return fQName;
        }

        ByteSpan name() const noexcept { return fName; }         // The name
        ByteSpan ns() const noexcept { return fNamespace; }      // The namespace
    };
}

namespace waavs {

    // Representation of an xml element
    // The xml scanner will generate these
    struct XmlElement : public XmlElementInfo
    {
    private:
        XmlName fXmlName{};


        ByteSpan scanNameSpan(ByteSpan data)
        {
            ByteSpan s = data;
            bool start = false, end = false;

            if (!s)
                return {};

            if (*s == '/')
            {
                s++;
                end = true;
            }
            else {
                start = true;
            }

            fNameSpan = s;
            // skip leading whitespace
            while (s && !chrWspChars[*s])
                s++;

            fNameSpan.fEnd = s.fStart;
            fXmlName.reset(fNameSpan);

            return s;  // Instead of modifying `fData`, return the new position
        }

        // Clear this element to a default state
        void clear()
        {
            fElementKind = XML_ELEMENT_TYPE_INVALID;
            fXmlName.reset({});
            fData.reset();
        }


    public:
        XmlElement() = default;

        XmlElement(const XmlElement& other)
			: XmlElementInfo(other)
            , fXmlName(other.fXmlName)
        {
        }

        XmlElement(XmlElement&& other) noexcept
            :XmlElementInfo(other)
            ,fXmlName(std::move(other.fXmlName))
        {
            other.clear();
        }

		XmlElement(const XmlElementInfo& info)
			:XmlElementInfo(info)
		{
			//reset(info);
		}

        //XmlElement(int kind, const ByteSpan& data)
		//	:XmlElementInfo()
        //{
        //    reset(kind, data);
        //}

        // Returning information about the element
        XmlName xmlName() const { return fXmlName; }
        ByteSpan tagName() const { return fXmlName.name(); }
        ByteSpan tagNamespace() const { return fXmlName.ns(); }
        ByteSpan name() const { return fXmlName.name(); }


		XmlElement& reset(const XmlElementInfo& info)
		{
            clear();

			fElementKind = info.kind();
            fData = info.data();
            fNameSpan = info.nameSpan();
			fXmlName.reset(fNameSpan);
			return *this;
		}

        XmlElement & reset(int kind, const ByteSpan &tagName, const ByteSpan& data)
        {
            clear();
            fElementKind = kind;
            fData = data;
            fNameSpan = tagName;
            fXmlName.reset(fNameSpan);


            switch (kind)
            {
			    case XML_ELEMENT_TYPE_START_TAG:
			    case XML_ELEMENT_TYPE_SELF_CLOSING:
			    case XML_ELEMENT_TYPE_END_TAG:
				    scanNameSpan(fNameSpan);
				break;

                case XML_ELEMENT_TYPE_XMLDECL:
                case XML_ELEMENT_TYPE_COMMENT:
				case XML_ELEMENT_TYPE_CDATA:
				case XML_ELEMENT_TYPE_CONTENT:
				case XML_ELEMENT_TYPE_DOCTYPE:
				case XML_ELEMENT_TYPE_ENTITY:
				case XML_ELEMENT_TYPE_PROCESSING_INSTRUCTION:
				case XML_ELEMENT_TYPE_EMPTY_TAG:
				default:
					break;

            }


            return *this;
        }

        // Move assignment
        XmlElement& operator=(XmlElement&& other) noexcept
        {
            if (this != &other)
            {
                fElementKind = other.fElementKind;
                fNameSpan = other.fNameSpan;
                fXmlName = std::move(other.fXmlName);
                fData = other.fData;
                other.clear();
            }
            return *this;
        }

        XmlElement& operator=(const XmlElement& other)
        {
            if (this != &other)
            {
                fElementKind = other.fElementKind;
                fNameSpan = other.fNameSpan;
                fData = other.fData;
                fXmlName.reset(other.fXmlName.fqname());  // Use reset()
            }
            return *this;
        }

    };
}
