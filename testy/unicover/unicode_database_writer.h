// unicode_database_writer.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <utility>
#include <vector>

#include "unicode_database_builder.h"
#include "unicode_database_format.h"


namespace waavs
{
    // ========================================================================
    // UnicodeDatabaseWriteError
    // ========================================================================

    enum class UnicodeDatabaseWriteError : uint8_t
    {
        None = 0,

        UnsupportedByteOrder,

        TooManyCoverages,
        TooManyMasterPages,
        TooManyBitPages,

        TooManyValueProperties8,
        TooManyValueTables8,
        TooManyValueMasterPages8,
        TooManyValuePages8,

        TooManyDecompositionMasterPages,
        TooManyDecompositionPages,
        TooManyDecompositionRecords,
        InvalidDecomposition,

        TooManyCompositionRecords,
        InvalidComposition,

        TooManyBlocks,
        TooManyScripts,
        TooManyProperties,

        StringPoolTooLarge,
        InvalidStringPool,
        DatabaseTooLarge,

        FileOpenFailed,
        FileWriteFailed
    };


    // ========================================================================
    // UnicodeDatabaseWriteResult
    // ========================================================================

    struct UnicodeDatabaseWriteResult
    {
        UnicodeDatabaseWriteError error{ UnicodeDatabaseWriteError::None };
        uint32_t databaseSize{ 0 };


        [[nodiscard]]
        bool success() const noexcept
        {
            return error == UnicodeDatabaseWriteError::None;
        }


        explicit operator bool() const noexcept
        {
            return success();
        }
    };


    // ========================================================================
    // unicodeDatabaseWriteErrorString
    // ========================================================================

    static inline const char* unicodeDatabaseWriteErrorString(
        UnicodeDatabaseWriteError error) noexcept
    {
        switch (error)
        {
        case UnicodeDatabaseWriteError::None:
            return "no error";

        case UnicodeDatabaseWriteError::UnsupportedByteOrder:
            return "database writer requires a little-endian host";

        case UnicodeDatabaseWriteError::TooManyCoverages:
            return "too many Unicode coverage records";

        case UnicodeDatabaseWriteError::TooManyMasterPages:
            return "too many Unicode master pages";

        case UnicodeDatabaseWriteError::TooManyBitPages:
            return "too many Unicode bit pages";

        case UnicodeDatabaseWriteError::TooManyValueProperties8:
            return "too many Unicode VALUE8 property records";

        case UnicodeDatabaseWriteError::TooManyValueTables8:
            return "too many Unicode VALUE8 tables";

        case UnicodeDatabaseWriteError::TooManyValueMasterPages8:
            return "too many Unicode VALUE8 master pages";

        case UnicodeDatabaseWriteError::TooManyValuePages8:
            return "too many Unicode VALUE8 pages";

        case UnicodeDatabaseWriteError::TooManyDecompositionMasterPages:
            return "too many Unicode decomposition master pages";

        case UnicodeDatabaseWriteError::TooManyDecompositionPages:
            return "too many Unicode decomposition pages";

        case UnicodeDatabaseWriteError::TooManyDecompositionRecords:
            return "too many Unicode decomposition records";

        case UnicodeDatabaseWriteError::InvalidDecomposition:
            return "invalid Unicode canonical decomposition data";

        case UnicodeDatabaseWriteError::TooManyCompositionRecords:
            return "too many Unicode composition records";

        case UnicodeDatabaseWriteError::InvalidComposition:
            return "invalid Unicode canonical composition data";

        case UnicodeDatabaseWriteError::TooManyBlocks:
            return "too many Unicode block records";

        case UnicodeDatabaseWriteError::TooManyScripts:
            return "too many Unicode script records";

        case UnicodeDatabaseWriteError::TooManyProperties:
            return "too many Unicode binary property records";

        case UnicodeDatabaseWriteError::StringPoolTooLarge:
            return "Unicode string pool exceeds database format limits";

        case UnicodeDatabaseWriteError::InvalidStringPool:
            return "Unicode string pool is invalid";

        case UnicodeDatabaseWriteError::DatabaseTooLarge:
            return "Unicode database exceeds 32-bit database size limit";

        case UnicodeDatabaseWriteError::FileOpenFailed:
            return "unable to open Unicode database output file";

        case UnicodeDatabaseWriteError::FileWriteFailed:
            return "unable to write Unicode database output file";
        }

        return "unknown Unicode database writer error";
    }


