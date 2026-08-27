// unicode_decomposition_builder.h

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "unicode_decomposition_data.h"
#include "unicode_decomposition_page_pool_builder.h"


namespace waavs
{
    // ========================================================================
    // UnicodeDecompositionBuilderStats
    // ========================================================================

    struct UnicodeDecompositionBuilderStats
    {
        size_t mappingsAdded{ 0 };
        size_t singletonMappings{ 0 };
        size_t pairMappings{ 0 };
    };


    // ========================================================================
    // UnicodeDecompositionBuilder
    //
    // Mutable generator-side representation of canonical decomposition:
    //
    //      Unicode code point
    //          ->
    //      one or two Unicode code points
    //
    //
    // Construction representation:
    //
    //      dense UnicodeDecompositionRecordRef[kUnicodeLimit]
    //
    // plus:
    //
    //      UnicodeDecompositionRecord[]
    //
    //
    // Record references are 1-based:
    //
    //      0           no decomposition
    //      1           records[0]
    //      2           records[1]
    //      ...
    //
    //
    // The dense construction array is approximately:
    //
    //      1,114,112 * 2 bytes
    //      ~= 2.125 MiB
    //
    // This is generator-only memory.
    //
    //
    // finalize() converts the dense logical table into:
    //
    //      UnicodeDecompositionData
    //              |
    //              v
    //      shared UnicodeDecompositionMasterPage pool
    //              |
    //              v
    //      shared UnicodeDecompositionPage pool
    //
    //
    // Empty leaf pages and empty master regions are collapsed by
    // UnicodeDecompositionPagePoolBuilder.
    //
    // ========================================================================

    class UnicodeDecompositionBuilder
    {
    public:
        UnicodeDecompositionBuilder() = default;


        // ====================================================================
        // clear
        // ====================================================================

        void clear() noexcept
        {
            mMappings.fill(
                kUnicodeDecompositionRecordNone);

            mRecords.clear();

            mStats = {};
        }


        // ====================================================================
        // reserveRecords
        //
        // Optional generator-side optimization.
        //
        // Unicode 17.0.0 contains only 2,081 explicit canonical decomposition
        // mappings, so the persistent 16-bit record-reference space is ample.
        // ====================================================================

        void reserveRecords(size_t count)
        {
            if (count > kUnicodeDecompositionMaxRecords)
                count = kUnicodeDecompositionMaxRecords;

            mRecords.reserve(count);
        }


        // ====================================================================
        // addSingleton
        //
        // Add:
        //
        //      cp -> first
        //
        // Returns false when:
        //
        //      - cp is outside Unicode
        //      - first is outside Unicode
        //      - cp already has a decomposition
        //      - the record-reference space is exhausted
        //
        // ====================================================================

        bool addSingleton(uint32_t cp, uint32_t first)
        {
            return add(
                cp,
                first,
                kUnicodeDecompositionSecondNone);
        }


        // ====================================================================
        // addPair
        //
        // Add:
        //
        //      cp -> first second
        //
        // Returns false under the same conditions as addSingleton().
        // ====================================================================

        bool addPair(uint32_t cp, uint32_t first, uint32_t second)
        {
            if (second == kUnicodeDecompositionSecondNone)
                return false;

            return add(
                cp,
                first,
                second);
        }


        // ====================================================================
        // add
        //
        // Low-level canonical decomposition insertion.
        //
        // second == kUnicodeDecompositionSecondNone means singleton.
        //
        // Duplicate source code points are rejected rather than overwritten.
        // This makes malformed input visible to the parser instead of silently
        // replacing an earlier mapping.
        //
        // ====================================================================

