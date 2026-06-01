// bspan_utils.h

#pragma once

#include <cstring>  // for std::memchr

#include "bspan.h"
#include "scanning.h"
#include "charset.h"


namespace waavs
{
    // copyFrom()
    // 
    // BUGBUG: This is kind of dangerous, as we're only copying stuff
    // and not doing any bounds checking
    static INLINE void bspan_copy_from(ByteSpan &a, const void* src, size_t sz) noexcept
    {
        if (sz > 0) 
            std::memcpy(const_cast<uint8_t*>(a.data()), src, sz);
    }

    static INLINE bool bspan_starts_with(const ByteSpan &a, const ByteSpan& b) noexcept
    {
        return (a.take(b.size()) == b);
    }

    static INLINE bool bspan_ends_with(const ByteSpan &a, const ByteSpan& b) noexcept
    {
        return (a.subSpan(a.size() - b.size(), b.size()) == b);
    }

    static INLINE ByteSpan bspan_skip_spaces(ByteSpan &a) noexcept
    {
        const uint8_t* start = find_first_not_of4(a.begin(), a.end(), ' ', '\t', '\r', '\n');
        a.resetStart(start);
        
        return a;
    }

    static INLINE ByteSpan& bspan_skip_while(ByteSpan &a, const charset& aset) noexcept
    {
        const unsigned char * startAt = aset.skipWhile(a.begin(), a.end());
        a.resetStart(startAt);

        return a;
    }

    static INLINE ByteSpan bspan_read_prefix(ByteSpan& src, const charset& allowed) noexcept
    {
        const uint8_t* start = src.begin();
        const uint8_t* end = src.end();

        const uint8_t* p = allowed.skipWhile(start, end);
        src.resetStart(p);

        return ByteSpan::fromPointers(start, p);
    }
}




namespace waavs {

    static INLINE int comparen(const ByteSpan& a, const ByteSpan& b, int n) noexcept
    {
        size_t maxBytes = a.size() < b.size() ? a.size() : b.size();
        if (maxBytes > n)
            maxBytes = n;
        return memcmp(a.data(), b.data(), maxBytes);
    }

    static INLINE int comparen_cstr(const ByteSpan& a, const char* b, int n) noexcept
    {
        size_t maxBytes = a.size() < n ? a.size() : n;

        return memcmp(a.data(), b, maxBytes);
    }

}



// Implementation of hash function for ByteSpan
// so it can be used in 'map' collections
// Note:  We've moved away from this style, preferring
// interned strings as keys in maps instead
/*
namespace std {
    template<>
    struct hash<waavs::ByteSpan> {
        size_t operator()(const waavs::ByteSpan& span) const
        {
            return waavs::fnv1a_32(span.data(), span.size());
        }
    };
}
*/

namespace waavs {
    struct ByteSpanHash {
        size_t operator()(const ByteSpan& span) const noexcept {
            return waavs::fnv1a_32(span.data(), span.size());
        }
    };

    struct ByteSpanEquivalent {
        bool operator()(const ByteSpan& a, const ByteSpan& b) const noexcept {
            if (a.size() != b.size())
                return false;
            return memcmp(a.data(), b.data(), a.size()) == 0;
        }
    };

    // Case insensitive 'string' comparison
    struct ByteSpanInsensitiveHash {
        size_t operator()(const ByteSpan& span) const noexcept {
            return waavs::fnv1a_32_case_insensitive(span.data(), span.size());
        }
    };

    struct ByteSpanCaseInsensitive {
        bool operator()(const ByteSpan& a, const ByteSpan& b) const noexcept {
            if (a.size() != b.size())
                return false;

            for (size_t i = 0; i < a.size(); ++i) {
                //if (TOLOWER(a[i]) != TOLOWER(b[i]))  // Case-insensitive comparison
                if (std::tolower(a[i]) != std::tolower(b[i]))  // Case-insensitive comparison

                    return false;
            }

            return true;
        }
    };
}


// Functions that are implemented here

