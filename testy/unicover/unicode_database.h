#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "core_nametable.h"
#include "lang_span.h"

#include "unicode_coverage.h"
#include "unicode_database_format.h"


namespace waavs
{
    // ========================================================================
    // UnicodeDatabase
    //
    // Read-only runtime view of a generated Unicode database.
    //
    // The database owns no memory.  The ByteSpan supplied to reset() must
    // remain valid for the lifetime of this object.
    //
    // reset() performs structural validation once and establishes pointers
    // directly into the database memory.
    //
    // After successful initialization:
    //
    //      UnicodeCoverageData[]
    //              |
    //              v
    //      UnicodeMasterPage[]
    //              |
    //              v
    //      UnicodeBitPage[]
    //
    // are used directly from the database without reconstruction.
    //
    // ========================================================================

    class UnicodeDatabase
    {
    public:
        UnicodeDatabase() noexcept = default;


        explicit UnicodeDatabase(const ByteSpan& data) noexcept
        {
            reset(data);
        }


        // ====================================================================
        // State
        // ====================================================================

        [[nodiscard]]
        bool valid() const noexcept
        {
            return mHeader != nullptr;
        }


        explicit operator bool() const noexcept
        {
            return valid();
        }


        void clear() noexcept
        {
            mData.reset();

            mHeader = nullptr;

            mCoverages = nullptr;
            mBlocks = nullptr;
            mScripts = nullptr;
            mProperties = nullptr;

            mStringPool = nullptr;
            mStringPoolSize = 0;

            mPools = {};
        }


        // ====================================================================
        // reset
        //
        // Validate and attach to a Unicode database.
        //
        // The supplied memory remains owned by the caller.
        //
        // ====================================================================

