// unicode_decomposition_page_pool_builder.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "core_hash.h"
#include "core_openhashmap.h"

#include "unicode_decomposition_data.h"


namespace waavs
{
    // ========================================================================
    // UnicodeDecompositionPagePoolStats
    //
    // Construction statistics for the shared canonical-decomposition page
    // pools.
    //
    // Requests include pages which collapse to the empty sentinel.
    //
    // Reused counts refer to actual stored pages that were found to already
    // exist in the pool.
    //
    // Hash collisions count cases where a hash-table key was occupied but
    // the referenced physical page was not identical.
    //
    // ========================================================================

    struct UnicodeDecompositionPagePoolStats
    {
        size_t pageRequests{ 0 };
        size_t pageEmpty{ 0 };
        size_t pageReused{ 0 };
        size_t pageHashCollisions{ 0 };
        size_t uniquePages{ 0 };

        size_t masterPageRequests{ 0 };
        size_t masterPageEmpty{ 0 };
        size_t masterPageReused{ 0 };
        size_t masterPageHashCollisions{ 0 };
        size_t uniqueMasterPages{ 0 };


        [[nodiscard]]
        size_t storedPageCount() const noexcept
        {
            return uniquePages + uniqueMasterPages;
        }


        [[nodiscard]]
        size_t storedBytes() const noexcept
        {
            return
                uniquePages * sizeof(UnicodeDecompositionPage) +
                uniqueMasterPages * sizeof(UnicodeDecompositionMasterPage);
        }
    };


    // ========================================================================
    // UnicodeDecompositionPagePoolBuilder
    //
    // Maintains the two shared page pools used by canonical decomposition:
    //
    //      UnicodeDecompositionMasterPage[]
    //      UnicodeDecompositionPage[]
    //
    // Both pools are deduplicated independently.
    //
    //
    // Decomposition pages
    // -------------------
    //
    // Empty:
    //
    //      kUnicodeDecompositionPageEmpty
    //
    // Nonempty:
    //
    //      index into shared UnicodeDecompositionPage pool
    //
    //
    // Master pages
    // ------------
    //
    // Empty:
    //
    //      kUnicodeDecompositionPageEmpty
    //
    // Nonempty:
    //
    //      index into shared UnicodeDecompositionMasterPage pool
    //
    //
    // A decomposition leaf page is empty when every entry contains:
    //
    //      kUnicodeDecompositionRecordNone
    //
    // There is intentionally no "full" or arbitrary uniform representation.
    // The only useful canonical special case for decomposition is absence.
    //
    // Both persistent structures have deterministic byte representations, so
    // no additional master-page canonicalization metadata is necessary.
    //
    // ========================================================================

    class UnicodeDecompositionPagePoolBuilder
    {
    private:
        using PageHashMap =
            WSOpenHashMap<uint64_t, UnicodeDecompositionPageRef, WSHash64>;

        using MasterPageHashMap =
            WSOpenHashMap<uint64_t, UnicodeDecompositionMasterPageRef, WSHash64>;


    public:
        UnicodeDecompositionPagePoolBuilder() = default;


        // ====================================================================
        // clear
        // ====================================================================

        void clear() noexcept
        {
            mPages.clear();
            mMasterPages.clear();

            mPageHashes.clear();
            mMasterPageHashes.clear();

            mStats = {};
        }


        // ====================================================================
        // reserve
        //
        // Optional generator-side optimization.
        //
        // Physical page pools cannot exceed
        // kUnicodeDecompositionMaxPoolPages because 0xFFFF is reserved for
        // kUnicodeDecompositionPageEmpty.
        // ====================================================================

        bool reservePages(size_t count)
        {
            if (count > kUnicodeDecompositionMaxPoolPages)
                count = kUnicodeDecompositionMaxPoolPages;

            if (!mPageHashes.reserve(count))
                return false;

            mPages.reserve(count);

            return true;
        }


        bool reserveMasterPages(size_t count)
        {
            if (count > kUnicodeDecompositionMaxPoolPages)
                count = kUnicodeDecompositionMaxPoolPages;

            if (!mMasterPageHashes.reserve(count))
                return false;

            mMasterPages.reserve(count);

            return true;
        }


        // ====================================================================
        // internPage
        //
        // Intern one 1024-code-point decomposition page.
        //
        // A page containing no decomposition mappings is represented by
        // kUnicodeDecompositionPageEmpty and is never stored physically.
        //
        // Nonempty pages are globally deduplicated.
        //
        // This builder does not validate UnicodeDecompositionRecordRef values.
        // Record ownership belongs to the decomposition-table builder. At this
        // layer the record references are simply persistent leaf values.
        //
        // Returns false only when:
        //
        //      - the physical decomposition-page pool is exhausted
        //      - the hash map cannot allocate/grow
        //
        // ====================================================================

