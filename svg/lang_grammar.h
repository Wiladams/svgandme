// bspan_utils.h

#pragma once

#include <cstring>  // for std::memchr

#include "scanning.h"
#include "lang_span.h"
#include "lang_charset.h"


namespace waavs
{

    static INLINE bool bspan_starts_with(const ByteSpan &a, const ByteSpan& b) noexcept
    {
        return (a.take(b.size()) == b);
    }

    static INLINE bool bspan_ends_with(const ByteSpan& a, const ByteSpan& b) noexcept
    {
        if (b.size() > a.size())
            return false;

        return a.subSpan(a.size() - b.size(), b.size()) == b;
    }



    static INLINE ByteSpan& bspan_ltrim(ByteSpan& src, const charset& skippable) noexcept
    {
        const uint8_t* start = skippable.advanceWhile(src.begin(), src.end());
        src.resetStart(start);
        return src;
    }

    // same as bspan_ltrim, with an implied whitespace set
    static INLINE ByteSpan bspan_skip_spaces(ByteSpan& a) noexcept
    {
        const uint8_t* start = find_first_not_of4(a.begin(), a.end(), ' ', '\t', '\r', '\n');
        a.resetStart(start);

        return a;
    }

    static INLINE ByteSpan& bspan_ltrim_spaces(ByteSpan& src)
    {
        return bspan_ltrim(src, chrWspChars);
    }

    static INLINE ByteSpan& bspan_rtrim(ByteSpan& src, const charset& skippable) noexcept
    {
        const uint8_t* start = src.begin();
        const uint8_t* end = src.end();

        while (start < end && skippable(*(end - 1)))
            --end;

        src.resetEnd(end);
        return src;
    }

    static INLINE ByteSpan& bspan_trim(ByteSpan& src, const charset& skippable) noexcept
    {
        bspan_ltrim(src, skippable);
        bspan_rtrim(src, skippable);
        return src;
    }

    // Trim whitespace from both ends
    static INLINE ByteSpan& bspan_trim_spaces(ByteSpan& src)
    {
        return bspan_trim(src, chrWspChars);
    }


    static  ByteSpan& bspan_trim_matching_quotes(ByteSpan& src) noexcept
    {
        if (src.size() < 2)
            return src;

        const uint8_t* start = src.begin();
        const uint8_t* end = src.end();

        const uint8_t first = *start;
        const uint8_t last = *(end - 1);

        if ((first == '\'' && last == '\'') ||
            (first == '"' && last == '"'))
        {
            src.resetPointers(start + 1, end - 1);
        }

        return src;
    }


    // bspan_is_all()
    // 
    // Check if all characters in the span are in the specified charset.
    // This is typically used when you're trying to determine if the 
    // whole span is whitespace.
    static INLINE bool bspan_is_all(const ByteSpan& src, const charset& aset)
    {
        auto found = aset.advanceWhile(src.begin(), src.end());
        return found == src.end();
    }


    // bspan_read_until
    //
    // This is similar to bspan_read_prefix, but has the opposite
    // effect.  We want the span of characters from the beginning
    // until we see one of the characters in the 'delims' set, or 
    // single character.
    // The return value is that front span
    // and 'src' is advanced past the span.
    static INLINE ByteSpan bspan_read_until(ByteSpan& src, const uint8_t delim) noexcept
    {
        if (!src)
            return {};

        const uint8_t* start = src.begin();
        const uint8_t* end = src.end();

        const uint8_t* p = static_cast<const uint8_t*>(
            std::memchr(start, delim, end - start));

        if (!p)
            p = end;

        src.resetStart(p < end ? p + 1 : p);

        return ByteSpan::fromPointers(start, p);
    }

    static INLINE ByteSpan bspan_read_until(ByteSpan& src, const charset& delims) noexcept
    {
        if (!src)
            return {};

        const uint8_t* start = src.begin();
        const uint8_t* end = src.end();

        const uint8_t* p = delims.advanceUntil(start, end);

        src.resetStart(p < end ? p + 1 : p);

        return ByteSpan::fromPointers(start, p);
    }


