// unicode_database_builder.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include "core_nametable.h"
#include "core_openhashmap.h"

#include "unicode_coverage_data.h"
#include "unicode_database_format.h"
#include "unicode_page_pool_builder.h"


namespace waavs
{
    // ========================================================================
    // UnicodeDatabaseBuilderStats
    //
    // Statistics describing database construction and deduplication.
    //
    // Page-pool statistics are maintained independently by
    // UnicodePagePoolBuilder.
    // ========================================================================

    struct UnicodeDatabaseBuilderStats
    {
        size_t coverageRequests{ 0 };
        size_t coverageReused{ 0 };
        size_t coverageHashCollisions{ 0 };
        size_t uniqueCoverages{ 0 };

        size_t stringRequests{ 0 };
        size_t stringReused{ 0 };
        size_t uniqueStrings{ 0 };
    };


    // ========================================================================
    // UnicodeDatabaseBuilder
    //
    // Generator-side representation of the logical Unicode database.
    //
    // Responsibilities:
    //
    //      - own the shared page pools
    //      - deduplicate UnicodeCoverageData records
    //      - build the UTF-8 string pool
    //      - accumulate block records
    //      - accumulate script records
    //      - accumulate property records
    //
    // It deliberately does NOT:
    //
    //      - parse UCD text files
    //      - calculate physical file offsets
    //      - write the .ucdb file
    //
    // Those jobs belong to the parser and UnicodeDatabaseWriter.
    //
    // ========================================================================

    class UnicodeDatabaseBuilder
    {
    private:
        using CoverageHashMap =
            WSOpenHashMap<uint64_t, UnicodeCoverageIndex, WSHash64>;

        using StringOffsetMap =
            WSOpenHashMap<InternedKey, UnicodeStringOffset>;

    public:
        // ====================================================================
        // Construction
        //
        // String-pool offset zero is always the empty string.
        // ====================================================================

        UnicodeDatabaseBuilder()
        {
            initializeStringPool();
        }


        UnicodeDatabaseBuilder(const UnicodeDatabaseBuilder&) = delete;
        UnicodeDatabaseBuilder& operator=(const UnicodeDatabaseBuilder&) = delete;

        UnicodeDatabaseBuilder(UnicodeDatabaseBuilder&&) noexcept = default;
        UnicodeDatabaseBuilder& operator=(UnicodeDatabaseBuilder&&) noexcept = default;


        // ====================================================================
        // clear
        //
        // Return the builder to its initial empty state.
        // ====================================================================

        void clear()
        {
            mPagePool.clear();

            mCoverages.clear();
            mCoverageHashes.clear();

            mBlocks.clear();
            mScripts.clear();
            mProperties.clear();

            mStringPool.clear();
            mStringOffsets.clear();

            mStats = {};

            initializeStringPool();
        }


        // ====================================================================
        // Page pool
        // ====================================================================

        [[nodiscard]]
        UnicodePagePoolBuilder& pagePool() noexcept
        {
            return mPagePool;
        }


        [[nodiscard]]
        const UnicodePagePoolBuilder& pagePool() const noexcept
        {
            return mPagePool;
        }


        // ====================================================================
        // Coverage
        //
        // Complete UnicodeCoverageData records are themselves deduplicated.
        //
        // This gives us three levels of sharing:
        //
        //      UnicodeCoverageData
        //              |
        //              v
        //      UnicodeMasterPage
        //              |
        //              v
        //      UnicodeBitPage
        //
        // Returns false only if:
        //
        //      - the coverage index space is exhausted
        //      - WSOpenHashMap cannot grow
        //
        // ====================================================================