    // ========================================================================
    // UnicodeDatabaseWriter
    //
    // Serializes UnicodeDatabaseBuilder into the persistent .ucdb format.
    //
    // Version 1.3 layout currently emitted:
    //
    //      UnicodeDatabaseHeader
    //
    //      UnicodeBlockRecord[]
    //      UnicodeScriptRecord[]
    //      UnicodePropertyRecord[]
    //      UnicodeValueProperty8Record[]
    //
    //      UnicodeCoverageData[]
    //      UnicodeValueTable8Data[]
    //
    //      UnicodeDecompositionSection       optional
    //      UnicodeCompositionSection         optional
    //
    //      alignment
    //      UnicodeMasterPage[]
    //
    //      alignment
    //      UnicodeBitPage[]
    //
    //      alignment
    //      UnicodeValueMasterPage8[]
    //
    //      alignment
    //      UnicodeValuePage8[]
    //
    //      alignment
    //      UnicodeDecompositionMasterPage[]  optional
    //
    //      alignment
    //      UnicodeDecompositionPage[]        optional
    //
    //      alignment
    //      UnicodeDecompositionRecord[]      optional
    //
    //      alignment
    //      UnicodeCompositionRecord[]        optional
    //
    //      UTF-8 string pool
    //
    // Section ordering itself is not part of the persistent ABI. Offsets in
    // UnicodeDatabaseHeader, UnicodeDecompositionSection, and
    // UnicodeCompositionSection define the physical layout.
    //
    // All page pools have already been canonicalized and deduplicated by the
    // corresponding generator-side pool builders.
    //
    // ========================================================================

    class UnicodeDatabaseWriter
    {
    private:
        // ====================================================================
        // Layout
        //
        // Calculated completely before output storage is allocated.
        // ====================================================================

        struct Layout
        {
            uint32_t blockOffset{ 0 };
            uint32_t scriptOffset{ 0 };
            uint32_t propertyOffset{ 0 };
            uint32_t valueProperty8Offset{ 0 };

            uint32_t coverageOffset{ 0 };
            uint32_t valueTable8Offset{ 0 };

            uint32_t decompositionOffset{ 0 };
            uint32_t compositionOffset{ 0 };

            uint32_t masterPageOffset{ 0 };
            uint32_t bitPageOffset{ 0 };

            uint32_t valueMasterPage8Offset{ 0 };
            uint32_t valuePage8Offset{ 0 };

            uint32_t decompositionMasterPageOffset{ 0 };
            uint32_t decompositionPageOffset{ 0 };
            uint32_t decompositionRecordOffset{ 0 };

            uint32_t compositionRecordOffset{ 0 };

            uint32_t stringPoolOffset{ 0 };

            uint32_t databaseSize{ 0 };
        };


    public:
        // ====================================================================
        // write
        //
        // Serialize the complete builder into an in-memory database image.
        //
        // unicodeMajor / Minor / Patch describe the Unicode source data.
        //
        // outData is replaced only after complete validation and serialization
        // succeed.
        // ====================================================================

