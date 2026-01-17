#pragma once

#include <set>
#include <string>
#include <string_view>
#include <memory>

#include "bspan.h"

namespace waavs {

    // PSNameTable
    //
    // A global name table for interned strings.
    struct PSNameTable 
    {
    private:
        std::set<std::string, std::less<> > pool;

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
    };


}