        bool reset(const ByteSpan& data) noexcept
        {
            clear();


            // ---------------------------------------------------------------
            // Basic size / architecture checks
            // ---------------------------------------------------------------

            if (data.size() < sizeof(UnicodeDatabaseHeader))
                return false;

            if (!hostIsLittleEndian())
                return false;

            if (!pointerIsAligned(data.data(), alignof(UnicodeDatabaseHeader)))
                return false;


            // ---------------------------------------------------------------
            // Signature
            // ---------------------------------------------------------------

            if (std::memcmp(
                data.data(),
                kUnicodeDatabaseSignature,
                sizeof(kUnicodeDatabaseSignature)) != 0)
            {
                return false;
            }


            const auto* header =
                reinterpret_cast<const UnicodeDatabaseHeader*>(
                    data.data());


            // ---------------------------------------------------------------
            // Format version
            // ---------------------------------------------------------------

            if (header->formatMajor != kUnicodeDatabaseFormatMajor)
                return false;

            if (header->formatMinor > kUnicodeDatabaseFormatMinor)
                return false;


            // ---------------------------------------------------------------
            // Header / database sizes
            // ---------------------------------------------------------------

            if (header->headerSize != sizeof(UnicodeDatabaseHeader))
                return false;

            if (header->databaseSize < header->headerSize)
                return false;

            if (header->databaseSize > data.size())
                return false;


            const size_t databaseSize =
                static_cast<size_t>(header->databaseSize);


            // ---------------------------------------------------------------
            // Endian marker
            // ---------------------------------------------------------------

            if (header->byteOrder != kUnicodeDatabaseByteOrder)
                return false;


            // Version 1 currently defines no flags.
            if (header->flags != 0)
                return false;


            // ---------------------------------------------------------------
            // Reserved header fields must be zero.
            // ---------------------------------------------------------------

            if (header->reserved0 != 0)
                return false;

            for (size_t i = 0; i < 10; ++i) {
                if (header->reserved[i] != 0)
                    return false;
            }


            // ---------------------------------------------------------------
            // Validate typed sections
            // ---------------------------------------------------------------

            if (!sectionValid<UnicodeCoverageData>(
                data.data(),
                databaseSize,
                header->coverageOffset,
                header->coverageCount))
            {
                return false;
            }

            if (!sectionValid<UnicodeMasterPage>(
                data.data(),
                databaseSize,
                header->masterPageOffset,
                header->masterPageCount))
            {
                return false;
            }

            if (!sectionValid<UnicodeBitPage>(
                data.data(),
                databaseSize,
                header->bitPageOffset,
                header->bitPageCount))
            {
                return false;
            }

            if (!sectionValid<UnicodeBlockRecord>(
                data.data(),
                databaseSize,
                header->blockOffset,
                header->blockCount))
            {
                return false;
            }

            if (!sectionValid<UnicodeScriptRecord>(
                data.data(),
                databaseSize,
                header->scriptOffset,
                header->scriptCount))
            {
                return false;
            }

            if (!sectionValid<UnicodePropertyRecord>(
                data.data(),
                databaseSize,
                header->propertyOffset,
                header->propertyCount))
            {
                return false;
            }


            // ---------------------------------------------------------------
            // Validate string pool
            // ---------------------------------------------------------------

            if (!byteSectionValid(
                databaseSize,
                header->stringPoolOffset,
                header->stringPoolSize))
            {
                return false;
            }


            const uint8_t* base =
                data.data();


            const auto* coverages =
                reinterpret_cast<const UnicodeCoverageData*>(
                    base + header->coverageOffset);

            const auto* masterPages =
                reinterpret_cast<const UnicodeMasterPage*>(
                    base + header->masterPageOffset);

            const auto* bitPages =
                reinterpret_cast<const UnicodeBitPage*>(
                    base + header->bitPageOffset);

            const auto* blocks =
                reinterpret_cast<const UnicodeBlockRecord*>(
                    base + header->blockOffset);

            const auto* scripts =
                reinterpret_cast<const UnicodeScriptRecord*>(
                    base + header->scriptOffset);

            const auto* properties =
                reinterpret_cast<const UnicodePropertyRecord*>(
                    base + header->propertyOffset);

            const uint8_t* stringPool =
                base + header->stringPoolOffset;


            // ---------------------------------------------------------------
            // Pool-count bounds imposed by 16-bit references
            // ---------------------------------------------------------------

            if (header->masterPageCount > kUnicodeMaxPoolPages)
                return false;

            if (header->bitPageCount > kUnicodeMaxPoolPages)
                return false;


            // ---------------------------------------------------------------
            // Validate coverage -> MasterPage references
            // ---------------------------------------------------------------

            if (!validateCoverages(
                coverages,
                header->coverageCount,
                header->masterPageCount))
            {
                return false;
            }


            // ---------------------------------------------------------------
            // Validate MasterPage -> BitPage references and masks
            // ---------------------------------------------------------------

            if (!validateMasterPages(
                masterPages,
                header->masterPageCount,
                header->bitPageCount))
            {
                return false;
            }


            // ---------------------------------------------------------------
            // Validate metadata records
            // ---------------------------------------------------------------

            if (!validateBlocks(
                blocks,
                header->blockCount,
                header->coverageCount,
                stringPool,
                header->stringPoolSize))
            {
                return false;
            }

            if (!validateScripts(
                scripts,
                header->scriptCount,
                header->coverageCount,
                stringPool,
                header->stringPoolSize))
            {
                return false;
            }

            if (!validateProperties(
                properties,
                header->propertyCount,
                header->coverageCount,
                stringPool,
                header->stringPoolSize))
            {
                return false;
            }


            // ---------------------------------------------------------------
            // Commit.
            //
            // No member pointers are exposed until every validation step has
            // succeeded.
            // ---------------------------------------------------------------

            mData = ByteSpan(
                data.data(),
                databaseSize);

            mHeader = header;

            mCoverages = coverages;
            mBlocks = blocks;
            mScripts = scripts;
            mProperties = properties;

            mStringPool = stringPool;
            mStringPoolSize =
                header->stringPoolSize;

            mPools.masterPages = masterPages;
            mPools.bitPages = bitPages;

            mPools.masterPageCount =
                header->masterPageCount;

            mPools.bitPageCount =
                header->bitPageCount;


            return true;
        }


        // ====================================================================
        // Database information
        // ====================================================================

        [[nodiscard]]
        const UnicodeDatabaseHeader* header() const noexcept
        {
            return mHeader;
        }


        [[nodiscard]]
        const ByteSpan& data() const noexcept
        {
            return mData;
        }


        [[nodiscard]]
        uint16_t unicodeMajor() const noexcept
        {
            return mHeader
                ? mHeader->unicodeMajor
                : 0;
        }


        [[nodiscard]]
        uint16_t unicodeMinor() const noexcept
        {
            return mHeader
                ? mHeader->unicodeMinor
                : 0;
        }


        [[nodiscard]]
        uint16_t unicodePatch() const noexcept
        {
            return mHeader
                ? mHeader->unicodePatch
                : 0;
        }


        // ====================================================================
        // Counts
        // ====================================================================

        [[nodiscard]]
        uint32_t coverageCount() const noexcept
        {
            return mHeader
                ? mHeader->coverageCount
                : 0;
        }


