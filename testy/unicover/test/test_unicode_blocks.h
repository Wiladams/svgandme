#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#include "ucd_blocks_parser.h"
#include "unicode_coverage.h"


static bool testUnicodeBlocks(const char* blocksPath)
{
    using namespace waavs;


    // ========================================================================
    // Load Blocks.txt
    // ========================================================================

    std::ifstream input(
        blocksPath,
        std::ios::binary | std::ios::ate);

    if (!input)
    {
        std::cout
            << "Unable to open Blocks.txt: "
            << blocksPath
            << "\n";

        return false;
    }


    const std::streamsize fileSize =
        input.tellg();

    if (fileSize <= 0)
    {
        std::cout
            << "Blocks.txt is empty\n";

        return false;
    }


    input.seekg(
        0,
        std::ios::beg);


    std::vector<uint8_t> fileData(
        static_cast<size_t>(fileSize));


    if (!input.read(
        reinterpret_cast<char*>(fileData.data()),
        fileSize))
    {
        std::cout
            << "Failed to read Blocks.txt\n";

        return false;
    }


    ByteSpan source(
        fileData.data(),
        fileData.size());


    // ========================================================================
    // Parse
    // ========================================================================

    UnicodeDatabaseBuilder database;

    UCDBlocksParseResult parseResult;


    if (!ucdParseBlocks(
        source,
        database,
        parseResult))
    {
        std::cout
            << "Blocks.txt parse FAILED\n"
            << "  line:  "
            << parseResult.lineNumber
            << "\n"
            << "  error: "
            << ucdBlocksParseErrorString(
                parseResult.error)
            << "\n";

        return false;
    }


    // ========================================================================
    // Print basic database statistics
    // ========================================================================

    const UnicodeDatabaseBuilderStats& dbStats =
        database.stats();

    const UnicodePagePoolStats& pageStats =
        database.pagePool().stats();


    std::cout
        << "\n"
        << "============================================================\n"
        << "Unicode Blocks.txt parse\n"
        << "============================================================\n";

    std::cout
        << "Blocks parsed:           "
        << parseResult.blockCount
        << "\n";

    std::cout
        << "Block records:           "
        << database.blockCount()
        << "\n";

    std::cout
        << "Coverage records:        "
        << database.coverageCount()
        << "\n";

    std::cout
        << "String pool bytes:       "
        << database.stringPoolSize()
        << "\n";


    // ========================================================================
    // Coverage deduplication statistics
    // ========================================================================

    std::cout
        << "\n"
        << "Coverage deduplication\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "Coverage requests:       "
        << dbStats.coverageRequests
        << "\n";

    std::cout
        << "Unique coverages:        "
        << dbStats.uniqueCoverages
        << "\n";

    std::cout
        << "Coverage reused:         "
        << dbStats.coverageReused
        << "\n";

    std::cout
        << "Hash collisions:         "
        << dbStats.coverageHashCollisions
        << "\n";


    // ========================================================================
    // Bit-page statistics
    // ========================================================================

    std::cout
        << "\n"
        << "Bit page pool\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "Requests:                "
        << pageStats.bitPageRequests
        << "\n";

    std::cout
        << "Empty sentinels:         "
        << pageStats.bitPageEmpty
        << "\n";

    std::cout
        << "Full sentinels:          "
        << pageStats.bitPageFull
        << "\n";

    std::cout
        << "Reused pages:            "
        << pageStats.bitPageReused
        << "\n";

    std::cout
        << "Unique stored pages:     "
        << pageStats.uniqueBitPages
        << "\n";

    std::cout
        << "Hash collisions:         "
        << pageStats.bitPageHashCollisions
        << "\n";


    // ========================================================================
    // Master-page statistics
    // ========================================================================

    std::cout
        << "\n"
        << "Master page pool\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "Requests:                "
        << pageStats.masterPageRequests
        << "\n";

    std::cout
        << "Empty sentinels:         "
        << pageStats.masterPageEmpty
        << "\n";

    std::cout
        << "Full sentinels:          "
        << pageStats.masterPageFull
        << "\n";

    std::cout
        << "Reused pages:            "
        << pageStats.masterPageReused
        << "\n";

    std::cout
        << "Unique stored pages:     "
        << pageStats.uniqueMasterPages
        << "\n";

    std::cout
        << "Hash collisions:         "
        << pageStats.masterPageHashCollisions
        << "\n";


    // ========================================================================
    // String statistics
    // ========================================================================

    std::cout
        << "\n"
        << "String pool\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "String requests:         "
        << dbStats.stringRequests
        << "\n";

    std::cout
        << "Unique strings:          "
        << dbStats.uniqueStrings
        << "\n";

    std::cout
        << "Strings reused:          "
        << dbStats.stringReused
        << "\n";


    // ========================================================================
    // Physical page storage
    // ========================================================================

    std::cout
        << "\n"
        << "Physical page storage\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "Master pages:            "
        << database.pagePool().masterPageCount()
        << "\n";

    std::cout
        << "Bit pages:               "
        << database.pagePool().bitPageCount()
        << "\n";

    std::cout
        << "Total stored pages:      "
        << pageStats.storedPageCount()
        << "\n";

    std::cout
        << "Total page bytes:        "
        << pageStats.storedBytes()
        << "\n";


    // ========================================================================
    // Construct runtime pool view directly over builder storage
    //
    // This lets us test UnicodeCoverage before writing a .ucdb file.
    // ========================================================================

    UnicodeCoveragePools pools{};

    pools.masterPages =
        database.pagePool().masterPageData();

    pools.bitPages =
        database.pagePool().bitPageData();

    pools.masterPageCount =
        static_cast<uint32_t>(
            database.pagePool().masterPageCount());

    pools.bitPageCount =
        static_cast<uint32_t>(
            database.pagePool().bitPageCount());


    const auto& blocks =
        database.blocks();

    const auto& coverages =
        database.coverages();


    // ========================================================================
    // Verify every block
    //
    // Since Blocks.txt describes one contiguous range per block, coverage
    // should contain:
    //
    //      first
    //      midpoint
    //      last
    //
    // and should NOT contain:
    //
    //      first - 1
    //      last + 1
    //
    // where those code points exist.
    //
    // Also verify the exact covered-code-point count.
    // ========================================================================

    size_t verificationFailures = 0;


    for (size_t i = 0; i < blocks.size(); ++i)
    {
        const UnicodeBlockRecord& block =
            blocks[i];


        if (block.coverageIndex >=
            coverages.size())
        {
            std::cout
                << "Block "
                << i
                << ": invalid coverage index\n";

            ++verificationFailures;
            continue;
        }


        UnicodeCoverage coverage(
            &coverages[block.coverageIndex],
            &pools);


        bool good =
            true;


        // ---------------------------------------------------------------
        // Beginning
        // ---------------------------------------------------------------

        if (!coverage.contains(block.first))
            good = false;


        // ---------------------------------------------------------------
        // Middle
        // ---------------------------------------------------------------

        const uint32_t midpoint =
            block.first +
            ((block.last - block.first) >> 1);


        if (!coverage.contains(midpoint))
            good = false;


        // ---------------------------------------------------------------
        // End
        // ---------------------------------------------------------------

        if (!coverage.contains(block.last))
            good = false;


        // ---------------------------------------------------------------
        // Immediately before
        // ---------------------------------------------------------------

        if (block.first > 0)
        {
            if (coverage.contains(
                block.first - 1))
            {
                good = false;
            }
        }


        // ---------------------------------------------------------------
        // Immediately after
        // ---------------------------------------------------------------

        if (block.last + 1u <
            kUnicodeLimit)
        {
            if (coverage.contains(
                block.last + 1u))
            {
                good = false;
            }
        }


        // ---------------------------------------------------------------
        // Exact coverage size
        // ---------------------------------------------------------------

        const UnicodeCoverageStats stats =
            coverage.stats();


        const size_t expectedCodePoints =
            static_cast<size_t>(
                block.last -
                block.first +
                1u);


        if (stats.coveredCodePoints !=
            expectedCodePoints)
        {
            good = false;
        }


        if (!good)
        {
            ++verificationFailures;


            // Don't flood the console if something fundamental is broken.
            if (verificationFailures <= 10)
            {
                std::cout
                    << "Coverage verification failed for block "
                    << i
                    << "\n";

                std::cout
                    << "  range: U+"
                    << std::hex
                    << block.first
                    << "..U+"
                    << block.last
                    << std::dec
                    << "\n";

                std::cout
                    << "  expected code points: "
                    << expectedCodePoints
                    << "\n";

                std::cout
                    << "  actual code points:   "
                    << stats.coveredCodePoints
                    << "\n";
            }
        }
    }


    // ========================================================================
    // Final result
    // ========================================================================

    std::cout
        << "\n"
        << "Verification\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "Blocks checked:          "
        << blocks.size()
        << "\n";

    std::cout
        << "Failures:                "
        << verificationFailures
        << "\n";


    if (verificationFailures != 0)
    {
        std::cout
            << "\n"
            << "Blocks.txt test: FAILED\n";

        return false;
    }


    std::cout
        << "\n"
        << "Blocks.txt test: PASS\n";


    return true;
}

