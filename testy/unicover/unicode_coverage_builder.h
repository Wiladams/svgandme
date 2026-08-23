// unicode_coverage_builder.h

#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

#include "unicode_coverage_data.h"
#include "unicode_page_pool_builder.h"


namespace waavs
{
    // ========================================================================
    // UnicodeCoverageBuilder
    //
    // Mutable Unicode coverage representation.
    //
    // This representation is optimized for construction rather than lookup
    // or persistent storage.
    //
    // Unicode is divided into:
    //
    //      master plane    32768 code points
    //      sub-plane        1024 code points
    //
    // Temporary storage is sparse:
    //
    //      MasterPlane objects are allocated only when used.
    //      SubPlane objects are allocated only when used.
    //
    // finalize() converts the mutable representation into the canonical
    // persistent UnicodeCoverageData representation using a shared
    // UnicodePagePoolBuilder.
    //
    // The resulting hierarchy is:
    //
    //      UnicodeCoverageData
    //              |
    //              v
    //      shared UnicodeMasterPage pool
    //              |
    //              v
    //      shared UnicodeBitPage pool
    //
    // Empty and full regions are collapsed to sentinel references by the
    // page-pool builder.
    //
    // ========================================================================

    class UnicodeCoverageBuilder
    {
    private:
        // ====================================================================
        // SubPlane
        //
        // Temporary 1024-code-point bitmap.
        // ====================================================================

        struct SubPlane
        {
            std::array<uint64_t, 16> bits{};

            [[nodiscard]]
            bool empty() const noexcept
            {
                for (uint64_t word : bits)
                {
                    if (word != 0)
                        return false;
                }

                return true;
            }


            [[nodiscard]]
            bool full() const noexcept
            {
                for (uint64_t word : bits)
                {
                    if (word != ~uint64_t(0))
                        return false;
                }

                return true;
            }
        };


        // ====================================================================
        // MasterPlane
        //
        // Temporary sparse collection of 32 SubPlanes.
        // ====================================================================

        struct MasterPlane
        {
            std::array<
                std::unique_ptr<SubPlane>,
                kUnicodeSubsPerMaster> sub{};
        };


    public:
        UnicodeCoverageBuilder() = default;

        UnicodeCoverageBuilder(const UnicodeCoverageBuilder&) = delete;
        UnicodeCoverageBuilder& operator=(const UnicodeCoverageBuilder&) = delete;

        UnicodeCoverageBuilder(UnicodeCoverageBuilder&&) noexcept = default;
        UnicodeCoverageBuilder& operator=(UnicodeCoverageBuilder&&) noexcept = default;


        // ====================================================================
        // clear
        // ====================================================================

        void clear() noexcept
        {
            for (auto& master : mMasters)
                master.reset();
        }


        // ====================================================================
        // empty
        // ====================================================================

        [[nodiscard]]
        bool empty() const noexcept
        {
            for (const auto& master : mMasters)
            {
                if (master)
                    return false;
            }

            return true;
        }


        // ====================================================================
        // add
        //
        // Add one Unicode code point.
        //
        // Values outside the Unicode code-point space are ignored.
        // ====================================================================

        void add(uint32_t cp)
        {
            if (cp >= kUnicodeLimit)
                return;


            const uint32_t masterIndex =
                cp >> kUnicodeMasterShift;


            const uint32_t subIndex =
                (cp >> kUnicodeSubShift) &
                (kUnicodeSubsPerMaster - 1u);


            const uint32_t bitIndex =
                cp & (kUnicodeSubSize - 1u);


            SubPlane& sub =
                ensureSubPlane(masterIndex, subIndex);


            sub.bits[bitIndex >> 6] |=
                uint64_t(1) << (bitIndex & 63u);
        }


        // ====================================================================
        // addRange
        //
        // Add the inclusive range:
        //
        //      first ... last
        //
        // The range is clipped to the Unicode code-point address space.
        //
        // Processing is performed one 1024-code-point sub-plane at a time,
        // rather than one code point at a time.
        // ====================================================================