        bool addCoverage(const UnicodeCoverageData& coverage, UnicodeCoverageIndex& outIndex)
        {
            ++mStats.coverageRequests;


            const uint64_t baseHash =
                hashCoverage(coverage);


            uint64_t hashKey =
                baseHash;


            for (;;)
            {
                const UnicodeCoverageIndex* existing =
                    mCoverageHashes.getRef(hashKey);


                if (existing)
                {
                    if (*existing < mCoverages.size() &&
                        coveragesEqual(mCoverages[*existing], coverage))
                    {
                        ++mStats.coverageReused;

                        outIndex = *existing;
                        return true;
                    }


                    // A different 68-byte coverage produced the same content
                    // hash key.  Walk to another map key.
                    ++mStats.coverageHashCollisions;

                    ++hashKey;
                    continue;
                }


                // -----------------------------------------------------------
                // New coverage record.
                // -----------------------------------------------------------

                if (mCoverages.size() >= kUnicodeCoverageIndexInvalid)
                    return false;


                const UnicodeCoverageIndex newIndex =
                    static_cast<UnicodeCoverageIndex>(
                        mCoverages.size());


                mCoverages.push_back(coverage);


                if (!mCoverageHashes.put(hashKey, newIndex))
                {
                    mCoverages.pop_back();
                    return false;
                }


                ++mStats.uniqueCoverages;

                outIndex = newIndex;
                return true;
            }
        }


        // ====================================================================
        // String pool
        //
        // All semantic names should arrive as InternedKey values.
        //
        // We canonicalize through WSNameSet::INTERN() again so callers do
        // not have to rely on the typedef alone guaranteeing that the supplied
        // pointer is already canonical.
        //
        // String offsets are relative to the beginning of the database string
        // pool, not the beginning of the complete database.
        //
        // Offset zero is reserved for the empty string.
        // ====================================================================

        bool internString(InternedKey value, UnicodeStringOffset& outOffset)
        {
            ++mStats.stringRequests;


            if (!value)
            {
                outOffset =
                    kUnicodeStringOffsetInvalid;

                return false;
            }


            // ---------------------------------------------------------------
            // Empty string is permanently present at offset zero.
            // ---------------------------------------------------------------

            if (*value == '\0')
            {
                outOffset = 0;

                ++mStats.stringReused;
                return true;
            }


            // ---------------------------------------------------------------
            // Ensure pointer identity is canonical.
            // ---------------------------------------------------------------

            InternedKey key = WSNameSet::INTERN(value);

            if (!key)
            {
                outOffset = kUnicodeStringOffsetInvalid;

                return false;
            }


            // ---------------------------------------------------------------
            // Existing string.
            // ---------------------------------------------------------------

            const UnicodeStringOffset* existing = mStringOffsets.getRef(key);


            if (existing)
            {
                ++mStats.stringReused;

                outOffset = *existing;
                return true;
            }


            // ---------------------------------------------------------------
            // New string.
            // ---------------------------------------------------------------

            const size_t length = std::strlen(key);


            const size_t currentSize = mStringPool.size();


            // 0xFFFFFFFF is reserved as the invalid offset, so no string may
            // begin there.
            if (currentSize >=
                static_cast<size_t>(kUnicodeStringOffsetInvalid))
            {
                outOffset =
                    kUnicodeStringOffsetInvalid;

                return false;
            }


            // Ensure the complete NUL-terminated string fits in the uint32_t
            // string-pool address space.
            const size_t maximumSize =
                static_cast<size_t>(
                    std::numeric_limits<uint32_t>::max());


            if (length >= maximumSize - currentSize)
            {
                outOffset =
                    kUnicodeStringOffsetInvalid;

                return false;
            }


            const UnicodeStringOffset newOffset =
                static_cast<UnicodeStringOffset>(
                    currentSize);


            const size_t oldSize =
                mStringPool.size();


            mStringPool.insert(
                mStringPool.end(),
                reinterpret_cast<const uint8_t*>(key),
                reinterpret_cast<const uint8_t*>(key) + length);


            mStringPool.push_back(0);


            // ---------------------------------------------------------------
            // Add pointer -> string-pool-offset mapping.
            //
            // Roll back the byte pool if the hash map cannot grow.
            // ---------------------------------------------------------------

            if (!mStringOffsets.put(key, newOffset))
            {
                mStringPool.resize(oldSize);

                outOffset =
                    kUnicodeStringOffsetInvalid;

                return false;
            }


            ++mStats.uniqueStrings;

            outOffset = newOffset;
            return true;
        }


