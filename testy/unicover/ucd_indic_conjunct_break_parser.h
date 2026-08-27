// ucd_indic_conjunct_break_parser.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ucd_parser.h"
#include "unicode_coverage_builder.h"
#include "unicode_indic_conjunct_break.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    // ========================================================================
    // UCDIndicConjunctBreakParseError
    // ========================================================================

    enum class UCDIndicConjunctBreakParseError : uint8_t
    {
        None = 0,
        InvalidRange,
        MissingValue,
        UnexpectedField,
        UnknownValue,
        OverlappingRange,
        NoPropertyData
    };


    // ========================================================================
    // UCDIndicConjunctBreakParseResult
    // ========================================================================

    struct UCDIndicConjunctBreakParseResult
    {
        UCDIndicConjunctBreakParseError error{
            UCDIndicConjunctBreakParseError::None
        };

        uint32_t lineNumber{ 0 };
        uint32_t rangeCount{ 0 };

        size_t explicitCodePoints{ 0 };
        size_t defaultedCodePoints{ 0 };


        [[nodiscard]]
        bool success() const noexcept {
            return error == UCDIndicConjunctBreakParseError::None;
        }


        explicit operator bool() const noexcept {
            return success();
        }
    };


    // ========================================================================
    // Error text
    // ========================================================================

    static inline const char* ucdIndicConjunctBreakParseErrorString(
        UCDIndicConjunctBreakParseError error) noexcept
    {
        switch (error)
        {
        case UCDIndicConjunctBreakParseError::None:
            return "no error";

        case UCDIndicConjunctBreakParseError::InvalidRange:
            return "invalid Indic_Conjunct_Break code-point range";

        case UCDIndicConjunctBreakParseError::MissingValue:
            return "missing Indic_Conjunct_Break value";

        case UCDIndicConjunctBreakParseError::UnexpectedField:
            return "unexpected extra field in Indic_Conjunct_Break record";

        case UCDIndicConjunctBreakParseError::UnknownValue:
            return "unknown Indic_Conjunct_Break value";

        case UCDIndicConjunctBreakParseError::OverlappingRange:
            return "Indic_Conjunct_Break ranges overlap";

        case UCDIndicConjunctBreakParseError::NoPropertyData:
            return "no Indic_Conjunct_Break data found";
        }

        return "unknown Indic_Conjunct_Break parser error";
    }


    namespace ucd_indic_conjunct_break_detail
    {
        // ====================================================================
        // spanEquals
        // ====================================================================

        static inline bool spanEquals(const ByteSpan& span, const char* text) noexcept
        {
            const size_t length = std::strlen(text);

            return span.size() == length &&
                std::memcmp(span.data(), text, length) == 0;
        }


        // ====================================================================
        // valueFromField
        // ====================================================================

        static inline bool valueFromField(const ByteSpan& field,
            UnicodeIndicConjunctBreak& outValue) noexcept
        {
            if (spanEquals(field, "None"))
            {
                outValue = UnicodeIndicConjunctBreak::None;
                return true;
            }

            if (spanEquals(field, "Consonant"))
            {
                outValue = UnicodeIndicConjunctBreak::Consonant;
                return true;
            }

            if (spanEquals(field, "Extend"))
            {
                outValue = UnicodeIndicConjunctBreak::Extend;
                return true;
            }

            if (spanEquals(field, "Linker"))
            {
                outValue = UnicodeIndicConjunctBreak::Linker;
                return true;
            }

            return false;
        }


        // ====================================================================
        // coverageIntersectsRange
        // ====================================================================

        static inline bool coverageIntersectsRange(const UnicodeCoverageBuilder& coverage,
            uint32_t first, uint32_t last) noexcept
        {
            for (uint32_t cp = first;; ++cp)
            {
                if (coverage.contains(cp))
                    return true;

                if (cp == last)
                    break;
            }

            return false;
        }

    } // namespace ucd_indic_conjunct_break_detail


    // ========================================================================
    // ucdParseIndicConjunctBreak
    //
    // Parse Indic_Conjunct_Break from DerivedCoreProperties.txt.
    //
    // The source file contains many unrelated derived properties. Only records
    // whose second field is "InCB" are consumed here.
    //
    // InCB records have the form:
    //
    //      code-point-range ; InCB ; value
    //
    // All code points not explicitly listed have the default value None.
    // ========================================================================

    static inline bool ucdParseIndicConjunctBreak(const ByteSpan& source,
        UnicodeValueTable8Builder& values,
        UCDIndicConjunctBreakParseResult& outResult)
    {
        outResult = {};

        values.clear(
            static_cast<uint8_t>(
                UnicodeIndicConjunctBreak::None));


        UnicodeCoverageBuilder assignedCoverage;

        UCDParser parser(source);
        UCDLine line;


        while (parser.next(line))
        {
            ByteSpan fields = line.data;


            // ----------------------------------------------------------------
            // Field 1: code-point range
            //
            // Every active data record in DerivedCoreProperties.txt begins
            // with a code-point range, regardless of property.
            // ----------------------------------------------------------------

            UCDCodePointRange range;

            if (!ucdReadCodePointRange(fields, range))
            {
                outResult.error =
                    UCDIndicConjunctBreakParseError::InvalidRange;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Field 2: property name
            //
            // Most records in this file belong to unrelated properties. Ignore
            // them immediately. Their remaining field structure is irrelevant
            // to this parser.
            // ----------------------------------------------------------------

            ByteSpan propertyField;

            if (!ucdReadField(fields, propertyField) || !propertyField)
                continue;


            if (!ucd_indic_conjunct_break_detail::spanEquals(
                propertyField, "InCB"))
            {
                continue;
            }


            // ----------------------------------------------------------------
            // Field 3: Indic_Conjunct_Break value
            // ----------------------------------------------------------------

            ByteSpan valueField;

            if (!ucdReadField(fields, valueField) || !valueField)
            {
                outResult.error =
                    UCDIndicConjunctBreakParseError::MissingValue;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // InCB records contain exactly three fields.
            // ----------------------------------------------------------------

            bspan_trim_spaces(fields);

            if (fields)
            {
                outResult.error =
                    UCDIndicConjunctBreakParseError::UnexpectedField;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Decode value.
            // ----------------------------------------------------------------

            UnicodeIndicConjunctBreak value;

            if (!ucd_indic_conjunct_break_detail::valueFromField(
                valueField, value))
            {
                outResult.error =
                    UCDIndicConjunctBreakParseError::UnknownValue;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // InCB is single-valued. Explicit ranges therefore must not
            // overlap one another.
            // ----------------------------------------------------------------

            if (ucd_indic_conjunct_break_detail::coverageIntersectsRange(
                assignedCoverage, range.first, range.last))
            {
                outResult.error =
                    UCDIndicConjunctBreakParseError::OverlappingRange;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Store explicit value.
            // ----------------------------------------------------------------

            values.setRange(
                range.first,
                range.last,
                static_cast<uint8_t>(value));

            assignedCoverage.addRange(
                range.first,
                range.last);


            ++outResult.rangeCount;

            outResult.explicitCodePoints +=
                static_cast<size_t>(
                    range.last - range.first + 1u);
        }


        // ====================================================================
        // Ensure the requested property was actually present.
        // ====================================================================

        if (outResult.rangeCount == 0)
        {
            outResult.error =
                UCDIndicConjunctBreakParseError::NoPropertyData;

            return false;
        }


        // ====================================================================
        // Remaining Unicode positions have the documented default value None.
        // ====================================================================

        outResult.defaultedCodePoints =
            static_cast<size_t>(kUnicodeLimit) -
            outResult.explicitCodePoints;


        return true;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static inline bool ucdParseIndicConjunctBreak(const ByteSpan& source, UnicodeValueTable8Builder& values)
    {
        UCDIndicConjunctBreakParseResult result;

        return ucdParseIndicConjunctBreak(
            source,
            values,
            result);
    }

} // namespace waavs