        void addRange(uint32_t first, uint32_t last)
        {
            if (first > last)
                return;

            if (first >= kUnicodeLimit)
                return;

            if (last >= kUnicodeLimit)
                last = kUnicodeLimit - 1u;


            while (first <= last)
            {
                const uint32_t masterIndex =
                    first >> kUnicodeMasterShift;


                const uint32_t subIndex =
                    (first >> kUnicodeSubShift) &
                    (kUnicodeSubsPerMaster - 1u);


                const uint32_t subStart =
                    first & ~(kUnicodeSubSize - 1u);


                const uint32_t subEnd =
                    subStart +
                    kUnicodeSubSize -
                    1u;


                const uint32_t final =
                    last < subEnd
                    ? last
                    : subEnd;


                SubPlane& sub =
                    ensureSubPlane(masterIndex, subIndex);


                setBits(
                    sub,
                    first - subStart,
                    final - subStart);


                if (final == last)
                    break;


                first =
                    final + 1u;
            }
        }


        // ====================================================================
        // contains
        //
        // Query the mutable builder directly.
        //
        // Primarily useful during construction and testing.
        // ====================================================================

        [[nodiscard]]
        bool contains(uint32_t cp) const noexcept
        {
            if (cp >= kUnicodeLimit)
                return false;


            const uint32_t masterIndex =
                cp >> kUnicodeMasterShift;


            const MasterPlane* master =
                mMasters[masterIndex].get();


            if (!master)
                return false;


            const uint32_t subIndex =
                (cp >> kUnicodeSubShift) &
                (kUnicodeSubsPerMaster - 1u);


            const SubPlane* sub =
                master->sub[subIndex].get();


            if (!sub)
                return false;


            const uint32_t bitIndex =
                cp & (kUnicodeSubSize - 1u);


            return
                (sub->bits[bitIndex >> 6] &
                    (uint64_t(1) << (bitIndex & 63u))) != 0;
        }


        // ====================================================================
        // finalize
        //
        // Convert this mutable coverage into its canonical persistent form.
        //
        // Leaf pages are interned first:
        //
        //      SubPlane
        //          ->
        //      UnicodeBitPage
        //          ->
        //      UnicodeBitPageRef
        //
        // Those references are then assembled into a UnicodeMasterPage:
        //
        //      32 UnicodeBitPageRef values
        //          ->
        //      UnicodeMasterPage
        //          ->
        //      UnicodeMasterPageRef
        //
        // Finally the 34 master references form UnicodeCoverageData.
        //
        // Returns false if the shared page-pool builder cannot accept one of
        // the generated pages.
        //
        // outCoverage is changed only after the complete coverage has been
        // successfully finalized.
        // ====================================================================

        [[nodiscard]]
        bool finalize(UnicodePagePoolBuilder& pool, UnicodeCoverageData& outCoverage) const
        {
            UnicodeCoverageData result;


            // ---------------------------------------------------------------
            // Start with a completely empty coverage.
            // ---------------------------------------------------------------

            for (uint32_t mi = 0; mi < kUnicodeMasterCount; ++mi)
                result.masters[mi] = kUnicodePageEmpty;


            // ---------------------------------------------------------------
            // Process each master region independently.
            // ---------------------------------------------------------------

            for (uint32_t mi = 0; mi < kUnicodeMasterCount; ++mi)
            {
                const MasterPlane* sourceMaster =
                    mMasters[mi].get();


                if (!sourceMaster)
                    continue;


                UnicodeMasterPage masterPage{};


                // -----------------------------------------------------------
                // Begin with every sub-plane empty.
                //
                // The pool builder will derive nonEmptyMask and fullMask
                // itself, so those values need not be constructed here.
                // -----------------------------------------------------------

                for (uint32_t si = 0; si < kUnicodeSubsPerMaster; ++si)
                    masterPage.sub[si] = kUnicodePageEmpty;


                // -----------------------------------------------------------
                // Finalize each allocated sub-plane.
                // -----------------------------------------------------------

                for (uint32_t si = 0; si < kUnicodeSubsPerMaster; ++si)
                {
                    const SubPlane* sourceSub =
                        sourceMaster->sub[si].get();


                    if (!sourceSub)
                        continue;


                    // This normally cannot occur because SubPlane allocation
                    // happens only as bits are added, but keeping the test
                    // makes the temporary representation robust.
                    if (sourceSub->empty())
                        continue;


                    UnicodeBitPage bitPage{};


                    std::memcpy(
                        bitPage.bits,
                        sourceSub->bits.data(),
                        sizeof(bitPage.bits));


                    UnicodeBitPageRef bitRef;


                    if (!pool.internBitPage(bitPage, bitRef))
                        return false;


                    masterPage.sub[si] =
                        bitRef;
                }


                // -----------------------------------------------------------
                // Intern the master page.
                //
                // UnicodePagePoolBuilder canonicalizes:
                //
                //      nonEmptyMask
                //      fullMask
                //      reserved[]
                //
                // and collapses completely empty/full masters into sentinel
                // references.
                // -----------------------------------------------------------

                UnicodeMasterPageRef masterRef;


                if (!pool.internMasterPage(masterPage, masterRef))
                    return false;


                result.masters[mi] =
                    masterRef;
            }


            // ---------------------------------------------------------------
            // Commit only after complete success.
            // ---------------------------------------------------------------

            outCoverage =
                result;


            return true;
        }


