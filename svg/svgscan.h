#pragma once

#include "lang_grammar.h"
#include "core_nametable.h"

namespace waavs
{
    // Data structure to hold a function invocation.
    // We retain the name and the payload
    // Example:  "translate(10,20)"  would have name="translate" and payload="10,20"
    struct Invocation
    {
        ByteSpan name;
        InternedKey nameKey;
        ByteSpan payload;
    };



    // readInvocation()
    //
    // Read a function invocation from the source span, 
    // The invocation consists of an identifier
    // followed by a parenthesized body, which 
    // may contain nested parentheses.
    //
    static INLINE bool readInvocation(ByteSpan& src, Invocation& out, bool ignoreCase=false) noexcept
    {
        out.name.reset();
        out.payload.reset();

        bspan_skip_spaces(src);

        if (!identifier_read(src, out.name, chrAlphaChars, chrAlphaChars))
            return false;
        
        if (ignoreCase)
            out.nameKey = WSNameSet::INTERN_CI(out.name);
        else
            out.nameKey = WSNameSet::INTERN(out.name);

        bspan_skip_spaces(src);

        return bspan_read_enclosed(src, out.payload, '(', ')');
    }
}