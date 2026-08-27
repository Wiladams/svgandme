// ucd_unicode_data_parser.h

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "ucd_parser.h"
#include "unicode_decomposition_builder.h"


namespace waavs
{
    // ========================================================================
    // UCDUnicodeDataParseError
    // ========================================================================

    enum class UCDUnicodeDataParseError : uint8_t
    {
        None = 0,

        InvalidFieldCount,
        InvalidCodePoint,
        InvalidCodePointOrder,
        InvalidDecomposition,
        CanonicalDecompositionTooLong,
        DecompositionAddFailed
    };


    // ========================================================================
    // UCDUnicodeDataParseResult
    //
    // recordCount:
    //
    //      Number of successfully parsed UnicodeData.txt records.
    //
    // canonicalMappingCount:
    //
    //      Number of explicit canonical decomposition mappings.
    //
    // singletonMappingCount:
    //
    //      Number of canonical mappings:
    //
    //          cp -> first
    //
    // pairMappingCount:
    //
    //      Number of canonical mappings:
    //
    //          cp -> first second
    //
    // compatibilityMappingCount:
    //
    //      Number of compatibility decomposition records encountered and
    //      intentionally ignored.
    //
    // ========================================================================

    struct UCDUnicodeDataParseResult
    {
        UCDUnicodeDataParseError error{
            UCDUnicodeDataParseError::None
        };

        uint32_t lineNumber{ 0 };

        size_t recordCount{ 0 };

        size_t canonicalMappingCount{ 0 };
        size_t singletonMappingCount{ 0 };
        size_t pairMappingCount{ 0 };

        size_t compatibilityMappingCount{ 0 };


        [[nodiscard]]
        bool success() const noexcept
        {
            return
                error ==
                UCDUnicodeDataParseError::None;
        }


        explicit operator bool() const noexcept
        {
            return success();
        }
    };


    // ========================================================================
    // ucdUnicodeDataParseErrorString
    // ========================================================================

    static inline const char* ucdUnicodeDataParseErrorString(
        UCDUnicodeDataParseError error) noexcept
    {
        switch (error)
        {
        case UCDUnicodeDataParseError::None:
            return "no error";

        case UCDUnicodeDataParseError::InvalidFieldCount:
            return "UnicodeData.txt record does not contain exactly 15 fields";

        case UCDUnicodeDataParseError::InvalidCodePoint:
            return "invalid UnicodeData.txt code point";

        case UCDUnicodeDataParseError::InvalidCodePointOrder:
            return "UnicodeData.txt code points are not strictly increasing";

        case UCDUnicodeDataParseError::InvalidDecomposition:
            return "invalid UnicodeData.txt decomposition mapping";

        case UCDUnicodeDataParseError::CanonicalDecompositionTooLong:
            return "canonical decomposition contains more than two code points";

        case UCDUnicodeDataParseError::DecompositionAddFailed:
            return "failed to add canonical decomposition mapping";
        }

        return "unknown UnicodeData.txt parser error";
    }


    namespace ucd_unicode_data_detail
    {
        // ====================================================================
        // UnicodeData.txt field layout
        //
        //      0   Code Point
        //      1   Name
        //      2   General Category
        //      3   Canonical Combining Class
        //      4   Bidi Class
        //      5   Decomposition Mapping
        //      6   Decimal Digit Value
        //      7   Digit Value
        //      8   Numeric Value
        //      9   Bidi Mirrored
        //      10  Unicode 1 Name
        //      11  ISO Comment
        //      12  Simple Uppercase Mapping
        //      13  Simple Lowercase Mapping
        //      14  Simple Titlecase Mapping
        //
        // ====================================================================

        static constexpr size_t kUnicodeDataFieldCount = 15;
        static constexpr size_t kUnicodeDataCodePointField = 0;
        static constexpr size_t kUnicodeDataDecompositionField = 5;


        // ====================================================================
        // splitFields
        //
        // Split one UnicodeData.txt record into exactly 15 fields.
        //
        // This parser does not use ucdReadField() because UnicodeData.txt
        // commonly ends with an empty fifteenth field:
        //
        //      ....;UPPER;LOWER;
        //
        // The final empty field is semantically real and must participate in
        // field-count validation.
        // ====================================================================

