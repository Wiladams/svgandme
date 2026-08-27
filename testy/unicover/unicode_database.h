// unicode_database.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "core_nametable.h"
#include "lang_span.h"

#include "unicode_bidi_class.h"
#include "unicode_combining_class.h"
#include "unicode_composition.h"
#include "unicode_coverage.h"
#include "unicode_database_format.h"
#include "unicode_decomposition.h"
#include "unicode_general_category.h"
#include "unicode_grapheme_cluster_break.h"
#include "unicode_indic_conjunct_break.h"
#include "unicode_value_table8.h"


namespace waavs
{
    // ========================================================================
    // UnicodeDatabase
    //
    // Read-only runtime view of a generated Unicode database.
    //
    // The database owns no memory. The ByteSpan supplied to reset() must
    // remain valid for the lifetime of this object.
    //
    // reset() performs structural and semantic validation once and establishes
    // pointers directly into database memory.
    //
    //
    // SET storage:
    //
    //      UnicodeCoverageData[]
    //              |
    //              v
    //      UnicodeMasterPage[]
    //              |
    //              v
    //      UnicodeBitPage[]
    //
    //
    // VALUE8 storage:
    //
    //      UnicodeValueTable8Data[]
    //              |
    //              v
    //      UnicodeValueMasterPage8[]
    //              |
    //              v
    //      UnicodeValuePage8[]
    //
    // Canonical decomposition storage:
    //
    //      UnicodeDecompositionSection
    //              |
    //              v
    //      master pages -> leaf pages -> records
    //
    // Canonical composition storage:
    //
    //      UnicodeCompositionSection
    //              |
    //              v
    //      UnicodeCompositionRecord[]
    //
    //
    // After successful initialization all of these structures are interpreted
    // directly from database memory without reconstruction.
    //
    // Format 1.1 requires the core VALUE8 properties:
    //
    //      General_Category
    //      Canonical_Combining_Class
    //
    // Their table indices are resolved once during reset() and cached for
    // allocation-free scalar runtime access.
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

            mExtendedPictographicCoverage = kUnicodeCoverageIndexInvalid;

            mValueTables8 = nullptr;
            mValueProperties8 = nullptr;

            mDecompositionSection = nullptr;
            mDecompositionPools = {};

            mCompositionSection = nullptr;
            mCompositionRecords = nullptr;

            mStringPool = nullptr;
            mStringPoolSize = 0;

            mPools = {};
            mValuePools8 = {};

            mGeneralCategoryTable =
                kUnicodeValueTable8IndexInvalid;

            mCombiningClassTable =
                kUnicodeValueTable8IndexInvalid;

