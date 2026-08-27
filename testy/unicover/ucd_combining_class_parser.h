// ucd_combining_class_parser.h

#pragma once

#include <cstddef>
#include <cstdint>

#include "ucd_parser.h"
#include "unicode_combining_class.h"
#include "unicode_coverage_builder.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    // ========================================================================
    // UCDCombiningClassParseError
    // ========================================================================

    enum class UCDCombiningClassParseError : uint8_t
    {
        None = 0,

        InvalidRange,
        MissingCombiningClass,
        UnexpectedField,
        InvalidCombiningClass,
        OverlappingRange
    };


    // ========================================================================
    // UCDCombiningClassParseResult
    //
    // rangeCount:
    //
    //      Number of explicit DerivedCombiningClass.txt ranges parsed.
    //
    // explicitCodePoints:
    //
    //      Number of code points explicitly represented by input records.
    //
    // nonZeroCodePoints:
    //
    //      Number of explicitly represented code points whose CCC is nonzero.
    //
    // Code points not explicitly represented by the file remain:
    //
    //      Canonical_Combining_Class = 0
    //
    // ========================================================================

    struct UCDCombiningClassParseResult
    {
        UCDCombiningClassParseError error{ UCDCombiningClassParseError::None };

        uint32_t lineNumber{ 0 };
        uint32_t rangeCount{ 0 };

        size_t explicitCodePoints{ 0 };
        size_t nonZeroCodePoints{ 0 };


        [[nodiscard]]
        bool success() const noexcept
        {
            return error == UCDCombiningClassParseError::None;
        }


        explicit operator bool() const noexcept
        {
            return success();
        }
    };


    // ========================================================================
    // ucdCombiningClassParseErrorString
    // ========================================================================

    static inline const char* ucdCombiningClassParseErrorString(
        UCDCombiningClassParseError error) noexcept
    {
        switch (error)
        {
        case UCDCombiningClassParseError::None:
            return "no error";

        case UCDCombiningClassParseError::InvalidRange:
            return "invalid Canonical_Combining_Class code-point range";

        case UCDCombiningClassParseError::MissingCombiningClass:
            return "missing Canonical_Combining_Class value";

        case UCDCombiningClassParseError::UnexpectedField:
            return "unexpected extra field in Canonical_Combining_Class record";

        case UCDCombiningClassParseError::InvalidCombiningClass:
            return "invalid Canonical_Combining_Class value";

        case UCDCombiningClassParseError::OverlappingRange:
            return "Canonical_Combining_Class ranges overlap";
        }

        return "unknown Canonical_Combining_Class parser error";
    }


    namespace ucd_combining_class_detail
    {
        // ====================================================================
        // parseCombiningClass
        //
        // DerivedCombiningClass.txt stores the Canonical_Combining_Class as a
        // decimal integer:
        //
        //      0300..0314 ; 230
        //      0315       ; 232
        //
        // Valid Unicode values are 0 .. 254.
        // ====================================================================

        static inline bool parseCombiningClass(
            const ByteSpan& source,
            UnicodeCombiningClass& outValue) noexcept
        {
            ByteSpan field = source;

            bspan_trim_spaces(field);

            if (!field)
                return false;


            uint32_t value = 0;

            const uint8_t* p = field.begin();
            const uint8_t* end = field.end();


            while (p < end)
            {
                const uint8_t ch = *p++;

                if (ch < '0' || ch > '9')
                    return false;

                value =
                    value * 10u +
                    static_cast<uint32_t>(ch - '0');

                if (!unicodeCombiningClassIsValid(value))
                    return false;
            }


            outValue =
                static_cast<UnicodeCombiningClass>(value);

            return true;
        }


        // ====================================================================
        // coverageIntersectsRange
        //
        // Canonical_Combining_Class is single-valued, so explicit input
        // ranges must never overlap.
        //
        // DerivedCombiningClass.txt is grouped by combining-class value rather
        // than globally ordered by code point, so validation must not rely on
        // previous-range ordering.
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
    // ucdParseCombiningClass
    //
    // Parse DerivedCombiningClass.txt into a mutable VALUE8 table.
    //
    // Expected meaningful line syntax:
    //
    //      range ; decimal-combining-class
    //
    // Examples:
    //
    //      0000..001F ; 0
    //      0300..0314 ; 230
    //      0315       ; 232
    //
    //
    // Construction:
    //
    //      DerivedCombiningClass.txt
    //              |
    //              v
    //      range + decimal CCC
    //              |
    //              v
    //      UnicodeCombiningClass
    //              |
    //              v
    //      UnicodeValueTable8Builder
    //
    //
    // The complete table is initialized to:
    //
    //      Not_Reordered = 0
    //
    // before explicit records are applied.
    //
    // This implements the documented @missing default without requiring
    // UCDParser to expose comment-only metadata lines.
    //
    // Every explicit range is tracked independently so malformed overlapping
    // records are detected even when both records contain value zero.
    //
    // On failure 'values' may contain assignments produced before the error.
    // Generation should therefore be considered failed and the builder
    // discarded or cleared.
    //
    // ========================================================================

    static inline bool ucdParseCombiningClass(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values,
        UCDCombiningClassParseResult& outResult)
    {
        outResult = {};


        // --------------------------------------------------------------------
        // Canonical_Combining_Class default:
        //
        //      Not_Reordered = 0
        // --------------------------------------------------------------------

        values.clear(
            kUnicodeCombiningClassNotReordered);


        // --------------------------------------------------------------------
        // Track explicit assignments independently from their values.
        //
        // Value zero is itself a legitimate explicit CCC assignment, so the
        // VALUE8 table cannot be used to detect whether a code point has
        // already appeared in the input.
        // --------------------------------------------------------------------

        UnicodeCoverageBuilder assignedCoverage;


        UCDParser parser(source);
        UCDLine line;


        while (parser.next(line))
        {
            ByteSpan fields = line.data;


            // ----------------------------------------------------------------
            // Field 1: code-point range
            // ----------------------------------------------------------------

            UCDCodePointRange range;


            if (!ucdReadCodePointRange(fields, range))
            {
                outResult.error =
                    UCDCombiningClassParseError::InvalidRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Field 2: decimal Canonical_Combining_Class
            // ----------------------------------------------------------------

            ByteSpan combiningClassField;


            if (!ucdReadField(fields, combiningClassField) ||
                !combiningClassField)
            {
                outResult.error =
                    UCDCombiningClassParseError::MissingCombiningClass;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // DerivedCombiningClass.txt records contain exactly two fields.
            // ----------------------------------------------------------------

            bspan_trim_spaces(fields);


            if (fields)
            {
                outResult.error =
                    UCDCombiningClassParseError::UnexpectedField;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Parse decimal CCC.
            // ----------------------------------------------------------------

            UnicodeCombiningClass combiningClass = 0;


            if (!ucd_combining_class_detail::parseCombiningClass(
                combiningClassField,
                combiningClass))
            {
                outResult.error =
                    UCDCombiningClassParseError::InvalidCombiningClass;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // CCC is single-valued. Explicit ranges must not overlap.
            // ----------------------------------------------------------------

            if (ucd_combining_class_detail::coverageIntersectsRange(
                assignedCoverage,
                range.first,
                range.last))
            {
                outResult.error =
                    UCDCombiningClassParseError::OverlappingRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Apply assignment.
            // ----------------------------------------------------------------

            values.setRange(
                range.first,
                range.last,
                combiningClass);


            assignedCoverage.addRange(
                range.first,
                range.last);


            const size_t codePointCount =
                static_cast<size_t>(
                    range.last -
                    range.first +
                    1u);


            ++outResult.rangeCount;

            outResult.explicitCodePoints +=
                codePointCount;


            if (combiningClass !=
                kUnicodeCombiningClassNotReordered)
            {
                outResult.nonZeroCodePoints +=
                    codePointCount;
            }
        }


        return true;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static inline bool ucdParseCombiningClass(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values)
    {
        UCDCombiningClassParseResult result;

        return
            ucdParseCombiningClass(
                source,
                values,
                result);
    }

} // namespace waavs
