// test_unicode_extended_pictographic_database.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "test_core.h"

#include "ucd_parser.h"
#include "unicode_database.h"


namespace waavs
{
    namespace test_extended_pictographic_detail
    {
        // ====================================================================
        // Source statistics
        // ====================================================================

        struct SourceStats
        {
            uint32_t rangeCount{ 0 };
            size_t codePoints{ 0 };
        };


        // ====================================================================
        // spanEquals
        // ====================================================================

        static bool spanEquals(const ByteSpan& span, const char* text) noexcept
        {
            const size_t length = std::strlen(text);

            return span.size() == length &&
                std::memcmp(span.data(), text, length) == 0;
        }


        // ====================================================================
        // parseExpected
        //
        // Independently extract Extended_Pictographic from emoji-data.txt.
        //
        // This deliberately does NOT use ucdParseExtendedPictographic().
        // ====================================================================

        static bool parseExpected(const ByteSpan& source, std::vector<uint8_t>& expected, SourceStats& stats)
        {
            expected.assign(kUnicodeLimit, 0);
            stats = {};

            UCDParser parser(source);
            UCDLine line;


            while (parser.next(line))
            {
                ByteSpan fields = line.data;
                UCDCodePointRange range;


                if (!ucdReadCodePointRange(fields, range))
                {
                    std::printf(
                        "Unicode Extended_Pictographic database: FAIL\n"
                        "  Invalid source range at line %u\n",
                        line.lineNumber);

                    return false;
                }


                ByteSpan propertyField;

                if (!ucdReadField(fields, propertyField) || !propertyField)
                {
                    std::printf(
                        "Unicode Extended_Pictographic database: FAIL\n"
                        "  Missing property field at line %u\n",
                        line.lineNumber);

                    return false;
                }


                bspan_trim_spaces(fields);

                if (fields)
                {
                    std::printf(
                        "Unicode Extended_Pictographic database: FAIL\n"
                        "  Unexpected extra field at line %u\n",
                        line.lineNumber);

                    return false;
                }


                if (!spanEquals(propertyField, "Extended_Pictographic"))
                    continue;


                ++stats.rangeCount;


                for (uint32_t cp = range.first;; ++cp)
                {
                    if (expected[cp] != 0)
                    {
                        std::printf(
                            "Unicode Extended_Pictographic database: FAIL\n"
                            "  Overlapping source range at U+%04X\n"
                            "  Line: %u\n",
                            cp,
                            line.lineNumber);

                        return false;
                    }


                    expected[cp] = 1;
                    ++stats.codePoints;


                    if (cp == range.last)
                        break;
                }
            }


            if (stats.rangeCount == 0)
            {
                std::printf(
                    "Unicode Extended_Pictographic database: FAIL\n"
                    "  Source contains no Extended_Pictographic ranges\n");

                return false;
            }


            return true;
        }


        // ====================================================================
        // Mismatch accumulator
        // ====================================================================

        struct MismatchStats
        {
            size_t sourceVsProperty{ 0 };
            size_t sourceVsCachedCoverage{ 0 };
            size_t sourceVsAccessor{ 0 };
            size_t propertyVsCachedCoverage{ 0 };
            size_t cachedCoverageVsAccessor{ 0 };

            uint32_t firstSourceVsProperty{ kUnicodeLimit };
            uint32_t firstSourceVsCachedCoverage{ kUnicodeLimit };
            uint32_t firstSourceVsAccessor{ kUnicodeLimit };
            uint32_t firstPropertyVsCachedCoverage{ kUnicodeLimit };
            uint32_t firstCachedCoverageVsAccessor{ kUnicodeLimit };

            [[nodiscard]]
            bool empty() const noexcept
            {
                return
                    sourceVsProperty == 0 &&
                    sourceVsCachedCoverage == 0 &&
                    sourceVsAccessor == 0 &&
                    propertyVsCachedCoverage == 0 &&
                    cachedCoverageVsAccessor == 0;
            }
        };


        static void recordMismatch(size_t& count, uint32_t& first, uint32_t cp) noexcept
        {
            if (count == 0)
                first = cp;

            ++count;
        }


        static void printMismatch(const char* description, size_t count, uint32_t first)
        {
            if (count == 0)
                return;

            std::printf(
                "  %-34s %zu\n"
                "    First mismatch: U+%04X\n",
                description,
                count,
                first);
        }

    } // namespace test_extended_pictographic_detail


