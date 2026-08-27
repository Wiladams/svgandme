// unicode_database_builder.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>

#include "core_nametable.h"
#include "core_openhashmap.h"

#include "unicode_coverage_data.h"
#include "unicode_database_format.h"
#include "unicode_composition_data.h"
#include "unicode_decomposition_page_pool_builder.h"
#include "unicode_page_pool_builder.h"
#include "unicode_value_page_pool_builder8.h"


namespace waavs
{
    // ========================================================================
    // UnicodeDatabaseBuilderStats
    //
    // Statistics describing database construction and deduplication.
    //
    // Coverage page-pool statistics are maintained independently by
    // UnicodePagePoolBuilder.
    //
    // VALUE8 page-pool statistics are maintained independently by
    // UnicodeValuePagePoolBuilder8.
    //
    // Canonical-decomposition page-pool statistics are maintained independently
    // by UnicodeDecompositionPagePoolBuilder.
    // ========================================================================

    struct UnicodeDatabaseBuilderStats
    {
        size_t coverageRequests{ 0 };
        size_t coverageReused{ 0 };
        size_t coverageHashCollisions{ 0 };
        size_t uniqueCoverages{ 0 };

        size_t valueTable8Requests{ 0 };
        size_t valueTable8Reused{ 0 };
        size_t valueTable8HashCollisions{ 0 };
        size_t uniqueValueTables8{ 0 };

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
    //      - own shared SET page pools
    //      - own shared VALUE8 page pools
    //      - own canonical-decomposition page pools
    //      - own canonical-decomposition records
    //      - own canonical-composition records
    //      - deduplicate UnicodeCoverageData records
    //      - deduplicate UnicodeValueTable8Data records
    //      - associate semantic VALUE8 properties with value tables
    //      - build the UTF-8 string pool
    //      - accumulate block records
    //      - accumulate script records
    //      - accumulate binary property records
    //
    // It deliberately does NOT:
    //
    //      - parse UCD text files
    //      - calculate physical file offsets
    //      - write the .ucdb file
    //
    // Those jobs belong to file-specific parsers and UnicodeDatabaseWriter.
    //
    // ========================================================================

    class UnicodeDatabaseBuilder
    {
    private:
        using CoverageHashMap = WSOpenHashMap<uint64_t, UnicodeCoverageIndex, WSHash64>;

        using ValueTable8HashMap = WSOpenHashMap<uint64_t, UnicodeValueTable8Index, WSHash64>;

        using StringOffsetMap = WSOpenHashMap<InternedKey, UnicodeStringOffset>;

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
            mValuePagePool8.clear();
            mDecompositionPagePool.clear();

            mCoverages.clear();
            mCoverageHashes.clear();

            mValueTables8.clear();
            mValueTable8Hashes.clear();
            mValueProperties8.clear();

            mDecomposition = {};
            mDecompositionRecords.clear();
            mHasDecomposition = false;

            mCompositionRecords.clear();
            mHasComposition = false;

            mBlocks.clear();
            mScripts.clear();
            mProperties.clear();

            mStringPool.clear();
            mStringOffsets.clear();

            mStats = {};

            initializeStringPool();
        }


        // ====================================================================
        // SET page pool
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
        // VALUE8 page pool
        //
        // All uint8-valued Unicode properties share this pool.
        // ====================================================================

        [[nodiscard]]
        UnicodeValuePagePoolBuilder8& valuePagePool8() noexcept
        {
            return mValuePagePool8;
        }


        [[nodiscard]]
        const UnicodeValuePagePoolBuilder8& valuePagePool8() const noexcept
        {
            return mValuePagePool8;
        }


        // ====================================================================
        // Canonical-decomposition page pool
        //
        // Canonical decomposition has its own independent master-page and
        // leaf-page reference spaces.
        // ====================================================================

        [[nodiscard]]
        UnicodeDecompositionPagePoolBuilder& decompositionPagePool() noexcept
        {
            return mDecompositionPagePool;
        }


