// test_unicode_canonical_decomposer.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <vector>

#include "unicode_canonical_decomposer.h"
#include "unicode_database.h"


namespace waavs
{
    // ========================================================================
    // Test helpers
    // ========================================================================

    static bool unicodeCanonicalDecompositionEquals(
        const UnicodeCanonicalDecomposition& result,
        std::initializer_list<uint32_t> expected) noexcept
    {
        if (result.size() != expected.size())
            return false;

        size_t index = 0;

        for (uint32_t cp : expected) {
            if (result[index++] != cp)
                return false;
        }

        return true;
    }


    static bool unicodeCanonicalTestIsHangulSyllable(uint32_t cp) noexcept
    {
        static constexpr uint32_t kSBase = 0xAC00u;
        static constexpr uint32_t kSCount = 11172u;

        return cp >= kSBase && cp < kSBase + kSCount;
    }


    // ========================================================================
    // testUnicodeCanonicalDecomposer
    //
    // Load a generated .ucdb and test full recursive canonical decomposition.
    //
    // The exhaustive portion verifies:
    //
    //      - every Unicode code point decomposes successfully
    //      - every result contains 1..4 code points
    //      - code points without decomposition return themselves
    //      - explicit canonical mappings do not return themselves
    //      - every Hangul syllable is algorithmically decomposed
    //      - all emitted code points are terminal decomposition leaves
    //      - no emitted code point is itself a Hangul syllable
    //      - Unicode 17 maximum recursive decomposition length is four
    //
    // ========================================================================

    static bool testUnicodeCanonicalDecomposer(const char* databaseFilename)
    {
        auto fail = [](const char* message) -> bool {
            std::printf(
                "Unicode canonical decomposer: FAIL: %s\n",
                message);

            return false;
            };


        // ====================================================================
        // Load .ucdb
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
            static_cast<std::streamsize>(databaseBytes.size()));


        if (!input)
            return fail("unable to read Unicode database");


        // ====================================================================
        // Runtime database
        // ====================================================================

        UnicodeDatabase database;


        if (!database.reset(
            ByteSpan(
                databaseBytes.data(),
                databaseBytes.size())))
        {
            return fail("UnicodeDatabase rejected database");
        }


        if (!database.hasDecomposition())
            return fail("database contains no canonical decomposition data");


        UnicodeDecomposition direct =
            database.decomposition();


        if (!direct)
            return fail("unable to obtain direct decomposition view");


        UnicodeCanonicalDecomposer decomposer(
            direct);


        if (!decomposer)
            return fail("canonical decomposer is invalid");


        // ====================================================================
        // Result helper
        // ====================================================================

        UnicodeCanonicalDecomposition result;


        auto check =
            [&](uint32_t cp,
                std::initializer_list<uint32_t> expected,
                const char* description) -> bool
            {
                result.clear();


                if (!decomposer.decompose(cp, result))
                {
                    std::printf(
                        "Unicode canonical decomposer: FAIL: %s\n"
                        "  U+%04X failed to decompose\n",
                        description,
                        cp);

                    return false;
                }


                if (!unicodeCanonicalDecompositionEquals(
                    result,
                    expected))
                {
                    std::printf(
                        "Unicode canonical decomposer: FAIL: %s\n"
                        "  U+%04X\n"
                        "  Expected:",
                        description,
                        cp);


                    for (uint32_t value : expected)
                        std::printf(" U+%04X", value);


                    std::printf(
                        "\n"
                        "  Actual:  ");


                    for (uint32_t i = 0; i < result.count; ++i)
                        std::printf(" U+%04X", result[i]);


                    std::printf("\n");

                    return false;
                }


                return true;
            };


        // ====================================================================
        // Ordinary decomposition leaf
        //
        // U+0041 has no canonical decomposition.
        // ====================================================================

        if (!check(
            0x0041,
            { 0x0041 },
            "ordinary decomposition leaf"))
        {
            return false;
        }


        // ====================================================================
        // Singleton
        //
        // U+0340 -> U+0300
        // ====================================================================

        if (!check(
            0x0340,
            { 0x0300 },
            "singleton decomposition"))
        {
            return false;
        }


        // ====================================================================
        // Pair
        //
        // U+00E9 -> U+0065 U+0301
        // ====================================================================

        if (!check(
            0x00E9,
            { 0x0065, 0x0301 },
            "pair decomposition"))
        {
            return false;
        }


        // ====================================================================
        // Recursive pair
        //
        // U+01D5 -> U+00DC U+0304
        // U+00DC -> U+0055 U+0308
        //
        // Final:
        //
        //      U+0055 U+0308 U+0304
        // ====================================================================