            mBidiClassTable = kUnicodeValueTable8IndexInvalid;
            mGraphemeClusterBreakTable = kUnicodeValueTable8IndexInvalid;
            mIndicConjunctBreakTable = kUnicodeValueTable8IndexInvalid;
        }


        // ====================================================================
        // reset
        //
        // Validate and attach to a Unicode database.
        //
        // No member pointers are committed until every structural and semantic
        // validation step has succeeded.
        //
        // Format 1.0:
        //
        //      Existing coverage/block/script/property database.
        //      VALUE8 fields must remain zero because they occupied reserved
        //      header space in that version.
        //
        // Format 1.1:
        //
        //      Adds VALUE8 sections and requires:
        //
        //          General_Category
        //          Canonical_Combining_Class
        //
        // Format 1.2:
        //
        //      Adds optional canonical-decomposition storage.
        //
        // Format 1.3:
        //
        //      Adds optional canonical-composition storage.
        //
        // ====================================================================

        bool reset(const ByteSpan& data) noexcept
        {
            clear();


            // ================================================================
            // Basic size / architecture checks
            // ================================================================

            if (data.size() < sizeof(UnicodeDatabaseHeader))
                return false;

            if (!hostIsLittleEndian())
                return false;

            if (!pointerIsAligned(
                data.data(),
                alignof(UnicodeDatabaseHeader)))
            {
                return false;
            }


            // ================================================================
            // Signature
            // ================================================================

            if (std::memcmp( data.data(), kUnicodeDatabaseSignature,
                sizeof(kUnicodeDatabaseSignature)) != 0)
            {
                return false;
            }


            const auto* header = reinterpret_cast<const UnicodeDatabaseHeader*>( data.data());


            // ================================================================
            // Format version
            // ================================================================

            if (header->formatMajor !=
                kUnicodeDatabaseFormatMajor)
            {
                return false;
            }

            if (header->formatMinor >
                kUnicodeDatabaseFormatMinor)
            {
                return false;
            }


            // ================================================================
            // Header / database sizes
            // ================================================================

            if (header->headerSize !=
                sizeof(UnicodeDatabaseHeader))
            {
                return false;
            }

            if (header->databaseSize <
                header->headerSize)
            {
                return false;
            }

            if (header->databaseSize >
                data.size())
            {
                return false;
            }


            const size_t databaseSize =
                static_cast<size_t>(
                    header->databaseSize);


            // ================================================================
            // Endian marker
            // ================================================================

            if (header->byteOrder !=
                kUnicodeDatabaseByteOrder)
            {
                return false;
            }


            // Version 1 currently defines no flags.
            if (header->flags != 0)
                return false;


            // ================================================================
            // Version-specific extension fields
            //
            // In older format revisions these header words were not yet
            // defined. A database claiming an older revision must therefore
            // leave the corresponding offsets zero.
            // ================================================================

            if (header->formatMinor < 2 &&
                header->decompositionOffset != 0)
            {
                return false;
            }

            if (header->formatMinor < 3 &&
                header->compositionOffset != 0)
            {
                return false;
            }


            // ---------------------------------------------------------------
            // In format 1.0 the bytes now occupied by the VALUE8 section
            // descriptors were not defined and therefore must remain zero.
            // ---------------------------------------------------------------

            if (header->formatMinor == 0)
            {
                if (header->valueProperty8Offset != 0 ||
                    header->valueProperty8Count != 0 ||
                    header->valueTable8Offset != 0 ||
                    header->valueTable8Count != 0 ||
                    header->valueMasterPage8Offset != 0 ||
                    header->valueMasterPage8Count != 0 ||
                    header->valuePage8Offset != 0 ||
                    header->valuePage8Count != 0)
                {
                    return false;
                }
            }


            // ================================================================
            // Validate existing typed sections
            // ================================================================

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


            // ================================================================
            // Validate VALUE8 typed sections
            // ================================================================

            if (!sectionValid<UnicodeValueProperty8Record>(
                data.data(),
                databaseSize,
                header->valueProperty8Offset,
                header->valueProperty8Count))
            {
                return false;
            }

            if (!sectionValid<UnicodeValueTable8Data>(
                data.data(),
                databaseSize,
                header->valueTable8Offset,
                header->valueTable8Count))
            {
                return false;
            }

            if (!sectionValid<UnicodeValueMasterPage8>(
                data.data(),
                databaseSize,
                header->valueMasterPage8Offset,
                header->valueMasterPage8Count))
            {
                return false;
            }

            if (!sectionValid<UnicodeValuePage8>(
                data.data(),
                databaseSize,
                header->valuePage8Offset,
                header->valuePage8Count))
            {
                return false;
            }


            // ================================================================
            // Validate string pool
            // ================================================================

            if (!byteSectionValid(
                databaseSize,
                header->stringPoolOffset,
                header->stringPoolSize))
            {
                return false;
            }


            // ================================================================
            // Establish local section pointers.
            //
            // These are not committed to the object until validation completes.
            // ================================================================

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


            const auto* valueProperties8 =
                reinterpret_cast<const UnicodeValueProperty8Record*>(
                    base + header->valueProperty8Offset);

            const auto* valueTables8 =
                reinterpret_cast<const UnicodeValueTable8Data*>(
                    base + header->valueTable8Offset);

            const auto* valueMasterPages8 =
                reinterpret_cast<const UnicodeValueMasterPage8*>(
                    base + header->valueMasterPage8Offset);

            const auto* valuePages8 =
                reinterpret_cast<const UnicodeValuePage8*>(
                    base + header->valuePage8Offset);


            const uint8_t* stringPool =
                base + header->stringPoolOffset;


            // ================================================================
            // Canonical decomposition
            //
            // Format 1.2 adds one optional descriptor. All offsets stored in
            // the descriptor are absolute database offsets.
            // ================================================================

            const UnicodeDecompositionSection* decompositionSection = nullptr;
            const UnicodeDecompositionMasterPage* decompositionMasterPages = nullptr;
            const UnicodeDecompositionPage* decompositionPages = nullptr;
            const UnicodeDecompositionRecord* decompositionRecords = nullptr;

            if (header->decompositionOffset != 0)
            {
                if (header->formatMinor < 2)
                    return false;

                if (!sectionValid<UnicodeDecompositionSection>(
                    base,
                    databaseSize,
                    header->decompositionOffset,
                    1))
                {
                    return false;
                }

                decompositionSection =
                    reinterpret_cast<const UnicodeDecompositionSection*>(
                        base + header->decompositionOffset);

                if (decompositionSection->masterPageCount >
                    kUnicodeDecompositionMaxPoolPages)
                {
                    return false;
                }

                if (decompositionSection->pageCount >
                    kUnicodeDecompositionMaxPoolPages)
                {
                    return false;
                }

                if (decompositionSection->recordCount >
                    kUnicodeDecompositionMaxRecords)
                {
                    return false;
                }

                if (!sectionValid<UnicodeDecompositionMasterPage>(
                    base,
                    databaseSize,
                    decompositionSection->masterPageOffset,
                    decompositionSection->masterPageCount))
                {
                    return false;
                }

                if (!sectionValid<UnicodeDecompositionPage>(
                    base,
                    databaseSize,
                    decompositionSection->pageOffset,
                    decompositionSection->pageCount))
                {
                    return false;
                }

                if (!sectionValid<UnicodeDecompositionRecord>(
                    base,
                    databaseSize,
                    decompositionSection->recordOffset,
                    decompositionSection->recordCount))
                {
                    return false;
                }

                decompositionMasterPages =
                    decompositionSection->masterPageCount != 0
                    ? reinterpret_cast<const UnicodeDecompositionMasterPage*>(
                        base + decompositionSection->masterPageOffset)
                    : nullptr;

                decompositionPages =
                    decompositionSection->pageCount != 0
                    ? reinterpret_cast<const UnicodeDecompositionPage*>(
                        base + decompositionSection->pageOffset)
                    : nullptr;

                decompositionRecords =
                    decompositionSection->recordCount != 0
                    ? reinterpret_cast<const UnicodeDecompositionRecord*>(
                        base + decompositionSection->recordOffset)
                    : nullptr;

                if (!validateDecomposition(
                    *decompositionSection,
                    decompositionMasterPages,
                    decompositionPages,
                    decompositionRecords))
                {
                    return false;
                }
            }


            // ================================================================
            // Canonical composition
            //
            // Format 1.3 adds one optional descriptor containing a directly
            // searchable, strictly ordered UnicodeCompositionRecord array.
            // ================================================================

            const UnicodeCompositionSection* compositionSection = nullptr;
            const UnicodeCompositionRecord* compositionRecords = nullptr;

            if (header->compositionOffset != 0)
            {
                if (header->formatMinor < 3)
                    return false;

                if (!sectionValid<UnicodeCompositionSection>(
                    base,
                    databaseSize,
                    header->compositionOffset,
                    1))
                {
                    return false;
                }

                compositionSection =
                    reinterpret_cast<const UnicodeCompositionSection*>(
                        base + header->compositionOffset);

                // A present composition section is meaningful only when it
                // contains at least one explicit pair. Hangul remains
                // algorithmic and is not represented here.
                if (compositionSection->recordCount == 0 ||
                    compositionSection->recordOffset == 0)
                {
                    return false;
                }

                if (!sectionValid<UnicodeCompositionRecord>(
                    base,
                    databaseSize,
                    compositionSection->recordOffset,
                    compositionSection->recordCount))
                {
                    return false;
                }

                compositionRecords =
                    reinterpret_cast<const UnicodeCompositionRecord*>(
                        base + compositionSection->recordOffset);

                if (!validateCompositionRecords(
                    compositionRecords,
                    compositionSection->recordCount))
                {
                    return false;
                }
            }


            // ================================================================
            // Pool-count bounds imposed by persistent references
            // ================================================================

            if (header->masterPageCount >
                kUnicodeMaxPoolPages)
            {
                return false;
            }

            if (header->bitPageCount >
                kUnicodeMaxPoolPages)
            {
                return false;
            }


            if (header->valueMasterPage8Count >
                kUnicodeValue8MaxPoolPages)
            {
                return false;
            }

            if (header->valuePage8Count >
                kUnicodeValue8MaxPoolPages)
            {
                return false;
            }


            // ================================================================
            // Validate SET hierarchy
            // ================================================================

            if (!validateCoverages(
                coverages,
                header->coverageCount,
                header->masterPageCount))
            {
                return false;
            }


            if (!validateMasterPages(
                masterPages,
                header->masterPageCount,
                header->bitPageCount))
            {
                return false;
            }


            // ================================================================
            // Validate VALUE8 hierarchy
            // ================================================================

            if (!validateValueTables8(
                valueTables8,
                header->valueTable8Count,
                header->valueMasterPage8Count))
            {
                return false;
            }


            if (!validateValueMasterPages8(
                valueMasterPages8,
                header->valueMasterPage8Count,
                header->valuePage8Count))
            {
                return false;
            }


            // ================================================================
            // Validate existing metadata records
            // ================================================================

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

            UnicodeCoverageIndex extendedPictographicCoverage =
                kUnicodeCoverageIndexInvalid;

            for (uint32_t i = 0; i < header->propertyCount; ++i)
            {
                const UnicodePropertyRecord& record = properties[i];

                if (record.source != UnicodePropertySourceEmojiData)
                    continue;

                if (!stringEquals(
                    record.nameOffset, stringPool, header->stringPoolSize,
                    "Extended_Pictographic"))
                {
                    continue;
                }

                if (extendedPictographicCoverage != kUnicodeCoverageIndexInvalid)
                    return false;

                extendedPictographicCoverage = record.coverageIndex;
            }
            // ================================================================
            // Validate VALUE8 property directory and resolve core tables.
            // ================================================================

            UnicodeValueTable8Index generalCategoryTable = kUnicodeValueTable8IndexInvalid;
            UnicodeValueTable8Index combiningClassTable = kUnicodeValueTable8IndexInvalid;
            UnicodeValueTable8Index bidiClassTable = kUnicodeValueTable8IndexInvalid;
            UnicodeValueTable8Index graphemeClusterBreakTable = kUnicodeValueTable8IndexInvalid;
            UnicodeValueTable8Index indicConjunctBreakTable = kUnicodeValueTable8IndexInvalid;


            // --------------------------------------------------------------------
            // VALUE8 is an optional database capability.
            //
            // If any VALUE8 storage is present, require the currently defined core
            // semantic properties:
            //
            //      General_Category
            //      Canonical_Combining_Class
            //
            // A database containing no VALUE8 data remains valid.
            // --------------------------------------------------------------------

            //const bool hasValue8 =
            //    header->valueProperty8Count != 0 ||
            //    header->valueTable8Count != 0 ||
            //    header->valueMasterPage8Count != 0 ||
            //    header->valuePage8Count != 0;


            if (!validateValueProperties8(valueProperties8, header->valueProperty8Count,
                header->valueTable8Count, header->formatMinor,
                generalCategoryTable, combiningClassTable,
                bidiClassTable, graphemeClusterBreakTable, 
                indicConjunctBreakTable))
            {
                return false;
            }


            // ================================================================
            // Semantic VALUE8 validation
            //
            // Structural reference validation alone cannot guarantee that a
            // table identified as General_Category contains valid enum values,
            // or that CCC avoids the invalid byte value 255.
            //
            // Validate the reachable contents once at database attachment.
            // ================================================================

            if (generalCategoryTable !=
                kUnicodeValueTable8IndexInvalid)
            {
                if (!validateValueTable8Maximum(
                    valueTables8[generalCategoryTable],
                    valueMasterPages8,
                    header->valueMasterPage8Count,
                    valuePages8,
                    header->valuePage8Count,
                    static_cast<uint8_t>(
                        kUnicodeGeneralCategoryCount - 1u)))
                {
                    return false;
                }
            }


            if (combiningClassTable !=
                kUnicodeValueTable8IndexInvalid)
            {
                if (!validateValueTable8Maximum(
                    valueTables8[combiningClassTable],
                    valueMasterPages8,
                    header->valueMasterPage8Count,
                    valuePages8,
                    header->valuePage8Count,
                    static_cast<uint8_t>(
                        kUnicodeCombiningClassMaximum)))
                {
                    return false;
                }
            }

            if (bidiClassTable != kUnicodeValueTable8IndexInvalid)
            {
                if (!validateValueTable8Maximum(
                    valueTables8[bidiClassTable],
                    valueMasterPages8,
                    header->valueMasterPage8Count,
                    valuePages8,
                    header->valuePage8Count,
                    static_cast<uint8_t>(
                        kUnicodeBidiClassCount - 1u)))
                {
                    return false;
                }
            }

            if (graphemeClusterBreakTable != kUnicodeValueTable8IndexInvalid)
            {
                if (!validateValueTable8Maximum(
                    valueTables8[graphemeClusterBreakTable],
                    valueMasterPages8, header->valueMasterPage8Count,
                    valuePages8, header->valuePage8Count,
                    static_cast<uint8_t>(kUnicodeGraphemeClusterBreakCount - 1u)))
                {
                    return false;
                }
            }


            if (indicConjunctBreakTable !=
                kUnicodeValueTable8IndexInvalid)
            {
                if (!validateValueTable8Maximum(
                    valueTables8[indicConjunctBreakTable],
                    valueMasterPages8,
                    header->valueMasterPage8Count,
                    valuePages8,
                    header->valuePage8Count,
                    static_cast<uint8_t>(
                        kUnicodeIndicConjunctBreakCount - 1u)))
                {
                    return false;
                }
            }

            // ================================================================
            // Commit
            //
            // No runtime member pointer becomes visible until every validation
            // step above has succeeded.
            // ================================================================

            mData = ByteSpan( data.data(), databaseSize);

            mHeader = header;


            // ---------------------------------------------------------------
            // SET
            // ---------------------------------------------------------------

            mCoverages = coverages;
            mBlocks = blocks;
            mScripts = scripts;
            mProperties = properties;
            mPools.masterPages = masterPages;
            mPools.bitPages = bitPages;
            mPools.masterPageCount = header->masterPageCount;
            mPools.bitPageCount = header->bitPageCount;

            mExtendedPictographicCoverage = extendedPictographicCoverage;


            // ---------------------------------------------------------------
            // VALUE8
            // ---------------------------------------------------------------

            mValueTables8 =
                header->valueTable8Count != 0
                ? valueTables8
                : nullptr;

            mValueProperties8 =
                header->valueProperty8Count != 0
                ? valueProperties8
                : nullptr;


            mValuePools8.masterPages =
                header->valueMasterPage8Count != 0
                ? valueMasterPages8
                : nullptr;

            mValuePools8.valuePages =
                header->valuePage8Count != 0
                ? valuePages8
                : nullptr;

            mValuePools8.masterPageCount =
                header->valueMasterPage8Count;

            mValuePools8.valuePageCount =
                header->valuePage8Count;


            mGeneralCategoryTable = generalCategoryTable;
            mCombiningClassTable = combiningClassTable;
            mBidiClassTable = bidiClassTable;
            mGraphemeClusterBreakTable = graphemeClusterBreakTable;
            mIndicConjunctBreakTable = indicConjunctBreakTable;

            // ---------------------------------------------------------------
            // Canonical decomposition
            // ---------------------------------------------------------------

            mDecompositionSection =
                decompositionSection;

            mDecompositionPools.masterPages =
                decompositionMasterPages;

            mDecompositionPools.pages =
                decompositionPages;

            mDecompositionPools.records =
                decompositionRecords;

            mDecompositionPools.masterPageCount =
                decompositionSection
                ? decompositionSection->masterPageCount
                : 0;

            mDecompositionPools.pageCount =
                decompositionSection
                ? decompositionSection->pageCount
                : 0;

            mDecompositionPools.recordCount =
                decompositionSection
                ? decompositionSection->recordCount
                : 0;


            // ---------------------------------------------------------------
            // Canonical composition
            // ---------------------------------------------------------------

            mCompositionSection =
                compositionSection;

            mCompositionRecords =
                compositionRecords;


            // ---------------------------------------------------------------
            // Strings
            // ---------------------------------------------------------------

            mStringPool =
                stringPool;

            mStringPoolSize =
                header->stringPoolSize;


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
        // SET counts
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


        // ====================================================================
        // Semantic directory counts
        // ====================================================================

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
        // VALUE8 counts
        // ====================================================================

        [[nodiscard]]
        uint32_t valueProperty8Count() const noexcept
        {
            return mHeader
                ? mHeader->valueProperty8Count
                : 0;
        }


        [[nodiscard]]
        uint32_t valueTable8Count() const noexcept
        {
            return mHeader
                ? mHeader->valueTable8Count
                : 0;
        }


        [[nodiscard]]
        uint32_t valueMasterPage8Count() const noexcept
        {
            return mHeader
                ? mHeader->valueMasterPage8Count
                : 0;
        }


        [[nodiscard]]
        uint32_t valuePage8Count() const noexcept
        {
            return mHeader
                ? mHeader->valuePage8Count
                : 0;
        }


        // ====================================================================
        // Canonical decomposition
        // ====================================================================

        [[nodiscard]]
        bool hasDecomposition() const noexcept
        {
            return mDecompositionSection != nullptr;
        }


        [[nodiscard]]
        uint32_t decompositionMasterPageCount() const noexcept
        {
            return mDecompositionSection
                ? mDecompositionSection->masterPageCount
                : 0;
        }


        [[nodiscard]]
        uint32_t decompositionPageCount() const noexcept
        {
            return mDecompositionSection
                ? mDecompositionSection->pageCount
                : 0;
        }


        [[nodiscard]]
        uint32_t decompositionRecordCount() const noexcept
        {
            return mDecompositionSection
                ? mDecompositionSection->recordCount
                : 0;
        }


        [[nodiscard]]
        UnicodeDecomposition decomposition() const noexcept
        {
            if (!mDecompositionSection)
                return {};

            return UnicodeDecomposition(
                &mDecompositionSection->data,
                &mDecompositionPools);
        }


        [[nodiscard]]
        const UnicodeDecompositionSection* decompositionSection() const noexcept
        {
            return mDecompositionSection;
        }


        [[nodiscard]]
        const UnicodeDecompositionData* decompositionData() const noexcept
        {
            return mDecompositionSection
                ? &mDecompositionSection->data
                : nullptr;
        }


        [[nodiscard]]
        const UnicodeDecompositionPools& decompositionPools() const noexcept
        {
            return mDecompositionPools;
        }


        // ====================================================================
        // Canonical composition
        // ====================================================================

        [[nodiscard]]
        bool hasComposition() const noexcept
        {
            return mCompositionSection != nullptr;
        }


        [[nodiscard]]
        uint32_t compositionRecordCount() const noexcept
        {
            return mCompositionSection
                ? mCompositionSection->recordCount
                : 0;
        }

        [[nodiscard]]
        UnicodeComposition composition() const noexcept
        {
            if (!mCompositionSection)
                return {};

            return UnicodeComposition(
                mCompositionRecords,
                mCompositionSection->recordCount);
        }

        [[nodiscard]]
        const UnicodeCompositionSection* compositionSection() const noexcept
        {
            return mCompositionSection;
        }


        [[nodiscard]]
        const UnicodeCompositionRecord* compositionRecords() const noexcept
        {
            return mCompositionRecords;
        }


        [[nodiscard]]
        const UnicodeCompositionRecord* compositionRecord(uint32_t index) const noexcept
        {
            if (!mCompositionSection ||
                index >= mCompositionSection->recordCount)
            {
                return nullptr;
            }

            return &mCompositionRecords[index];
        }


        // ====================================================================
        // Coverage
        // ====================================================================

        [[nodiscard]]
        UnicodeCoverage coverage(UnicodeCoverageIndex index) const noexcept
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
        const UnicodeCoverageData* coverageData(UnicodeCoverageIndex index) const noexcept
        {
            if (!mHeader ||
                index >= mHeader->coverageCount)
            {
                return nullptr;
            }


            return &mCoverages[index];
        }


        // ====================================================================
        // VALUE8 tables
        // ====================================================================

        [[nodiscard]]
        UnicodeValueTable8 valueTable8(
            UnicodeValueTable8Index index) const noexcept
        {
            if (!mHeader ||
                !mValueTables8 ||
                index >= mHeader->valueTable8Count)
            {
                return {};
            }


            return UnicodeValueTable8(
                &mValueTables8[index],
                &mValuePools8);
        }


        [[nodiscard]]
        const UnicodeValueTable8Data* valueTable8Data(
            UnicodeValueTable8Index index) const noexcept
        {
            if (!mHeader ||
                !mValueTables8 ||
                index >= mHeader->valueTable8Count)
            {
                return nullptr;
            }


            return &mValueTables8[index];
        }


        // ====================================================================
        // VALUE8 property directory
        // ====================================================================

        [[nodiscard]]
        const UnicodeValueProperty8Record* valueProperty8Record(
            uint32_t index) const noexcept
        {
            if (!mHeader ||
                !mValueProperties8 ||
                index >= mHeader->valueProperty8Count)
            {
                return nullptr;
            }


            return &mValueProperties8[index];
        }

        
        // ====================================================================
        // Core shaping-oriented VALUE8 properties
        // ====================================================================

        [[nodiscard]]
        bool hasBidiClass() const noexcept
        {
            return mBidiClassTable !=
                kUnicodeValueTable8IndexInvalid;
        }

        [[nodiscard]]
        bool hasGraphemeClusterBreak() const noexcept {
            return mGraphemeClusterBreakTable != kUnicodeValueTable8IndexInvalid;
        }

        [[nodiscard]]
        bool hasIndicConjunctBreak() const noexcept
        {
            return mIndicConjunctBreakTable !=
                kUnicodeValueTable8IndexInvalid;
        }

        [[nodiscard]]
        bool hasGeneralCategory() const noexcept
        {
            return mGeneralCategoryTable !=
                kUnicodeValueTable8IndexInvalid;
        }


        [[nodiscard]]
        bool hasCombiningClass() const noexcept
        {
            return mCombiningClassTable !=
                kUnicodeValueTable8IndexInvalid;
        }

        // Accessors
        [[nodiscard]]
        UnicodeGeneralCategory generalCategory(uint32_t cp) const noexcept
        {
            if (mGeneralCategoryTable ==
                kUnicodeValueTable8IndexInvalid)
            {
                return UnicodeGeneralCategory::Unassigned;
            }


            return static_cast<UnicodeGeneralCategory>(
                valueTable8(
                    mGeneralCategoryTable).value(cp));
        }


        [[nodiscard]]
        UnicodeCombiningClass combiningClass(uint32_t cp) const noexcept
        {
            if (mCombiningClassTable ==
                kUnicodeValueTable8IndexInvalid)
            {
                return kUnicodeCombiningClassNotReordered;
            }


            return
                valueTable8(
                    mCombiningClassTable).value(cp);
        }

        [[nodiscard]]
        UnicodeBidiClass bidiClass(uint32_t cp) const noexcept
        {
            if (mBidiClassTable ==
                kUnicodeValueTable8IndexInvalid)
            {
                return UnicodeBidiClass::LeftToRight;
            }


            return static_cast<UnicodeBidiClass>(
                valueTable8(
                    mBidiClassTable).value(cp));
        }

        [[nodiscard]]
        UnicodeGraphemeClusterBreak graphemeClusterBreak(uint32_t cp) const noexcept
        {
            if (mGraphemeClusterBreakTable == kUnicodeValueTable8IndexInvalid)
                return UnicodeGraphemeClusterBreak::Other;

            return static_cast<UnicodeGraphemeClusterBreak>(
                valueTable8(mGraphemeClusterBreakTable).value(cp));
        }

        [[nodiscard]]
        UnicodeIndicConjunctBreak indicConjunctBreak(uint32_t cp) const noexcept
        {
            if (mIndicConjunctBreakTable == kUnicodeValueTable8IndexInvalid)
            {
                return UnicodeIndicConjunctBreak::None;
            }

            return static_cast<UnicodeIndicConjunctBreak>(valueTable8(mIndicConjunctBreakTable).value(cp));
        }

        // ====================================================================
        // Blocks
        // ====================================================================

        [[nodiscard]]
        const UnicodeBlockRecord* blockRecord(uint32_t index) const noexcept
        {
            if (!mHeader ||
                index >= mHeader->blockCount)
            {
                return nullptr;
            }


            return &mBlocks[index];
        }


        [[nodiscard]]
        UnicodeCoverage blockCoverage(uint32_t index) const noexcept
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
        const UnicodeScriptRecord* scriptRecord(uint32_t index) const noexcept
        {
            if (!mHeader ||
                index >= mHeader->scriptCount)
            {
                return nullptr;
            }


            return &mScripts[index];
        }


        [[nodiscard]]
        UnicodeCoverage scriptCoverage(uint32_t index) const noexcept
        {
            const UnicodeScriptRecord* record =
                scriptRecord(index);


            if (!record)
                return {};


            return coverage(
                record->coverageIndex);
        }


        // ====================================================================
        // Binary SET properties
        // ====================================================================

        [[nodiscard]]
        const UnicodePropertyRecord* propertyRecord(uint32_t index) const noexcept
        {
            if (!mHeader ||
                index >= mHeader->propertyCount)
            {
                return nullptr;
            }


            return &mProperties[index];
        }


        [[nodiscard]]
        UnicodeCoverage propertyCoverage(uint32_t index) const noexcept
        {
            const UnicodePropertyRecord* record =
                propertyRecord(index);


            if (!record)
                return {};


            return coverage(
                record->coverageIndex);
        }

        // ==============================================
        // Extended Pictographic property coverage
        // ==============================================
        // 
        [[nodiscard]]
        bool hasExtendedPictographic() const noexcept {
            return mExtendedPictographicCoverage != kUnicodeCoverageIndexInvalid;
        }


        [[nodiscard]]
        UnicodeCoverage extendedPictographicCoverage() const noexcept
        {
            if (mExtendedPictographicCoverage == kUnicodeCoverageIndexInvalid)
                return {};

            return coverage(mExtendedPictographicCoverage);
        }


        [[nodiscard]]
        bool isExtendedPictographic(uint32_t cp) const noexcept
        {
            if (mExtendedPictographicCoverage == kUnicodeCoverageIndexInvalid)
                return false;

            return coverage(mExtendedPictographicCoverage).contains(cp);
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


            const uint8_t* begin =
                mStringPool + offset;

            const size_t remaining =
                mStringPoolSize - offset;


            const void* found =
                std::memchr(
                    begin,
                    0,
                    remaining);


            if (!found)
                return {};


            const uint8_t* end =
                static_cast<const uint8_t*>(
                    found);


            return ByteSpan::fromPointers(
                begin,
                end);
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
        InternedKey blockName(uint32_t index) const
        {
            const UnicodeBlockRecord* record =
                blockRecord(index);


            return record
                ? internString(record->nameOffset)
                : nullptr;
        }


        [[nodiscard]]
        InternedKey scriptName(uint32_t index) const
        {
            const UnicodeScriptRecord* record =
                scriptRecord(index);


            return record
                ? internString(record->nameOffset)
                : nullptr;
        }


        [[nodiscard]]
        InternedKey scriptISO15924(uint32_t index) const
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
        // Shared SET pools
        // ====================================================================

        [[nodiscard]]
        const UnicodeCoveragePools& pools() const noexcept
        {
            return mPools;
        }


        // ====================================================================
        // Shared VALUE8 pools
        // ====================================================================

        [[nodiscard]]
        const UnicodeValueTable8Pools& valuePools8() const noexcept
        {
            return mValuePools8;
        }


    private:
        ByteSpan mData{};

        const UnicodeDatabaseHeader* mHeader{ nullptr };


        // ====================================================================
        // SET data
        // ====================================================================

        const UnicodeCoverageData* mCoverages{ nullptr };

        const UnicodeBlockRecord* mBlocks{ nullptr };
        const UnicodeScriptRecord* mScripts{ nullptr };
        const UnicodePropertyRecord* mProperties{ nullptr };

        UnicodeCoveragePools mPools{};

        UnicodeCoverageIndex mExtendedPictographicCoverage{ kUnicodeCoverageIndexInvalid };

        // ====================================================================
        // VALUE8 data
        // ====================================================================

        const UnicodeValueTable8Data* mValueTables8{ nullptr };
        const UnicodeValueProperty8Record* mValueProperties8{ nullptr };

        UnicodeValueTable8Pools mValuePools8{};

        UnicodeValueTable8Index mGeneralCategoryTable{
            kUnicodeValueTable8IndexInvalid
        };

        UnicodeValueTable8Index mCombiningClassTable{
            kUnicodeValueTable8IndexInvalid
        };

        UnicodeValueTable8Index mBidiClassTable{
            kUnicodeValueTable8IndexInvalid
        };

        UnicodeValueTable8Index mGraphemeClusterBreakTable{
            kUnicodeValueTable8IndexInvalid
        };

        UnicodeValueTable8Index mIndicConjunctBreakTable{
            kUnicodeValueTable8IndexInvalid
        };

        // ====================================================================
        // Canonical decomposition
        // ====================================================================

        const UnicodeDecompositionSection* mDecompositionSection{ nullptr };
        UnicodeDecompositionPools mDecompositionPools{};


        // ====================================================================
        // Canonical composition
        // ====================================================================

        const UnicodeCompositionSection* mCompositionSection{ nullptr };
        const UnicodeCompositionRecord* mCompositionRecords{ nullptr };


        // ====================================================================
        // String pool
        // ====================================================================

        const uint8_t* mStringPool{ nullptr };
        uint32_t mStringPoolSize{ 0 };


        // ====================================================================
        // Architecture helpers
        // ====================================================================

        [[nodiscard]]
        static bool hostIsLittleEndian() noexcept
        {
            const uint16_t value = 1;


            return
                *reinterpret_cast<
                const unsigned char*>(
                    &value) == 1;
        }


        [[nodiscard]]
        static bool pointerIsAligned(
            const void* ptr,
            size_t alignment) noexcept
        {
            if (!ptr ||
                alignment == 0)
            {
                return false;
            }


            const uintptr_t address =
                reinterpret_cast<uintptr_t>(
                    ptr);


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


            if (offset >=
                stringPoolSize)
            {
                return false;
            }


            const uint8_t* begin =
                stringPool + offset;

            const size_t remaining =
                stringPoolSize - offset;


            return
                std::memchr(
                    begin,
                    0,
                    remaining) != nullptr;
        }

        [[nodiscard]]
        static bool stringEquals(UnicodeStringOffset offset, const uint8_t* stringPool,
            uint32_t stringPoolSize, const char* text) noexcept
        {
            if (!stringOffsetValid(offset, stringPool, stringPoolSize) || !text)
                return false;

            const size_t length = std::strlen(text);
            const size_t remaining = stringPoolSize - offset;

            if (length + 1u > remaining)
                return false;

            const uint8_t* begin = stringPool + offset;

            return std::memcmp(begin, text, length) == 0 &&
                begin[length] == 0;
        }

        // ====================================================================
        // SET coverage validation
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


                    if (ref >=
                        masterPageCount)
                    {
                        return false;
                    }
                }
            }


            return true;
        }


        // ====================================================================
        // SET master-page validation
        //
        // Summary masks are relied upon by UnicodeCoverage operations and must
        // therefore agree exactly with sub[].
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


                    if (ref ==
                        kUnicodePageEmpty)
                    {
                        continue;
                    }


                    expectedNonEmpty |=
                        mask;


                    if (ref ==
                        kUnicodePageFull)
                    {
                        expectedFull |=
                            mask;

                        continue;
                    }


                    if (ref >=
                        bitPageCount)
                    {
                        return false;
                    }
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

            }


            return true;
        }


        // ====================================================================
        // VALUE8 table-root validation
        //
        // Every non-uniform master reference must address an actual physical
        // UnicodeValueMasterPage8.
        // ====================================================================

        [[nodiscard]]
        static bool validateValueTables8(
            const UnicodeValueTable8Data* tables,
            uint32_t tableCount,
            uint32_t masterPageCount) noexcept
        {
            for (uint32_t ti = 0;
                ti < tableCount;
                ++ti)
            {
                const UnicodeValueTable8Data& table =
                    tables[ti];


                for (uint32_t mi = 0;
                    mi < kUnicodeMasterCount;
                    ++mi)
                {
                    const UnicodeValueMasterPage8Ref ref =
                        table.masters[mi];


                    if (unicodeValue8RefIsUniform(ref))
                        continue;


                    if (ref >=
                        masterPageCount)
                    {
                        return false;
                    }
                }
            }


            return true;
        }


        // ====================================================================
        // VALUE8 master-page validation
        //
        // Every non-uniform leaf reference must address an actual physical
        // UnicodeValuePage8.
        // ====================================================================

        [[nodiscard]]
        static bool validateValueMasterPages8(
            const UnicodeValueMasterPage8* pages,
            uint32_t masterPageCount,
            uint32_t valuePageCount) noexcept
        {
            for (uint32_t pi = 0;
                pi < masterPageCount;
                ++pi)
            {
                const UnicodeValueMasterPage8& page =
                    pages[pi];


                for (uint32_t si = 0;
                    si < kUnicodeSubsPerMaster;
                    ++si)
                {
                    const UnicodeValuePage8Ref ref =
                        page.sub[si];


                    if (unicodeValue8RefIsUniform(ref))
                        continue;


                    if (ref >=
                        valuePageCount)
                    {
                        return false;
                    }
                }
            }


            return true;
        }


        // ====================================================================
        // Canonical-decomposition validation
        //
        // Structural section bounds have already been established before this
        // helper is called. Validate the persistent reference graph:
        //
        //      root -> master pages -> leaf pages -> records
        // ====================================================================

        [[nodiscard]]
        static bool validateDecomposition(
            const UnicodeDecompositionSection& section,
            const UnicodeDecompositionMasterPage* masterPages,
            const UnicodeDecompositionPage* pages,
            const UnicodeDecompositionRecord* records) noexcept
        {
            if (section.masterPageCount != 0 && !masterPages)
                return false;

            if (section.pageCount != 0 && !pages)
                return false;

            if (section.recordCount != 0 && !records)
                return false;


            // ---------------------------------------------------------------
            // Records
            // ---------------------------------------------------------------

            for (uint32_t i = 0; i < section.recordCount; ++i)
            {
                const UnicodeDecompositionRecord& record =
                    records[i];

                if (record.first >= kUnicodeLimit)
                    return false;

                if (record.second != kUnicodeDecompositionSecondNone &&
                    record.second >= kUnicodeLimit)
                {
                    return false;
                }
            }


            // ---------------------------------------------------------------
            // Root -> master pages
            // ---------------------------------------------------------------

            for (uint32_t mi = 0; mi < kUnicodeMasterCount; ++mi)
            {
                const UnicodeDecompositionMasterPageRef ref =
                    section.data.masters[mi];

                if (ref == kUnicodeDecompositionPageEmpty)
                    continue;

                if (ref >= section.masterPageCount)
                    return false;
            }


            // ---------------------------------------------------------------
            // Master pages -> leaf pages
            // ---------------------------------------------------------------

            for (uint32_t mi = 0; mi < section.masterPageCount; ++mi)
            {
                const UnicodeDecompositionMasterPage& master =
                    masterPages[mi];

                for (uint32_t si = 0; si < kUnicodeSubsPerMaster; ++si)
                {
                    const UnicodeDecompositionPageRef ref =
                        master.sub[si];

                    if (ref == kUnicodeDecompositionPageEmpty)
                        continue;

                    if (ref >= section.pageCount)
                        return false;
                }
            }


            // ---------------------------------------------------------------
            // Leaf pages -> decomposition records
            // ---------------------------------------------------------------

            for (uint32_t pi = 0; pi < section.pageCount; ++pi)
            {
                const UnicodeDecompositionPage& page =
                    pages[pi];

                for (uint32_t i = 0; i < kUnicodeSubSize; ++i)
                {
                    const UnicodeDecompositionRecordRef ref =
                        page.mapping[i];

                    if (ref == kUnicodeDecompositionRecordNone)
                        continue;

                    if (unicodeDecompositionRecordIndex(ref) >=
                        section.recordCount)
                    {
                        return false;
                    }
                }
            }


            return true;
        }


        // ====================================================================
        // Canonical-composition validation
        //
        // Records must be strictly ordered by (first, second). This validates
        // the runtime binary-search invariant and simultaneously rejects both
        // duplicate and conflicting pair keys.
        // ====================================================================

        [[nodiscard]]
        static bool validateCompositionRecords(
            const UnicodeCompositionRecord* records,
            uint32_t recordCount) noexcept
        {
            if (recordCount != 0 && !records)
                return false;

            for (uint32_t i = 0; i < recordCount; ++i)
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


                if (havePrevious &&
                    record.first <=
                    previousLast)
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


                previousLast =
                    record.last;

                havePrevious =
                    true;
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

            }


            return true;
        }


        // ====================================================================
        // Binary SET property validation
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

            }


            return true;
        }


        // ====================================================================
        // VALUE8 property validation
        //
        // Validate:
        //
        //      property id
        //      table index
        //      semantic-property uniqueness
        //
        // Resolve the two core shaping tables while scanning.
        //
        // When VALUE8 data is present, require both currently defined core
        // properties exactly once.
        // ====================================================================

        [[nodiscard]]
        static bool validateValueProperties8(const UnicodeValueProperty8Record* records,
            uint32_t count, uint32_t tableCount, uint16_t formatMinor,
            UnicodeValueTable8Index& outGeneralCategory,
            UnicodeValueTable8Index& outCombiningClass,
            UnicodeValueTable8Index& outBidiClass,
            UnicodeValueTable8Index& outGraphemeClusterBreak,
            UnicodeValueTable8Index& outIndicConjunctBreak) noexcept
        {
            outGeneralCategory = kUnicodeValueTable8IndexInvalid;
            outCombiningClass = kUnicodeValueTable8IndexInvalid;
            outBidiClass = kUnicodeValueTable8IndexInvalid;
            outGraphemeClusterBreak = kUnicodeValueTable8IndexInvalid;
            outIndicConjunctBreak = kUnicodeValueTable8IndexInvalid;

            // ------------------------------------------------------------------------
            // Maximum semantic property understood by each format revision.
            // ------------------------------------------------------------------------

            uint16_t maximumProperty =
                UnicodeValueProperty8CanonicalCombiningClass;

            if (formatMinor >= 4)
                maximumProperty = UnicodeValueProperty8BidiClass;

            if (formatMinor >= 5)
                maximumProperty = UnicodeValueProperty8GraphemeClusterBreak;

            if (formatMinor >= 6)
                maximumProperty = UnicodeValueProperty8IndicConjunctBreak;

            // ------------------------------------------------------------------------
            // Validate directory records and resolve semantic table indices.
            // ------------------------------------------------------------------------

            for (uint32_t i = 0; i < count; ++i)
            {
                const UnicodeValueProperty8Record& record = records[i];

                if (record.reserved != 0)
                    return false;

                if (record.tableIndex >= tableCount)
                    return false;

                if (record.property <= UnicodeValueProperty8Unknown ||
                    record.property > maximumProperty)
                {
                    return false;
                }


                // --------------------------------------------------------------------
                // Semantic properties must be unique.
                // --------------------------------------------------------------------

                for (uint32_t previous = 0; previous < i; ++previous)
                {
                    if (records[previous].property == record.property)
                        return false;
                }


                switch (static_cast<UnicodeValueProperty8>(record.property))
                {
                case UnicodeValueProperty8GeneralCategory:
                    outGeneralCategory = record.tableIndex;
                    break;

                case UnicodeValueProperty8CanonicalCombiningClass:
                    outCombiningClass = record.tableIndex;
                    break;

                case UnicodeValueProperty8BidiClass:
                    outBidiClass = record.tableIndex;
                    break;

                case UnicodeValueProperty8GraphemeClusterBreak:
                    outGraphemeClusterBreak = record.tableIndex;
                    break;

                case UnicodeValueProperty8IndicConjunctBreak:
                    outIndicConjunctBreak = record.tableIndex;
                    break;


                default:
                    return false;
                }
            }


            // ------------------------------------------------------------------------
            // Format contracts.
            //
            // 1.1 - 1.3:
            //     General_Category
            //     Canonical_Combining_Class
            //
            // 1.4:
            //     adds Bidi_Class
            //
            // 1.5:
            //     adds Grapheme_Cluster_Break
            // 
            // 1.6:
            //     adds Indic_Conjunct_Break
            // ------------------------------------------------------------------------

            if (formatMinor >= 1)
            {
                if (outGeneralCategory == kUnicodeValueTable8IndexInvalid)
                    return false;

                if (outCombiningClass == kUnicodeValueTable8IndexInvalid)
                    return false;
            }

            if (formatMinor >= 4)
            {
                if (outBidiClass == kUnicodeValueTable8IndexInvalid)
                    return false;
            }

            if (formatMinor >= 5)
            {
                if (outGraphemeClusterBreak == kUnicodeValueTable8IndexInvalid)
                    return false;
            }

            if (formatMinor >= 6)
            {
                if (outIndicConjunctBreak == kUnicodeValueTable8IndexInvalid)
                    return false;
            }

            return true;
        }


        // ====================================================================
        // VALUE8 semantic-range validation
        //
        // Verify every value reachable from one table is <= maxValue.
        //
        // This catches corruption which is structurally valid but semantically
        // invalid for a particular property.
        //
        // Examples:
        //
        //      General_Category:
        //          valid 0 .. 29
        //
        //      Canonical_Combining_Class:
        //          valid 0 .. 254
        //
        // Structural reference validation must already have succeeded before
        // this function is called.
        // ====================================================================

        [[nodiscard]]
        static bool validateValueTable8Maximum(
            const UnicodeValueTable8Data& table,
            const UnicodeValueMasterPage8* masterPages,
            uint32_t masterPageCount,
            const UnicodeValuePage8* valuePages,
            uint32_t valuePageCount,
            uint8_t maxValue) noexcept
        {
            for (uint32_t mi = 0;
                mi < kUnicodeMasterCount;
                ++mi)
            {
                const UnicodeValueMasterPage8Ref masterRef =
                    table.masters[mi];


                // -----------------------------------------------------------
                // Uniform master.
                // -----------------------------------------------------------

                if (unicodeValue8RefIsUniform(
                    masterRef))
                {
                    if (unicodeValue8RefUniformValue(
                        masterRef) > maxValue)
                    {
                        return false;
                    }

                    continue;
                }


                if (masterRef >=
                    masterPageCount)
                {
                    return false;
                }


                const UnicodeValueMasterPage8& master =
                    masterPages[masterRef];


                for (uint32_t si = 0;
                    si < kUnicodeSubsPerMaster;
                    ++si)
                {
                    const UnicodeValuePage8Ref pageRef =
                        master.sub[si];


                    // -------------------------------------------------------
                    // Uniform leaf page.
                    // -------------------------------------------------------

                    if (unicodeValue8RefIsUniform(
                        pageRef))
                    {
                        if (unicodeValue8RefUniformValue(
                            pageRef) > maxValue)
                        {
                            return false;
                        }

                        continue;
                    }


                    if (pageRef >=
                        valuePageCount)
                    {
                        return false;
                    }


                    const UnicodeValuePage8& page =
                        valuePages[pageRef];


                    for (uint32_t vi = 0;
                        vi < kUnicodeSubSize;
                        ++vi)
                    {
                        if (page.values[vi] >
                            maxValue)
                        {
                            return false;
                        }
                    }
                }
            }


            return true;
        }
    };

} // namespace waavs