        static inline bool splitFields(const ByteSpan& source,
            std::array<ByteSpan, kUnicodeDataFieldCount>& outFields) noexcept
        {
            outFields = {};


            const uint8_t* fieldBegin =
                source.begin();

            const uint8_t* p =
                source.begin();

            const uint8_t* end =
                source.end();


            size_t fieldIndex = 0;


            while (p < end)
            {
                if (*p == ';')
                {
                    // --------------------------------------------------------
                    // Fifteen fields require exactly fourteen separators.
                    // --------------------------------------------------------

                    if (fieldIndex >=
                        kUnicodeDataFieldCount - 1)
                    {
                        return false;
                    }


                    outFields[fieldIndex] =
                        ByteSpan::fromPointers(
                            fieldBegin,
                            p);


                    bspan_trim_spaces(
                        outFields[fieldIndex]);


                    ++fieldIndex;

                    fieldBegin =
                        p + 1;
                }


                ++p;
            }


            // ---------------------------------------------------------------
            // Exactly fourteen semicolons must have been encountered.
            // ---------------------------------------------------------------

            if (fieldIndex !=
                kUnicodeDataFieldCount - 1)
            {
                return false;
            }


            // ---------------------------------------------------------------
            // Final field. It may legitimately be empty.
            // ---------------------------------------------------------------

            outFields[fieldIndex] =
                ByteSpan::fromPointers(
                    fieldBegin,
                    end);


            bspan_trim_spaces(
                outFields[fieldIndex]);


            return true;
        }


        // ====================================================================
        // isAsciiSpace
        // ====================================================================

        [[nodiscard]]
        static constexpr bool isAsciiSpace(uint8_t ch) noexcept
        {
            return
                ch == ' ' ||
                ch == '\t' ||
                ch == '\r' ||
                ch == '\n' ||
                ch == '\f' ||
                ch == '\v';
        }


        // ====================================================================
        // readToken
        //
        // Read one whitespace-delimited token.
        //
        // Used within the Decomposition_Mapping field, whose components are:
        //
        //      0041
        //
        //      0041 0300
        //
        // or:
        //
        //      <compat> 0041 0042 ...
        //
        // ====================================================================

        static inline bool readToken(
            ByteSpan& source,
            ByteSpan& outToken) noexcept
        {
            outToken.reset();

            bspan_trim_spaces(source);


            if (!source)
                return false;


            const uint8_t* begin =
                source.begin();

            const uint8_t* p =
                begin;

            const uint8_t* end =
                source.end();


            while (p < end &&
                !isAsciiSpace(*p))
            {
                ++p;
            }


            outToken =
                ByteSpan::fromPointers(
                    begin,
                    p);


            source =
                ByteSpan::fromPointers(
                    p,
                    end);


            bspan_trim_spaces(source);


            return
                static_cast<bool>(
                    outToken);
        }


        // ====================================================================
        // validateCompatibilityDecomposition
        //
        // Compatibility mappings begin with a decomposition-type tag:
        //
        //      <compat> 0061
        //      <font> 0041
        //      <fraction> 0031 2044 0032
        //
        // We do not store these yet, but malformed decomposition syntax should
        // still make generation fail rather than silently disappear.
        //
        // The exact compatibility tag is intentionally not enumerated here.
        // For canonical decomposition we care only that:
        //
        //      - a nonempty <...> tag exists
        //      - at least one valid Unicode code point follows
        //
        // ====================================================================

        static inline bool validateCompatibilityDecomposition(
            const ByteSpan& source) noexcept
        {
            ByteSpan field =
                source;


            bspan_trim_spaces(field);


            if (!field ||
                field.data()[0] != '<')
            {
                return false;
            }


            const uint8_t* begin =
                field.begin();

            const uint8_t* end =
                field.end();

            const uint8_t* close =
                nullptr;


            for (const uint8_t* p = begin + 1; p < end; ++p)
            {
                if (*p == '>')
                {
                    close = p;
                    break;
                }
            }


            // ---------------------------------------------------------------
            // Require:
            //
            //      <tag>
            //
            // with at least one byte in tag.
            // ---------------------------------------------------------------

            if (!close ||
                close == begin + 1)
            {
                return false;
            }


            // ---------------------------------------------------------------
            // The tag itself may not contain ASCII whitespace.
            // ---------------------------------------------------------------

            for (const uint8_t* p = begin + 1; p < close; ++p)
            {
                if (isAsciiSpace(*p))
                    return false;
            }


            // ---------------------------------------------------------------
            // Validate every mapping code point following the tag.
            // ---------------------------------------------------------------

            ByteSpan mapping =
                ByteSpan::fromPointers(
                    close + 1,
                    end);


            bspan_trim_spaces(mapping);


            if (!mapping)
                return false;


            size_t codePointCount = 0;


            while (mapping)
            {
                ByteSpan token;


                if (!readToken(
                    mapping,
                    token))
                {
                    return false;
                }


                uint32_t cp = 0;


                if (!ucdParseCodePoint(
                    token,
                    cp))
                {
                    return false;
                }


                ++codePointCount;
            }


            return codePointCount != 0;
        }


