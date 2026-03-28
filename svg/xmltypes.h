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

namespace waavs 
{
    // readNextCSSKeyValue()
    // 
    // Properties are separated by ';'
    // values are separated from the key with ':'
    // Ex: <tagname style="stroke:black;fill:white" />
    // Return
    //   true - if a valid key/value pair was found
    //      in this case, key, and value will be populated
    //   false - if no key/value pair was found, or end of string
    //      in this case, key, and value will be undefined
    //
    static bool readNextCSSKeyValue(ByteSpan& src, ByteSpan& key, ByteSpan& value, const unsigned char fieldDelimeter = ';', const unsigned char keyValueSeparator = ':') noexcept
    {
        // Trim leading whitespace to begin
        src = chunk_ltrim(src, chrWspChars);

        // If the string is now blank, return immediately
        if (!src)
            return false;

        // peel off a key/value pair by taking a token up to the fieldDelimeter
        value = chunk_token_char(src, fieldDelimeter);

        // Now, separate the key from the value using the keyValueSeparator
        key = chunk_token_char(value, keyValueSeparator);

        // trim the key and value fields of whitespace
        key = chunk_trim(key, chrWspChars);
        value = chunk_trim(value, chrWspChars);

        return true;
    }

    // Efficiently reads the next key-value attribute pair from `src`
    // Attributes are separated by '=' and values are enclosed in '"' or '\''
    static bool readNextKeyAttribute(ByteSpan& src, ByteSpan& key, ByteSpan& value) noexcept
    {
        key.reset();
        value.reset();

        // Trim leading whitespace
        src.skipWhile(chrWspChars);

        if (!src)
            return false;

        // Handle end tag scenario (e.g., '/>')
        if (*src == '/')
            return false;

        // Capture attribute name up to '='
        const uint8_t* keyStart = src.fStart;
        const uint8_t* keyEnd = keyStart; // track last non-whitespace char seen
        while (src.fStart < src.fEnd && *src.fStart != '=')
        {
            if (!chrWspChars(*src.fStart))
            {
                keyEnd = src.fStart + 1; // past the last non-space character
            }
            ++src.fStart;
        }

        // If no '=' found, return false
        if (src.empty())
            return false;

        // Assign key, trimmed to exclude any trailing whitespace
        key = ByteSpan::fromPointers(keyStart, keyEnd);

        // Move past '='
        ++src.fStart;

        // Skip any whitespace
        src.skipWhile(chrWspChars);

        if (src.empty())
            return false;

        // Ensure we have a quoted value
        uint8_t quoteChar = *src;
        if (quoteChar != '"' && quoteChar != '\'')
            return false;

        // Move past the opening quote
        src++;

        // Locate the closing quote using `memchr`
        const uint8_t* endQuote = static_cast<const uint8_t*>(std::memchr(src.fStart, quoteChar, src.size()));

        if (!endQuote)
            return false; // No closing quote found

        // Assign the attribute value (excluding quotes)
        value = ByteSpan::fromPointers(src.fStart, endQuote);

        // Move past the closing quote
        src.fStart = endQuote + 1;

        return true;
    }