        if (!check(
            0x01D5,
            { 0x0055, 0x0308, 0x0304 },
            "recursive decomposition"))
        {
            return false;
        }


        // ====================================================================
        // Maximum Unicode 17 recursive length
        //
        // U+1F82
        //
        //      -> U+1F02 U+0345
        //      -> U+1F00 U+0300 U+0345
        //      -> U+03B1 U+0313 U+0300 U+0345
        //
        // Final length = 4.
        // ====================================================================

        if (!check(
            0x1F82,
            { 0x03B1, 0x0313, 0x0300, 0x0345 },
            "four-code-point recursive decomposition"))
        {
            return false;
        }


        // ====================================================================
        // Compatibility decomposition must remain absent.
        //
        // U+00A0 has compatibility decomposition <noBreak> U+0020, but no
        // canonical decomposition.
        // ====================================================================

        if (!check(
            0x00A0,
            { 0x00A0 },
            "compatibility mapping exclusion"))
        {
            return false;
        }


        // ====================================================================
        // Hangul LV
        //
        // U+AC00 HANGUL SYLLABLE GA
        //
        //      -> U+1100 U+1161
        // ====================================================================

        if (!check(
            0xAC00,
            { 0x1100, 0x1161 },
            "Hangul LV decomposition"))
        {
            return false;
        }


        // ====================================================================
        // Hangul LVT
        //
        // U+AC01 HANGUL SYLLABLE GAG
        //
        //      -> U+1100 U+1161 U+11A8
        // ====================================================================

        if (!check(
            0xAC01,
            { 0x1100, 0x1161, 0x11A8 },
            "Hangul LVT decomposition"))
        {
            return false;
        }


        // ====================================================================
        // Final Hangul syllable
        //
        // U+D7A3
        //
        //      -> U+1112 U+1175 U+11C2
        //
        // This also checks the upper boundary of the algorithmic range.
        // ====================================================================

        if (!check(
            0xD7A3,
            { 0x1112, 0x1175, 0x11C2 },
            "final Hangul syllable"))
        {
            return false;
        }


        // ====================================================================
        // Immediately outside the Hangul syllable range.
        // ====================================================================

        if (!check(
            0xD7A4,
            { 0xD7A4 },
            "code point after Hangul syllable range"))
        {
            return false;
        }


        // ====================================================================
        // Transactional failure
        //
        // An invalid input must not modify the caller's result.
        // ====================================================================

        UnicodeCanonicalDecomposition unchanged;

        unchanged.codePoints[0] = 0x1234;
        unchanged.codePoints[1] = 0x5678;
        unchanged.count = 2;


        if (decomposer.decompose(
            kUnicodeLimit,
            unchanged))
        {
            return fail("out-of-range code point unexpectedly succeeded");
        }


        if (unchanged.count != 2 ||
            unchanged.codePoints[0] != 0x1234 ||
            unchanged.codePoints[1] != 0x5678)
        {
            return fail("failed decomposition modified output result");
        }


        // ====================================================================
        // Invalid decomposer
        // ====================================================================

        {
            UnicodeCanonicalDecomposer invalid;

            UnicodeCanonicalDecomposition output;

            output.codePoints[0] = 0x4321;
            output.count = 1;


            if (invalid.decompose(0x0041, output))
                return fail("invalid decomposer unexpectedly succeeded");


            if (output.count != 1 ||
                output.codePoints[0] != 0x4321)
            {
                return fail("invalid decomposer modified output result");
            }
        }


        // ====================================================================
        // Exhaustive verification
        // ====================================================================

        size_t explicitMappings = 0;
        size_t hangulMappings = 0;
        size_t decomposableSources = 0;
        size_t unchangedSources = 0;
        size_t emittedCodePoints = 0;

        uint32_t maximumLength = 0;
        uint32_t maximumLengthCodePoint = 0;


