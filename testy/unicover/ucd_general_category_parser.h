
// ucd_general_category_parser.h

#pragma once

#include <cstddef>
#include <cstdint>

#include "ucd_parser.h"
#include "unicode_coverage_builder.h"
#include "unicode_general_category.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    // ========================================================================
    // UCDGeneralCategoryParseError
    // ========================================================================

    enum class UCDGeneralCategoryParseError : uint8_t
    {
        None = 0,

        InvalidRange,
        MissingCategory,
        UnexpectedField,
        UnknownCategory,
        OverlappingRange,
        IncompleteCoverage
    };


    // ========================================================================
    // UCDGeneralCategoryParseResult
    //
    // lineNumber:
    //
    //      Physical source line on which an error occurred.
    //      Zero on successful completion.
    //
    // rangeCount:
    //
    //      Number of General_Category ranges parsed.
    //
    // assignedCodePoints:
    //
    //      Number of code points explicitly assigned by the input.
    //
    // ========================================================================

    struct UCDGeneralCategoryParseResult
    {
        UCDGeneralCategoryParseError error{ UCDGeneralCategoryParseError::None };

        uint32_t lineNumber{ 0 };
        uint32_t rangeCount{ 0 };

        size_t assignedCodePoints{ 0 };


        [[nodiscard]]
        bool success() const noexcept
        {
            return error == UCDGeneralCategoryParseError::None;
        }


        explicit operator bool() const noexcept
        {
            return success();
        }
    };


    // ========================================================================
    // ucdGeneralCategoryParseErrorString
    // ========================================================================

    static inline const char* ucdGeneralCategoryParseErrorString(
        UCDGeneralCategoryParseError error) noexcept
    {
        switch (error)
        {
        case UCDGeneralCategoryParseError::None:
            return "no error";

        case UCDGeneralCategoryParseError::InvalidRange:
            return "invalid General_Category code-point range";

        case UCDGeneralCategoryParseError::MissingCategory:
            return "missing General_Category value";

        case UCDGeneralCategoryParseError::UnexpectedField:
            return "unexpected extra field in General_Category record";

        case UCDGeneralCategoryParseError::UnknownCategory:
            return "unknown General_Category value";

        case UCDGeneralCategoryParseError::OverlappingRange:
            return "General_Category ranges overlap";

        case UCDGeneralCategoryParseError::IncompleteCoverage:
            return "General_Category data does not cover the complete Unicode address space";
        }

        return "unknown General_Category parser error";
    }


    namespace ucd_general_category_detail
    {
        // ====================================================================
        // categoryFromField
        //
        // Convert the canonical two-letter UCD General_Category abbreviation
        // into the stable runtime/database enum.
        //
        // DerivedGeneralCategory.txt uses:
        //
        //      Cc Cf Cn Co Cs
        //      Ll Lm Lo Lt Lu
        //      Mc Me Mn
        //      Nd Nl No
        //      Pc Pd Pe Pf Pi Po Ps
        //      Sc Sk Sm So
        //      Zl Zp Zs
        //
        // The parser deliberately accepts only these canonical abbreviations.
        // ====================================================================

        static inline bool categoryFromField(const ByteSpan& field,
            UnicodeGeneralCategory& outCategory) noexcept
        {
            if (field.size() != 2)
                return false;


            const uint8_t a = field.data()[0];
            const uint8_t b = field.data()[1];


            switch (a)
            {
                // ---------------------------------------------------------------
                // Other
                // ---------------------------------------------------------------

            case 'C':
                switch (b)
                {
                case 'c':
                    outCategory = UnicodeGeneralCategory::Control;
                    return true;

                case 'f':
                    outCategory = UnicodeGeneralCategory::Format;
                    return true;

                case 'n':
                    outCategory = UnicodeGeneralCategory::Unassigned;
                    return true;

                case 'o':
                    outCategory = UnicodeGeneralCategory::PrivateUse;
                    return true;

                case 's':
                    outCategory = UnicodeGeneralCategory::Surrogate;
                    return true;
                }

                break;


                // ---------------------------------------------------------------
                // Letter
                // ---------------------------------------------------------------

            case 'L':
                switch (b)
                {
                case 'u':
                    outCategory = UnicodeGeneralCategory::UppercaseLetter;
                    return true;

                case 'l':
                    outCategory = UnicodeGeneralCategory::LowercaseLetter;
                    return true;

                case 't':
                    outCategory = UnicodeGeneralCategory::TitlecaseLetter;
                    return true;

                case 'm':
                    outCategory = UnicodeGeneralCategory::ModifierLetter;
                    return true;

                case 'o':
                    outCategory = UnicodeGeneralCategory::OtherLetter;
                    return true;
                }

                break;


                // ---------------------------------------------------------------
                // Mark
                // ---------------------------------------------------------------

            case 'M':
                switch (b)
                {
                case 'n':
                    outCategory = UnicodeGeneralCategory::NonspacingMark;
                    return true;

                case 'c':
                    outCategory = UnicodeGeneralCategory::SpacingMark;
                    return true;

                case 'e':
                    outCategory = UnicodeGeneralCategory::EnclosingMark;
                    return true;
                }

                break;


                // ---------------------------------------------------------------
                // Number
                // ---------------------------------------------------------------

            case 'N':
                switch (b)
                {
                case 'd':
                    outCategory = UnicodeGeneralCategory::DecimalNumber;
                    return true;

                case 'l':
                    outCategory = UnicodeGeneralCategory::LetterNumber;
                    return true;

                case 'o':
                    outCategory = UnicodeGeneralCategory::OtherNumber;
                    return true;
                }

                break;


                // ---------------------------------------------------------------
                // Punctuation
                // ---------------------------------------------------------------

            case 'P':
                switch (b)
                {
                case 'c':
                    outCategory = UnicodeGeneralCategory::ConnectorPunctuation;
                    return true;

                case 'd':
                    outCategory = UnicodeGeneralCategory::DashPunctuation;
                    return true;

                case 's':
                    outCategory = UnicodeGeneralCategory::OpenPunctuation;
                    return true;

                case 'e':
                    outCategory = UnicodeGeneralCategory::ClosePunctuation;
                    return true;

                case 'i':
                    outCategory = UnicodeGeneralCategory::InitialPunctuation;
                    return true;

                case 'f':
                    outCategory = UnicodeGeneralCategory::FinalPunctuation;
                    return true;

                case 'o':
                    outCategory = UnicodeGeneralCategory::OtherPunctuation;
                    return true;
                }

                break;


                // ---------------------------------------------------------------
                // Symbol
                // ---------------------------------------------------------------

            case 'S':
                switch (b)
                {
                case 'm':
                    outCategory = UnicodeGeneralCategory::MathSymbol;
                    return true;

                case 'c':
                    outCategory = UnicodeGeneralCategory::CurrencySymbol;
                    return true;

                case 'k':
                    outCategory = UnicodeGeneralCategory::ModifierSymbol;
                    return true;

                case 'o':
                    outCategory = UnicodeGeneralCategory::OtherSymbol;
                    return true;
                }

                break;


                // ---------------------------------------------------------------
                // Separator
                // ---------------------------------------------------------------

            case 'Z':
                switch (b)
                {
                case 's':
                    outCategory = UnicodeGeneralCategory::SpaceSeparator;
                    return true;

                case 'l':
                    outCategory = UnicodeGeneralCategory::LineSeparator;
                    return true;

                case 'p':
                    outCategory = UnicodeGeneralCategory::ParagraphSeparator;
                    return true;
                }

                break;
            }


            return false;
        }


        // ====================================================================
        // coverageIntersectsRange
        //
        // Generator-side validation helper.
        //
        // General_Category is single-valued, so no code point may appear in
        // more than one explicit range.
        //
        // Because successful input is non-overlapping, the total number of
        // code points inspected across all calls is roughly the size of the
        // Unicode address space rather than rangeCount * kUnicodeLimit.
        // ====================================================================

        static inline bool coverageIntersectsRange(
            const UnicodeCoverageBuilder& coverage,
            uint32_t first, uint32_t last) noexcept
        {
            uint32_t cp = first;

            for (;;)
            {
                if (coverage.contains(cp))
                    return true;

                if (cp == last)
                    break;

                ++cp;
            }

            return false;
        }
    }


    // ========================================================================
    // ucdParseGeneralCategory
    //
    // Parse DerivedGeneralCategory.txt into a mutable VALUE8 table.
    //
    // Expected meaningful line syntax:
    //
    //      range ; general-category
    //
    // Examples:
    //
    //      0000..001F ; Cc
    //      0041       ; Lu
    //      0061       ; Ll
    //      0300..036F ; Mn
    //
    //
    // Parsing:
    //
    //      DerivedGeneralCategory.txt
    //              |
    //              v
    //      range + category abbreviation
    //              |
    //              v
    //      UnicodeGeneralCategory
    //              |
    //              v
    //      UnicodeValueTable8Builder
    //
    //
    // The builder is initialized to General_Category=Unassigned before
    // parsing.
    //
    // Every explicit range is also accumulated into assignedCoverage so that
    // overlapping input can be rejected.
    //
    // The extracted General_Category data is expected to account for the
    // complete Unicode code-point address space.  After overlap validation,
    // therefore:
    //
    //      assignedCodePoints == kUnicodeLimit
    //
    // establishes complete coverage.
    //
    //
    // On failure 'values' may contain assignments produced before the error.
    // Generation should therefore be considered failed and the builder
    // discarded or cleared.
    //
    // ========================================================================

    static inline bool ucdParseGeneralCategory(const ByteSpan& source,
        UnicodeValueTable8Builder& values,
        UCDGeneralCategoryParseResult& outResult)
    {
        outResult = {};


        // --------------------------------------------------------------------
        // General_Category default.
        //
        // UnicodeGeneralCategory::Unassigned is deliberately value zero.
        // --------------------------------------------------------------------

        values.clear(
            static_cast<uint8_t>(
                UnicodeGeneralCategory::Unassigned));


        // --------------------------------------------------------------------
        // Tracks every explicitly assigned code point.
        // --------------------------------------------------------------------

        UnicodeCoverageBuilder assignedCoverage;


        UCDParser parser(source);
        UCDLine line;


        while (parser.next(line))
        {
            ByteSpan fields =
                line.data;


            // ----------------------------------------------------------------
            // Field 1: code-point range
            // ----------------------------------------------------------------

            UCDCodePointRange range;


            if (!ucdReadCodePointRange(fields, range))
            {
                outResult.error =
                    UCDGeneralCategoryParseError::InvalidRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Field 2: General_Category value
            // ----------------------------------------------------------------

            ByteSpan categoryField;


            if (!ucdReadField(fields, categoryField) ||
                !categoryField)
            {
                outResult.error =
                    UCDGeneralCategoryParseError::MissingCategory;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // DerivedGeneralCategory.txt records contain exactly two fields.
            // ----------------------------------------------------------------

            bspan_trim_spaces(fields);


            if (fields)
            {
                outResult.error =
                    UCDGeneralCategoryParseError::UnexpectedField;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Convert the canonical UCD abbreviation to our stable enum.
            // ----------------------------------------------------------------

            UnicodeGeneralCategory category;


            if (!ucd_general_category_detail::categoryFromField(
                categoryField,
                category))
            {
                outResult.error =
                    UCDGeneralCategoryParseError::UnknownCategory;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // General_Category is single-valued.
            //
            // No code point may receive more than one explicit assignment.
            // The extracted file need not be globally ordered, so overlap
            // validation cannot rely on previousLast.
            // ----------------------------------------------------------------

            if (ucd_general_category_detail::coverageIntersectsRange(
                assignedCoverage,
                range.first,
                range.last))
            {
                outResult.error =
                    UCDGeneralCategoryParseError::OverlappingRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Store the property value and record explicit assignment.
            // ----------------------------------------------------------------

            values.setRange(
                range.first,
                range.last,
                static_cast<uint8_t>(category));


            assignedCoverage.addRange(
                range.first,
                range.last);


            ++outResult.rangeCount;


            outResult.assignedCodePoints +=
                static_cast<size_t>(
                    range.last -
                    range.first +
                    1u);
        }


        // ====================================================================
        // Complete coverage validation.
        //
        // Because:
        //
        //      - every parsed range is within Unicode
        //      - overlapping ranges have been rejected
        //
        // an explicit cardinality equal to kUnicodeLimit means every Unicode
        // code point has exactly one General_Category assignment.
        // ====================================================================

        if (outResult.assignedCodePoints !=
            static_cast<size_t>(kUnicodeLimit))
        {
            outResult.error =
                UCDGeneralCategoryParseError::IncompleteCoverage;

            return false;
        }


        return true;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static inline bool ucdParseGeneralCategory(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values)
    {
        UCDGeneralCategoryParseResult result;

        return
            ucdParseGeneralCategory(
                source,
                values,
                result);
    }

} // namespace waavs