        static bool write(const UnicodeDatabaseBuilder& database,
            uint16_t unicodeMajor, uint16_t unicodeMinor, uint16_t unicodePatch,
            std::vector<uint8_t>& outData, UnicodeDatabaseWriteResult& outResult)
        {
            outResult = {};


            // ---------------------------------------------------------------
            // Version 1 databases are canonical little-endian images.
            // ---------------------------------------------------------------

            if (!hostIsLittleEndian())
            {
                outResult.error =
                    UnicodeDatabaseWriteError::UnsupportedByteOrder;

                return false;
            }


            // ================================================================
            // SET count validation
            // ================================================================

            if (database.coverageCount() >=
                static_cast<size_t>(kUnicodeCoverageIndexInvalid))
            {
                outResult.error =
                    UnicodeDatabaseWriteError::TooManyCoverages;

                return false;
            }


            if (database.pagePool().masterPageCount() >
                kUnicodeMaxPoolPages)
            {
                outResult.error =
                    UnicodeDatabaseWriteError::TooManyMasterPages;

                return false;
            }


            if (database.pagePool().bitPageCount() >
                kUnicodeMaxPoolPages)
            {
                outResult.error =
                    UnicodeDatabaseWriteError::TooManyBitPages;

                return false;
            }


            // ================================================================
            // VALUE8 count validation
            // ================================================================

            if (database.valueProperty8Count() >
                std::numeric_limits<uint32_t>::max())
            {
                outResult.error =
                    UnicodeDatabaseWriteError::TooManyValueProperties8;

                return false;
            }


            if (database.valueTable8Count() >=
                static_cast<size_t>(kUnicodeValueTable8IndexInvalid))
            {
                outResult.error =
                    UnicodeDatabaseWriteError::TooManyValueTables8;

                return false;
            }


            if (database.valuePagePool8().masterPageCount() >
                kUnicodeValue8MaxPoolPages)
            {
                outResult.error =
                    UnicodeDatabaseWriteError::TooManyValueMasterPages8;

                return false;
            }


            if (database.valuePagePool8().valuePageCount() >
                kUnicodeValue8MaxPoolPages)
            {
                outResult.error =
                    UnicodeDatabaseWriteError::TooManyValuePages8;

                return false;
            }


            // ================================================================
            // Canonical-decomposition validation
            // ================================================================

            if (database.decompositionPagePool().masterPageCount() >
                kUnicodeDecompositionMaxPoolPages)
            {
                outResult.error =
                    UnicodeDatabaseWriteError::TooManyDecompositionMasterPages;

                return false;
            }


            if (database.decompositionPagePool().pageCount() >
                kUnicodeDecompositionMaxPoolPages)
            {
                outResult.error =
                    UnicodeDatabaseWriteError::TooManyDecompositionPages;

                return false;
            }


            if (database.decompositionRecordCount() >
                kUnicodeDecompositionMaxRecords)
            {
                outResult.error =
                    UnicodeDatabaseWriteError::TooManyDecompositionRecords;

                return false;
            }


            if (!validateDecomposition(database))
            {
                outResult.error =
                    UnicodeDatabaseWriteError::InvalidDecomposition;

                return false;
            }


            // ================================================================
            // Canonical-composition validation
            // ================================================================

            if (database.compositionRecordCount() >
                std::numeric_limits<uint32_t>::max())
            {
                outResult.error =
                    UnicodeDatabaseWriteError::TooManyCompositionRecords;

                return false;
            }


            if (!validateComposition(database))
            {
                outResult.error =
                    UnicodeDatabaseWriteError::InvalidComposition;

                return false;
            }


            // ================================================================
            // Semantic directories
            // ================================================================

            if (database.blockCount() >
                std::numeric_limits<uint32_t>::max())
            {
                outResult.error =
                    UnicodeDatabaseWriteError::TooManyBlocks;

                return false;
            }


            if (database.scriptCount() >
                std::numeric_limits<uint32_t>::max())
            {
                outResult.error =
                    UnicodeDatabaseWriteError::TooManyScripts;

                return false;
            }


            if (database.propertyCount() >
                std::numeric_limits<uint32_t>::max())
            {
                outResult.error =
                    UnicodeDatabaseWriteError::TooManyProperties;

                return false;
            }


            // ================================================================
            // String pool
            // ================================================================

            if (database.stringPoolSize() >
                std::numeric_limits<uint32_t>::max())
            {
                outResult.error =
                    UnicodeDatabaseWriteError::StringPoolTooLarge;

                return false;
            }


            if (database.stringPool().empty() ||
                database.stringPool()[0] != 0)
            {
                outResult.error =
                    UnicodeDatabaseWriteError::InvalidStringPool;

                return false;
            }


            // ================================================================
            // Calculate complete physical layout
            // ================================================================

            Layout layout{};


            if (!calculateLayout(
                database,
                layout))
            {
                outResult.error =
                    UnicodeDatabaseWriteError::DatabaseTooLarge;

                return false;
            }


            // ================================================================
            // Allocate complete database image
            //
            // Zero initialization deliberately makes alignment padding and
            // reserved bytes deterministic.
            // ================================================================

            std::vector<uint8_t> result(
                static_cast<size_t>(layout.databaseSize),
                uint8_t(0));


            // ================================================================
            // Header
            // ================================================================

            UnicodeDatabaseHeader header{};


            std::memcpy(
                header.signature,
                kUnicodeDatabaseSignature,
                sizeof(header.signature));


            header.formatMajor =
                kUnicodeDatabaseFormatMajor;

            header.formatMinor =
                kUnicodeDatabaseFormatMinor;


            header.unicodeMajor =
                unicodeMajor;

            header.unicodeMinor =
                unicodeMinor;

            header.unicodePatch =
                unicodePatch;


            header.headerSize =
                static_cast<uint32_t>(
                    sizeof(UnicodeDatabaseHeader));

            header.databaseSize =
                layout.databaseSize;

            header.byteOrder =
                kUnicodeDatabaseByteOrder;

            header.flags =
                0;


            // ---------------------------------------------------------------
            // SET roots / pools
            // ---------------------------------------------------------------

            header.coverageOffset =
                layout.coverageOffset;

            header.coverageCount =
                static_cast<uint32_t>(
                    database.coverageCount());


            header.masterPageOffset =
                layout.masterPageOffset;

            header.masterPageCount =
                static_cast<uint32_t>(
                    database.pagePool().masterPageCount());


            header.bitPageOffset =
                layout.bitPageOffset;

            header.bitPageCount =
                static_cast<uint32_t>(
                    database.pagePool().bitPageCount());


            // ---------------------------------------------------------------
            // Semantic directories
            // ---------------------------------------------------------------

            header.blockOffset =
                layout.blockOffset;

            header.blockCount =
                static_cast<uint32_t>(
                    database.blockCount());


            header.scriptOffset =
                layout.scriptOffset;

            header.scriptCount =
                static_cast<uint32_t>(
                    database.scriptCount());


            header.propertyOffset =
                layout.propertyOffset;

            header.propertyCount =
                static_cast<uint32_t>(
                    database.propertyCount());


            // ---------------------------------------------------------------
            // String pool
            // ---------------------------------------------------------------

            header.stringPoolOffset =
                layout.stringPoolOffset;

            header.stringPoolSize =
                static_cast<uint32_t>(
                    database.stringPoolSize());


            // ---------------------------------------------------------------
            // VALUE8 property directory
            // ---------------------------------------------------------------

            header.valueProperty8Offset =
                layout.valueProperty8Offset;

            header.valueProperty8Count =
                static_cast<uint32_t>(
                    database.valueProperty8Count());


            // ---------------------------------------------------------------
            // VALUE8 table roots
            // ---------------------------------------------------------------

            header.valueTable8Offset =
                layout.valueTable8Offset;

            header.valueTable8Count =
                static_cast<uint32_t>(
                    database.valueTable8Count());


            // ---------------------------------------------------------------
            // VALUE8 master-page pool
            // ---------------------------------------------------------------

            header.valueMasterPage8Offset =
                layout.valueMasterPage8Offset;

            header.valueMasterPage8Count =
                static_cast<uint32_t>(
                    database.valuePagePool8().masterPageCount());


            // ---------------------------------------------------------------
            // VALUE8 leaf-page pool
            // ---------------------------------------------------------------

            header.valuePage8Offset =
                layout.valuePage8Offset;

            header.valuePage8Count =
                static_cast<uint32_t>(
                    database.valuePagePool8().valuePageCount());


            // ---------------------------------------------------------------
            // Canonical decomposition
            //
            // Zero means this database does not contain decomposition data.
            // ---------------------------------------------------------------

            header.decompositionOffset =
                database.hasDecomposition()
                ? layout.decompositionOffset
                : 0;


            // ---------------------------------------------------------------
            // Canonical composition
            //
            // Zero means this database does not contain composition data.
            // ---------------------------------------------------------------

            header.compositionOffset =
                database.hasComposition()
                ? layout.compositionOffset
                : 0;


            // ---------------------------------------------------------------
            // Write header
            // ---------------------------------------------------------------

            std::memcpy(
                result.data(),
                &header,
                sizeof(header));


            // ================================================================
            // Semantic directories
            // ================================================================

            copyArray(
                result,
                layout.blockOffset,
                database.blocks());


            copyArray(
                result,
                layout.scriptOffset,
                database.scripts());


            copyArray(
                result,
                layout.propertyOffset,
                database.properties());


            copyArray(
                result,
                layout.valueProperty8Offset,
                database.valueProperties8());


            // ================================================================
            // Root tables
            // ================================================================

            copyArray(
                result,
                layout.coverageOffset,
                database.coverages());


            copyArray(
                result,
                layout.valueTable8Offset,
                database.valueTables8());


            // ================================================================
            // Canonical-decomposition descriptor
            // ================================================================

            if (database.hasDecomposition())
            {
                UnicodeDecompositionSection section{};


                section.data =
                    database.decomposition();


                section.masterPageOffset =
                    layout.decompositionMasterPageOffset;

                section.masterPageCount =
                    static_cast<uint32_t>(
                        database.decompositionPagePool().masterPageCount());


                section.pageOffset =
                    layout.decompositionPageOffset;

                section.pageCount =
                    static_cast<uint32_t>(
                        database.decompositionPagePool().pageCount());


                section.recordOffset =
                    layout.decompositionRecordOffset;

                section.recordCount =
                    static_cast<uint32_t>(
                        database.decompositionRecordCount());


                std::memcpy(
                    result.data() + layout.decompositionOffset,
                    &section,
                    sizeof(section));
            }


            // ================================================================
            // Canonical-composition descriptor
            // ================================================================

            if (database.hasComposition())
            {
                UnicodeCompositionSection section{};

                section.recordOffset =
                    layout.compositionRecordOffset;

                section.recordCount =
                    static_cast<uint32_t>(
                        database.compositionRecordCount());


                std::memcpy(
                    result.data() + layout.compositionOffset,
                    &section,
                    sizeof(section));
            }


            // ================================================================
            // SET pools
            // ================================================================

            copyArray(
                result,
                layout.masterPageOffset,
                database.pagePool().masterPages());


            copyArray(
                result,
                layout.bitPageOffset,
                database.pagePool().bitPages());


            // ================================================================
            // VALUE8 pools
            // ================================================================

            copyArray(
                result,
                layout.valueMasterPage8Offset,
                database.valuePagePool8().masterPages());


            copyArray(
                result,
                layout.valuePage8Offset,
                database.valuePagePool8().valuePages());


            // ================================================================
            // Canonical-decomposition pools
            // ================================================================

            if (database.hasDecomposition())
            {
                copyArray(
                    result,
                    layout.decompositionMasterPageOffset,
                    database.decompositionPagePool().masterPages());


                copyArray(
                    result,
                    layout.decompositionPageOffset,
                    database.decompositionPagePool().pages());


                copyArray(
                    result,
                    layout.decompositionRecordOffset,
                    database.decompositionRecords());
            }


            // ================================================================
            // Canonical-composition records
            // ================================================================

            if (database.hasComposition())
            {
                copyArray(
                    result,
                    layout.compositionRecordOffset,
                    database.compositionRecords());
            }


            // ================================================================
            // String pool
            // ================================================================

            if (!database.stringPool().empty())
            {
                std::memcpy(
                    result.data() + layout.stringPoolOffset,
                    database.stringPool().data(),
                    database.stringPool().size());
            }


            // ================================================================
            // Commit
            // ================================================================

            outData =
                std::move(result);

            outResult.databaseSize =
                layout.databaseSize;


            return true;
        }


