// unicode_bidi_class.h

#pragma once

#include <cstdint>
#include <type_traits>


namespace waavs
{
    // ========================================================================
    // UnicodeBidiClass
    //
    // Unicode Bidi_Class property values.
    //
    // These numeric values are part of our persistent Unicode database
    // representation and must remain stable once databases using them are
    // emitted.
    //
    // LeftToRight is zero because it is the broad Unicode default and gives
    // VALUE8 a useful zero-initialized representation.
    // ========================================================================

    enum class UnicodeBidiClass : uint8_t
    {
        // Strong types

        LeftToRight = 0,             // L
        RightToLeft = 1,             // R
        ArabicLetter = 2,            // AL

        // Weak types

        EuropeanNumber = 3,          // EN
        EuropeanSeparator = 4,       // ES
        EuropeanTerminator = 5,      // ET
        ArabicNumber = 6,            // AN
        CommonSeparator = 7,         // CS
        NonspacingMark = 8,          // NSM
        BoundaryNeutral = 9,         // BN

        // Neutral types

        ParagraphSeparator = 10,     // B
        SegmentSeparator = 11,       // S
        WhiteSpace = 12,             // WS
        OtherNeutral = 13,           // ON

        // Explicit formatting types

        LeftToRightEmbedding = 14,   // LRE
        LeftToRightOverride = 15,    // LRO
        RightToLeftEmbedding = 16,   // RLE
        RightToLeftOverride = 17,    // RLO
        PopDirectionalFormat = 18,   // PDF
        LeftToRightIsolate = 19,     // LRI
        RightToLeftIsolate = 20,     // RLI
        FirstStrongIsolate = 21,     // FSI
        PopDirectionalIsolate = 22   // PDI
    };


    static constexpr uint8_t kUnicodeBidiClassCount = 23;


    [[nodiscard]]
    static constexpr bool unicodeBidiClassIsValid(uint8_t value) noexcept {
        return value < kUnicodeBidiClassCount;
    }


    [[nodiscard]]
    static constexpr bool unicodeBidiClassIsValid(UnicodeBidiClass value) noexcept {
        return unicodeBidiClassIsValid(static_cast<uint8_t>(value));
    }


    static_assert(sizeof(UnicodeBidiClass) == 1,
        "UnicodeBidiClass must be exactly one byte");

    static_assert(std::is_same<
        std::underlying_type<UnicodeBidiClass>::type,
        uint8_t>::value,
        "UnicodeBidiClass must use uint8_t storage");

    static_assert(static_cast<uint8_t>(
        UnicodeBidiClass::LeftToRight) == 0,
        "Unicode Bidi Class L must remain value zero");

    static_assert(static_cast<uint8_t>(
        UnicodeBidiClass::PopDirectionalIsolate) + 1u ==
        kUnicodeBidiClassCount,
        "Unicode Bidi Class count is inconsistent");

} // namespace waavs
