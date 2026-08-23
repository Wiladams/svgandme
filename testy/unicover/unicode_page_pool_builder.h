#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "core_hash.h"
#include "core_openhashmap.h"

#include "unicode_coverage_data.h"


namespace waavs
{
    // ========================================================================
    // UnicodePagePoolStats
    //
    // Construction statistics for the two shared deduplicated page pools.
    //
    // Requests include pages which collapse to the Empty / Full sentinels.
    //
    // Reused counts refer to actual stored pages that were found to already
    // exist in the pool.
    //
    // Hash collisions count cases where a hash-table key was occupied but
    // the referenced 128-byte page was not identical.
    //
    // ========================================================================

    struct UnicodePagePoolStats
    {
        size_t bitPageRequests{ 0 };
        size_t bitPageEmpty{ 0 };
        size_t bitPageFull{ 0 };
        size_t bitPageReused{ 0 };
        size_t bitPageHashCollisions{ 0 };
        size_t uniqueBitPages{ 0 };

        size_t masterPageRequests{ 0 };
        size_t masterPageEmpty{ 0 };
        size_t masterPageFull{ 0 };
        size_t masterPageReused{ 0 };
        size_t masterPageHashCollisions{ 0 };
        size_t uniqueMasterPages{ 0 };

        [[nodiscard]]
        size_t storedPageCount() const noexcept
        {
            return uniqueBitPages + uniqueMasterPages;
        }

        [[nodiscard]]
        size_t storedBytes() const noexcept
        {
            return
                uniqueBitPages * sizeof(UnicodeBitPage) +
                uniqueMasterPages * sizeof(UnicodeMasterPage);
        }
    };


    // ========================================================================
    // UnicodePagePoolBuilder
    //
    // Maintains the two shared page pools:
    //
    //      UnicodeMasterPage[]
    //      UnicodeBitPage[]
    //
    // Both pools are deduplicated independently.
    //
    //
    // Bit pages
    // ---------
    //
    // Empty:
    //
    //      kUnicodePageEmpty
    //
    // Full:
    //
    //      kUnicodePageFull
    //
    // Partial:
    //
    //      index into shared UnicodeBitPage pool
    //
    //
    // Master pages
    // ------------
    //
    // Empty:
    //
    //      kUnicodePageEmpty
    //
    // Full:
    //
    //      kUnicodePageFull
    //
    // Partial:
    //
    //      index into shared UnicodeMasterPage pool
    //
    //
    // Master pages are canonicalized before hashing:
    //
    //      nonEmptyMask is derived from sub[]
    //      fullMask is derived from sub[]
    //      reserved[] is zeroed
    //
    // This guarantees deterministic byte representation and therefore
    // deterministic deduplication.
    //
    // ========================================================================

    class UnicodePagePoolBuilder
    {
    private:
        using BitPageHashMap =
            WSOpenHashMap<uint64_t, UnicodeBitPageRef, WSHash64>;

        using MasterPageHashMap =
            WSOpenHashMap<uint64_t, UnicodeMasterPageRef, WSHash64>;

    public:
        UnicodePagePoolBuilder() = default;


        // ====================================================================
        // clear
        // ====================================================================

        void clear() noexcept
        {
            mBitPages.clear();
            mMasterPages.clear();

            mBitPageHashes.clear();
            mMasterPageHashes.clear();

            mStats = {};
        }


        // ====================================================================
        // reserve
        //
        // Optional generator-side optimization.
        //
        // The page pools themselves cannot exceed kUnicodeMaxPoolPages
        // because page references are 16-bit.
        // ====================================================================

        bool reserveBitPages(size_t count)
        {
            if (count > kUnicodeMaxPoolPages)
                count = kUnicodeMaxPoolPages;

            if (!mBitPageHashes.reserve(count))
                return false;

            mBitPages.reserve(count);

            return true;
        }


        bool reserveMasterPages(size_t count)
        {
            if (count > kUnicodeMaxPoolPages)
                count = kUnicodeMaxPoolPages;

            if (!mMasterPageHashes.reserve(count))
                return false;

            mMasterPages.reserve(count);

            return true;
        }


        // ====================================================================
        // internBitPage
        //
        // Intern one bit page into the shared pool.
        //
        // Empty and full pages are represented by sentinel references and are
        // never stored in the pool.
        //
        // Returns false only when:
        //
        //      - the 16-bit bit-page pool is exhausted
        //      - the hash map cannot allocate/grow
        //
        // ====================================================================

