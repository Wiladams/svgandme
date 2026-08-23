#ifndef XML_ATTRIBUTE_COLLECTION_H_INCLUDED
#define XML_ATTRIBUTE_COLLECTION_H_INCLUDED

#include "core_nametable.h"
#include "core_table.h"
#include "lang_grammar.h"
#include "xml_scan.h"


namespace waavs
{
    using AttrKey = InternedKey;
    using AttrDictionary = WSNameMap<ByteSpan>;

    struct XmlAttributeCollection
    {
        AttrDictionary fAttributes{ 8 };

        const AttrDictionary& values() const noexcept { return fAttributes; }

        size_t size() const noexcept { return fAttributes.size(); }
        void clear() noexcept { fAttributes.clear(); }

        bool hasValue(AttrKey key) const noexcept
        {
            return fAttributes.contains(key);
        }

        void addValue(AttrKey key, const ByteSpan& valueChunk) noexcept
        {
            fAttributes.put(key, valueChunk);
        }

        void addValueBySpan(const ByteSpan& name, const ByteSpan& valueChunk) noexcept
        {
            addValue(WSNameSet::INTERN(name), valueChunk);
        }

        bool getValue(AttrKey key, ByteSpan& value) const noexcept
        {
            value.reset();

            if (!key)
            {
                return false;
            }

            return fAttributes.get(key, value);
        }

        ByteSpan getValue(AttrKey key) const noexcept
        {
            ByteSpan value{};
            fAttributes.get(key, value);
            return value;
        }

        bool getValueBySpan(const ByteSpan& name, ByteSpan& value) const noexcept
        {
            return getValue(WSNameSet::INTERN(name), value);
        }

        XmlAttributeCollection& mergeAttributes(const XmlAttributeCollection& other) noexcept
        {
            other.fAttributes.forEach([this](AttrKey key, const ByteSpan& value) {
                addValue(key, value);
                });

            return *this;
        }
    };

    // scanAttributes()
    // Given a chunk that contains attribute key value pairs
    // separated by whitespace, parse them, and store the key/value pairs 
    // in the fAttributes map
    static bool scanAttributes(XmlAttributeCollection& attrs, const ByteSpan& inChunk) noexcept
    {
        ByteSpan src = inChunk;
        ByteSpan name;
        ByteSpan value;

        while (xmlattribute_read_next(src, name, value))
        {
            attrs.addValueBySpan(name, value);
        }

        // if we consumed everything successfully, 
        // src should be empty
        bspan_ltrim_spaces(src);
        return src.empty();
    }
}

#endif