        [[nodiscard]]
        uint32_t masterPageCount() const noexcept
        {
            return mHeader
                ? mHeader->masterPageCount
                : 0;
        }


        [[nodiscard]]
        uint32_t bitPageCount() const noexcept
        {
            return mHeader
                ? mHeader->bitPageCount
                : 0;
        }


        [[nodiscard]]
        uint32_t blockCount() const noexcept
        {
            return mHeader
                ? mHeader->blockCount
                : 0;
        }


        [[nodiscard]]
        uint32_t scriptCount() const noexcept
        {
            return mHeader
                ? mHeader->scriptCount
                : 0;
        }


        [[nodiscard]]
        uint32_t propertyCount() const noexcept
        {
            return mHeader
                ? mHeader->propertyCount
                : 0;
        }


        // ====================================================================
        // Coverage
        // ====================================================================

        [[nodiscard]]
        UnicodeCoverage coverage(
            UnicodeCoverageIndex index) const noexcept
        {
            if (!mHeader ||
                index >= mHeader->coverageCount)
            {
                return {};
            }

            return UnicodeCoverage(
                &mCoverages[index],
                &mPools);
        }


        [[nodiscard]]
        const UnicodeCoverageData* coverageData(
            UnicodeCoverageIndex index) const noexcept
        {
            if (!mHeader ||
                index >= mHeader->coverageCount)
            {
                return nullptr;
            }

            return &mCoverages[index];
        }


        // ====================================================================
        // Blocks
        // ====================================================================

        [[nodiscard]]
        const UnicodeBlockRecord* blockRecord(
            uint32_t index) const noexcept
        {
            if (!mHeader ||
                index >= mHeader->blockCount)
            {
                return nullptr;
            }

            return &mBlocks[index];
        }


        [[nodiscard]]
        UnicodeCoverage blockCoverage(
            uint32_t index) const noexcept
        {
            const UnicodeBlockRecord* record =
                blockRecord(index);

            if (!record)
                return {};

            return coverage(
                record->coverageIndex);
        }


        // ====================================================================
        // Scripts
        // ====================================================================

        [[nodiscard]]
        const UnicodeScriptRecord* scriptRecord(
            uint32_t index) const noexcept
        {
            if (!mHeader ||
                index >= mHeader->scriptCount)
            {
                return nullptr;
            }

            return &mScripts[index];
        }


        [[nodiscard]]
        UnicodeCoverage scriptCoverage(
            uint32_t index) const noexcept
        {
            const UnicodeScriptRecord* record =
                scriptRecord(index);

            if (!record)
                return {};

            return coverage(
                record->coverageIndex);
        }


        // ====================================================================
        // Properties
        // ====================================================================

        [[nodiscard]]
        const UnicodePropertyRecord* propertyRecord(
            uint32_t index) const noexcept
        {
            if (!mHeader ||
                index >= mHeader->propertyCount)
            {
                return nullptr;
            }

            return &mProperties[index];
        }


        [[nodiscard]]
        UnicodeCoverage propertyCoverage(
            uint32_t index) const noexcept
        {
            const UnicodePropertyRecord* record =
                propertyRecord(index);

            if (!record)
                return {};

            return coverage(
                record->coverageIndex);
        }


        // ====================================================================
        // Database strings
        //
        // The persistent format stores UTF-8 NUL-terminated strings.
        //
        // stringSpan() provides a non-owning view.
        //
        // internString() converts one into the project's canonical
        // InternedKey representation.
        // ====================================================================

        [[nodiscard]]
        ByteSpan stringSpan(UnicodeStringOffset offset) const noexcept
        {
            if (!mStringPool ||
                offset == kUnicodeStringOffsetInvalid ||
                offset >= mStringPoolSize)
            {
                return {};
            }


            const uint8_t* begin = mStringPool + offset;
            const size_t remaining = mStringPoolSize - offset;

            const void* found =
                std::memchr(
                    begin,
                    0,
                    remaining);

            if (!found)
                return {};


            const uint8_t* end = static_cast<const uint8_t*>(found);

            return ByteSpan::fromPointers(begin, end);
        }


        [[nodiscard]]
        InternedKey internString(UnicodeStringOffset offset) const
        {
            const ByteSpan value =
                stringSpan(offset);

            if (!value)
                return nullptr;

            return WSNameSet::INTERN(value);
        }


        // ====================================================================
        // Names
        // ====================================================================

        [[nodiscard]]
        InternedKey blockName( uint32_t index) const
        {
            const UnicodeBlockRecord* record =
                blockRecord(index);

            return record
                ? internString(record->nameOffset)
                : nullptr;
        }