        bool add(uint32_t cp, uint32_t first, uint32_t second)
        {
            if (cp >= kUnicodeLimit)
                return false;

            if (first >= kUnicodeLimit)
                return false;

            if (second != kUnicodeDecompositionSecondNone &&
                second >= kUnicodeLimit)
            {
                return false;
            }


            if (mMappings[cp] !=
                kUnicodeDecompositionRecordNone)
            {
                return false;
            }


            if (mRecords.size() >=
                kUnicodeDecompositionMaxRecords)
            {
                return false;
            }


            const uint32_t recordIndex =
                static_cast<uint32_t>(
                    mRecords.size());


            mRecords.push_back(
                UnicodeDecompositionRecord{
                    first,
                    second
                });


            mMappings[cp] =
                unicodeDecompositionRecordRef(
                    recordIndex);


            ++mStats.mappingsAdded;


            if (second ==
                kUnicodeDecompositionSecondNone)
            {
                ++mStats.singletonMappings;
            }
            else
            {
                ++mStats.pairMappings;
            }


            return true;
        }


        // ====================================================================
        // hasMapping
        // ====================================================================

        [[nodiscard]]
        bool hasMapping(uint32_t cp) const noexcept
        {
            if (cp >= kUnicodeLimit)
                return false;

            return
                mMappings[cp] !=
                kUnicodeDecompositionRecordNone;
        }


        // ====================================================================
        // recordRef
        //
        // Return the decomposition-record reference associated with cp.
        //
        // Returns kUnicodeDecompositionRecordNone for:
        //
        //      - unmapped code points
        //      - out-of-range code points
        //
        // ====================================================================

        [[nodiscard]]
        UnicodeDecompositionRecordRef recordRef(uint32_t cp) const noexcept
        {
            if (cp >= kUnicodeLimit)
                return kUnicodeDecompositionRecordNone;

            return mMappings[cp];
        }


        // ====================================================================
        // record
        //
        // Return the decomposition record associated with cp.
        //
        // Returns nullptr when cp has no decomposition.
        // ====================================================================

        [[nodiscard]]
        const UnicodeDecompositionRecord* record(uint32_t cp) const noexcept
        {
            const UnicodeDecompositionRecordRef ref =
                recordRef(cp);


            if (ref ==
                kUnicodeDecompositionRecordNone)
            {
                return nullptr;
            }


            const uint32_t index =
                unicodeDecompositionRecordIndex(ref);


            if (index >= mRecords.size())
                return nullptr;


            return &mRecords[index];
        }


        // ====================================================================
        // recordByRef
        // ====================================================================

        [[nodiscard]]
        const UnicodeDecompositionRecord* recordByRef(
            UnicodeDecompositionRecordRef ref) const noexcept
        {
            if (ref ==
                kUnicodeDecompositionRecordNone)
            {
                return nullptr;
            }


            const uint32_t index =
                unicodeDecompositionRecordIndex(ref);


            if (index >= mRecords.size())
                return nullptr;


            return &mRecords[index];
        }


        // ====================================================================
        // Dense logical mapping data
        //
        // Deliberately const-only.
        //
        // Unlike VALUE8, arbitrary mutation of these references could create
        // dangling references into mRecords, so mutation should go through
        // addSingleton()/addPair().
        // ====================================================================

        [[nodiscard]]
        const UnicodeDecompositionRecordRef* data() const noexcept
        {
            return mMappings.data();
        }


        [[nodiscard]]
        static constexpr size_t size() noexcept
        {
            return kUnicodeLimit;
        }


        // ====================================================================
        // Record access
        // ====================================================================

        [[nodiscard]]
        const std::vector<UnicodeDecompositionRecord>& records() const noexcept
        {
            return mRecords;
        }


        [[nodiscard]]
        const UnicodeDecompositionRecord* recordData() const noexcept
        {
            return
                mRecords.empty()
                ? nullptr
                : mRecords.data();
        }


        [[nodiscard]]
        size_t recordCount() const noexcept
        {
            return mRecords.size();
        }


        // ====================================================================
        // Statistics
        // ====================================================================

        [[nodiscard]]
        const UnicodeDecompositionBuilderStats& stats() const noexcept
        {
            return mStats;
        }


