#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "unicode_coverage_data.h"


namespace waavs
{
    // ========================================================================
    // UnicodeCoveragePools
    //
    // Non-owning view of the shared page pools used by UnicodeCoverageData.
    //
    // Master-page and bit-page references occupy independent 16-bit address
    // spaces.
    //
    // The memory backing these pools must remain valid for the lifetime of
    // every UnicodeCoverage that references them.
    //
    // ========================================================================

    struct UnicodeCoveragePools
    {
        const UnicodeMasterPage* masterPages{ nullptr };
        const UnicodeBitPage* bitPages{ nullptr };

        uint32_t masterPageCount{ 0 };
        uint32_t bitPageCount{ 0 };


        [[nodiscard]]
        constexpr bool valid() const noexcept
        {
            return
                (masterPageCount == 0 || masterPages != nullptr) &&
                (bitPageCount == 0 || bitPages != nullptr);
        }


        [[nodiscard]]
        const UnicodeMasterPage&
            masterPage(UnicodeMasterPageRef ref) const noexcept
        {
            assert(ref < masterPageCount);
            return masterPages[ref];
        }


        [[nodiscard]]
        const UnicodeBitPage&
            bitPage(UnicodeBitPageRef ref) const noexcept
        {
            assert(ref < bitPageCount);
            return bitPages[ref];
        }
    };


    // ========================================================================
    // UnicodeCoverageStats
    //
    // Runtime statistics describing the logical representation of a coverage.
    //
    // Because pages are shared globally, these values describe pages REFERENCED
    // by this coverage, not memory owned exclusively by it.
    //
    // ========================================================================

    struct UnicodeCoverageStats
    {
        size_t masterPlanesUsed{ 0 };
        size_t fullMasterPlanes{ 0 };

        size_t masterPagesReferenced{ 0 };
        size_t fullSubPlanes{ 0 };
        size_t bitPagesReferenced{ 0 };

        size_t coveredCodePoints{ 0 };
    };


    // ========================================================================
    // UnicodeCoverage
    //
    // Immutable, non-owning runtime view of Unicode coverage.
    //
    // A coverage consists of:
    //
    //      UnicodeCoverageData
    //
    //          masters[]
    //              |
    //              v
    //      shared UnicodeMasterPage pool
    //
    //          sub[]
    //              |
    //              v
    //      shared UnicodeBitPage pool
    //
    //
    // UnicodeCoverage contains no allocated storage and performs no lifetime
    // management.  It can refer to:
    //
    //      - a memory-mapped Unicode database
    //      - dynamically generated font coverage
    //      - any other storage using UnicodeCoverageData + shared pools
    //
    // ========================================================================

    class UnicodeCoverage
    {
    public:
        constexpr UnicodeCoverage() noexcept = default;


        constexpr UnicodeCoverage(
            const UnicodeCoverageData* data,
            const UnicodeCoveragePools* pools) noexcept
            : mData(data)
            , mPools(pools)
        {
        }


        // --------------------------------------------------------------------
        // Basic state
        // --------------------------------------------------------------------

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept
        {
            return mData != nullptr &&
                mPools != nullptr;
        }


        [[nodiscard]]
        constexpr bool valid() const noexcept
        {
            return mData != nullptr &&
                mPools != nullptr &&
                mPools->valid();
        }


        [[nodiscard]]
        constexpr const UnicodeCoverageData*
            data() const noexcept
        {
            return mData;
        }


        [[nodiscard]]
        constexpr const UnicodeCoveragePools*
            pools() const noexcept
        {
            return mPools;
        }


        // --------------------------------------------------------------------
        // empty
        //
        // Return true if the coverage contains no Unicode code points.
        // --------------------------------------------------------------------

        [[nodiscard]]
        bool empty() const noexcept
        {
            if (!mData)
                return true;

            for (uint32_t mi = 0;
                mi < kUnicodeMasterCount;
                ++mi)
            {
                if (mData->masters[mi] != kUnicodePageEmpty)
                    return false;
            }

            return true;
        }