        [[nodiscard]]
        InternedKey scriptName( uint32_t index) const
        {
            const UnicodeScriptRecord* record =
                scriptRecord(index);

            return record
                ? internString(record->nameOffset)
                : nullptr;
        }


        [[nodiscard]]
        InternedKey scriptISO15924( uint32_t index) const
        {
            const UnicodeScriptRecord* record =
                scriptRecord(index);

            return record
                ? internString(record->iso15924Offset)
                : nullptr;
        }


        [[nodiscard]]
        InternedKey propertyName(uint32_t index) const
        {
            const UnicodePropertyRecord* record =
                propertyRecord(index);

            return record
                ? internString(record->nameOffset)
                : nullptr;
        }


        // ====================================================================
        // Shared coverage pools
        // ====================================================================

        [[nodiscard]]
        const UnicodeCoveragePools& pools() const noexcept
        {
            return mPools;
        }


    private:
        ByteSpan mData{};

        const UnicodeDatabaseHeader* mHeader{ nullptr };

        const UnicodeCoverageData* mCoverages{ nullptr };

        const UnicodeBlockRecord* mBlocks{ nullptr };
        const UnicodeScriptRecord* mScripts{ nullptr };
        const UnicodePropertyRecord* mProperties{ nullptr };

        const uint8_t* mStringPool{ nullptr };
        uint32_t mStringPoolSize{ 0 };

        UnicodeCoveragePools mPools{};


        // ====================================================================
        // Architecture helpers
        // ====================================================================

        [[nodiscard]]
        static bool hostIsLittleEndian() noexcept
        {
            const uint16_t value = 1;

            return
                *reinterpret_cast<
                const unsigned char*>(&value) == 1;
        }


        [[nodiscard]]
        static bool pointerIsAligned(
            const void* ptr,
            size_t alignment) noexcept
        {
            if (!ptr || alignment == 0)
                return false;

            const uintptr_t address =
                reinterpret_cast<uintptr_t>(ptr);

            return
                (address % alignment) == 0;
        }


        // ====================================================================
        // Section validation
        // ====================================================================

        template <typename T>
        [[nodiscard]]
        static bool sectionValid(
            const uint8_t* base,
            size_t databaseSize,
            uint32_t offset,
            uint32_t count) noexcept
        {
            if (count == 0)
                return offset <= databaseSize;


            if (offset > databaseSize)
                return false;


            const size_t remaining =
                databaseSize - offset;


            if (static_cast<size_t>(count) >
                remaining / sizeof(T))
            {
                return false;
            }


            const void* section =
                base + offset;


            return pointerIsAligned(
                section,
                alignof(T));
        }


        [[nodiscard]]
        static bool byteSectionValid(
            size_t databaseSize,
            uint32_t offset,
            uint32_t size) noexcept
        {
            if (offset > databaseSize)
                return false;

            return
                static_cast<size_t>(size) <=
                databaseSize - offset;
        }


        // ====================================================================
        // String validation
        // ====================================================================

        [[nodiscard]]
        static bool stringOffsetValid(
            UnicodeStringOffset offset,
            const uint8_t* stringPool,
            uint32_t stringPoolSize) noexcept
        {
            if (!stringPool)
                return false;

            if (offset ==
                kUnicodeStringOffsetInvalid)
            {
                return false;
            }

            if (offset >= stringPoolSize)
                return false;


            const uint8_t* begin =
                stringPool + offset;

            const size_t remaining =
                stringPoolSize - offset;


            return std::memchr(
                begin,
                0,
                remaining) != nullptr;
        }


        // ====================================================================
        // Coverage validation
        // ====================================================================

        [[nodiscard]]
        static bool validateCoverages(
            const UnicodeCoverageData* coverages,
            uint32_t coverageCount,
            uint32_t masterPageCount) noexcept
        {
            for (uint32_t ci = 0;
                ci < coverageCount;
                ++ci)
            {
                const UnicodeCoverageData& coverage =
                    coverages[ci];


                for (uint32_t mi = 0;
                    mi < kUnicodeMasterCount;
                    ++mi)
                {
                    const UnicodeMasterPageRef ref =
                        coverage.masters[mi];


                    if (ref == kUnicodePageEmpty ||
                        ref == kUnicodePageFull)
                    {
                        continue;
                    }


                    if (ref >= masterPageCount)
                        return false;
                }
            }


            return true;
        }