        // ====================================================================
        // Convenience overload
        // ====================================================================

        static bool write(const UnicodeDatabaseBuilder& database,
            uint16_t unicodeMajor, uint16_t unicodeMinor, uint16_t unicodePatch,
            std::vector<uint8_t>& outData)
        {
            UnicodeDatabaseWriteResult result;


            return write(
                database,
                unicodeMajor,
                unicodeMinor,
                unicodePatch,
                outData,
                result);
        }


        // ====================================================================
        // writeFile
        // ====================================================================

        static bool writeFile(const char* filename,
            const UnicodeDatabaseBuilder& database,
            uint16_t unicodeMajor, uint16_t unicodeMinor, uint16_t unicodePatch,
            UnicodeDatabaseWriteResult& outResult)
        {
            std::vector<uint8_t> data;


            if (!write(
                database,
                unicodeMajor,
                unicodeMinor,
                unicodePatch,
                data,
                outResult))
            {
                return false;
            }


            std::ofstream output(
                filename,
                std::ios::binary |
                std::ios::out |
                std::ios::trunc);


            if (!output)
            {
                outResult.error =
                    UnicodeDatabaseWriteError::FileOpenFailed;

                return false;
            }


            output.write(
                reinterpret_cast<const char*>(
                    data.data()),
                static_cast<std::streamsize>(
                    data.size()));


            if (!output)
            {
                outResult.error =
                    UnicodeDatabaseWriteError::FileWriteFailed;

                return false;
            }


            output.close();


            if (!output)
            {
                outResult.error =
                    UnicodeDatabaseWriteError::FileWriteFailed;

                return false;
            }


            return true;
        }