        // ====================================================================
        // Blocks
        //
        // Blocks are expected to be inserted in ascending code-point order.
        //
        // The builder enforces the important Blocks.txt invariants:
        //
        //      first <= last
        //      last < kUnicodeLimit
        //      no overlap
        //      ascending order
        //
        // ====================================================================

        bool addBlock(uint32_t first, uint32_t last, InternedKey name,
            UnicodeCoverageIndex coverageIndex)
        {
            uint32_t ignoredIndex = 0;

            return addBlock(
                first,
                last,
                name,
                coverageIndex,
                ignoredIndex);
        }


        bool addBlock(uint32_t first, uint32_t last, InternedKey name,
            UnicodeCoverageIndex coverageIndex, uint32_t& outIndex)
        {
            if (first > last)
                return false;


            if (last >= kUnicodeLimit)
                return false;


            if (!validCoverageIndex(coverageIndex))
                return false;


            if (!validRequiredName(name))
                return false;


            // ---------------------------------------------------------------
            // Blocks.txt is ordered and blocks do not overlap.
            // ---------------------------------------------------------------

            if (!mBlocks.empty())
            {
                const UnicodeBlockRecord& previous =
                    mBlocks.back();


                if (first <= previous.last)
                    return false;
            }


            UnicodeStringOffset nameOffset;


            if (!internString(name, nameOffset))
                return false;


            if (mBlocks.size() >=
                std::numeric_limits<uint32_t>::max())
            {
                return false;
            }


            UnicodeBlockRecord record{};

            record.first =
                first;

            record.last =
                last;

            record.nameOffset =
                nameOffset;

            record.coverageIndex =
                coverageIndex;


            const uint32_t newIndex =
                static_cast<uint32_t>(
                    mBlocks.size());


            mBlocks.push_back(record);


            outIndex =
                newIndex;


            return true;
        }


        // ====================================================================
        // Scripts
        // ====================================================================

        bool addScript(InternedKey name, InternedKey iso15924,
            UnicodeCoverageIndex coverageIndex)
        {
            uint32_t ignoredIndex = 0;

            return addScript(
                name,
                iso15924,
                coverageIndex,
                ignoredIndex);
        }


        bool addScript(InternedKey name, InternedKey iso15924,
            UnicodeCoverageIndex coverageIndex, uint32_t& outIndex)
        {
            if (!validCoverageIndex(coverageIndex))
                return false;


            if (!validRequiredName(name))
                return false;


            if (!validRequiredName(iso15924))
                return false;


            UnicodeStringOffset nameOffset;
            UnicodeStringOffset iso15924Offset;


            if (!internString(name, nameOffset))
                return false;


            if (!internString(iso15924, iso15924Offset))
                return false;


            if (mScripts.size() >=
                std::numeric_limits<uint32_t>::max())
            {
                return false;
            }


            UnicodeScriptRecord record{};

            record.nameOffset =
                nameOffset;

            record.iso15924Offset =
                iso15924Offset;

            record.coverageIndex =
                coverageIndex;

            record.reserved =
                0;


            const uint32_t newIndex =
                static_cast<uint32_t>(
                    mScripts.size());


            mScripts.push_back(record);


            outIndex =
                newIndex;


            return true;
        }


        // ====================================================================
        // Binary properties
        // ====================================================================

        bool addProperty(InternedKey name, UnicodeCoverageIndex coverageIndex,
            UnicodeDatabasePropertySource source)
        {
            uint32_t ignoredIndex = 0;

            return addProperty(
                name,
                coverageIndex,
                source,
                ignoredIndex);
        }