        [[nodiscard]]
        const UnicodeDecompositionPagePoolBuilder& decompositionPagePool() const noexcept
        {
            return mDecompositionPagePool;
        }


        // ====================================================================
        // Coverage
        //
        // Complete UnicodeCoverageData records are deduplicated.
        //
        // This gives SET data three levels of sharing:
        //
        //      UnicodeCoverageData
        //              |
        //              v
        //      UnicodeMasterPage
        //              |
        //              v
        //      UnicodeBitPage
        // ====================================================================

        bool addCoverage(const UnicodeCoverageData& coverage,
            UnicodeCoverageIndex& outIndex)
        {
            ++mStats.coverageRequests;


            const uint64_t baseHash =
                hashFixedRecord68(coverage);

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


                    ++mStats.coverageHashCollisions;
                    ++hashKey;
                    continue;
                }


                if (mCoverages.size() >= kUnicodeCoverageIndexInvalid)
                    return false;


                const UnicodeCoverageIndex newIndex =
                    static_cast<UnicodeCoverageIndex>(mCoverages.size());


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
        // VALUE8 tables
        //
        // Complete UnicodeValueTable8Data roots are themselves deduplicated.
        //
        // This gives VALUE8 data three levels of sharing:
        //
        //      UnicodeValueTable8Data
        //              |
        //              v
        //      UnicodeValueMasterPage8
        //              |
        //              v
        //      UnicodeValuePage8
        //
        // The page levels are deduplicated by UnicodeValuePagePoolBuilder8.
        // ====================================================================

        bool addValueTable8(const UnicodeValueTable8Data& table,
            UnicodeValueTable8Index& outIndex)
        {
            ++mStats.valueTable8Requests;


            const uint64_t baseHash =
                hashFixedRecord68(table);

            uint64_t hashKey =
                baseHash;


            for (;;)
            {
                const UnicodeValueTable8Index* existing =
                    mValueTable8Hashes.getRef(hashKey);


                if (existing)
                {
                    if (*existing < mValueTables8.size() &&
                        valueTables8Equal(mValueTables8[*existing], table))
                    {
                        ++mStats.valueTable8Reused;

                        outIndex = *existing;
                        return true;
                    }


                    ++mStats.valueTable8HashCollisions;
                    ++hashKey;
                    continue;
                }


                if (mValueTables8.size() >= kUnicodeValueTable8IndexInvalid)
                    return false;


                const UnicodeValueTable8Index newIndex =
                    static_cast<UnicodeValueTable8Index>(mValueTables8.size());


                mValueTables8.push_back(table);


                if (!mValueTable8Hashes.put(hashKey, newIndex))
                {
                    mValueTables8.pop_back();
                    return false;
                }


                ++mStats.uniqueValueTables8;

                outIndex = newIndex;
                return true;
            }
        }


        // ====================================================================
        // VALUE8 semantic properties
        //
        // Associate one semantic Unicode property with one VALUE8 table.
        //
        // A semantic property may appear only once.
        // ====================================================================

        bool addValueProperty8(UnicodeValueProperty8 property,
            UnicodeValueTable8Index tableIndex)
        {
            uint32_t ignoredIndex = 0;

            return addValueProperty8(
                property,
                tableIndex,
                ignoredIndex);
        }


        bool addValueProperty8(UnicodeValueProperty8 property,
            UnicodeValueTable8Index tableIndex, uint32_t& outIndex)
        {
            if (!validValueTable8Index(tableIndex))
                return false;


            if (property <= UnicodeValueProperty8Unknown ||
                property > UnicodeValueProperty8MAX)
            {
                return false;
            }


            const uint16_t storedProperty =
                static_cast<uint16_t>(property);


            for (const UnicodeValueProperty8Record& existing : mValueProperties8)
            {
                if (existing.property == storedProperty)
                    return false;
            }


            if (mValueProperties8.size() >=
                std::numeric_limits<uint32_t>::max())
            {
                return false;
            }


            UnicodeValueProperty8Record record{};

            record.property = storedProperty;
            record.reserved = 0;
            record.tableIndex = tableIndex;


            const uint32_t newIndex =
                static_cast<uint32_t>(mValueProperties8.size());


            mValueProperties8.push_back(record);

            outIndex = newIndex;

            return true;
        }


