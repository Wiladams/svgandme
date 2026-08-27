// test_ucd_normalization_props_parser.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

#include "ucd_normalization_props_parser.h"


namespace waavs
{
    // ========================================================================
    // testUCDNormalizationPropsParser
    // ========================================================================

    static bool testUCDNormalizationPropsParser(const char* filename)
    {
        auto fail = [](const char* message) -> bool {
            std::printf(
                "DerivedNormalizationProps.txt parser: FAIL: %s\n",
                message);

            return false;
            };


        // ====================================================================
        // Load DerivedNormalizationProps.txt
        // ====================================================================

        std::ifstream input(
            filename,
            std::ios::binary |
            std::ios::ate);

        if (!input)
            return fail("unable to open DerivedNormalizationProps.txt");


        const std::streampos end =
            input.tellg();

        if (end <= 0)
            return fail("DerivedNormalizationProps.txt is empty");


        std::vector<uint8_t> sourceBytes(
            static_cast<size_t>(end));

        input.seekg(
            0,
            std::ios::beg);

        input.read(
            reinterpret_cast<char*>(
                sourceBytes.data()),
            static_cast<std::streamsize>(
                sourceBytes.size()));

        if (!input)
            return fail("unable to read DerivedNormalizationProps.txt");


        const ByteSpan source(
            sourceBytes.data(),
            sourceBytes.size());


        // ====================================================================
        // Parse Full_Composition_Exclusion
        // ====================================================================

        UnicodeCoverageBuilder exclusion;
        UCDNormalizationPropsParseResult result;


        if (!ucdParseFullCompositionExclusion(
            source,
            exclusion,
            result))
        {
            std::printf(
                "DerivedNormalizationProps.txt parser: FAIL\n"
                "  Line:  %u\n"
                "  Error: %s\n",
                result.lineNumber,
                ucdNormalizationPropsParseErrorString(
                    result.error));

            return false;
        }


        // ====================================================================
        // Structural checks
        //
        // Unicode 17 Full_Composition_Exclusion:
        //
        //      ranges:      74
        //      code points: 1120
        // ====================================================================

        if (result.rangeCount != 74)
        {
            std::printf(
                "DerivedNormalizationProps.txt parser: FAIL\n"
                "  Expected exclusion ranges: 74\n"
                "  Actual exclusion ranges:   %u\n",
                result.rangeCount);

            return false;
        }


        if (result.codePointCount != 1120)
        {
            std::printf(
                "DerivedNormalizationProps.txt parser: FAIL\n"
                "  Expected excluded code points: 1120\n"
                "  Actual excluded code points:   %zu\n",
                result.codePointCount);

            return false;
        }


        if (result.recordCount == 0)
            return fail("no records examined");


        if (exclusion.empty())
            return fail("exclusion coverage is empty");


        // ====================================================================
        // Positive spot checks
        //
        // These code points are members of Full_Composition_Exclusion.
        // ====================================================================

        const uint32_t expectedExcluded[] = {
            0x0340,
            0x0341,
            0x0343,
            0x0344,
            0x0374,
            0x0958,
            0x2126,
            0x212A,
            0x212B,
            0xF900,
            0x2F800,
            0x2FA1D
        };


        for (uint32_t cp : expectedExcluded)
        {
            if (!exclusion.contains(cp))
            {
                std::printf(
                    "DerivedNormalizationProps.txt parser: FAIL\n"
                    "  Expected excluded code point missing: U+%04X\n",
                    cp);

                return false;
            }
        }


        // ====================================================================
        // Negative spot checks
        //
        // Ordinary canonically composable characters should not be excluded.
        // ====================================================================

        const uint32_t expectedComposable[] = {
            0x0041,
            0x0061,
            0x00C0,
            0x00C1,
            0x00C5,
            0x00E9,
            0x01FA
        };


        for (uint32_t cp : expectedComposable)
        {
            if (exclusion.contains(cp))
            {
                std::printf(
                    "DerivedNormalizationProps.txt parser: FAIL\n"
                    "  Unexpected excluded code point: U+%04X\n",
                    cp);

                return false;
            }
        }


        // ====================================================================
        // Boundary checks
        // ====================================================================

        if (exclusion.contains(0x033F))
            return fail("U+033F unexpectedly excluded");

        if (!exclusion.contains(0x0340))
            return fail("U+0340 unexpectedly absent");

        if (!exclusion.contains(0x0341))
            return fail("U+0341 unexpectedly absent");

        if (exclusion.contains(0x0342))
            return fail("U+0342 unexpectedly excluded");


        if (!exclusion.contains(0x2FA1D))
            return fail("last expected exclusion U+2FA1D absent");

        if (exclusion.contains(0x2FA1E))
            return fail("U+2FA1E unexpectedly excluded");


        // ====================================================================
        // Invalid range
        // ====================================================================

        {
            const char malformed[] =
                "0041..0030 ; Full_Composition_Exclusion\n";

            UnicodeCoverageBuilder malformedCoverage;
            UCDNormalizationPropsParseResult malformedResult;


            if (ucdParseFullCompositionExclusion(
                ByteSpan(malformed),
                malformedCoverage,
                malformedResult))
            {
                return fail("invalid range unexpectedly accepted");
            }


            if (malformedResult.error !=
                UCDNormalizationPropsParseError::InvalidRange)
            {
                return fail("invalid range returned wrong error");
            }


            if (!malformedCoverage.empty())
                return fail("invalid range modified output coverage");
        }


        // ====================================================================
        // Overlapping ranges
        // ====================================================================

        {
            const char malformed[] =
                "0340..0343 ; Full_Composition_Exclusion\n"
                "0343..0344 ; Full_Composition_Exclusion\n";

            UnicodeCoverageBuilder malformedCoverage;
            UCDNormalizationPropsParseResult malformedResult;


            if (ucdParseFullCompositionExclusion(
                ByteSpan(malformed),
                malformedCoverage,
                malformedResult))
            {
                return fail("overlapping ranges unexpectedly accepted");
            }


            if (malformedResult.error !=
                UCDNormalizationPropsParseError::OverlappingRange)
            {
                return fail("overlap returned wrong error");
            }


            if (!malformedCoverage.empty())
                return fail("overlap failure modified output coverage");
        }


        // ====================================================================
        // Missing property
        // ====================================================================

        {
            const char malformed[] =
                "0041 ; NFC_QC ; N\n"
                "0042 ; NFD_QC ; N\n";

            UnicodeCoverageBuilder malformedCoverage;
            UCDNormalizationPropsParseResult malformedResult;


            if (ucdParseFullCompositionExclusion(
                ByteSpan(malformed),
                malformedCoverage,
                malformedResult))
            {
                return fail(
                    "missing Full_Composition_Exclusion unexpectedly accepted");
            }


            if (malformedResult.error !=
                UCDNormalizationPropsParseError::
                MissingFullCompositionExclusion)
            {
                return fail("missing property returned wrong error");
            }


            if (!malformedCoverage.empty())
                return fail("missing property modified output coverage");
        }


        // ====================================================================
        // Ignore unrelated properties with additional fields
        //
        // This is important because DerivedNormalizationProps.txt is not a
        // uniform two-field file.
        // ====================================================================

        {
            const char synthetic[] =
                "0041 ; NFC_QC ; N\n"
                "0042 ; NFKC_QC ; M\n"
                "0043 ; FC_NFKC ; 0061 0062\n"
                "0340 ; Full_Composition_Exclusion\n";

            UnicodeCoverageBuilder syntheticCoverage;
            UCDNormalizationPropsParseResult syntheticResult;


            if (!ucdParseFullCompositionExclusion(
                ByteSpan(synthetic),
                syntheticCoverage,
                syntheticResult))
            {
                return fail(
                    "unrelated property fields caused parser failure");
            }


            if (syntheticResult.rangeCount != 1 ||
                syntheticResult.codePointCount != 1)
            {
                return fail(
                    "synthetic exclusion accounting incorrect");
            }


            if (!syntheticCoverage.contains(0x0340))
                return fail("synthetic exclusion missing U+0340");


            if (syntheticCoverage.contains(0x0041) ||
                syntheticCoverage.contains(0x0042) ||
                syntheticCoverage.contains(0x0043))
            {
                return fail(
                    "unrelated properties entered exclusion coverage");
            }
        }


        // ====================================================================
        // Transactional output behavior
        //
        // Seed the destination with a known value, then attempt a parse that
        // fails after encountering a valid exclusion record.
        //
        // The caller's original coverage must remain intact.
        // ====================================================================

        {
            UnicodeCoverageBuilder transactionalCoverage;

            transactionalCoverage.add(
                0x1234);


            const char malformed[] =
                "0340 ; Full_Composition_Exclusion\n"
                "0340 ; Full_Composition_Exclusion\n";

            UCDNormalizationPropsParseResult malformedResult;


            if (ucdParseFullCompositionExclusion(
                ByteSpan(malformed),
                transactionalCoverage,
                malformedResult))
            {
                return fail(
                    "transactional failure case unexpectedly succeeded");
            }


            if (malformedResult.error !=
                UCDNormalizationPropsParseError::OverlappingRange)
            {
                return fail(
                    "transactional failure returned wrong error");
            }


            if (!transactionalCoverage.contains(0x1234))
                return fail(
                    "failed parse destroyed original output coverage");


            if (transactionalCoverage.contains(0x0340))
                return fail(
                    "failed parse partially committed new coverage");
        }


        // ====================================================================
        // PASS
        // ====================================================================

        std::printf(
            "DerivedNormalizationProps.txt parser: PASS\n"
            "  Records examined:                   %u\n"
            "  Full_Composition_Exclusion ranges:  %u\n"
            "  Excluded code points:               %zu\n"
            "  Positive spot checks:               %zu\n"
            "  Negative spot checks:               %zu\n"
            "  Boundary checks:                    PASS\n"
            "  Invalid range rejection:            PASS\n"
            "  Overlap rejection:                  PASS\n"
            "  Missing property rejection:         PASS\n"
            "  Unrelated property handling:        PASS\n"
            "  Transactional failure behavior:     PASS\n",
            result.recordCount,
            result.rangeCount,
            result.codePointCount,
            sizeof(expectedExcluded) /
            sizeof(expectedExcluded[0]),
            sizeof(expectedComposable) /
            sizeof(expectedComposable[0]));


        return true;
    }

} // namespace waavs