        bool addProperty(InternedKey name, UnicodeCoverageIndex coverageIndex,
            UnicodeDatabasePropertySource source, uint32_t& outIndex)
        {
            if (!validCoverageIndex(coverageIndex))
                return false;


            if (!validRequiredName(name))
                return false;


            if (source < UnicodePropertySourceUnknown ||
                source > UnicodePropertySourceEmojiData)
            {
                return false;
            }


            UnicodeStringOffset nameOffset;


            if (!internString(name, nameOffset))
                return false;


            if (mProperties.size() >=
                std::numeric_limits<uint32_t>::max())
            {
                return false;
            }


            UnicodePropertyRecord record{};

            record.nameOffset =
                nameOffset;

            record.coverageIndex =
                coverageIndex;

            record.source =
                static_cast<uint16_t>(source);

            record.reserved0 =
                0;

            record.reserved1 =
                0;


            const uint32_t newIndex =
                static_cast<uint32_t>(
                    mProperties.size());


            mProperties.push_back(record);


            outIndex =
                newIndex;


            return true;
        }


        // ====================================================================
        // Optional reserve helpers
        //
        // These affect generator performance only.
        // ====================================================================

        bool reserveCoverages(size_t count)
        {
            if (count >=
                static_cast<size_t>(kUnicodeCoverageIndexInvalid))
            {
                return false;
            }


            if (!mCoverageHashes.reserve(count))
                return false;


            mCoverages.reserve(count);

            return true;
        }


        bool reserveStrings(size_t count, size_t bytes)
        {
            if (bytes >
                static_cast<size_t>(
                    std::numeric_limits<uint32_t>::max()))
            {
                return false;
            }


            if (!mStringOffsets.reserve(count))
                return false;


            mStringPool.reserve(bytes);

            return true;
        }


        void reserveBlocks(size_t count)
        {
            mBlocks.reserve(count);
        }


        void reserveScripts(size_t count)
        {
            mScripts.reserve(count);
        }


        void reserveProperties(size_t count)
        {
            mProperties.reserve(count);
        }


        // ====================================================================
        // Database contents
        //
        // UnicodeDatabaseWriter will consume these directly.
        // ====================================================================

        [[nodiscard]]
        const std::vector<UnicodeCoverageData>& coverages() const noexcept
        {
            return mCoverages;
        }


        [[nodiscard]]
        const std::vector<UnicodeBlockRecord>& blocks() const noexcept
        {
            return mBlocks;
        }


        [[nodiscard]]
        const std::vector<UnicodeScriptRecord>& scripts() const noexcept
        {
            return mScripts;
        }


        [[nodiscard]]
        const std::vector<UnicodePropertyRecord>& properties() const noexcept
        {
            return mProperties;
        }


        [[nodiscard]]
        const std::vector<uint8_t>& stringPool() const noexcept
        {
            return mStringPool;
        }


        // ====================================================================
        // Raw section access
        // ====================================================================

        [[nodiscard]]
        const UnicodeCoverageData* coverageData() const noexcept
        {
            return mCoverages.empty()
                ? nullptr
                : mCoverages.data();
        }


        [[nodiscard]]
        const UnicodeBlockRecord* blockData() const noexcept
        {
            return mBlocks.empty()
                ? nullptr
                : mBlocks.data();
        }


        [[nodiscard]]
        const UnicodeScriptRecord* scriptData() const noexcept
        {
            return mScripts.empty()
                ? nullptr
                : mScripts.data();
        }


        [[nodiscard]]
        const UnicodePropertyRecord* propertyData() const noexcept
        {
            return mProperties.empty()
                ? nullptr
                : mProperties.data();
        }


        [[nodiscard]]
        const uint8_t* stringPoolData() const noexcept
        {
            return mStringPool.empty()
                ? nullptr
                : mStringPool.data();
        }


        // ====================================================================
        // Counts
        // ====================================================================

