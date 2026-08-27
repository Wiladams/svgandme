// unicode_value_page_pool_builder8.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "core_hash.h"
#include "core_openhashmap.h"

#include "unicode_value_table8_data.h"


namespace waavs
{
    // ========================================================================
    // UnicodeValuePagePoolStats8
    //
    // Construction statistics for the shared VALUE8 page pools.
    //
    // Requests include pages which collapse to inline uniform references.
    //
    // Reused counts refer to actual stored pages that were found to already
    // exist in the pool.
    //
    // Hash collisions count cases where a hash-table key was occupied but
    // the referenced physical page was not identical.
    //
    // ========================================================================

    struct UnicodeValuePagePoolStats8
    {
        size_t valuePageRequests{ 0 };
        size_t valuePageUniform{ 0 };
        size_t valuePageReused{ 0 };
        size_t valuePageHashCollisions{ 0 };
        size_t uniqueValuePages{ 0 };

        size_t masterPageRequests{ 0 };
        size_t masterPageUniform{ 0 };
        size_t masterPageReused{ 0 };
        size_t masterPageHashCollisions{ 0 };
        size_t uniqueMasterPages{ 0 };


        [[nodiscard]]
        size_t storedPageCount() const noexcept
        {
            return uniqueValuePages + uniqueMasterPages;
        }


        [[nodiscard]]
        size_t storedBytes() const noexcept
        {
            return
                uniqueValuePages * sizeof(UnicodeValuePage8) +
                uniqueMasterPages * sizeof(UnicodeValueMasterPage8);
        }
    };


    // ========================================================================
    // UnicodeValuePagePoolBuilder8
    //
    // Maintains the two shared pools used by UnicodeValueTable8:
    //
    //      UnicodeValueMasterPage8[]
    //      UnicodeValuePage8[]
    //
    // Both pools are deduplicated independently.
    //
    //
    // Value pages
    // -----------
    //
    // Uniform:
    //
    //      inline UnicodeValuePage8Ref
    //
    // Nonuniform:
    //
    //      index into shared UnicodeValuePage8 pool
    //
    //
    // Master pages
    // ------------
    //
    // Uniform:
    //
    //      inline UnicodeValueMasterPage8Ref
    //
    // Nonuniform:
    //
    //      index into shared UnicodeValueMasterPage8 pool
    //
    //
    // All uint8_t values can be represented inline:
    //
    //      0xFF00 .. 0xFFFF
    //          ->
    //      value 0 .. 255
    //
    //
    // Unlike UnicodePagePoolBuilder, no master-page canonicalization metadata
    // is necessary. UnicodeValueMasterPage8 contains only its 32 sub-page
    // references and therefore already has a deterministic representation.
    //
    // ========================================================================

    class UnicodeValuePagePoolBuilder8
    {
    private:
        using ValuePageHashMap =
            WSOpenHashMap<uint64_t, UnicodeValuePage8Ref, WSHash64>;

        using MasterPageHashMap =
            WSOpenHashMap<uint64_t, UnicodeValueMasterPage8Ref, WSHash64>;


    public:
        UnicodeValuePagePoolBuilder8() = default;


        // ====================================================================
        // clear
        // ====================================================================

        void clear() noexcept
        {
            mValuePages.clear();
            mMasterPages.clear();

            mValuePageHashes.clear();
            mMasterPageHashes.clear();

            mStats = {};
        }


        // ====================================================================
        // reserve
        //
        // Optional generator-side optimization.
        //
        // Physical page pools cannot exceed kUnicodeValue8MaxPoolPages
        // because the upper reference range is reserved for inline uniform
        // values.
        // ====================================================================

        bool reserveValuePages(size_t count)
        {
            if (count > kUnicodeValue8MaxPoolPages)
                count = kUnicodeValue8MaxPoolPages;

            if (!mValuePageHashes.reserve(count))
                return false;

            mValuePages.reserve(count);

            return true;
        }


        bool reserveMasterPages(size_t count)
        {
            if (count > kUnicodeValue8MaxPoolPages)
                count = kUnicodeValue8MaxPoolPages;

            if (!mMasterPageHashes.reserve(count))
                return false;

            mMasterPages.reserve(count);

            return true;
        }