        // ====================================================================
        // CanonicalDecompositionParseResult
        // ====================================================================

        enum class CanonicalDecompositionParseResult : uint8_t
        {
            Success = 0,

            Invalid,
            TooLong
        };


        // ====================================================================
        // parseCanonicalDecomposition
        //
        // Parse the untagged canonical decomposition mapping.
        //
        // Unicode 17.0.0 contains only:
        //
        //      cp
        //
        // or:
        //
        //      cp cp
        //
        // for explicit canonical decomposition.
        //
        // We enforce that invariant here because it is fundamental to the
        // persistent UnicodeDecompositionRecord representation.
        //
        // If a future Unicode version introduces a longer canonical mapping,
        // generation fails explicitly instead of truncating the mapping.
        // ====================================================================

        static inline CanonicalDecompositionParseResult
            parseCanonicalDecomposition(
                const ByteSpan& source,
                uint32_t& outFirst,
                uint32_t& outSecond,
                uint32_t& outLength) noexcept
        {
            outFirst = 0;

            outSecond =
                kUnicodeDecompositionSecondNone;

            outLength = 0;


            ByteSpan mapping =
                source;


            bspan_trim_spaces(mapping);


            if (!mapping)
            {
                return
                    CanonicalDecompositionParseResult::Invalid;
            }


            // ---------------------------------------------------------------
            // First code point
            // ---------------------------------------------------------------

            ByteSpan firstField;


            if (!readToken(
                mapping,
                firstField))
            {
                return
                    CanonicalDecompositionParseResult::Invalid;
            }


            if (!ucdParseCodePoint(
                firstField,
                outFirst))
            {
                return
                    CanonicalDecompositionParseResult::Invalid;
            }


            outLength = 1;


            // ---------------------------------------------------------------
            // Singleton mapping.
            // ---------------------------------------------------------------

            if (!mapping)
            {
                return
                    CanonicalDecompositionParseResult::Success;
            }


            // ---------------------------------------------------------------
            // Second code point
            // ---------------------------------------------------------------

            ByteSpan secondField;


            if (!readToken(
                mapping,
                secondField))
            {
                return
                    CanonicalDecompositionParseResult::Invalid;
            }


            if (!ucdParseCodePoint(
                secondField,
                outSecond))
            {
                return
                    CanonicalDecompositionParseResult::Invalid;
            }


            outLength = 2;


            // ---------------------------------------------------------------
            // Anything remaining would require a representation longer than
            // UnicodeDecompositionRecord can express.
            // ---------------------------------------------------------------

            if (mapping)
            {
                return
                    CanonicalDecompositionParseResult::TooLong;
            }


            return
                CanonicalDecompositionParseResult::Success;
        }

    } // namespace ucd_unicode_data_detail


    // ========================================================================
    // ucdParseUnicodeData
    //
    // Parse canonical decomposition information from UnicodeData.txt.
    //
    //
    // UnicodeData.txt decomposition field:
    //
    //      empty
    //
    //          no decomposition
    //
    //
    //      0041 0300
    //
    //          canonical decomposition
    //
    //
    //      <compat> 0041
    //
    //          compatibility decomposition
    //
    //
    // Only untagged canonical mappings are added to:
    //
    //      UnicodeDecompositionBuilder
    //
    //
    // Compatibility mappings are syntactically validated and counted but are
    // intentionally not stored.
    //
    //
    // UnicodeData.txt also contains First/Last range-marker records, for
    // example Hangul and CJK algorithmic ranges. These records have empty
    // decomposition fields and therefore require no special treatment here.
    //
    // Hangul canonical decomposition will be handled algorithmically at
    // runtime rather than represented by explicit decomposition records.
    //
    //
    // On failure 'decompositions' may contain mappings produced before the
    // error. Generation should therefore be considered failed and the builder
    // discarded or cleared.
    //
    // ========================================================================

