#pragma once

#include "bspan_utils.h"

namespace waavs
{
    // Data structure to hold a function invocation.
    // We retain the name and the payload
    // Example:  "translate(10,20)"  would have name="translate" and payload="10,20"
    struct Invocation
    {
        ByteSpan name;
        ByteSpan payload;
    };



    // readInvocation()
    //
    // Read a function invocation from the source span, 
    // The invocation consists of an identifier
    // followed by a parenthesized body, which 
    // may contain nested parentheses.
    //
    static INLINE bool readInvocation(ByteSpan& src, Invocation& out) noexcept
    {
        out.name.reset();
        out.payload.reset();

        bspan_skip_spaces(src);

        if (!bspan_read_identifier(src, out.name, chrAlphaChars, chrAlphaChars))
            return false;

        bspan_skip_spaces(src);

        return bspan_read_enclosed(src, out.payload, '(', ')');
    }
}