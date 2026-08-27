// test_unicode_composition.h

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

#include "test_core.h"
#include "unicode_database.h"

namespace waavs {
    static bool testUnicodeComposition(const char* databaseFilename)
    {
        if (!databaseFilename)
            return false;


        // ========================================================================
        // Load the actual generated .ucdb image
        // ========================================================================

        std::ifstream input(
            databaseFilename,
            std::ios::binary |
            std::ios::ate);


        if (!input)
        {
            std::printf(
                "Unicode composition lookup: FAIL\n"
                "  Unable to open database: %s\n",
                databaseFilename);

            return false;
        }


        const std::streampos end =
            input.tellg();


        if (end <= 0)
        {
            std::printf(
                "Unicode composition lookup: FAIL\n"
                "  Database is empty or unreadable\n");

            return false;
        }


        std::vector<uint8_t> bytes(
            static_cast<size_t>(end));


        input.seekg(
            0,
            std::ios::beg);


        input.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));


        if (!input)
        {
            std::printf(
                "Unicode composition lookup: FAIL\n"
                "  Unable to read database\n");

            return false;
        }


        // ========================================================================
        // Attach the real runtime database reader
        // ========================================================================

        UnicodeDatabase database;


        if (!database.reset(
            ByteSpan(bytes.data(), bytes.size())))
        {
            std::printf(
                "Unicode composition lookup: FAIL\n"
                "  UnicodeDatabase::reset() failed\n");

            return false;
        }


        if (!database.hasComposition())
        {
            std::printf(
                "Unicode composition lookup: FAIL\n"
                "  Database has no canonical composition section\n");

            return false;
        }


        if (database.compositionRecordCount() != 961)
        {
            std::printf(
                "Unicode composition lookup: FAIL\n"
                "  Expected records: 961\n"
                "  Actual records:   %u\n",
                database.compositionRecordCount());

            return false;
        }


        // ========================================================================
        // Obtain the runtime composition view
        // ========================================================================

        const UnicodeComposition composition =
            database.composition();


        if (!composition.valid())
        {
            std::printf(
                "Unicode composition lookup: FAIL\n"
                "  UnicodeComposition view is invalid\n");

            return false;
        }


        if (composition.size() !=
            database.compositionRecordCount())
        {
            std::printf(
                "Unicode composition lookup: FAIL\n"
                "  View size:     %u\n"
                "  Database size: %u\n",
                composition.size(),
                database.compositionRecordCount());

            return false;
        }


        if (composition.data() !=
            database.compositionRecords())
        {
            std::printf(
                "Unicode composition lookup: FAIL\n"
                "  View does not reference persistent database records\n");

            return false;
        }


        // ========================================================================
        // Known explicit mappings
        //
        // These verify recognizable semantic results independently of the
        // exhaustive record walk below.
        // ========================================================================

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


        for (const KnownMapping& expected : knownMappings)
        {
            const UnicodeCompositionRecord* record =
                composition.record(
                    expected.first,
                    expected.second);


            if (!record ||
                record->composite != expected.composite)
            {
                std::printf(
                    "Unicode composition lookup: FAIL\n"
                    "  Known mapping failed\n"
                    "  First:     U+%04X\n"
                    "  Second:    U+%04X\n"
                    "  Expected:  U+%04X\n"
                    "  Actual:    U+%04X\n",
                    expected.first,
                    expected.second,
                    expected.composite,
                    record
                    ? record->composite
                    : kUnicodeCompositionNone);

                return false;
            }


            if (composition.composite(
                expected.first,
                expected.second) !=
                expected.composite)
            {
                std::printf(
                    "Unicode composition lookup: FAIL\n"
                    "  composite() failed for U+%04X + U+%04X\n",
                    expected.first,
                    expected.second);

                return false;
            }


            if (!composition.hasMapping(
                expected.first,
                expected.second))
            {
                std::printf(
                    "Unicode composition lookup: FAIL\n"
                    "  hasMapping() failed for U+%04X + U+%04X\n",
                    expected.first,
                    expected.second);

                return false;
            }
        }


        // ========================================================================
        // Exhaustive persistent-record lookup
        //
        // Walk every canonical composition record physically stored in the live
        // database and look the pair back up through UnicodeComposition.
        //
        // For every record:
        //
        //      persistent record
        //              |
        //              v
        //      UnicodeComposition::record(first, second)
        //              |
        //              v
        //      same persistent record
        //
        // This exercises all 961 explicit Unicode 17 canonical composition pairs.
        // ========================================================================

        uint32_t exhaustiveLookups = 0;


        for (uint32_t i = 0;
            i < database.compositionRecordCount();
            ++i)
        {
            const UnicodeCompositionRecord* persistent =
                database.compositionRecord(i);


            if (!persistent)
            {
                std::printf(
                    "Unicode composition lookup: FAIL\n"
                    "  Unable to access persistent record %u\n",
                    i);

                return false;
            }


            // --------------------------------------------------------------------
            // Indexed access through the view must identify the same mapped record.
            // --------------------------------------------------------------------

            const UnicodeCompositionRecord* indexed =
                composition.recordAt(i);


            if (indexed != persistent)
            {
                std::printf(
                    "Unicode composition lookup: FAIL\n"
                    "  recordAt() pointer mismatch at record %u\n",
                    i);

                return false;
            }


            // --------------------------------------------------------------------
            // Verify persistent ordering independently.
            //
            // UnicodeDatabase::reset() already validates this invariant, but doing
            // so here makes the exhaustive test's assumptions explicit.
            // --------------------------------------------------------------------

            if (i != 0)
            {
                const UnicodeCompositionRecord* previous =
                    database.compositionRecord(i - 1u);


                if (!previous)
                    return false;


                if (previous->first > persistent->first ||
                    (previous->first == persistent->first &&
                        previous->second >= persistent->second))
                {
                    std::printf(
                        "Unicode composition lookup: FAIL\n"
                        "  Persistent ordering failure at record %u\n",
                        i);

                    return false;
                }
            }


            // --------------------------------------------------------------------
            // Binary lookup must return this exact persistent record.
            // --------------------------------------------------------------------

            const UnicodeCompositionRecord* found =
                composition.record(
                    persistent->first,
                    persistent->second);


            if (found != persistent)
            {
                std::printf(
                    "Unicode composition lookup: FAIL\n"
                    "  Lookup mismatch at record %u\n"
                    "  First:      U+%04X\n"
                    "  Second:     U+%04X\n"
                    "  Composite:  U+%04X\n",
                    i,
                    persistent->first,
                    persistent->second,
                    persistent->composite);

                return false;
            }


            // --------------------------------------------------------------------
            // Convenience APIs must agree with record().
            // --------------------------------------------------------------------

            if (!composition.hasMapping(
                persistent->first,
                persistent->second))
            {
                std::printf(
                    "Unicode composition lookup: FAIL\n"
                    "  hasMapping() failed at record %u\n",
                    i);

                return false;
            }


            if (composition.composite(
                persistent->first,
                persistent->second) !=
                persistent->composite)
            {
                std::printf(
                    "Unicode composition lookup: FAIL\n"
                    "  composite() failed at record %u\n",
                    i);

                return false;
            }


            ++exhaustiveLookups;
        }


        // ========================================================================
        // Hangul is deliberately absent from the explicit composition table
        //
        // These mappings will eventually be handled algorithmically by
        // UnicodeCanonicalComposer.
        //
        //      U+1100 + U+1161 -> U+AC00
        //      U+AC00 + U+11A8 -> U+AC01
        //
        // Neither pair should exist in UnicodeComposition.
        // ========================================================================

        struct AbsentMapping
        {
            uint32_t first;
            uint32_t second;
        };


        static constexpr AbsentMapping absentMappings[] =
        {
            { 0x1100u, 0x1161u },
            { 0xAC00u, 0x11A8u }
        };


        for (const AbsentMapping& absent : absentMappings)
        {
            if (composition.record(
                absent.first,
                absent.second) != nullptr)
            {
                std::printf(
                    "Unicode composition lookup: FAIL\n"
                    "  Algorithmic Hangul pair found in explicit table\n"
                    "  First:  U+%04X\n"
                    "  Second: U+%04X\n",
                    absent.first,
                    absent.second);

                return false;
            }


            if (composition.hasMapping(
                absent.first,
                absent.second))
            {
                std::printf(
                    "Unicode composition lookup: FAIL\n"
                    "  hasMapping() accepted algorithmic Hangul pair\n");

                return false;
            }


            if (composition.composite(
                absent.first,
                absent.second) !=
                kUnicodeCompositionNone)
            {
                std::printf(
                    "Unicode composition lookup: FAIL\n"
                    "  composite() accepted algorithmic Hangul pair\n");

                return false;
            }
        }


        // ========================================================================
        // Invalid Unicode input
        // ========================================================================

        if (composition.record(
            kUnicodeLimit,
            0x0300u) != nullptr ||
            composition.record(
                0x0041u,
                kUnicodeLimit) != nullptr)
        {
            std::printf(
                "Unicode composition lookup: FAIL\n"
                "  Out-of-range Unicode input was accepted\n");

            return false;
        }


        if (composition.composite(
            kUnicodeLimit,
            0x0300u) !=
            kUnicodeCompositionNone ||
            composition.composite(
                0x0041u,
                kUnicodeLimit) !=
            kUnicodeCompositionNone)
        {
            std::printf(
                "Unicode composition lookup: FAIL\n"
                "  composite() accepted out-of-range input\n");

            return false;
        }


        // ========================================================================
        // Success
        // ========================================================================

        std::printf(
            "Unicode composition lookup: PASS\n"
            "  Database bytes:          %zu\n"
            "  Composition records:     %u\n"
            "  Exhaustive lookups:      %u\n"
            "  Known mappings checked:  %zu\n"
            "  Hangul pairs excluded:   %zu\n"
            "  Invalid input checks:    2\n",
            bytes.size(),
            database.compositionRecordCount(),
            exhaustiveLookups,
            sizeof(knownMappings) / sizeof(knownMappings[0]),
            sizeof(absentMappings) / sizeof(absentMappings[0]));


        return true;
    }
}