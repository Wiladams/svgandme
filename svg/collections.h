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
        XmlAttributeCollection(const XmlAttributeCollection& other)
            :fAttributes(other.fAttributes)
        {}

        XmlAttributeCollection(const ByteSpan& inChunk)
        {
            scanAttributes(inChunk);
        }

        // Return a const attribute collection
        const std::unordered_map<ByteSpan, ByteSpan, ByteSpanHash>& attributes() const { return fAttributes; }

        size_t size() const { return fAttributes.size(); }

        virtual void clear() { fAttributes.clear(); }

        // scanAttributes()
        // Given a chunk that contains attribute key value pairs
        // separated by whitespace, parse them, and store the key/value pairs 
        // in the fAttributes map
        bool scanAttributes(const ByteSpan& inChunk)
        {
            ByteSpan src = inChunk;
            ByteSpan key;
            ByteSpan value;

            while (nextAttributeKeyValue(src, key, value))
            {
                //std::string keyStr((const char*)key.fStart, key.size());
                //fAttributes[keyStr] = value;
                fAttributes[key] = value;
            }

            return true;
        }

        //bool hasAttribute(const std::string& inName) const
        bool hasAttribute(const ByteSpan& inName) const
        {
            return fAttributes.find(inName) != fAttributes.end();
        }


        // Add a single attribute to our collection of attributes
        // if the attribute already exists, replace its value
        // with the new value
        //void addAttribute(const std::string& name, const ByteSpan& valueChunk)
        void addAttribute(const ByteSpan& name, const ByteSpan& valueChunk)
        {
            fAttributes[name] = valueChunk;
        }


        //ByteSpan getAttribute(const std::string& name) const
        ByteSpan getAttribute(const ByteSpan& name) const
        {
            auto it = fAttributes.find(name);
            if (it != fAttributes.end())
                return it->second;
            return {};
        }


        XmlAttributeCollection& mergeProperties(const XmlAttributeCollection& other)
        {
            for (auto& attr : other.fAttributes)
            {
                fAttributes[attr.first] = attr.second;
            }
            return *this;
        }

    };
}

