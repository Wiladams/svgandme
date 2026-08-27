// test_ucd_grapheme_cluster_break_parser.h

#pragma once

#include "test_core.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#include "ucd_grapheme_cluster_break_parser.h"
#include "unicode_coverage_builder.h"
#include "unicode_grapheme_cluster_break.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    // ========================================================================
    // Test helpers
    // ========================================================================

    static bool testGraphemeClusterBreakSpanEquals(const ByteSpan& span, const char* text) noexcept
    {
        const size_t length = std::strlen(text);

        return span.size() == length &&
            std::memcmp(span.data(), text, length) == 0;
    }


    static bool testGraphemeClusterBreakSpanStartsWith(const ByteSpan& span, const char* text) noexcept
    {
        const size_t length = std::strlen(text);

        return span.size() >= length &&
            std::memcmp(span.data(), text, length) == 0;
    }


    // ========================================================================
    // Independent property-value mapping
    //
    // This intentionally does not call the parser's breakClassFromField().
    // The official GraphemeBreakProperty.txt uses the long property names.
    // ========================================================================

    static bool testGraphemeClusterBreakClassFromField(const ByteSpan& field, UnicodeGraphemeClusterBreak& outValue) noexcept
    {
        if (testGraphemeClusterBreakSpanEquals(field, "Other"))
            outValue = UnicodeGraphemeClusterBreak::Other;
        else if (testGraphemeClusterBreakSpanEquals(field, "CR"))
            outValue = UnicodeGraphemeClusterBreak::CR;
        else if (testGraphemeClusterBreakSpanEquals(field, "LF"))
            outValue = UnicodeGraphemeClusterBreak::LF;
        else if (testGraphemeClusterBreakSpanEquals(field, "Control"))
            outValue = UnicodeGraphemeClusterBreak::Control;
        else if (testGraphemeClusterBreakSpanEquals(field, "Extend"))
            outValue = UnicodeGraphemeClusterBreak::Extend;
        else if (testGraphemeClusterBreakSpanEquals(field, "ZWJ"))
            outValue = UnicodeGraphemeClusterBreak::ZWJ;
        else if (testGraphemeClusterBreakSpanEquals(field, "Regional_Indicator"))
            outValue = UnicodeGraphemeClusterBreak::RegionalIndicator;
        else if (testGraphemeClusterBreakSpanEquals(field, "Prepend"))
            outValue = UnicodeGraphemeClusterBreak::Prepend;
        else if (testGraphemeClusterBreakSpanEquals(field, "SpacingMark"))
            outValue = UnicodeGraphemeClusterBreak::SpacingMark;
        else if (testGraphemeClusterBreakSpanEquals(field, "L"))
            outValue = UnicodeGraphemeClusterBreak::L;
        else if (testGraphemeClusterBreakSpanEquals(field, "V"))
            outValue = UnicodeGraphemeClusterBreak::V;
        else if (testGraphemeClusterBreakSpanEquals(field, "T"))
            outValue = UnicodeGraphemeClusterBreak::T;
        else if (testGraphemeClusterBreakSpanEquals(field, "LV"))
            outValue = UnicodeGraphemeClusterBreak::LV;
        else if (testGraphemeClusterBreakSpanEquals(field, "LVT"))
            outValue = UnicodeGraphemeClusterBreak::LVT;
        else
            return false;

        return true;
    }


    // ========================================================================
    // Verify @missing metadata
    //
    // Unicode 17 contains exactly:
    //
    //      # @missing: 0000..10FFFF; Other
    //
    // ========================================================================

    static bool testGraphemeClusterBreakMissingMetadata(const ByteSpan& source, uint32_t& outCount)
    {
        outCount = 0;

        ByteSpan remaining = source;

        while (remaining)
        {
            ByteSpan line = bspan_read_until(remaining, '\n');

            bspan_trim_spaces(line);

            if (!line || *line != '#')
                continue;

            ++line;
            bspan_ltrim_spaces(line);

            if (!testGraphemeClusterBreakSpanStartsWith(line, "@missing:"))
                continue;

            ++outCount;

            if (!testGraphemeClusterBreakSpanEquals(line, "@missing: 0000..10FFFF; Other"))
                return false;
        }

        return true;
    }


    // ========================================================================
    // Build independent expected table from the actual UCD source
    //
    // This intentionally performs a second pass independent of
    // ucdParseGraphemeClusterBreak().
    // ========================================================================

    static bool testBuildExpectedGraphemeClusterBreakTable(const ByteSpan& source,
        std::vector<uint8_t>& expected, UnicodeCoverageBuilder& explicitCoverage,
        uint32_t& outRangeCount, size_t& outExplicitCodePoints)
    {
        expected.assign(
            kUnicodeLimit,
            static_cast<uint8_t>(UnicodeGraphemeClusterBreak::Other));

        outRangeCount = 0;
        outExplicitCodePoints = 0;

        UCDParser parser(source);
        UCDLine line;

        while (parser.next(line))
        {
            ByteSpan fields = line.data;

            UCDCodePointRange range;

            if (!ucdReadCodePointRange(fields, range))
                return false;

            ByteSpan classField;

            if (!ucdReadField(fields, classField) || !classField)
                return false;

            bspan_trim_spaces(fields);

            if (fields)
                return false;

            UnicodeGraphemeClusterBreak breakClass;

            if (!testGraphemeClusterBreakClassFromField(classField, breakClass))
                return false;

            for (uint32_t cp = range.first;; ++cp)
            {
                if (explicitCoverage.contains(cp))
                    return false;

                expected[cp] = static_cast<uint8_t>(breakClass);

                if (cp == range.last)
                    break;
            }

            explicitCoverage.addRange(range.first, range.last);

            ++outRangeCount;

            outExplicitCodePoints +=
                static_cast<size_t>(range.last - range.first + 1u);
        }

        return true;
    }


    // ========================================================================
    // testUnicodeGraphemeClusterBreakParser
    // ========================================================================

    static bool testUnicodeGraphemeClusterBreakParser(const ByteSpan& source)
    {
        auto fail = [](const char* message) -> bool {
            std::printf(
                "Unicode Grapheme_Cluster_Break parser: FAIL: %s\n",
                message);

            return false;
            };


        if (!source)
            return fail("source is empty");


        // ====================================================================
        // Verify source @missing declaration
        // ====================================================================

        uint32_t missingRangeCount = 0;

        if (!testGraphemeClusterBreakMissingMetadata(source, missingRangeCount))
            return fail("invalid @missing declaration");

        if (missingRangeCount != 1)
            return fail("unexpected @missing declaration count");


        // ====================================================================
        // Independently construct exact expected table
        // ====================================================================

        std::vector<uint8_t> expected;
        UnicodeCoverageBuilder explicitCoverage;

        uint32_t sourceRangeCount = 0;
        size_t sourceExplicitCodePoints = 0;

        if (!testBuildExpectedGraphemeClusterBreakTable(
            source, expected, explicitCoverage,
            sourceRangeCount, sourceExplicitCodePoints))
        {
            return fail("unable to independently construct expected table");
        }


        if (expected.size() != static_cast<size_t>(kUnicodeLimit))
            return fail("expected table has incorrect size");


        // ====================================================================
        // Run parser under test
        // ====================================================================

        auto values = std::make_unique<UnicodeValueTable8Builder>();

        UCDGraphemeClusterBreakParseResult result;

        if (!ucdParseGraphemeClusterBreak(source, *values, result))
        {
            std::printf(
                "Unicode Grapheme_Cluster_Break parser: FAIL\n"
                "  Error: %s\n"
                "  Line:  %u\n",
                ucdGraphemeClusterBreakParseErrorString(result.error),
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

        if (result.rangeCount != sourceRangeCount)
            return fail("explicit range count does not match source");

        if (result.explicitCodePoints != sourceExplicitCodePoints)
            return fail("explicit code-point count does not match source");

        if (result.explicitCodePoints + result.defaultedCodePoints !=
            static_cast<size_t>(kUnicodeLimit))
        {
            return fail("explicit plus defaulted code points do not cover Unicode");
        }


        // ====================================================================
        // Exhaustive full-table comparison
        //
        // Every Unicode code-point position must exactly match the independently
        // constructed expected table.
        // ====================================================================

        std::array<size_t, kUnicodeGraphemeClusterBreakCount> expectedTotals{};
        std::array<size_t, kUnicodeGraphemeClusterBreakCount> actualTotals{};

        size_t unicodeCodePointsChecked = 0;

        for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
        {
            const uint8_t expectedValue = expected[cp];
            const uint8_t actualValue = values->value(cp);

            if (!unicodeGraphemeClusterBreakIsValid(expectedValue))
            {
                std::printf(
                    "Unicode Grapheme_Cluster_Break parser: FAIL\n"
                    "  Invalid expected value: %u\n"
                    "  Code point:             U+%04X\n",
                    expectedValue,
                    cp);

                return false;
            }

            if (!unicodeGraphemeClusterBreakIsValid(actualValue))
            {
                std::printf(
                    "Unicode Grapheme_Cluster_Break parser: FAIL\n"
                    "  Invalid parser value: %u\n"
                    "  Code point:           U+%04X\n",
                    actualValue,
                    cp);

                return false;
            }

            if (actualValue != expectedValue)
            {
                std::printf(
                    "Unicode Grapheme_Cluster_Break parser: FAIL\n"
                    "  Code point: U+%04X\n"
                    "  Expected:   %u\n"
                    "  Actual:     %u\n",
                    cp,
                    expectedValue,
                    actualValue);

                return false;
            }

            ++expectedTotals[expectedValue];
            ++actualTotals[actualValue];
            ++unicodeCodePointsChecked;
        }


        if (unicodeCodePointsChecked != static_cast<size_t>(kUnicodeLimit))
            return fail("Unicode address-space accounting failed");


        // ====================================================================
        // Class population verification
        // ====================================================================

        size_t expectedTotal = 0;
        size_t actualTotal = 0;
        uint32_t classesMatched = 0;

        for (uint32_t i = 0; i < kUnicodeGraphemeClusterBreakCount; ++i)
        {
            if (expectedTotals[i] != actualTotals[i])
            {
                std::printf(
                    "Unicode Grapheme_Cluster_Break parser: FAIL\n"
                    "  Class value: %u\n"
                    "  Expected:    %zu\n"
                    "  Actual:      %zu\n",
                    i,
                    expectedTotals[i],
                    actualTotals[i]);

                return false;
            }

            if (actualTotals[i] == 0)
            {
                std::printf(
                    "Unicode Grapheme_Cluster_Break parser: FAIL\n"
                    "  Break class %u has zero population\n",
                    i);

                return false;
            }

            expectedTotal += expectedTotals[i];
            actualTotal += actualTotals[i];

            ++classesMatched;
        }


        if (expectedTotal != static_cast<size_t>(kUnicodeLimit))
            return fail("expected class populations do not cover Unicode");

        if (actualTotal != static_cast<size_t>(kUnicodeLimit))
            return fail("actual class populations do not cover Unicode");


        // ====================================================================
        // Semantic spot checks
        //
        // Exercise every one of the 14 current GCB values.
        // ====================================================================

        struct SpotCheck
        {
            uint32_t cp;
            UnicodeGraphemeClusterBreak expected;
        };


        static constexpr SpotCheck spotChecks[] =
        {
            { 0x0041u,  UnicodeGraphemeClusterBreak::Other },
            { 0x000Du,  UnicodeGraphemeClusterBreak::CR },
            { 0x000Au,  UnicodeGraphemeClusterBreak::LF },
            { 0x0000u,  UnicodeGraphemeClusterBreak::Control },
            { 0x0301u,  UnicodeGraphemeClusterBreak::Extend },
            { 0x200Du,  UnicodeGraphemeClusterBreak::ZWJ },
            { 0x1F1E6u, UnicodeGraphemeClusterBreak::RegionalIndicator },
            { 0x0600u,  UnicodeGraphemeClusterBreak::Prepend },
            { 0x0903u,  UnicodeGraphemeClusterBreak::SpacingMark },
            { 0x1100u,  UnicodeGraphemeClusterBreak::L },
            { 0x1161u,  UnicodeGraphemeClusterBreak::V },
            { 0x11A8u,  UnicodeGraphemeClusterBreak::T },
            { 0xAC00u,  UnicodeGraphemeClusterBreak::LV },
            { 0xAC01u,  UnicodeGraphemeClusterBreak::LVT }
        };


        size_t semanticSpotChecks = 0;

        for (const SpotCheck& check : spotChecks)
        {
            const UnicodeGraphemeClusterBreak actual =
                static_cast<UnicodeGraphemeClusterBreak>(values->value(check.cp));

            if (actual != check.expected)
            {
                std::printf(
                    "Unicode Grapheme_Cluster_Break parser: FAIL\n"
                    "  Semantic spot check: U+%04X\n"
                    "  Expected value:      %u\n"
                    "  Actual value:        %u\n",
                    check.cp,
                    static_cast<unsigned>(check.expected),
                    static_cast<unsigned>(actual));

                return false;
            }

            ++semanticSpotChecks;
        }


        // ====================================================================
        // Prove Other is genuinely coming from the default path.
        // ====================================================================

        if (explicitCoverage.contains(0x0041u))
            return fail("Other default fixture U+0041 is explicitly assigned");

        if (values->value(0x0041u) !=
            static_cast<uint8_t>(UnicodeGraphemeClusterBreak::Other))
        {
            return fail("default Other assignment failed for U+0041");
        }


        // ====================================================================
        // Failure-path checks
        // ====================================================================

        size_t failureChecks = 0;


        // --------------------------------------------------------------------
        // Unknown break class
        // --------------------------------------------------------------------

        {
            static constexpr char text[] =
                "0041 ; Unknown_Break_Class\n";

            UCDGraphemeClusterBreakParseResult failureResult;

            if (ucdParseGraphemeClusterBreak(ByteSpan(text), *values, failureResult) ||
                failureResult.error != UCDGraphemeClusterBreakParseError::UnknownBreakClass)
            {
                return fail("unknown break class was not rejected");
            }

            ++failureChecks;
        }


        // --------------------------------------------------------------------
        // Overlapping explicit ranges
        // --------------------------------------------------------------------

        {
            static constexpr char text[] =
                "0041..005A ; Other\n"
                "0050..0060 ; Other\n";

            UCDGraphemeClusterBreakParseResult failureResult;

            if (ucdParseGraphemeClusterBreak(ByteSpan(text), *values, failureResult) ||
                failureResult.error != UCDGraphemeClusterBreakParseError::OverlappingRange)
            {
                return fail("overlapping explicit ranges were not rejected");
            }

            ++failureChecks;
        }


        // --------------------------------------------------------------------
        // Missing break class
        // --------------------------------------------------------------------

        {
            static constexpr char text[] =
                "0041 ;\n";

            UCDGraphemeClusterBreakParseResult failureResult;

            if (ucdParseGraphemeClusterBreak(ByteSpan(text), *values, failureResult) ||
                failureResult.error != UCDGraphemeClusterBreakParseError::MissingBreakClass)
            {
                return fail("missing break class was not rejected");
            }

            ++failureChecks;
        }


        // --------------------------------------------------------------------
        // Unexpected extra field
        // --------------------------------------------------------------------

        {
            static constexpr char text[] =
                "0041 ; Other ; Extra\n";

            UCDGraphemeClusterBreakParseResult failureResult;

            if (ucdParseGraphemeClusterBreak(ByteSpan(text), *values, failureResult) ||
                failureResult.error != UCDGraphemeClusterBreakParseError::UnexpectedField)
            {
                return fail("unexpected extra field was not rejected");
            }

            ++failureChecks;
        }


        // ====================================================================
        // PASS
        // ====================================================================

        std::printf(
            "Unicode Grapheme_Cluster_Break parser: PASS\n"
            "  Source bytes:              %zu\n"
            "  Explicit ranges:           %u\n"
            "  @missing ranges:           %u\n"
            "  Explicit code points:      %zu\n"
            "  Defaulted code points:     %zu\n"
            "  Unicode code points:       %zu\n"
            "  Break classes:             %u\n"
            "  Class totals matched:      %u\n"
            "  Semantic spot checks:      %zu\n"
            "  Failure-path checks:       %zu\n"
            "  Full table comparison:     PASS\n",
            source.size(),
            result.rangeCount,
            missingRangeCount,
            result.explicitCodePoints,
            result.defaultedCodePoints,
            unicodeCodePointsChecked,
            static_cast<unsigned>(kUnicodeGraphemeClusterBreakCount),
            classesMatched,
            semanticSpotChecks,
            failureChecks);


        return true;
    }


    // ========================================================================
    // Filename convenience overload
    // ========================================================================

    static bool testUnicodeGraphemeClusterBreakParser(const char* filename)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(filename, fileData))
        {
            std::printf(
                "Unicode Grapheme_Cluster_Break parser: FAIL\n"
                "  Unable to read file: %s\n",
                filename ? filename : "(null)");

            return false;
        }

        const ByteSpan source(fileData.data(), fileData.size());

        return testUnicodeGraphemeClusterBreakParser(source);
    }

} // namespace waavs

