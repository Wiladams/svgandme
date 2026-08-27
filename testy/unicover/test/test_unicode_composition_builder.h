// test_unicode_composition_builder.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

#include "ucd_normalization_props_parser.h"
#include "ucd_parser.h"
#include "unicode_composition_builder.h"


namespace waavs
{
    // ========================================================================
    // unicodeCompositionTestLoadFile
    // ========================================================================

    static bool unicodeCompositionTestLoadFile(
        const char* filename,
        std::vector<uint8_t>& outBytes)
    {
        outBytes.clear();

        std::ifstream input(
            filename,
            std::ios::binary |
            std::ios::ate);

        if (!input)
            return false;

        const std::streampos end = input.tellg();

        if (end <= 0)
            return false;

        outBytes.resize(
            static_cast<size_t>(end));

        input.seekg(
            0,
            std::ios::beg);

        input.read(
            reinterpret_cast<char*>(outBytes.data()),
            static_cast<std::streamsize>(outBytes.size()));

        return static_cast<bool>(input);
    }


    // ========================================================================
    // unicodeCompositionTestParseDecomposition
    //
    // Parse the UnicodeData.txt decomposition field.
    //
    // Examples:
    //
    //      ""
    //
    //          no decomposition
    //
    //      "0041 0300"
    //
    //          canonical pair
    //
    //      "212B"
    //
    //          canonical singleton
    //
    //      "<compat> 0020"
    //
    //          compatibility decomposition
    //
    //
    // For canonical decomposition, Unicode 17 contains only singleton and
    // pair mappings. Anything longer is rejected by this test helper.
    // ========================================================================

    static bool unicodeCompositionTestParseDecomposition(
        const ByteSpan& source,
        bool& outCompatibility,
        uint32_t& outFirst,
        uint32_t& outSecond,
        uint32_t& outCount) noexcept
    {
        outCompatibility = false;
        outFirst = 0;
        outSecond = 0;
        outCount = 0;

        ByteSpan mapping = source;

        bspan_trim_spaces(mapping);

        if (!mapping)
            return true;


        // --------------------------------------------------------------------
        // Compatibility decompositions begin with a tag:
        //
        //      <compat>
        //      <font>
        //      <circle>
        //      ...
        //
        // We do not need to parse the mapping itself.
        // --------------------------------------------------------------------

        if (*mapping == '<')
        {
            outCompatibility = true;
            return true;
        }


        // --------------------------------------------------------------------
        // Parse canonical decomposition code points.
        // --------------------------------------------------------------------

        while (mapping)
        {
            bspan_trim_spaces(mapping);

            if (!mapping)
                break;


            const uint8_t* tokenStart = mapping.begin();
            const uint8_t* p = tokenStart;
            const uint8_t* end = mapping.end();


            while (p < end &&
                *p != ' ' &&
                *p != '\t')
            {
                ++p;
            }


            ByteSpan token =
                ByteSpan::fromPointers(
                    tokenStart,
                    p);


            uint32_t cp = 0;

            if (!ucdParseCodePoint(
                token,
                cp))
            {
                return false;
            }


            if (outCount == 0)
            {
                outFirst = cp;
            }
            else if (outCount == 1)
            {
                outSecond = cp;
            }
            else
            {
                // Canonical decomposition longer than two code points would
                // invalidate the representation assumptions being tested.
                return false;
            }


            ++outCount;

            mapping.resetStart(p);
        }


        return outCount == 1 ||
            outCount == 2;
    }


    // ========================================================================
    // unicodeCompositionTestFind
    //
    // Binary search the finalized composition table.
    // ========================================================================

    static const UnicodeCompositionRecord* unicodeCompositionTestFind(
        const std::vector<UnicodeCompositionRecord>& records,
        uint32_t first,
        uint32_t second) noexcept
    {
        size_t lo = 0;
        size_t hi = records.size();


        while (lo < hi)
        {
            const size_t mid =
                lo + (hi - lo) / 2;

            const UnicodeCompositionRecord& record =
                records[mid];


            if (record.first < first ||
                (record.first == first &&
                    record.second < second))
            {
                lo = mid + 1;
            }
            else
            {
                hi = mid;
            }
        }


        if (lo >= records.size())
            return nullptr;


        const UnicodeCompositionRecord& record =
            records[lo];


        if (record.first != first ||
            record.second != second)
        {
            return nullptr;
        }


        return &record;
    }


