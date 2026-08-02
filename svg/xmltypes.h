#ifndef XMLTYPES_H_INCLUDED
#define XMLTYPES_H_INCLUDED



#include "lang_scanner.h"
#include "core_nametable.h"


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

namespace waavs
{
    // XmlDocTypeDecl
    // Data structure for holding onto DOCTYPE declaration
    struct XmlDocTypeDecl
    {
        ByteSpan rootName{};
        ByteSpan externalKind{};    // PUBLIC or SYSTEM, could be interned
        ByteSpan publicId{};
        ByteSpan systemId{};
        ByteSpan internalSubset{};  // content sinside [...]
    };
}

namespace waavs 
{
    // XmlElement
    // 
    // This data structure contains the raw scanned information for an XML element.
    // The information is separated into three components
    // 1.  fElementKind - What kind of element is it?  start tag?  PI, TEXT, etc
    // 2.  fQName - The part of the element that indicates a name, if it's a tag
    // 3.  fData - If a tag, it's the span that includes the attributes.  For elements
    //     That have content (TEXT, CDATA, etc), it's the content between start and end tags
    // 
    // 

    static void splitQName(const ByteSpan& q, ByteSpan& prefix, ByteSpan& local) noexcept
    {
        prefix.reset();
        local = q;
        if (!q) return;

        // find ':'; if found, split
        const unsigned char* p =
            static_cast<const unsigned char*>(std::memchr(q.begin(), ':', q.size()));
        if (p) {
            prefix = ByteSpan::fromPointers(q.begin(), p);
            local = ByteSpan::fromPointers(p + 1, q.end());
        }
    }

    // Efficiently reads the next key-value attribute pair from `src`
    // Attributes are separated by '=' and values are enclosed in '"' or '\''
    static INLINE bool xmlattribute_read_next(
        ByteSpan& src,
        ByteSpan& key,
        ByteSpan& value) noexcept
    {
        key.reset();
        value.reset();

        bspan_ltrim_spaces(src);

        if (!src || *src == '/')
            return false;

        key = bspan_read_until(src, '=');
        bspan_trim_spaces(key);

        if (!key)
            return false;

        bspan_ltrim_spaces(src);

        if (!bspan_read_quoted(src, value))
            return false;

        return true;
    }

    // getXmlAttributeValue()
    // Get a specific attribute value from a span of attributes.
    //
    static bool getXmlAttributeValue(const ByteSpan& attrSpan,
        const ByteSpan& wantedKey, ByteSpan& value) noexcept
    {
        value.reset();

        ByteSpan src = attrSpan;

        while (src)
        {
            ByteSpan key{};
            ByteSpan val{};

            if (!xmlattribute_read_next(src, key, val))
                return false;

            if (key == wantedKey)
            {
                value = val;
                return true;
            }
        }

        return false;
    }


    struct XmlElement
    {
        uint8_t fElementKind{ XML_ELEMENT_TYPE_INVALID };

        ByteSpan fData{};

        const char* fQNameAtom{ nullptr };      // Atomized name for faster comparisons
        const char* fLocalNameAtom{ nullptr };  // Atomized local name (without namespace)
        const char* fPrefixAtom{ nullptr };     // Atomized prefix

        XmlElement() = default;

        // Copy
        XmlElement(const XmlElement&) noexcept = default;
        
        // Move constructor
        XmlElement(XmlElement&&) noexcept = default;

        ~XmlElement() = default;


        // copy assignment
        XmlElement& operator = (const XmlElement&) noexcept = default;
        // move assignment
        XmlElement& operator=(XmlElement&&) noexcept = default;


        void reset()
        {
            fElementKind = XML_ELEMENT_TYPE_INVALID;
            fData.reset();

            fQNameAtom = nullptr;
            fLocalNameAtom = nullptr;
            fPrefixAtom = nullptr;
        }

        void reset(int kind, const ByteSpan& data)
        {
            fElementKind = kind;
            fData = data;

            fQNameAtom = nullptr;
            fLocalNameAtom = nullptr;
            fPrefixAtom = nullptr;
        }

        void reset(int kind, const ByteSpan& name, const ByteSpan& data)
        {
            fElementKind = kind;
            ByteSpan fQName = name;
            ByteSpan fLocalName{};
            ByteSpan fPrefix{};

            fData = data;

            splitQName(fQName, fPrefix, fLocalName);

            if (kind == XML_ELEMENT_TYPE_START_TAG ||
                kind == XML_ELEMENT_TYPE_SELF_CLOSING ||
                kind == XML_ELEMENT_TYPE_END_TAG)
            {
                fQNameAtom = !fQName.empty() ? WSNameSet::INTERN(fQName) : nullptr;
                fLocalNameAtom = !fLocalName.empty() ? WSNameSet::INTERN(fLocalName) : nullptr;
                fPrefixAtom = !fPrefix.empty() ? WSNameSet::INTERN(fPrefix) : nullptr;
            }
            else {
                fQNameAtom = nullptr;
                fLocalNameAtom = nullptr;
                fPrefixAtom = nullptr;
            }

        }

        constexpr bool empty() const noexcept { return fElementKind == XML_ELEMENT_TYPE_INVALID; }
        explicit operator bool() const noexcept { return !empty(); }

        uint32_t kind() const noexcept { return fElementKind; }
        void setKind(const uint32_t kind) { fElementKind = kind; }

        ByteSpan data() const noexcept { return fData; }

        const char* qNameAtom() const noexcept { return fQNameAtom; }
        const char* nameAtom() const noexcept { return fLocalNameAtom; }
        const char* prefixAtom() const noexcept { return fPrefixAtom; } 


        // You can get the bytespan that represents a specific attribute value. 
        // If the attribute is not found, the function returns false
        // If it is found, true is returned, and the value is in the 'value' parameter
        // The attribute value is not parsed in any way.
        bool getElementAttribute(const ByteSpan& key, ByteSpan& value) const
        {
            value.reset();

            // Only a start tag or self-closing tag can have attributes, 
            // so if this is not one of those, return false immediately
            if (!isStart() && !isSelfClosing())
                return false;

            return getXmlAttributeValue(data(), key, value);
        }

        // Get an attribute using an interned string as a key.
        bool getElementAttribute(const char* key, ByteSpan& value) const
        {
            return getElementAttribute(ByteSpan(key), value);
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


#endif