        // --------------------------------------------------------------------
        // contains
        //
        // Test whether a single Unicode code point is present.
        // --------------------------------------------------------------------

        [[nodiscard]]
        bool contains(uint32_t cp) const noexcept
        {
            if (!mData ||
                !mPools ||
                cp >= kUnicodeLimit)
            {
                return false;
            }


            const uint32_t masterIndex =
                cp >> kUnicodeMasterShift;

            const UnicodeMasterPageRef masterRef =
                mData->masters[masterIndex];


            if (masterRef == kUnicodePageEmpty)
                return false;

            if (masterRef == kUnicodePageFull)
                return true;


            assert(masterRef < mPools->masterPageCount);

            const UnicodeMasterPage& master =
                mPools->masterPage(masterRef);


            const uint32_t subIndex =
                (cp >> kUnicodeSubShift) &
                (kUnicodeSubsPerMaster - 1u);

            const UnicodeBitPageRef bitRef =
                master.sub[subIndex];


            if (bitRef == kUnicodePageEmpty)
                return false;

            if (bitRef == kUnicodePageFull)
                return true;


            assert(bitRef < mPools->bitPageCount);

            const UnicodeBitPage& bits =
                mPools->bitPage(bitRef);


            const uint32_t bitIndex =
                cp & (kUnicodeSubSize - 1u);

            return
                (bits.bits[bitIndex >> 6] &
                    (uint64_t(1) <<
                        (bitIndex & 63u))) != 0;
        }


        // --------------------------------------------------------------------
        // containsAll
        //
        // Return true if every code point in 'other' is also contained in
        // this coverage.
        //
        // The two coverages may reference entirely different page pools.
        // --------------------------------------------------------------------

        [[nodiscard]]
        bool containsAll(
            const UnicodeCoverage& other) const noexcept
        {
            if (!other.mData)
                return true;

            if (!mData ||
                !mPools ||
                !other.mPools)
            {
                return false;
            }


            for (uint32_t mi = 0;
                mi < kUnicodeMasterCount;
                ++mi)
            {
                const UnicodeMasterPageRef mineRef =
                    mData->masters[mi];

                const UnicodeMasterPageRef otherRef =
                    other.mData->masters[mi];


                // Other contains nothing here.
                if (otherRef == kUnicodePageEmpty)
                    continue;


                // Mine contains the entire master plane.
                if (mineRef == kUnicodePageFull)
                    continue;


                // Other contains something, mine contains nothing.
                if (mineRef == kUnicodePageEmpty)
                    return false;


                // Other requires the complete master plane but mine does not
                // provide it completely.
                if (otherRef == kUnicodePageFull)
                    return false;


                const UnicodeMasterPage& mine =
                    mPools->masterPage(mineRef);

                const UnicodeMasterPage& theirs =
                    other.mPools->masterPage(otherRef);


                // ------------------------------------------------------------
                // Fast rejection.
                //
                // If theirs contains anything in a sub-plane where mine
                // contains nothing, containment is impossible.
                // ------------------------------------------------------------

                if ((theirs.nonEmptyMask &
                    ~mine.nonEmptyMask) != 0)
                {
                    return false;
                }


                // ------------------------------------------------------------
                // If theirs contains a completely full sub-plane, mine must
                // also contain that sub-plane completely.
                // ------------------------------------------------------------

                if ((theirs.fullMask &
                    ~mine.fullMask) != 0)
                {
                    return false;
                }


                // ------------------------------------------------------------
                // Examine only partially populated sub-planes in 'theirs'.
                // ------------------------------------------------------------

                uint32_t active =
                    theirs.nonEmptyMask &
                    ~theirs.fullMask;


                while (active != 0)
                {
                    const uint32_t si =
                        firstSetBit(active);

                    active &= active - 1u;


                    const UnicodeBitPageRef theirBitRef =
                        theirs.sub[si];

                    const UnicodeBitPageRef myBitRef =
                        mine.sub[si];


                    assert(
                        theirBitRef != kUnicodePageEmpty &&
                        theirBitRef != kUnicodePageFull);


                    // Mine completely contains this sub-plane.
                    if (myBitRef == kUnicodePageFull)
                        continue;


                    if (myBitRef == kUnicodePageEmpty)
                        return false;


                    const UnicodeBitPage& a =
                        mPools->bitPage(myBitRef);

                    const UnicodeBitPage& b =
                        other.mPools->bitPage(theirBitRef);


                    for (uint32_t wi = 0;
                        wi < 16;
                        ++wi)
                    {
                        if ((b.bits[wi] &
                            ~a.bits[wi]) != 0)
                        {
                            return false;
                        }
                    }
                }
            }


            return true;
        }


