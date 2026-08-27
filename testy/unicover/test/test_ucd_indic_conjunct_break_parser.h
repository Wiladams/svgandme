// test_ucd_indic_conjunct_break_parser.h
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#include "test_core.h"
#include "ucd_indic_conjunct_break_parser.h"


namespace waavs
{
    struct TestIndicConjunctBreakExpected
    {
        uint32_t rangeCount{ 0 };
        size_t explicitCodePoints{ 0 };
    };


    static bool testIndicConjunctBreakSpanEquals(const ByteSpan& span, const char* text) noexcept
    {
        const size_t length = std::strlen(text);

        return span.size() == length &&
            std::memcmp(span.data(), text, length) == 0;
    }


    static bool testIndicConjunctBreakValueFromField(const ByteSpan& field, uint8_t& outValue) noexcept
    {
        if (testIndicConjunctBreakSpanEquals(field, "None"))
        {
            outValue = static_cast<uint8_t>(UnicodeIndicConjunctBreak::None);
            return true;
        }

        if (testIndicConjunctBreakSpanEquals(field, "Consonant"))
        {
            outValue = static_cast<uint8_t>(UnicodeIndicConjunctBreak::Consonant);
            return true;
        }

        if (testIndicConjunctBreakSpanEquals(field, "Extend"))
        {
            outValue = static_cast<uint8_t>(UnicodeIndicConjunctBreak::Extend);
            return true;
        }

        if (testIndicConjunctBreakSpanEquals(field, "Linker"))
        {
            outValue = static_cast<uint8_t>(UnicodeIndicConjunctBreak::Linker);
            return true;
        }

        return false;
    }


    static bool testIndicConjunctBreakBuildExpected(const ByteSpan& source,
        std::vector<uint8_t>& expected,
        TestIndicConjunctBreakExpected& outResult)
    {
        expected.assign(
            kUnicodeLimit,
            static_cast<uint8_t>(UnicodeIndicConjunctBreak::None));

        std::vector<uint8_t> assigned(kUnicodeLimit, 0);

        outResult = {};

        UCDParser parser(source);
        UCDLine line;

        while (parser.next(line))
        {
            ByteSpan fields = line.data;
            UCDCodePointRange range;

            if (!ucdReadCodePointRange(fields, range))
            {
                std::printf(
                    "Unicode Indic_Conjunct_Break parser: FAIL\n"
                    "  Independent parser found invalid range\n"
                    "  Line: %u\n",
                    line.lineNumber);

                return false;
            }

            ByteSpan propertyField;

            if (!ucdReadField(fields, propertyField) || !propertyField)
                continue;

            if (!testIndicConjunctBreakSpanEquals(propertyField, "InCB"))
                continue;

            ByteSpan valueField;

            if (!ucdReadField(fields, valueField) || !valueField)
            {
                std::printf(
                    "Unicode Indic_Conjunct_Break parser: FAIL\n"
                    "  Independent parser found missing value\n"
                    "  Line: %u\n",
                    line.lineNumber);

                return false;
            }

            bspan_trim_spaces(fields);

            if (fields)
            {
                std::printf(
                    "Unicode Indic_Conjunct_Break parser: FAIL\n"
                    "  Independent parser found unexpected field\n"
                    "  Line: %u\n",
                    line.lineNumber);

                return false;
            }

            uint8_t value = 0;

            if (!testIndicConjunctBreakValueFromField(valueField, value))
            {
                std::printf(
                    "Unicode Indic_Conjunct_Break parser: FAIL\n"
                    "  Independent parser found unknown value\n"
                    "  Line: %u\n",
                    line.lineNumber);

                return false;
            }

            for (uint32_t cp = range.first;; ++cp)
            {
                if (assigned[cp] != 0)
                {
                    std::printf(
                        "Unicode Indic_Conjunct_Break parser: FAIL\n"
                        "  Independent parser found overlapping range\n"
                        "  Line: %u\n"
                        "  Code point: U+%04X\n",
                        line.lineNumber,
                        cp);

                    return false;
                }

                assigned[cp] = 1;
                expected[cp] = value;

                ++outResult.explicitCodePoints;

                if (cp == range.last)
                    break;
            }

            ++outResult.rangeCount;
        }

        if (outResult.rangeCount == 0)
        {
            std::printf(
                "Unicode Indic_Conjunct_Break parser: FAIL\n"
                "  No InCB records found\n");

            return false;
        }

        return true;
    }