        // ====================================================================
        // Canonical decomposition
        //
        // Register the one canonical-decomposition dataset belonging to this
        // database.
        //
        // The caller first finalizes UnicodeDecompositionBuilder into:
        //
        //      decompositionPagePool()
        //
        // and receives a UnicodeDecompositionData root.
        //
        // The builder's decomposition records are then supplied here without
        // modification. Leaf-page record references are already indexes into
        // this record sequence, so record order must be preserved.
        //
        // A database may contain at most one canonical-decomposition dataset.
        // ====================================================================

        bool setDecomposition(const UnicodeDecompositionData& data,
            const std::vector<UnicodeDecompositionRecord>& records)
        {
            if (mHasDecomposition)
                return false;


            if (records.size() >
                static_cast<size_t>(kUnicodeDecompositionMaxRecords))
            {
                return false;
            }


            if (!validateDecompositionRecords(records))
                return false;


            if (!validateDecompositionHierarchy(data, records.size()))
                return false;


            mDecomposition = data;
            mDecompositionRecords = records;
            mHasDecomposition = true;

            return true;
        }


        // ====================================================================
        // Canonical composition
        //
        // Register the one canonical-composition dataset belonging to this
        // database.
        //
        // UnicodeCompositionBuilder::finalize() supplies records already
        // ordered strictly by:
        //
        //      first
        //      second
        //
        // The database builder validates that persistent invariant again at the
        // handoff boundary. This simultaneously verifies sorting and guarantees
        // that no duplicate or conflicting pair key can enter the database.
        //
        // Hangul composition is not stored here. It remains algorithmic.
        //
        // A database may contain at most one canonical-composition dataset.
        // ====================================================================

        bool setComposition(std::vector<UnicodeCompositionRecord>&& records)
        {
            if (mHasComposition)
                return false;

            if (records.empty())
                return false;

            if (records.size() >
                static_cast<size_t>(std::numeric_limits<uint32_t>::max()))
            {
                return false;
            }

            if (!validateCompositionRecords(records))
                return false;

            mCompositionRecords = std::move(records);
            mHasComposition = true;

            return true;
        }


        // ====================================================================
        // String pool
        //
        // All semantic names should arrive as InternedKey values.
        //
        // String offsets are relative to the beginning of the database string
        // pool.
        //
        // Offset zero is permanently the empty string.
        // ====================================================================

        bool internString(InternedKey value, UnicodeStringOffset& outOffset)
        {
            ++mStats.stringRequests;


            if (!value)
            {
                outOffset = kUnicodeStringOffsetInvalid;
                return false;
            }


            if (*value == '\0')
            {
                outOffset = 0;

                ++mStats.stringReused;
                return true;
            }


            InternedKey key =
                WSNameSet::INTERN(value);


            if (!key)
            {
                outOffset = kUnicodeStringOffsetInvalid;
                return false;
            }


            const UnicodeStringOffset* existing =
                mStringOffsets.getRef(key);


            if (existing)
            {
                ++mStats.stringReused;

                outOffset = *existing;
                return true;
            }


            const size_t length =
                std::strlen(key);

            const size_t currentSize =
                mStringPool.size();


            if (currentSize >=
                static_cast<size_t>(kUnicodeStringOffsetInvalid))
            {
                outOffset = kUnicodeStringOffsetInvalid;
                return false;
            }


            const size_t maximumSize =
                static_cast<size_t>(
                    std::numeric_limits<uint32_t>::max());


            if (length >= maximumSize - currentSize)
            {
                outOffset = kUnicodeStringOffsetInvalid;
                return false;
            }


            const UnicodeStringOffset newOffset =
                static_cast<UnicodeStringOffset>(currentSize);

            const size_t oldSize =
                mStringPool.size();


            mStringPool.insert(
                mStringPool.end(),
                reinterpret_cast<const uint8_t*>(key),
                reinterpret_cast<const uint8_t*>(key) + length);

            mStringPool.push_back(0);


            if (!mStringOffsets.put(key, newOffset))
            {
                mStringPool.resize(oldSize);

                outOffset = kUnicodeStringOffsetInvalid;
                return false;
            }


            ++mStats.uniqueStrings;

            outOffset = newOffset;
            return true;
        }


