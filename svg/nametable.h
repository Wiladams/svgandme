#pragma once

#include <set>
#include <string>
#include <string_view>
#include <memory>
#include <unordered_map>

#include "bspan.h"


namespace waavs {
    //  Structure for using interned strings as keys in hash maps
    using InternedKey = const char*;

    struct InternedKeyHash {
        size_t operator()(InternedKey p) const noexcept {
            return std::hash<const void*>{}(p);
        }
    };

    struct InternedKeyEquivalent {
        bool operator()(InternedKey a, InternedKey b) const noexcept {
            return a == b;
        }
    };
}


namespace waavs {

    // PSNameTable
    //
    // A global name table for interned strings.
    struct PSNameTable 
    {
    private:
        std::set<std::string, std::less<> > pool;

    public:
        bool hasName(const char * name) const
        {
            return pool.find(name) != pool.end();
        }

        const char* intern(std::string_view sv)
        {
            // emplace does the lookup; constructs std::string only on insertion
            auto [it, inserted] = pool.emplace(sv);
            return it->c_str();
        }

        const char* intern(const ByteSpan& span) 
        { 
            return intern(std::string_view(reinterpret_cast<const char*>(span.data()), 
                span.size())); 
        }
        const char* intern(const char* cstr) { return intern(std::string_view(cstr)); }

        static PSNameTable* getSingletonTable() {
            static PSNameTable gTable;
            return &gTable;
        }

    public:
        static const char* INTERN(const ByteSpan& span) { return getSingletonTable()->intern(span); }
        static const char* INTERN(const char* cstr) { return getSingletonTable()->intern(cstr); }
    
        // Case-insensitive versions of the intern functions
        static const char* INTERN_CI(const ByteSpan& span)
        {
            std::string tmp;
            tmp.reserve(span.size());

            for (size_t i = 0; i < span.size(); ++i)
            {
                unsigned char ch = span[i];

                if (ch >= 'A' && ch <= 'Z')
                    ch = static_cast<unsigned char>(ch - 'A' + 'a');

                tmp.push_back(static_cast<char>(ch));
            }

            return getSingletonTable()->intern(std::string_view(tmp));
        }

        static const char* INTERN_CI(const char* cstr)
        {
            return INTERN_CI(ByteSpan(cstr));
        }
    };



}