        bool internBitPage(const UnicodeBitPage& page, UnicodeBitPageRef& outRef)
        {
            ++mStats.bitPageRequests;


            // ---------------------------------------------------------------
            // Empty page
            // ---------------------------------------------------------------

            if (bitPageIsEmpty(page))
            {
                ++mStats.bitPageEmpty;

                outRef = kUnicodePageEmpty;
                return true;
            }


            // ---------------------------------------------------------------
            // Full page
            // ---------------------------------------------------------------

            if (bitPageIsFull(page))
            {
                ++mStats.bitPageFull;

                outRef = kUnicodePageFull;
                return true;
            }


            // ---------------------------------------------------------------
            // Deduplicated page
            //
            // Usually there is exactly one hash-table probe.
            //
            // If two different 128-byte pages happen to produce the same
            // 64-bit page hash, move to another hash key.
            //
            // WSHash64 then independently hashes this key for placement in
            // the open-addressed table.
            // ---------------------------------------------------------------

            const uint64_t baseHash = hashPage(&page);

            uint64_t hashKey = baseHash;


            for (;;)
            {
                const UnicodeBitPageRef* existing =
                    mBitPageHashes.getRef(hashKey);


                if (existing)
                {
                    if (*existing < mBitPages.size() &&
                        bitPagesEqual(mBitPages[*existing], page))
                    {
                        ++mStats.bitPageReused;

                        outRef = *existing;
                        return true;
                    }


                    // Extremely rare true hash-key collision.
                    ++mStats.bitPageHashCollisions;

                    ++hashKey;
                    continue;
                }


                // -----------------------------------------------------------
                // New page
                // -----------------------------------------------------------

                if (mBitPages.size() >= kUnicodeMaxPoolPages)
                    return false;


                const UnicodeBitPageRef newRef =
                    static_cast<UnicodeBitPageRef>(mBitPages.size());


                mBitPages.push_back(page);


                if (!mBitPageHashes.put(hashKey, newRef))
                {
                    mBitPages.pop_back();
                    return false;
                }


                ++mStats.uniqueBitPages;

                outRef = newRef;
                return true;
            }
        }


        // ====================================================================
        // internMasterPage
        //
        // Canonicalize and intern a master page.
        //
        // All actual BitPage references must already refer to entries in this
        // builder's bit-page pool.
        //
        // Returns false when:
        //
        //      - an actual BitPage reference is invalid
        //      - the 16-bit master-page pool is exhausted
        //      - the hash map cannot allocate/grow
        //
        // ====================================================================

        bool internMasterPage(const UnicodeMasterPage& page, UnicodeMasterPageRef& outRef)
        {
            ++mStats.masterPageRequests;


            UnicodeMasterPage canonical{};


            if (!canonicalizeMasterPage(page, canonical))
                return false;


            // ---------------------------------------------------------------
            // Empty master
            // ---------------------------------------------------------------

            if (canonical.nonEmptyMask == 0)
            {
                ++mStats.masterPageEmpty;

                outRef = kUnicodePageEmpty;
                return true;
            }


            // ---------------------------------------------------------------
            // Full master
            //
            // There are exactly 32 sub-planes, so all mask bits being set
            // means this master covers all 32768 code points.
            // ---------------------------------------------------------------

            if (canonical.fullMask == 0xFFFFFFFFu)
            {
                ++mStats.masterPageFull;

                outRef = kUnicodePageFull;
                return true;
            }


            // ---------------------------------------------------------------
            // Deduplicate canonical master page
            // ---------------------------------------------------------------

            const uint64_t baseHash = hashPage(&canonical);

            uint64_t hashKey = baseHash;


            for (;;)
            {
                const UnicodeMasterPageRef* existing =
                    mMasterPageHashes.getRef(hashKey);


                if (existing)
                {
                    if (*existing < mMasterPages.size() &&
                        masterPagesEqual(mMasterPages[*existing], canonical))
                    {
                        ++mStats.masterPageReused;

                        outRef = *existing;
                        return true;
                    }


                    ++mStats.masterPageHashCollisions;

                    ++hashKey;
                    continue;
                }


                // -----------------------------------------------------------
                // New page
                // -----------------------------------------------------------

                if (mMasterPages.size() >= kUnicodeMaxPoolPages)
                    return false;


                const UnicodeMasterPageRef newRef =
                    static_cast<UnicodeMasterPageRef>(mMasterPages.size());


                mMasterPages.push_back(canonical);


                if (!mMasterPageHashes.put(hashKey, newRef))
                {
                    mMasterPages.pop_back();
                    return false;
                }


                ++mStats.uniqueMasterPages;

                outRef = newRef;
                return true;
            }
        }


        // ====================================================================
        // Pool access
        // ====================================================================

        [[nodiscard]]
        const std::vector<UnicodeBitPage>& bitPages() const noexcept
        {
            return mBitPages;
        }


        [[nodiscard]]
        const std::vector<UnicodeMasterPage>& masterPages() const noexcept
        {
            return mMasterPages;
        }


        [[nodiscard]]
        const UnicodeBitPage* bitPageData() const noexcept
        {
            return mBitPages.empty() ? nullptr : mBitPages.data();
        }


        [[nodiscard]]
        const UnicodeMasterPage* masterPageData() const noexcept
        {
            return mMasterPages.empty() ? nullptr : mMasterPages.data();
        }


        [[nodiscard]]
        size_t bitPageCount() const noexcept
        {
            return mBitPages.size();
        }


