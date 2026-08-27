// ucd_grapheme_cluster_break_parser.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ucd_parser.h"
#include "unicode_coverage_builder.h"
#include "unicode_grapheme_cluster_break.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    // ========================================================================
    // UCDGraphemeClusterBreakParseError
    // ========================================================================

    enum class UCDGraphemeClusterBreakParseError : uint8_t
    {
        None = 0,

        InvalidRange,
        MissingBreakClass,
        UnexpectedField,
        UnknownBreakClass,
        OverlappingRange
    };


    // ========================================================================
    // UCDGraphemeClusterBreakParseResult
    // ========================================================================

    struct UCDGraphemeClusterBreakParseResult
    {
        UCDGraphemeClusterBreakParseError error{
            UCDGraphemeClusterBreakParseError::None
        };

        uint32_t lineNumber{ 0 };
        uint32_t rangeCount{ 0 };

        size_t explicitCodePoints{ 0 };
        size_t defaultedCodePoints{ 0 };


        [[nodiscard]]
        bool success() const noexcept {
            return error == UCDGraphemeClusterBreakParseError::None;
        }


        explicit operator bool() const noexcept {
            return success();
        }
    };


    // ========================================================================
    // Error text
    // ========================================================================

    static inline const char* ucdGraphemeClusterBreakParseErrorString(
        UCDGraphemeClusterBreakParseError error) noexcept
    {
        switch (error)
        {
        case UCDGraphemeClusterBreakParseError::None:
            return "no error";

        case UCDGraphemeClusterBreakParseError::InvalidRange:
            return "invalid Grapheme_Cluster_Break code-point range";

        case UCDGraphemeClusterBreakParseError::MissingBreakClass:
            return "missing Grapheme_Cluster_Break value";

        case UCDGraphemeClusterBreakParseError::UnexpectedField:
            return "unexpected extra field in Grapheme_Cluster_Break record";

        case UCDGraphemeClusterBreakParseError::UnknownBreakClass:
            return "unknown Grapheme_Cluster_Break value";

        case UCDGraphemeClusterBreakParseError::OverlappingRange:
            return "Grapheme_Cluster_Break ranges overlap";
        }

        return "unknown Grapheme_Cluster_Break parser error";
    }


    namespace ucd_grapheme_cluster_break_detail
    {
        // ====================================================================
        // spanEquals
        // ====================================================================

        static inline bool spanEquals(const ByteSpan& span,
            const char* text) noexcept
        {
            const size_t length =
                std::strlen(text);

            return span.size() == length &&
                std::memcmp(
                    span.data(),
                    text,
                    length) == 0;
        }


        // ====================================================================
        // breakClassFromField
        //
        // Accept both the preferred short and long aliases defined by
        // PropertyValueAliases.txt.
        //
        // Historical retained aliases such as E_Base, E_Modifier and
        // Glue_After_Zwj are deliberately not accepted because they are not
        // part of the current UnicodeGraphemeClusterBreak runtime domain.
        // ====================================================================

        static inline bool breakClassFromField(const ByteSpan& field,
            UnicodeGraphemeClusterBreak& outValue) noexcept
        {
            // ----------------------------------------------------------------
            // Default
            // ----------------------------------------------------------------

            if (spanEquals(field, "XX") ||
                spanEquals(field, "Other"))
            {
                outValue =
                    UnicodeGraphemeClusterBreak::Other;

                return true;
            }


            // ----------------------------------------------------------------
            // Controls
            // ----------------------------------------------------------------

            if (spanEquals(field, "CR"))
            {
                outValue =
                    UnicodeGraphemeClusterBreak::CR;

                return true;
            }


            if (spanEquals(field, "LF"))
            {
                outValue =
                    UnicodeGraphemeClusterBreak::LF;

                return true;
            }


            if (spanEquals(field, "CN") ||
                spanEquals(field, "Control"))
            {
                outValue =
                    UnicodeGraphemeClusterBreak::Control;

                return true;
            }


            // ----------------------------------------------------------------
            // Extenders and special boundaries
            // ----------------------------------------------------------------

            if (spanEquals(field, "EX") ||
                spanEquals(field, "Extend"))
            {
                outValue =
                    UnicodeGraphemeClusterBreak::Extend;

                return true;
            }


            if (spanEquals(field, "ZWJ"))
            {
                outValue =
                    UnicodeGraphemeClusterBreak::ZWJ;

                return true;
            }


            if (spanEquals(field, "RI") ||
                spanEquals(field, "Regional_Indicator"))
            {
                outValue =
                    UnicodeGraphemeClusterBreak::RegionalIndicator;

                return true;
            }


            if (spanEquals(field, "PP") ||
                spanEquals(field, "Prepend"))
            {
                outValue =
                    UnicodeGraphemeClusterBreak::Prepend;

                return true;
            }


            if (spanEquals(field, "SM") ||
                spanEquals(field, "SpacingMark"))
            {
                outValue =
                    UnicodeGraphemeClusterBreak::SpacingMark;

                return true;
            }


            // ----------------------------------------------------------------
            // Hangul
            // ----------------------------------------------------------------

            if (spanEquals(field, "L"))
            {
                outValue =
                    UnicodeGraphemeClusterBreak::L;

                return true;
            }


            if (spanEquals(field, "V"))
            {
                outValue =
                    UnicodeGraphemeClusterBreak::V;

                return true;
            }


            if (spanEquals(field, "T"))
            {
                outValue =
                    UnicodeGraphemeClusterBreak::T;

                return true;
            }


            if (spanEquals(field, "LV"))
            {
                outValue =
                    UnicodeGraphemeClusterBreak::LV;

                return true;
            }


            if (spanEquals(field, "LVT"))
            {
                outValue =
                    UnicodeGraphemeClusterBreak::LVT;

                return true;
            }


            return false;
        }


        // ====================================================================
        // coverageIntersectsRange
        //
        // Grapheme_Cluster_Break is single-valued. Explicit source ranges must
        // therefore never overlap.
        //
        // The UCD file is grouped by property value, not globally by code
        // point, so overlap checking cannot depend on range ordering.
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

    } // namespace ucd_grapheme_cluster_break_detail


    // ========================================================================
    // ucdParseGraphemeClusterBreak
    //
    // Parse auxiliary/GraphemeBreakProperty.txt into a mutable VALUE8 table.
    //
    // Grapheme_Cluster_Break has the simple documented default:
    //
    //      @missing: 0000..10FFFF; Other
    //
    // UCDParser intentionally ignores comment-only @missing metadata, so seed
    // the complete table with Other and then apply every explicit assignment.
    //
    // Unlike Bidi_Class, there are no ordered overlapping @missing rules to
    // preserve.
    // ========================================================================

    static inline bool ucdParseGraphemeClusterBreak(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values,
        UCDGraphemeClusterBreakParseResult& outResult)
    {
        outResult = {};


        // ====================================================================
        // Default entire Unicode address space to Other.
        // ====================================================================

        values.clear(
            static_cast<uint8_t>(
                UnicodeGraphemeClusterBreak::Other));


        // ====================================================================
        // Explicit assignment coverage
        //
        // The VALUE8 table itself cannot distinguish:
        //
        //      default Other
        //
        // from:
        //
        //      explicitly assigned Other
        //
        // so use separate coverage for overlap detection and accounting.
        // ====================================================================

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


            if (!ucdReadCodePointRange(
                fields,
                range))
            {
                outResult.error =
                    UCDGraphemeClusterBreakParseError::InvalidRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Field 2: Grapheme_Cluster_Break
            // ----------------------------------------------------------------

            ByteSpan breakClassField;


            if (!ucdReadField(
                fields,
                breakClassField) ||
                !breakClassField)
            {
                outResult.error =
                    UCDGraphemeClusterBreakParseError::MissingBreakClass;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // GraphemeBreakProperty.txt records contain exactly two fields.
            // ----------------------------------------------------------------

            bspan_trim_spaces(fields);


            if (fields)
            {
                outResult.error =
                    UCDGraphemeClusterBreakParseError::UnexpectedField;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Property value
            // ----------------------------------------------------------------

            UnicodeGraphemeClusterBreak breakClass;


            if (!ucd_grapheme_cluster_break_detail::breakClassFromField(
                breakClassField,
                breakClass))
            {
                outResult.error =
                    UCDGraphemeClusterBreakParseError::UnknownBreakClass;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Explicit ranges must not overlap.
            // ----------------------------------------------------------------

            if (ucd_grapheme_cluster_break_detail::coverageIntersectsRange(
                assignedCoverage,
                range.first,
                range.last))
            {
                outResult.error =
                    UCDGraphemeClusterBreakParseError::OverlappingRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Apply explicit assignment.
            // ----------------------------------------------------------------

            values.setRange(
                range.first,
                range.last,
                static_cast<uint8_t>(
                    breakClass));


            assignedCoverage.addRange( range.first, range.last);


            ++outResult.rangeCount;


            outResult.explicitCodePoints +=
                static_cast<size_t>(
                    range.last -
                    range.first +
                    1u);
        }


        // ====================================================================
        // Everything not explicitly listed retains Other.
        // ====================================================================

        outResult.defaultedCodePoints = static_cast<size_t>(kUnicodeLimit) - outResult.explicitCodePoints;


        return true;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static inline bool ucdParseGraphemeClusterBreak(const ByteSpan& source, UnicodeValueTable8Builder& values)
    {
        UCDGraphemeClusterBreakParseResult result;

        return ucdParseGraphemeClusterBreak(
            source,
            values,
            result);
    }

} // namespace waavs
