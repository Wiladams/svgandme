#pragma once

#include "ocspan.h"
#include "charset.h"

namespace waavs {

    // Left trim - Remove leading characters found in charset
    static OcSpan ocspan_ltrim(const OcSpan& span, const waavs::charset& cs) {
        std::size_t start = 0;
        while (start < span.size() && cs.contains(span[start])) {
            ++start;
        }
        return OcSpan(span.data() + start, span.size() - start);
    }

    // Right trim - Remove trailing characters found in charset
    static OcSpan ocspan_rtrim(const OcSpan& span, const waavs::charset& cs) {
        std::size_t end = span.size();
        while (end > 0 && cs.contains(span[end - 1])) {
            --end;
        }
        return OcSpan(span.data(), end);
    }

    // Full trim - Remove characters from both ends
    static OcSpan ocspan_trim(const OcSpan& span, const waavs::charset& cs) {
        return ocspan_rtrim(ocspan_ltrim(span, cs), cs);
    }

    // trimQuotes()
    // Remove the first, and last characters, if they are the same
    static OcSpan ocspan_trimQuotes(const OcSpan& src)
    {
        if (src.size() < 2) {
            return src;  // Not enough characters to be quoted
        }

        const uint8_t* start = src.data();
        const uint8_t* end = src.data() + src.size() - 1;

        // Check if the first and last characters are the same
        if (*start == *end) {
            return OcSpan(start + 1, src.size() - 2);  // Trim both ends
        }

        return src;  // No matching pair, return the original span
    }

    static OcSpan ocspan_frontToken(const OcSpan& src, const uint8_t delim, OcSpan& rest) {
        const uint8_t* start = src.data();
        const uint8_t* end = src.data() + src.size();

        // Find the delimiter
        const uint8_t* pos = start;
        while (pos < end && *pos != delim) {
            ++pos;
        }

        if (pos == end) {
            // Delimiter not found, return full src, rest is empty
            rest = OcSpan();
            return src;
        }
        else {
            // Found the delimiter, split into token and rest
            rest = OcSpan(pos + 1, end - (pos + 1));  // Rest starts after the delimiter
            return OcSpan(start, pos - start);       // Token from start to delimiter
        }
    }

}