    // bspan_read_while()
    //
    // Read a span from the beginning of the source span, 
    // until we encounter a character that is not in the 'allowed' charset.
    // RETURN
    //  From the beginning of the source span, until the
    //  first character that is not in the 'allowed' charset.
    //  src - advances past the characters that were read.
    //
    static INLINE ByteSpan bspan_read_while(ByteSpan& src, const charset& allowed) noexcept
    {
        const uint8_t* start = src.begin();
        const uint8_t* end = src.end();

        const uint8_t* p = allowed.advanceWhile(start, end);
        src.resetStart(p);

        return ByteSpan::fromPointers(start, p);
    }



    // bspan_read_enclosed()
    //
    // Read a span that is enclosed by a pair of brackets, 
    // such as parentheses or square brackets.
    // The source span is advanced past the closing bracket.
    // out - the span that is enclosed by the brackets, 
    //      excluding the brackets themselves
    // lbracket - the opening bracket character
    // rbracket - the closing bracket character
    // 
    // Note: This will handle nested brackets, but if they are not
    // balanced, it will return 'false'.
    //
    static INLINE bool bspan_read_enclosed(
        ByteSpan& src,
        ByteSpan& out,
        const unsigned char lbracket,
        const unsigned char rbracket) noexcept
    {
        out.reset();

        if (!src || *src != lbracket)
            return false;

        const unsigned char* p = src.data() + 1;
        const unsigned char* const e = src.end();
        const unsigned char* const payloadStart = p;

        size_t depth = 1;

        while (p < e)
        {
            const unsigned char ch = *p++;

            if (ch == lbracket)
            {
                ++depth;
                continue;
            }

            if (ch == rbracket)
            {
                --depth;

                if (depth == 0)
                {
                    const unsigned char* const payloadEnd = p - 1;

                    out.resetPointers(payloadStart, payloadEnd);
                    src.resetStart(p);
                    return true;
                }
            }
        }

        return false;
    }

    static INLINE bool bspan_read_until_unquoted(
        ByteSpan& src, uint8_t delim,
        ByteSpan& out) noexcept
    {
        out.reset();

        const uint8_t* start = src.begin();
        uint8_t quote = 0;

        while (src)
        {
            uint8_t c = *src;
            ++src;

            if (quote)
            {
                if (c == quote)
                    quote = 0;
                continue;
            }

            if (c == '"' || c == '\'')
            {
                quote = c;
                continue;
            }

            if (c == delim)
            {
                out = ByteSpan::fromPointers(start, src.begin() - 1);
                return true;
            }
        }

        return false;
    }

    static INLINE bool bspan_read_quoted(ByteSpan& src, ByteSpan& out) noexcept
    {
        out.reset();

        //bspan_skip_spaces(src);

        if (!src)
            return false;

        const uint8_t quote = *src;

        if (quote != '"' && quote != '\'')
            return false;

        ++src;

        const uint8_t* start = src.begin();
        const uint8_t* end = src.end();

        const uint8_t* close =
            static_cast<const uint8_t*>(std::memchr(start, quote, end - start));

        if (!close)
            return false;

        out.resetPointers(start, close);
        src.resetStart(close + 1);

        return true;
    }

    // 
    // bspan_find_span()
    // 
    // Scan a ByteSpan 'src', looking for the search span 'str'
    // return true if it's found, and set the 'value' ByteSpan 
    // to the location.
    static INLINE bool bspan_find_span(
        const ByteSpan& src,
        const ByteSpan& needle,
        ByteSpan& outSpan) noexcept
    {
        outSpan.reset();

        if (!src || !needle || needle.size() > src.size())
            return false;

        const uint8_t* cur = src.begin();
        const uint8_t* last = src.end() - needle.size();

        const uint8_t first = *needle.begin();
        const size_t needleSize = needle.size();

        while (cur <= last)
        {
            cur = static_cast<const uint8_t*>(
                std::memchr(cur, first, static_cast<size_t>((last - cur) + 1)));

            if (!cur)
                return false;

            if (std::memcmp(cur, needle.data(), needleSize) == 0)
            {
                outSpan = ByteSpan::fromPointers(cur, cur + needleSize);
                return true;
            }

            ++cur;
        }

        return false;
    }

}



namespace waavs
{
    // is_all_whitespace()
    // 
    // Check if all characters in the span are 
    // whitespace (space, tab, newline, carriage return)
    // This is highly specialized to use SIMD instructions
    // to check 16 bytes at a time, and then fall back 
    // to scalar checking for the remaining characters.
    // Note: Probably not that useful in practice, as most
    // whitespace is much smaller than 16 bytes.
    //

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