namespace waavs
{
    static INLINE size_t copy_to_cstr(char* str, size_t len, const ByteSpan& a) noexcept
    {
        size_t maxBytes = a.size() < len ? a.size() : len;
        memcpy(str, a.data(), maxBytes);
        str[maxBytes] = 0;

        return maxBytes;
    }

    // Trim the left side of skippable characters
    static INLINE ByteSpan chunk_ltrim(const ByteSpan& a, const charset& skippable) noexcept
    {
        const uint8_t* startAt = a.begin();
        const uint8_t* endAt = a.end();
        while (startAt < endAt && skippable(*startAt))
            ++startAt;

        return ByteSpan::fromPointers(startAt, endAt);
    }

    // trim the right side of skippable characters
    static INLINE ByteSpan chunk_rtrim(const ByteSpan& a, const charset& skippable) noexcept
    {
        const uint8_t* start = a.begin();
        const uint8_t* end = a.end();
        while (start < end && skippable(*(end - 1)))
            --end;

        return ByteSpan::fromPointers(start, end);
    }

    // trim the left and right side of skippable characters
    static INLINE ByteSpan chunk_trim(const ByteSpan& a, const charset& skippable) noexcept
    {
        const uint8_t* start = a.begin();
        const uint8_t* end = a.end();

        // trim from the beginning
        while (start < end && skippable(*start))
            ++start;

        // trim from the end
        while (start < end && skippable(*(end - 1)))
            --end;

        return ByteSpan::fromPointers(start, end);
    }


    static INLINE ByteSpan chunk_skip_until_cstr(const ByteSpan& inChunk, const char* str) noexcept
    {
        const uint8_t* start = inChunk.begin();
        const uint8_t* end = inChunk.end();
        size_t len = std::strlen(str);

        while (start < end - len + 1)  // Ensure we don't read past the buffer
        {
            if (std::memcmp(start, str, len) == 0)
                return ByteSpan::fromPointers(start, end);  // Return position at match

            ++start;
        }

        return ByteSpan::null();  // No match found, return empty
    }

    static INLINE ByteSpan chunk_skip_until_chunk(const ByteSpan& inChunk, const ByteSpan& match) noexcept
    {
        const uint8_t* start = inChunk.begin();
        const uint8_t* end = inChunk.end();
        size_t len = match.size();

        if (len == 0 || start == end)
            return ByteSpan::null();  // Empty input or match, return empty

        while (start < end - len + 1)
        {
            if (std::memcmp(start, match.data(), len) == 0)
                return ByteSpan::fromPointers(start, end);  // Return position at match

            ++start;
        }

        return ByteSpan::fromPointers(end, end);  // No match found, return empty
    }



    // chuk_token_char()
    // 
    // Given a source chunk, and a single character delimeter
    // split the source into two chunks, the token, and the rest
    // The 'token', represents the portion of the source before the delim
    // The 'rest', represents the portion of the source after the delim
    // 
    // Returns - false if the delimeter is not found

    static INLINE bool chunk_token_char(const ByteSpan& src, const uint8_t delim, ByteSpan& tok, ByteSpan& rest)
    {
        const unsigned char * startAt = src.begin();
        const unsigned char * endAt = src.end();

        // Start with token being entire source input
        tok = src;

        // Try to find the delimeter
        const uint8_t* tokenEnd = static_cast<const uint8_t*>(std::memchr(startAt, delim, endAt - startAt));

        if (!tokenEnd) {
            // No delimiter found, return entire input
            //tok.fStart = src.begin();
            //tok.fEnd = src.end();

            // mark the 'rest' as being blank
            rest.resetPointers(endAt, endAt);

            return false;
        }
        else {
            // truncate the token to match where the 
            // delimeter was found
            tok.resetEnd(tokenEnd);

            // Advance past the delimiter
            rest.resetPointers(tokenEnd + 1, endAt);
        }

        return true;
    }

