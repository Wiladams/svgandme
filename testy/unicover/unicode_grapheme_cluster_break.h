
// unicode_grapheme_cluster_break.h

#pragma once

#include <cstdint>
#include <type_traits>


namespace waavs
{
    enum class UnicodeGraphemeClusterBreak : uint8_t
    {
        Other = 0,                  // XX

        CR = 1,                     // CR
        LF = 2,                     // LF
        Control = 3,                // CN

        Extend = 4,                 // EX
        ZWJ = 5,                    // ZWJ
        RegionalIndicator = 6,      // RI
        Prepend = 7,                // PP
        SpacingMark = 8,            // SM

        L = 9,                      // L
        V = 10,                     // V
        T = 11,                     // T
        LV = 12,                    // LV
        LVT = 13                    // LVT
    };


    static constexpr uint8_t kUnicodeGraphemeClusterBreakCount = 14;


    [[nodiscard]]
    static constexpr bool unicodeGraphemeClusterBreakIsValid(uint8_t value) noexcept {
        return value < kUnicodeGraphemeClusterBreakCount;
    }


    [[nodiscard]]
    static constexpr bool unicodeGraphemeClusterBreakIsValid(UnicodeGraphemeClusterBreak value) noexcept
    {
        return unicodeGraphemeClusterBreakIsValid(static_cast<uint8_t>(value));
    }


    static_assert(
        sizeof(UnicodeGraphemeClusterBreak) == 1,
        "UnicodeGraphemeClusterBreak must be exactly one byte");

    static_assert(
        std::is_same<
        std::underlying_type<UnicodeGraphemeClusterBreak>::type,
        uint8_t>::value,
        "UnicodeGraphemeClusterBreak must use uint8_t storage");

    static_assert(
        static_cast<uint8_t>(
            UnicodeGraphemeClusterBreak::Other) == 0,
        "Unicode Grapheme_Cluster_Break Other must remain value zero");

    static_assert(
        static_cast<uint8_t>(
            UnicodeGraphemeClusterBreak::LVT) + 1u ==
        kUnicodeGraphemeClusterBreakCount,
        "Unicode Grapheme_Cluster_Break count is inconsistent");

} // namespace waavs