        for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
        {
            UnicodeCanonicalDecomposition decomposition;


            if (!decomposer.decompose(
                cp,
                decomposition))
            {
                std::printf(
                    "Unicode canonical decomposer: FAIL: "
                    "decomposition failed at U+%04X\n",
                    cp);

                return false;
            }


            // ---------------------------------------------------------------
            // Every valid scalar position produces at least itself.
            // ---------------------------------------------------------------

            if (decomposition.count == 0)
            {
                std::printf(
                    "Unicode canonical decomposer: FAIL: "
                    "empty result at U+%04X\n",
                    cp);

                return false;
            }


            // ---------------------------------------------------------------
            // Fixed-capacity representation invariant.
            // ---------------------------------------------------------------

            if (decomposition.count >
                kUnicodeCanonicalDecompositionMaxLength)
            {
                std::printf(
                    "Unicode canonical decomposer: FAIL: "
                    "result too long at U+%04X: %u\n",
                    cp,
                    decomposition.count);

                return false;
            }


            // ---------------------------------------------------------------
            // Track maximum observed result length.
            // ---------------------------------------------------------------

            if (decomposition.count > maximumLength)
            {
                maximumLength =
                    decomposition.count;

                maximumLengthCodePoint =
                    cp;
            }


            const bool hasDirectMapping =
                direct.hasMapping(cp);


            const bool isHangul =
                unicodeCanonicalTestIsHangulSyllable(cp);


            if (hasDirectMapping)
                ++explicitMappings;


            if (isHangul)
                ++hangulMappings;


            // ---------------------------------------------------------------
            // A source with neither explicit nor algorithmic decomposition
            // must return exactly itself.
            // ---------------------------------------------------------------

            if (!hasDirectMapping &&
                !isHangul)
            {
                if (decomposition.count != 1 ||
                    decomposition[0] != cp)
                {
                    std::printf(
                        "Unicode canonical decomposer: FAIL: "
                        "unchanged code point U+%04X produced:",
                        cp);


                    for (uint32_t i = 0;
                        i < decomposition.count;
                        ++i)
                    {
                        std::printf(
                            " U+%04X",
                            decomposition[i]);
                    }


                    std::printf("\n");

                    return false;
                }


                ++unchangedSources;
            }
            else
            {
                // -----------------------------------------------------------
                // A canonical or Hangul decomposition must actually change
                // the source representation.
                // -----------------------------------------------------------

                if (decomposition.count == 1 &&
                    decomposition[0] == cp)
                {
                    std::printf(
                        "Unicode canonical decomposer: FAIL: "
                        "decomposable U+%04X returned itself\n",
                        cp);

                    return false;
                }


                ++decomposableSources;
            }


            // ---------------------------------------------------------------
            // Every emitted code point must be a terminal decomposition leaf.
            //
            // If an output still has an explicit mapping, recursion stopped
            // too early.
            //
            // Likewise a Hangul syllable must never survive in the final
            // decomposition.
            // ---------------------------------------------------------------

            for (uint32_t i = 0;
                i < decomposition.count;
                ++i)
            {
                const uint32_t value =
                    decomposition[i];


                if (value >= kUnicodeLimit)
                {
                    std::printf(
                        "Unicode canonical decomposer: FAIL: "
                        "out-of-range output U+%04X from U+%04X\n",
                        value,
                        cp);

                    return false;
                }


                if (direct.hasMapping(value))
                {
                    std::printf(
                        "Unicode canonical decomposer: FAIL: "
                        "non-terminal output U+%04X from U+%04X\n",
                        value,
                        cp);

                    return false;
                }


                if (unicodeCanonicalTestIsHangulSyllable(value))
                {
                    std::printf(
                        "Unicode canonical decomposer: FAIL: "
                        "Hangul syllable U+%04X survived decomposition of U+%04X\n",
                        value,
                        cp);

                    return false;
                }


                ++emittedCodePoints;
            }
        }


        // ====================================================================
        // Unicode 17 invariants
        // ====================================================================

        if (explicitMappings != 2081)
            return fail("explicit canonical mapping count is not 2081");


        if (hangulMappings != 11172)
            return fail("Hangul syllable count is not 11172");


        if (decomposableSources !=
            explicitMappings + hangulMappings)
        {
            return fail("decomposable source count is inconsistent");
        }


        if (decomposableSources != 13253)
            return fail("total decomposable source count is not 13253");


        if (unchangedSources + decomposableSources !=
            static_cast<size_t>(kUnicodeLimit))
        {
            return fail("source accounting does not cover Unicode space");
        }


        if (maximumLength !=
            kUnicodeCanonicalDecompositionMaxLength)
        {
            return fail("maximum recursive decomposition length is not four");
        }


        // ====================================================================
        // PASS
        // ====================================================================

        std::printf(
            "Unicode canonical decomposer: PASS\n"
            "  Unicode code points:        %u\n"
            "  Explicit mappings:          %zu\n"
            "  Hangul mappings:            %zu\n"
            "  Total decomposable:         %zu\n"
            "  Unchanged code points:      %zu\n"
            "  Maximum result length:      %u\n"
            "  First maximum at:           U+%04X\n"
            "  Total emitted code points:  %zu\n",
            kUnicodeLimit,
            explicitMappings,
            hangulMappings,
            decomposableSources,
            unchangedSources,
            maximumLength,
            maximumLengthCodePoint,
            emittedCodePoints);


        return true;
    }

} // namespace waavs
