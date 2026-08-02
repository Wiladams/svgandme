#pragma once

#include <set>
#include <string>
#include <string_view>
#include <memory>

#include "lang_span.h"


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
    struct WSNameSet 
    {
    private:
        std::set<std::string, std::less<> > pool;

    public:
        bool hasName(const char * name) const
        {
            return pool.find(name) != pool.end();
        }

        const char* findInterned(std::string_view sv) const noexcept
        {
            auto it = pool.find(sv);
            if (it == pool.end())
                return nullptr;

            return it->c_str();
        }

        const char* findInterned(const ByteSpan& span) const noexcept
        {
            return findInterned(
                std::string_view(
                    reinterpret_cast<const char*>(span.data()),
                    span.size()));
        }

        const char* findInterned(const char* cstr) const noexcept
        {
            auto it = pool.find(cstr);
            if (it == pool.end())
                return nullptr;

            return it->c_str();
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

        static WSNameSet* getSingletonTable() {
            static WSNameSet gTable;
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

        static const char* FIND(const ByteSpan& span)
        {
            return getSingletonTable()->findInterned(span);
        }

        static const char* FIND(const char* cstr)
        {
            return getSingletonTable()->findInterned(cstr);
        }
    };



}