    // Given an input chunk
    // split it into two chunks, 
    // Returns - the first chunk before delimeter
    //	a - adjusted to reflect the rest of the input after delims
    // If delimeter NOT found
    //	returns the entire input chunk
    //	and 'a' is set to an empty chunk
    static INLINE ByteSpan chunk_token_char(ByteSpan& a, const char delim) noexcept
    {
        if (!a) return {}; // Return empty ByteSpan if 'a' is empty

        const uint8_t* start = a.begin();
        const uint8_t* end = a.end();

        // Use std::memchr() to find the delimiter efficiently
        const uint8_t* tokenEnd = static_cast<const uint8_t*>(std::memchr(start, delim, end - start));

        if (tokenEnd) {
            a.resetStart(tokenEnd + 1);  // Advance past the delimiter
        }
        else {
            tokenEnd = end;  // No delimiter found, return entire input
            a.resetStart(end);  // Move 'a' to empty state
        }

        return ByteSpan::fromPointers(start, tokenEnd);
    }


    static INLINE ByteSpan chunk_token(ByteSpan& a, const charset& delims) noexcept
    {
        if (!a) {
            a = {};
            return {};
        }

        const uint8_t* start = a.begin();
        const uint8_t* end = a.end();
        const uint8_t* tokenEnd = start;

        // skip forward until we see a delimiting character
        while (tokenEnd < end && !delims(*tokenEnd))
            ++tokenEnd;

        // If we stopped because we saw a delimiting character
        // advance past that.
        if (tokenEnd < end && delims(*tokenEnd))
        {
            a.resetStart(tokenEnd + 1);
        }
        else {
            a.resetStart(tokenEnd);
        }

        return ByteSpan::fromPointers(start, tokenEnd);
    }

    // name alias
    static INLINE ByteSpan nextToken(ByteSpan& a, const charset&& delims) noexcept
    {
        return chunk_token(a, delims);
    }



    // Given an input chunk
    // find the first instance of a specified character
    // Return 
    //	If character found, return chunk beginning where the character was found
    //  If character not found, return empty chunk
    static INLINE ByteSpan chunk_find_char(const ByteSpan& a, char c) noexcept
    {
        if (!a) return {}; // Return empty chunk if input is empty

        const uint8_t* start = a.begin();
        const uint8_t* end = a.end();

        // Use std::memchr() to locate the first occurrence of 'c'
        const uint8_t* found = static_cast<const uint8_t*>(std::memchr(start, c, end - start));

        if (!found) return {}; // Character not found, return empty chunk

        return ByteSpan::fromPointers(found, end);
    }


    // 
    // chunk_find()
    // 
    // Scan a ByteSpan 'src', looking for the search span 'str'
    // return true if it's found, and set the 'value' ByteSpan 
    // to the location.
    static INLINE bool chunk_find(const ByteSpan& src, const ByteSpan& str, ByteSpan& value) noexcept
    {
        if (src.size() < str.size() || str.empty())
            return false;

        const uint8_t* srcStart = src.begin();
        const uint8_t* srcEnd = src.end() - (str.size() - 1); // Avoid over-scanning
        const uint8_t* pattern = str.begin();
        const size_t patternSize = str.size();

        while (srcStart < srcEnd)
        {
            // Fast forward to the first occurrence of the first character of `str`
            srcStart = static_cast<const uint8_t*>(std::memchr(srcStart, *pattern, srcEnd - srcStart));
            if (!srcStart) return false; // Not found

            // Check if the rest of the `str` matches
            if (std::memcmp(srcStart, pattern, patternSize) == 0)
            {
                value = ByteSpan::fromPointers(srcStart, srcStart + patternSize);
                return true;
            }

            ++srcStart; // Move to the next position
        }

        return false;
    }


    static INLINE ByteSpan chunk_find_cstr(const ByteSpan& a, const char* c) noexcept
    {
        if (!a || !c || *c == '\0')
            return {};  // Return empty ByteSpan if input is invalid

        const uint8_t* start = a.begin();
        const uint8_t* end = a.end();
        const size_t clen = std::strlen(c);

        if (clen > static_cast<size_t>(end - start))
            return {};  // If search string is larger than input, it can't be found

        const uint8_t firstChar = static_cast<uint8_t>(c[0]);

        while (start < end)
        {
            // Use memchr() to quickly find the first character
            start = static_cast<const uint8_t*>(std::memchr(start, firstChar, end - start));
            if (!start || start + clen > end)
                return {};  // If first char is not found or rest of string doesn't fit, return empty

            // Check the rest of the string using memcmp()
            if (std::memcmp(start, c, clen) == 0)
                return ByteSpan::fromPointers(start, end);

            ++start;  // Move to next potential match
        }

        return {};
    }




