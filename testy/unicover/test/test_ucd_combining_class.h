// test_ucd_combining_class.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>

#include "test_core.h"

#include "ucd_combining_class_parser.h"

#include "unicode_combining_class.h"
#include "unicode_value_page_pool_builder8.h"
#include "unicode_value_table8.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    // ========================================================================
    // testUCDCombiningClass
    //
    // Parse the real DerivedCombiningClass.txt source and exercise:
    //
    //      UCD parser
    //          ->
    //      UnicodeValueTable8Builder
    //          ->
    //      UnicodeValuePagePoolBuilder8
    //          ->
    //      UnicodeValueTable8Data
    //          ->
    //      UnicodeValueTable8 runtime lookup
    //
    // source must contain the complete DerivedCombiningClass.txt file.
    // ========================================================================

    static bool testUCDCombiningClass(const ByteSpan& source)
    {
        auto fail = [](const char* message) -> bool {
            std::printf("Combining Class: FAIL: %s\n", message);
            return false;
            };


        // ====================================================================
        // Parse.
        //
        // UnicodeValueTable8Builder is approximately 1.1 MiB, so keep it off
        // the thread stack.
        // ====================================================================

        auto builder =
            std::make_unique<UnicodeValueTable8Builder>();

        UCDCombiningClassParseResult parseResult;


        if (!ucdParseCombiningClass(
            source,
            *builder,
            parseResult))
        {
            std::printf(
                "Combining Class: FAIL\n"
                "  Error:                %s\n"
                "  Line:                 %u\n"
                "  Ranges:               %u\n"
                "  Explicit code points: %zu\n"
                "  Non-zero code points: %zu\n",
                ucdCombiningClassParseErrorString(
                    parseResult.error),
                parseResult.lineNumber,
                parseResult.rangeCount,
                parseResult.explicitCodePoints,
                parseResult.nonZeroCodePoints);

            return false;
        }


        // ====================================================================
        // Basic parser invariants.
        // ====================================================================

        if (parseResult.rangeCount == 0)
            return fail("no Canonical_Combining_Class ranges were parsed");

        if (parseResult.explicitCodePoints == 0)
            return fail("no Canonical_Combining_Class code points were parsed");

        if (parseResult.nonZeroCodePoints == 0)
            return fail("no non-zero Canonical_Combining_Class values were parsed");


        // ====================================================================
        // Check the mutable builder before finalization.
        // ====================================================================

        auto checkBuilder =
            [&](uint32_t cp,
                UnicodeCombiningClass expected,
                const char* description) -> bool
            {
                const UnicodeCombiningClass actual =
                    builder->value(cp);

                if (actual != expected)
                {
                    std::printf(
                        "Combining Class: FAIL: builder %s\n"
                        "  U+%04X expected=%u actual=%u\n",
                        description,
                        cp,
                        static_cast<unsigned>(expected),
                        static_cast<unsigned>(actual));

                    return false;
                }

                return true;
            };


        // --------------------------------------------------------------------
        // Ordinary base characters are Not_Reordered.
        // --------------------------------------------------------------------

        if (!checkBuilder(
            0x0041,
            0,
            "Latin capital A"))
        {
            return false;
        }

        if (!checkBuilder(
            0x0061,
            0,
            "Latin small a"))
        {
            return false;
        }

        if (!checkBuilder(
            0x0020,
            0,
            "space"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // Common combining marks.
        // --------------------------------------------------------------------

        if (!checkBuilder(
            0x0300,
            230,
            "combining grave accent"))
        {
            return false;
        }

        if (!checkBuilder(
            0x0301,
            230,
            "combining acute accent"))
        {
            return false;
        }

        if (!checkBuilder(
            0x0315,
            232,
            "combining comma above right"))
        {
            return false;
        }

        if (!checkBuilder(
            0x0327,
            202,
            "combining cedilla"))
        {
            return false;
        }

        if (!checkBuilder(
            0x0345,
            240,
            "combining Greek ypogegrammeni"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // Unassigned code points also receive the default CCC zero.
        // --------------------------------------------------------------------

        if (!checkBuilder(
            0x0378,
            0,
            "unassigned code point"))
        {
            return false;
        }


        // --------------------------------------------------------------------
        // Surrogates remain part of the database code-point address space and
        // receive the default CCC value zero.
        // --------------------------------------------------------------------

        if (!checkBuilder(
            0xD800,
            0,
            "high surrogate"))
        {
            return false;
        }


        // ====================================================================
        // Finalize into canonical VALUE8 storage.
        // ====================================================================

        UnicodeValuePagePoolBuilder8 pool;
        UnicodeValueTable8Data tableData{};


        if (!builder->finalize(
            pool,
            tableData))
        {
            return fail("unable to finalize Canonical_Combining_Class VALUE8 table");
        }


        // ====================================================================
        // Establish runtime pool view.
        // ====================================================================

        UnicodeValueTable8Pools runtimePools{};

        runtimePools.masterPages =
            pool.masterPageData();

        runtimePools.valuePages =
            pool.valuePageData();

        runtimePools.masterPageCount =
            static_cast<uint32_t>(
                pool.masterPageCount());

        runtimePools.valuePageCount =
            static_cast<uint32_t>(
                pool.valuePageCount());


        UnicodeValueTable8 table(
            &tableData,
            &runtimePools);


        if (!table.valid())
            return fail("runtime Canonical_Combining_Class table is invalid");


        // ====================================================================
        // Runtime semantic checks.
        // ====================================================================

        auto checkRuntime =
            [&](uint32_t cp,
                UnicodeCombiningClass expected,
                const char* description) -> bool
            {
                const UnicodeCombiningClass actual =
                    table.value(cp);

                if (actual != expected)
                {
                    std::printf(
                        "Combining Class: FAIL: runtime %s\n"
                        "  U+%04X expected=%u actual=%u\n",
                        description,
                        cp,
                        static_cast<unsigned>(expected),
                        static_cast<unsigned>(actual));

                    return false;
                }

                return true;
            };


        if (!checkRuntime(
            0x0041,
            0,
            "Latin capital A"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0061,
            0,
            "Latin small a"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0020,
            0,
            "space"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0300,
            230,
            "combining grave accent"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0301,
            230,
            "combining acute accent"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0315,
            232,
            "combining comma above right"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0327,
            202,
            "combining cedilla"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0345,
            240,
            "combining Greek ypogegrammeni"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0378,
            0,
            "unassigned code point"))
        {
            return false;
        }

        if (!checkRuntime(
            0xD800,
            0,
            "high surrogate"))
        {
            return false;
        }


        // ====================================================================
        // Boundary checks around nonuniform pages.
        //
        // These are useful because CCC has many large zero regions surrounding
        // relatively small clusters of combining marks.
        // ====================================================================

        if (!checkRuntime(
            0x02FF,
            0,
            "code point before combining diacritical marks"))
        {
            return false;
        }

        if (!checkRuntime(
            0x036F,
            230,
            "combining Latin small letter x"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0370,
            0,
            "code point after combining diacritical marks"))
        {
            return false;
        }


        // ====================================================================
        // Out-of-range runtime behavior.
        // ====================================================================

        if (table.value(kUnicodeLimit) != 0)
            return fail("out-of-range runtime lookup did not return zero");


        // ====================================================================
        // Pool diagnostics.
        //
        // Do not make the physical pool counts test invariants. Unicode can
        // evolve while the representation and semantic behavior remain valid.
        // ====================================================================

        const UnicodeValuePagePoolStats8& stats =
            pool.stats();


        std::printf(
            "Combining Class: PASS\n"
            "  Ranges parsed:             %u\n"
            "  Explicit code points:      %zu\n"
            "  Non-zero code points:      %zu\n",
            parseResult.rangeCount,
            parseResult.explicitCodePoints,
            parseResult.nonZeroCodePoints);


        std::printf(
            "  Value page requests:       %zu\n"
            "  Uniform value pages:       %zu\n"
            "  Reused value pages:        %zu\n"
            "  Unique value pages:        %zu\n"
            "  Value hash collisions:     %zu\n",
            stats.valuePageRequests,
            stats.valuePageUniform,
            stats.valuePageReused,
            stats.uniqueValuePages,
            stats.valuePageHashCollisions);


        std::printf(
            "  Master page requests:      %zu\n"
            "  Uniform master pages:      %zu\n"
            "  Reused master pages:       %zu\n"
            "  Unique master pages:       %zu\n"
            "  Master hash collisions:    %zu\n",
            stats.masterPageRequests,
            stats.masterPageUniform,
            stats.masterPageReused,
            stats.uniqueMasterPages,
            stats.masterPageHashCollisions);


        std::printf(
            "  Stored VALUE8 bytes:       %zu\n",
            stats.storedBytes());


        return true;
    }

    bool testUCDCombiningClass(const char* filename)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(filename, fileData))
        {
            printf("FAILED: unable to read file\n");
            printf("  file: %s\n", filename);
            return false;
        }

        const ByteSpan source = ByteSpan(fileData.data(), fileData.size());


        if (source.empty())
        {
            std::printf(
                "General Category: FAIL: unable to load source\n");
            return false;
        }
        return testUCDCombiningClass(source);
    }

} // namespace waavs