    // ========================================================================
    // testUnicodeExtendedPictographicDatabase
    //
    // Exhaustively compare Unicode 17 emoji-data.txt against an already
    // generated .ucdb image.
    //
    // Verifies:
    //
    //   - database can be loaded
    //   - Unicode version is 17.0.0
    //   - Extended_Pictographic runtime capability is present
    //   - exactly one named property record exists
    //   - property source is EmojiData
    //   - persisted coverage cardinality matches source
    //   - raw propertyCoverage() matches source for every Unicode position
    //   - cached extendedPictographicCoverage() matches source
    //   - isExtendedPictographic() matches source
    //   - raw and cached representations agree
    //   - cached representation and scalar accessor agree
    //   - out-of-range lookup returns false
    //
    // The expected table is built independently of
    // ucdParseExtendedPictographic().
    // ========================================================================

    static bool testUnicodeExtendedPictographicDatabase(const ByteSpan& emojiDataSource, const ByteSpan& databaseImage)
    {
        using namespace test_extended_pictographic_detail;


        auto fail = [](const char* message) -> bool
            {
                std::printf(
                    "Unicode Extended_Pictographic database: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Build independent expected coverage from emoji-data.txt
        // ====================================================================

        std::vector<uint8_t> expected;
        SourceStats sourceStats;


        if (!parseExpected(emojiDataSource, expected, sourceStats))
            return false;


        // Unicode 17.0.0 invariant.

        if (sourceStats.codePoints != 2848)
        {
            std::printf(
                "Unicode Extended_Pictographic database: FAIL\n"
                "  Unexpected Unicode 17 Extended_Pictographic count\n"
                "  Expected: 2848\n"
                "  Actual:   %zu\n",
                sourceStats.codePoints);

            return false;
        }


        // ====================================================================
        // Load generated database
        // ====================================================================

        UnicodeDatabase database;


        if (!database.reset(databaseImage))
            return fail("unable to load generated Unicode database");


        if (database.unicodeMajor() != 17 ||
            database.unicodeMinor() != 0 ||
            database.unicodePatch() != 0)
        {
            std::printf(
                "Unicode Extended_Pictographic database: FAIL\n"
                "  Unexpected Unicode database version: %u.%u.%u\n",
                database.unicodeMajor(),
                database.unicodeMinor(),
                database.unicodePatch());

            return false;
        }


        if (!database.hasExtendedPictographic())
            return fail("database does not report Extended_Pictographic");


        // ====================================================================
        // Locate persistent binary-property directory record
        //
        // Do not assume it has any particular property index. Other binary
        // properties may be added to the database later.
        // ====================================================================

        uint32_t propertyMatchCount = 0;
        uint32_t extendedPictographicPropertyIndex = 0;
        const UnicodePropertyRecord* propertyRecord = nullptr;


        for (uint32_t i = 0; i < database.propertyCount(); ++i)
        {
            InternedKey name = database.propertyName(i);

            if (!name)
                return fail("binary property has invalid name");


            if (std::strcmp(name, "Extended_Pictographic") != 0)
                continue;


            ++propertyMatchCount;
            extendedPictographicPropertyIndex = i;
            propertyRecord = database.propertyRecord(i);
        }


        if (propertyMatchCount != 1)
        {
            std::printf(
                "Unicode Extended_Pictographic database: FAIL\n"
                "  Expected exactly one Extended_Pictographic property\n"
                "  Found: %u\n",
                propertyMatchCount);

            return false;
        }


        if (!propertyRecord)
            return fail("Extended_Pictographic property record is null");


        if (propertyRecord->source !=
            static_cast<uint16_t>(UnicodePropertySourceEmojiData))
        {
            std::printf(
                "Unicode Extended_Pictographic database: FAIL\n"
                "  Unexpected property source\n"
                "  Expected: %u\n"
                "  Actual:   %u\n",
                static_cast<unsigned>(
                    UnicodePropertySourceEmojiData),
                static_cast<unsigned>(
                    propertyRecord->source));

            return false;
        }


        // ====================================================================
        // Acquire both persistent views
        //
        // propertyCoverage():
        //     generic binary-property directory path
        //
        // extendedPictographicCoverage():
        //     cached semantic runtime path
        //
        // These should identify the same logical set.
        // ====================================================================

        const UnicodeCoverage propertyCoverage =
            database.propertyCoverage(
                extendedPictographicPropertyIndex);

        const UnicodeCoverage cachedCoverage =
            database.extendedPictographicCoverage();


        // ====================================================================
        // Coverage cardinality
        // ====================================================================

        const UnicodeCoverageStats propertyCoverageStats =
            propertyCoverage.stats();

        const UnicodeCoverageStats cachedCoverageStats =
            cachedCoverage.stats();


        if (propertyCoverageStats.coveredCodePoints != sourceStats.codePoints)
        {
            std::printf(
                "Unicode Extended_Pictographic database: FAIL\n"
                "  Generic property coverage count mismatch\n"
                "  Expected: %zu\n"
                "  Actual:   %zu\n",
                sourceStats.codePoints,
                static_cast<size_t>(
                    propertyCoverageStats.coveredCodePoints));

            return false;
        }


        if (cachedCoverageStats.coveredCodePoints != sourceStats.codePoints)
        {
            std::printf(
                "Unicode Extended_Pictographic database: FAIL\n"
                "  Cached coverage count mismatch\n"
                "  Expected: %zu\n"
                "  Actual:   %zu\n",
                sourceStats.codePoints,
                static_cast<size_t>(
                    cachedCoverageStats.coveredCodePoints));

            return false;
        }


        // ====================================================================
        // Semantic source spot checks
        //
        // These also make sure somebody has not accidentally pointed this test
        // at a source file with radically different semantics.
        // ====================================================================

        struct SpotCheck
        {
            uint32_t cp;
            bool expected;
            const char* description;
        };


        static constexpr SpotCheck spots[] =
        {
            { 0x0041, false, "LATIN CAPITAL LETTER A" },
            { 0x0301, false, "COMBINING ACUTE ACCENT" },
            { 0x1F600, true, "GRINNING FACE" }
        };


        size_t spotChecks = 0;


        for (const SpotCheck& spot : spots)
        {
            const bool sourceValue =
                expected[spot.cp] != 0;

            const bool runtimeValue =
                database.isExtendedPictographic(spot.cp);


            if (sourceValue != spot.expected)
            {
                std::printf(
                    "Unicode Extended_Pictographic database: FAIL\n"
                    "  Source spot check failed\n"
                    "  U+%04X %s\n"
                    "  Expected: %s\n"
                    "  Source:   %s\n",
                    spot.cp,
                    spot.description,
                    spot.expected ? "true" : "false",
                    sourceValue ? "true" : "false");

                return false;
            }


            if (runtimeValue != spot.expected)
            {
                std::printf(
                    "Unicode Extended_Pictographic database: FAIL\n"
                    "  Runtime spot check failed\n"
                    "  U+%04X %s\n"
                    "  Expected: %s\n"
                    "  Runtime:  %s\n",
                    spot.cp,
                    spot.description,
                    spot.expected ? "true" : "false",
                    runtimeValue ? "true" : "false");

                return false;
            }


            ++spotChecks;
        }


        // ====================================================================
        // Exhaustive Unicode comparison
        // ====================================================================

        MismatchStats mismatches;

        size_t expectedTrueCount = 0;
        size_t propertyTrueCount = 0;
        size_t cachedTrueCount = 0;
        size_t accessorTrueCount = 0;


        for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
        {
            const bool expectedValue =
                expected[cp] != 0;

            const bool propertyValue =
                propertyCoverage.contains(cp);

            const bool cachedValue =
                cachedCoverage.contains(cp);

            const bool accessorValue =
                database.isExtendedPictographic(cp);


            if (expectedValue)
                ++expectedTrueCount;

            if (propertyValue)
                ++propertyTrueCount;

            if (cachedValue)
                ++cachedTrueCount;

            if (accessorValue)
                ++accessorTrueCount;


            if (expectedValue != propertyValue)
            {
                recordMismatch(
                    mismatches.sourceVsProperty,
                    mismatches.firstSourceVsProperty,
                    cp);
            }


            if (expectedValue != cachedValue)
            {
                recordMismatch(
                    mismatches.sourceVsCachedCoverage,
                    mismatches.firstSourceVsCachedCoverage,
                    cp);
            }


            if (expectedValue != accessorValue)
            {
                recordMismatch(
                    mismatches.sourceVsAccessor,
                    mismatches.firstSourceVsAccessor,
                    cp);
            }


            if (propertyValue != cachedValue)
            {
                recordMismatch(
                    mismatches.propertyVsCachedCoverage,
                    mismatches.firstPropertyVsCachedCoverage,
                    cp);
            }


            if (cachedValue != accessorValue)
            {
                recordMismatch(
                    mismatches.cachedCoverageVsAccessor,
                    mismatches.firstCachedCoverageVsAccessor,
                    cp);
            }
        }


        if (!mismatches.empty())
        {
            std::printf(
                "Unicode Extended_Pictographic database: FAIL\n"
                "  Exhaustive comparison detected mismatches\n");


            printMismatch(
                "Source vs property coverage:",
                mismatches.sourceVsProperty,
                mismatches.firstSourceVsProperty);

            printMismatch(
                "Source vs cached coverage:",
                mismatches.sourceVsCachedCoverage,
                mismatches.firstSourceVsCachedCoverage);

            printMismatch(
                "Source vs runtime accessor:",
                mismatches.sourceVsAccessor,
                mismatches.firstSourceVsAccessor);

            printMismatch(
                "Property vs cached coverage:",
                mismatches.propertyVsCachedCoverage,
                mismatches.firstPropertyVsCachedCoverage);

            printMismatch(
                "Cached coverage vs accessor:",
                mismatches.cachedCoverageVsAccessor,
                mismatches.firstCachedCoverageVsAccessor);


            std::printf(
                "  Expected true count:            %zu\n"
                "  Property coverage true count:   %zu\n"
                "  Cached coverage true count:     %zu\n"
                "  Runtime accessor true count:    %zu\n",
                expectedTrueCount,
                propertyTrueCount,
                cachedTrueCount,
                accessorTrueCount);


            return false;
        }


        // ====================================================================
        // Independent count agreement
        // ====================================================================

        if (expectedTrueCount != sourceStats.codePoints)
            return fail("expected table count does not match source parser count");

        if (propertyTrueCount != sourceStats.codePoints)
            return fail("property coverage count does not match source count");

        if (cachedTrueCount != sourceStats.codePoints)
            return fail("cached coverage count does not match source count");

        if (accessorTrueCount != sourceStats.codePoints)
            return fail("runtime accessor count does not match source count");


        // ====================================================================
        // Out-of-range behavior
        // ====================================================================

        if (propertyCoverage.contains(kUnicodeLimit))
            return fail("generic property coverage accepts out-of-range code point");

        if (cachedCoverage.contains(kUnicodeLimit))
            return fail("cached coverage accepts out-of-range code point");

        if (database.isExtendedPictographic(kUnicodeLimit))
            return fail("runtime accessor accepts out-of-range code point");


        // ====================================================================
        // PASS diagnostics
        // ====================================================================

        std::printf(
            "Unicode Extended_Pictographic database: PASS\n"
            "  Source bytes:               %zu\n"
            "  Database bytes:             %zu\n"
            "  Source ranges:              %u\n"
            "  Source code points:         %zu\n"
            "  Persisted code points:      %zu\n"
            "  Binary properties:          %u\n"
            "  Property directory matches: %u\n"
            "  Property source:            EmojiData\n"
            "  Semantic spot checks:       %zu\n"
            "  Unicode code points:        %u\n"
            "  Generic coverage compare:   PASS\n"
            "  Cached coverage compare:    PASS\n"
            "  Runtime accessor compare:   PASS\n"
            "  Out-of-range behavior:      PASS\n"
            "  Full coverage comparison:   PASS\n",
            emojiDataSource.size(),
            databaseImage.size(),
            sourceStats.rangeCount,
            sourceStats.codePoints,
            static_cast<size_t>(
                propertyCoverageStats.coveredCodePoints),
            database.propertyCount(),
            propertyMatchCount,
            spotChecks,
            static_cast<unsigned>(kUnicodeLimit));


        return true;
    }


    // ========================================================================
    // Filename convenience wrapper
    // ========================================================================

    static bool testUnicodeExtendedPictographicDatabase(const char* emojiDataFilename, const char* databaseFilename)
    {
        std::vector<uint8_t> emojiData;

        if (!readFileData(emojiDataFilename, emojiData))
        {
            std::printf(
                "Unicode Extended_Pictographic database: FAIL\n"
                "  Unable to read source file: %s\n",
                emojiDataFilename);

            return false;
        }


        std::vector<uint8_t> databaseData;

        if (!readFileData(databaseFilename, databaseData))
        {
            std::printf(
                "Unicode Extended_Pictographic database: FAIL\n"
                "  Unable to read database file: %s\n",
                databaseFilename);

            return false;
        }


        const ByteSpan emojiDataSource(
            emojiData.data(),
            emojiData.size());

        const ByteSpan databaseImage(
            databaseData.data(),
            databaseData.size());


        return testUnicodeExtendedPictographicDatabase(
            emojiDataSource, databaseImage);
    }

} // namespace waavs
