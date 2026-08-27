// test_unicode_decomposition.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <vector>

#include "ucd_unicode_data_parser.h"
#include "unicode_decomposition.h"
#include "unicode_decomposition_builder.h"
#include "unicode_decomposition_page_pool_builder.h"


namespace waavs
{
    // ========================================================================
    // testUnicodeDecomposition
    //
    // End-to-end test:
    //
    //      UnicodeData.txt
    //          |
    //          v
    //      ucdParseUnicodeData()
    //          |
    //          v
    //      UnicodeDecompositionBuilder
    //          |
    //          v
    //      UnicodeDecompositionPagePoolBuilder
    //          |
    //          v
    //      UnicodeDecompositionData
    //          |
    //          v
    //      UnicodeDecomposition
    //          |
    //          v
    //      direct runtime canonical-decomposition lookup
    //
    // This deliberately stops before persistent .ucdb integration.
    //
    // ========================================================================

    static bool testUnicodeDecomposition(const ByteSpan& unicodeDataSource)
    {
        auto fail = [](const char* message) -> bool {
            std::printf(
                "Unicode canonical decomposition: FAIL: %s\n",
                message);

            return false;
            };


        // ====================================================================
        // Builder
        //
        // UnicodeDecompositionBuilder contains a dense uint16_t table covering
        // the Unicode address space, so keep it off the stack.
        // ====================================================================

        auto builder =
            std::make_unique<UnicodeDecompositionBuilder>();


        builder->reserveRecords(2200);


        // ====================================================================
        // Parse UnicodeData.txt
        // ====================================================================

        UCDUnicodeDataParseResult parseResult;


        if (!ucdParseUnicodeData(
            unicodeDataSource,
            *builder,
            parseResult))
        {
            std::printf(
                "Unicode canonical decomposition: FAIL: parser\n"
                "  Error: %s\n"
                "  Line:  %u\n",
                ucdUnicodeDataParseErrorString(
                    parseResult.error),
                parseResult.lineNumber);

            return false;
        }


        // ====================================================================
        // Unicode 17.0.0 source-data invariants
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


        if (parseResult.singletonMappingCount +
            parseResult.pairMappingCount !=
            parseResult.canonicalMappingCount)
        {
            return fail(
                "singleton + pair count does not equal canonical count");
        }


        // ====================================================================
        // Builder invariants
        // ====================================================================

        if (builder->recordCount() != 2081)
            return fail("builder record count is not 2081");


        const UnicodeDecompositionBuilderStats& builderStats =
            builder->stats();


        if (builderStats.mappingsAdded != 2081)
            return fail("builder mapping count is not 2081");


        if (builderStats.singletonMappings != 1035)
            return fail("builder singleton count is not 1035");


        if (builderStats.pairMappings != 1046)
            return fail("builder pair count is not 1046");


        // ====================================================================
        // Builder mapping helper
        // ====================================================================

        auto checkBuilderMapping =
            [&](uint32_t cp,
                uint32_t expectedFirst,
                uint32_t expectedSecond,
                const char* description) -> bool
            {
                const UnicodeDecompositionRecord* record =
                    builder->record(cp);


                if (!record)
                {
                    std::printf(
                        "Unicode canonical decomposition: FAIL: %s\n"
                        "  U+%04X has no builder mapping\n",
                        description,
                        cp);

                    return false;
                }


                if (record->first != expectedFirst ||
                    record->second != expectedSecond)
                {
                    std::printf(
                        "Unicode canonical decomposition: FAIL: %s\n"
                        "  U+%04X\n"
                        "  expected first:  U+%04X\n"
                        "  actual first:    U+%04X\n",
                        description,
                        cp,
                        expectedFirst,
                        record->first);


                    if (expectedSecond ==
                        kUnicodeDecompositionSecondNone)
                    {
                        std::printf(
                            "  expected second: none\n");
                    }
                    else
                    {
                        std::printf(
                            "  expected second: U+%04X\n",
                            expectedSecond);
                    }


                    if (record->second ==
                        kUnicodeDecompositionSecondNone)
                    {
                        std::printf(
                            "  actual second:   none\n");
                    }
                    else
                    {
                        std::printf(
                            "  actual second:   U+%04X\n",
                            record->second);
                    }


                    return false;
                }


                return true;
            };


        // ====================================================================
        // Known singleton canonical mappings
        // ====================================================================

        if (!checkBuilderMapping(
            0x0340,
            0x0300,
            kUnicodeDecompositionSecondNone,
            "COMBINING GRAVE TONE MARK"))
        {
            return false;
        }


        if (!checkBuilderMapping(
            0x0341,
            0x0301,
            kUnicodeDecompositionSecondNone,
            "COMBINING ACUTE TONE MARK"))
        {
            return false;
        }


        if (!checkBuilderMapping(
            0x0374,
            0x02B9,
            kUnicodeDecompositionSecondNone,
            "GREEK NUMERAL SIGN"))
        {
            return false;
        }


        // ====================================================================
        // Known pair canonical mappings
        // ====================================================================

        if (!checkBuilderMapping(
            0x00C0,
            0x0041,
            0x0300,
            "LATIN CAPITAL LETTER A WITH GRAVE"))
        {
            return false;
        }


        if (!checkBuilderMapping(
            0x00E9,
            0x0065,
            0x0301,
            "LATIN SMALL LETTER E WITH ACUTE"))
        {
            return false;
        }


        if (!checkBuilderMapping(
            0x0344,
            0x0308,
            0x0301,
            "COMBINING GREEK DIALYTIKA TONOS"))
        {
            return false;
        }


        if (!checkBuilderMapping(
            0x0622,
            0x0627,
            0x0653,
            "ARABIC LETTER ALEF WITH MADDA ABOVE"))
        {
            return false;
        }


        // ====================================================================
        // Direct mapping must remain direct.
        //
        // U+01D5 directly decomposes to:
        //
        //      U+00DC U+0304
        //
        // U+00DC itself directly decomposes to:
        //
        //      U+0055 U+0308
        //
        // Recursive decomposition belongs to the normalization layer.
        // ====================================================================

        if (!checkBuilderMapping(
            0x01D5,
            0x00DC,
            0x0304,
            "LATIN CAPITAL LETTER U WITH DIAERESIS AND MACRON"))
        {
            return false;
        }


        if (!checkBuilderMapping(
            0x00DC,
            0x0055,
            0x0308,
            "LATIN CAPITAL LETTER U WITH DIAERESIS"))
        {
            return false;
        }


        // ====================================================================
        // Characters without canonical decomposition
        // ====================================================================

        if (builder->hasMapping(0x0041))
            return fail("ASCII A unexpectedly has a canonical decomposition");


        if (builder->record(0x0041) != nullptr)
            return fail("ASCII A unexpectedly returned a decomposition record");


        // ====================================================================
        // Compatibility mappings must not enter the canonical table.
        //
        // U+00A0:
        //
        //      <noBreak> 0020
        //
        // U+00A8:
        //
        //      <compat> 0020 0308
        // ====================================================================

        if (builder->hasMapping(0x00A0))
            return fail("NO-BREAK SPACE compatibility mapping entered canonical table");


        if (builder->hasMapping(0x00A8))
            return fail("DIAERESIS compatibility mapping entered canonical table");


        // ====================================================================
        // Builder out-of-range behavior
        // ====================================================================

        if (builder->hasMapping(kUnicodeLimit))
            return fail("out-of-range code point reports a builder mapping");


        if (builder->recordRef(kUnicodeLimit) !=
            kUnicodeDecompositionRecordNone)
        {
            return fail("out-of-range code point returned a builder record reference");
        }


        if (builder->record(kUnicodeLimit) != nullptr)
            return fail("out-of-range code point returned a builder record");


        // ====================================================================
        // Logical sparsity
        //
        // Measure how much of the 34 x 32 page geometry actually contains
        // canonical decomposition mappings.
        // ====================================================================

        size_t logicalNonEmptyPages = 0;
        size_t logicalNonEmptyMasters = 0;


        const UnicodeDecompositionRecordRef* mappings =
            builder->data();


        for (uint32_t mi = 0; mi < kUnicodeMasterCount; ++mi)
        {
            bool masterNonEmpty = false;


            for (uint32_t si = 0; si < kUnicodeSubsPerMaster; ++si)
            {
                const uint32_t pageStart =
                    (mi << kUnicodeMasterShift) +
                    (si << kUnicodeSubShift);


                bool pageNonEmpty = false;


                for (uint32_t i = 0; i < kUnicodeSubSize; ++i)
                {
                    if (mappings[pageStart + i] !=
                        kUnicodeDecompositionRecordNone)
                    {
                        pageNonEmpty = true;
                        break;
                    }
                }


                if (pageNonEmpty)
                {
                    ++logicalNonEmptyPages;
                    masterNonEmpty = true;
                }
            }


            if (masterNonEmpty)
                ++logicalNonEmptyMasters;
        }


        if (logicalNonEmptyPages != 19)
            return fail("unexpected number of non-empty logical decomposition pages");


        if (logicalNonEmptyMasters != 5)
            return fail("unexpected number of non-empty logical decomposition masters");


        // ====================================================================
        // Finalize into persistent page architecture
        // ====================================================================

        UnicodeDecompositionPagePoolBuilder pool;


        if (!pool.reservePages(32))
            return fail("unable to reserve decomposition page pool");


        if (!pool.reserveMasterPages(8))
            return fail("unable to reserve decomposition master-page pool");


        UnicodeDecompositionData decompositionData{};


        if (!builder->finalize(
            pool,
            decompositionData))
        {
            return fail("unable to finalize decomposition page hierarchy");
        }


        // ====================================================================
        // Page-pool statistics
        //
        // Finalization visits:
        //
        //      34 * 32 = 1088 logical pages
        //
        // Unicode 17 has mappings in 19 of them.
        // ====================================================================

        const UnicodeDecompositionPagePoolStats& poolStats =
            pool.stats();


        if (poolStats.pageRequests !=
            kUnicodeMasterCount * kUnicodeSubsPerMaster)
        {
            return fail("unexpected decomposition page request count");
        }


        if (poolStats.pageEmpty !=
            (kUnicodeMasterCount * kUnicodeSubsPerMaster) - 19u)
        {
            return fail("unexpected empty decomposition page count");
        }


        if (poolStats.masterPageRequests !=
            kUnicodeMasterCount)
        {
            return fail("unexpected decomposition master-page request count");
        }


        if (poolStats.masterPageEmpty !=
            kUnicodeMasterCount - 5u)
        {
            return fail("unexpected empty decomposition master-page count");
        }


        // ====================================================================
        // Validate root structure
        // ====================================================================

        size_t rootNonEmptyMasters = 0;


        for (uint32_t mi = 0; mi < kUnicodeMasterCount; ++mi)
        {
            const UnicodeDecompositionMasterPageRef ref =
                decompositionData.masters[mi];


            if (ref ==
                kUnicodeDecompositionPageEmpty)
            {
                continue;
            }


            if (ref >= pool.masterPageCount())
                return fail("root contains invalid decomposition master reference");


            ++rootNonEmptyMasters;
        }


        if (rootNonEmptyMasters != 5)
            return fail("finalized root does not contain five non-empty masters");


        // ====================================================================
        // Validate physical master -> page references
        // ====================================================================

        for (size_t mi = 0; mi < pool.masterPageCount(); ++mi)
        {
            const UnicodeDecompositionMasterPage& master =
                pool.masterPages()[mi];


            for (uint32_t si = 0; si < kUnicodeSubsPerMaster; ++si)
            {
                const UnicodeDecompositionPageRef ref =
                    master.sub[si];


                if (ref ==
                    kUnicodeDecompositionPageEmpty)
                {
                    continue;
                }


                if (ref >= pool.pageCount())
                    return fail("master contains invalid decomposition page reference");
            }
        }


        // ====================================================================
        // Validate physical page -> record references
        // ====================================================================

        for (const UnicodeDecompositionPage& page : pool.pages())
        {
            for (uint32_t i = 0; i < kUnicodeSubSize; ++i)
            {
                const UnicodeDecompositionRecordRef ref =
                    page.mapping[i];


                if (ref ==
                    kUnicodeDecompositionRecordNone)
                {
                    continue;
                }


                if (unicodeDecompositionRecordIndex(ref) >=
                    builder->recordCount())
                {
                    return fail(
                        "physical page contains invalid decomposition record reference");
                }
            }
        }


        // ====================================================================
        // Construct immutable runtime view
        // ====================================================================

        UnicodeDecompositionPools runtimePools{
            pool.masterPageData(),
            pool.pageData(),
            builder->recordData(),
            static_cast<uint32_t>(pool.masterPageCount()),
            static_cast<uint32_t>(pool.pageCount()),
            static_cast<uint32_t>(builder->recordCount())
        };


        if (!runtimePools.valid())
            return fail("runtime decomposition pools are invalid");


        UnicodeDecomposition decomposition(
            &decompositionData,
            &runtimePools);


        if (!decomposition.valid())
            return fail("runtime decomposition view is invalid");


        if (!decomposition)
            return fail("runtime decomposition view evaluates false");


        // ====================================================================
        // Runtime mapping helper
        // ====================================================================

        auto checkRuntimeMapping =
            [&](uint32_t cp,
                uint32_t expectedFirst,
                uint32_t expectedSecond,
                const char* description) -> bool
            {
                if (!decomposition.hasMapping(cp))
                {
                    std::printf(
                        "Unicode canonical decomposition: FAIL: %s\n"
                        "  U+%04X reports no runtime mapping\n",
                        description,
                        cp);

                    return false;
                }


                const UnicodeDecompositionRecordRef ref =
                    decomposition.recordRef(cp);


                if (ref ==
                    kUnicodeDecompositionRecordNone)
                {
                    std::printf(
                        "Unicode canonical decomposition: FAIL: %s\n"
                        "  U+%04X returned no runtime record reference\n",
                        description,
                        cp);

                    return false;
                }


                if (unicodeDecompositionRecordIndex(ref) >=
                    runtimePools.recordCount)
                {
                    std::printf(
                        "Unicode canonical decomposition: FAIL: %s\n"
                        "  U+%04X returned invalid runtime record reference\n",
                        description,
                        cp);

                    return false;
                }


                const UnicodeDecompositionRecord* record =
                    decomposition.record(cp);


                if (!record)
                {
                    std::printf(
                        "Unicode canonical decomposition: FAIL: %s\n"
                        "  U+%04X returned no runtime record\n",
                        description,
                        cp);

                    return false;
                }


                if (record->first != expectedFirst ||
                    record->second != expectedSecond)
                {
                    std::printf(
                        "Unicode canonical decomposition: FAIL: %s\n"
                        "  U+%04X\n"
                        "  expected first:  U+%04X\n"
                        "  actual first:    U+%04X\n",
                        description,
                        cp,
                        expectedFirst,
                        record->first);


                    if (expectedSecond ==
                        kUnicodeDecompositionSecondNone)
                    {
                        std::printf(
                            "  expected second: none\n");
                    }
                    else
                    {
                        std::printf(
                            "  expected second: U+%04X\n",
                            expectedSecond);
                    }


                    if (record->second ==
                        kUnicodeDecompositionSecondNone)
                    {
                        std::printf(
                            "  actual second:   none\n");
                    }
                    else
                    {
                        std::printf(
                            "  actual second:   U+%04X\n",
                            record->second);
                    }


                    return false;
                }


                const uint32_t expectedLength =
                    expectedSecond ==
                    kUnicodeDecompositionSecondNone
                    ? 1u
                    : 2u;


                if (decomposition.length(cp) !=
                    expectedLength)
                {
                    std::printf(
                        "Unicode canonical decomposition: FAIL: %s\n"
                        "  U+%04X expected length=%u actual length=%u\n",
                        description,
                        cp,
                        expectedLength,
                        decomposition.length(cp));

                    return false;
                }


                return true;
            };


        // ====================================================================
        // Runtime singleton mappings
        // ====================================================================

        if (!checkRuntimeMapping(
            0x0340,
            0x0300,
            kUnicodeDecompositionSecondNone,
            "runtime COMBINING GRAVE TONE MARK"))
        {
            return false;
        }


        if (!checkRuntimeMapping(
            0x0341,
            0x0301,
            kUnicodeDecompositionSecondNone,
            "runtime COMBINING ACUTE TONE MARK"))
        {
            return false;
        }


        if (!checkRuntimeMapping(
            0x0374,
            0x02B9,
            kUnicodeDecompositionSecondNone,
            "runtime GREEK NUMERAL SIGN"))
        {
            return false;
        }


        // ====================================================================
        // Runtime pair mappings
        // ====================================================================

        if (!checkRuntimeMapping(
            0x00C0,
            0x0041,
            0x0300,
            "runtime LATIN CAPITAL LETTER A WITH GRAVE"))
        {
            return false;
        }


        if (!checkRuntimeMapping(
            0x00E9,
            0x0065,
            0x0301,
            "runtime LATIN SMALL LETTER E WITH ACUTE"))
        {
            return false;
        }


        if (!checkRuntimeMapping(
            0x0344,
            0x0308,
            0x0301,
            "runtime COMBINING GREEK DIALYTIKA TONOS"))
        {
            return false;
        }


        if (!checkRuntimeMapping(
            0x0622,
            0x0627,
            0x0653,
            "runtime ARABIC LETTER ALEF WITH MADDA ABOVE"))
        {
            return false;
        }


        // ====================================================================
        // Runtime direct recursive relationship
        // ====================================================================

        if (!checkRuntimeMapping(
            0x01D5,
            0x00DC,
            0x0304,
            "runtime LATIN CAPITAL LETTER U WITH DIAERESIS AND MACRON"))
        {
            return false;
        }


        if (!checkRuntimeMapping(
            0x00DC,
            0x0055,
            0x0308,
            "runtime LATIN CAPITAL LETTER U WITH DIAERESIS"))
        {
            return false;
        }


        // ====================================================================
        // Runtime missing mappings
        // ====================================================================

        if (decomposition.hasMapping(0x0041))
            return fail("runtime ASCII A unexpectedly has a decomposition");


        if (decomposition.recordRef(0x0041) !=
            kUnicodeDecompositionRecordNone)
        {
            return fail("runtime ASCII A returned a decomposition record reference");
        }


        if (decomposition.record(0x0041) != nullptr)
            return fail("runtime ASCII A returned a decomposition record");


        if (decomposition.length(0x0041) != 0)
            return fail("runtime ASCII A returned a nonzero decomposition length");


        // ====================================================================
        // Runtime compatibility mappings remain absent
        // ====================================================================

        if (decomposition.hasMapping(0x00A0))
            return fail("runtime NO-BREAK SPACE has canonical decomposition");


        if (decomposition.record(0x00A0) != nullptr)
            return fail("runtime NO-BREAK SPACE returned canonical decomposition");


        if (decomposition.hasMapping(0x00A8))
            return fail("runtime DIAERESIS has canonical decomposition");


        if (decomposition.record(0x00A8) != nullptr)
            return fail("runtime DIAERESIS returned canonical decomposition");


        // ====================================================================
        // Runtime out-of-range behavior
        // ====================================================================

        if (decomposition.hasMapping(kUnicodeLimit))
            return fail("out-of-range runtime code point reports a mapping");


        if (decomposition.recordRef(kUnicodeLimit) !=
            kUnicodeDecompositionRecordNone)
        {
            return fail("out-of-range runtime code point returned a record reference");
        }


        if (decomposition.record(kUnicodeLimit) != nullptr)
            return fail("out-of-range runtime code point returned a record");


        if (decomposition.length(kUnicodeLimit) != 0)
            return fail("out-of-range runtime code point returned nonzero length");


        // ====================================================================
        // Exhaustive builder/runtime comparison
        //
        // This is inexpensive enough for a generator-side test and gives us
        // much stronger validation than spot checks alone.
        //
        // Every code point must resolve to exactly the same direct canonical
        // decomposition through:
        //
        //      dense builder
        //
        // and:
        //
        //      finalized runtime page hierarchy
        //
        // ====================================================================

        size_t runtimeMappings = 0;


        for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
        {
            const UnicodeDecompositionRecord* builderRecord =
                builder->record(cp);


            const UnicodeDecompositionRecord* runtimeRecord =
                decomposition.record(cp);


            if (!builderRecord)
            {
                if (runtimeRecord)
                {
                    std::printf(
                        "Unicode canonical decomposition: FAIL: "
                        "runtime-only mapping at U+%04X\n",
                        cp);

                    return false;
                }


                continue;
            }


            if (!runtimeRecord)
            {
                std::printf(
                    "Unicode canonical decomposition: FAIL: "
                    "missing runtime mapping at U+%04X\n",
                    cp);

                return false;
            }


            if (builderRecord->first !=
                runtimeRecord->first ||
                builderRecord->second !=
                runtimeRecord->second)
            {
                std::printf(
                    "Unicode canonical decomposition: FAIL: "
                    "builder/runtime mismatch at U+%04X\n",
                    cp);

                return false;
            }


            ++runtimeMappings;
        }


        if (runtimeMappings !=
            parseResult.canonicalMappingCount)
        {
            return fail(
                "runtime mapping count does not match parsed canonical mapping count");
        }


        // ====================================================================
        // Diagnostics
        // ====================================================================

        const size_t recordBytes =
            builder->recordCount() *
            sizeof(UnicodeDecompositionRecord);


        const size_t pageBytes =
            poolStats.storedBytes();


        const size_t totalBytes =
            recordBytes +
            pageBytes +
            sizeof(UnicodeDecompositionData);


        std::printf(
            "Unicode canonical decomposition: PASS\n"
            "  Records parsed:             %zu\n"
            "  Canonical mappings:         %zu\n"
            "  Singleton mappings:         %zu\n"
            "  Pair mappings:              %zu\n"
            "  Compatibility mappings:     %zu\n",
            parseResult.recordCount,
            parseResult.canonicalMappingCount,
            parseResult.singletonMappingCount,
            parseResult.pairMappingCount,
            parseResult.compatibilityMappingCount);


        std::printf(
            "  Logical non-empty pages:    %zu\n"
            "  Logical non-empty masters:  %zu\n",
            logicalNonEmptyPages,
            logicalNonEmptyMasters);


        std::printf(
            "  Page requests:              %zu\n"
            "  Empty pages:                %zu\n"
            "  Reused pages:               %zu\n"
            "  Unique physical pages:      %zu\n"
            "  Page hash collisions:       %zu\n",
            poolStats.pageRequests,
            poolStats.pageEmpty,
            poolStats.pageReused,
            poolStats.uniquePages,
            poolStats.pageHashCollisions);


        std::printf(
            "  Master page requests:       %zu\n"
            "  Empty master pages:         %zu\n"
            "  Reused master pages:        %zu\n"
            "  Unique physical masters:    %zu\n"
            "  Master hash collisions:     %zu\n",
            poolStats.masterPageRequests,
            poolStats.masterPageEmpty,
            poolStats.masterPageReused,
            poolStats.uniqueMasterPages,
            poolStats.masterPageHashCollisions);


        std::printf(
            "  Decomposition records:      %zu\n"
            "  Runtime mappings verified:  %zu\n"
            "  Record bytes:               %zu\n"
            "  Stored page bytes:          %zu\n"
            "  Total decomposition bytes:  %zu\n",
            builder->recordCount(),
            runtimeMappings,
            recordBytes,
            pageBytes,
            totalBytes);


        return true;
    }


    // ========================================================================
    // Filename convenience wrapper
    // ========================================================================

    static bool testUnicodeDecomposition(const char* filename)
    {
        std::ifstream input(
            filename,
            std::ios::binary |
            std::ios::ate);


        if (!input)
        {
            std::printf(
                "Unicode canonical decomposition: FAIL: "
                "unable to open %s\n",
                filename);

            return false;
        }


        const std::streampos end =
            input.tellg();


        if (end <= 0)
        {
            std::printf(
                "Unicode canonical decomposition: FAIL: "
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
                "Unicode canonical decomposition: FAIL: "
                "unable to read %s\n",
                filename);

            return false;
        }


        return testUnicodeDecomposition(
            ByteSpan(
                data.data(),
                data.size()));
    }

} // namespace waavs