        bool internPage(const UnicodeDecompositionPage& page,
            UnicodeDecompositionPageRef& outRef)
        {
            ++mStats.pageRequests;


            // ---------------------------------------------------------------
            // Empty page
            // ---------------------------------------------------------------

            if (pageIsEmpty(page))
            {
                ++mStats.pageEmpty;

                outRef = kUnicodeDecompositionPageEmpty;
                return true;
            }


            // ---------------------------------------------------------------
            // Deduplicated physical page
            // ---------------------------------------------------------------

            const uint64_t baseHash = hashObject(page);

            uint64_t hashKey = baseHash;


            for (;;)
            {
                const UnicodeDecompositionPageRef* existing =
                    mPageHashes.getRef(hashKey);


                if (existing)
                {
                    if (*existing < mPages.size() &&
                        pagesEqual(mPages[*existing], page))
                    {
                        ++mStats.pageReused;

                        outRef = *existing;
                        return true;
                    }


                    ++mStats.pageHashCollisions;

                    ++hashKey;
                    continue;
                }


                // -----------------------------------------------------------
                // New physical page
                // -----------------------------------------------------------

                if (mPages.size() >= kUnicodeDecompositionMaxPoolPages)
                    return false;


                const UnicodeDecompositionPageRef newRef =
                    static_cast<UnicodeDecompositionPageRef>(
                        mPages.size());


                mPages.push_back(page);


                if (!mPageHashes.put(hashKey, newRef))
                {
                    mPages.pop_back();
                    return false;
                }


                ++mStats.uniquePages;

                outRef = newRef;
                return true;
            }
        }


        // ====================================================================
        // internMasterPage
        //
        // Intern one decomposition master page.
        //
        // Every non-empty sub-page reference must already refer to an actual
        // UnicodeDecompositionPage in this builder's page pool.
        //
        // If all 32 sub-page references are empty, the complete master region
        // is represented by kUnicodeDecompositionPageEmpty and no physical
        // master page is stored.
        //
        // Returns false when:
        //
        //      - a physical decomposition-page reference is invalid
        //      - the physical master-page pool is exhausted
        //      - the hash map cannot allocate/grow
        //
        // ====================================================================

