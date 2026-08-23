// unicode_database_writer.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
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
    //
    // databaseSize is valid after successful serialization.
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

    static inline const char* unicodeDatabaseWriteErrorString(UnicodeDatabaseWriteError error) noexcept
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

        case UnicodeDatabaseWriteError::TooManyBlocks:
            return "too many Unicode block records";

        case UnicodeDatabaseWriteError::TooManyScripts:
            return "too many Unicode script records";

        case UnicodeDatabaseWriteError::TooManyProperties:
            return "too many Unicode property records";

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
    // Version 1 format:
    //
    //      UnicodeDatabaseHeader
    //      UnicodeBlockRecord[]
    //      UnicodeScriptRecord[]
    //      UnicodePropertyRecord[]
    //      UnicodeCoverageData[]
    //
    //      alignment to kUnicodeDatabasePoolAlignment
    //
    //      UnicodeMasterPage[]
    //
    //      alignment to kUnicodeDatabasePoolAlignment
    //
    //      UnicodeBitPage[]
    //
    //      UTF-8 string pool
    //
    //
    // The page pools are already canonical and deduplicated by the builder.
    // The writer performs no conversion or further deduplication.
    //
    // ========================================================================

    class UnicodeDatabaseWriter
    {
    private:
        // ====================================================================
        // Layout
        //
        // Calculated completely before any output buffer is allocated.
        // ====================================================================

        struct Layout
        {
            uint32_t blockOffset{ 0 };
            uint32_t scriptOffset{ 0 };
            uint32_t propertyOffset{ 0 };
            uint32_t coverageOffset{ 0 };

            uint32_t masterPageOffset{ 0 };
            uint32_t bitPageOffset{ 0 };

            uint32_t stringPoolOffset{ 0 };

            uint32_t databaseSize{ 0 };
        };


    public:
        // ====================================================================
        // write
        //
        // Serialize the complete builder into an in-memory database image.
        //
        // unicodeMajor / Minor / Patch describe the UCD source data used to
        // construct the database.  For the current data this will be:
        //
        //      17, 0, 0
        //
        // outData is replaced only after layout validation succeeds.
        // ====================================================================

        static bool write(const UnicodeDatabaseBuilder& database,
            uint16_t unicodeMajor, uint16_t unicodeMinor, uint16_t unicodePatch,
            std::vector<uint8_t>& outData, UnicodeDatabaseWriteResult& outResult)
        {
            outResult = {};


            // ---------------------------------------------------------------
            // Version 1 is a canonical little-endian format.
            //
            // We currently write the canonical aggregate representations
            // directly, so generation must occur on a little-endian host.
            // ---------------------------------------------------------------

            if (!hostIsLittleEndian())
            {
                outResult.error =
                    UnicodeDatabaseWriteError::UnsupportedByteOrder;

                return false;
            }


            // ---------------------------------------------------------------
            // Validate section counts against persistent field widths.
            // ---------------------------------------------------------------

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


            if (database.stringPoolSize() >
                std::numeric_limits<uint32_t>::max())
            {
                outResult.error =
                    UnicodeDatabaseWriteError::StringPoolTooLarge;

                return false;
            }


            // ---------------------------------------------------------------
            // String pool offset zero must represent the empty string.
            // ---------------------------------------------------------------

            if (database.stringPool().empty() ||
                database.stringPool()[0] != 0)
            {
                outResult.error =
                    UnicodeDatabaseWriteError::InvalidStringPool;

                return false;
            }


            // ---------------------------------------------------------------
            // Calculate complete physical layout.
            // ---------------------------------------------------------------

            Layout layout{};


            if (!calculateLayout(database, layout))
            {
                outResult.error =
                    UnicodeDatabaseWriteError::DatabaseTooLarge;

                return false;
            }


            // ---------------------------------------------------------------
            // Allocate the complete database in one operation.
            //
            // Zero initialization deliberately leaves alignment padding and
            // any otherwise unwritten bytes deterministic.
            // ---------------------------------------------------------------

            std::vector<uint8_t> result(
                static_cast<size_t>(layout.databaseSize),
                uint8_t(0));


            // ---------------------------------------------------------------
            // Construct header.
            // ---------------------------------------------------------------

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

            header.reserved0 =
                0;


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
            // Coverage records
            // ---------------------------------------------------------------

            header.coverageOffset =
                layout.coverageOffset;

            header.coverageCount =
                static_cast<uint32_t>(
                    database.coverageCount());


            // ---------------------------------------------------------------
            // Master-page pool
            // ---------------------------------------------------------------

            header.masterPageOffset =
                layout.masterPageOffset;

            header.masterPageCount =
                static_cast<uint32_t>(
                    database.pagePool().masterPageCount());


            // ---------------------------------------------------------------
            // Bit-page pool
            // ---------------------------------------------------------------

            header.bitPageOffset =
                layout.bitPageOffset;

            header.bitPageCount =
                static_cast<uint32_t>(
                    database.pagePool().bitPageCount());


            // ---------------------------------------------------------------
            // Block directory
            // ---------------------------------------------------------------

            header.blockOffset =
                layout.blockOffset;

            header.blockCount =
                static_cast<uint32_t>(
                    database.blockCount());


            // ---------------------------------------------------------------
            // Script directory
            // ---------------------------------------------------------------

            header.scriptOffset =
                layout.scriptOffset;

            header.scriptCount =
                static_cast<uint32_t>(
                    database.scriptCount());


            // ---------------------------------------------------------------
            // Property directory
            // ---------------------------------------------------------------

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
            // reserved[] remains zero because header was value-initialized.
            // ---------------------------------------------------------------


            // ---------------------------------------------------------------
            // Write header.
            // ---------------------------------------------------------------

            std::memcpy(
                result.data(),
                &header,
                sizeof(header));


            // ---------------------------------------------------------------
            // Write semantic directories.
            // ---------------------------------------------------------------

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


            // ---------------------------------------------------------------
            // Write coverage records.
            // ---------------------------------------------------------------

            copyArray(
                result,
                layout.coverageOffset,
                database.coverages());


            // ---------------------------------------------------------------
            // Write shared page pools.
            // ---------------------------------------------------------------

            copyArray(
                result,
                layout.masterPageOffset,
                database.pagePool().masterPages());


            copyArray(
                result,
                layout.bitPageOffset,
                database.pagePool().bitPages());


            // ---------------------------------------------------------------
            // Write string pool.
            // ---------------------------------------------------------------

            if (!database.stringPool().empty())
            {
                std::memcpy(
                    result.data() + layout.stringPoolOffset,
                    database.stringPool().data(),
                    database.stringPool().size());
            }


            // ---------------------------------------------------------------
            // Commit.
            // ---------------------------------------------------------------

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
        //
        // Serialize and write directly to a .ucdb file.
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
        // Perform layout calculations using uint64_t so overflow can be
        // detected before values are narrowed into persistent uint32 fields.
        // ====================================================================

        static bool calculateLayout(const UnicodeDatabaseBuilder& database,
            Layout& outLayout) noexcept
        {
            uint64_t cursor =
                sizeof(UnicodeDatabaseHeader);


            // ---------------------------------------------------------------
            // Blocks
            // ---------------------------------------------------------------

            outLayout.blockOffset =
                static_cast<uint32_t>(cursor);


            if (!advanceArray<UnicodeBlockRecord>(
                cursor,
                database.blockCount()))
            {
                return false;
            }


            // ---------------------------------------------------------------
            // Scripts
            // ---------------------------------------------------------------

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


            // ---------------------------------------------------------------
            // Properties
            // ---------------------------------------------------------------

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


            // ---------------------------------------------------------------
            // Coverages
            // ---------------------------------------------------------------

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


            // ---------------------------------------------------------------
            // Master-page pool
            //
            // Its file offset is explicitly aligned even though the C++
            // structure itself need not have 64-byte type alignment.
            // ---------------------------------------------------------------

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


            // ---------------------------------------------------------------
            // Bit-page pool
            // ---------------------------------------------------------------

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


            // ---------------------------------------------------------------
            // String pool
            // ---------------------------------------------------------------

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


            // ---------------------------------------------------------------
            // Final database size
            // ---------------------------------------------------------------

            if (!fitsUint32(cursor))
                return false;


            outLayout.databaseSize =
                static_cast<uint32_t>(cursor);


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


            const uint64_t bytes =
                count64 *
                sizeof(T);


            return advanceBytes( cursor, bytes);
        }


        // ====================================================================
        // advanceBytes
        // ====================================================================

        static bool advanceBytes(uint64_t& cursor, uint64_t bytes) noexcept
        {
            if (bytes > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()))
            {
                return false;
            }


            if (cursor > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) - bytes)
            {
                return false;
            }

            cursor += bytes;

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
            {
                return false;
            }


            const uint64_t mask = static_cast<uint64_t>( alignment - 1u);


            if (cursor > static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) - mask)
            {
                return false;
            }


            cursor = (cursor + mask) & ~mask;

            return true;
        }


        // ====================================================================
        // fitsUint32
        // ====================================================================

        static bool fitsUint32(uint64_t value) noexcept
        {
            return value <= static_cast<uint64_t>(std::numeric_limits<uint32_t>::max());
        }


        // ====================================================================
        // copyArray
        //
        // The layout has already been validated, so this is intentionally a
        // simple raw copy of the persistent aggregate representation.
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
            const uint16_t value = 1;

            return *reinterpret_cast<const uint8_t*>(&value) == 1;
        }
    };

} // namespace waavs