    static bool testUCDIndicConjunctBreakParser(const ByteSpan& source)
    {
        auto fail = [](const char* message) -> bool
            {
                std::printf(
                    "Unicode Indic_Conjunct_Break parser: FAIL\n"
                    "  %s\n",
                    message);

                return false;
            };


        // ====================================================================
        // Build independent expected table
        // ====================================================================

        std::vector<uint8_t> expected;
        TestIndicConjunctBreakExpected expectedResult;

        if (!testIndicConjunctBreakBuildExpected(
            source, expected, expectedResult))
        {
            return false;
        }


        // ====================================================================
        // Run production parser
        //
        // UnicodeValueTable8Builder is intentionally about 1.1 MiB, so keep
        // it off the stack.
        // ====================================================================

        auto values = std::make_unique<UnicodeValueTable8Builder>();

        UCDIndicConjunctBreakParseResult result;

        if (!ucdParseIndicConjunctBreak(
            source, *values, result))
        {
            std::printf(
                "Unicode Indic_Conjunct_Break parser: FAIL\n"
                "  Error: %s\n"
                "  Line:  %u\n",
                ucdIndicConjunctBreakParseErrorString(result.error),
                result.lineNumber);

            return false;
        }


        // ====================================================================
        // Parser accounting
        // ====================================================================

        if (result.rangeCount != expectedResult.rangeCount)
        {
            std::printf(
                "Unicode Indic_Conjunct_Break parser: FAIL\n"
                "  Range count mismatch\n"
                "  Expected: %u\n"
                "  Actual:   %u\n",
                expectedResult.rangeCount,
                result.rangeCount);

            return false;
        }

        if (result.explicitCodePoints != expectedResult.explicitCodePoints)
        {
            std::printf(
                "Unicode Indic_Conjunct_Break parser: FAIL\n"
                "  Explicit code-point count mismatch\n"
                "  Expected: %zu\n"
                "  Actual:   %zu\n",
                expectedResult.explicitCodePoints,
                result.explicitCodePoints);

            return false;
        }

        if (result.explicitCodePoints +
            result.defaultedCodePoints != kUnicodeLimit)
        {
            return fail(
                "explicit plus defaulted code points do not cover Unicode");
        }


        // ====================================================================
        // Unicode 17 population invariants
        // ====================================================================

        if (result.explicitCodePoints != 3148)
            return fail("unexpected explicit InCB code-point count");

        if (result.defaultedCodePoints != 1110964)
            return fail("unexpected defaulted InCB code-point count");


        // ====================================================================
        // Exhaustive comparison
        // ====================================================================

        size_t mismatchCount = 0;
        uint32_t firstMismatch = kUnicodeLimit;

        for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
        {
            const uint8_t expectedValue = expected[cp];
            const uint8_t actualValue = values->value(cp);

            if (expectedValue == actualValue)
                continue;

            if (mismatchCount == 0)
                firstMismatch = cp;

            ++mismatchCount;
        }

        if (mismatchCount != 0)
        {
            std::printf(
                "Unicode Indic_Conjunct_Break parser: FAIL\n"
                "  Full-table mismatches: %zu\n"
                "  First mismatch: U+%04X\n"
                "  Expected: %u\n"
                "  Actual:   %u\n",
                mismatchCount,
                firstMismatch,
                static_cast<unsigned>(expected[firstMismatch]),
                static_cast<unsigned>(values->value(firstMismatch)));

            return false;
        }


        // ====================================================================
        // Class populations
        // ====================================================================

        size_t classTotals[kUnicodeIndicConjunctBreakCount]{};

        for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
        {
            const uint8_t value = values->value(cp);

            if (!unicodeIndicConjunctBreakIsValid(value))
            {
                std::printf(
                    "Unicode Indic_Conjunct_Break parser: FAIL\n"
                    "  Invalid class value at U+%04X: %u\n",
                    cp,
                    static_cast<unsigned>(value));

                return false;
            }

            ++classTotals[value];
        }

        static constexpr size_t expectedClassTotals[
            kUnicodeIndicConjunctBreakCount] =
            {
                1110964,   // None
                911,       // Consonant
                2217,      // Extend
                20         // Linker
            };

            for (uint32_t i = 0; i < kUnicodeIndicConjunctBreakCount; ++i)
            {
                if (classTotals[i] == expectedClassTotals[i])
                    continue;

                std::printf(
                    "Unicode Indic_Conjunct_Break parser: FAIL\n"
                    "  Class population mismatch\n"
                    "  Class:    %u\n"
                    "  Expected: %zu\n"
                    "  Actual:   %zu\n",
                    i,
                    expectedClassTotals[i],
                    classTotals[i]);

                return false;
            }


            // ====================================================================
            // Semantic spot checks
            // ====================================================================

            struct SpotCheck
            {
                uint32_t cp;
                UnicodeIndicConjunctBreak expected;
                const char* description;
            };

            static constexpr SpotCheck spots[] =
            {
                {
                    0x0041,
                    UnicodeIndicConjunctBreak::None,
                    "ASCII A"
                },
                {
                    0x0915,
                    UnicodeIndicConjunctBreak::Consonant,
                    "DEVANAGARI LETTER KA"
                },
                {
                    0x094D,
                    UnicodeIndicConjunctBreak::Linker,
                    "DEVANAGARI SIGN VIRAMA"
                },
                {
                    0x0300,
                    UnicodeIndicConjunctBreak::Extend,
                    "COMBINING GRAVE ACCENT"
                }
            };

            size_t spotCheckCount = 0;

            for (const SpotCheck& spot : spots)
            {
                const uint8_t actual = values->value(spot.cp);
                const uint8_t expectedValue =
                    static_cast<uint8_t>(spot.expected);

                if (actual != expectedValue)
                {
                    std::printf(
                        "Unicode Indic_Conjunct_Break parser: FAIL\n"
                        "  Spot check failed\n"
                        "  U+%04X %s\n"
                        "  Expected: %u\n"
                        "  Actual:   %u\n",
                        spot.cp,
                        spot.description,
                        static_cast<unsigned>(expectedValue),
                        static_cast<unsigned>(actual));

                    return false;
                }

                ++spotCheckCount;
            }


            // ====================================================================
            // Selective parsing
            //
            // Unrelated DerivedCoreProperties records must be ignored.
            // ====================================================================

            size_t selectiveCheckCount = 0;

            {
                static constexpr char text[] =
                    "0041 ; Math\n"
                    "0915 ; InCB ; Consonant\n"
                    "0061 ; Alphabetic\n";

                const ByteSpan testSource(
                    reinterpret_cast<const uint8_t*>(text),
                    sizeof(text) - 1u);

                auto testValues =
                    std::make_unique<UnicodeValueTable8Builder>();

                UCDIndicConjunctBreakParseResult testResult;

                if (!ucdParseIndicConjunctBreak(
                    testSource, *testValues, testResult))
                {
                    return fail(
                        "parser rejected unrelated DerivedCoreProperties records");
                }

                if (testResult.rangeCount != 1 ||
                    testResult.explicitCodePoints != 1)
                {
                    return fail(
                        "selective parsing produced incorrect accounting");
                }

                if (testValues->value(0x0915) !=
                    static_cast<uint8_t>(
                        UnicodeIndicConjunctBreak::Consonant))
                {
                    return fail(
                        "selective parsing lost InCB record");
                }

                if (testValues->value(0x0041) !=
                    static_cast<uint8_t>(
                        UnicodeIndicConjunctBreak::None))
                {
                    return fail(
                        "unrelated property modified InCB table");
                }

                ++selectiveCheckCount;
            }


            // ====================================================================
            // Failure-path tests
            // ====================================================================

            size_t failurePathChecks = 0;

            auto expectFailure = [&](const char* text,
                UCDIndicConjunctBreakParseError expectedError,
                const char* description) -> bool
                {
                    const ByteSpan testSource(
                        reinterpret_cast<const uint8_t*>(text),
                        std::strlen(text));

                    auto testValues =
                        std::make_unique<UnicodeValueTable8Builder>();

                    UCDIndicConjunctBreakParseResult testResult;

                    if (ucdParseIndicConjunctBreak(
                        testSource, *testValues, testResult))
                    {
                        std::printf(
                            "Unicode Indic_Conjunct_Break parser: FAIL\n"
                            "  Failure-path test unexpectedly succeeded\n"
                            "  Test: %s\n",
                            description);

                        return false;
                    }

                    if (testResult.error != expectedError)
                    {
                        std::printf(
                            "Unicode Indic_Conjunct_Break parser: FAIL\n"
                            "  Failure-path returned wrong error\n"
                            "  Test:     %s\n"
                            "  Expected: %s\n"
                            "  Actual:   %s\n",
                            description,
                            ucdIndicConjunctBreakParseErrorString(expectedError),
                            ucdIndicConjunctBreakParseErrorString(testResult.error));

                        return false;
                    }

                    ++failurePathChecks;
                    return true;
                };


            if (!expectFailure(
                "ZZZZ ; InCB ; Extend\n",
                UCDIndicConjunctBreakParseError::InvalidRange,
                "invalid range"))
            {
                return false;
            }

            if (!expectFailure(
                "0041 ; InCB\n",
                UCDIndicConjunctBreakParseError::MissingValue,
                "missing value"))
            {
                return false;
            }

            if (!expectFailure(
                "0041 ; InCB ; Bogus\n",
                UCDIndicConjunctBreakParseError::UnknownValue,
                "unknown value"))
            {
                return false;
            }

            if (!expectFailure(
                "0041 ; InCB ; Extend ; Extra\n",
                UCDIndicConjunctBreakParseError::UnexpectedField,
                "unexpected field"))
            {
                return false;
            }

            if (!expectFailure(
                "0915..0916 ; InCB ; Consonant\n"
                "0916 ; InCB ; Extend\n",
                UCDIndicConjunctBreakParseError::OverlappingRange,
                "overlapping ranges"))
            {
                return false;
            }

            if (!expectFailure(
                "0041 ; Math\n"
                "0061 ; Alphabetic\n",
                UCDIndicConjunctBreakParseError::NoPropertyData,
                "no InCB records"))
            {
                return false;
            }


            // ====================================================================
            // PASS
            // ====================================================================

            std::printf(
                "Unicode Indic_Conjunct_Break parser: PASS\n"
                "  Source bytes:              %zu\n"
                "  Explicit ranges:           %u\n"
                "  Explicit code points:      %zu\n"
                "  Defaulted code points:     %zu\n"
                "  Unicode code points:       %u\n"
                "  Break classes:             %u\n"
                "  Class totals matched:      %u\n"
                "  Semantic spot checks:      %zu\n"
                "  Selective-source checks:   %zu\n"
                "  Failure-path checks:       %zu\n"
                "  Full table comparison:     PASS\n",
                source.size(),
                result.rangeCount,
                result.explicitCodePoints,
                result.defaultedCodePoints,
                static_cast<unsigned>(kUnicodeLimit),
                static_cast<unsigned>(
                    kUnicodeIndicConjunctBreakCount),
                static_cast<unsigned>(
                    kUnicodeIndicConjunctBreakCount),
                spotCheckCount,
                selectiveCheckCount,
                failurePathChecks);

            return true;
    }


    static bool testUCDIndicConjunctBreakParser(const char* filename)
    {
        std::vector<uint8_t> fileData;

        if (!readFileData(filename, fileData))
        {
            std::printf(
                "Unicode Indic_Conjunct_Break parser: FAIL\n"
                "  Unable to read file: %s\n",
                filename);

            return false;
        }

        const ByteSpan source(
            fileData.data(),
            fileData.size());

        return testUCDIndicConjunctBreakParser(source);
    }

} // namespace waavs
