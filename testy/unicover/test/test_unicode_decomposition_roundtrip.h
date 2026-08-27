// test_unicode_decomposition_roundtrip.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <vector>

#include "ucd_unicode_data_parser.h"
#include "unicode_database.h"
#include "unicode_database_builder.h"
#include "unicode_database_writer.h"
#include "unicode_decomposition_builder.h"


namespace waavs
{
    // ========================================================================
    // testUnicodeDecompositionRoundTrip
    //
    // Build a decomposition-only Unicode 17.0.0 database, serialize it,
    // reload it through UnicodeDatabase, and verify the complete direct
    // canonical-decomposition mapping.
    //
    // This deliberately does not add General_Category, CCC, blocks, scripts,
    // or binary properties. That also verifies that format 1.2 capabilities
    // remain independently optional.
    //
    // ========================================================================

    static bool testUnicodeDecompositionRoundTrip(
        const ByteSpan& unicodeDataSource)
    {
        auto fail = [](const char* message) -> bool {
            std::printf(
                "Unicode decomposition round-trip: FAIL: %s\n",
                message);

            return false;
            };


        // ====================================================================
        // Parse canonical decomposition
        //
        // Keep the dense decomposition builder off the stack.
        // ====================================================================

        auto decompositionBuilder =
            std::make_unique<UnicodeDecompositionBuilder>();


        decompositionBuilder->reserveRecords(2200);


        UCDUnicodeDataParseResult parseResult;


        if (!ucdParseUnicodeData(
            unicodeDataSource,
            *decompositionBuilder,
            parseResult))
        {
            std::printf(
                "Unicode decomposition round-trip: FAIL: parser\n"
                "  Error: %s\n"
                "  Line:  %u\n",
                ucdUnicodeDataParseErrorString(
                    parseResult.error),
                parseResult.lineNumber);

            return false;
        }


        // ====================================================================
        // Source-data invariants for Unicode 17.0.0
        // ====================================================================

        if (parseResult.recordCount != 40575)
            return fail("unexpected UnicodeData.txt record count");

        if (parseResult.canonicalMappingCount != 2081)
            return fail("unexpected canonical decomposition count");

        if (parseResult.singletonMappingCount != 1035)
            return fail("unexpected singleton decomposition count");

        if (parseResult.pairMappingCount != 1046)
            return fail("unexpected pair decomposition count");

        if (parseResult.compatibilityMappingCount != 3833)
            return fail("unexpected compatibility decomposition count");


        // ====================================================================
        // Build persistent database representation
        // ====================================================================

        UnicodeDatabaseBuilder databaseBuilder;


        if (!databaseBuilder.decompositionPagePool().reservePages(32))
            return fail("unable to reserve decomposition page pool");


        if (!databaseBuilder.decompositionPagePool().reserveMasterPages(8))
            return fail("unable to reserve decomposition master-page pool");


        UnicodeDecompositionData decompositionData{};


        if (!decompositionBuilder->finalize(
            databaseBuilder.decompositionPagePool(),
            decompositionData))
        {
            return fail("unable to finalize decomposition hierarchy");
        }


        if (!databaseBuilder.setDecomposition(
            decompositionData,
            decompositionBuilder->records()))
        {
            return fail("unable to register decomposition with database builder");
        }


        // ====================================================================
        // Database-builder invariants
        // ====================================================================

        if (!databaseBuilder.hasDecomposition())
            return fail("database builder does not report decomposition");


        if (databaseBuilder.decompositionRecordCount() != 2081)
            return fail("database builder decomposition record count is not 2081");


        if (databaseBuilder.decompositionPagePool().pageCount() != 19)
            return fail("database builder decomposition page count is not 19");


        if (databaseBuilder.decompositionPagePool().masterPageCount() != 5)
            return fail("database builder decomposition master count is not 5");


        // ====================================================================
        // Write in-memory format 1.2 database
        // ====================================================================

        std::vector<uint8_t> databaseImage;

        UnicodeDatabaseWriteResult writeResult;


        if (!UnicodeDatabaseWriter::write(
            databaseBuilder,
            17,
            0,
            0,
            databaseImage,
            writeResult))
        {
            std::printf(
                "Unicode decomposition round-trip: FAIL: writer\n"
                "  Error: %s\n",
                unicodeDatabaseWriteErrorString(
                    writeResult.error));

            return false;
        }


        if (databaseImage.empty())
            return fail("writer produced an empty database image");


        if (writeResult.databaseSize != databaseImage.size())
            return fail("writer result size does not match image size");


        // ====================================================================
        // Inspect persistent header directly
        // ====================================================================

        if (databaseImage.size() <
            sizeof(UnicodeDatabaseHeader))
        {
            return fail("database image is smaller than database header");
        }


        const auto* writtenHeader =
            reinterpret_cast<const UnicodeDatabaseHeader*>(
                databaseImage.data());


        if (writtenHeader->formatMajor != kUnicodeDatabaseFormatMajor ||
            writtenHeader->formatMinor != kUnicodeDatabaseFormatMinor)
        {
            return fail("database format version is incorrect");
        }


        if (writtenHeader->unicodeMajor != 17 ||
            writtenHeader->unicodeMinor != 0 ||
            writtenHeader->unicodePatch != 0)
        {
            return fail("database Unicode version is not 17.0.0");
        }


        if (writtenHeader->headerSize !=
            sizeof(UnicodeDatabaseHeader))
        {
            return fail("database header size is incorrect");
        }


        if (writtenHeader->databaseSize !=
            databaseImage.size())
        {
            return fail("header database size does not match image size");
        }


        if (writtenHeader->decompositionOffset == 0)
            return fail("header contains no decomposition section offset");



        // ====================================================================
        // This database intentionally contains no VALUE8 data.
        //
        // This verifies that format 1.2 does not require GC or CCC merely
        // because the format supports VALUE8.
        // ====================================================================

        if (writtenHeader->valueProperty8Count != 0 ||
            writtenHeader->valueTable8Count != 0 ||
            writtenHeader->valueMasterPage8Count != 0 ||
            writtenHeader->valuePage8Count != 0)
        {
            return fail("decomposition-only database unexpectedly contains VALUE8 data");
        }


        // ====================================================================
        // Reload through UnicodeDatabase
        // ====================================================================

        UnicodeDatabase database;


        if (!database.reset(
            ByteSpan(
                databaseImage.data(),
                databaseImage.size())))
        {
            return fail("UnicodeDatabase rejected written database image");
        }


        if (!database.valid())
            return fail("loaded UnicodeDatabase is invalid");


        // ====================================================================
        // Database version
        // ====================================================================

        if (database.unicodeMajor() != 17 ||
            database.unicodeMinor() != 0 ||
            database.unicodePatch() != 0)
        {
            return fail("loaded Unicode version is not 17.0.0");
        }


        const UnicodeDatabaseHeader* header =
            database.header();


        if (!header)
            return fail("loaded database returned no header");


        if (header->formatMajor != kUnicodeDatabaseFormatMajor ||
            header->formatMinor != kUnicodeDatabaseFormatMinor)
        {
            return fail("loaded database format version is incorrect");
        }


        // ====================================================================
        // Optional-property behavior
        // ====================================================================

        if (database.hasGeneralCategory())
            return fail("decomposition-only database unexpectedly has General_Category");


        if (database.hasCombiningClass())
            return fail("decomposition-only database unexpectedly has CCC");


        // Established defaults must still work.

        if (database.generalCategory(0x0041) !=
            UnicodeGeneralCategory::Unassigned)
        {
            return fail("missing General_Category does not return default");
        }


        if (database.combiningClass(0x0301) !=
            kUnicodeCombiningClassNotReordered)
        {
            return fail("missing CCC does not return default");
        }


        // ====================================================================
        // Persistent decomposition section
        // ====================================================================

        if (!database.hasDecomposition())
            return fail("loaded database does not contain decomposition");


        if (database.decompositionMasterPageCount() != 5)
            return fail("loaded decomposition master-page count is not 5");


        if (database.decompositionPageCount() != 19)
            return fail("loaded decomposition page count is not 19");


        if (database.decompositionRecordCount() != 2081)
            return fail("loaded decomposition record count is not 2081");


        const UnicodeDecompositionSection* section =
            database.decompositionSection();


        if (!section)
            return fail("loaded database returned no decomposition section");


        if (section->reserved != 0)
            return fail("decomposition section reserved field is nonzero");


        if (section->masterPageCount != 5 ||
            section->pageCount != 19 ||
            section->recordCount != 2081)
        {
            return fail("decomposition section counts are incorrect");
        }


        if (section->masterPageOffset == 0 ||
            section->pageOffset == 0 ||
            section->recordOffset == 0)
        {
            return fail("non-empty decomposition pool has zero file offset");
        }


        // ====================================================================
        // Writer alignment
        // ====================================================================

        if ((section->masterPageOffset %
            kUnicodeDatabasePoolAlignment) != 0)
        {
            return fail("decomposition master-page pool is not aligned");
        }


        if ((section->pageOffset %
            kUnicodeDatabasePoolAlignment) != 0)
        {
            return fail("decomposition page pool is not aligned");
        }


        if ((section->recordOffset %
            kUnicodeDatabasePoolAlignment) != 0)
        {
            return fail("decomposition record pool is not aligned");
        }


        // ====================================================================
        // Runtime decomposition view
        // ====================================================================

        UnicodeDecomposition decomposition =
            database.decomposition();


        if (!decomposition)
            return fail("database returned empty decomposition view");


        if (!decomposition.valid())
            return fail("database returned invalid decomposition view");


        // ====================================================================
        // Mapping helper
        // ====================================================================

        auto checkMapping =
            [&](uint32_t cp,
                uint32_t expectedFirst,
                uint32_t expectedSecond,
                const char* description) -> bool
            {
                const UnicodeDecompositionRecord* record =
                    decomposition.record(cp);


                if (!record)
                {
                    std::printf(
                        "Unicode decomposition round-trip: FAIL: %s\n"
                        "  U+%04X has no mapping\n",
                        description,
                        cp);

                    return false;
                }


                if (record->first != expectedFirst ||
                    record->second != expectedSecond)
                {
                    std::printf(
                        "Unicode decomposition round-trip: FAIL: %s\n"
                        "  U+%04X\n"
                        "  Expected first: U+%04X\n"
                        "  Actual first:   U+%04X\n",
                        description,
                        cp,
                        expectedFirst,
                        record->first);


                    if (expectedSecond ==
                        kUnicodeDecompositionSecondNone)
                    {
                        std::printf(
                            "  Expected second: none\n");
                    }
                    else
                    {
                        std::printf(
                            "  Expected second: U+%04X\n",
                            expectedSecond);
                    }


                    if (record->second ==
                        kUnicodeDecompositionSecondNone)
                    {
                        std::printf(
                            "  Actual second:   none\n");
                    }
                    else
                    {
                        std::printf(
                            "  Actual second:   U+%04X\n",
                            record->second);
                    }


                    return false;
                }


                const uint32_t expectedLength =
                    expectedSecond ==
                    kUnicodeDecompositionSecondNone
                    ? 1u
                    : 2u;


                if (decomposition.length(cp) != expectedLength)
                {
                    std::printf(
                        "Unicode decomposition round-trip: FAIL: %s\n"
                        "  U+%04X expected length %u, actual %u\n",
                        description,
                        cp,
                        expectedLength,
                        decomposition.length(cp));

                    return false;
                }


                return true;
            };


        // ====================================================================
        // Singleton mappings
        // ====================================================================

        if (!checkMapping(
            0x0340,
            0x0300,
            kUnicodeDecompositionSecondNone,
            "COMBINING GRAVE TONE MARK"))
        {
            return false;
        }


        if (!checkMapping(
            0x0341,
            0x0301,
            kUnicodeDecompositionSecondNone,
            "COMBINING ACUTE TONE MARK"))
        {
            return false;
        }


        if (!checkMapping(
            0x0374,
            0x02B9,
            kUnicodeDecompositionSecondNone,
            "GREEK NUMERAL SIGN"))
        {
            return false;
        }


        // ====================================================================
        // Pair mappings
        // ====================================================================

        if (!checkMapping(
            0x00C0,
            0x0041,
            0x0300,
            "LATIN CAPITAL LETTER A WITH GRAVE"))
        {
            return false;
        }


        if (!checkMapping(
            0x00E9,
            0x0065,
            0x0301,
            "LATIN SMALL LETTER E WITH ACUTE"))
        {
            return false;
        }


        if (!checkMapping(
            0x0344,
            0x0308,
            0x0301,
            "COMBINING GREEK DIALYTIKA TONOS"))
        {
            return false;
        }


        if (!checkMapping(
            0x0622,
            0x0627,
            0x0653,
            "ARABIC LETTER ALEF WITH MADDA ABOVE"))
        {
            return false;
        }


        // ====================================================================
        // Direct recursive relationship remains intact.
        //
        // U+01D5:
        //
        //      U+01D5 -> U+00DC U+0304
        //
        // U+00DC:
        //
        //      U+00DC -> U+0055 U+0308
        //
        // The persistent database stores direct UCD mappings, not recursively
        // flattened mappings.
        // ====================================================================

        if (!checkMapping(
            0x01D5,
            0x00DC,
            0x0304,
            "LATIN CAPITAL LETTER U WITH DIAERESIS AND MACRON"))
        {
            return false;
        }


        if (!checkMapping(
            0x00DC,
            0x0055,
            0x0308,
            "LATIN CAPITAL LETTER U WITH DIAERESIS"))
        {
            return false;
        }


        // ====================================================================
        // Missing canonical mappings
        // ====================================================================

        if (decomposition.hasMapping(0x0041))
            return fail("ASCII A unexpectedly has canonical decomposition");


        if (decomposition.record(0x0041) != nullptr)
            return fail("ASCII A unexpectedly returned decomposition record");


        if (decomposition.length(0x0041) != 0)
            return fail("ASCII A unexpectedly returned decomposition length");


        // ====================================================================
        // Compatibility mappings remain absent.
        // ====================================================================

        if (decomposition.hasMapping(0x00A0))
            return fail("NO-BREAK SPACE compatibility mapping entered canonical table");


        if (decomposition.hasMapping(0x00A8))
            return fail("DIAERESIS compatibility mapping entered canonical table");


        // ====================================================================
        // Out-of-range behavior
        // ====================================================================

        if (decomposition.hasMapping(kUnicodeLimit))
            return fail("out-of-range code point reports decomposition");


        if (decomposition.recordRef(kUnicodeLimit) !=
            kUnicodeDecompositionRecordNone)
        {
            return fail("out-of-range code point returned decomposition record reference");
        }


        if (decomposition.record(kUnicodeLimit) != nullptr)
            return fail("out-of-range code point returned decomposition record");


        if (decomposition.length(kUnicodeLimit) != 0)
            return fail("out-of-range code point returned decomposition length");


        // ====================================================================
        // Exhaustive round-trip validation
        //
        // Compare every Unicode position between:
        //
        //      original dense generator representation
        //
        // and:
        //
        //      serialized -> loaded -> runtime page representation
        //
        // ====================================================================

        size_t mappingsVerified = 0;


        for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
        {
            const UnicodeDecompositionRecord* sourceRecord =
                decompositionBuilder->record(cp);


            const UnicodeDecompositionRecord* loadedRecord =
                decomposition.record(cp);


            if (!sourceRecord)
            {
                if (loadedRecord)
                {
                    std::printf(
                        "Unicode decomposition round-trip: FAIL: "
                        "unexpected loaded mapping at U+%04X\n",
                        cp);

                    return false;
                }


                continue;
            }


            if (!loadedRecord)
            {
                std::printf(
                    "Unicode decomposition round-trip: FAIL: "
                    "missing loaded mapping at U+%04X\n",
                    cp);

                return false;
            }


            if (sourceRecord->first != loadedRecord->first ||
                sourceRecord->second != loadedRecord->second)
            {
                std::printf(
                    "Unicode decomposition round-trip: FAIL: "
                    "mapping mismatch at U+%04X\n",
                    cp);

                return false;
            }


            ++mappingsVerified;
        }


        if (mappingsVerified !=
            parseResult.canonicalMappingCount)
        {
            return fail(
                "exhaustive mapping count does not match parsed mapping count");
        }


        // ====================================================================
        // Diagnostics
        // ====================================================================

        const UnicodeDecompositionPagePoolStats& poolStats =
            databaseBuilder.decompositionPagePool().stats();


        std::printf(
            "Unicode decomposition round-trip: PASS\n"
            "  Database bytes:             %zu\n"
            "  Format version:             %u.%u\n"
            "  Unicode version:            %u.%u.%u\n"
            "  Records parsed:             %zu\n"
            "  Canonical mappings:         %zu\n"
            "  Singleton mappings:         %zu\n"
            "  Pair mappings:              %zu\n"
            "  Compatibility mappings:     %zu\n",
            databaseImage.size(),
            header->formatMajor,
            header->formatMinor,
            database.unicodeMajor(),
            database.unicodeMinor(),
            database.unicodePatch(),
            parseResult.recordCount,
            parseResult.canonicalMappingCount,
            parseResult.singletonMappingCount,
            parseResult.pairMappingCount,
            parseResult.compatibilityMappingCount);


        std::printf(
            "  Decomposition section:      %u\n"
            "  Master page offset:         %u\n"
            "  Master pages:               %u\n"
            "  Leaf page offset:           %u\n"
            "  Leaf pages:                 %u\n"
            "  Record offset:              %u\n"
            "  Decomposition records:      %u\n",
            header->decompositionOffset,
            section->masterPageOffset,
            section->masterPageCount,
            section->pageOffset,
            section->pageCount,
            section->recordOffset,
            section->recordCount);


        std::printf(
            "  Page requests:              %zu\n"
            "  Empty pages:                %zu\n"
            "  Reused pages:               %zu\n"
            "  Unique physical pages:      %zu\n"
            "  Master page requests:       %zu\n"
            "  Empty master pages:         %zu\n"
            "  Reused master pages:        %zu\n"
            "  Unique physical masters:    %zu\n",
            poolStats.pageRequests,
            poolStats.pageEmpty,
            poolStats.pageReused,
            poolStats.uniquePages,
            poolStats.masterPageRequests,
            poolStats.masterPageEmpty,
            poolStats.masterPageReused,
            poolStats.uniqueMasterPages);


        std::printf(
            "  Runtime mappings verified:  %zu\n",
            mappingsVerified);


        return true;
    }


    // ========================================================================
    // Filename convenience wrapper
    // ========================================================================

    static bool testUnicodeDecompositionRoundTrip(const char* filename)
    {
        std::ifstream input(
            filename,
            std::ios::binary |
            std::ios::ate);


        if (!input)
        {
            std::printf(
                "Unicode decomposition round-trip: FAIL: "
                "unable to open %s\n",
                filename);

            return false;
        }


        const std::streampos end =
            input.tellg();


        if (end <= 0)
        {
            std::printf(
                "Unicode decomposition round-trip: FAIL: "
                "empty or unreadable file %s\n",
                filename);

            return false;
        }


        std::vector<uint8_t> data(
            static_cast<size_t>(end));


        input.seekg(
            0,
            std::ios::beg);


        input.read(
            reinterpret_cast<char*>(
                data.data()),
            static_cast<std::streamsize>(
                data.size()));


        if (!input)
        {
            std::printf(
                "Unicode decomposition round-trip: FAIL: "
                "unable to read %s\n",
                filename);

            return false;
        }


        return testUnicodeDecompositionRoundTrip(
            ByteSpan(
                data.data(),
                data.size()));
    }

} // namespace waavs