        // --------------------------------------------------------------------
        // intersects
        //
        // Return true if this coverage and 'other' contain at least one
        // code point in common.
        //
        // The two coverages may reference entirely different page pools.
        // --------------------------------------------------------------------

        [[nodiscard]]
        bool intersects(
            const UnicodeCoverage& other) const noexcept
        {
            if (!mData ||
                !mPools ||
                !other.mData ||
                !other.mPools)
            {
                return false;
            }


            for (uint32_t mi = 0;
                mi < kUnicodeMasterCount;
                ++mi)
            {
                const UnicodeMasterPageRef mineRef =
                    mData->masters[mi];

                const UnicodeMasterPageRef otherRef =
                    other.mData->masters[mi];


                if (mineRef == kUnicodePageEmpty ||
                    otherRef == kUnicodePageEmpty)
                {
                    continue;
                }


                // Both are known to contain something, and one covers the
                // complete master plane.
                if (mineRef == kUnicodePageFull ||
                    otherRef == kUnicodePageFull)
                {
                    return true;
                }


                const UnicodeMasterPage& mine =
                    mPools->masterPage(mineRef);

                const UnicodeMasterPage& theirs =
                    other.mPools->masterPage(otherRef);


                uint32_t active =
                    mine.nonEmptyMask &
                    theirs.nonEmptyMask;


                if (active == 0)
                    continue;


                // ------------------------------------------------------------
                // Any sub-plane that is full on one side and non-empty on
                // the other intersects immediately.
                // ------------------------------------------------------------

                if (((mine.fullMask &
                    theirs.nonEmptyMask) |
                    (theirs.fullMask &
                        mine.nonEmptyMask)) != 0)
                {
                    return true;
                }


                // Only partially populated sub-planes remain.
                active &=
                    ~(mine.fullMask |
                        theirs.fullMask);


                while (active != 0)
                {
                    const uint32_t si =
                        firstSetBit(active);

                    active &= active - 1u;


                    const UnicodeBitPageRef myBitRef =
                        mine.sub[si];

                    const UnicodeBitPageRef theirBitRef =
                        theirs.sub[si];


                    const UnicodeBitPage& a =
                        mPools->bitPage(myBitRef);

                    const UnicodeBitPage& b =
                        other.mPools->bitPage(theirBitRef);


                    for (uint32_t wi = 0;
                        wi < 16;
                        ++wi)
                    {
                        if ((a.bits[wi] &
                            b.bits[wi]) != 0)
                        {
                            return true;
                        }
                    }
                }
            }


            return false;
        }


        // --------------------------------------------------------------------
        // stats
        //
        // Calculate logical coverage statistics.
        //
        // Note that referenced pages may be shared by other coverage objects,
        // so these are not exclusive memory-ownership statistics.
        // --------------------------------------------------------------------