        bool internMasterPage(const UnicodeDecompositionMasterPage& page,
            UnicodeDecompositionMasterPageRef& outRef)
        {
            ++mStats.masterPageRequests;


            // ---------------------------------------------------------------
            // Validate child references.
            // ---------------------------------------------------------------

            if (!masterPageReferencesValid(page))
                return false;


            // ---------------------------------------------------------------
            // Empty master
            // ---------------------------------------------------------------

            if (masterPageIsEmpty(page))
            {
                ++mStats.masterPageEmpty;

                outRef = kUnicodeDecompositionPageEmpty;
                return true;
            }


            // ---------------------------------------------------------------
            // Deduplicated physical master page
            // ---------------------------------------------------------------

            const uint64_t baseHash = hashObject(page);

            uint64_t hashKey = baseHash;


            for (;;)
            {
                const UnicodeDecompositionMasterPageRef* existing =
                    mMasterPageHashes.getRef(hashKey);


                if (existing)
                {
                    if (*existing < mMasterPages.size() &&
                        masterPagesEqual(mMasterPages[*existing], page))
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
                // New physical master page
                // -----------------------------------------------------------

                if (mMasterPages.size() >=
                    kUnicodeDecompositionMaxPoolPages)
                {
                    return false;
                }


                const UnicodeDecompositionMasterPageRef newRef =
                    static_cast<UnicodeDecompositionMasterPageRef>(
                        mMasterPages.size());


                mMasterPages.push_back(page);


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
        const std::vector<UnicodeDecompositionPage>& pages() const noexcept
        {
            return mPages;
        }


        [[nodiscard]]
        const std::vector<UnicodeDecompositionMasterPage>& masterPages() const noexcept
        {
            return mMasterPages;
        }


        [[nodiscard]]
        const UnicodeDecompositionPage* pageData() const noexcept
        {
            return mPages.empty() ? nullptr : mPages.data();
        }


        [[nodiscard]]
        const UnicodeDecompositionMasterPage* masterPageData() const noexcept
        {
            return mMasterPages.empty() ? nullptr : mMasterPages.data();
        }


        [[nodiscard]]
        size_t pageCount() const noexcept
        {
            return mPages.size();
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
        const UnicodeDecompositionPage* page(
            UnicodeDecompositionPageRef ref) const noexcept
        {
            if (ref == kUnicodeDecompositionPageEmpty ||
                ref >= mPages.size())
            {
                return nullptr;
            }

            return &mPages[ref];
        }


        [[nodiscard]]
        const UnicodeDecompositionMasterPage* masterPage(
            UnicodeDecompositionMasterPageRef ref) const noexcept
        {
            if (ref == kUnicodeDecompositionPageEmpty ||
                ref >= mMasterPages.size())
            {
                return nullptr;
            }

            return &mMasterPages[ref];
        }


        // ====================================================================
        // Statistics
        // ====================================================================

        [[nodiscard]]
        const UnicodeDecompositionPagePoolStats& stats() const noexcept
        {
            return mStats;
        }


    private:
        std::vector<UnicodeDecompositionPage> mPages;
        std::vector<UnicodeDecompositionMasterPage> mMasterPages;

        PageHashMap mPageHashes;
        MasterPageHashMap mMasterPageHashes;

        UnicodeDecompositionPagePoolStats mStats{};


        // ====================================================================
        // pageIsEmpty
        //
        // A zero-filled leaf page represents 1024 code points having no
        // canonical decomposition mapping.
        // ====================================================================

        [[nodiscard]]
        static bool pageIsEmpty(const UnicodeDecompositionPage& page) noexcept
        {
            for (uint32_t i = 0; i < kUnicodeSubSize; ++i)
            {
                if (page.mapping[i] !=
                    kUnicodeDecompositionRecordNone)
                {
                    return false;
                }
            }

            return true;
        }


        // ====================================================================
        // masterPageReferencesValid
        //
        // Empty references are intrinsically valid.
        //
        // Every physical reference must address an existing decomposition
        // leaf page.
        // ====================================================================

        [[nodiscard]]
        bool masterPageReferencesValid(
            const UnicodeDecompositionMasterPage& page) const noexcept
        {
            for (uint32_t si = 0; si < kUnicodeSubsPerMaster; ++si)
            {
                const UnicodeDecompositionPageRef ref =
                    page.sub[si];


                if (ref == kUnicodeDecompositionPageEmpty)
                    continue;


                if (ref >= mPages.size())
                    return false;
            }


            return true;
        }


        // ====================================================================
        // masterPageIsEmpty
        // ====================================================================

        [[nodiscard]]
        static bool masterPageIsEmpty(
            const UnicodeDecompositionMasterPage& page) noexcept
        {
            for (uint32_t si = 0; si < kUnicodeSubsPerMaster; ++si)
            {
                if (page.sub[si] !=
                    kUnicodeDecompositionPageEmpty)
                {
                    return false;
                }
            }


            return true;
        }


        // ====================================================================
        // Page equality
        //
        // Both persistent structures have deterministic representations and
        // contain no padding that participates in their logical content.
        // ====================================================================

        [[nodiscard]]
        static bool pagesEqual(const UnicodeDecompositionPage& a,
            const UnicodeDecompositionPage& b) noexcept
        {
            return
                std::memcmp(
                    &a,
                    &b,
                    sizeof(UnicodeDecompositionPage)) == 0;
        }


        [[nodiscard]]
        static bool masterPagesEqual(
            const UnicodeDecompositionMasterPage& a,
            const UnicodeDecompositionMasterPage& b) noexcept
        {
            return
                std::memcmp(
                    &a,
                    &b,
                    sizeof(UnicodeDecompositionMasterPage)) == 0;
        }


        // ====================================================================
        // hashObject
        //
        // Produce a strong 64-bit content hash for a persistent decomposition
        // page.
        //
        // Both page structures are exact multiples of uint64_t:
        //
        //      UnicodeDecompositionPage        2048 bytes
        //      UnicodeDecompositionMasterPage   64 bytes
        //
        // The hash is generator-only state and is not part of the persistent
        // database ABI.
        //
        // Hash equality never establishes page equality; the complete object
        // is compared before reuse.
        //
        // ====================================================================

        template <typename T>
        [[nodiscard]]
        static uint64_t hashObject(const T& object) noexcept
        {
            static_assert(
                sizeof(T) % sizeof(uint64_t) == 0,
                "decomposition page hashing requires an integral number of uint64_t words");


            const uint8_t* bytes =
                reinterpret_cast<const uint8_t*>(
                    &object);


            constexpr size_t wordCount =
                sizeof(T) / sizeof(uint64_t);


            uint64_t hash =
                0x9E3779B97F4A7C15ull;


            for (size_t i = 0; i < wordCount; ++i)
            {
                uint64_t word = 0;


                std::memcpy(
                    &word,
                    bytes + i * sizeof(uint64_t),
                    sizeof(uint64_t));


                hash ^=
                    fmix64(
                        word +
                        0x9E3779B97F4A7C15ull +
                        static_cast<uint64_t>(i));


                hash =
                    rotateLeft64(hash, 27);


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
