// converters.h

#pragma once

//
// Converters
// Various routines to convert from one data type to another.
// Mainly parsing numeric types
//

#include <string>

#include "lang_grammar.h"

namespace waavs
{
    // In these routines, the difference between parse and read
    // is that parse does not modify the input ByteSpan, whereas
    // read will advance the start of the ByteSpan to after the
    // last character that was parsed.
    
    // parse64u
    // Parse a 64 bit unsigned integer from a string
    // return true if successful, false if not
    // If successful, the value is stored in the out parameter
    //
    static INLINE bool parse64u(const ByteSpan& inChunk, uint64_t& v) noexcept
    {
        ByteSpan s = inChunk;
        size_t digitsRead{ 0 };

        return u64_read(s, v, digitsRead);
    }

    static INLINE bool parseNumber(const ByteSpan& inChunk, double& value)
    {
        ByteSpan s = inChunk;
        return number_read(s, value);
    }

}
