// ucd_bidi_class_parser.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ucd_parser.h"
#include "unicode_bidi_class.h"
#include "unicode_coverage_builder.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    // ========================================================================
    // UCDBidiClassParseError
    // ========================================================================

    enum class UCDBidiClassParseError : uint8_t
    {
        None = 0,

        MissingDefault,
        InvalidDefaultRange,
        MissingDefaultClass,
        UnexpectedDefaultField,
        UnknownDefaultClass,

        InvalidRange,
        MissingClass,
        UnexpectedField,
        UnknownClass,
        OverlappingRange
    };


    // ========================================================================
    // UCDBidiClassParseResult
    // ========================================================================

    struct UCDBidiClassParseResult
    {
        UCDBidiClassParseError error{ UCDBidiClassParseError::None };

        uint32_t lineNumber{ 0 };
        uint32_t rangeCount{ 0 };
        uint32_t missingRangeCount{ 0 };

        size_t explicitCodePoints{ 0 };
        size_t defaultedCodePoints{ 0 };


        [[nodiscard]] bool success() const noexcept {
            return error == UCDBidiClassParseError::None;
        }

        explicit operator bool() const noexcept {
            return success();
        }
    };


    // ========================================================================
    // ucdBidiClassParseErrorString
    // ========================================================================

    static inline const char* ucdBidiClassParseErrorString(
        UCDBidiClassParseError error) noexcept
    {
        switch (error)
        {
        case UCDBidiClassParseError::None:
            return "no error";

        case UCDBidiClassParseError::MissingDefault:
            return "missing full-range Bidi_Class @missing default";

        case UCDBidiClassParseError::InvalidDefaultRange:
            return "invalid Bidi_Class @missing code-point range";

        case UCDBidiClassParseError::MissingDefaultClass:
            return "missing Bidi_Class @missing value";

        case UCDBidiClassParseError::UnexpectedDefaultField:
            return "unexpected extra field in Bidi_Class @missing record";

        case UCDBidiClassParseError::UnknownDefaultClass:
            return "unknown Bidi_Class @missing value";

        case UCDBidiClassParseError::InvalidRange:
            return "invalid Bidi_Class code-point range";

        case UCDBidiClassParseError::MissingClass:
            return "missing Bidi_Class value";

        case UCDBidiClassParseError::UnexpectedField:
            return "unexpected extra field in Bidi_Class record";

        case UCDBidiClassParseError::UnknownClass:
            return "unknown Bidi_Class value";

        case UCDBidiClassParseError::OverlappingRange:
            return "explicit Bidi_Class ranges overlap";
        }

        return "unknown Bidi_Class parser error";
    }


    namespace ucd_bidi_class_detail
    {
        // ====================================================================
        // spanEquals
        // ====================================================================

        static inline bool spanEquals(const ByteSpan& span, const char* text) noexcept
        {
            if (!text)
                return false;

            const size_t length = std::strlen(text);

            return span.size() == length &&
                (length == 0 || std::memcmp(span.data(), text, length) == 0);
        }


        // ====================================================================
        // spanStartsWith
        // ====================================================================

        static inline bool spanStartsWith(const ByteSpan& span, const char* text) noexcept
        {
            if (!text)
                return false;

            const size_t length = std::strlen(text);

            return span.size() >= length &&
                (length == 0 || std::memcmp(span.data(), text, length) == 0);
        }


        // ====================================================================
        // bidiClassFromField
        //
        // Accept both canonical short aliases used by explicit data records and
        // canonical long aliases used by @missing records.
        // ====================================================================

        static inline bool bidiClassFromField(
            const ByteSpan& field,
            UnicodeBidiClass& outClass) noexcept
        {
            struct NameRecord
            {
                const char* shortName;
                const char* longName;
                UnicodeBidiClass value;
            };


            static constexpr NameRecord names[] =
            {
                { "L",   "Left_To_Right",          UnicodeBidiClass::LeftToRight },
                { "R",   "Right_To_Left",          UnicodeBidiClass::RightToLeft },
                { "AL",  "Arabic_Letter",          UnicodeBidiClass::ArabicLetter },

                { "EN",  "European_Number",        UnicodeBidiClass::EuropeanNumber },
                { "ES",  "European_Separator",     UnicodeBidiClass::EuropeanSeparator },
                { "ET",  "European_Terminator",    UnicodeBidiClass::EuropeanTerminator },
                { "AN",  "Arabic_Number",          UnicodeBidiClass::ArabicNumber },
                { "CS",  "Common_Separator",       UnicodeBidiClass::CommonSeparator },
                { "NSM", "Nonspacing_Mark",        UnicodeBidiClass::NonspacingMark },
                { "BN",  "Boundary_Neutral",       UnicodeBidiClass::BoundaryNeutral },

                { "B",   "Paragraph_Separator",    UnicodeBidiClass::ParagraphSeparator },
                { "S",   "Segment_Separator",      UnicodeBidiClass::SegmentSeparator },
                { "WS",  "White_Space",            UnicodeBidiClass::WhiteSpace },
                { "ON",  "Other_Neutral",          UnicodeBidiClass::OtherNeutral },

                { "LRE", "Left_To_Right_Embedding", UnicodeBidiClass::LeftToRightEmbedding },
                { "LRO", "Left_To_Right_Override", UnicodeBidiClass::LeftToRightOverride },
                { "RLE", "Right_To_Left_Embedding", UnicodeBidiClass::RightToLeftEmbedding },
                { "RLO", "Right_To_Left_Override", UnicodeBidiClass::RightToLeftOverride },
                { "PDF", "Pop_Directional_Format", UnicodeBidiClass::PopDirectionalFormat },
                { "LRI", "Left_To_Right_Isolate",  UnicodeBidiClass::LeftToRightIsolate },
                { "RLI", "Right_To_Left_Isolate",  UnicodeBidiClass::RightToLeftIsolate },
                { "FSI", "First_Strong_Isolate",   UnicodeBidiClass::FirstStrongIsolate },
                { "PDI", "Pop_Directional_Isolate", UnicodeBidiClass::PopDirectionalIsolate }
            };


            for (const NameRecord& name : names)
            {
                if (spanEquals(field, name.shortName) ||
                    spanEquals(field, name.longName))
                {
                    outClass = name.value;
                    return true;
                }
            }


            return false;
        }


        // ====================================================================
        // coverageIntersectsRange
        //
        // Explicit Bidi_Class data is single-valued and may not overlap.
        //
        // @missing ranges are intentionally NOT checked with this function:
        // Unicode permits overlapping @missing ranges whose later assignments
        // override earlier defaults.
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


        // ====================================================================
        // applyMissingDefaults
        //
        // Scan the original source rather than UCDParser because @missing
        // records are comment-only lines.
        //
        // Multiple @missing records are applied in source order. Later records
        // therefore override earlier defaults exactly as required by UAX #44.
        // ====================================================================

        static inline bool applyMissingDefaults(
            const ByteSpan& source,
            UnicodeValueTable8Builder& values,
            UCDBidiClassParseResult& outResult)
        {
            static constexpr char prefix[] = "@missing:";
            static constexpr size_t prefixLength = sizeof(prefix) - 1;


            ByteSpan remaining = source;

            uint32_t lineNumber = 0;
            bool sawFullDefault = false;


            while (remaining)
            {
                ByteSpan line =
                    bspan_read_until(remaining, '\n');

                ++lineNumber;

                bspan_trim_spaces(line);

                if (!line || *line != '#')
                    continue;


                ++line;
                bspan_ltrim_spaces(line);


                if (!spanStartsWith(line, prefix))
                    continue;


                line.advance(prefixLength);
                bspan_trim_spaces(line);


                UCDCodePointRange range;

                if (!ucdReadCodePointRange(line, range))
                {
                    outResult.error =
                        UCDBidiClassParseError::InvalidDefaultRange;

                    outResult.lineNumber =
                        lineNumber;

                    return false;
                }


                ByteSpan classField;

                if (!ucdReadField(line, classField) ||
                    !classField)
                {
                    outResult.error =
                        UCDBidiClassParseError::MissingDefaultClass;

                    outResult.lineNumber =
                        lineNumber;

                    return false;
                }


                bspan_trim_spaces(line);

                if (line)
                {
                    outResult.error =
                        UCDBidiClassParseError::UnexpectedDefaultField;

                    outResult.lineNumber =
                        lineNumber;

                    return false;
                }


                UnicodeBidiClass bidiClass;

                if (!bidiClassFromField(
                    classField,
                    bidiClass))
                {
                    outResult.error =
                        UCDBidiClassParseError::UnknownDefaultClass;

                    outResult.lineNumber =
                        lineNumber;

                    return false;
                }


                values.setRange(
                    range.first,
                    range.last,
                    static_cast<uint8_t>(bidiClass));


                if (range.first == 0 &&
                    range.last == kUnicodeLimit - 1u)
                {
                    sawFullDefault = true;
                }


                ++outResult.missingRangeCount;
            }


            if (!sawFullDefault)
            {
                outResult.error =
                    UCDBidiClassParseError::MissingDefault;

                return false;
            }


            return true;
        }

    } // namespace ucd_bidi_class_detail


    // ========================================================================
    // ucdParseBidiClass
    //
    // Parse extracted/DerivedBidiClass.txt into a VALUE8 builder.
    //
    // Processing:
    //
    //      pass 1:
    //
    //          apply all @missing defaults in source order
    //
    //      pass 2:
    //
    //          parse explicit Bidi_Class records
    //          reject explicit overlap
    //          overwrite defaults with explicit assignments
    //
    // The resulting table contains one Bidi_Class value for every Unicode
    // code-point position.
    // ========================================================================

    static inline bool ucdParseBidiClass(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values,
        UCDBidiClassParseResult& outResult)
    {
        outResult = {};


        // --------------------------------------------------------------------
        // Begin with the broad Unicode default.
        //
        // The source file is still required to contain its full-range
        // @missing record; this initialization merely gives deterministic
        // state before the source-driven defaults are applied.
        // --------------------------------------------------------------------

        values.clear(
            static_cast<uint8_t>(
                UnicodeBidiClass::LeftToRight));


        // --------------------------------------------------------------------
        // Pass 1: complex @missing defaults.
        // --------------------------------------------------------------------

        if (!ucd_bidi_class_detail::applyMissingDefaults(
            source,
            values,
            outResult))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // Pass 2: explicit data.
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
                    UCDBidiClassParseError::InvalidRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Field 2: Bidi_Class short alias
            // ----------------------------------------------------------------

            ByteSpan classField;

            if (!ucdReadField(fields, classField) ||
                !classField)
            {
                outResult.error =
                    UCDBidiClassParseError::MissingClass;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // DerivedBidiClass.txt data records contain exactly two fields.
            // ----------------------------------------------------------------

            bspan_trim_spaces(fields);

            if (fields)
            {
                outResult.error =
                    UCDBidiClassParseError::UnexpectedField;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            UnicodeBidiClass bidiClass;

            if (!ucd_bidi_class_detail::bidiClassFromField(
                classField,
                bidiClass))
            {
                outResult.error =
                    UCDBidiClassParseError::UnknownClass;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Explicit Bidi_Class ranges are single-valued and may not overlap.
            // ----------------------------------------------------------------

            if (ucd_bidi_class_detail::coverageIntersectsRange(
                assignedCoverage,
                range.first,
                range.last))
            {
                outResult.error =
                    UCDBidiClassParseError::OverlappingRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Explicit assignments override the previously established
            // @missing defaults.
            // ----------------------------------------------------------------

            values.setRange(
                range.first,
                range.last,
                static_cast<uint8_t>(bidiClass));

            assignedCoverage.addRange(
                range.first,
                range.last);


            ++outResult.rangeCount;

            outResult.explicitCodePoints +=
                static_cast<size_t>(
                    range.last -
                    range.first +
                    1u);
        }


        outResult.defaultedCodePoints =
            static_cast<size_t>(kUnicodeLimit) -
            outResult.explicitCodePoints;


        return true;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static inline bool ucdParseBidiClass(
        const ByteSpan& source,
        UnicodeValueTable8Builder& values)
    {
        UCDBidiClassParseResult result;

        return ucdParseBidiClass(
            source,
            values,
            result);
    }

} // namespace waavs
