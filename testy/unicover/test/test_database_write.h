#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

#include "ucd_blocks_parser.h"
#include "unicode_database.h"
#include "unicode_database_writer.h"


static bool readFile(const char* filename, std::vector<uint8_t>& data)
{
    std::ifstream input(filename, std::ios::binary | std::ios::ate);

    if (!input)
    {
        printf("FAILED: unable to open file\n");
        printf("  file: %s\n", filename);
        return false;
    }

    const std::streamsize size = input.tellg();

    if (size <= 0)
    {
        printf("FAILED: file is empty\n");
        printf("  file: %s\n", filename);
        return false;
    }

    input.seekg(0, std::ios::beg);

    data.resize(static_cast<size_t>(size));

    if (!input.read(reinterpret_cast<char*>(data.data()), size))
    {
        printf("FAILED: unable to read file\n");
        printf("  file: %s\n", filename);
        return false;
    }

    return true;
}


static bool testUnicodeDatabaseRoundTrip(const char* blocksPath, const char* databasePath)
{
    using namespace waavs;

    printf("\n");
    printf("============================================================\n");
    printf("Unicode Database Round Trip\n");
    printf("============================================================\n");


    // ========================================================================
    // Read Blocks.txt
    // ========================================================================

    printf("\n[1] Reading Blocks.txt\n");

    std::vector<uint8_t> blocksData;

    if (!readFile(blocksPath, blocksData))
        return false;

    printf("  file:  %s\n", blocksPath);
    printf("  bytes: %zu\n", blocksData.size());
    printf("  PASS\n");


    // ========================================================================
    // Parse Blocks.txt
    // ========================================================================

    printf("\n[2] Parsing Blocks.txt\n");

    ByteSpan source(blocksData.data(), blocksData.size());

    UnicodeDatabaseBuilder builder;
    UCDBlocksParseResult parseResult;

    if (!ucdParseBlocks(source, builder, parseResult))
    {
        printf("  FAILED\n");
        printf("  line:  %u\n", parseResult.lineNumber);
        printf("  error: %s\n", ucdBlocksParseErrorString(parseResult.error));
        return false;
    }

    printf("  blocks:        %zu\n", builder.blockCount());
    printf("  coverages:     %zu\n", builder.coverageCount());
    printf("  master pages:  %zu\n", builder.pagePool().masterPageCount());
    printf("  bit pages:     %zu\n", builder.pagePool().bitPageCount());
    printf("  string bytes:  %zu\n", builder.stringPoolSize());
    printf("  PASS\n");


    // ========================================================================
    // Write database
    // ========================================================================

    printf("\n[3] Writing database\n");
    printf("  file: %s\n", databasePath);

    UnicodeDatabaseWriteResult writeResult;

    if (!UnicodeDatabaseWriter::writeFile(databasePath, builder, 17, 0, 0, writeResult))
    {
        printf("  FAILED\n");
        printf("  error: %s\n", unicodeDatabaseWriteErrorString(writeResult.error));
        return false;
    }

    printf("  database bytes: %u\n", writeResult.databaseSize);
    printf("  PASS\n");


    // ========================================================================
    // Read database back
    // ========================================================================

    printf("\n[4] Reading database back\n");

    std::vector<uint8_t> databaseData;

    if (!readFile(databasePath, databaseData))
        return false;

    printf("  bytes read: %zu\n", databaseData.size());

    if (databaseData.size() != writeResult.databaseSize)
    {
        printf("  FAILED: written size does not match file size\n");
        printf("  writer size: %u\n", writeResult.databaseSize);
        printf("  file size:   %zu\n", databaseData.size());
        return false;
    }

    printf("  PASS\n");


    // ========================================================================
    // Inspect raw header before calling UnicodeDatabase::reset()
    //
    // This is especially useful if reset() itself fails.
    // ========================================================================

    printf("\n[5] Inspecting raw database header\n");

    if (databaseData.size() < sizeof(UnicodeDatabaseHeader))
    {
        printf("  FAILED: file smaller than UnicodeDatabaseHeader\n");
        printf("  file size:   %zu\n", databaseData.size());
        printf("  header size: %zu\n", sizeof(UnicodeDatabaseHeader));
        return false;
    }

    const UnicodeDatabaseHeader* rawHeader =
        reinterpret_cast<const UnicodeDatabaseHeader*>(databaseData.data());

    printf("  signature:       %c%c%c%c\n",
        rawHeader->signature[0],
        rawHeader->signature[1],
        rawHeader->signature[2],
        rawHeader->signature[3]);

    printf("  format:          %u.%u\n",
        rawHeader->formatMajor,
        rawHeader->formatMinor);

    printf("  Unicode:         %u.%u.%u\n",
        rawHeader->unicodeMajor,
        rawHeader->unicodeMinor,
        rawHeader->unicodePatch);

    printf("  header size:     %u\n", rawHeader->headerSize);
    printf("  database size:   %u\n", rawHeader->databaseSize);

    printf("  blocks:          offset=%u count=%u\n",
        rawHeader->blockOffset,
        rawHeader->blockCount);

    printf("  scripts:         offset=%u count=%u\n",
        rawHeader->scriptOffset,
        rawHeader->scriptCount);

    printf("  properties:      offset=%u count=%u\n",
        rawHeader->propertyOffset,
        rawHeader->propertyCount);

    printf("  coverages:       offset=%u count=%u\n",
        rawHeader->coverageOffset,
        rawHeader->coverageCount);

    printf("  master pages:    offset=%u count=%u\n",
        rawHeader->masterPageOffset,
        rawHeader->masterPageCount);

    printf("  bit pages:       offset=%u count=%u\n",
        rawHeader->bitPageOffset,
        rawHeader->bitPageCount);

    printf("  string pool:     offset=%u size=%u\n",
        rawHeader->stringPoolOffset,
        rawHeader->stringPoolSize);

    printf("  master aligned:  %s\n",
        (rawHeader->masterPageOffset % kUnicodeDatabasePoolAlignment) == 0
        ? "yes"
        : "NO");

    printf("  bit aligned:     %s\n",
        (rawHeader->bitPageOffset % kUnicodeDatabasePoolAlignment) == 0
        ? "yes"
        : "NO");

    printf("  PASS\n");


    // ========================================================================
    // Load through UnicodeDatabase
    // ========================================================================

    printf("\n[6] Loading UnicodeDatabase\n");

    ByteSpan databaseSpan(databaseData.data(), databaseData.size());

    UnicodeDatabase database;

    if (!database.reset(databaseSpan))
    {
        printf("  FAILED: UnicodeDatabase::reset() returned false\n");
        printf("  Raw header above should help identify the format check that failed.\n");
        return false;
    }

    printf("  PASS\n");


    // ========================================================================
    // Basic metadata comparison
    // ========================================================================

    printf("\n[7] Comparing database metadata\n");

    if (database.unicodeMajor() != 17)
    {
        printf("  FAILED: Unicode major version\n");
        printf("  expected: 17\n");
        printf("  actual:   %u\n", database.unicodeMajor());
        return false;
    }

    if (database.unicodeMinor() != 0)
    {
        printf("  FAILED: Unicode minor version\n");
        printf("  expected: 0\n");
        printf("  actual:   %u\n", database.unicodeMinor());
        return false;
    }

    if (database.unicodePatch() != 0)
    {
        printf("  FAILED: Unicode patch version\n");
        printf("  expected: 0\n");
        printf("  actual:   %u\n", database.unicodePatch());
        return false;
    }

    if (database.coverageCount() != builder.coverageCount())
    {
        printf("  FAILED: coverage count\n");
        printf("  expected: %zu\n", builder.coverageCount());
        printf("  actual:   %u\n", database.coverageCount());
        return false;
    }

    if (database.blockCount() != builder.blockCount())
    {
        printf("  FAILED: block count\n");
        printf("  expected: %zu\n", builder.blockCount());
        printf("  actual:   %u\n", database.blockCount());
        return false;
    }

    if (database.scriptCount() != builder.scriptCount())
    {
        printf("  FAILED: script count\n");
        printf("  expected: %zu\n", builder.scriptCount());
        printf("  actual:   %u\n", database.scriptCount());
        return false;
    }

    if (database.propertyCount() != builder.propertyCount())
    {
        printf("  FAILED: property count\n");
        printf("  expected: %zu\n", builder.propertyCount());
        printf("  actual:   %u\n", database.propertyCount());
        return false;
    }

    if (database.masterPageCount() != builder.pagePool().masterPageCount())
    {
        printf("  FAILED: master page count\n");
        printf("  expected: %zu\n", builder.pagePool().masterPageCount());
        printf("  actual:   %u\n", database.masterPageCount());
        return false;
    }

    if (database.bitPageCount() != builder.pagePool().bitPageCount())
    {
        printf("  FAILED: bit page count\n");
        printf("  expected: %zu\n", builder.pagePool().bitPageCount());
        printf("  actual:   %u\n", database.bitPageCount());
        return false;
    }

    printf("  PASS\n");


    // ========================================================================
    // Coverage records
    // ========================================================================

    printf("\n[8] Comparing UnicodeCoverageData records\n");

    const auto& sourceCoverages = builder.coverages();

    for (size_t i = 0; i < sourceCoverages.size(); ++i)
    {
        const UnicodeCoverageData* loaded =
            database.coverageData(static_cast<UnicodeCoverageIndex>(i));

        if (!loaded)
        {
            printf("  FAILED: coverageData() returned nullptr\n");
            printf("  index: %zu\n", i);
            return false;
        }

        if (std::memcmp(&sourceCoverages[i], loaded, sizeof(UnicodeCoverageData)) != 0)
        {
            printf("  FAILED: coverage record mismatch\n");
            printf("  index: %zu\n", i);
            return false;
        }
    }

    printf("  compared: %zu\n", sourceCoverages.size());
    printf("  PASS\n");


    // ========================================================================
    // Block records
    // ========================================================================

    printf("\n[9] Comparing UnicodeBlockRecord records\n");

    const auto& sourceBlocks = builder.blocks();

    for (size_t i = 0; i < sourceBlocks.size(); ++i)
    {
        const UnicodeBlockRecord* loaded =
            database.blockRecord(static_cast<uint32_t>(i));

        if (!loaded)
        {
            printf("  FAILED: blockRecord() returned nullptr\n");
            printf("  index: %zu\n", i);
            return false;
        }

        if (std::memcmp(&sourceBlocks[i], loaded, sizeof(UnicodeBlockRecord)) != 0)
        {
            printf("  FAILED: block record mismatch\n");
            printf("  index: %zu\n", i);

            printf("  source range: U+%04X..U+%04X\n",
                sourceBlocks[i].first,
                sourceBlocks[i].last);

            printf("  loaded range: U+%04X..U+%04X\n",
                loaded->first,
                loaded->last);

            printf("  source name offset: %u\n", sourceBlocks[i].nameOffset);
            printf("  loaded name offset: %u\n", loaded->nameOffset);

            printf("  source coverage: %u\n", sourceBlocks[i].coverageIndex);
            printf("  loaded coverage: %u\n", loaded->coverageIndex);

            return false;
        }
    }

    printf("  compared: %zu\n", sourceBlocks.size());
    printf("  PASS\n");


    // ========================================================================
    // Script records
    // ========================================================================

    printf("\n[10] Comparing UnicodeScriptRecord records\n");

    const auto& sourceScripts = builder.scripts();

    for (size_t i = 0; i < sourceScripts.size(); ++i)
    {
        const UnicodeScriptRecord* loaded =
            database.scriptRecord(static_cast<uint32_t>(i));

        if (!loaded)
        {
            printf("  FAILED: scriptRecord() returned nullptr\n");
            printf("  index: %zu\n", i);
            return false;
        }

        if (std::memcmp(&sourceScripts[i], loaded, sizeof(UnicodeScriptRecord)) != 0)
        {
            printf("  FAILED: script record mismatch\n");
            printf("  index: %zu\n", i);
            return false;
        }
    }

    printf("  compared: %zu\n", sourceScripts.size());
    printf("  PASS\n");


    // ========================================================================
    // Property records
    // ========================================================================

    printf("\n[11] Comparing UnicodePropertyRecord records\n");

    const auto& sourceProperties = builder.properties();

    for (size_t i = 0; i < sourceProperties.size(); ++i)
    {
        const UnicodePropertyRecord* loaded =
            database.propertyRecord(static_cast<uint32_t>(i));

        if (!loaded)
        {
            printf("  FAILED: propertyRecord() returned nullptr\n");
            printf("  index: %zu\n", i);
            return false;
        }

        if (std::memcmp(&sourceProperties[i], loaded, sizeof(UnicodePropertyRecord)) != 0)
        {
            printf("  FAILED: property record mismatch\n");
            printf("  index: %zu\n", i);
            return false;
        }
    }

    printf("  compared: %zu\n", sourceProperties.size());
    printf("  PASS\n");


    // ========================================================================
    // Master-page pool
    // ========================================================================

    printf("\n[12] Comparing UnicodeMasterPage pool\n");

    const UnicodeCoveragePools& loadedPools = database.pools();
    const auto& sourceMasterPages = builder.pagePool().masterPages();

    if (sourceMasterPages.size() != loadedPools.masterPageCount)
    {
        printf("  FAILED: master page count mismatch\n");
        printf("  expected: %zu\n", sourceMasterPages.size());
        printf("  actual:   %u\n", loadedPools.masterPageCount);
        return false;
    }

    for (size_t i = 0; i < sourceMasterPages.size(); ++i)
    {
        if (std::memcmp(
            &sourceMasterPages[i],
            &loadedPools.masterPages[i],
            sizeof(UnicodeMasterPage)) != 0)
        {
            printf("  FAILED: master page mismatch\n");
            printf("  index: %zu\n", i);
            return false;
        }
    }

    printf("  compared: %zu pages\n", sourceMasterPages.size());
    printf("  PASS\n");


    // ========================================================================
    // Bit-page pool
    // ========================================================================

    printf("\n[13] Comparing UnicodeBitPage pool\n");

    const auto& sourceBitPages = builder.pagePool().bitPages();

    if (sourceBitPages.size() != loadedPools.bitPageCount)
    {
        printf("  FAILED: bit page count mismatch\n");
        printf("  expected: %zu\n", sourceBitPages.size());
        printf("  actual:   %u\n", loadedPools.bitPageCount);
        return false;
    }

    for (size_t i = 0; i < sourceBitPages.size(); ++i)
    {
        if (std::memcmp(
            &sourceBitPages[i],
            &loadedPools.bitPages[i],
            sizeof(UnicodeBitPage)) != 0)
        {
            printf("  FAILED: bit page mismatch\n");
            printf("  index: %zu\n", i);
            return false;
        }
    }

    printf("  compared: %zu pages\n", sourceBitPages.size());
    printf("  PASS\n");


    // ========================================================================
    // String pool
    // ========================================================================

    printf("\n[14] Comparing string pool\n");

    const UnicodeDatabaseHeader* header = database.header();

    if (!header)
    {
        printf("  FAILED: database.header() returned nullptr\n");
        return false;
    }

    if (header->stringPoolSize != builder.stringPoolSize())
    {
        printf("  FAILED: string pool size mismatch\n");
        printf("  expected: %zu\n", builder.stringPoolSize());
        printf("  actual:   %u\n", header->stringPoolSize);
        return false;
    }

    const uint8_t* loadedStringPool =
        database.data().data() + header->stringPoolOffset;

    if (std::memcmp(
        builder.stringPool().data(),
        loadedStringPool,
        builder.stringPoolSize()) != 0)
    {
        printf("  FAILED: string pool contents differ\n");

        for (size_t i = 0; i < builder.stringPoolSize(); ++i)
        {
            if (builder.stringPool()[i] != loadedStringPool[i])
            {
                printf("  first mismatch at byte: %zu\n", i);
                printf("  expected: 0x%02X\n", builder.stringPool()[i]);
                printf("  actual:   0x%02X\n", loadedStringPool[i]);
                break;
            }
        }

        return false;
    }

    printf("  compared: %zu bytes\n", builder.stringPoolSize());
    printf("  PASS\n");


    // ========================================================================
    // Semantic block coverage
    // ========================================================================

    printf("\n[15] Verifying loaded block coverage\n");

    for (uint32_t i = 0; i < database.blockCount(); ++i)
    {
        const UnicodeBlockRecord* block = database.blockRecord(i);

        if (!block)
        {
            printf("  FAILED: blockRecord() returned nullptr\n");
            printf("  block index: %u\n", i);
            return false;
        }

        UnicodeCoverage coverage = database.blockCoverage(i);

        if (!coverage)
        {
            printf("  FAILED: blockCoverage() returned invalid coverage\n");
            printf("  block index: %u\n", i);
            printf("  range: U+%04X..U+%04X\n", block->first, block->last);
            return false;
        }


        // ---------------------------------------------------------------
        // First
        // ---------------------------------------------------------------

        if (!coverage.contains(block->first))
        {
            printf("  FAILED: first code point missing\n");
            printf("  block index: %u\n", i);
            printf("  code point: U+%04X\n", block->first);
            return false;
        }


        // ---------------------------------------------------------------
        // Midpoint
        // ---------------------------------------------------------------

        const uint32_t midpoint =
            block->first + ((block->last - block->first) >> 1);

        if (!coverage.contains(midpoint))
        {
            printf("  FAILED: midpoint missing\n");
            printf("  block index: %u\n", i);
            printf("  code point: U+%04X\n", midpoint);
            return false;
        }


        // ---------------------------------------------------------------
        // Last
        // ---------------------------------------------------------------

        if (!coverage.contains(block->last))
        {
            printf("  FAILED: last code point missing\n");
            printf("  block index: %u\n", i);
            printf("  code point: U+%04X\n", block->last);
            return false;
        }


        // ---------------------------------------------------------------
        // Immediately before
        // ---------------------------------------------------------------

        if (block->first > 0 && coverage.contains(block->first - 1u))
        {
            printf("  FAILED: coverage extends before block\n");
            printf("  block index: %u\n", i);
            printf("  unexpected code point: U+%04X\n", block->first - 1u);
            return false;
        }


        // ---------------------------------------------------------------
        // Immediately after
        // ---------------------------------------------------------------

        if (block->last + 1u < kUnicodeLimit &&
            coverage.contains(block->last + 1u))
        {
            printf("  FAILED: coverage extends after block\n");
            printf("  block index: %u\n", i);
            printf("  unexpected code point: U+%04X\n", block->last + 1u);
            return false;
        }


        // ---------------------------------------------------------------
        // Exact coverage count
        // ---------------------------------------------------------------

        const UnicodeCoverageStats stats = coverage.stats();
        const size_t expectedCount =
            static_cast<size_t>(block->last - block->first + 1u);

        if (stats.coveredCodePoints != expectedCount)
        {
            printf("  FAILED: incorrect coverage size\n");
            printf("  block index: %u\n", i);
            printf("  range: U+%04X..U+%04X\n", block->first, block->last);
            printf("  expected: %zu\n", expectedCount);
            printf("  actual:   %zu\n", stats.coveredCodePoints);
            return false;
        }
    }

    printf("  verified: %u blocks\n", database.blockCount());
    printf("  PASS\n");


    // ========================================================================
    // Complete
    // ========================================================================

    printf("\n");
    printf("============================================================\n");
    printf("Unicode Database Round Trip: PASS\n");
    printf("============================================================\n");

    return true;
}
