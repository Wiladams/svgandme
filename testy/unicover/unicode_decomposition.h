// unicode_decomposition.h

#pragma once

#include <cassert>
#include <cstdint>

#include "unicode_decomposition_data.h"


namespace waavs
{
    // ========================================================================
    // UnicodeDecompositionPools
    //
    // Non-owning view of the shared storage used by UnicodeDecompositionData.
    //
    // The three persistent pools are:
    //
    //      UnicodeDecompositionMasterPage[]
    //      UnicodeDecompositionPage[]
    //      UnicodeDecompositionRecord[]
    //
    // Master-page references and page references occupy independent 16-bit
    // address spaces.
    //
    // Decomposition-record references are 1-based:
    //
    //      0           no decomposition
    //      1           records[0]
    //      2           records[1]
    //      ...
    //
    // The backing memory must remain valid for the lifetime of every
    // UnicodeDecomposition view which references these pools.
    //
    // ========================================================================

    struct UnicodeDecompositionPools
    {
        const UnicodeDecompositionMasterPage* masterPages{ nullptr };
        const UnicodeDecompositionPage* pages{ nullptr };
        const UnicodeDecompositionRecord* records{ nullptr };

        uint32_t masterPageCount{ 0 };
        uint32_t pageCount{ 0 };
        uint32_t recordCount{ 0 };


        [[nodiscard]]
        constexpr bool valid() const noexcept
        {
            return
                (masterPageCount == 0 || masterPages != nullptr) &&
                (pageCount == 0 || pages != nullptr) &&
                (recordCount == 0 || records != nullptr);
        }


        [[nodiscard]]
        const UnicodeDecompositionMasterPage& masterPage(
            UnicodeDecompositionMasterPageRef ref) const noexcept
        {
            assert(ref != kUnicodeDecompositionPageEmpty);
            assert(ref < masterPageCount);

            return masterPages[ref];
        }


        [[nodiscard]]
        const UnicodeDecompositionPage& page(
            UnicodeDecompositionPageRef ref) const noexcept
        {
            assert(ref != kUnicodeDecompositionPageEmpty);
            assert(ref < pageCount);

            return pages[ref];
        }


        [[nodiscard]]
        const UnicodeDecompositionRecord& record(
            UnicodeDecompositionRecordRef ref) const noexcept
        {
            assert(ref != kUnicodeDecompositionRecordNone);

            const uint32_t index =
                unicodeDecompositionRecordIndex(ref);

            assert(index < recordCount);

            return records[index];
        }
    };


    // ========================================================================
    // UnicodeDecomposition
    //
    // Immutable, non-owning runtime view of direct canonical decomposition:
    //
    //      Unicode code point
    //          ->
    //      zero, one, or two Unicode code points
    //
    //
    // This view returns the DIRECT decomposition stored in UnicodeData.txt.
    //
    // It does not recursively decompose mapping results.
    //
    // For example:
    //
    //      U+01D5
    //          ->
    //      U+00DC U+0304
    //
    // and:
    //
    //      U+00DC
    //          ->
    //      U+0055 U+0308
    //
    // Recursive canonical decomposition will be implemented by the
    // normalization layer above this primitive.
    //
    //
    // Storage hierarchy:
    //
    //      UnicodeDecompositionData
    //              |
    //              v
    //      UnicodeDecompositionMasterPage
    //              |
    //              v
    //      UnicodeDecompositionPage
    //              |
    //              v
    //      UnicodeDecompositionRecord
    //
    //
    // Empty master regions and leaf pages require no physical page lookup.
    //
    // ========================================================================

    class UnicodeDecomposition
    {
    public:
        constexpr UnicodeDecomposition() noexcept = default;


        constexpr UnicodeDecomposition(
            const UnicodeDecompositionData* data,
            const UnicodeDecompositionPools* pools) noexcept
            : mData(data)
            , mPools(pools)
        {
        }


        // ====================================================================
        // Basic state
        // ====================================================================

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept
        {
            return
                mData != nullptr &&
                mPools != nullptr;
        }


        [[nodiscard]]
        constexpr bool valid() const noexcept
        {
            return
                mData != nullptr &&
                mPools != nullptr &&
                mPools->valid();
        }


        [[nodiscard]]
        constexpr const UnicodeDecompositionData* data() const noexcept
        {
            return mData;
        }