        [[nodiscard]]
        UnicodeCoverageStats stats() const noexcept
        {
            UnicodeCoverageStats result;


            if (!mData || !mPools)
                return result;


            for (uint32_t mi = 0;
                mi < kUnicodeMasterCount;
                ++mi)
            {
                const UnicodeMasterPageRef masterRef =
                    mData->masters[mi];


                if (masterRef == kUnicodePageEmpty)
                    continue;


                ++result.masterPlanesUsed;


                const uint32_t masterStart =
                    mi << kUnicodeMasterShift;

                const uint32_t masterRemaining =
                    kUnicodeLimit - masterStart;

                const uint32_t validMasterSize =
                    masterRemaining < kUnicodeMasterSize
                    ? masterRemaining
                    : kUnicodeMasterSize;


                if (masterRef == kUnicodePageFull)
                {
                    ++result.fullMasterPlanes;

                    result.coveredCodePoints +=
                        validMasterSize;

                    result.fullSubPlanes +=
                        validMasterSize /
                        kUnicodeSubSize;

                    continue;
                }


                ++result.masterPagesReferenced;


                const UnicodeMasterPage& master =
                    mPools->masterPage(masterRef);


                for (uint32_t si = 0;
                    si < kUnicodeSubsPerMaster;
                    ++si)
                {
                    const UnicodeBitPageRef bitRef =
                        master.sub[si];


                    if (bitRef == kUnicodePageEmpty)
                        continue;


                    const uint32_t subStart =
                        masterStart +
                        (si << kUnicodeSubShift);


                    if (subStart >= kUnicodeLimit)
                        break;


                    const uint32_t remaining =
                        kUnicodeLimit - subStart;

                    const uint32_t validSubSize =
                        remaining < kUnicodeSubSize
                        ? remaining
                        : kUnicodeSubSize;


                    if (bitRef == kUnicodePageFull)
                    {
                        ++result.fullSubPlanes;

                        result.coveredCodePoints +=
                            validSubSize;

                        continue;
                    }


                    ++result.bitPagesReferenced;


                    const UnicodeBitPage& page =
                        mPools->bitPage(bitRef);


                    for (uint32_t wi = 0;
                        wi < 16;
                        ++wi)
                    {
                        result.coveredCodePoints +=
                            popCount64(page.bits[wi]);
                    }
                }
            }


            return result;
        }


    private:
        const UnicodeCoverageData* mData{ nullptr };
        const UnicodeCoveragePools* mPools{ nullptr };


        // --------------------------------------------------------------------
        // firstSetBit
        // --------------------------------------------------------------------

        [[nodiscard]]
        static uint32_t firstSetBit(
            uint32_t value) noexcept
        {
            assert(value != 0);

#if defined(_MSC_VER)

            unsigned long index;

            _BitScanForward(
                &index,
                value);

            return
                static_cast<uint32_t>(index);

#elif defined(__GNUC__) || defined(__clang__)

            return
                static_cast<uint32_t>(
                    __builtin_ctz(value));

#else

            uint32_t index = 0;

            while ((value & 1u) == 0)
            {
                value >>= 1;
                ++index;
            }

            return index;

#endif
        }


        // --------------------------------------------------------------------
        // popCount64
        // --------------------------------------------------------------------

        [[nodiscard]]
        static uint32_t popCount64(
            uint64_t value) noexcept
        {
#if defined(_MSC_VER) && defined(_M_X64)

            return
                static_cast<uint32_t>(
                    __popcnt64(value));

#elif defined(__GNUC__) || defined(__clang__)

            return
                static_cast<uint32_t>(
                    __builtin_popcountll(value));

#else

            uint32_t count = 0;

            while (value != 0)
            {
                value &= value - 1;
                ++count;
            }

            return count;

#endif
        }
    };


    // ========================================================================
    // Convenience relationships
    // ========================================================================

    [[nodiscard]]
    inline bool operator==(
        const UnicodeCoverage& a,
        const UnicodeCoverage& b) noexcept
    {
        // This deliberately means identity of the backing coverage data,
        // not semantic equality of the represented sets.
        //
        // Semantic equality can be provided separately if needed.
        return
            a.data() == b.data() &&
            a.pools() == b.pools();
    }


    [[nodiscard]]
    inline bool operator!=(
        const UnicodeCoverage& a,
        const UnicodeCoverage& b) noexcept
    {
        return !(a == b);
    }

} // namespace waavs

