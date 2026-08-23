#pragma once

#include "core_bytestream.h"
#include "lang_span.h"

namespace waavs
{
    /*
    // ========================================================================
         // Condition-based Reading
         // ========================================================================

    inline bool readUntil(uint8_t delim, ByteSpan& out) noexcept {
        const uint8_t* start = mData.begin();
        const uint8_t* end = mData.end();

        const uint8_t* found = static_cast<const uint8_t*>(
            std::memchr(start, delim, end - start));

        if (!found) {
            out = mData;
            mData.advanceToEnd();
            return !out.empty();
        }

        out = ByteSpan::fromPointers(start, found);
        mData.resetStart(found + 1);
        return true;
    }

    inline bool readUntil(const charset& delims, ByteSpan& out) noexcept {
        const uint8_t* start = mData.begin();
        const uint8_t* end = mData.end();
        const uint8_t* found = delims.advanceUntil(start, end);

        out = ByteSpan::fromPointers(start, found);
        if (found < end) {
            mData.resetStart(found + 1);
        }
        else {
            mData.resetStart(found);
        }
        return !out.empty();
    }

    inline bool readWhile(const charset& allowed, ByteSpan& out) noexcept {
        const uint8_t* start = mData.begin();
        const uint8_t* end = mData.end();
        const uint8_t* found = allowed.advanceWhile(start, end);

        out = ByteSpan::fromPointers(start, found);
        mData.resetStart(found);
        return !out.empty();
    }
    */

    /*
inline bool readCodepoint(uint32_t& codepoint) noexcept {
    if (mData.empty()) return false;

    uint32_t state = UTF8_ACCEPT;
    uint32_t cp = 0;
    const uint8_t* start = mData.begin();
    const uint8_t* end = mData.end();
    const uint8_t* p = start;

    while (p < end) {
        uint32_t newState = decode(&state, &cp, *p);
        ++p;

        if (newState == UTF8_ACCEPT) {
            codepoint = cp;
            size_t bytesRead = static_cast<size_t>(p - start);
            mData.advance(bytesRead);
            return true;
        }

        if (newState == UTF8_REJECT) {
            return false;
        }
    }

    return false;
}

inline bool skipCodepoints(size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        uint32_t codepoint;
        if (!readCodepoint(codepoint)) {
            return false;
        }
    }
    return true;
}

inline size_t countCodepoints() const noexcept {
    ByteSpan temp = mData;
    size_t count = 0;

    while (!temp.empty()) {
        uint32_t state = UTF8_ACCEPT;
        uint32_t cp = 0;
        const uint8_t* p = temp.begin();
        const uint8_t* end = temp.end();

        while (p < end) {
            uint32_t newState = decode(&state, &cp, *p);
            ++p;

            if (newState == UTF8_ACCEPT) {
                ++count;
                temp.resetStart(p);
                break;
            }

            if (newState == UTF8_REJECT) {
                return count;
            }
        }

        if (p == end) break;
    }

    return count;
}
*/
}