    // ========================================================================
    // unicodeCompositionTestIsStrictlySorted
    // ========================================================================

    static bool unicodeCompositionTestIsStrictlySorted(
        const std::vector<UnicodeCompositionRecord>& records) noexcept
    {
        for (size_t i = 1; i < records.size(); ++i)
        {
            const UnicodeCompositionRecord& previous =
                records[i - 1];

            const UnicodeCompositionRecord& current =
                records[i];


            if (previous.first < current.first)
                continue;


            if (previous.first == current.first &&
                previous.second < current.second)
            {
                continue;
            }


            return false;
        }


        return true;
    }


    // ========================================================================
    // testUnicodeCompositionBuilder
    // ========================================================================

    static bool testUnicodeCompositionBuilder(
        const char* unicodeDataFilename,
        const char* normalizationPropsFilename)
    {
        auto fail = [](const char* message) -> bool {
            std::printf(
                "Unicode composition builder: FAIL: %s\n",
                message);

            return false;
            };


        // ====================================================================
        // Synthetic test: ordinary construction + exclusion + sorting
        // ====================================================================

        {
            UnicodeCoverageBuilder exclusions;

            // U+0344 is a Full_Composition_Exclusion character.
            exclusions.add(0x0344);


            UnicodeCompositionBuilder builder;

            // Deliberately add these out of sorted order.
            if (!builder.addCanonicalPair(
                0x00E9,
                0x0065,
                0x0301,
                exclusions))
            {
                return fail("synthetic add U+00E9 failed");
            }


            if (!builder.addCanonicalPair(
                0x00C5,
                0x0041,
                0x030A,
                exclusions))
            {
                return fail("synthetic add U+00C5 failed");
            }


            if (!builder.addCanonicalPair(
                0x0344,
                0x0308,
                0x0301,
                exclusions))
            {
                return fail("synthetic excluded add failed");
            }


            if (!builder.addCanonicalPair(
                0x00C0,
                0x0041,
                0x0300,
                exclusions))
            {
                return fail("synthetic add U+00C0 failed");
            }


            std::vector<UnicodeCompositionRecord> records;
            UnicodeCompositionBuildResult result;


            if (!builder.finalize(
                records,
                result))
            {
                return fail("synthetic finalize failed");
            }


            if (result.candidatePairCount != 4)
                return fail("synthetic candidate count incorrect");

            if (result.excludedPairCount != 1)
                return fail("synthetic exclusion count incorrect");

            if (result.compositionPairCount != 3)
                return fail("synthetic composition count incorrect");

            if (records.size() != 3)
                return fail("synthetic record count incorrect");


            if (!unicodeCompositionTestIsStrictlySorted(
                records))
            {
                return fail("synthetic records are not sorted");
            }


            const UnicodeCompositionRecord* record = nullptr;


            record = unicodeCompositionTestFind(
                records,
                0x0041,
                0x0300);

            if (!record ||
                record->composite != 0x00C0)
            {
                return fail("synthetic U+0041 + U+0300 lookup failed");
            }


            record = unicodeCompositionTestFind(
                records,
                0x0041,
                0x030A);

            if (!record ||
                record->composite != 0x00C5)
            {
                return fail("synthetic U+0041 + U+030A lookup failed");
            }


            record = unicodeCompositionTestFind(
                records,
                0x0065,
                0x0301);

            if (!record ||
                record->composite != 0x00E9)
            {
                return fail("synthetic U+0065 + U+0301 lookup failed");
            }


            if (unicodeCompositionTestFind(
                records,
                0x0308,
                0x0301))
            {
                return fail("excluded U+0344 composition was emitted");
            }
        }


        // ====================================================================
        // Synthetic test: exclusion applies to composite, not components
        // ====================================================================

        {
            UnicodeCoverageBuilder exclusions;

            // Exclude the first component, but not the composite.
            exclusions.add(0x0041);


            UnicodeCompositionBuilder builder;


            if (!builder.addCanonicalPair(
                0x00C0,
                0x0041,
                0x0300,
                exclusions))
            {
                return fail("component exclusion test add failed");
            }


            std::vector<UnicodeCompositionRecord> records;
            UnicodeCompositionBuildResult result;


            if (!builder.finalize(
                records,
                result))
            {
                return fail("component exclusion test finalize failed");
            }


            if (records.size() != 1 ||
                result.excludedPairCount != 0)
            {
                return fail(
                    "builder incorrectly applied exclusion to decomposition component");
            }
        }


        // ====================================================================
        // Synthetic test: duplicate pair
        //
        //      A + grave -> A-grave
        //      A + grave -> A-grave
        //
        // Exact duplicate input is malformed.
        // ====================================================================

        {
            UnicodeCoverageBuilder exclusions;
            UnicodeCompositionBuilder builder;


            if (!builder.addCanonicalPair(
                0x00C0,
                0x0041,
                0x0300,
                exclusions))
            {
                return fail("duplicate test first add failed");
            }


            if (!builder.addCanonicalPair(
                0x00C0,
                0x0041,
                0x0300,
                exclusions))
            {
                return fail("duplicate test second add unexpectedly failed early");
            }


            std::vector<UnicodeCompositionRecord> output{
                { 0x1111, 0x2222, 0x3333 }
            };

            UnicodeCompositionBuildResult result;


            if (builder.finalize(
                output,
                result))
            {
                return fail("duplicate pair unexpectedly accepted");
            }


            if (result.error !=
                UnicodeCompositionBuildError::DuplicatePair)
            {
                return fail("duplicate pair returned wrong error");
            }


            if (result.first != 0x0041 ||
                result.second != 0x0300 ||
                result.existingComposite != 0x00C0 ||
                result.incomingComposite != 0x00C0)
            {
                return fail("duplicate pair diagnostics incorrect");
            }


            // finalize() is transactional.
            if (output.size() != 1 ||
                output[0].first != 0x1111 ||
                output[0].second != 0x2222 ||
                output[0].composite != 0x3333)
            {
                return fail("duplicate failure modified output records");
            }
        }


        // ====================================================================
        // Synthetic test: conflicting pair
        //
        //      A + grave -> X
        //      A + grave -> Y
        // ====================================================================

        {
            UnicodeCoverageBuilder exclusions;
            UnicodeCompositionBuilder builder;


            if (!builder.addCanonicalPair(
                0x00C0,
                0x0041,
                0x0300,
                exclusions))
            {
                return fail("conflict test first add failed");
            }


            if (!builder.addCanonicalPair(
                0x1234,
                0x0041,
                0x0300,
                exclusions))
            {
                return fail("conflict test second add unexpectedly failed early");
            }


            std::vector<UnicodeCompositionRecord> records;
            UnicodeCompositionBuildResult result;


            if (builder.finalize(
                records,
                result))
            {
                return fail("conflicting pair unexpectedly accepted");
            }


            if (result.error !=
                UnicodeCompositionBuildError::ConflictingPair)
            {
                return fail("conflicting pair returned wrong error");
            }


            if (result.first != 0x0041 ||
                result.second != 0x0300)
            {
                return fail("conflicting pair diagnostics incorrect");
            }


            if (result.existingComposite ==
                result.incomingComposite)
            {
                return fail("conflicting pair did not report distinct composites");
            }
        }


        // ====================================================================
        // Synthetic test: invalid code point
        // ====================================================================

        {
            UnicodeCoverageBuilder exclusions;
            UnicodeCompositionBuilder builder;


            if (builder.addCanonicalPair(
                kUnicodeLimit,
                0x0041,
                0x0300,
                exclusions))
            {
                return fail("invalid composite unexpectedly accepted");
            }


            if (builder.error() !=
                UnicodeCompositionBuildError::InvalidCodePoint)
            {
                return fail("invalid composite returned wrong builder error");
            }


            std::vector<UnicodeCompositionRecord> records;
            UnicodeCompositionBuildResult result;


            if (builder.finalize(
                records,
                result))
            {
                return fail("failed builder unexpectedly finalized");
            }


            if (result.error !=
                UnicodeCompositionBuildError::InvalidCodePoint)
            {
                return fail("invalid code-point finalize returned wrong error");
            }
        }


        // ====================================================================
        // Load real Unicode 17 source files
        // ====================================================================

        std::vector<uint8_t> unicodeDataBytes;
        std::vector<uint8_t> normalizationPropsBytes;


        if (!unicodeCompositionTestLoadFile(
            unicodeDataFilename,
            unicodeDataBytes))
        {
            return fail("unable to read UnicodeData.txt");
        }


        if (!unicodeCompositionTestLoadFile(
            normalizationPropsFilename,
            normalizationPropsBytes))
        {
            return fail("unable to read DerivedNormalizationProps.txt");
        }


        const ByteSpan unicodeDataSource(
            unicodeDataBytes.data(),
            unicodeDataBytes.size());

        const ByteSpan normalizationPropsSource(
            normalizationPropsBytes.data(),
            normalizationPropsBytes.size());


        // ====================================================================
        // Parse Full_Composition_Exclusion
        // ====================================================================

        UnicodeCoverageBuilder exclusions;
        UCDNormalizationPropsParseResult normalizationResult;


        if (!ucdParseFullCompositionExclusion(
            normalizationPropsSource,
            exclusions,
            normalizationResult))
        {
            std::printf(
                "Unicode composition builder: FAIL\n"
                "  DerivedNormalizationProps.txt line: %u\n"
                "  Error: %s\n",
                normalizationResult.lineNumber,
                ucdNormalizationPropsParseErrorString(
                    normalizationResult.error));

            return false;
        }


        if (normalizationResult.rangeCount != 74)
            return fail("unexpected Full_Composition_Exclusion range count");

        if (normalizationResult.codePointCount != 1120)
            return fail("unexpected Full_Composition_Exclusion code-point count");


        // ====================================================================
        // Parse UnicodeData.txt direct decomposition mappings.
        //
        // UnicodeData.txt field order:
        //
        //      1   code point
        //      2   name
        //      3   general category
        //      4   canonical combining class
        //      5   bidi class
        //      6   decomposition mapping
        //
        // We need only fields 1 and 6.
        // ====================================================================

        UnicodeCompositionBuilder builder;

        // We know Unicode 17 is around one thousand canonical pairs.
        builder.reserve(1100);


        size_t unicodeDataRecordCount = 0;
        size_t canonicalMappingCount = 0;
        size_t canonicalSingletonCount = 0;
        size_t canonicalPairCount = 0;
        size_t compatibilityMappingCount = 0;


        UCDParser parser(
            unicodeDataSource);

        UCDLine line;


        while (parser.next(line))
        {
            ByteSpan fields =
                line.data;


            ++unicodeDataRecordCount;


            // ----------------------------------------------------------------
            // Field 1: composite code point
            // ----------------------------------------------------------------

            ByteSpan codePointField;


            if (!ucdReadField(
                fields,
                codePointField))
            {
                std::printf(
                    "Unicode composition builder: FAIL\n"
                    "  UnicodeData.txt line: %u\n"
                    "  Error: missing code-point field\n",
                    line.lineNumber);

                return false;
            }


            uint32_t composite = 0;


            if (!ucdParseCodePoint(
                codePointField,
                composite))
            {
                std::printf(
                    "Unicode composition builder: FAIL\n"
                    "  UnicodeData.txt line: %u\n"
                    "  Error: invalid code point\n",
                    line.lineNumber);

                return false;
            }


            // ----------------------------------------------------------------
            // Fields 2 through 5 are not needed here.
            // ----------------------------------------------------------------

            for (uint32_t fieldIndex = 0;
                fieldIndex < 4;
                ++fieldIndex)
            {
                ByteSpan ignored;


                if (!ucdReadField(
                    fields,
                    ignored))
                {
                    std::printf(
                        "Unicode composition builder: FAIL\n"
                        "  UnicodeData.txt line: %u\n"
                        "  Error: missing field before decomposition mapping\n",
                        line.lineNumber);

                    return false;
                }
            }


            // ----------------------------------------------------------------
            // Field 6: decomposition mapping
            // ----------------------------------------------------------------

            ByteSpan decompositionField;


            if (!ucdReadField(
                fields,
                decompositionField))
            {
                std::printf(
                    "Unicode composition builder: FAIL\n"
                    "  UnicodeData.txt line: %u\n"
                    "  Error: missing decomposition field\n",
                    line.lineNumber);

                return false;
            }


            bool compatibility = false;
            uint32_t first = 0;
            uint32_t second = 0;
            uint32_t decompositionCount = 0;


            if (!unicodeCompositionTestParseDecomposition(
                decompositionField,
                compatibility,
                first,
                second,
                decompositionCount))
            {
                std::printf(
                    "Unicode composition builder: FAIL\n"
                    "  UnicodeData.txt line: %u\n"
                    "  Composite: U+%04X\n"
                    "  Error: invalid canonical decomposition mapping\n",
                    line.lineNumber,
                    composite);

                return false;
            }


            // No decomposition.
            if (!decompositionField)
                continue;


            // Compatibility decomposition.
            if (compatibility)
            {
                ++compatibilityMappingCount;
                continue;
            }


            ++canonicalMappingCount;


            // Canonical singleton mappings never become pair-composition
            // records.
            if (decompositionCount == 1)
            {
                ++canonicalSingletonCount;
                continue;
            }


            if (decompositionCount != 2)
                return fail("canonical decomposition is neither singleton nor pair");


            ++canonicalPairCount;


            // ----------------------------------------------------------------
            // Reverse:
            //
            //      composite -> first second
            //
            // into:
            //
            //      first + second -> composite
            // ----------------------------------------------------------------

            if (!builder.addCanonicalPair(
                composite,
                first,
                second,
                exclusions))
            {
                std::printf(
                    "Unicode composition builder: FAIL\n"
                    "  UnicodeData.txt line: %u\n"
                    "  Mapping: U+%04X + U+%04X -> U+%04X\n"
                    "  Error: %s\n",
                    line.lineNumber,
                    first,
                    second,
                    composite,
                    unicodeCompositionBuildErrorString(
                        builder.error()));

                return false;
            }
        }


        // ====================================================================
        // Verify UnicodeData.txt structure already established by our
        // decomposition work.
        // ====================================================================

        if (unicodeDataRecordCount != 40575)
        {
            std::printf(
                "Unicode composition builder: FAIL\n"
                "  Expected UnicodeData records: 40575\n"
                "  Actual UnicodeData records:   %zu\n",
                unicodeDataRecordCount);

            return false;
        }


        if (canonicalMappingCount != 2081)
        {
            std::printf(
                "Unicode composition builder: FAIL\n"
                "  Expected canonical mappings: 2081\n"
                "  Actual canonical mappings:   %zu\n",
                canonicalMappingCount);

            return false;
        }


        if (canonicalSingletonCount != 1035)
        {
            std::printf(
                "Unicode composition builder: FAIL\n"
                "  Expected canonical singletons: 1035\n"
                "  Actual canonical singletons:   %zu\n",
                canonicalSingletonCount);

            return false;
        }


        if (canonicalPairCount != 1046)
        {
            std::printf(
                "Unicode composition builder: FAIL\n"
                "  Expected canonical pairs: 1046\n"
                "  Actual canonical pairs:   %zu\n",
                canonicalPairCount);

            return false;
        }


        if (compatibilityMappingCount != 3833)
        {
            std::printf(
                "Unicode composition builder: FAIL\n"
                "  Expected compatibility mappings: 3833\n"
                "  Actual compatibility mappings:   %zu\n",
                compatibilityMappingCount);

            return false;
        }


        // ====================================================================
        // Finalize real Unicode composition table
        // ====================================================================

        std::vector<UnicodeCompositionRecord> records;
        UnicodeCompositionBuildResult buildResult;


        if (!builder.finalize(
            records,
            buildResult))
        {
            std::printf(
                "Unicode composition builder: FAIL\n"
                "  Error: %s\n"
                "  Pair: U+%04X + U+%04X\n"
                "  Existing composite: U+%04X\n"
                "  Incoming composite: U+%04X\n",
                unicodeCompositionBuildErrorString(
                    buildResult.error),
                buildResult.first,
                buildResult.second,
                buildResult.existingComposite,
                buildResult.incomingComposite);

            return false;
        }


        // ====================================================================
        // Unicode 17 composition counts
        // ====================================================================

        if (buildResult.candidatePairCount != 1046)
            return fail("composition candidate count is not 1046");

        if (buildResult.excludedPairCount != 85)
            return fail("excluded canonical pair count is not 85");

        if (buildResult.compositionPairCount != 961)
            return fail("composition record count is not 961");

        if (records.size() != 961)
            return fail("final record array does not contain 961 records");


        // ====================================================================
        // Persistent ordering invariant
        // ====================================================================

        if (!unicodeCompositionTestIsStrictlySorted(
            records))
        {
            return fail("real composition records are not strictly sorted");
        }


        // ====================================================================
        // Every emitted composite must be outside
        // Full_Composition_Exclusion.
        // ====================================================================

        for (const UnicodeCompositionRecord& record : records)
        {
            if (record.first >= kUnicodeLimit ||
                record.second >= kUnicodeLimit ||
                record.composite >= kUnicodeLimit)
            {
                return fail("final record contains invalid code point");
            }


            if (exclusions.contains(
                record.composite))
            {
                std::printf(
                    "Unicode composition builder: FAIL\n"
                    "  Excluded composite emitted: U+%04X\n"
                    "  Pair: U+%04X + U+%04X\n",
                    record.composite,
                    record.first,
                    record.second);

                return false;
            }
        }


        // ====================================================================
        // Known positive composition mappings
        // ====================================================================

        struct ExpectedComposition
        {
            uint32_t first;
            uint32_t second;
            uint32_t composite;
        };


        const ExpectedComposition expected[] = {
            { 0x0041, 0x0300, 0x00C0 },
            { 0x0041, 0x0301, 0x00C1 },
            { 0x0041, 0x030A, 0x00C5 },
            { 0x0065, 0x0301, 0x00E9 },
            { 0x00C5, 0x0301, 0x01FA }
        };


        for (const ExpectedComposition& item : expected)
        {
            const UnicodeCompositionRecord* record =
                unicodeCompositionTestFind(
                    records,
                    item.first,
                    item.second);


            if (!record)
            {
                std::printf(
                    "Unicode composition builder: FAIL\n"
                    "  Missing composition: U+%04X + U+%04X -> U+%04X\n",
                    item.first,
                    item.second,
                    item.composite);

                return false;
            }


            if (record->composite !=
                item.composite)
            {
                std::printf(
                    "Unicode composition builder: FAIL\n"
                    "  Incorrect composition: U+%04X + U+%04X\n"
                    "  Expected: U+%04X\n"
                    "  Actual:   U+%04X\n",
                    item.first,
                    item.second,
                    item.composite,
                    record->composite);

                return false;
            }
        }


        // ====================================================================
        // Known excluded pair mappings
        //
        // U+0344:
        //
        //      0308 0301
        //
        // U+0958:
        //
        //      0915 093C
        //
        // Both have canonical pair decompositions but must not be reconstructed
        // during canonical composition.
        // ====================================================================

        if (!exclusions.contains(0x0344))
            return fail("U+0344 is unexpectedly absent from exclusions");

        if (!exclusions.contains(0x0958))
            return fail("U+0958 is unexpectedly absent from exclusions");


        if (unicodeCompositionTestFind(
            records,
            0x0308,
            0x0301))
        {
            return fail(
                "excluded U+0308 + U+0301 composition was emitted");
        }


        if (unicodeCompositionTestFind(
            records,
            0x0915,
            0x093C))
        {
            return fail(
                "excluded U+0915 + U+093C composition was emitted");
        }


        // ====================================================================
        // Accounting identity
        // ====================================================================

        if (buildResult.candidatePairCount !=
            buildResult.excludedPairCount +
            buildResult.compositionPairCount)
        {
            return fail("composition accounting identity failed");
        }


        // ====================================================================
        // PASS
        // ====================================================================

        std::printf(
            "Unicode composition builder: PASS\n"
            "  UnicodeData records:              %zu\n"
            "  Canonical mappings:               %zu\n"
            "    Singleton mappings:             %zu\n"
            "    Pair mappings:                  %zu\n"
            "  Compatibility mappings:           %zu\n"
            "  Full composition exclusions:      %zu\n"
            "  Candidate canonical pairs:        %zu\n"
            "  Excluded pair composites:         %zu\n"
            "  Composition records:              %zu\n"
            "  Composition record bytes:         %zu\n"
            "  Strict pair ordering:             PASS\n"
            "  Known composition mappings:       PASS\n"
            "  Known excluded mappings:          PASS\n"
            "  Duplicate rejection:              PASS\n"
            "  Conflict rejection:               PASS\n"
            "  Invalid code-point rejection:     PASS\n"
            "  Transactional finalize behavior:  PASS\n",
            unicodeDataRecordCount,
            canonicalMappingCount,
            canonicalSingletonCount,
            canonicalPairCount,
            compatibilityMappingCount,
            normalizationResult.codePointCount,
            buildResult.candidatePairCount,
            buildResult.excludedPairCount,
            buildResult.compositionPairCount,
            records.size() *
            sizeof(UnicodeCompositionRecord));


        return true;
    }

} // namespace waavs