        // ====================================================================
        // finalize
        //
        // Convert the mutable dense decomposition table into its canonical
        // persistent page hierarchy.
        //
        //
        // For each 1024-code-point logical leaf:
        //
        //      dense record refs
        //              |
        //              v
        //      UnicodeDecompositionPage
        //              |
        //              v
        //      pool.internPage()
        //
        //
        // Groups of 32 page references become:
        //
        //      UnicodeDecompositionMasterPage
        //              |
        //              v
        //      pool.internMasterPage()
        //
        //
        // The resulting 34 master references form:
        //
        //      UnicodeDecompositionData
        //
        //
        // outData is committed only after the complete hierarchy has
        // successfully finalized.
        //
        // The page pool itself may contain pages successfully interned before
        // a later failure, matching the behavior of the other pool builders.
        //
        // ====================================================================

        [[nodiscard]]
        bool finalize(UnicodeDecompositionPagePoolBuilder& pool,
            UnicodeDecompositionData& outData) const
        {
            // ---------------------------------------------------------------
            // Verify all persistent record references before creating pages.
            //
            // This should always succeed when the builder has only been
            // modified through addSingleton()/addPair(), but retaining this
            // validation makes finalize() responsible for guaranteeing the
            // persistent hierarchy it emits.
            // ---------------------------------------------------------------

            if (!validateRecordReferences())
                return false;


            UnicodeDecompositionData result{};


            for (uint32_t mi = 0; mi < kUnicodeMasterCount; ++mi)
            {
                UnicodeDecompositionMasterPage masterPage{};


                const uint32_t masterStart =
                    mi << kUnicodeMasterShift;


                // -----------------------------------------------------------
                // Finalize the 32 logical decomposition pages belonging to
                // this master region.
                // -----------------------------------------------------------

                for (uint32_t si = 0; si < kUnicodeSubsPerMaster; ++si)
                {
                    const uint32_t subStart =
                        masterStart +
                        (si << kUnicodeSubShift);


                    UnicodeDecompositionPage page{};


                    std::memcpy(
                        page.mapping,
                        mMappings.data() + subStart,
                        sizeof(page.mapping));


                    UnicodeDecompositionPageRef pageRef;


                    if (!pool.internPage(
                        page,
                        pageRef))
                    {
                        return false;
                    }


                    masterPage.sub[si] =
                        pageRef;
                }


                // -----------------------------------------------------------
                // Intern the complete master page.
                //
                // If all 32 child pages are empty, the pool collapses the
                // complete master region to kUnicodeDecompositionPageEmpty.
                // -----------------------------------------------------------

                UnicodeDecompositionMasterPageRef masterRef;


                if (!pool.internMasterPage(
                    masterPage,
                    masterRef))
                {
                    return false;
                }


                result.masters[mi] =
                    masterRef;
            }


            // ---------------------------------------------------------------
            // Commit only after complete success.
            // ---------------------------------------------------------------

            outData = result;

            return true;
        }


    private:
        // ====================================================================
        // Dense generator-side logical mapping
        //
        // Approximately 2.125 MiB.
        //
        // Zero initialization naturally means:
        //
        //      no canonical decomposition
        //
        // ====================================================================

        std::array<
            UnicodeDecompositionRecordRef,
            kUnicodeLimit> mMappings{};


        // ====================================================================
        // Decomposition records
        //
        // These are intentionally not deduplicated at this stage.
        //
        // Unicode 17.0.0 contains 2,081 canonical decomposition mappings and
        // only about 100 duplicate direct sequences. Deduplicating them would
        // save only about 800 bytes while introducing another record-pool
        // abstraction.
        // ====================================================================

        std::vector<UnicodeDecompositionRecord> mRecords;

        UnicodeDecompositionBuilderStats mStats{};


        // ====================================================================
        // validateRecordReferences
        // ====================================================================

        [[nodiscard]]
        bool validateRecordReferences() const noexcept
        {
            for (UnicodeDecompositionRecordRef ref : mMappings)
            {
                if (ref ==
                    kUnicodeDecompositionRecordNone)
                {
                    continue;
                }


                if (unicodeDecompositionRecordIndex(ref) >=
                    mRecords.size())
                {
                    return false;
                }
            }


            return true;
        }
    };

} // namespace waavs