    private:
        // ====================================================================
        // Temporary sparse storage
        // ====================================================================

        std::array<
            std::unique_ptr<MasterPlane>,
            kUnicodeMasterCount> mMasters{};


        // ====================================================================
        // ensureMaster
        // ====================================================================

        [[nodiscard]]
        MasterPlane& ensureMaster(uint32_t masterIndex)
        {
            assert(masterIndex < kUnicodeMasterCount);


            auto& ptr =
                mMasters[masterIndex];


            if (!ptr)
                ptr = std::make_unique<MasterPlane>();


            return *ptr;
        }


        // ====================================================================
        // ensureSubPlane
        // ====================================================================

        [[nodiscard]]
        SubPlane& ensureSubPlane(uint32_t masterIndex, uint32_t subIndex)
        {
            assert(masterIndex < kUnicodeMasterCount);
            assert(subIndex < kUnicodeSubsPerMaster);


            MasterPlane& master =
                ensureMaster(masterIndex);


            auto& ptr =
                master.sub[subIndex];


            if (!ptr)
                ptr = std::make_unique<SubPlane>();


            return *ptr;
        }


        // ====================================================================
        // setBits
        //
        // Set an inclusive range within one 1024-bit SubPlane.
        //
        //      first <= last
        //      last < 1024
        //
        // Handles:
        //
        //      partial first word
        //      complete middle words
        //      partial last word
        //
        // ====================================================================

        static void setBits(SubPlane& sub, uint32_t first, uint32_t last) noexcept
        {
            assert(first <= last);
            assert(last < kUnicodeSubSize);


            const uint32_t firstWord =
                first >> 6;


            const uint32_t lastWord =
                last >> 6;


            const uint32_t firstBit =
                first & 63u;


            const uint32_t lastBit =
                last & 63u;


            // ---------------------------------------------------------------
            // Entire range fits within one uint64_t.
            // ---------------------------------------------------------------

            if (firstWord == lastWord)
            {
                const uint64_t lowerMask =
                    ~uint64_t(0) << firstBit;


                const uint64_t upperMask =
                    lastBit == 63u
                    ? ~uint64_t(0)
                    : ((uint64_t(1) << (lastBit + 1u)) - 1u);


                sub.bits[firstWord] |=
                    lowerMask & upperMask;


                return;
            }


            // ---------------------------------------------------------------
            // First partial word.
            // ---------------------------------------------------------------

            sub.bits[firstWord] |=
                ~uint64_t(0) << firstBit;


            // ---------------------------------------------------------------
            // Complete middle words.
            // ---------------------------------------------------------------

            for (uint32_t wi = firstWord + 1u; wi < lastWord; ++wi)
                sub.bits[wi] = ~uint64_t(0);


            // ---------------------------------------------------------------
            // Last partial word.
            // ---------------------------------------------------------------

            sub.bits[lastWord] |=
                lastBit == 63u
                ? ~uint64_t(0)
                : ((uint64_t(1) << (lastBit + 1u)) - 1u);
        }
    };

} // namespace waavs
