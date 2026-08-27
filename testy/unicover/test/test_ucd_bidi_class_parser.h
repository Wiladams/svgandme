// test_ucd_bidi_class_parser.cpp

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>

#include "test_core.h"

#include "ucd_bidi_class_parser.h"
#include "unicode_bidi_class.h"
#include "unicode_coverage_builder.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    struct UnicodeBidiClassTestMetadata
    {
        std::array<size_t, kUnicodeBidiClassCount> classTotals{};
        std::array<bool, kUnicodeBidiClassCount> classSections{};
        std::array<bool, kUnicodeBidiClassCount> classTotalSeen{};

        uint32_t classSectionCount{ 0 };
        uint32_t missingRangeCount{ 0 };
    };


    static bool unicodeBidiClassTestParseDecimal(
        const ByteSpan& source,
        size_t& outValue) noexcept
    {
        ByteSpan value = source;

        bspan_trim_spaces(value);

        if (!value)
            return false;


        size_t result = 0;


        while (value)
        {
            const uint8_t ch = *value;

            if (ch < '0' || ch > '9')
                return false;


            const size_t digit =
                static_cast<size_t>(ch - '0');


            if (result >
                (std::numeric_limits<size_t>::max() - digit) / 10u)
            {
                return false;
            }


            result =
                result * 10u +
                digit;

            ++value;
        }


        outValue = result;

        return true;
    }


    // ========================================================================
    // unicodeBidiClassTestReadMetadata
    //
    // Independently scan the source comments for:
    //
    //      # @missing:
    //
    // and:
    //
    //      # Bidi_Class=Left_To_Right
    //      ...
    //      # Total code points: 1095407
    //
    // This lets the test compare the complete generated table against the
    // cardinalities declared by the actual UCD source file.
    // ========================================================================

    static bool unicodeBidiClassTestReadMetadata(
        const ByteSpan& source,
        UnicodeBidiClassTestMetadata& outMetadata)
    {
        static constexpr char classPrefix[] =
            "Bidi_Class=";

        static constexpr char totalPrefix[] =
            "Total code points:";

        static constexpr char missingPrefix[] =
            "@missing:";

        static constexpr size_t classPrefixLength =
            sizeof(classPrefix) - 1u;

        static constexpr size_t totalPrefixLength =
            sizeof(totalPrefix) - 1u;


        outMetadata = {};


        ByteSpan remaining = source;

        uint32_t currentClass =
            kUnicodeBidiClassCount;


        while (remaining)
        {
            ByteSpan line =
                bspan_read_until(remaining, '\n');

            bspan_trim_spaces(line);


            if (!line || *line != '#')
                continue;


            ++line;
            bspan_ltrim_spaces(line);


            // ---------------------------------------------------------------
            // Count @missing records independently.
            // ---------------------------------------------------------------

            if (ucd_bidi_class_detail::spanStartsWith(
                line,
                missingPrefix))
            {
                ++outMetadata.missingRangeCount;
                continue;
            }


            // ---------------------------------------------------------------
            // Begin one Bidi_Class section.
            // ---------------------------------------------------------------

            if (ucd_bidi_class_detail::spanStartsWith(
                line,
                classPrefix))
            {
                line.advance(
                    classPrefixLength);

                bspan_trim_spaces(line);


                UnicodeBidiClass bidiClass;


                if (!ucd_bidi_class_detail::bidiClassFromField(
                    line,
                    bidiClass))
                {
                    return false;
                }


                const uint32_t index =
                    static_cast<uint32_t>(
                        bidiClass);


                if (index >=
                    kUnicodeBidiClassCount)
                {
                    return false;
                }


                if (outMetadata.classSections[index])
                    return false;


                outMetadata.classSections[index] =
                    true;

                ++outMetadata.classSectionCount;

                currentClass =
                    index;

                continue;
            }


            // ---------------------------------------------------------------
            // End of the current Bidi_Class section.
            // ---------------------------------------------------------------

            if (ucd_bidi_class_detail::spanStartsWith(
                line,
                totalPrefix))
            {
                if (currentClass >=
                    kUnicodeBidiClassCount)
                {
                    return false;
                }


                if (outMetadata.classTotalSeen[currentClass])
                    return false;


                line.advance(
                    totalPrefixLength);

                bspan_trim_spaces(line);


                size_t total = 0;


                if (!unicodeBidiClassTestParseDecimal(
                    line,
                    total))
                {
                    return false;
                }


                outMetadata.classTotals[currentClass] =
                    total;

                outMetadata.classTotalSeen[currentClass] =
                    true;

                currentClass =
                    kUnicodeBidiClassCount;
            }
        }


        if (outMetadata.classSectionCount !=
            kUnicodeBidiClassCount)
        {
            return false;
        }


        for (uint32_t i = 0;
            i < kUnicodeBidiClassCount;
            ++i)
        {
            if (!outMetadata.classSections[i] ||
                !outMetadata.classTotalSeen[i])
            {
                return false;
            }
        }


        return true;
    }


    // ========================================================================
    // testUnicodeBidiClassParser
    // ========================================================================

    static bool testUnicodeBidiClassParser(const ByteSpan& source)
    {
        auto fail = [](const char* message) -> bool {
            std::printf(
                "Unicode Bidi_Class parser: FAIL: %s\n",
                message);

            return false;
            };


        if (!source)
            return fail("source is empty");


        // ====================================================================
        // Read independent metadata from the actual UCD source
        // ====================================================================

        UnicodeBidiClassTestMetadata metadata;


        if (!unicodeBidiClassTestReadMetadata(
            source,
            metadata))
        {
            return fail(
                "unable to read Bidi_Class source metadata");
        }


        if (metadata.classSectionCount !=
            kUnicodeBidiClassCount)
        {
            return fail(
                "source does not contain all Bidi_Class sections");
        }


        if (metadata.missingRangeCount == 0)
            return fail("source contains no @missing records");


        // ====================================================================
        // Independently account explicit source ranges
        //
        // We also retain explicit coverage so the @missing spot checks can
        // prove that their selected code points really were supplied by the
        // default machinery rather than an explicit data record.
        // ====================================================================

        UnicodeCoverageBuilder explicitCoverage;

        uint32_t sourceRangeCount = 0;
        size_t sourceExplicitCodePoints = 0;


        {
            UCDParser parser(source);
            UCDLine line;


            while (parser.next(line))
            {
                ByteSpan fields =
                    line.data;

                UCDCodePointRange range;


                if (!ucdReadCodePointRange(
                    fields,
                    range))
                {
                    return fail(
                        "unable to independently parse explicit source range");
                }


                if (ucd_bidi_class_detail::coverageIntersectsRange(
                    explicitCoverage,
                    range.first,
                    range.last))
                {
                    return fail(
                        "explicit source ranges overlap");
                }


                explicitCoverage.addRange(
                    range.first,
                    range.last);


                ++sourceRangeCount;

                sourceExplicitCodePoints +=
                    static_cast<size_t>(
                        range.last -
                        range.first +
                        1u);
            }
        }


        // ====================================================================
        // Parse Bidi_Class
        //
        // UnicodeValueTable8Builder is about 1.1 MiB, so keep it off the stack.
        // ====================================================================

        auto values =
            std::make_unique<UnicodeValueTable8Builder>();

        UCDBidiClassParseResult result;


        if (!ucdParseBidiClass(
            source,
            *values,
            result))
        {
            std::printf(
                "Unicode Bidi_Class parser: FAIL\n"
                "  Error: %s\n"
                "  Line:  %u\n",
                ucdBidiClassParseErrorString(
                    result.error),
                result.lineNumber);

            return false;
        }


        // ====================================================================
        // Parser accounting
        // ====================================================================

        if (!result.success())
            return fail("parse result does not report success");


        if (result.lineNumber != 0)
            return fail("successful parse retained an error line number");


        if (result.rangeCount !=
            sourceRangeCount)
        {
            return fail(
                "explicit range count does not match source");
        }


        if (result.missingRangeCount !=
            metadata.missingRangeCount)
        {
            return fail(
                "@missing range count does not match source");
        }


        if (result.explicitCodePoints !=
            sourceExplicitCodePoints)
        {
            return fail(
                "explicit code-point count does not match source");
        }


        if (result.explicitCodePoints +
            result.defaultedCodePoints !=
            static_cast<size_t>(kUnicodeLimit))
        {
            return fail(
                "explicit plus defaulted code points do not cover Unicode");
        }


        // ====================================================================
        // Exhaustive Unicode address-space validation
        //
        // Count the actual population of every Bidi_Class value across all
        // 1,114,112 Unicode code-point positions.
        // ====================================================================

        std::array<size_t, kUnicodeBidiClassCount> actualClassTotals{};

        size_t unicodeCodePointsChecked = 0;


        for (uint32_t cp = 0;
            cp < kUnicodeLimit;
            ++cp)
        {
            const uint8_t value =
                values->value(cp);


            if (!unicodeBidiClassIsValid(value))
            {
                std::printf(
                    "Unicode Bidi_Class parser: FAIL\n"
                    "  Invalid value: %u\n"
                    "  Code point:   U+%04X\n",
                    value,
                    cp);

                return false;
            }


            ++actualClassTotals[value];
            ++unicodeCodePointsChecked;
        }


        if (unicodeCodePointsChecked !=
            static_cast<size_t>(kUnicodeLimit))
        {
            return fail(
                "Unicode address-space accounting failed");
        }


        // ====================================================================
        // Compare every class population against the counts declared directly
        // in DerivedBidiClass.txt.
        //
        // This is the strongest whole-table check in the test.
        // ====================================================================

        size_t metadataTotal = 0;


        for (uint32_t i = 0;
            i < kUnicodeBidiClassCount;
            ++i)
        {
            if (actualClassTotals[i] !=
                metadata.classTotals[i])
            {
                std::printf(
                    "Unicode Bidi_Class parser: FAIL\n"
                    "  Class value: %u\n"
                    "  Expected:    %zu\n"
                    "  Actual:      %zu\n",
                    i,
                    metadata.classTotals[i],
                    actualClassTotals[i]);

                return false;
            }


            if (actualClassTotals[i] == 0)
            {
                std::printf(
                    "Unicode Bidi_Class parser: FAIL\n"
                    "  Bidi class %u has zero population\n",
                    i);

                return false;
            }


            metadataTotal +=
                metadata.classTotals[i];
        }


        if (metadataTotal !=
            static_cast<size_t>(kUnicodeLimit))
        {
            return fail(
                "source Bidi_Class totals do not cover Unicode");
        }


        // ====================================================================
        // Explicit semantic spot checks
        //
        // Exercise every one of the 23 Bidi_Class values.
        // ====================================================================

        struct SpotCheck
        {
            uint32_t cp;
            UnicodeBidiClass expected;
        };


        static constexpr SpotCheck explicitChecks[] =
        {
            { 0x0041u, UnicodeBidiClass::LeftToRight },
            { 0x05D0u, UnicodeBidiClass::RightToLeft },
            { 0x0627u, UnicodeBidiClass::ArabicLetter },

            { 0x0030u, UnicodeBidiClass::EuropeanNumber },
            { 0x002Bu, UnicodeBidiClass::EuropeanSeparator },
            { 0x0024u, UnicodeBidiClass::EuropeanTerminator },
            { 0x0660u, UnicodeBidiClass::ArabicNumber },
            { 0x002Cu, UnicodeBidiClass::CommonSeparator },
            { 0x0301u, UnicodeBidiClass::NonspacingMark },
            { 0x0000u, UnicodeBidiClass::BoundaryNeutral },

            { 0x000Au, UnicodeBidiClass::ParagraphSeparator },
            { 0x0009u, UnicodeBidiClass::SegmentSeparator },
            { 0x0020u, UnicodeBidiClass::WhiteSpace },
            { 0x0021u, UnicodeBidiClass::OtherNeutral },

            { 0x202Au, UnicodeBidiClass::LeftToRightEmbedding },
            { 0x202Du, UnicodeBidiClass::LeftToRightOverride },
            { 0x202Bu, UnicodeBidiClass::RightToLeftEmbedding },
            { 0x202Eu, UnicodeBidiClass::RightToLeftOverride },
            { 0x202Cu, UnicodeBidiClass::PopDirectionalFormat },

            { 0x2066u, UnicodeBidiClass::LeftToRightIsolate },
            { 0x2067u, UnicodeBidiClass::RightToLeftIsolate },
            { 0x2068u, UnicodeBidiClass::FirstStrongIsolate },
            { 0x2069u, UnicodeBidiClass::PopDirectionalIsolate }
        };


        size_t explicitSpotChecks = 0;


        for (const SpotCheck& check : explicitChecks)
        {
            const UnicodeBidiClass actual =
                static_cast<UnicodeBidiClass>(
                    values->value(check.cp));


            if (actual !=
                check.expected)
            {
                std::printf(
                    "Unicode Bidi_Class parser: FAIL\n"
                    "  Explicit spot check: U+%04X\n"
                    "  Expected value:      %u\n"
                    "  Actual value:        %u\n",
                    check.cp,
                    static_cast<unsigned>(
                        check.expected),
                    static_cast<unsigned>(
                        actual));

                return false;
            }


            ++explicitSpotChecks;
        }


        // ====================================================================
        // @missing semantic spot checks
        //
        // Each selected code point must NOT appear in the explicit data.
        //
        // Therefore a correct value proves that the @missing pre-pass supplied
        // the result.
        //
        // These exercise:
        //
        //      broad L default
        //      R defaults
        //      AL defaults
        //      ET default
        // ====================================================================

        static constexpr SpotCheck missingChecks[] =
        {
            { 0x0378u, UnicodeBidiClass::LeftToRight },

            { 0x0590u, UnicodeBidiClass::RightToLeft },
            { 0xFB37u, UnicodeBidiClass::RightToLeft },
            { 0x1ECC0u, UnicodeBidiClass::RightToLeft },
            { 0x1EF00u, UnicodeBidiClass::RightToLeft },

            { 0x07B2u, UnicodeBidiClass::ArabicLetter },
            { 0x10D3Au, UnicodeBidiClass::ArabicLetter },

            { 0x20C2u, UnicodeBidiClass::EuropeanTerminator }
        };


        size_t missingSpotChecks = 0;


        for (const SpotCheck& check : missingChecks)
        {
            if (explicitCoverage.contains(
                check.cp))
            {
                std::printf(
                    "Unicode Bidi_Class parser: FAIL\n"
                    "  @missing fixture is explicitly assigned: U+%04X\n",
                    check.cp);

                return false;
            }


            const UnicodeBidiClass actual =
                static_cast<UnicodeBidiClass>(
                    values->value(check.cp));


            if (actual !=
                check.expected)
            {
                std::printf(
                    "Unicode Bidi_Class parser: FAIL\n"
                    "  @missing spot check: U+%04X\n"
                    "  Expected value:     %u\n"
                    "  Actual value:       %u\n",
                    check.cp,
                    static_cast<unsigned>(
                        check.expected),
                    static_cast<unsigned>(
                        actual));

                return false;
            }


            ++missingSpotChecks;
        }


        // ====================================================================
        // Failure-path checks
        //
        // Keep these small and synthetic. Their purpose is only to exercise
        // parser rejection paths; the successful parser test above always uses
        // the actual UCD source.
        // ====================================================================

        size_t failureChecks = 0;


        // --------------------------------------------------------------------
        // Missing full-range @missing default.
        // --------------------------------------------------------------------

        {
            static constexpr char text[] =
                "0041; L\n";

            UCDBidiClassParseResult failureResult;


            if (ucdParseBidiClass(
                ByteSpan(text),
                *values,
                failureResult) ||
                failureResult.error !=
                UCDBidiClassParseError::MissingDefault)
            {
                return fail(
                    "missing @missing default was not rejected");
            }


            ++failureChecks;
        }


        // --------------------------------------------------------------------
        // Unknown explicit Bidi_Class.
        // --------------------------------------------------------------------

        {
            static constexpr char text[] =
                "# @missing: 0000..10FFFF; Left_To_Right\n"
                "0041; XX\n";

            UCDBidiClassParseResult failureResult;


            if (ucdParseBidiClass(
                ByteSpan(text),
                *values,
                failureResult) ||
                failureResult.error !=
                UCDBidiClassParseError::UnknownClass)
            {
                return fail(
                    "unknown explicit Bidi_Class was not rejected");
            }


            ++failureChecks;
        }


        // --------------------------------------------------------------------
        // Overlapping explicit ranges.
        // --------------------------------------------------------------------

        {
            static constexpr char text[] =
                "# @missing: 0000..10FFFF; Left_To_Right\n"
                "0041..005A; L\n"
                "0050..0060; L\n";

            UCDBidiClassParseResult failureResult;


            if (ucdParseBidiClass(
                ByteSpan(text),
                *values,
                failureResult) ||
                failureResult.error !=
                UCDBidiClassParseError::OverlappingRange)
            {
                return fail(
                    "overlapping explicit ranges were not rejected");
            }


            ++failureChecks;
        }


        // --------------------------------------------------------------------
        // Unknown @missing Bidi_Class.
        // --------------------------------------------------------------------

        {
            static constexpr char text[] =
                "# @missing: 0000..10FFFF; Unknown_Bidi_Class\n";

            UCDBidiClassParseResult failureResult;


            if (ucdParseBidiClass(
                ByteSpan(text),
                *values,
                failureResult) ||
                failureResult.error !=
                UCDBidiClassParseError::UnknownDefaultClass)
            {
                return fail(
                    "unknown @missing Bidi_Class was not rejected");
            }


            ++failureChecks;
        }


        // ====================================================================
        // PASS
        // ====================================================================

        std::printf(
            "Unicode Bidi_Class parser: PASS\n"
            "  Source bytes:              %zu\n"
            "  Explicit ranges:           %u\n"
            "  @missing ranges:           %u\n"
            "  Explicit code points:      %zu\n"
            "  Defaulted code points:     %zu\n"
            "  Unicode code points:       %zu\n"
            "  Bidi classes:              %u\n"
            "  Class totals matched:      %u\n"
            "  Explicit spot checks:      %zu\n"
            "  @missing spot checks:      %zu\n"
            "  Failure-path checks:       %zu\n",
            source.size(),
            result.rangeCount,
            result.missingRangeCount,
            result.explicitCodePoints,
            result.defaultedCodePoints,
            unicodeCodePointsChecked,
            static_cast<unsigned>(
                kUnicodeBidiClassCount),
            static_cast<unsigned>(
                kUnicodeBidiClassCount),
            explicitSpotChecks,
            missingSpotChecks,
            failureChecks);


        return true;
    }

    static bool testUnicodeBidiClassParserFile(const char *filename)
    {
        std::vector<uint8_t> buffer;
        if (!readFile(filename, buffer))
            return false;

        return testUnicodeBidiClassParser(ByteSpan(buffer.data(), buffer.size()));
    }
} // namespace waavs
