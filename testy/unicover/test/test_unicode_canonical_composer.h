#pragma once
// test_unicode_canonical_composer.cpp

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

#include "test_core.h"

#include "unicode_canonical_composer.h"
#include "unicode_database.h"


namespace waavs
{
    static bool testUnicodeCanonicalComposer(const char* databaseFilename)
    {
        if (!databaseFilename)
            return false;


        // ====================================================================
        // Load the actual generated .ucdb image
        // ====================================================================

        std::ifstream input(databaseFilename, std::ios::binary | std::ios::ate);

        if (!input)
        {
            std::printf(
                "Unicode canonical composer: FAIL\n"
                "  Unable to open database: %s\n",
                databaseFilename);

            return false;
        }


        const std::streampos end = input.tellg();

        if (end <= 0)
        {
            std::printf(
                "Unicode canonical composer: FAIL\n"
                "  Database is empty or unreadable\n");

            return false;
        }


        std::vector<uint8_t> bytes(static_cast<size_t>(end));

        input.seekg(0, std::ios::beg);

        input.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));

        if (!input)
        {
            std::printf(
                "Unicode canonical composer: FAIL\n"
                "  Unable to read database\n");

            return false;
        }


        // ====================================================================
        // Attach the real runtime database
        // ====================================================================

        UnicodeDatabase database;

        if (!database.reset(ByteSpan(bytes.data(), bytes.size())))
        {
            std::printf(
                "Unicode canonical composer: FAIL\n"
                "  UnicodeDatabase::reset() failed\n");

            return false;
        }


        if (!database.hasComposition())
        {
            std::printf(
                "Unicode canonical composer: FAIL\n"
                "  Database has no canonical composition section\n");

            return false;
        }


        if (database.compositionRecordCount() != 961)
        {
            std::printf(
                "Unicode canonical composer: FAIL\n"
                "  Expected explicit records: 961\n"
                "  Actual explicit records:   %u\n",
                database.compositionRecordCount());

            return false;
        }


        // ====================================================================
        // Construct the runtime composer
        // ====================================================================

        UnicodeCanonicalComposer composer(database.composition());

        if (!composer.valid())
        {
            std::printf(
                "Unicode canonical composer: FAIL\n"
                "  Composer is invalid\n");

            return false;
        }


        // ====================================================================
        // Explicit composition table
        //
        // Every persistent composition pair must round-trip through the
        // canonical composer and produce its persistent composite.
        // ====================================================================

        uint32_t explicitCompositions = 0;

        for (uint32_t i = 0; i < database.compositionRecordCount(); ++i)
        {
            const UnicodeCompositionRecord* record =
                database.compositionRecord(i);

            if (!record)
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  Unable to access explicit record %u\n",
                    i);

                return false;
            }


            uint32_t result = kUnicodeCompositionNone;

            if (!composer.compose(record->first, record->second, result))
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  Explicit composition failed at record %u\n"
                    "  First:     U+%04X\n"
                    "  Second:    U+%04X\n"
                    "  Expected:  U+%04X\n",
                    i,
                    record->first,
                    record->second,
                    record->composite);

                return false;
            }


            if (result != record->composite)
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  Explicit composition mismatch at record %u\n"
                    "  First:     U+%04X\n"
                    "  Second:    U+%04X\n"
                    "  Expected:  U+%04X\n"
                    "  Actual:    U+%04X\n",
                    i,
                    record->first,
                    record->second,
                    record->composite,
                    result);

                return false;
            }


            if (composer.composite(record->first, record->second) !=
                record->composite)
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  composite() mismatch at explicit record %u\n",
                    i);

                return false;
            }


            if (!composer.canCompose(record->first, record->second))
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  canCompose() failed at explicit record %u\n",
                    i);

                return false;
            }


            ++explicitCompositions;
        }


        // ====================================================================
        // Known explicit mappings
        // ====================================================================

        struct KnownMapping
        {
            uint32_t first;
            uint32_t second;
            uint32_t composite;
        };


        static constexpr KnownMapping knownMappings[] =
        {
            { 0x0041u, 0x0300u, 0x00C0u },
            { 0x0041u, 0x030Au, 0x00C5u },
            { 0x00C5u, 0x0301u, 0x01FAu }
        };


        for (const KnownMapping& mapping : knownMappings)
        {
            if (composer.composite(mapping.first, mapping.second) !=
                mapping.composite)
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  Known explicit mapping failed\n"
                    "  First:     U+%04X\n"
                    "  Second:    U+%04X\n"
                    "  Expected:  U+%04X\n",
                    mapping.first,
                    mapping.second,
                    mapping.composite);

                return false;
            }
        }


        // ====================================================================
        // Hangul constants
        // ====================================================================

        static constexpr uint32_t kSBase = 0xAC00u;
        static constexpr uint32_t kLBase = 0x1100u;
        static constexpr uint32_t kVBase = 0x1161u;
        static constexpr uint32_t kTBase = 0x11A7u;

        static constexpr uint32_t kLCount = 19u;
        static constexpr uint32_t kVCount = 21u;
        static constexpr uint32_t kTCount = 28u;

        static constexpr uint32_t kNCount = kVCount * kTCount;
        static constexpr uint32_t kSCount = kLCount * kNCount;


        // ====================================================================
        // Exhaustive Hangul L + V -> LV
        //
        //      19 * 21 = 399
        //
        // Every LV Hangul syllable is tested.
        // ====================================================================

        uint32_t hangulLVCompositions = 0;


        for (uint32_t lIndex = 0; lIndex < kLCount; ++lIndex)
        {
            const uint32_t l = kLBase + lIndex;

            for (uint32_t vIndex = 0; vIndex < kVCount; ++vIndex)
            {
                const uint32_t v = kVBase + vIndex;

                const uint32_t expected =
                    kSBase +
                    (lIndex * kVCount + vIndex) *
                    kTCount;


                uint32_t result = kUnicodeCompositionNone;

                if (!composer.compose(l, v, result))
                {
                    std::printf(
                        "Unicode canonical composer: FAIL\n"
                        "  Hangul L+V composition failed\n"
                        "  L:         U+%04X\n"
                        "  V:         U+%04X\n"
                        "  Expected:  U+%04X\n",
                        l,
                        v,
                        expected);

                    return false;
                }


                if (result != expected)
                {
                    std::printf(
                        "Unicode canonical composer: FAIL\n"
                        "  Hangul L+V composition mismatch\n"
                        "  L:         U+%04X\n"
                        "  V:         U+%04X\n"
                        "  Expected:  U+%04X\n"
                        "  Actual:    U+%04X\n",
                        l,
                        v,
                        expected,
                        result);

                    return false;
                }


                if (composer.composite(l, v) != expected ||
                    !composer.canCompose(l, v))
                {
                    std::printf(
                        "Unicode canonical composer: FAIL\n"
                        "  Hangul L+V convenience API failure\n"
                        "  L: U+%04X\n"
                        "  V: U+%04X\n",
                        l,
                        v);

                    return false;
                }


                ++hangulLVCompositions;
            }
        }


        if (hangulLVCompositions != kLCount * kVCount)
        {
            std::printf(
                "Unicode canonical composer: FAIL\n"
                "  Unexpected Hangul L+V count: %u\n",
                hangulLVCompositions);

            return false;
        }


        // ====================================================================
        // Exhaustive Hangul LV + T -> LVT
        //
        // There are:
        //
        //      399 LV syllables
        //
        // and:
        //
        //      27 actual trailing consonants
        //
        // giving:
        //
        //      399 * 27 = 10773
        //
        // Every LVT Hangul syllable is tested.
        // ====================================================================

        uint32_t hangulLVTCompositions = 0;


        for (uint32_t lIndex = 0; lIndex < kLCount; ++lIndex)
        {
            for (uint32_t vIndex = 0; vIndex < kVCount; ++vIndex)
            {
                const uint32_t lv =
                    kSBase +
                    (lIndex * kVCount + vIndex) *
                    kTCount;


                for (uint32_t tIndex = 1; tIndex < kTCount; ++tIndex)
                {
                    const uint32_t t =
                        kTBase + tIndex;

                    const uint32_t expected =
                        lv + tIndex;


                    uint32_t result = kUnicodeCompositionNone;

                    if (!composer.compose(lv, t, result))
                    {
                        std::printf(
                            "Unicode canonical composer: FAIL\n"
                            "  Hangul LV+T composition failed\n"
                            "  LV:        U+%04X\n"
                            "  T:         U+%04X\n"
                            "  Expected:  U+%04X\n",
                            lv,
                            t,
                            expected);

                        return false;
                    }


                    if (result != expected)
                    {
                        std::printf(
                            "Unicode canonical composer: FAIL\n"
                            "  Hangul LV+T composition mismatch\n"
                            "  LV:        U+%04X\n"
                            "  T:         U+%04X\n"
                            "  Expected:  U+%04X\n"
                            "  Actual:    U+%04X\n",
                            lv,
                            t,
                            expected,
                            result);

                        return false;
                    }


                    if (composer.composite(lv, t) != expected ||
                        !composer.canCompose(lv, t))
                    {
                        std::printf(
                            "Unicode canonical composer: FAIL\n"
                            "  Hangul LV+T convenience API failure\n"
                            "  LV: U+%04X\n"
                            "  T:  U+%04X\n",
                            lv,
                            t);

                        return false;
                    }


                    ++hangulLVTCompositions;
                }
            }
        }


        const uint32_t expectedLVTCount =
            kLCount * kVCount * (kTCount - 1u);


        if (hangulLVTCompositions != expectedLVTCount)
        {
            std::printf(
                "Unicode canonical composer: FAIL\n"
                "  Unexpected Hangul LV+T count: %u\n",
                hangulLVTCompositions);

            return false;
        }


        // ====================================================================
        // Complete Hangul accounting
        //
        //      399 LV
        //    10773 LVT
        //    -----
        //    11172 total Hangul syllables
        // ====================================================================

        const uint32_t totalHangulCompositions =
            hangulLVCompositions +
            hangulLVTCompositions;


        if (totalHangulCompositions != kSCount)
        {
            std::printf(
                "Unicode canonical composer: FAIL\n"
                "  Expected Hangul compositions: %u\n"
                "  Actual Hangul compositions:   %u\n",
                kSCount,
                totalHangulCompositions);

            return false;
        }


        // ====================================================================
        // Hangul rejection cases
        // ====================================================================

        uint32_t failureChecks = 0;


        // TBase represents no trailing consonant and must not compose.

        {
            uint32_t result = 0x12345678u;

            if (composer.compose(kSBase, kTBase, result) ||
                result != 0x12345678u)
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  Hangul TBase was incorrectly accepted\n");

                return false;
            }

            ++failureChecks;
        }


        // One code point beyond the valid T jamo range must not compose.

        {
            uint32_t result = 0x12345678u;

            if (composer.compose(
                kSBase,
                kTBase + kTCount,
                result) ||
                result != 0x12345678u)
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  Out-of-range Hangul T was incorrectly accepted\n");

                return false;
            }

            ++failureChecks;
        }


        // An LVT syllable already has a trailing consonant and cannot accept
        // another T.

        {
            const uint32_t lvt =
                kSBase + 1u;

            const uint32_t t =
                kTBase + 1u;

            uint32_t result = 0x12345678u;

            if (composer.compose(lvt, t, result) ||
                result != 0x12345678u)
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  Completed Hangul LVT accepted another T\n");

                return false;
            }

            ++failureChecks;
        }


        // ====================================================================
        // Ordinary absent pair
        //
        // Also verifies that compose() leaves the output unchanged on failure.
        // ====================================================================

        {
            uint32_t result = 0x12345678u;

            if (composer.compose(0x0041u, 0x0042u, result) ||
                result != 0x12345678u)
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  Non-composable ordinary pair was accepted\n");

                return false;
            }


            if (composer.composite(0x0041u, 0x0042u) !=
                kUnicodeCompositionNone)
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  composite() accepted non-composable pair\n");

                return false;
            }


            if (composer.canCompose(0x0041u, 0x0042u))
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  canCompose() accepted non-composable pair\n");

                return false;
            }

            ++failureChecks;
        }


        // ====================================================================
        // Invalid Unicode inputs
        // ====================================================================

        {
            uint32_t result = 0x12345678u;

            if (composer.compose(kUnicodeLimit, 0x0300u, result) ||
                result != 0x12345678u)
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  Invalid first code point was accepted\n");

                return false;
            }

            ++failureChecks;
        }


        {
            uint32_t result = 0x12345678u;

            if (composer.compose(0x0041u, kUnicodeLimit, result) ||
                result != 0x12345678u)
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  Invalid second code point was accepted\n");

                return false;
            }

            ++failureChecks;
        }


        // ====================================================================
        // Invalid composer state
        // ====================================================================

        {
            UnicodeCanonicalComposer emptyComposer;

            uint32_t result = 0x12345678u;

            if (emptyComposer.valid() ||
                emptyComposer.compose(0x0041u, 0x0300u, result) ||
                result != 0x12345678u)
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  Invalid composer state was not handled correctly\n");

                return false;
            }

            ++failureChecks;
        }


        // ====================================================================
        // reset()
        // ====================================================================

        {
            UnicodeCanonicalComposer resetComposer;

            resetComposer.reset(database.composition());

            if (!resetComposer.valid())
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  reset(composition) did not attach composer\n");

                return false;
            }


            resetComposer.reset();

            if (resetComposer.valid())
            {
                std::printf(
                    "Unicode canonical composer: FAIL\n"
                    "  reset() did not clear composer\n");

                return false;
            }

            ++failureChecks;
        }


        // ====================================================================
        // Success
        // ====================================================================

        const uint32_t totalSuccessfulCompositions =
            explicitCompositions +
            totalHangulCompositions;


        std::printf(
            "Unicode canonical composer: PASS\n"
            "  Database bytes:              %zu\n"
            "  Explicit compositions:       %u\n"
            "  Hangul L+V compositions:      %u\n"
            "  Hangul LV+T compositions:     %u\n"
            "  Total Hangul compositions:    %u\n"
            "  Total successful pairs:       %u\n"
            "  Known explicit mappings:      %zu\n"
            "  Failure-path checks:          %u\n",
            bytes.size(),
            explicitCompositions,
            hangulLVCompositions,
            hangulLVTCompositions,
            totalHangulCompositions,
            totalSuccessfulCompositions,
            sizeof(knownMappings) / sizeof(knownMappings[0]),
            failureChecks);


        return true;
    }

} // namespace waavs