        // ====================================================================
        // Convenience writeFile overload
        // ====================================================================

        static bool writeFile(const char* filename,
            const UnicodeDatabaseBuilder& database,
            uint16_t unicodeMajor, uint16_t unicodeMinor, uint16_t unicodePatch)
        {
            UnicodeDatabaseWriteResult result;


            return writeFile(
                filename,
                database,
                unicodeMajor,
                unicodeMinor,
                unicodePatch,
                result);
        }


    private:
        // ====================================================================
        // calculateLayout
        //
        // All calculations use uint64_t so overflow is detected before values
        // are narrowed into persistent uint32_t fields.
        //
        // An absent decomposition or composition dataset consumes no storage.
        //
        // For decomposition pools, an empty pool has offset zero. This matches
        // the UnicodeDecompositionSection persistent contract.
        // ====================================================================

        static bool calculateLayout(const UnicodeDatabaseBuilder& database,
            Layout& outLayout) noexcept
        {
            uint64_t cursor =
                sizeof(UnicodeDatabaseHeader);


            // ================================================================
            // Semantic directories
            // ================================================================

            outLayout.blockOffset =
                static_cast<uint32_t>(cursor);


            if (!advanceArray<UnicodeBlockRecord>(
                cursor,
                database.blockCount()))
            {
                return false;
            }


            if (!fitsUint32(cursor))
                return false;


            outLayout.scriptOffset =
                static_cast<uint32_t>(cursor);


            if (!advanceArray<UnicodeScriptRecord>(
                cursor,
                database.scriptCount()))
            {
                return false;
            }


            if (!fitsUint32(cursor))
                return false;


            outLayout.propertyOffset =
                static_cast<uint32_t>(cursor);


            if (!advanceArray<UnicodePropertyRecord>(
                cursor,
                database.propertyCount()))
            {
                return false;
            }


            if (!fitsUint32(cursor))
                return false;


            outLayout.valueProperty8Offset =
                static_cast<uint32_t>(cursor);


            if (!advanceArray<UnicodeValueProperty8Record>(
                cursor,
                database.valueProperty8Count()))
            {
                return false;
            }


            // ================================================================
            // Root records
            // ================================================================

            if (!fitsUint32(cursor))
                return false;


            outLayout.coverageOffset =
                static_cast<uint32_t>(cursor);


            if (!advanceArray<UnicodeCoverageData>(
                cursor,
                database.coverageCount()))
            {
                return false;
            }


            if (!fitsUint32(cursor))
                return false;


            outLayout.valueTable8Offset =
                static_cast<uint32_t>(cursor);


            if (!advanceArray<UnicodeValueTable8Data>(
                cursor,
                database.valueTable8Count()))
            {
                return false;
            }


            // ================================================================
            // Canonical-decomposition descriptor
            // ================================================================

            if (database.hasDecomposition())
            {
                if (!fitsUint32(cursor))
                    return false;


                outLayout.decompositionOffset =
                    static_cast<uint32_t>(cursor);


                if (!advanceArray<UnicodeDecompositionSection>(
                    cursor,
                    1))
                {
                    return false;
                }
            }


            // ================================================================
            // Canonical-composition descriptor
            // ================================================================

            if (database.hasComposition())
            {
                if (!fitsUint32(cursor))
                    return false;


                outLayout.compositionOffset =
                    static_cast<uint32_t>(cursor);


                if (!advanceArray<UnicodeCompositionSection>(
                    cursor,
                    1))
                {
                    return false;
                }
            }


            // ================================================================
            // SET master-page pool
            // ================================================================

            if (!alignCursor(
                cursor,
                kUnicodeDatabasePoolAlignment))
            {
                return false;
            }


            if (!fitsUint32(cursor))
                return false;


            outLayout.masterPageOffset =
                static_cast<uint32_t>(cursor);


            if (!advanceArray<UnicodeMasterPage>(
                cursor,
                database.pagePool().masterPageCount()))
            {
                return false;
            }


            // ================================================================
            // SET bit-page pool
            // ================================================================

            if (!alignCursor(
                cursor,
                kUnicodeDatabasePoolAlignment))
            {
                return false;
            }


            if (!fitsUint32(cursor))
                return false;


            outLayout.bitPageOffset =
                static_cast<uint32_t>(cursor);


            if (!advanceArray<UnicodeBitPage>(
                cursor,
                database.pagePool().bitPageCount()))
            {
                return false;
            }


            // ================================================================
            // VALUE8 master-page pool
            // ================================================================

            if (!alignCursor(
                cursor,
                kUnicodeDatabasePoolAlignment))
            {
                return false;
            }


            if (!fitsUint32(cursor))
                return false;


            outLayout.valueMasterPage8Offset =
                static_cast<uint32_t>(cursor);


            if (!advanceArray<UnicodeValueMasterPage8>(
                cursor,
                database.valuePagePool8().masterPageCount()))
            {
                return false;
            }


            // ================================================================
            // VALUE8 leaf-page pool
            // ================================================================

            if (!alignCursor(
                cursor,
                kUnicodeDatabasePoolAlignment))
            {
                return false;
            }


            if (!fitsUint32(cursor))
                return false;


            outLayout.valuePage8Offset =
                static_cast<uint32_t>(cursor);


            if (!advanceArray<UnicodeValuePage8>(
                cursor,
                database.valuePagePool8().valuePageCount()))
            {
                return false;
            }


            // ================================================================
            // Canonical-decomposition master-page pool
            // ================================================================

            if (database.hasDecomposition() &&
                database.decompositionPagePool().masterPageCount() != 0)
            {
                if (!alignCursor(
                    cursor,
                    kUnicodeDatabasePoolAlignment))
                {
                    return false;
                }


                if (!fitsUint32(cursor))
                    return false;


                outLayout.decompositionMasterPageOffset =
                    static_cast<uint32_t>(cursor);


                if (!advanceArray<UnicodeDecompositionMasterPage>(
                    cursor,
                    database.decompositionPagePool().masterPageCount()))
                {
                    return false;
                }
            }


            // ================================================================
            // Canonical-decomposition leaf-page pool
            // ================================================================

            if (database.hasDecomposition() &&
                database.decompositionPagePool().pageCount() != 0)
            {
                if (!alignCursor(
                    cursor,
                    kUnicodeDatabasePoolAlignment))
                {
                    return false;
                }


                if (!fitsUint32(cursor))
                    return false;


                outLayout.decompositionPageOffset =
                    static_cast<uint32_t>(cursor);


                if (!advanceArray<UnicodeDecompositionPage>(
                    cursor,
                    database.decompositionPagePool().pageCount()))
                {
                    return false;
                }
            }


            // ================================================================
            // Canonical-decomposition record pool
            // ================================================================

            if (database.hasDecomposition() &&
                database.decompositionRecordCount() != 0)
            {
                if (!alignCursor(
                    cursor,
                    kUnicodeDatabasePoolAlignment))
                {
                    return false;
                }


                if (!fitsUint32(cursor))
                    return false;


                outLayout.decompositionRecordOffset =
                    static_cast<uint32_t>(cursor);


                if (!advanceArray<UnicodeDecompositionRecord>(
                    cursor,
                    database.decompositionRecordCount()))
                {
                    return false;
                }
            }


            // ================================================================
            // Canonical-composition record array
            // ================================================================

            if (database.hasComposition() &&
                database.compositionRecordCount() != 0)
            {
                if (!alignCursor(
                    cursor,
                    kUnicodeDatabasePoolAlignment))
                {
                    return false;
                }


                if (!fitsUint32(cursor))
                    return false;


                outLayout.compositionRecordOffset =
                    static_cast<uint32_t>(cursor);


                if (!advanceArray<UnicodeCompositionRecord>(
                    cursor,
                    database.compositionRecordCount()))
                {
                    return false;
                }
            }


            // ================================================================
            // String pool
            // ================================================================

            if (!fitsUint32(cursor))
                return false;


            outLayout.stringPoolOffset =
                static_cast<uint32_t>(cursor);


            if (!advanceBytes(
                cursor,
                database.stringPoolSize()))
            {
                return false;
            }


            // ================================================================
            // Final database size
            // ================================================================

            if (!fitsUint32(cursor))
                return false;


            outLayout.databaseSize =
                static_cast<uint32_t>(cursor);


            return true;
        }


