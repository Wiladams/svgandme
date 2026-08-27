// ucd_normalization_props_parser.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

#include "ucd_parser.h"
#include "unicode_coverage_builder.h"


namespace waavs
{
    // ========================================================================
    // UCDNormalizationPropsParseError
    // ========================================================================

    enum class UCDNormalizationPropsParseError : uint8_t
    {
        None = 0,

        InvalidRange,
        MissingPropertyName,
        UnexpectedField,
        OverlappingRange,
        MissingFullCompositionExclusion
    };


    // ========================================================================
    // UCDNormalizationPropsParseResult
    //
    // recordCount:
    //      Number of meaningful records examined in
    //      DerivedNormalizationProps.txt.
    //
    // rangeCount:
    //      Number of Full_Composition_Exclusion ranges.
    //
    // codePointCount:
    //      Number of code points contained in those ranges.
    // ========================================================================

    struct UCDNormalizationPropsParseResult
    {
        UCDNormalizationPropsParseError error{
            UCDNormalizationPropsParseError::None };

        uint32_t lineNumber{ 0 };
        uint32_t recordCount{ 0 };
        uint32_t rangeCount{ 0 };

        size_t codePointCount{ 0 };


        [[nodiscard]]
        bool success() const noexcept {
            return error == UCDNormalizationPropsParseError::None;
        }


        explicit operator bool() const noexcept {
            return success();
        }
    };


    // ========================================================================
    // Error text
    // ========================================================================

    static inline const char* ucdNormalizationPropsParseErrorString(
        UCDNormalizationPropsParseError error) noexcept
    {
        switch (error)
        {
        case UCDNormalizationPropsParseError::None:
            return "no error";

        case UCDNormalizationPropsParseError::InvalidRange:
            return "invalid code-point range";

        case UCDNormalizationPropsParseError::MissingPropertyName:
            return "missing normalization property name";

        case UCDNormalizationPropsParseError::UnexpectedField:
            return "unexpected extra field in Full_Composition_Exclusion record";

        case UCDNormalizationPropsParseError::OverlappingRange:
            return "Full_Composition_Exclusion contains overlapping ranges";

        case UCDNormalizationPropsParseError::MissingFullCompositionExclusion:
            return "Full_Composition_Exclusion property not found";
        }

        return "unknown DerivedNormalizationProps.txt parser error";
    }


    namespace ucd_normalization_props_detail
    {
        // ====================================================================
        // coverageIntersectsRange
        //
        // Generator-side overlap validation.
        //
        // Full_Composition_Exclusion is a binary property. Each code point
        // should therefore occur at most once in its explicit range list.
        //
        // The property is small, so direct lookup across the incoming range is
        // simple and entirely adequate here.
        // ====================================================================

        static inline bool coverageIntersectsRange(
            const UnicodeCoverageBuilder& coverage,
            uint32_t first,
            uint32_t last) noexcept
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
    // ucdParseFullCompositionExclusion
    //
    // Parse:
    //
    //      DerivedNormalizationProps.txt
    //
    // and extract:
    //
    //      Full_Composition_Exclusion
    //
    // into a mutable UnicodeCoverageBuilder.
    //
    //
    // Example source records:
    //
    //      0340..0341 ; Full_Composition_Exclusion
    //      0343..0344 ; Full_Composition_Exclusion
    //      0374       ; Full_Composition_Exclusion
    //      ...
    //
    //
    // DerivedNormalizationProps.txt contains many other properties:
    //
    //      FC_NFKC
    //      NFD_QC
    //      NFC_QC
    //      NFKD_QC
    //      NFKC_QC
    //      ...
    //
    // Those records are intentionally ignored.
    //
    // Some of those other properties contain a third field, so extra-field
    // validation is performed only after identifying a
    // Full_Composition_Exclusion record.
    //
    //
    // The operation is transactional with respect to outCoverage:
    //
    //      success -> outCoverage is replaced with the parsed coverage
    //      failure -> outCoverage is unchanged
    //
    // ========================================================================

    static inline bool ucdParseFullCompositionExclusion(
        const ByteSpan& source,
        UnicodeCoverageBuilder& outCoverage,
        UCDNormalizationPropsParseResult& outResult)
    {
        outResult = {};


        // ====================================================================
        // Build locally so failure never leaves a partially populated caller
        // result.
        // ====================================================================

        UnicodeCoverageBuilder coverage;


        UCDParser parser(source);
        UCDLine line;


        while (parser.next(line))
        {
            ByteSpan fields =
                line.data;


            // ----------------------------------------------------------------
            // Field 1: code-point range
            //
            // Every meaningful DerivedNormalizationProps.txt record begins
            // with a Unicode code-point or inclusive range.
            // ----------------------------------------------------------------

            UCDCodePointRange range;


            if (!ucdReadCodePointRange(
                fields,
                range))
            {
                outResult.error =
                    UCDNormalizationPropsParseError::InvalidRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Field 2: property name
            // ----------------------------------------------------------------

            ByteSpan propertyName;


            if (!ucdReadField(
                fields,
                propertyName) ||
                !propertyName)
            {
                outResult.error =
                    UCDNormalizationPropsParseError::MissingPropertyName;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            ++outResult.recordCount;


            // ----------------------------------------------------------------
            // We are deliberately interested in only one property.
            //
            // Do not validate the remaining fields of unrelated records:
            // properties such as FC_NFKC and the Quick_Check properties have
            // additional value/mapping fields.
            // ----------------------------------------------------------------

            if (propertyName !=
                "Full_Composition_Exclusion")
            {
                continue;
            }


            // ----------------------------------------------------------------
            // Full_Composition_Exclusion is a binary property record and has
            // no additional data field.
            // ----------------------------------------------------------------

            bspan_trim_spaces(fields);


            if (fields)
            {
                outResult.error =
                    UCDNormalizationPropsParseError::UnexpectedField;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Reject duplicate or overlapping Full_Composition_Exclusion
            // ranges.
            // ----------------------------------------------------------------

            if (ucd_normalization_props_detail::coverageIntersectsRange(
                coverage,
                range.first,
                range.last))
            {
                outResult.error =
                    UCDNormalizationPropsParseError::OverlappingRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Add the range.
            // ----------------------------------------------------------------

            coverage.addRange(
                range.first,
                range.last);


            ++outResult.rangeCount;


            outResult.codePointCount +=
                static_cast<size_t>(
                    range.last -
                    range.first +
                    1u);
        }


        // ====================================================================
        // The property is required for generation of the canonical composition
        // map.
        // ====================================================================

        if (outResult.rangeCount == 0)
        {
            outResult.error =
                UCDNormalizationPropsParseError::
                MissingFullCompositionExclusion;

            return false;
        }


        // ====================================================================
        // Commit only after complete success.
        // ====================================================================

        outCoverage =
            std::move(coverage);


        return true;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static inline bool ucdParseFullCompositionExclusion(
        const ByteSpan& source,
        UnicodeCoverageBuilder& outCoverage)
    {
        UCDNormalizationPropsParseResult result;

        return
            ucdParseFullCompositionExclusion(
                source,
                outCoverage,
                result);
    }

} // namespace waavs
