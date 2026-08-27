// test_unicode_value_table8.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>

#include "unicode_value_table8.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    // ========================================================================
    // testUnicodeValueTable8
    //
    // Exercise the complete VALUE8 construction/runtime path entirely
    // in memory.
    //
    // The test deliberately constructs:
    //
    //      - many completely uniform zero masters
    //      - two identical mixed masters
    //      - one uniform-value sub-page within each mixed master
    //      - two identical nonuniform sub-pages within each mixed master
    //
    // This allows us to verify:
    //
    //      uniform master collapse
    //      uniform sub-page collapse
    //      physical value-page lookup
    //      value-page deduplication
    //      master-page deduplication
    //      all 256 uniform reference values
    //
    // Expected canonical pool:
    //
    //      unique value pages:    1
    //      unique master pages:   1
    //
    // ========================================================================

    static bool testUnicodeValueTable8()
    {
        auto fail = [](const char* message) -> bool {
            std::printf("UnicodeValueTable8: FAIL: %s\n", message);
            return false;
            };


        // ====================================================================
        // Verify the complete inline uniform-reference encoding.
        // ====================================================================

        for (uint32_t value = 0; value < 256; ++value)
        {
            const uint8_t byteValue =
                static_cast<uint8_t>(value);

            const uint16_t ref =
                unicodeValue8UniformRef(byteValue);

            if (!unicodeValue8RefIsUniform(ref))
                return fail("uniform reference not recognized");

            if (unicodeValue8RefUniformValue(ref) != byteValue)
                return fail("uniform reference decoded incorrectly");
        }


        // ====================================================================
        // Builder
        //
        // UnicodeValueTable8Builder is approximately 1.1 MiB, so deliberately
        // place it on the heap rather than consuming thread stack space.
        // ====================================================================

        auto builder =
            std::make_unique<UnicodeValueTable8Builder>(0);


        // ====================================================================
        // Construct two identical mixed masters.
        //
        // Within each:
        //
        //      sub 0       uniform value 7
        //
        //      sub 1       sparse nonuniform pattern
        //      sub 2       identical sparse nonuniform pattern
        //
        //      sub 3..31   uniform value 0
        //
        //
        // Because masters 1 and 2 have identical logical contents, their
        // finalized master-page representation should deduplicate.
        //
        // Because sub-pages 1 and 2 contain the same sparse pattern in both
        // masters, all four nonuniform logical pages should deduplicate to
        // one physical UnicodeValuePage8.
        // ====================================================================

        for (uint32_t mi = 1; mi <= 2; ++mi)
        {
            const uint32_t masterStart =
                mi << kUnicodeMasterShift;


            // ---------------------------------------------------------------
            // Sub-page 0: uniform value 7.
            // ---------------------------------------------------------------

            builder->setRange(
                masterStart,
                masterStart + kUnicodeSubSize - 1u,
                7);


            // ---------------------------------------------------------------
            // Sub-pages 1 and 2: identical nonuniform pattern.
            // ---------------------------------------------------------------

            for (uint32_t si = 1; si <= 2; ++si)
            {
                const uint32_t pageStart =
                    masterStart +
                    (si << kUnicodeSubShift);

                builder->set(pageStart + 0u, 11);
                builder->set(pageStart + 1u, 22);
                builder->set(pageStart + 511u, 33);
                builder->set(pageStart + 1023u, 44);
            }
        }


        // ====================================================================
        // Verify mutable builder values before finalization.
        // ====================================================================

        const uint32_t master1 =
            1u << kUnicodeMasterShift;

        const uint32_t master2 =
            2u << kUnicodeMasterShift;


        if (builder->value(0) != 0)
            return fail("default builder value is incorrect");

        if (builder->value(master1) != 7)
            return fail("uniform builder page value is incorrect");

        if (builder->value(master1 + kUnicodeSubSize + 0u) != 11)
            return fail("builder sparse value 0 is incorrect");

        if (builder->value(master1 + kUnicodeSubSize + 1u) != 22)
            return fail("builder sparse value 1 is incorrect");

        if (builder->value(master1 + kUnicodeSubSize + 511u) != 33)
            return fail("builder sparse value 511 is incorrect");

        if (builder->value(master1 + kUnicodeSubSize + 1023u) != 44)
            return fail("builder sparse value 1023 is incorrect");


        // ====================================================================
        // Finalize.
        // ====================================================================

        UnicodeValuePagePoolBuilder8 pool;
        UnicodeValueTable8Data tableData{};


        if (!builder->finalize(pool, tableData))
            return fail("finalize failed");


        // ====================================================================
        // Expected pool statistics.
        //
        // There are:
        //
        //      34 * 32 = 1088 logical sub-pages
        //
        // Four are nonuniform:
        //
        //      master 1 / sub 1
        //      master 1 / sub 2
        //      master 2 / sub 1
        //      master 2 / sub 2
        //
        // They are all identical, so:
        //
        //      unique physical value pages = 1
        //      reused value pages          = 3
        //
        //
        // Of 34 masters:
        //
        //      32 are completely uniform zero
        //       2 are identical mixed masters
        //
        // Therefore:
        //
        //      unique physical masters = 1
        //      reused masters          = 1
        //
        // ====================================================================

        const UnicodeValuePagePoolStats8& stats =
            pool.stats();


        if (stats.valuePageRequests !=
            static_cast<size_t>(kUnicodeMasterCount) *
            kUnicodeSubsPerMaster)
        {
            return fail("unexpected value-page request count");
        }

        if (stats.valuePageUniform != 1084)
            return fail("unexpected uniform value-page count");

        if (stats.uniqueValuePages != 1)
            return fail("unexpected unique value-page count");

        if (stats.valuePageReused != 3)
            return fail("unexpected reused value-page count");


        if (stats.masterPageRequests != kUnicodeMasterCount)
            return fail("unexpected master-page request count");

        if (stats.masterPageUniform != 32)
            return fail("unexpected uniform master-page count");

        if (stats.uniqueMasterPages != 1)
            return fail("unexpected unique master-page count");

        if (stats.masterPageReused != 1)
            return fail("unexpected reused master-page count");


        // ====================================================================
        // Inspect canonical references directly.
        // ====================================================================

        const UnicodeValueMasterPage8Ref master0Ref =
            tableData.masters[0];

        const UnicodeValueMasterPage8Ref master1Ref =
            tableData.masters[1];

        const UnicodeValueMasterPage8Ref master2Ref =
            tableData.masters[2];

        const UnicodeValueMasterPage8Ref master3Ref =
            tableData.masters[3];


        // ---------------------------------------------------------------
        // Default masters should collapse directly to uniform zero.
        // ---------------------------------------------------------------

        if (!unicodeValue8RefIsUniform(master0Ref))
            return fail("master 0 did not collapse to uniform");

        if (unicodeValue8RefUniformValue(master0Ref) != 0)
            return fail("master 0 uniform value is incorrect");

        if (!unicodeValue8RefIsUniform(master3Ref))
            return fail("master 3 did not collapse to uniform");

        if (unicodeValue8RefUniformValue(master3Ref) != 0)
            return fail("master 3 uniform value is incorrect");


        // ---------------------------------------------------------------
        // Masters 1 and 2 are mixed and therefore physical.
        // ---------------------------------------------------------------

        if (unicodeValue8RefIsUniform(master1Ref))
            return fail("mixed master 1 collapsed incorrectly");

        if (unicodeValue8RefIsUniform(master2Ref))
            return fail("mixed master 2 collapsed incorrectly");


        // ---------------------------------------------------------------
        // Their complete representation is identical, so both should
        // reference the same physical master page.
        // ---------------------------------------------------------------

        if (master1Ref != master2Ref)
            return fail("identical master pages were not deduplicated");


        const UnicodeValueMasterPage8* mixedMaster =
            pool.masterPage(master1Ref);

        if (!mixedMaster)
            return fail("unable to retrieve mixed master page");


        // ---------------------------------------------------------------
        // Sub-page 0 should be inline uniform value 7.
        // ---------------------------------------------------------------

        if (!unicodeValue8RefIsUniform(mixedMaster->sub[0]))
            return fail("uniform sub-page was stored physically");

        if (unicodeValue8RefUniformValue(mixedMaster->sub[0]) != 7)
            return fail("uniform sub-page value is incorrect");


        // ---------------------------------------------------------------
        // Sub-pages 1 and 2 are nonuniform.
        // ---------------------------------------------------------------

        if (unicodeValue8RefIsUniform(mixedMaster->sub[1]))
            return fail("nonuniform sub-page 1 collapsed incorrectly");

        if (unicodeValue8RefIsUniform(mixedMaster->sub[2]))
            return fail("nonuniform sub-page 2 collapsed incorrectly");


        // ---------------------------------------------------------------
        // Their contents are identical, so they should reference the same
        // physical value page.
        // ---------------------------------------------------------------

        if (mixedMaster->sub[1] != mixedMaster->sub[2])
            return fail("identical value pages were not deduplicated");


        const UnicodeValuePage8* physicalPage =
            pool.valuePage(mixedMaster->sub[1]);

        if (!physicalPage)
            return fail("unable to retrieve physical value page");


        if (physicalPage->values[0] != 11 ||
            physicalPage->values[1] != 22 ||
            physicalPage->values[511] != 33 ||
            physicalPage->values[1023] != 44)
        {
            return fail("physical value-page contents are incorrect");
        }


        if (physicalPage->values[2] != 0 ||
            physicalPage->values[510] != 0 ||
            physicalPage->values[512] != 0)
        {
            return fail("physical value-page default values are incorrect");
        }


        // ====================================================================
        // Construct runtime non-owning view.
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
            return fail("runtime table is invalid");


        // ====================================================================
        // Runtime lookup: uniform master.
        // ====================================================================

        if (table.value(0x0041) != 0)
            return fail("uniform master lookup failed");

        if (table.value(3u << kUnicodeMasterShift) != 0)
            return fail("later uniform master lookup failed");


        // ====================================================================
        // Runtime lookup: uniform sub-page inside mixed master.
        // ====================================================================

        if (table.value(master1 + 0u) != 7)
            return fail("uniform sub-page first value failed");

        if (table.value(master1 + 500u) != 7)
            return fail("uniform sub-page middle value failed");

        if (table.value(master1 + 1023u) != 7)
            return fail("uniform sub-page last value failed");


        // ====================================================================
        // Runtime lookup: physical nonuniform page.
        // ====================================================================

        const uint32_t page1 =
            master1 +
            kUnicodeSubSize;


        if (table.value(page1 + 0u) != 11)
            return fail("physical page value 0 failed");

        if (table.value(page1 + 1u) != 22)
            return fail("physical page value 1 failed");

        if (table.value(page1 + 2u) != 0)
            return fail("physical page default value failed");

        if (table.value(page1 + 511u) != 33)
            return fail("physical page value 511 failed");

        if (table.value(page1 + 1023u) != 44)
            return fail("physical page value 1023 failed");


        // ====================================================================
        // Runtime lookup through the duplicate logical page.
        // ====================================================================

        const uint32_t page2 =
            master1 +
            2u * kUnicodeSubSize;


        if (table.value(page2 + 0u) != 11 ||
            table.value(page2 + 511u) != 33 ||
            table.value(page2 + 1023u) != 44)
        {
            return fail("duplicate logical page lookup failed");
        }


        // ====================================================================
        // Runtime lookup through deduplicated second master.
        // ====================================================================

        if (table.value(master2 + 0u) != 7)
            return fail("deduplicated master uniform-page lookup failed");

        if (table.value(master2 + kUnicodeSubSize + 1u) != 22)
            return fail("deduplicated master physical-page lookup failed");


        // ====================================================================
        // Out-of-range behavior.
        // ====================================================================

        if (table.value(kUnicodeLimit) != 0)
            return fail("out-of-range lookup did not return zero");


        // ====================================================================
        // Diagnostics
        // ====================================================================

        std::printf("UnicodeValueTable8: PASS\n");

        std::printf(
            "  Value page requests:      %zu\n"
            "  Uniform value pages:      %zu\n"
            "  Reused value pages:       %zu\n"
            "  Unique value pages:       %zu\n"
            "  Value hash collisions:    %zu\n",
            stats.valuePageRequests,
            stats.valuePageUniform,
            stats.valuePageReused,
            stats.uniqueValuePages,
            stats.valuePageHashCollisions);

        std::printf(
            "  Master page requests:     %zu\n"
            "  Uniform master pages:     %zu\n"
            "  Reused master pages:      %zu\n"
            "  Unique master pages:      %zu\n"
            "  Master hash collisions:   %zu\n",
            stats.masterPageRequests,
            stats.masterPageUniform,
            stats.masterPageReused,
            stats.uniqueMasterPages,
            stats.masterPageHashCollisions);

        std::printf(
            "  Stored VALUE8 bytes:      %zu\n",
            stats.storedBytes());


        return true;
    }

} // namespace waavs
