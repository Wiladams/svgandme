// test_ucd_general_category.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>

#include "test_core.h"

#include "ucd_general_category_parser.h"

#include "unicode_general_category.h"
#include "unicode_value_table8.h"
#include "unicode_value_table8_builder.h"
#include "unicode_value_page_pool_builder8.h"


namespace waavs
{
    // ========================================================================
    // testUCDGeneralCategory
    //
    // Parse the real DerivedGeneralCategory.txt source and exercise:
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
    // source must contain the complete DerivedGeneralCategory.txt file.
    // ========================================================================

    static bool testUCDGeneralCategory(const ByteSpan& source)
    {
        auto fail = [](const char* message) -> bool {
            std::printf("General Category: FAIL: %s\n", message);
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

        UCDGeneralCategoryParseResult parseResult;


        if (!ucdParseGeneralCategory(
            source,
            *builder,
            parseResult))
        {
            std::printf(
                "General Category: FAIL\n"
                "  Error:       %s\n"
                "  Line:        %u\n"
                "  Ranges:      %u\n"
                "  Code points: %zu\n",
                ucdGeneralCategoryParseErrorString(
                    parseResult.error),
                parseResult.lineNumber,
                parseResult.rangeCount,
                parseResult.assignedCodePoints);

            return false;
        }


        // ====================================================================
        // Basic parser invariants.
        // ====================================================================

        if (parseResult.rangeCount == 0)
            return fail("no General_Category ranges were parsed");

        if (parseResult.assignedCodePoints !=
            static_cast<size_t>(kUnicodeLimit))
        {
            return fail("General_Category did not cover all Unicode code points");
        }


        // ====================================================================
        // Check the mutable builder before finalization.
        //
        // These code points exercise several different General_Category
        // values and several parts of Unicode.
        // ====================================================================

        auto checkBuilder =
            [&](uint32_t cp,
                UnicodeGeneralCategory expected,
                const char* description) -> bool
            {
                const uint8_t actual =
                    builder->value(cp);

                const uint8_t wanted =
                    static_cast<uint8_t>(expected);

                if (actual != wanted)
                {
                    std::printf(
                        "General Category: FAIL: builder %s\n"
                        "  U+%04X expected=%u actual=%u\n",
                        description,
                        cp,
                        static_cast<unsigned>(wanted),
                        static_cast<unsigned>(actual));

                    return false;
                }

                return true;
            };


        if (!checkBuilder(
            0x0041,
            UnicodeGeneralCategory::UppercaseLetter,
            "Latin capital A"))
        {
            return false;
        }

        if (!checkBuilder(
            0x0061,
            UnicodeGeneralCategory::LowercaseLetter,
            "Latin small a"))
        {
            return false;
        }

        if (!checkBuilder(
            0x0030,
            UnicodeGeneralCategory::DecimalNumber,
            "digit zero"))
        {
            return false;
        }

        if (!checkBuilder(
            0x0020,
            UnicodeGeneralCategory::SpaceSeparator,
            "space"))
        {
            return false;
        }

        if (!checkBuilder(
            0x000A,
            UnicodeGeneralCategory::Control,
            "line feed"))
        {
            return false;
        }

        if (!checkBuilder(
            0x0301,
            UnicodeGeneralCategory::NonspacingMark,
            "combining acute accent"))
        {
            return false;
        }

        if (!checkBuilder(
            0x0C15,
            UnicodeGeneralCategory::OtherLetter,
            "Telugu KA"))
        {
            return false;
        }

        if (!checkBuilder(
            0x200D,
            UnicodeGeneralCategory::Format,
            "zero width joiner"))
        {
            return false;
        }

        if (!checkBuilder(
            0x0378,
            UnicodeGeneralCategory::Unassigned,
            "unassigned code point"))
        {
            return false;
        }

        if (!checkBuilder(
            0xD800,
            UnicodeGeneralCategory::Surrogate,
            "high surrogate"))
        {
            return false;
        }

        if (!checkBuilder(
            0xE000,
            UnicodeGeneralCategory::PrivateUse,
            "private-use code point"))
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
            return fail("unable to finalize General_Category VALUE8 table");
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
            return fail("runtime General_Category table is invalid");


        // ====================================================================
        // Runtime semantic checks.
        //
        // Repeat the builder tests against the finalized representation.
        // ====================================================================

        auto checkRuntime =
            [&](uint32_t cp,
                UnicodeGeneralCategory expected,
                const char* description) -> bool
            {
                const uint8_t actual =
                    table.value(cp);

                const uint8_t wanted =
                    static_cast<uint8_t>(expected);

                if (actual != wanted)
                {
                    std::printf(
                        "General Category: FAIL: runtime %s\n"
                        "  U+%04X expected=%u actual=%u\n",
                        description,
                        cp,
                        static_cast<unsigned>(wanted),
                        static_cast<unsigned>(actual));

                    return false;
                }

                return true;
            };


        if (!checkRuntime(
            0x0041,
            UnicodeGeneralCategory::UppercaseLetter,
            "Latin capital A"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0061,
            UnicodeGeneralCategory::LowercaseLetter,
            "Latin small a"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0030,
            UnicodeGeneralCategory::DecimalNumber,
            "digit zero"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0020,
            UnicodeGeneralCategory::SpaceSeparator,
            "space"))
        {
            return false;
        }

        if (!checkRuntime(
            0x000A,
            UnicodeGeneralCategory::Control,
            "line feed"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0301,
            UnicodeGeneralCategory::NonspacingMark,
            "combining acute accent"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0C15,
            UnicodeGeneralCategory::OtherLetter,
            "Telugu KA"))
        {
            return false;
        }

        if (!checkRuntime(
            0x200D,
            UnicodeGeneralCategory::Format,
            "zero width joiner"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0378,
            UnicodeGeneralCategory::Unassigned,
            "unassigned code point"))
        {
            return false;
        }

        if (!checkRuntime(
            0xD800,
            UnicodeGeneralCategory::Surrogate,
            "high surrogate"))
        {
            return false;
        }

        if (!checkRuntime(
            0xE000,
            UnicodeGeneralCategory::PrivateUse,
            "private-use code point"))
        {
            return false;
        }


        // ====================================================================
        // Additional punctuation / symbol checks.
        // ====================================================================

        if (!checkRuntime(
            0x005F,
            UnicodeGeneralCategory::ConnectorPunctuation,
            "underscore"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0028,
            UnicodeGeneralCategory::OpenPunctuation,
            "left parenthesis"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0029,
            UnicodeGeneralCategory::ClosePunctuation,
            "right parenthesis"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0021,
            UnicodeGeneralCategory::OtherPunctuation,
            "exclamation mark"))
        {
            return false;
        }

        if (!checkRuntime(
            0x002B,
            UnicodeGeneralCategory::MathSymbol,
            "plus sign"))
        {
            return false;
        }

        if (!checkRuntime(
            0x0024,
            UnicodeGeneralCategory::CurrencySymbol,
            "dollar sign"))
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
        // These values are intentionally diagnostic rather than hard-coded
        // pass/fail criteria. They may change as Unicode evolves while the
        // semantic behavior remains correct.
        // ====================================================================

        const UnicodeValuePagePoolStats8& stats =
            pool.stats();


        std::printf(
            "General Category: PASS\n"
            "  Ranges parsed:             %u\n"
            "  Assigned code points:      %zu\n",
            parseResult.rangeCount,
            parseResult.assignedCodePoints);


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

    bool testUCDGeneralCategory(const char *filename)
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
        return testUCDGeneralCategory(source);
    }

} // namespace waavs