        // ====================================================================
        // validateDecomposition
        //
        // Validate the complete decomposition hierarchy before serialization.
        //
        // This intentionally repeats the important persistent-reference checks
        // performed by UnicodeDatabaseBuilder::setDecomposition(). The builder
        // exposes its page pool for generator construction, so validation here
        // ensures later generator-side mutation cannot result in an invalid
        // serialized image.
        // ====================================================================

        [[nodiscard]]
        static bool validateDecomposition(
            const UnicodeDatabaseBuilder& database) noexcept
        {
            const UnicodeDecompositionPagePoolBuilder& pool =
                database.decompositionPagePool();


            // ---------------------------------------------------------------
            // An absent decomposition dataset must not leave orphaned
            // decomposition storage in the database builder.
            // ---------------------------------------------------------------

            if (!database.hasDecomposition())
            {
                return
                    pool.masterPageCount() == 0 &&
                    pool.pageCount() == 0 &&
                    database.decompositionRecordCount() == 0;
            }


            const size_t recordCount =
                database.decompositionRecordCount();


            // ---------------------------------------------------------------
            // Validate decomposition records.
            // ---------------------------------------------------------------

            for (const UnicodeDecompositionRecord& record :
                database.decompositionRecords())
            {
                if (record.first >= kUnicodeLimit)
                    return false;


                if (record.second != kUnicodeDecompositionSecondNone &&
                    record.second >= kUnicodeLimit)
                {
                    return false;
                }
            }


            // ---------------------------------------------------------------
            // Root -> master pages.
            // ---------------------------------------------------------------

            const UnicodeDecompositionData& data =
                database.decomposition();


            for (uint32_t mi = 0; mi < kUnicodeMasterCount; ++mi)
            {
                const UnicodeDecompositionMasterPageRef ref =
                    data.masters[mi];


                if (ref == kUnicodeDecompositionPageEmpty)
                    continue;


                if (ref >= pool.masterPageCount())
                    return false;
            }


            // ---------------------------------------------------------------
            // Master pages -> leaf pages.
            // ---------------------------------------------------------------

            for (const UnicodeDecompositionMasterPage& master :
                pool.masterPages())
            {
                for (uint32_t si = 0; si < kUnicodeSubsPerMaster; ++si)
                {
                    const UnicodeDecompositionPageRef ref =
                        master.sub[si];


                    if (ref == kUnicodeDecompositionPageEmpty)
                        continue;


                    if (ref >= pool.pageCount())
                        return false;
                }
            }


            // ---------------------------------------------------------------
            // Leaf pages -> decomposition records.
            // ---------------------------------------------------------------

            for (const UnicodeDecompositionPage& page :
                pool.pages())
            {
                for (uint32_t i = 0; i < kUnicodeSubSize; ++i)
                {
                    const UnicodeDecompositionRecordRef ref =
                        page.mapping[i];


                    if (ref == kUnicodeDecompositionRecordNone)
                        continue;


                    if (unicodeDecompositionRecordIndex(ref) >= recordCount)
                        return false;
                }
            }


            return true;
        }


