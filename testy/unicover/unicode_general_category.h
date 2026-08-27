// unicode_general_category.h

#pragma once

#include <cstdint>
#include <type_traits>


namespace waavs
{
    // ========================================================================
    // UnicodeGeneralCategory
    //
    // Unicode General_Category property values.
    //
    // These numeric values are part of our persistent Unicode database
    // representation.  They must therefore remain stable once databases
    // using them are emitted.
    //
    // The values are deliberately assigned explicitly rather than depending
    // on declaration order.
    //
    // Unassigned is zero because General_Category defaults naturally to Cn
    // for code points not otherwise assigned.
    //
    // ========================================================================

    enum class UnicodeGeneralCategory : uint8_t
    {
        // --------------------------------------------------------------------
        // Other
        // --------------------------------------------------------------------

        Unassigned = 0,   // Cn
        Control = 1,   // Cc
        Format = 2,   // Cf
        PrivateUse = 3,   // Co
        Surrogate = 4,   // Cs


        // --------------------------------------------------------------------
        // Letter
        // --------------------------------------------------------------------

        UppercaseLetter = 5,   // Lu
        LowercaseLetter = 6,   // Ll
        TitlecaseLetter = 7,   // Lt
        ModifierLetter = 8,   // Lm
        OtherLetter = 9,   // Lo


        // --------------------------------------------------------------------
        // Mark
        // --------------------------------------------------------------------

        NonspacingMark = 10,  // Mn
        SpacingMark = 11,  // Mc
        EnclosingMark = 12,  // Me


        // --------------------------------------------------------------------
        // Number
        // --------------------------------------------------------------------

        DecimalNumber = 13,  // Nd
        LetterNumber = 14,  // Nl
        OtherNumber = 15,  // No


        // --------------------------------------------------------------------
        // Punctuation
        // --------------------------------------------------------------------

        ConnectorPunctuation = 16, // Pc
        DashPunctuation = 17,  // Pd
        OpenPunctuation = 18,  // Ps
        ClosePunctuation = 19,  // Pe
        InitialPunctuation = 20,  // Pi
        FinalPunctuation = 21,  // Pf
        OtherPunctuation = 22,  // Po


        // --------------------------------------------------------------------
        // Symbol
        // --------------------------------------------------------------------

        MathSymbol = 23,  // Sm
        CurrencySymbol = 24,  // Sc
        ModifierSymbol = 25,  // Sk
        OtherSymbol = 26,  // So


        // --------------------------------------------------------------------
        // Separator
        // --------------------------------------------------------------------

        SpaceSeparator = 27,  // Zs
        LineSeparator = 28,  // Zl
        ParagraphSeparator = 29   // Zp
    };


    // ========================================================================
    // General Category information
    // ========================================================================

    static constexpr uint8_t kUnicodeGeneralCategoryCount = 30;


    [[nodiscard]]
    static constexpr bool unicodeGeneralCategoryIsValid(uint8_t value) noexcept
    {
        return value < kUnicodeGeneralCategoryCount;
    }


    [[nodiscard]]
    static constexpr bool unicodeGeneralCategoryIsValid(
        UnicodeGeneralCategory value) noexcept
    {
        return unicodeGeneralCategoryIsValid(
            static_cast<uint8_t>(value));
    }


    // ========================================================================
    // ABI checks
    // ========================================================================

    static_assert(
        sizeof(UnicodeGeneralCategory) == 1,
        "UnicodeGeneralCategory must be exactly one byte");


    static_assert(
        std::is_same<
        std::underlying_type<UnicodeGeneralCategory>::type,
        uint8_t>::value,
        "UnicodeGeneralCategory must use uint8_t storage");


    static_assert(
        static_cast<uint8_t>(
            UnicodeGeneralCategory::Unassigned) == 0,
        "Unicode General Category Cn must remain value zero");


    static_assert(
        static_cast<uint8_t>(
            UnicodeGeneralCategory::ParagraphSeparator) + 1u ==
        kUnicodeGeneralCategoryCount,
        "Unicode General Category count is inconsistent");

} // namespace waavs
