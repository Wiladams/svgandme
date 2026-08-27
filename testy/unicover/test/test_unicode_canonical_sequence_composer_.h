// test_unicode_canonical_sequence_composer.cpp

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

#include "test_core.h"

#include "unicode_canonical_sequence_composer.h"
#include "unicode_database.h"


namespace waavs
{
    static bool testUnicodeCanonicalSequenceComposer(const char* databaseFilename)
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
                "Unicode canonical sequence composer: FAIL\n"
                "  Unable to open database: %s\n",
                databaseFilename);

            return false;
        }


        const std::streampos end = input.tellg();

        if (end <= 0)
        {
            std::printf(
                "Unicode canonical sequence composer: FAIL\n"
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
                "Unicode canonical sequence composer: FAIL\n"
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
                "Unicode canonical sequence composer: FAIL\n"
                "  UnicodeDatabase::reset() failed\n");

            return false;
        }


        if (!database.hasComposition())
        {
            std::printf(
                "Unicode canonical sequence composer: FAIL\n"
                "  Database has no canonical composition section\n");

            return false;
        }


        if (database.compositionRecordCount() != 961)
        {
            std::printf(
                "Unicode canonical sequence composer: FAIL\n"
                "  Expected explicit records: 961\n"
                "  Actual explicit records:   %u\n",
                database.compositionRecordCount());

            return false;
        }


        // ====================================================================
        // Construct the runtime sequence composer
        // ====================================================================

        UnicodeCanonicalSequenceComposer composer(database);

        if (!composer.valid())
        {
            std::printf(
                "Unicode canonical sequence composer: FAIL\n"
                "  Composer is invalid\n");

            return false;
        }


        const UnicodeCanonicalComposer& pairComposer =
            composer.composer();


        // ====================================================================
        // Basic state and empty input
        // ====================================================================

        uint32_t basicChecks = 0;


        {
            size_t count = 0;

            if (!composer.compose(nullptr, count) || count != 0)
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Empty sequence failed\n");

                return false;
            }

            ++basicChecks;
        }


        {
            size_t count = 1;

            if (composer.compose(nullptr, count) || count != 1)
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Null non-empty sequence was accepted\n");

                return false;
            }

            ++basicChecks;
        }


        {
            uint32_t sequence[] = { 0x0041u };
            size_t count = 1;

            if (!composer.compose(sequence, count) ||
                count != 1 ||
                sequence[0] != 0x0041u)
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Single-code-point sequence changed\n");

                return false;
            }

            ++basicChecks;
        }


        // ====================================================================
        // Exhaustive explicit composition
        //
        // Every persistent pair is passed through the sequence composer:
        //
        //      first second
        //
        // must become:
        //
        //      composite
        //
        // This exercises all 961 explicit canonical composition records through
        // the actual database, pair composer, CCC lookup, blocking machinery,
        // and in-place sequence compaction.
        // ====================================================================

        uint32_t explicitSequences = 0;
        uint32_t explicitSecondCCCZero = 0;
        uint32_t explicitSecondCCCNonZero = 0;


        for (uint32_t i = 0; i < database.compositionRecordCount(); ++i)
        {
            const UnicodeCompositionRecord* record =
                database.compositionRecord(i);

            if (!record)
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Unable to access explicit record %u\n",
                    i);

                return false;
            }


            // Full composition records must have a starter as their first
            // decomposition component.

            if (database.combiningClass(record->first) !=
                kUnicodeCombiningClassNotReordered)
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Explicit record does not begin with a starter\n"
                    "  Record:    %u\n"
                    "  First:     U+%04X\n"
                    "  CCC:       %u\n",
                    i,
                    record->first,
                    database.combiningClass(record->first));

                return false;
            }


            const UnicodeCombiningClass secondCCC =
                database.combiningClass(record->second);

            if (secondCCC == kUnicodeCombiningClassNotReordered)
                ++explicitSecondCCCZero;
            else
                ++explicitSecondCCCNonZero;


            uint32_t sequence[] =
            {
                record->first,
                record->second
            };

            size_t count = 2;


            if (!composer.ordering().isOrdered(sequence, count))
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Explicit pair is not canonically ordered\n"
                    "  Record:    %u\n"
                    "  First:     U+%04X\n"
                    "  Second:    U+%04X\n",
                    i,
                    record->first,
                    record->second);

                return false;
            }


            if (!composer.compose(sequence, count))
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Explicit sequence composition failed\n"
                    "  Record:    %u\n"
                    "  First:     U+%04X\n"
                    "  Second:    U+%04X\n"
                    "  Expected:  U+%04X\n",
                    i,
                    record->first,
                    record->second,
                    record->composite);

                return false;
            }


            if (count != 1 ||
                sequence[0] != record->composite)
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Explicit sequence composition mismatch\n"
                    "  Record:    %u\n"
                    "  First:     U+%04X\n"
                    "  Second:    U+%04X\n"
                    "  Expected:  U+%04X\n"
                    "  Count:     %zu\n"
                    "  Actual:    U+%04X\n",
                    i,
                    record->first,
                    record->second,
                    record->composite,
                    count,
                    sequence[0]);

                return false;
            }


            ++explicitSequences;
        }


        // ====================================================================
        // Exhaustive two-stage explicit composition chains
        //
        // Find every pair of persistent mappings:
        //
        //      A + B -> X
        //      X + C -> Y
        //
        // for which:
        //
        //      A B C
        //
        // is already canonically ordered.
        //
        // The sequence composer must produce:
        //
        //      Y
        //
        // This is particularly important because successful composition of B
        // must NOT update lastCCC. Otherwise equal-CCC chains will incorrectly
        // block the second composition.
        // ====================================================================

        uint32_t explicitChains = 0;


        for (uint32_t i = 0; i < database.compositionRecordCount(); ++i)
        {
            const UnicodeCompositionRecord* firstRecord =
                database.compositionRecord(i);

            if (!firstRecord)
                return false;


            for (uint32_t j = 0; j < database.compositionRecordCount(); ++j)
            {
                const UnicodeCompositionRecord* secondRecord =
                    database.compositionRecord(j);

                if (!secondRecord)
                    return false;


                if (secondRecord->first != firstRecord->composite)
                    continue;


                uint32_t sequence[] =
                {
                    firstRecord->first,
                    firstRecord->second,
                    secondRecord->second
                };


                if (!composer.ordering().isOrdered(sequence, 3))
                    continue;


                size_t count = 3;


                if (!composer.compose(sequence, count))
                {
                    std::printf(
                        "Unicode canonical sequence composer: FAIL\n"
                        "  Explicit composition chain failed\n"
                        "  A:          U+%04X\n"
                        "  B:          U+%04X\n"
                        "  X:          U+%04X\n"
                        "  C:          U+%04X\n"
                        "  Expected:   U+%04X\n",
                        firstRecord->first,
                        firstRecord->second,
                        firstRecord->composite,
                        secondRecord->second,
                        secondRecord->composite);

                    return false;
                }


                if (count != 1 ||
                    sequence[0] != secondRecord->composite)
                {
                    std::printf(
                        "Unicode canonical sequence composer: FAIL\n"
                        "  Explicit composition chain mismatch\n"
                        "  A:          U+%04X\n"
                        "  B:          U+%04X\n"
                        "  X:          U+%04X\n"
                        "  C:          U+%04X\n"
                        "  Expected:   U+%04X\n"
                        "  Count:      %zu\n"
                        "  Actual:     U+%04X\n",
                        firstRecord->first,
                        firstRecord->second,
                        firstRecord->composite,
                        secondRecord->second,
                        secondRecord->composite,
                        count,
                        sequence[0]);

                    return false;
                }


                ++explicitChains;
            }
        }


        if (explicitChains == 0)
        {
            std::printf(
                "Unicode canonical sequence composer: FAIL\n"
                "  No explicit composition chains were exercised\n");

            return false;
        }


        // ====================================================================
        // Build NFD-safe non-starter candidates by CCC
        //
        // These will be used to manufacture blocking and non-blocking
        // sequences directly from the live Unicode database.
        //
        // A candidate must:
        //
        //      - have non-zero CCC
        //      - have no explicit canonical decomposition
        //
        // This keeps the generated test sequences already decomposed.
        // ====================================================================

        std::array<std::vector<uint32_t>, 256> cccCandidates;

        const UnicodeDecomposition decomposition =
            database.decomposition();


        if (!decomposition.valid())
        {
            std::printf(
                "Unicode canonical sequence composer: FAIL\n"
                "  Canonical decomposition view is invalid\n");

            return false;
        }


        for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
        {
            const UnicodeCombiningClass ccc =
                database.combiningClass(cp);

            if (ccc == kUnicodeCombiningClassNotReordered)
                continue;


            if (decomposition.record(cp))
                continue;


            cccCandidates[ccc].push_back(cp);
        }


        // ====================================================================
        // Equal-CCC blocking
        //
        // For each explicit mapping having a non-zero second CCC:
        //
        //      starter + second -> composite
        //
        // find an NFD-safe character B having:
        //
        //      CCC(B) == CCC(second)
        //
        // where:
        //
        //      starter + B
        //
        // does NOT compose.
        //
        // Then:
        //
        //      starter B second
        //
        // must remain unchanged because B blocks second.
        // ====================================================================

        uint32_t blockingEligible = 0;
        uint32_t blockingCases = 0;
        uint32_t blockingCandidatesUnavailable = 0;


        for (uint32_t i = 0; i < database.compositionRecordCount(); ++i)
        {
            const UnicodeCompositionRecord* record =
                database.compositionRecord(i);

            if (!record)
                return false;


            const UnicodeCombiningClass secondCCC =
                database.combiningClass(record->second);

            if (secondCCC == kUnicodeCombiningClassNotReordered)
                continue;


            ++blockingEligible;


            uint32_t blocker = kUnicodeCompositionNone;


            for (uint32_t candidate : cccCandidates[secondCCC])
            {
                if (pairComposer.canCompose(record->first, candidate))
                    continue;

                blocker = candidate;
                break;
            }


            if (blocker == kUnicodeCompositionNone)
            {
                ++blockingCandidatesUnavailable;
                continue;
            }


            uint32_t sequence[] =
            {
                record->first,
                blocker,
                record->second
            };

            const uint32_t expected[] =
            {
                record->first,
                blocker,
                record->second
            };


            if (!composer.ordering().isOrdered(sequence, 3))
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Generated blocking sequence is not ordered\n"
                    "  Starter:   U+%04X\n"
                    "  Blocker:   U+%04X\n"
                    "  Second:    U+%04X\n"
                    "  CCC:       %u\n",
                    record->first,
                    blocker,
                    record->second,
                    secondCCC);

                return false;
            }


            size_t count = 3;


            if (!composer.compose(sequence, count))
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Equal-CCC blocking sequence failed\n");

                return false;
            }


            if (count != 3 ||
                sequence[0] != expected[0] ||
                sequence[1] != expected[1] ||
                sequence[2] != expected[2])
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Equal-CCC character failed to block composition\n"
                    "  Starter:   U+%04X\n"
                    "  Blocker:   U+%04X\n"
                    "  Second:    U+%04X\n"
                    "  CCC:       %u\n",
                    record->first,
                    blocker,
                    record->second,
                    secondCCC);

                return false;
            }


            ++blockingCases;
        }


        if (blockingCases == 0)
        {
            std::printf(
                "Unicode canonical sequence composer: FAIL\n"
                "  No equal-CCC blocking cases were exercised\n");

            return false;
        }


        // ====================================================================
        // Lower-CCC intervening characters do not block
        //
        // For every explicit composition where possible, find an NFD-safe
        // retained character B satisfying:
        //
        //      0 < CCC(B) < CCC(second)
        //
        // and:
        //
        //      starter + B
        //
        // does not compose.
        //
        // Then:
        //
        //      starter B second
        //
        // must become:
        //
        //      composite B
        //
        // Composition therefore occurs across an intervening retained mark.
        // ====================================================================

        uint32_t unblockedEligible = 0;
        uint32_t unblockedCases = 0;
        uint32_t unblockedCandidatesUnavailable = 0;


        for (uint32_t i = 0; i < database.compositionRecordCount(); ++i)
        {
            const UnicodeCompositionRecord* record =
                database.compositionRecord(i);

            if (!record)
                return false;


            const UnicodeCombiningClass secondCCC =
                database.combiningClass(record->second);

            if (secondCCC <= 1)
                continue;


            ++unblockedEligible;


            uint32_t intervening = kUnicodeCompositionNone;


            for (uint32_t ccc = 1; ccc < secondCCC; ++ccc)
            {
                for (uint32_t candidate : cccCandidates[ccc])
                {
                    if (pairComposer.canCompose(record->first, candidate))
                        continue;

                    intervening = candidate;
                    break;
                }


                if (intervening != kUnicodeCompositionNone)
                    break;
            }


            if (intervening == kUnicodeCompositionNone)
            {
                ++unblockedCandidatesUnavailable;
                continue;
            }


            uint32_t sequence[] =
            {
                record->first,
                intervening,
                record->second
            };


            if (!composer.ordering().isOrdered(sequence, 3))
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Generated unblocked sequence is not ordered\n"
                    "  Starter:       U+%04X\n"
                    "  Intervening:   U+%04X\n"
                    "  Second:        U+%04X\n",
                    record->first,
                    intervening,
                    record->second);

                return false;
            }


            size_t count = 3;


            if (!composer.compose(sequence, count))
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Lower-CCC intervening sequence failed\n");

                return false;
            }


            if (count != 2 ||
                sequence[0] != record->composite ||
                sequence[1] != intervening)
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Lower-CCC character incorrectly blocked composition\n"
                    "  Starter:       U+%04X\n"
                    "  Intervening:   U+%04X\n"
                    "  Second:        U+%04X\n"
                    "  Expected:      U+%04X U+%04X\n",
                    record->first,
                    intervening,
                    record->second,
                    record->composite,
                    intervening);

                return false;
            }


            ++unblockedCases;
        }


        if (unblockedCases == 0)
        {
            std::printf(
                "Unicode canonical sequence composer: FAIL\n"
                "  No lower-CCC unblocked cases were exercised\n");

            return false;
        }


        // ====================================================================
        // Leading non-starter
        //
        // A leading non-starter must remain untouched. The following starter
        // can still establish a new composition sequence:
        //
        //      U+0301 U+0041 U+0300
        //
        // becomes:
        //
        //      U+0301 U+00C0
        // ====================================================================

        uint32_t structuralChecks = 0;


        {
            uint32_t sequence[] =
            {
                0x0301u,
                0x0041u,
                0x0300u
            };

            size_t count = 3;


            if (!composer.ordering().isOrdered(sequence, count))
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Leading non-starter test is not ordered\n");

                return false;
            }


            if (!composer.compose(sequence, count) ||
                count != 2 ||
                sequence[0] != 0x0301u ||
                sequence[1] != 0x00C0u)
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Leading non-starter handling failed\n");

                return false;
            }

            ++structuralChecks;
        }


        // ====================================================================
        // Sequence containing only non-starters
        // ====================================================================

        {
            uint32_t sequence[] =
            {
                0x0323u,
                0x0301u
            };

            size_t count = 2;


            if (!composer.ordering().isOrdered(sequence, count))
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Non-starter-only test is not ordered\n");

                return false;
            }


            if (!composer.compose(sequence, count) ||
                count != 2 ||
                sequence[0] != 0x0323u ||
                sequence[1] != 0x0301u)
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Non-starter-only sequence changed\n");

                return false;
            }

            ++structuralChecks;
        }


        // ====================================================================
        // Starter boundary
        //
        // A later starter replaces the previous active starter. Therefore:
        //
        //      A B grave
        //
        // must not allow:
        //
        //      A + grave
        //
        // across B.
        // ====================================================================

        {
            if (pairComposer.canCompose(0x0042u, 0x0300u))
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Starter-boundary fixture unexpectedly composes\n");

                return false;
            }


            uint32_t sequence[] =
            {
                0x0041u,
                0x0042u,
                0x0300u
            };

            size_t count = 3;


            if (!composer.compose(sequence, count) ||
                count != 3 ||
                sequence[0] != 0x0041u ||
                sequence[1] != 0x0042u ||
                sequence[2] != 0x0300u)
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Starter boundary failed\n");

                return false;
            }

            ++structuralChecks;
        }


        // ====================================================================
        // Exhaustive Hangul composition from decomposed Jamo
        //
        // For every Hangul syllable:
        //
        //      LV:
        //
        //          L V -> LV
        //
        //      LVT:
        //
        //          L V T -> LVT
        //
        // This exercises all 11,172 Hangul syllables through the sequence
        // composer, including the repeated starter replacement required by
        // L + V + T.
        // ====================================================================

        static constexpr uint32_t kSBase = 0xAC00u;
        static constexpr uint32_t kLBase = 0x1100u;
        static constexpr uint32_t kVBase = 0x1161u;
        static constexpr uint32_t kTBase = 0x11A7u;

        static constexpr uint32_t kLCount = 19u;
        static constexpr uint32_t kVCount = 21u;
        static constexpr uint32_t kTCount = 28u;

        static constexpr uint32_t kNCount =
            kVCount * kTCount;

        static constexpr uint32_t kSCount =
            kLCount * kNCount;


        uint32_t hangulLVSequences = 0;
        uint32_t hangulLVTSequences = 0;


        for (uint32_t lIndex = 0; lIndex < kLCount; ++lIndex)
        {
            const uint32_t l =
                kLBase + lIndex;


            for (uint32_t vIndex = 0; vIndex < kVCount; ++vIndex)
            {
                const uint32_t v =
                    kVBase + vIndex;

                const uint32_t lv =
                    kSBase +
                    (lIndex * kVCount + vIndex) *
                    kTCount;


                // -----------------------------------------------------------
                // L + V -> LV
                // -----------------------------------------------------------

                {
                    uint32_t sequence[] =
                    {
                        l,
                        v
                    };

                    size_t count = 2;


                    if (!composer.compose(sequence, count) ||
                        count != 1 ||
                        sequence[0] != lv)
                    {
                        std::printf(
                            "Unicode canonical sequence composer: FAIL\n"
                            "  Hangul L+V sequence failed\n"
                            "  L:         U+%04X\n"
                            "  V:         U+%04X\n"
                            "  Expected:  U+%04X\n",
                            l,
                            v,
                            lv);

                        return false;
                    }


                    ++hangulLVSequences;
                }


                // -----------------------------------------------------------
                // L + V + T -> LVT
                // -----------------------------------------------------------

                for (uint32_t tIndex = 1; tIndex < kTCount; ++tIndex)
                {
                    const uint32_t t =
                        kTBase + tIndex;

                    const uint32_t expected =
                        lv + tIndex;


                    uint32_t sequence[] =
                    {
                        l,
                        v,
                        t
                    };

                    size_t count = 3;


                    if (!composer.compose(sequence, count) ||
                        count != 1 ||
                        sequence[0] != expected)
                    {
                        std::printf(
                            "Unicode canonical sequence composer: FAIL\n"
                            "  Hangul L+V+T sequence failed\n"
                            "  L:         U+%04X\n"
                            "  V:         U+%04X\n"
                            "  T:         U+%04X\n"
                            "  Expected:  U+%04X\n",
                            l,
                            v,
                            t,
                            expected);

                        return false;
                    }


                    ++hangulLVTSequences;
                }
            }
        }


        const uint32_t totalHangulSequences =
            hangulLVSequences +
            hangulLVTSequences;


        if (hangulLVSequences != kLCount * kVCount ||
            hangulLVTSequences !=
            kLCount * kVCount * (kTCount - 1u) ||
            totalHangulSequences != kSCount)
        {
            std::printf(
                "Unicode canonical sequence composer: FAIL\n"
                "  Hangul sequence accounting mismatch\n"
                "  LV:       %u\n"
                "  LVT:      %u\n"
                "  Total:    %u\n",
                hangulLVSequences,
                hangulLVTSequences,
                totalHangulSequences);

            return false;
        }


        // ====================================================================
        // Unordered input must fail transactionally
        //
        //      acute       CCC 230
        //      dot below   CCC 220
        //
        // is deliberately out of canonical order.
        // ====================================================================

        uint32_t failureChecks = 0;


        {
            if (database.combiningClass(0x0301u) != 230 ||
                database.combiningClass(0x0323u) != 220)
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Unexpected CCC values for ordering fixture\n");

                return false;
            }


            uint32_t sequence[] =
            {
                0x0041u,
                0x0301u,
                0x0323u
            };

            const uint32_t original[] =
            {
                0x0041u,
                0x0301u,
                0x0323u
            };

            size_t count = 3;


            if (composer.compose(sequence, count))
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Unordered input was accepted\n");

                return false;
            }


            if (count != 3 ||
                sequence[0] != original[0] ||
                sequence[1] != original[1] ||
                sequence[2] != original[2])
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Unordered failure modified input\n");

                return false;
            }

            ++failureChecks;
        }


        // ====================================================================
        // Invalid Unicode input must fail transactionally
        // ====================================================================

        {
            uint32_t sequence[] =
            {
                0x0041u,
                kUnicodeLimit,
                0x0300u
            };

            const uint32_t original[] =
            {
                0x0041u,
                kUnicodeLimit,
                0x0300u
            };

            size_t count = 3;


            if (composer.compose(sequence, count))
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Invalid Unicode input was accepted\n");

                return false;
            }


            if (count != 3 ||
                sequence[0] != original[0] ||
                sequence[1] != original[1] ||
                sequence[2] != original[2])
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Invalid-input failure modified sequence\n");

                return false;
            }

            ++failureChecks;
        }


        // ====================================================================
        // Invalid composer state
        // ====================================================================

        {
            UnicodeCanonicalSequenceComposer emptyComposer;

            uint32_t sequence[] =
            {
                0x0041u,
                0x0300u
            };

            size_t count = 2;


            if (emptyComposer.valid() ||
                emptyComposer.compose(sequence, count))
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Invalid composer state was accepted\n");

                return false;
            }


            if (count != 2 ||
                sequence[0] != 0x0041u ||
                sequence[1] != 0x0300u)
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  Invalid composer modified sequence\n");

                return false;
            }

            ++failureChecks;
        }


        // ====================================================================
        // clear() and reset()
        // ====================================================================

        {
            UnicodeCanonicalSequenceComposer stateComposer(database);

            if (!stateComposer.valid())
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  State-test composer did not initialize\n");

                return false;
            }


            stateComposer.clear();

            if (stateComposer.valid())
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  clear() did not invalidate composer\n");

                return false;
            }


            if (!stateComposer.reset(database) ||
                !stateComposer.valid())
            {
                std::printf(
                    "Unicode canonical sequence composer: FAIL\n"
                    "  reset() did not restore composer\n");

                return false;
            }

            ++failureChecks;
        }


        // ====================================================================
        // Success
        // ====================================================================

        std::printf(
            "Unicode canonical sequence composer: PASS\n"
            "  Database bytes:                %zu\n"
            "  Explicit sequences:            %u\n"
            "  Explicit second CCC zero:      %u\n"
            "  Explicit second CCC non-zero:  %u\n"
            "  Explicit composition chains:   %u\n"
            "  Blocking pairs eligible:       %u\n"
            "  Equal-CCC blocking cases:      %u\n"
            "  Blocking candidates missing:   %u\n"
            "  Unblocked pairs eligible:      %u\n"
            "  Lower-CCC unblocked cases:     %u\n"
            "  Unblocked candidates missing:  %u\n"
            "  Hangul L+V sequences:           %u\n"
            "  Hangul L+V+T sequences:         %u\n"
            "  Total Hangul sequences:         %u\n"
            "  Basic checks:                   %u\n"
            "  Structural checks:              %u\n"
            "  Failure-path checks:            %u\n",
            bytes.size(),
            explicitSequences,
            explicitSecondCCCZero,
            explicitSecondCCCNonZero,
            explicitChains,
            blockingEligible,
            blockingCases,
            blockingCandidatesUnavailable,
            unblockedEligible,
            unblockedCases,
            unblockedCandidatesUnavailable,
            hangulLVSequences,
            hangulLVTSequences,
            totalHangulSequences,
            basicChecks,
            structuralChecks,
            failureChecks);


        return true;
    }

} // namespace waavs