        // ====================================================================
        // internValuePage
        //
        // Intern one 1024-byte value page.
        //
        // A page containing the same value at every code point is represented
        // directly by an inline uniform reference and is never stored.
        //
        // Nonuniform pages are globally deduplicated.
        //
        // Returns false only when:
        //
        //      - the physical value-page pool is exhausted
        //      - the hash map cannot allocate/grow
        //
        // ====================================================================

        bool internValuePage(const UnicodeValuePage8& page, UnicodeValuePage8Ref& outRef)
        {
            ++mStats.valuePageRequests;


            // ---------------------------------------------------------------
            // Uniform page
            // ---------------------------------------------------------------

            uint8_t uniformValue = 0;

            if (valuePageIsUniform(page, uniformValue))
            {
                ++mStats.valuePageUniform;

                outRef = unicodeValue8UniformRef(uniformValue);
                return true;
            }


            // ---------------------------------------------------------------
            // Deduplicated physical page
            // ---------------------------------------------------------------

            const uint64_t baseHash = hashObject(page);

            uint64_t hashKey = baseHash;


            for (;;)
            {
                const UnicodeValuePage8Ref* existing =
                    mValuePageHashes.getRef(hashKey);


                if (existing)
                {
                    if (*existing < mValuePages.size() &&
                        valuePagesEqual(mValuePages[*existing], page))
                    {
                        ++mStats.valuePageReused;

                        outRef = *existing;
                        return true;
                    }


                    ++mStats.valuePageHashCollisions;

                    ++hashKey;
                    continue;
                }


                // -----------------------------------------------------------
                // New physical page
                // -----------------------------------------------------------

                if (mValuePages.size() >= kUnicodeValue8MaxPoolPages)
                    return false;


                const UnicodeValuePage8Ref newRef =
                    static_cast<UnicodeValuePage8Ref>(mValuePages.size());


                mValuePages.push_back(page);


                if (!mValuePageHashes.put(hashKey, newRef))
                {
                    mValuePages.pop_back();
                    return false;
                }


                ++mStats.uniqueValuePages;

                outRef = newRef;
                return true;
            }
        }


        // ====================================================================
        // internMasterPage
        //
        // Intern one VALUE8 master page.
        //
        // Every nonuniform sub-page reference must already refer to an actual
        // UnicodeValuePage8 in this builder's value-page pool.
        //
        // If all 32 sub-page references represent the same uniform value, the
        // complete master region is represented directly by an inline uniform
        // reference and no physical master page is stored.
        //
        // Returns false when:
        //
        //      - a nonuniform value-page reference is invalid
        //      - the physical master-page pool is exhausted
        //      - the hash map cannot allocate/grow
        //
        // ====================================================================