        // ====================================================================
        // validateComposition
        //
        // Validate the complete canonical-composition record array before
        // serialization.
        //
        // This intentionally repeats the persistent invariants checked by
        // UnicodeDatabaseBuilder::setComposition().
        // ====================================================================

        [[nodiscard]]
        static bool validateComposition(
            const UnicodeDatabaseBuilder& database) noexcept
        {
            // ---------------------------------------------------------------
            // An absent composition dataset must not leave orphaned records.
            // ---------------------------------------------------------------

            if (!database.hasComposition())
                return database.compositionRecordCount() == 0;


            if (database.compositionRecordCount() == 0)
                return false;


            const std::vector<UnicodeCompositionRecord>& records =
                database.compositionRecords();


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
        // advanceArray
        // ====================================================================

        template <typename T>
        static bool advanceArray(uint64_t& cursor, size_t count) noexcept
        {
            const uint64_t count64 =
                static_cast<uint64_t>(count);


            if (count64 >
                std::numeric_limits<uint64_t>::max() /
                sizeof(T))
            {
                return false;
            }


            return advanceBytes(
                cursor,
                count64 * sizeof(T));
        }


        // ====================================================================
        // advanceBytes
        // ====================================================================

        static bool advanceBytes(uint64_t& cursor, uint64_t bytes) noexcept
        {
            constexpr uint64_t maximum =
                static_cast<uint64_t>(
                    std::numeric_limits<uint32_t>::max());


            if (bytes > maximum)
                return false;


            if (cursor > maximum - bytes)
                return false;


            cursor +=
                bytes;


            return true;
        }


        // ====================================================================
        // alignCursor
        //
        // alignment must be a power of two.
        // ====================================================================

        static bool alignCursor(uint64_t& cursor, uint32_t alignment) noexcept
        {
            if (alignment == 0)
                return false;


            if ((alignment & (alignment - 1u)) != 0)
                return false;


            const uint64_t mask =
                static_cast<uint64_t>(
                    alignment - 1u);


            constexpr uint64_t maximum =
                static_cast<uint64_t>(
                    std::numeric_limits<uint32_t>::max());


            if (cursor > maximum - mask)
                return false;


            cursor =
                (cursor + mask) &
                ~mask;


            return true;
        }


        // ====================================================================
        // fitsUint32
        // ====================================================================

        [[nodiscard]]
        static bool fitsUint32(uint64_t value) noexcept
        {
            return
                value <=
                static_cast<uint64_t>(
                    std::numeric_limits<uint32_t>::max());
        }


        // ====================================================================
        // copyArray
        //
        // Layout and persistent aggregate representations have already been
        // validated, so serialization is a direct raw copy.
        // ====================================================================

        template <typename T>
        static void copyArray(std::vector<uint8_t>& destination,
            uint32_t offset, const std::vector<T>& source) noexcept
        {
            if (source.empty())
                return;


            std::memcpy(
                destination.data() + offset,
                source.data(),
                source.size() * sizeof(T));
        }


        // ====================================================================
        // Host byte order
        // ====================================================================

        [[nodiscard]]
        static bool hostIsLittleEndian() noexcept
        {
            const uint16_t value =
                1;


            return
                *reinterpret_cast<const uint8_t*>(
                    &value) == 1;
        }
    };

} // namespace waavs