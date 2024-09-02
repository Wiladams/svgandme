#pragma once

#include <unordered_map>

#include "bspan.h"


namespace waavs {
    //============================================================
    // XmlAttributeCollection
    // A collection of the attibutes found on an XmlElement
    //============================================================

    struct XmlAttributeCollection
    {
        std::unordered_map<ByteSpan, ByteSpan, ByteSpanHash> fAttributes{};

        XmlAttributeCollection() = default;
        XmlAttributeCollection(const XmlAttributeCollection& other) noexcept
            :fAttributes(other.fAttributes)
        {}

        XmlAttributeCollection(const ByteSpan& inChunk) noexcept
        {
            scanAttributes(inChunk);
        }

        // Return a const attribute collection
        const std::unordered_map<ByteSpan, ByteSpan, ByteSpanHash>& attributes() const noexcept { return fAttributes; }

        size_t size() const noexcept { return fAttributes.size(); }

        void clear() noexcept { fAttributes.clear(); }

        // scanAttributes()
        // Given a chunk that contains attribute key value pairs
        // separated by whitespace, parse them, and store the key/value pairs 
        // in the fAttributes map
        bool scanAttributes(const ByteSpan& inChunk) noexcept
        {
            ByteSpan src = inChunk;
            ByteSpan key;
            ByteSpan value;

            while (readNextKeyValue(src, key, value))
            {
                addAttribute(key, value);
            }

            return true;
        }

        //bool hasAttribute(const std::string& inName) const
        bool hasAttribute(const ByteSpan& inName) const noexcept
        {
            return fAttributes.find(inName) != fAttributes.end();
        }


        // Add a single attribute to our collection of attributes
        // if the attribute already exists, replace its value
        // with the new value
        //void addAttribute(const std::string& name, const ByteSpan& valueChunk)
        void addAttribute(const ByteSpan& name, const ByteSpan& valueChunk) noexcept
        {
            fAttributes[name] = valueChunk;
        }


        //ByteSpan getAttribute(const std::string& name) const
        ByteSpan getAttribute(const ByteSpan& name) const noexcept
        {
            auto it = fAttributes.find(name);
            if (it != fAttributes.end())
                return it->second;
            return {};
        }


        XmlAttributeCollection& mergeAttributes(const XmlAttributeCollection& other) noexcept
        {
            for (auto& attr : other.fAttributes)
            {
                fAttributes[attr.first] = attr.second;
            }
            return *this;
        }

		// Given a name, find the attribute and return its value
        // return false if the name is not found
        static bool getValue(const ByteSpan &inChunk, const ByteSpan& key, ByteSpan& value) noexcept
        {
            ByteSpan src = inChunk;
            ByteSpan name{};

            while (readNextKeyValue(src, name, value))
            {
                if (name == key)
                    return true;
            }

            return false;
        }
    };
}