        // ====================================================================
        // Master-page validation
        //
        // The summary masks are relied upon by containsAll() and intersects(),
        // so they must agree exactly with sub[].
        // ====================================================================

        [[nodiscard]]
        static bool validateMasterPages(
            const UnicodeMasterPage* pages,
            uint32_t masterPageCount,
            uint32_t bitPageCount) noexcept
        {
            for (uint32_t pi = 0;
                pi < masterPageCount;
                ++pi)
            {
                const UnicodeMasterPage& page =
                    pages[pi];


                if ((page.fullMask &
                    ~page.nonEmptyMask) != 0)
                {
                    return false;
                }


                uint32_t expectedNonEmpty = 0;
                uint32_t expectedFull = 0;


                for (uint32_t si = 0;
                    si < kUnicodeSubsPerMaster;
                    ++si)
                {
                    const UnicodeBitPageRef ref =
                        page.sub[si];

                    const uint32_t mask =
                        uint32_t(1) << si;


                    if (ref == kUnicodePageEmpty)
                    {
                        continue;
                    }


                    expectedNonEmpty |= mask;


                    if (ref == kUnicodePageFull)
                    {
                        expectedFull |= mask;
                        continue;
                    }


                    if (ref >= bitPageCount)
                        return false;
                }


                if (page.nonEmptyMask !=
                    expectedNonEmpty)
                {
                    return false;
                }


                if (page.fullMask !=
                    expectedFull)
                {
                    return false;
                }


                for (size_t ri = 0;
                    ri < sizeof(page.reserved);
                    ++ri)
                {
                    if (page.reserved[ri] != 0)
                        return false;
                }
            }


            return true;
        }


        // ====================================================================
        // Block validation
        // ====================================================================

        [[nodiscard]]
        static bool validateBlocks(
            const UnicodeBlockRecord* records,
            uint32_t count,
            uint32_t coverageCount,
            const uint8_t* stringPool,
            uint32_t stringPoolSize) noexcept
        {
            uint32_t previousLast = 0;
            bool havePrevious = false;


            for (uint32_t i = 0;
                i < count;
                ++i)
            {
                const UnicodeBlockRecord& record =
                    records[i];


                if (record.first >
                    record.last)
                {
                    return false;
                }


                if (record.last >=
                    kUnicodeLimit)
                {
                    return false;
                }


                // Blocks should be ordered and non-overlapping.
                if (havePrevious &&
                    record.first <= previousLast)
                {
                    return false;
                }


                if (record.coverageIndex >=
                    coverageCount)
                {
                    return false;
                }


                if (!stringOffsetValid(
                    record.nameOffset,
                    stringPool,
                    stringPoolSize))
                {
                    return false;
                }


                previousLast = record.last;
                havePrevious = true;
            }


            return true;
        }


        // ====================================================================
        // Script validation
        // ====================================================================

        [[nodiscard]]
        static bool validateScripts(
            const UnicodeScriptRecord* records,
            uint32_t count,
            uint32_t coverageCount,
            const uint8_t* stringPool,
            uint32_t stringPoolSize) noexcept
        {
            for (uint32_t i = 0;
                i < count;
                ++i)
            {
                const UnicodeScriptRecord& record =
                    records[i];


                if (record.coverageIndex >=
                    coverageCount)
                {
                    return false;
                }


                if (!stringOffsetValid(
                    record.nameOffset,
                    stringPool,
                    stringPoolSize))
                {
                    return false;
                }


                if (!stringOffsetValid(
                    record.iso15924Offset,
                    stringPool,
                    stringPoolSize))
                {
                    return false;
                }


                if (record.reserved != 0)
                    return false;
            }


            return true;
        }


        // ====================================================================
        // Property validation
        // ====================================================================

        [[nodiscard]]
        static bool validateProperties(
            const UnicodePropertyRecord* records,
            uint32_t count,
            uint32_t coverageCount,
            const uint8_t* stringPool,
            uint32_t stringPoolSize) noexcept
        {
            for (uint32_t i = 0;
                i < count;
                ++i)
            {
                const UnicodePropertyRecord& record =
                    records[i];


                if (record.coverageIndex >=
                    coverageCount)
                {
                    return false;
                }


                if (!stringOffsetValid(
                    record.nameOffset,
                    stringPool,
                    stringPoolSize))
                {
                    return false;
                }


                if (record.source >
                    UnicodePropertySourceEmojiData)
                {
                    return false;
                }


                if (record.reserved0 != 0 ||
                    record.reserved1 != 0)
                {
                    return false;
                }
            }


            return true;
        }
    };

} // namespace waavs
