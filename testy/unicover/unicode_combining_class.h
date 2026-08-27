// unicode_combining_class.h

#pragma once

#include <cstdint>


namespace waavs
{
    // ========================================================================
    // UnicodeCombiningClass
    //
    // Canonical_Combining_Class is inherently a numeric Unicode property.
    //
    // Unlike General_Category, no private enum encoding is required. The
    // Unicode numeric value itself is stored directly in UnicodeValueTable8.
    //
    // Unicode defines Canonical_Combining_Class values in the range:
    //
    //      0 .. 254
    //
    // Value zero is:
    //
    //      Not_Reordered
    //
    // and is the default for every code point not explicitly assigned another
    // value.
    //
    // ========================================================================

    using UnicodeCombiningClass = uint8_t;


    static constexpr UnicodeCombiningClass kUnicodeCombiningClassNotReordered = 0;
    static constexpr uint32_t kUnicodeCombiningClassMaximum = 254u;


    // ========================================================================
    // unicodeCombiningClassIsValid
    // ========================================================================

    [[nodiscard]]
    static constexpr bool unicodeCombiningClassIsValid(uint32_t value) noexcept
    {
        return value <= kUnicodeCombiningClassMaximum;
    }


    // ========================================================================
    // ABI checks
    // ========================================================================

    static_assert(
        sizeof(UnicodeCombiningClass) == 1,
        "UnicodeCombiningClass must be exactly one byte");

    static_assert(
        kUnicodeCombiningClassNotReordered == 0,
        "Canonical_Combining_Class Not_Reordered must remain zero");

    static_assert(
        kUnicodeCombiningClassMaximum < 256,
        "Canonical_Combining_Class must fit in UnicodeValueTable8");

} // namespace waavs