    // getKeyValue()
    //
    // Searches `inChunk` for an attribute `key` and returns its value if found
    static bool getKeyValue(const ByteSpan& inChunk, const ByteSpan& key, ByteSpan& value) noexcept
    {
        ByteSpan src = inChunk;
        ByteSpan name{};
        bool insideQuotes = false;
        uint8_t quoteChar = 0;

        while (src)
        {
            // Skip leading whitespace
            src = chunk_ltrim(src, chrWspChars);

            if (!src)
                return false;

            // If we hit a quote, skip the entire quoted section
            if (*src == '"' || *src == '\'')
            {
                quoteChar = *src;
                src++; // Move past opening quote

                // Use `chunk_find_char` to efficiently skip to closing quote
                src = chunk_find_char(src, quoteChar);
                if (!src)
                    return false;

                src++; // Move past closing quote
                continue;
            }

            // Extract the next token as a potential key
            ByteSpan keyCandidate = chunk_token_char(src, '=');
            keyCandidate = chunk_trim(keyCandidate, chrWspChars);

            // If this matches the requested key, extract the value
            if (keyCandidate == key)
            {
                // Skip whitespace before value
                src = chunk_ltrim(src, chrWspChars);

                if (!src)
                    return false;

                // **Only accept quoted values**
                if (*src == '"' || *src == '\'')
                {
                    quoteChar = *src;
                    src++;
                    value.fStart = src.fStart;

                    // Find the closing quote **quickly**
                    src = chunk_find_char(src, quoteChar);
                    if (!src)
                        return false;

                    value.fEnd = src.fStart;  // Exclude the closing quote
                    src++;
                    return true; // Successfully found key and value
                }

                // **Reject unquoted values in XML**
                return false;
            }

            // If there was no `=`, continue scanning
            src = chunk_ltrim(src, chrWspChars);
            if (src && *src == '=')
                src++; // Skip past '=' and continue parsing
        }

        return false; // Key not found
    }

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
            prefix = ByteSpan::fromPointers(q.fStart, p);
            local = ByteSpan::fromPointers(p + 1, q.fEnd);
        }
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
                fQNameAtom = !fQName.empty() ? PSNameTable::INTERN(fQName) : nullptr;
                fLocalNameAtom = !fLocalName.empty() ? PSNameTable::INTERN(fLocalName) : nullptr;
                fPrefixAtom = !fPrefix.empty() ? PSNameTable::INTERN(fPrefix) : nullptr;
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

        //ByteSpan qname() = delete; // const noexcept { return fQName; }
        //ByteSpan name() = delete; // const noexcept { return fLocalName; }
        //ByteSpan prefix() = delete; // const noexcept { return fPrefix; }

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
    //============================================================
    // XmlAttributeCollection
    // A collection of the attibutes found on an XmlElement
    //============================================================
    using AttrKey = InternedKey;
    using AttrDictionary = std::unordered_map<AttrKey, ByteSpan, InternedKeyHash, InternedKeyEquivalent>;

    struct XmlAttributeCollection
    {
        AttrDictionary fAttributes{};

        XmlAttributeCollection() = default;
        XmlAttributeCollection(const XmlAttributeCollection& other) noexcept
            :fAttributes(other.fAttributes)
        {
        }


        // Return a const attribute collection
        const AttrDictionary& values() const noexcept { return fAttributes; }

        size_t size() const noexcept { return fAttributes.size(); }
        void clear() noexcept { fAttributes.clear(); }

        bool hasValue(AttrKey key) const noexcept {return key && (fAttributes.find(key) != fAttributes.end());}


        // Add a single attribute to our collection of attributes
        // if the attribute already exists, replace its value
        // with the new value
        void addValue(AttrKey key, const ByteSpan& valueChunk) noexcept
        {
            if (!key)
            {
                return;
            }
            fAttributes[key] = valueChunk;
        }

        void addValueBySpan(const ByteSpan& name, const ByteSpan& valueChunk) noexcept
        {
            AttrKey key = PSNameTable::INTERN(name);
            return addValue(key, valueChunk);
        }

        bool getValue(AttrKey key, ByteSpan& value) const noexcept
        {
            if (!key)
            {
                printf("XmlAttributeCollection::getValue(), invalid key\n");
                value.reset();
                return false;
            }
            auto it = fAttributes.find(key);
            if (it != fAttributes.end()) {
                value = it->second;
                return true;
            }

            // Do not explicitly clear value if attribute doesn't exist
            // That way, the caller can decide to set a value and override
            // it only if the attribute is found, or leave it alone if not found
            // This assumes the caller will clear the value before calling getValue() 
            // if they want it cleared when not found
            //value.reset(); 
            return false;
        }

        // get an attribute from the collection, based on a bytespan name
        bool getValueBySpan(const ByteSpan& name, ByteSpan& value) const noexcept
        {
            AttrKey key = PSNameTable::INTERN(name);
            return getValue(key, value);
        }


        // mergeAttributes()
        // 
        // Combine collections of attributes
        // If there are duplicates, the new value will replace the old
        XmlAttributeCollection& mergeAttributes(const XmlAttributeCollection& other) noexcept
        {
            for (const auto& attr : other.fAttributes)
            {
                fAttributes[attr.first] = attr.second;
            }
            return *this;
        }


    };
}


#endif // XMLTYPES_H_INCLUDED
