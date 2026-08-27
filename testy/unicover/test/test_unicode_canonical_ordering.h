// test_unicode_canonical_ordering.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

#include "unicode_canonical_ordering.h"
#include "unicode_database.h"


namespace waavs
{
    // ========================================================================
    // unicodeCanonicalOrderingEquals
    // ========================================================================

    static bool unicodeCanonicalOrderingEquals(
        const uint32_t* actual,
        const uint32_t* expected,
        size_t count) noexcept
    {
        for (size_t i = 0; i < count; ++i) {
            if (actual[i] != expected[i])
                return false;
        }

        return true;
    }


    // ========================================================================
    // testUnicodeCanonicalOrdering
    // ========================================================================

    static bool testUnicodeCanonicalOrdering(const char* databaseFilename)
    {
        auto fail = [](const char* message) -> bool {
            std::printf(
                "Unicode canonical ordering: FAIL: %s\n",
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


        UnicodeCanonicalOrdering ordering(database);

        if (!ordering)
            return fail("canonical ordering unavailable");


        // ====================================================================
        // Verify CCC values used by the tests
        //
        // U+0041 =   0
        // U+0323 = 220
        // U+0300 = 230
        // U+0301 = 230
        // U+0315 = 232
        // U+0345 = 240
        // ====================================================================

        if (database.combiningClass(0x0041) != 0)
            return fail("U+0041 CCC is not zero");

        if (database.combiningClass(0x0323) != 220)
            return fail("U+0323 CCC is not 220");

        if (database.combiningClass(0x0300) != 230)
            return fail("U+0300 CCC is not 230");

        if (database.combiningClass(0x0301) != 230)
            return fail("U+0301 CCC is not 230");

        if (database.combiningClass(0x0315) != 232)
            return fail("U+0315 CCC is not 232");

        if (database.combiningClass(0x0345) != 240)
            return fail("U+0345 CCC is not 240");


        // ====================================================================
        // Empty sequence
        // ====================================================================

        if (!ordering.order(nullptr, 0))
            return fail("empty sequence failed");

        if (!ordering.isOrdered(nullptr, 0))
            return fail("empty sequence reported unordered");


        // ====================================================================
        // Single starter
        // ====================================================================

        {
            uint32_t values[] = {
                0x0041
            };

            if (!ordering.order(values, 1))
                return fail("single starter failed");

            if (values[0] != 0x0041)
                return fail("single starter changed");

            if (!ordering.isOrdered(values, 1))
                return fail("single starter reported unordered");
        }


        // ====================================================================
        // Already ordered
        //
        //      A     DOT BELOW   ACUTE   COMMA ABOVE RIGHT
        //
        // CCC:
        //
        //      0       220        230        232
        // ====================================================================

        {
            uint32_t values[] = {
                0x0041,
                0x0323,
                0x0301,
                0x0315
            };

            const uint32_t expected[] = {
                0x0041,
                0x0323,
                0x0301,
                0x0315
            };

            constexpr size_t count =
                sizeof(values) / sizeof(values[0]);


            if (!ordering.isOrdered(values, count))
                return fail("ordered sequence reported unordered");


            if (!ordering.order(values, count))
                return fail("already ordered sequence failed");


            if (!unicodeCanonicalOrderingEquals(
                values,
                expected,
                count))
            {
                return fail("already ordered sequence changed");
            }
        }


        // ====================================================================
        // Reverse ordering
        //
        // before:
        //
        //      A   232 230 220
        //
        // after:
        //
        //      A   220 230 232
        // ====================================================================

        {
            uint32_t values[] = {
                0x0041,
                0x0315,
                0x0301,
                0x0323
            };

            const uint32_t expected[] = {
                0x0041,
                0x0323,
                0x0301,
                0x0315
            };

            constexpr size_t count =
                sizeof(values) / sizeof(values[0]);


            if (ordering.isOrdered(values, count))
                return fail("reverse sequence reported ordered");


            if (!ordering.order(values, count))
                return fail("reverse sequence ordering failed");


            if (!unicodeCanonicalOrderingEquals(
                values,
                expected,
                count))
            {
                return fail("reverse sequence incorrectly ordered");
            }


            if (!ordering.isOrdered(values, count))
                return fail("ordered reverse sequence reported unordered");
        }


        // ====================================================================
        // Stable equal-CCC ordering
        //
        // U+0300 and U+0301 both have CCC=230.
        //
        // before:
        //
        //      A   0300   0301   0323
        //      0    230    230    220
        //
        // after:
        //
        //      A   0323   0300   0301
        //      0    220    230    230
        //
        // U+0300 must remain before U+0301.
        // ====================================================================

        {
            uint32_t values[] = {
                0x0041,
                0x0300,
                0x0301,
                0x0323
            };

            const uint32_t expected[] = {
                0x0041,
                0x0323,
                0x0300,
                0x0301
            };

            constexpr size_t count =
                sizeof(values) / sizeof(values[0]);


            if (!ordering.order(values, count))
                return fail("stable ordering failed");


            if (!unicodeCanonicalOrderingEquals(
                values,
                expected,
                count))
            {
                return fail("equal CCC ordering was not stable");
            }
        }


        // ====================================================================
        // Leading combining marks
        //
        // A combining sequence does not require an initial starter.
        //
        //      232 230 220
        //
        // becomes:
        //
        //      220 230 232
        // ====================================================================

        {
            uint32_t values[] = {
                0x0315,
                0x0301,
                0x0323
            };

            const uint32_t expected[] = {
                0x0323,
                0x0301,
                0x0315
            };

            constexpr size_t count =
                sizeof(values) / sizeof(values[0]);


            if (!ordering.order(values, count))
                return fail("leading-mark ordering failed");


            if (!unicodeCanonicalOrderingEquals(
                values,
                expected,
                count))
            {
                return fail("leading marks incorrectly ordered");
            }
        }


        // ====================================================================
        // Multiple starters
        //
        // CCC zero establishes a new ordering boundary.
        //
        //      A 232 230 220   B 240 220 230
        //
        // becomes:
        //
        //      A 220 230 232   B 220 230 240
        // ====================================================================

        {
            uint32_t values[] = {
                0x0041,
                0x0315,
                0x0301,
                0x0323,

                0x0042,
                0x0345,
                0x0323,
                0x0301
            };

            const uint32_t expected[] = {
                0x0041,
                0x0323,
                0x0301,
                0x0315,

                0x0042,
                0x0323,
                0x0301,
                0x0345
            };

            constexpr size_t count =
                sizeof(values) / sizeof(values[0]);


            if (!ordering.order(values, count))
                return fail("multiple-starter ordering failed");


            if (!unicodeCanonicalOrderingEquals(
                values,
                expected,
                count))
            {
                return fail(
                    "multiple combining sequences incorrectly ordered");
            }
        }


        // ====================================================================
        // Cross-source decomposition boundary
        //
        // Imagine:
        //
        //      source cp1 -> A U+0300
        //      source cp2 -> U+0323
        //
        // After decomposition:
        //
        //      A U+0300 U+0323
        //      0   230    220
        //
        // Canonical ordering must cross the original source boundary:
        //
        //      A U+0323 U+0300
        //      0   220    230
        // ====================================================================

        {
            uint32_t values[] = {
                0x0041,
                0x0300,
                0x0323
            };

            const uint32_t expected[] = {
                0x0041,
                0x0323,
                0x0300
            };

            constexpr size_t count =
                sizeof(values) / sizeof(values[0]);


            if (!ordering.order(values, count))
                return fail("cross-source ordering failed");


            if (!unicodeCanonicalOrderingEquals(
                values,
                expected,
                count))
            {
                return fail("ordering stopped at source boundary");
            }
        }


        // ====================================================================
        // Idempotence
        //
        // Applying canonical ordering twice must produce the same sequence.
        // ====================================================================

        {
            uint32_t values[] = {
                0x0041,
                0x0315,
                0x0300,
                0x0323,
                0x0301,
                0x0345
            };

            constexpr size_t count =
                sizeof(values) / sizeof(values[0]);

            uint32_t once[
                sizeof(values) / sizeof(values[0])];


            if (!ordering.order(values, count))
                return fail("idempotence first ordering failed");


            for (size_t i = 0; i < count; ++i)
                once[i] = values[i];


            if (!ordering.order(values, count))
                return fail("idempotence second ordering failed");


            if (!unicodeCanonicalOrderingEquals(
                values,
                once,
                count))
            {
                return fail("canonical ordering is not idempotent");
            }
        }


        // ====================================================================
        // Invalid code point
        //
        // Input validation occurs before modification.
        // ====================================================================

        {
            uint32_t values[] = {
                0x0315,
                kUnicodeLimit,
                0x0323
            };

            const uint32_t original[] = {
                0x0315,
                kUnicodeLimit,
                0x0323
            };

            constexpr size_t count =
                sizeof(values) / sizeof(values[0]);


            if (ordering.order(values, count))
                return fail("invalid code point unexpectedly succeeded");


            if (!unicodeCanonicalOrderingEquals(
                values,
                original,
                count))
            {
                return fail("failed ordering modified input");
            }


            if (ordering.isOrdered(values, count))
                return fail("invalid sequence reported ordered");
        }


        // ====================================================================
        // Null buffer with non-zero length
        // ====================================================================

        if (ordering.order(nullptr, 1))
            return fail("null input unexpectedly succeeded");

        if (ordering.isOrdered(nullptr, 1))
            return fail("null input reported ordered");


        // ====================================================================
        // Scan complete CCC database
        //
        // Verify that the runtime database supplying the ordering algorithm
        // retains the expected Unicode 17 shape.
        // ====================================================================

        size_t nonZeroCombiningClasses = 0;
        uint32_t maximumCCC = 0;


        for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
        {
            const UnicodeCombiningClass ccc =
                database.combiningClass(cp);


            if (ccc !=
                kUnicodeCombiningClassNotReordered)
            {
                ++nonZeroCombiningClasses;
            }


            if (ccc > maximumCCC)
                maximumCCC = ccc;
        }


        if (nonZeroCombiningClasses != 968)
            return fail("non-zero CCC code-point count is not 968");


        // ====================================================================
        // PASS
        // ====================================================================

        std::printf(
            "Unicode canonical ordering: PASS\n"
            "  Unicode code points:        %u\n"
            "  Non-zero CCC code points:   %zu\n"
            "  Maximum CCC observed:       %u\n"
            "  Stable equal-CCC ordering:  PASS\n"
            "  Leading combining marks:    PASS\n"
            "  Multiple starters:          PASS\n"
            "  Cross-source ordering:      PASS\n"
            "  Idempotence:                PASS\n",
            kUnicodeLimit,
            nonZeroCombiningClasses,
            maximumCCC);


        return true;
    }

} // namespace waavs