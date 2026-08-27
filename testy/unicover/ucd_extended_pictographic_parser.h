// ucd_extended_pictographic_parser.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ucd_parser.h"
#include "unicode_coverage_builder.h"


namespace waavs
{
    // ========================================================================
    // UCDExtendedPictographicParseError
    // ========================================================================

    enum class UCDExtendedPictographicParseError : uint8_t
    {
        None = 0,

        InvalidRange,
        MissingProperty,
        UnexpectedField,
        OverlappingRange,
        NoPropertyData
    };


    // ========================================================================
    // UCDExtendedPictographicParseResult
    // ========================================================================

    struct UCDExtendedPictographicParseResult
    {
        UCDExtendedPictographicParseError error{
            UCDExtendedPictographicParseError::None
        };

        uint32_t lineNumber{ 0 };
        uint32_t rangeCount{ 0 };

        size_t codePoints{ 0 };


        [[nodiscard]]
        bool success() const noexcept {
            return error == UCDExtendedPictographicParseError::None;
        }


        explicit operator bool() const noexcept {
            return success();
        }
    };


    // ========================================================================
    // Error text
    // ========================================================================

    static inline const char* ucdExtendedPictographicParseErrorString(UCDExtendedPictographicParseError error) noexcept
    {
        switch (error)
        {
        case UCDExtendedPictographicParseError::None:
            return "no error";

        case UCDExtendedPictographicParseError::InvalidRange:
            return "invalid Extended_Pictographic code-point range";

        case UCDExtendedPictographicParseError::MissingProperty:
            return "missing emoji property";

        case UCDExtendedPictographicParseError::UnexpectedField:
            return "unexpected extra field in emoji-data record";

        case UCDExtendedPictographicParseError::OverlappingRange:
            return "Extended_Pictographic ranges overlap";

        case UCDExtendedPictographicParseError::NoPropertyData:
            return "no Extended_Pictographic data found";
        }

        return "unknown Extended_Pictographic parser error";
    }


    namespace ucd_extended_pictographic_detail
    {
        static inline bool spanEquals(const ByteSpan& span, const char* text) noexcept
        {
            const size_t length = std::strlen(text);

            return span.size() == length &&
                std::memcmp(span.data(), text, length) == 0;
        }


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

    } // namespace ucd_extended_pictographic_detail


    // ========================================================================
    // ucdParseExtendedPictographic
    //
    // Parse Extended_Pictographic from emoji/emoji-data.txt.
    //
    // emoji-data.txt contains several binary emoji properties. All records
    // belonging to properties other than Extended_Pictographic are ignored.
    //
    // Omitted code points have Extended_Pictographic=No, so an empty coverage
    // builder naturally represents the default state.
    // ========================================================================

    static inline bool ucdParseExtendedPictographic(const ByteSpan& source,
        UnicodeCoverageBuilder& coverage,
        UCDExtendedPictographicParseResult& outResult)
    {
        outResult = {};
        coverage.clear();

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
                    UCDExtendedPictographicParseError::InvalidRange;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Field 2: emoji property name
            // ----------------------------------------------------------------

            ByteSpan propertyField;

            if (!ucdReadField(fields, propertyField) || !propertyField)
            {
                outResult.error =
                    UCDExtendedPictographicParseError::MissingProperty;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // emoji-data.txt records contain exactly two fields before comment.
            // ----------------------------------------------------------------

            bspan_trim_spaces(fields);

            if (fields)
            {
                outResult.error =
                    UCDExtendedPictographicParseError::UnexpectedField;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Ignore every other emoji property in the source file.
            // ----------------------------------------------------------------

            if (!ucd_extended_pictographic_detail::spanEquals(
                propertyField, "Extended_Pictographic"))
            {
                continue;
            }


            // ----------------------------------------------------------------
            // Extended_Pictographic is binary. Its Yes ranges must not overlap.
            // ----------------------------------------------------------------

            if (ucd_extended_pictographic_detail::coverageIntersectsRange(
                coverage, range.first, range.last))
            {
                outResult.error =
                    UCDExtendedPictographicParseError::OverlappingRange;

                outResult.lineNumber = line.lineNumber;

                return false;
            }


            coverage.addRange(range.first, range.last);

            ++outResult.rangeCount;

            outResult.codePoints +=
                static_cast<size_t>(range.last - range.first + 1u);
        }


        if (outResult.rangeCount == 0)
        {
            outResult.error =
                UCDExtendedPictographicParseError::NoPropertyData;

            return false;
        }


        return true;
    }


    static inline bool ucdParseExtendedPictographic(const ByteSpan& source,
        UnicodeCoverageBuilder& coverage)
    {
        UCDExtendedPictographicParseResult result;

        return ucdParseExtendedPictographic(source, coverage, result);
    }

} // namespace waavs