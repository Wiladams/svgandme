#ifndef XMLTYPES_H_INCLUDED
#define XMLTYPES_H_INCLUDED



#include <cstdint>
#include <unordered_map>

#include "bspan.h"
#include "nametable.h"

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
            static_cast<const unsigned char*>(std::memchr(q.fStart, ':', q.size()));
        if (p) {
            prefix = ByteSpan(q.fStart, p);
            local = ByteSpan(p + 1, q.fEnd);
        }
    }

    struct XmlElement
    {
        uint8_t fElementKind{ XML_ELEMENT_TYPE_INVALID };

        ByteSpan fQName{};          // original raw name (could be "svg:rect")
        ByteSpan fLocalName{};      // local part ("rect")
        ByteSpan fPrefix{};         // prefix part ("svg")
        ByteSpan fData{};

        const char* fQNameAtom{ nullptr };      // Atomized name for faster comparisons
        const char* fLocalNameAtom{ nullptr };  // Atomized local name (without namespace)


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

            fQName.reset();
            fLocalName.reset();
            fPrefix.reset();

            fQNameAtom = nullptr;
            fLocalNameAtom = nullptr;
        }

        void reset(int kind, const ByteSpan& data)
        {
            fElementKind = kind;
            fData = data;
            fQName.reset();
            fLocalName.reset();
            fPrefix.reset();

            //fNameSpan = {};
            fQNameAtom = nullptr;
            fLocalNameAtom = nullptr;
        }

        void reset(int kind, const ByteSpan& name, const ByteSpan& data)
        {
            fElementKind = kind;
            //fNameSpan = name;
            fQName = name;
            fData = data;

            splitQName(fQName, fPrefix, fLocalName);

            if (kind == XML_ELEMENT_TYPE_START_TAG ||
                kind == XML_ELEMENT_TYPE_SELF_CLOSING ||
                kind == XML_ELEMENT_TYPE_END_TAG)
            {
                fQNameAtom = !fQName.empty() ? PSNameTable::INTERN(fQName) : nullptr;
                fLocalNameAtom = !fLocalName.empty() ? PSNameTable::INTERN(fLocalName) : nullptr;
            }
            else {
                fQNameAtom = nullptr;
                fLocalNameAtom = nullptr;
            }

        }

        constexpr bool empty() const noexcept { return fElementKind == XML_ELEMENT_TYPE_INVALID; }
        explicit operator bool() const noexcept { return !empty(); }

        uint32_t kind() const noexcept { return fElementKind; }
        void setKind(const uint32_t kind) { fElementKind = kind; }

        ByteSpan data() const noexcept { return fData; }

        //ByteSpan nameSpan() const noexcept { return fNameSpan; }
        ByteSpan qname() const noexcept { return fQName; }
        ByteSpan name() const noexcept { return fLocalName; }
        ByteSpan prefix() const noexcept { return fPrefix; }

        const char* qNameAtom() const noexcept { return fQNameAtom; }
        const char* nameAtom() const noexcept { return fLocalNameAtom; }

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
    //============================================================
    // XmlAttributeCollection
    // A collection of the attibutes found on an XmlElement
    //============================================================
    using AttrKey = InternedKey;
    using AttrDictionary = std::unordered_map<AttrKey, ByteSpan, InternedKeyHash, InternedKeyEquivalent>;

    //using BSpanDictionary = std::unordered_map<ByteSpan, ByteSpan, ByteSpanHash, ByteSpanEquivalent>;

    struct XmlAttributeCollection
    {
        AttrDictionary fAttributes{};

        XmlAttributeCollection() = default;
        XmlAttributeCollection(const XmlAttributeCollection& other) noexcept
            :fAttributes(other.fAttributes)
        {
        }


        // Return a const attribute collection
        const AttrDictionary& attributes() const noexcept { return fAttributes; }

        size_t size() const noexcept { return fAttributes.size(); }

        void clear() noexcept { fAttributes.clear(); }

        bool hasAttribute(AttrKey key) const noexcept
        {
            return key && (fAttributes.find(key) != fAttributes.end());
        }

        bool hasAttribute(const ByteSpan &name) const noexcept
        {
            AttrKey key = PSNameTable::INTERN(name);
            return hasAttribute(key);
        }


        // Add a single attribute to our collection of attributes
        // if the attribute already exists, replace its value
        // with the new value
        void addAttribute(const ByteSpan& name, const ByteSpan& valueChunk) noexcept
        {
            AttrKey key = PSNameTable::INTERN(name);
            fAttributes[key] = valueChunk;
        }

        bool getAttributeInterned(AttrKey key, ByteSpan& value) const noexcept
        {
            if (!key)
            {
                value.reset();
                return false;
            }
            auto it = fAttributes.find(key);
            if (it != fAttributes.end()) {
                value = it->second;
                return true;
            }
            value.reset(); // Explicitly clear value if attribute doesn't exist
            return false;
        }

        bool getAttribute(const ByteSpan& name, ByteSpan& value) const noexcept = delete;

        // get an attribute from the collection, based on a bytespan name
        bool getAttributeBySpan(const ByteSpan& name, ByteSpan& value) const noexcept
        {
            AttrKey key = PSNameTable::INTERN(name);
            return getAttributeInterned(key, value);
        }

        // Find an attribute based on a c-string name which is not interned
        bool getAttribute(const char* name, ByteSpan& value) const noexcept
        {
            AttrKey key = PSNameTable::INTERN(name);
            return getAttributeInterned(key, value);
        }


        // mergeAttributes()
        // Combine collections of attributes
        // If there are duplicates, the new value will replace the old
        XmlAttributeCollection& mergeAttributes(const XmlAttributeCollection& other) noexcept
        {
            for (const auto& attr : other.fAttributes)
            {
                fAttributes[attr.first] = attr.second;
                // Optimize update: Remove old value first, then emplace new one
                //fAttributes.erase(attr.first);
                //fAttributes.emplace(attr.first, attr.second);
            }
            return *this;
        }


    };
}

/*
ByteSpan scanNameSpan(ByteSpan data)
{
    ByteSpan s = data;
    //bool start = false;
    //bool end = false;

    if (!s)
        return {};

    if (*s == '/')
    {
        s++;
    }

    fNameSpan = s;
    // skip leading whitespace
    while (s && !chrWspChars[*s])
        s++;

    fNameSpan.fEnd = s.fStart;
    fXmlName.reset(fNameSpan);

    return s;  // Instead of modifying `fData`, return the new position
}
*/

#endif // XMLTYPES_H_INCLUDED