        [[nodiscard]]
        constexpr const UnicodeDecompositionPools* pools() const noexcept
        {
            return mPools;
        }


        // ====================================================================
        // recordRef
        //
        // Return the persistent decomposition-record reference associated with
        // one Unicode code point.
        //
        // Returns kUnicodeDecompositionRecordNone when:
        //
        //      - this view is unattached
        //      - cp is outside Unicode
        //      - the master region is empty
        //      - the leaf page is empty
        //      - cp has no canonical decomposition
        //
        // ====================================================================

        [[nodiscard]]
        UnicodeDecompositionRecordRef recordRef(uint32_t cp) const noexcept
        {
            if (!mData ||
                !mPools ||
                cp >= kUnicodeLimit)
            {
                return kUnicodeDecompositionRecordNone;
            }


            const uint32_t masterIndex =
                cp >> kUnicodeMasterShift;


            const UnicodeDecompositionMasterPageRef masterRef =
                mData->masters[masterIndex];


            // ---------------------------------------------------------------
            // Entire 32768-code-point master region contains no mappings.
            // ---------------------------------------------------------------

            if (masterRef ==
                kUnicodeDecompositionPageEmpty)
            {
                return kUnicodeDecompositionRecordNone;
            }


            assert(masterRef <
                mPools->masterPageCount);


            const UnicodeDecompositionMasterPage& master =
                mPools->masterPage(masterRef);


            const uint32_t subIndex =
                (cp >> kUnicodeSubShift) &
                (kUnicodeSubsPerMaster - 1u);


            const UnicodeDecompositionPageRef pageRef =
                master.sub[subIndex];


            // ---------------------------------------------------------------
            // Entire 1024-code-point leaf contains no mappings.
            // ---------------------------------------------------------------

            if (pageRef ==
                kUnicodeDecompositionPageEmpty)
            {
                return kUnicodeDecompositionRecordNone;
            }


            assert(pageRef <
                mPools->pageCount);


            const UnicodeDecompositionPage& page =
                mPools->page(pageRef);


            const uint32_t mappingIndex =
                cp &
                (kUnicodeSubSize - 1u);


            const UnicodeDecompositionRecordRef ref =
                page.mapping[mappingIndex];


            if (ref ==
                kUnicodeDecompositionRecordNone)
            {
                return kUnicodeDecompositionRecordNone;
            }


            assert(
                unicodeDecompositionRecordIndex(ref) <
                mPools->recordCount);


            return ref;
        }


        // ====================================================================
        // record
        //
        // Return the direct canonical-decomposition record associated with cp.
        //
        // Returns nullptr when cp has no direct canonical decomposition.
        //
        // The returned pointer refers directly into the persistent record pool.
        //
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


            return
                &mPools->record(ref);
        }


        // ====================================================================
        // hasMapping
        // ====================================================================

        [[nodiscard]]
        bool hasMapping(uint32_t cp) const noexcept
        {
            return
                recordRef(cp) !=
                kUnicodeDecompositionRecordNone;
        }


        // ====================================================================
        // length
        //
        // Return:
        //
        //      0   no direct decomposition
        //      1   singleton decomposition
        //      2   pair decomposition
        //
        // ====================================================================

        [[nodiscard]]
        uint32_t length(uint32_t cp) const noexcept
        {
            const UnicodeDecompositionRecord* mapping =
                record(cp);


            if (!mapping)
                return 0;


            return
                unicodeDecompositionLength(
                    *mapping);
        }


    private:
        const UnicodeDecompositionData* mData{ nullptr };
        const UnicodeDecompositionPools* mPools{ nullptr };
    };


    // ========================================================================
    // Convenience relationships
    //
    // Equality means identity of the backing persistent root and pools.
    // It does not perform semantic comparison of all Unicode mappings.
    // ========================================================================

    [[nodiscard]]
    inline bool operator==(const UnicodeDecomposition& a,
        const UnicodeDecomposition& b) noexcept
    {
        return
            a.data() == b.data() &&
            a.pools() == b.pools();
    }


    [[nodiscard]]
    inline bool operator!=(const UnicodeDecomposition& a,
        const UnicodeDecomposition& b) noexcept
    {
        return !(a == b);
    }

} // namespace waavs