        [[nodiscard]]
        size_t masterPageCount() const noexcept
        {
            return mMasterPages.size();
        }


        // ====================================================================
        // Individual page lookup
        // ====================================================================

        [[nodiscard]]
        const UnicodeBitPage* bitPage(UnicodeBitPageRef ref) const noexcept
        {
            if (ref >= mBitPages.size())
                return nullptr;

            return &mBitPages[ref];
        }


        [[nodiscard]]
        const UnicodeMasterPage* masterPage(UnicodeMasterPageRef ref) const noexcept
        {
            if (ref >= mMasterPages.size())
                return nullptr;

            return &mMasterPages[ref];
        }


        // ====================================================================
        // Statistics
        // ====================================================================

        [[nodiscard]]
        const UnicodePagePoolStats& stats() const noexcept
        {
            return mStats;
        }


    private:
        std::vector<UnicodeBitPage> mBitPages;
        std::vector<UnicodeMasterPage> mMasterPages;

        BitPageHashMap mBitPageHashes;
        MasterPageHashMap mMasterPageHashes;

        UnicodePagePoolStats mStats{};


        // ====================================================================
        // Bit-page classification
        // ====================================================================

        [[nodiscard]]
        static bool bitPageIsEmpty(const UnicodeBitPage& page) noexcept
        {
            for (uint32_t i = 0; i < 16; ++i)
            {
                if (page.bits[i] != 0)
                    return false;
            }

            return true;
        }


        [[nodiscard]]
        static bool bitPageIsFull(const UnicodeBitPage& page) noexcept
        {
            for (uint32_t i = 0; i < 16; ++i)
            {
                if (page.bits[i] != ~uint64_t(0))
                    return false;
            }

            return true;
        }


        // ====================================================================
        // Master-page canonicalization
        //
        // sub[] is authoritative.
        //
        // nonEmptyMask and fullMask are acceleration metadata derived from
        // those references.
        //
        // ====================================================================

        [[nodiscard]]
        bool canonicalizeMasterPage(const UnicodeMasterPage& source,
            UnicodeMasterPage& target) const noexcept
        {
            target.nonEmptyMask = 0;
            target.fullMask = 0;


            for (uint32_t si = 0; si < kUnicodeSubsPerMaster; ++si)
            {
                const UnicodeBitPageRef ref =
                    source.sub[si];


                target.sub[si] = ref;


                if (ref == kUnicodePageEmpty)
                    continue;


                const uint32_t mask =
                    uint32_t(1) << si;


                target.nonEmptyMask |= mask;


                if (ref == kUnicodePageFull)
                {
                    target.fullMask |= mask;
                    continue;
                }


                // Any non-sentinel reference must already address an actual
                // page in the shared bit-page pool.
                if (ref >= mBitPages.size())
                    return false;
            }


            std::memset(target.reserved, 0, sizeof(target.reserved));


            return true;
        }


        // ====================================================================
        // Page equality
        //
        // Both page structures have deterministic 128-byte representations.
        // ====================================================================

        [[nodiscard]]
        static bool bitPagesEqual(const UnicodeBitPage& a,
            const UnicodeBitPage& b) noexcept
        {
            return std::memcmp(&a, &b, sizeof(UnicodeBitPage)) == 0;
        }


        [[nodiscard]]
        static bool masterPagesEqual(const UnicodeMasterPage& a,
            const UnicodeMasterPage& b) noexcept
        {
            return std::memcmp(&a, &b, sizeof(UnicodeMasterPage)) == 0;
        }


        // ====================================================================
        // hashPage
        //
        // Produce a strong 64-bit content hash for one 128-byte page.
        //
        // This hash is builder-only state.  It is not serialized and is not
        // part of the Unicode database ABI.
        //
        // Hash equality alone never establishes page equality; it is always
        // followed by a complete 128-byte comparison.
        //
        // ====================================================================

        [[nodiscard]]
        static uint64_t hashPage(const void* page) noexcept
        {
            const uint8_t* bytes =
                static_cast<const uint8_t*>(page);


            uint64_t hash =
                0x9E3779B97F4A7C15ull;


            // 128 bytes == 16 uint64_t words.
            //
            // memcpy avoids alignment and strict-aliasing assumptions.
            for (uint32_t i = 0; i < 16; ++i)
            {
                uint64_t word = 0;

                std::memcpy(&word,
                    bytes + i * sizeof(uint64_t),
                    sizeof(uint64_t));


                hash ^= fmix64(
                    word +
                    0x9E3779B97F4A7C15ull +
                    static_cast<uint64_t>(i));


                hash = rotateLeft64(hash, 27);

                hash =
                    hash * 5ull +
                    0x52DCE729ull;
            }


            return fmix64(hash);
        }


        // ====================================================================
        // rotateLeft64
        // ====================================================================

        [[nodiscard]]
        static uint64_t rotateLeft64(uint64_t value, uint32_t count) noexcept
        {
            return
                (value << count) |
                (value >> (64u - count));
        }
    };

} // namespace waavs