    static INLINE ByteSpan chunk_read_bracketed(ByteSpan& src, const uint8_t lbracket, const uint8_t rbracket) noexcept
    {
        // Skip leading whitespace
        src = chunk_ltrim(src, chrWspChars);

        if (!src || *src != lbracket)
            return {};  // No valid opening bracket found

        // Advance past the opening bracket
        src++;
        const uint8_t* beginAttrValue = src.begin();

        // Use std::memchr() to find the closing rbracket
        const uint8_t* endAttrValue = static_cast<const uint8_t*>(std::memchr(beginAttrValue, rbracket, src.end() - beginAttrValue));

        if (!endAttrValue)
            return {};  // No closing bracket found

        // Advance `src` past the closing bracket
        src.resetStart(endAttrValue + 1);

        return ByteSpan::fromPointers(beginAttrValue, endAttrValue);
    }


    //
    // chunk_read_quoted()
    // 
    // Read a quoted string from the input stream
    // Read a first quote, then use that as the delimiter
    // to read to the end of the string
    //
    static INLINE bool chunk_read_quoted(ByteSpan& src, ByteSpan& dataChunk) noexcept
    {
        // Skip leading whitespace
        src = chunk_ltrim(src, chrWspChars);

        if (!src)
            return false;

        // Capture the opening quote character
        const uint8_t quote = *src;

        // Move past the opening quote
        src++;
        const uint8_t* beginAttrValue = src.begin();

        // Use std::memchr() to find the matching closing quote
        const uint8_t* endAttrValue = static_cast<const uint8_t*>(std::memchr(beginAttrValue, quote, src.end() - beginAttrValue));

        if (!endAttrValue)
            return false;  // No closing quote found

        // Assign the extracted span
        dataChunk = ByteSpan::fromPointers(beginAttrValue, endAttrValue);

        // Advance `src` past the closing quote
        src.resetStart(endAttrValue + 1);

        return true;
    }

}


namespace waavs
{
    // isAll()
    // Check if all characters in the span are in the specified charset.
    // This is typically used when you're trying to determine if the 
    // whole span is whitespace.
    static INLINE bool bspan_is_all(const ByteSpan& src, const charset& aset)
    {
        auto found = aset.skipWhile(src.begin(), src.end());
        return found == src.end();
    }

    // Check if all characters in the span are 
    // whitespace (space, tab, newline, carriage return)
    static INLINE bool is_all_whitespace(const ByteSpan& s) noexcept
    {
        const uint8_t* p = s.begin();
        const uint8_t* end = s.end();

#if WAAVS_HAS_NEON
        while ((end - p) >= 16)
        {
            if (!neon_span_is_all_xml_wsp_16(p))
                return false;

            p += 16;
        }
#endif

        while (p < end)
        {
            if (!chrWspChars(*p))
                return false;
            ++p;
        }

        return true;
    }
}


namespace waavs {

    static INLINE void writeChunk(const ByteSpan& chunk) noexcept
    {
        ByteSpan s = chunk;

        while (s && *s) {
            printf("%c", *s);
            s++;
        }
    }

    static INLINE void writeChunkBordered(const ByteSpan& chunk) noexcept
    {
        ByteSpan s = chunk;

        printf("||");
        while (s && *s) {
            printf("%c", *s);
            s++;
        }
        printf("||");
    }

    static INLINE void printChunk(const ByteSpan& chunk) noexcept
    {
        if (chunk)
        {
            writeChunk(chunk);
            printf("\n");
        }
        else
            printf("BLANK==CHUNK\n");

    }
}