        // ====================================================================
        // Blocks
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

            record.first = first;
            record.last = last;
            record.nameOffset = nameOffset;
            record.coverageIndex = coverageIndex;


            const uint32_t newIndex =
                static_cast<uint32_t>(mBlocks.size());


            mBlocks.push_back(record);

            outIndex = newIndex;

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

            record.nameOffset = nameOffset;
            record.iso15924Offset = iso15924Offset;
            record.coverageIndex = coverageIndex;
            record.reserved = 0;


            const uint32_t newIndex =
                static_cast<uint32_t>(mScripts.size());


            mScripts.push_back(record);

            outIndex = newIndex;

            return true;
        }


        // ====================================================================
        // Binary SET properties
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

            record.nameOffset = nameOffset;
            record.coverageIndex = coverageIndex;
            record.source = static_cast<uint16_t>(source);
            record.reserved0 = 0;
            record.reserved1 = 0;


            const uint32_t newIndex =
                static_cast<uint32_t>(mProperties.size());


            mProperties.push_back(record);

            outIndex = newIndex;

            return true;
        }


        // ====================================================================
        // Optional reserve helpers
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


        bool reserveValueTables8(size_t count)
        {
            if (count >=
                static_cast<size_t>(kUnicodeValueTable8IndexInvalid))
            {
                return false;
            }


            if (!mValueTable8Hashes.reserve(count))
                return false;


            mValueTables8.reserve(count);

            return true;
        }


        void reserveValueProperties8(size_t count)
        {
            mValueProperties8.reserve(count);
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
        // UnicodeDatabaseWriter consumes these directly.
        // ====================================================================

        [[nodiscard]]
        const std::vector<UnicodeCoverageData>& coverages() const noexcept
        {
            return mCoverages;
        }


        [[nodiscard]]
        const std::vector<UnicodeValueTable8Data>& valueTables8() const noexcept
        {
            return mValueTables8;
        }


        [[nodiscard]]
        const std::vector<UnicodeValueProperty8Record>& valueProperties8() const noexcept
        {
            return mValueProperties8;
        }


        // ====================================================================
        // Canonical decomposition contents
        // ====================================================================

        [[nodiscard]]
        bool hasDecomposition() const noexcept
        {
            return mHasDecomposition;
        }


        [[nodiscard]]
        const UnicodeDecompositionData& decomposition() const noexcept
        {
            return mDecomposition;
        }


        [[nodiscard]]
        const std::vector<UnicodeDecompositionRecord>& decompositionRecords() const noexcept
        {
            return mDecompositionRecords;
        }


        // ====================================================================
        // Canonical composition contents
        // ====================================================================

        [[nodiscard]]
        bool hasComposition() const noexcept
        {
            return mHasComposition;
        }


        [[nodiscard]]
        const std::vector<UnicodeCompositionRecord>& compositionRecords() const noexcept
        {
            return mCompositionRecords;
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
        const UnicodeValueTable8Data* valueTable8Data() const noexcept
        {
            return mValueTables8.empty()
                ? nullptr
                : mValueTables8.data();
        }


        [[nodiscard]]
        const UnicodeValueProperty8Record* valueProperty8Data() const noexcept
        {
            return mValueProperties8.empty()
                ? nullptr
                : mValueProperties8.data();
        }


        [[nodiscard]]
        const UnicodeDecompositionData* decompositionData() const noexcept
        {
            return mHasDecomposition
                ? &mDecomposition
                : nullptr;
        }


        [[nodiscard]]
        const UnicodeDecompositionRecord* decompositionRecordData() const noexcept
        {
            return mDecompositionRecords.empty()
                ? nullptr
                : mDecompositionRecords.data();
        }


        [[nodiscard]]
        const UnicodeCompositionRecord* compositionRecordData() const noexcept
        {
            return mCompositionRecords.empty()
                ? nullptr
                : mCompositionRecords.data();
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
        size_t valueTable8Count() const noexcept
        {
            return mValueTables8.size();
        }


        [[nodiscard]]
        size_t valueProperty8Count() const noexcept
        {
            return mValueProperties8.size();
        }


        [[nodiscard]]
        size_t decompositionRecordCount() const noexcept
        {
            return mDecompositionRecords.size();
        }


        [[nodiscard]]
        size_t compositionRecordCount() const noexcept
        {
            return mCompositionRecords.size();
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
        UnicodeValuePagePoolBuilder8 mValuePagePool8;
        UnicodeDecompositionPagePoolBuilder mDecompositionPagePool;

        std::vector<UnicodeCoverageData> mCoverages;

        std::vector<UnicodeValueTable8Data> mValueTables8;
        std::vector<UnicodeValueProperty8Record> mValueProperties8;

        UnicodeDecompositionData mDecomposition{};
        std::vector<UnicodeDecompositionRecord> mDecompositionRecords;
        bool mHasDecomposition{ false };

        std::vector<UnicodeCompositionRecord> mCompositionRecords;
        bool mHasComposition{ false };

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
        ValueTable8HashMap mValueTable8Hashes;
        StringOffsetMap mStringOffsets;

        UnicodeDatabaseBuilderStats mStats{};


        // ====================================================================
        // initializeStringPool
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
        // VALUE8 table validation
        // ====================================================================

        [[nodiscard]]
        bool validValueTable8Index(UnicodeValueTable8Index index) const noexcept
        {
            return
                index != kUnicodeValueTable8IndexInvalid &&
                index < mValueTables8.size();
        }


        // ====================================================================
        // Canonical-decomposition record validation
        // ====================================================================

        [[nodiscard]]
        static bool validateDecompositionRecords(
            const std::vector<UnicodeDecompositionRecord>& records) noexcept
        {
            for (const UnicodeDecompositionRecord& record : records)
            {
                if (record.first >= kUnicodeLimit)
                    return false;


                if (record.second != kUnicodeDecompositionSecondNone &&
                    record.second >= kUnicodeLimit)
                {
                    return false;
                }
            }


            return true;
        }


        // ====================================================================
        // Canonical-decomposition hierarchy validation
        //
        // Validate all persistent references which will eventually be written
        // into the decomposition section:
        //
        //      root -> master pages
        //      master pages -> leaf pages
        //      leaf pages -> decomposition records
        //
        // The decomposition page pool is dedicated to this one canonical
        // decomposition dataset, so all physical pages in the pool are
        // validated, not merely those encountered while walking the root.
        // ====================================================================

        [[nodiscard]]
        bool validateDecompositionHierarchy(const UnicodeDecompositionData& data,
            size_t recordCount) const noexcept
        {
            // ---------------------------------------------------------------
            // Root -> master pages
            // ---------------------------------------------------------------

            for (uint32_t mi = 0; mi < kUnicodeMasterCount; ++mi)
            {
                const UnicodeDecompositionMasterPageRef masterRef =
                    data.masters[mi];


                if (masterRef == kUnicodeDecompositionPageEmpty)
                    continue;


                if (masterRef >= mDecompositionPagePool.masterPageCount())
                    return false;
            }


            // ---------------------------------------------------------------
            // Master pages -> leaf pages
            // ---------------------------------------------------------------

            for (const UnicodeDecompositionMasterPage& master :
                mDecompositionPagePool.masterPages())
            {
                for (uint32_t si = 0; si < kUnicodeSubsPerMaster; ++si)
                {
                    const UnicodeDecompositionPageRef pageRef =
                        master.sub[si];


                    if (pageRef == kUnicodeDecompositionPageEmpty)
                        continue;


                    if (pageRef >= mDecompositionPagePool.pageCount())
                        return false;
                }
            }


            // ---------------------------------------------------------------
            // Leaf pages -> decomposition records
            // ---------------------------------------------------------------

            for (const UnicodeDecompositionPage& page :
                mDecompositionPagePool.pages())
            {
                for (uint32_t i = 0; i < kUnicodeSubSize; ++i)
                {
                    const UnicodeDecompositionRecordRef recordRef =
                        page.mapping[i];


                    if (recordRef == kUnicodeDecompositionRecordNone)
                        continue;


                    if (unicodeDecompositionRecordIndex(recordRef) >= recordCount)
                        return false;
                }
            }


            return true;
        }


        // ====================================================================
        // Canonical-composition record validation
        //
        // Persistent records must:
        //
        //      - contain only Unicode code points
        //      - be strictly ordered by (first, second)
        //
        // Strict ordering simultaneously guarantees unique pair keys, so a
        // persistent table cannot contain either duplicate or conflicting
        // composition mappings.
        // ====================================================================

        [[nodiscard]]
        static bool validateCompositionRecords(
            const std::vector<UnicodeCompositionRecord>& records) noexcept
        {
            for (size_t i = 0; i < records.size(); ++i)
            {
                const UnicodeCompositionRecord& record =
                    records[i];

                if (record.first >= kUnicodeLimit ||
                    record.second >= kUnicodeLimit ||
                    record.composite >= kUnicodeLimit)
                {
                    return false;
                }

                if (i == 0)
                    continue;

                const UnicodeCompositionRecord& previous =
                    records[i - 1];

                if (previous.first > record.first)
                    return false;

                if (previous.first == record.first &&
                    previous.second >= record.second)
                {
                    return false;
                }
            }

            return true;
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
        // Persistent root equality
        //
        // UnicodeCoverageData and UnicodeValueTable8Data are both exactly
        // 68 bytes with no padding.
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


        [[nodiscard]]
        static bool valueTables8Equal(const UnicodeValueTable8Data& a,
            const UnicodeValueTable8Data& b) noexcept
        {
            return
                std::memcmp(
                    &a,
                    &b,
                    sizeof(UnicodeValueTable8Data)) == 0;
        }


        // ====================================================================
        // hashFixedRecord68
        //
        // Both current deduplicated root structures are exactly 68 bytes:
        //
        //      UnicodeCoverageData
        //      UnicodeValueTable8Data
        //
        //      8 complete uint64_t chunks = 64 bytes
        //      1 uint32_t tail           =  4 bytes
        //
        // The hash is generator-only state and is not part of the persistent
        // database ABI.
        // ====================================================================

        template <typename T>
        [[nodiscard]]
        static uint64_t hashFixedRecord68(const T& value) noexcept
        {
            static_assert(
                sizeof(T) == 68,
                "hashFixedRecord68 requires an exact 68-byte record");


            const uint8_t* bytes =
                reinterpret_cast<const uint8_t*>(&value);


            uint64_t hash =
                0x9E3779B97F4A7C15ull;


            constexpr size_t wordCount =
                sizeof(T) /
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


            constexpr size_t consumed =
                wordCount *
                sizeof(uint64_t);


            static_assert(
                sizeof(T) - consumed == sizeof(uint32_t),
                "68-byte record hash expects a four-byte tail");


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