    static inline bool ucdParseUnicodeData(
        const ByteSpan& source,
        UnicodeDecompositionBuilder& decompositions,
        UCDUnicodeDataParseResult& outResult)
    {
        using namespace ucd_unicode_data_detail;


        outResult = {};


        // --------------------------------------------------------------------
        // Start from an empty canonical-decomposition map.
        // --------------------------------------------------------------------

        decompositions.clear();


        UCDParser parser(source);
        UCDLine line;


        bool havePreviousCodePoint = false;
        uint32_t previousCodePoint = 0;


        while (parser.next(line))
        {
            std::array<
                ByteSpan,
                kUnicodeDataFieldCount> fields;


            // ----------------------------------------------------------------
            // UnicodeData.txt has exactly fifteen semicolon-delimited fields.
            // ----------------------------------------------------------------

            if (!splitFields(
                line.data,
                fields))
            {
                outResult.error =
                    UCDUnicodeDataParseError::InvalidFieldCount;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Field 1: code point
            // ----------------------------------------------------------------

            uint32_t cp = 0;


            if (!ucdParseCodePoint(
                fields[kUnicodeDataCodePointField],
                cp))
            {
                outResult.error =
                    UCDUnicodeDataParseError::InvalidCodePoint;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // UnicodeData.txt records are ordered strictly by code point.
            //
            // This also detects duplicate source records.
            // ----------------------------------------------------------------

            if (havePreviousCodePoint &&
                cp <= previousCodePoint)
            {
                outResult.error =
                    UCDUnicodeDataParseError::InvalidCodePointOrder;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ----------------------------------------------------------------
            // Field 6: Decomposition_Mapping
            // ----------------------------------------------------------------

            ByteSpan decompositionField =
                fields[kUnicodeDataDecompositionField];


            if (decompositionField)
            {
                // ------------------------------------------------------------
                // Compatibility decomposition.
                //
                // Tagged mappings are deliberately not part of canonical
                // decomposition.
                // ------------------------------------------------------------

                if (decompositionField.data()[0] == '<')
                {
                    if (!validateCompatibilityDecomposition(
                        decompositionField))
                    {
                        outResult.error =
                            UCDUnicodeDataParseError::InvalidDecomposition;

                        outResult.lineNumber =
                            line.lineNumber;

                        return false;
                    }


                    ++outResult.compatibilityMappingCount;
                }

                // ------------------------------------------------------------
                // Canonical decomposition.
                // ------------------------------------------------------------

                else
                {
                    uint32_t first = 0;

                    uint32_t second =
                        kUnicodeDecompositionSecondNone;

                    uint32_t length = 0;


                    const CanonicalDecompositionParseResult parseResult =
                        parseCanonicalDecomposition(
                            decompositionField,
                            first,
                            second,
                            length);


                    if (parseResult ==
                        CanonicalDecompositionParseResult::Invalid)
                    {
                        outResult.error =
                            UCDUnicodeDataParseError::InvalidDecomposition;

                        outResult.lineNumber =
                            line.lineNumber;

                        return false;
                    }


                    if (parseResult ==
                        CanonicalDecompositionParseResult::TooLong)
                    {
                        outResult.error =
                            UCDUnicodeDataParseError::CanonicalDecompositionTooLong;

                        outResult.lineNumber =
                            line.lineNumber;

                        return false;
                    }


                    // --------------------------------------------------------
                    // Add the canonical mapping.
                    // --------------------------------------------------------

                    bool added = false;


                    if (length == 1)
                    {
                        added =
                            decompositions.addSingleton(
                                cp,
                                first);
                    }
                    else
                    {
                        added =
                            decompositions.addPair(
                                cp,
                                first,
                                second);
                    }


                    if (!added)
                    {
                        outResult.error =
                            UCDUnicodeDataParseError::DecompositionAddFailed;

                        outResult.lineNumber =
                            line.lineNumber;

                        return false;
                    }


                    ++outResult.canonicalMappingCount;


                    if (length == 1)
                    {
                        ++outResult.singletonMappingCount;
                    }
                    else
                    {
                        ++outResult.pairMappingCount;
                    }
                }
            }


            // ----------------------------------------------------------------
            // Complete successful record.
            // ----------------------------------------------------------------

            previousCodePoint =
                cp;

            havePreviousCodePoint =
                true;


            ++outResult.recordCount;
        }


        return true;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static inline bool ucdParseUnicodeData(
        const ByteSpan& source,
        UnicodeDecompositionBuilder& decompositions)
    {
        UCDUnicodeDataParseResult result;


        return
            ucdParseUnicodeData(
                source,
                decompositions,
                result);
    }

} // namespace waavs