        [[nodiscard]]
        size_t coverageCount() const noexcept
        {
            return mCoverages.size();
        }


        [[nodiscard]]
        size_t blockCount() const noexcept
        {
            return mBlocks.size();
        }


        [[nodiscard]]
        size_t scriptCount() const noexcept
        {
            return mScripts.size();
        }


        [[nodiscard]]
        size_t propertyCount() const noexcept
        {
            return mProperties.size();
        }


        [[nodiscard]]
        size_t stringPoolSize() const noexcept
        {
            return mStringPool.size();
        }


        // ====================================================================
        // Statistics
        // ====================================================================

        [[nodiscard]]
        const UnicodeDatabaseBuilderStats& stats() const noexcept
        {
            return mStats;
        }


    private:
        // ====================================================================
        // Persistent database content
        // ====================================================================

        UnicodePagePoolBuilder mPagePool;

        std::vector<UnicodeCoverageData> mCoverages;

        std::vector<UnicodeBlockRecord> mBlocks;
        std::vector<UnicodeScriptRecord> mScripts;
        std::vector<UnicodePropertyRecord> mProperties;

        std::vector<uint8_t> mStringPool;


        // ====================================================================
        // Generator-only indexing structures
        //
        // These are never serialized.
        // ====================================================================

        CoverageHashMap mCoverageHashes;
        StringOffsetMap mStringOffsets;


        UnicodeDatabaseBuilderStats mStats{};


        // ====================================================================
        // initializeStringPool
        //
        // Offset zero permanently represents the empty string.
        // ====================================================================

        void initializeStringPool()
        {
            if (mStringPool.empty())
                mStringPool.push_back(0);
        }


        // ====================================================================
        // Coverage validation
        // ====================================================================

        [[nodiscard]]
        bool validCoverageIndex(UnicodeCoverageIndex index) const noexcept
        {
            return
                index != kUnicodeCoverageIndexInvalid &&
                index < mCoverages.size();
        }


        // ====================================================================
        // Required semantic names
        // ====================================================================

        [[nodiscard]]
        static bool validRequiredName(InternedKey name) noexcept
        {
            return
                name != nullptr &&
                *name != '\0';
        }


        // ====================================================================
        // Coverage equality
        //
        // UnicodeCoverageData is exactly 68 bytes with no padding.
        // ====================================================================

        [[nodiscard]]
        static bool coveragesEqual(const UnicodeCoverageData& a,
            const UnicodeCoverageData& b) noexcept
        {
            return
                std::memcmp(
                    &a,
                    &b,
                    sizeof(UnicodeCoverageData)) == 0;
        }


        // ====================================================================
        // Coverage hash
        //
        // UnicodeCoverageData is 68 bytes:
        //
        //      8 complete uint64_t chunks = 64 bytes
        //      1 uint32_t tail           =  4 bytes
        //
        // The hash is generator-only and is not part of the database ABI.
        // ====================================================================

        [[nodiscard]]
        static uint64_t hashCoverage(const UnicodeCoverageData& coverage) noexcept
        {
            const uint8_t* bytes =
                reinterpret_cast<const uint8_t*>(
                    &coverage);


            uint64_t hash =
                0x9E3779B97F4A7C15ull;


            constexpr size_t wordCount =
                sizeof(UnicodeCoverageData) /
                sizeof(uint64_t);


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


            // ---------------------------------------------------------------
            // Remaining four bytes.
            // ---------------------------------------------------------------

            constexpr size_t consumed =
                wordCount *
                sizeof(uint64_t);


            static_assert(
                sizeof(UnicodeCoverageData) - consumed == sizeof(uint32_t),
                "UnicodeCoverageData hash expects a four-byte tail");


            uint32_t tail = 0;


            std::memcpy(
                &tail,
                bytes + consumed,
                sizeof(uint32_t));


            hash ^=
                fmix64(
                    static_cast<uint64_t>(tail) +
                    0xD6E8FEB86659FD93ull);


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