        bool internMasterPage(const UnicodeValueMasterPage8& page,
            UnicodeValueMasterPage8Ref& outRef)
        {
            ++mStats.masterPageRequests;


            // ---------------------------------------------------------------
            // Validate all child references.
            // ---------------------------------------------------------------

            if (!masterPageReferencesValid(page))
                return false;


            // ---------------------------------------------------------------
            // Uniform master
            // ---------------------------------------------------------------

            uint8_t uniformValue = 0;

            if (masterPageIsUniform(page, uniformValue))
            {
                ++mStats.masterPageUniform;

                outRef = unicodeValue8UniformRef(uniformValue);
                return true;
            }


            // ---------------------------------------------------------------
            // Deduplicate physical master page.
            // ---------------------------------------------------------------

            const uint64_t baseHash = hashObject(page);

            uint64_t hashKey = baseHash;


            for (;;)
            {
                const UnicodeValueMasterPage8Ref* existing =
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

                if (mMasterPages.size() >= kUnicodeValue8MaxPoolPages)
                    return false;


                const UnicodeValueMasterPage8Ref newRef =
                    static_cast<UnicodeValueMasterPage8Ref>(mMasterPages.size());


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
        const std::vector<UnicodeValuePage8>& valuePages() const noexcept
        {
            return mValuePages;
        }


        [[nodiscard]]
        const std::vector<UnicodeValueMasterPage8>& masterPages() const noexcept
        {
            return mMasterPages;
        }


        [[nodiscard]]
        const UnicodeValuePage8* valuePageData() const noexcept
        {
            return mValuePages.empty() ? nullptr : mValuePages.data();
        }


        [[nodiscard]]
        const UnicodeValueMasterPage8* masterPageData() const noexcept
        {
            return mMasterPages.empty() ? nullptr : mMasterPages.data();
        }


        [[nodiscard]]
        size_t valuePageCount() const noexcept
        {
            return mValuePages.size();
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
        const UnicodeValuePage8* valuePage(UnicodeValuePage8Ref ref) const noexcept
        {
            if (unicodeValue8RefIsUniform(ref) ||
                ref >= mValuePages.size())
            {
                return nullptr;
            }

            return &mValuePages[ref];
        }


        [[nodiscard]]
        const UnicodeValueMasterPage8* masterPage(UnicodeValueMasterPage8Ref ref) const noexcept
        {
            if (unicodeValue8RefIsUniform(ref) ||
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
        const UnicodeValuePagePoolStats8& stats() const noexcept
        {
            return mStats;
        }


    private:
        std::vector<UnicodeValuePage8> mValuePages;
        std::vector<UnicodeValueMasterPage8> mMasterPages;

        ValuePageHashMap mValuePageHashes;
        MasterPageHashMap mMasterPageHashes;

        UnicodeValuePagePoolStats8 mStats{};


        // ====================================================================
        // valuePageIsUniform
        // ====================================================================

        [[nodiscard]]
        static bool valuePageIsUniform(const UnicodeValuePage8& page,
            uint8_t& outValue) noexcept
        {
            const uint8_t value = page.values[0];


            for (uint32_t i = 1; i < kUnicodeSubSize; ++i)
            {
                if (page.values[i] != value)
                    return false;
            }


            outValue = value;
            return true;
        }


        // ====================================================================
        // masterPageReferencesValid
        //
        // Uniform references are intrinsically valid.
        //
        // Every physical reference must address an existing value page.
        // ====================================================================

        [[nodiscard]]
        bool masterPageReferencesValid(const UnicodeValueMasterPage8& page) const noexcept
        {
            for (uint32_t si = 0; si < kUnicodeSubsPerMaster; ++si)
            {
                const UnicodeValuePage8Ref ref = page.sub[si];


                if (unicodeValue8RefIsUniform(ref))
                    continue;


                if (ref >= mValuePages.size())
                    return false;
            }


            return true;
        }


        // ====================================================================
        // masterPageIsUniform
        //
        // A master page can collapse to an inline value only when all 32
        // sub-pages are themselves uniform and contain the same value.
        //
        // ====================================================================

        [[nodiscard]]
        static bool masterPageIsUniform(const UnicodeValueMasterPage8& page,
            uint8_t& outValue) noexcept
        {
            const UnicodeValuePage8Ref firstRef = page.sub[0];


            if (!unicodeValue8RefIsUniform(firstRef))
                return false;


            for (uint32_t si = 1; si < kUnicodeSubsPerMaster; ++si)
            {
                if (page.sub[si] != firstRef)
                    return false;
            }


            outValue = unicodeValue8RefUniformValue(firstRef);
            return true;
        }


        // ====================================================================
        // Page equality
        //
        // Both persistent structures have deterministic representations and
        // contain no padding that participates in their logical content.
        // ====================================================================

        [[nodiscard]]
        static bool valuePagesEqual(const UnicodeValuePage8& a,
            const UnicodeValuePage8& b) noexcept
        {
            return std::memcmp(&a, &b, sizeof(UnicodeValuePage8)) == 0;
        }


        [[nodiscard]]
        static bool masterPagesEqual(const UnicodeValueMasterPage8& a,
            const UnicodeValueMasterPage8& b) noexcept
        {
            return std::memcmp(&a, &b, sizeof(UnicodeValueMasterPage8)) == 0;
        }


        // ====================================================================
        // hashObject
        //
        // Produce a strong 64-bit content hash for a persistent VALUE8 page.
        //
        // Both current page structures are exact multiples of uint64_t:
        //
        //      UnicodeValuePage8        1024 bytes
        //      UnicodeValueMasterPage8    64 bytes
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
                "VALUE8 page hashing requires an integral number of uint64_t words");


            const uint8_t* bytes =
                reinterpret_cast<const uint8_t*>(&object);


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
