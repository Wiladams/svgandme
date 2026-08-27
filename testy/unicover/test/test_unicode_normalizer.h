// test_unicode_normalizer.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <vector>

#include "unicode_normalizer.h"
#include "unicode_database.h"


namespace waavs
{
    // ========================================================================
    // unicodeNormalizerEquals
    // ========================================================================

    static bool unicodeNormalizerEquals(
        const uint32_t* actual,
        size_t actualCount,
        std::initializer_list<uint32_t> expected) noexcept
    {
        if (actualCount != expected.size())
            return false;

        size_t index = 0;

        for (uint32_t cp : expected) {
            if (actual[index++] != cp)
                return false;
        }

        return true;
    }


    // ========================================================================
    // testUnicodeNormalizer
    // ========================================================================

    static bool testUnicodeNormalizer(const char* databaseFilename)
    {
        auto fail = [](const char* message) -> bool {
            std::printf(
                "Unicode normalizer NFD: FAIL: %s\n",
                message);

            return false;
            };


        // ====================================================================
        // Load database
        // ====================================================================

        std::ifstream input(
            databaseFilename,
            std::ios::binary |
            std::ios::ate);

        if (!input)
            return fail("unable to open Unicode database");


        const std::streampos end = input.tellg();

        if (end <= 0)
            return fail("Unicode database is empty");


        std::vector<uint8_t> databaseBytes(
            static_cast<size_t>(end));

        input.seekg(0, std::ios::beg);

        input.read(
            reinterpret_cast<char*>(databaseBytes.data()),
            static_cast<std::streamsize>(
                databaseBytes.size()));

        if (!input)
            return fail("unable to read Unicode database");


        UnicodeDatabase database;

        if (!database.reset(
            ByteSpan(
                databaseBytes.data(),
                databaseBytes.size())))
        {
            return fail("UnicodeDatabase rejected database");
        }


        UnicodeNormalizer normalizer(database);

        if (!normalizer)
            return fail("UnicodeNormalizer is invalid");


        // ====================================================================
        // Single code point - unchanged
        //
        //      U+0041 -> U+0041
        // ====================================================================

        {
            UnicodeCanonicalDecomposition result;

            if (!normalizer.nfd(
                0x0041,
                result))
            {
                return fail("single unchanged code point failed");
            }

            if (!unicodeNormalizerEquals(
                result.codePoints,
                result.count,
                { 0x0041 }))
            {
                return fail("single unchanged code point incorrect");
            }
        }


        // ====================================================================
        // Single code point - simple canonical decomposition
        //
        //      U+00E9 LATIN SMALL LETTER E WITH ACUTE
        //
        //      -> U+0065
        //         U+0301
        // ====================================================================

        {
            UnicodeCanonicalDecomposition result;

            if (!normalizer.nfd(
                0x00E9,
                result))
            {
                return fail("simple NFD decomposition failed");
            }

            if (!unicodeNormalizerEquals(
                result.codePoints,
                result.count,
                {
                    0x0065,
                    0x0301
                }))
            {
                return fail("simple NFD decomposition incorrect");
            }
        }


        // ====================================================================
        // Single code point - recursive canonical decomposition
        //
        //      U+01D5
        //
        //      -> U+00DC U+0304
        //      -> U+0055 U+0308 U+0304
        // ====================================================================

        {
            UnicodeCanonicalDecomposition result;

            if (!normalizer.nfd(
                0x01D5,
                result))
            {
                return fail("recursive NFD decomposition failed");
            }

            if (!unicodeNormalizerEquals(
                result.codePoints,
                result.count,
                {
                    0x0055,
                    0x0308,
                    0x0304
                }))
            {
                return fail("recursive NFD decomposition incorrect");
            }
        }


        // ====================================================================
        // Single code point - maximum current expansion
        //
        //      U+1F82
        //
        //      -> U+03B1 U+0313 U+0300 U+0345
        //
        // This demonstrates that the fixed four-code-point result is sufficient
        // for a single Unicode 17 input code point.
        // ====================================================================

        {
            UnicodeCanonicalDecomposition result;

            if (!normalizer.nfd(
                0x1F82,
                result))
            {
                return fail("maximum-length NFD decomposition failed");
            }

            if (!unicodeNormalizerEquals(
                result.codePoints,
                result.count,
                {
                    0x03B1,
                    0x0313,
                    0x0300,
                    0x0345
                }))
            {
                return fail("maximum-length NFD decomposition incorrect");
            }

            if (result.count !=
                kUnicodeCanonicalDecompositionMaxLength)
            {
                return fail("maximum-length result is not four code points");
            }
        }


        // ====================================================================
        // Compatibility decomposition must not occur in NFD
        //
        //      U+00A0 NO-BREAK SPACE
        //
        // has a compatibility mapping to U+0020, but NFD performs only
        // canonical decomposition.
        //
        // Therefore:
        //
        //      U+00A0 -> U+00A0
        // ====================================================================

        {
            UnicodeCanonicalDecomposition result;

            if (!normalizer.nfd(
                0x00A0,
                result))
            {
                return fail("compatibility exclusion test failed");
            }

            if (!unicodeNormalizerEquals(
                result.codePoints,
                result.count,
                { 0x00A0 }))
            {
                return fail("NFD incorrectly used compatibility mapping");
            }
        }


        // ====================================================================
        // Hangul
        //
        //      U+AC01 HANGUL SYLLABLE GAG
        //
        //      -> U+1100 U+1161 U+11A8
        // ====================================================================

        {
            UnicodeCanonicalDecomposition result;

            if (!normalizer.nfd(
                0xAC01,
                result))
            {
                return fail("Hangul NFD failed");
            }

            if (!unicodeNormalizerEquals(
                result.codePoints,
                result.count,
                {
                    0x1100,
                    0x1161,
                    0x11A8
                }))
            {
                return fail("Hangul NFD incorrect");
            }
        }


        // ====================================================================
        // Sequence - already decomposed and ordered
        //
        //      A U+0323 U+0300
        //
        // CCC:
        //
        //      0   220    230
        //
        // NFD must leave the sequence unchanged.
        // ====================================================================

        {
            const uint32_t source[] = {
                0x0041,
                0x0323,
                0x0300
            };

            uint32_t output[12]{};
            size_t outputCount = 0;

            constexpr size_t sourceCount =
                sizeof(source) / sizeof(source[0]);


            if (!normalizer.nfd(
                source,
                sourceCount,
                output,
                sizeof(output) / sizeof(output[0]),
                outputCount))
            {
                return fail("already-normalized sequence failed");
            }

            if (!unicodeNormalizerEquals(
                output,
                outputCount,
                {
                    0x0041,
                    0x0323,
                    0x0300
                }))
            {
                return fail("already-normalized sequence changed");
            }
        }


        // ====================================================================
        // Sequence - canonical ordering across source boundaries
        //
        // This is the important composition test.
        //
        // Source:
        //
        //      U+00C0 U+0323
        //
        // Decomposition:
        //
        //      U+00C0
        //          -> U+0041 U+0300
        //
        // producing:
        //
        //      U+0041 U+0300 U+0323
        //
        // CCC:
        //
        //          0       230      220
        //
        // Canonical ordering then produces:
        //
        //      U+0041 U+0323 U+0300
        //
        // The U+0323 originated from a different source code point but must
        // move ahead of U+0300.
        // ====================================================================

        {
            const uint32_t source[] = {
                0x00C0,
                0x0323
            };

            uint32_t output[8]{};
            size_t outputCount = 0;

            constexpr size_t sourceCount =
                sizeof(source) / sizeof(source[0]);


            if (!normalizer.nfd(
                source,
                sourceCount,
                output,
                sizeof(output) / sizeof(output[0]),
                outputCount))
            {
                return fail("cross-source NFD failed");
            }

            if (!unicodeNormalizerEquals(
                output,
                outputCount,
                {
                    0x0041,
                    0x0323,
                    0x0300
                }))
            {
                return fail("cross-source NFD ordering incorrect");
            }
        }


        // ====================================================================
        // Sequence - multiple decompositions
        //
        //      U+00C5 U+00E9
        //
        // becomes:
        //
        //      A + ring above
        //      e + acute
        //
        //      U+0041 U+030A U+0065 U+0301
        //
        // The second starter U+0065 naturally terminates the first combining
        // sequence.
        // ====================================================================

        {
            const uint32_t source[] = {
                0x00C5,
                0x00E9
            };

            uint32_t output[8]{};
            size_t outputCount = 0;

            constexpr size_t sourceCount =
                sizeof(source) / sizeof(source[0]);


            if (!normalizer.nfd(
                source,
                sourceCount,
                output,
                sizeof(output) / sizeof(output[0]),
                outputCount))
            {
                return fail("multiple decomposition NFD failed");
            }

            if (!unicodeNormalizerEquals(
                output,
                outputCount,
                {
                    0x0041,
                    0x030A,
                    0x0065,
                    0x0301
                }))
            {
                return fail("multiple decomposition NFD incorrect");
            }
        }


        // ====================================================================
        // Sequence - leading combining marks
        //
        // No starter is required for canonical ordering.
        //
        //      U+0315 U+0300 U+0323
        //
        // CCC:
        //
        //          232    230    220
        //
        // becomes:
        //
        //      U+0323 U+0300 U+0315
        // ====================================================================

        {
            const uint32_t source[] = {
                0x0315,
                0x0300,
                0x0323
            };

            uint32_t output[12]{};
            size_t outputCount = 0;

            constexpr size_t sourceCount =
                sizeof(source) / sizeof(source[0]);


            if (!normalizer.nfd(
                source,
                sourceCount,
                output,
                sizeof(output) / sizeof(output[0]),
                outputCount))
            {
                return fail("leading combining sequence NFD failed");
            }

            if (!unicodeNormalizerEquals(
                output,
                outputCount,
                {
                    0x0323,
                    0x0300,
                    0x0315
                }))
            {
                return fail("leading combining sequence NFD incorrect");
            }
        }


        // ====================================================================
        // Sequence - equal CCC stability
        //
        // U+0300 and U+0301 both have CCC 230.
        //
        //      A U+0300 U+0301 U+0323
        //
        // becomes:
        //
        //      A U+0323 U+0300 U+0301
        //
        // The two CCC=230 characters retain their original relative order.
        // ====================================================================

        {
            const uint32_t source[] = {
                0x0041,
                0x0300,
                0x0301,
                0x0323
            };

            uint32_t output[16]{};
            size_t outputCount = 0;

            constexpr size_t sourceCount =
                sizeof(source) / sizeof(source[0]);


            if (!normalizer.nfd(
                source,
                sourceCount,
                output,
                sizeof(output) / sizeof(output[0]),
                outputCount))
            {
                return fail("stable NFD ordering failed");
            }

            if (!unicodeNormalizerEquals(
                output,
                outputCount,
                {
                    0x0041,
                    0x0323,
                    0x0300,
                    0x0301
                }))
            {
                return fail("stable NFD ordering incorrect");
            }
        }


        // ====================================================================
        // nfdLength
        //
        //      U+00C5 -> 2
        //      U+00E9 -> 2
        //      U+0041 -> 1
        //
        // total = 5
        // ====================================================================

        {
            const uint32_t source[] = {
                0x00C5,
                0x00E9,
                0x0041
            };

            constexpr size_t sourceCount =
                sizeof(source) / sizeof(source[0]);

            size_t length = 0;


            if (!normalizer.nfdLength(
                source,
                sourceCount,
                length))
            {
                return fail("nfdLength failed");
            }

            if (length != 5)
                return fail("nfdLength returned incorrect result");
        }


        // ====================================================================
        // Worst-case capacity calculation
        // ====================================================================

        {
            size_t capacity = 0;

            if (!UnicodeNormalizer::nfdMaximumCapacity(
                10,
                capacity))
            {
                return fail("nfdMaximumCapacity failed");
            }

            if (capacity !=
                10 * kUnicodeCanonicalDecompositionMaxLength)
            {
                return fail("nfdMaximumCapacity returned incorrect value");
            }
        }


        // ====================================================================
        // Insufficient output capacity
        //
        //      U+00C0 U+0323
        //
        // requires three output code points.
        //
        // Failure must occur before anything is written.
        // ====================================================================

        {
            const uint32_t source[] = {
                0x00C0,
                0x0323
            };

            uint32_t output[] = {
                0x1111,
                0x2222
            };

            size_t outputCount = 1234;

            constexpr size_t sourceCount =
                sizeof(source) / sizeof(source[0]);


            if (normalizer.nfd(
                source,
                sourceCount,
                output,
                2,
                outputCount))
            {
                return fail("insufficient output buffer unexpectedly succeeded");
            }

            if (output[0] != 0x1111 ||
                output[1] != 0x2222)
            {
                return fail("capacity failure modified output buffer");
            }

            if (outputCount != 1234)
                return fail("capacity failure modified output count");
        }


        // ====================================================================
        // Invalid input code point
        // ====================================================================

        {
            const uint32_t source[] = {
                0x0041,
                kUnicodeLimit
            };

            uint32_t output[] = {
                0x1111,
                0x2222,
                0x3333,
                0x4444
            };

            size_t outputCount = 1234;

            constexpr size_t sourceCount =
                sizeof(source) / sizeof(source[0]);


            if (normalizer.nfd(
                source,
                sourceCount,
                output,
                sizeof(output) / sizeof(output[0]),
                outputCount))
            {
                return fail("invalid input unexpectedly succeeded");
            }

            if (output[0] != 0x1111 ||
                output[1] != 0x2222 ||
                output[2] != 0x3333 ||
                output[3] != 0x4444)
            {
                return fail("invalid input modified output buffer");
            }

            if (outputCount != 1234)
                return fail("invalid input modified output count");
        }


        // ====================================================================
        // Empty sequence
        // ====================================================================

        {
            size_t outputCount = 999;

            if (!normalizer.nfd(
                nullptr,
                0,
                nullptr,
                0,
                outputCount))
            {
                return fail("empty NFD sequence failed");
            }

            if (outputCount != 0)
                return fail("empty NFD sequence returned non-zero count");
        }


        // ====================================================================
        // NFD idempotence
        //
        // Normalize once, then normalize the result again.
        //
        //      NFD(NFD(x)) == NFD(x)
        // ====================================================================

        {
            const uint32_t source[] = {
                0x00C0,
                0x0315,
                0x0323,
                0x00E9
            };

            constexpr size_t sourceCount =
                sizeof(source) / sizeof(source[0]);

            uint32_t first[16]{};
            uint32_t second[64]{};

            size_t firstCount = 0;
            size_t secondCount = 0;


            if (!normalizer.nfd(
                source,
                sourceCount,
                first,
                sizeof(first) / sizeof(first[0]),
                firstCount))
            {
                return fail("NFD idempotence first pass failed");
            }


            if (!normalizer.nfd(
                first,
                firstCount,
                second,
                sizeof(second) / sizeof(second[0]),
                secondCount))
            {
                return fail("NFD idempotence second pass failed");
            }


            if (firstCount != secondCount)
                return fail("NFD idempotence changed sequence length");


            for (size_t i = 0; i < firstCount; ++i) {
                if (first[i] != second[i])
                    return fail("NFD is not idempotent");
            }
        }


        // ====================================================================
        // PASS
        // ====================================================================

        std::printf(
            "Unicode normalizer NFD: PASS\n"
            "  Single code point:          PASS\n"
            "  Recursive decomposition:    PASS\n"
            "  Maximum expansion:          %u\n"
            "  Compatibility exclusion:    PASS\n"
            "  Hangul decomposition:       PASS\n"
            "  Sequence normalization:     PASS\n"
            "  Cross-source ordering:      PASS\n"
            "  Leading combining marks:    PASS\n"
            "  Stable equal-CCC ordering:  PASS\n"
            "  Exact length calculation:   PASS\n"
            "  Capacity calculation:       PASS\n"
            "  Capacity failure safety:    PASS\n"
            "  Invalid input safety:       PASS\n"
            "  NFD idempotence:            PASS\n",
            kUnicodeCanonicalDecompositionMaxLength);


        return true;
    }

} // namespace waavs