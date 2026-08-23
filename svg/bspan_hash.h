#pragma once

#include "lang_span.h"
#include "core_hash.h"

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
    // Structures for using ByteSpan as keys in unordered maps, 
    // with proper hashing and equality checks
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
