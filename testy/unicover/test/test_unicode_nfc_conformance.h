// test_unicode_nfc_conformance.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "unicode_database.h"
#include "unicode_normalizer.h"


namespace waavs
{
    // ========================================================================
    // Basic text helpers
    // ========================================================================

    static bool unicodeNFCTestIsSpace(char ch) noexcept
    {
        return ch == ' ' ||
            ch == '\t' ||
            ch == '\r' ||
            ch == '\n';
    }


    static int unicodeNFCTestHexValue(char ch) noexcept
    {
        if (ch >= '0' && ch <= '9')
            return ch - '0';

        if (ch >= 'A' && ch <= 'F')
            return ch - 'A' + 10;

        if (ch >= 'a' && ch <= 'f')
            return ch - 'a' + 10;

        return -1;
    }


    static void unicodeNFCTestTrim(const char*& begin, const char*& end) noexcept
    {
        while (begin < end &&
            unicodeNFCTestIsSpace(*begin))
        {
            ++begin;
        }

        while (end > begin &&
            unicodeNFCTestIsSpace(*(end - 1)))
        {
            --end;
        }
    }


    // ========================================================================
    // unicodeNFCTestParseSequence
    //
    // Parse one code-point sequence such as:
    //
    //      0041 0323 0300
    //
    // into a vector of Unicode code points.
    // ========================================================================

    static bool unicodeNFCTestParseSequence(
        const char* begin,
        const char* end,
        std::vector<uint32_t>& out)
    {
        out.clear();

        unicodeNFCTestTrim(
            begin,
            end);


        while (begin < end)
        {
            while (begin < end &&
                unicodeNFCTestIsSpace(*begin))
            {
                ++begin;
            }


            if (begin == end)
                break;


            uint32_t cp = 0;
            uint32_t digits = 0;


            while (begin < end)
            {
                const int digit =
                    unicodeNFCTestHexValue(*begin);

                if (digit < 0)
                    break;


                if (digits >= 8)
                    return false;


                cp =
                    (cp << 4) |
                    static_cast<uint32_t>(digit);

                ++digits;
                ++begin;
            }


            if (digits == 0)
                return false;


            if (cp >= kUnicodeLimit)
                return false;


            out.push_back(cp);


            if (begin < end &&
                !unicodeNFCTestIsSpace(*begin))
            {
                return false;
            }
        }


        return !out.empty();
    }


    // ========================================================================
    // unicodeNFCTestParsePart
    //
    // Parse:
    //
    //      @Part0
    //      @Part1
    //      @Part2
    //      ...
    //
    // No assumption is made about the highest Part present in the file.
    // ========================================================================

    static bool unicodeNFCTestParsePart(
        const char* begin,
        const char* end,
        uint32_t& outPart) noexcept
    {
        static constexpr char prefix[] = "@Part";
        static constexpr size_t prefixLength =
            sizeof(prefix) - 1;


        if (static_cast<size_t>(end - begin) <=
            prefixLength)
        {
            return false;
        }


        if (std::memcmp(
            begin,
            prefix,
            prefixLength) != 0)
        {
            return false;
        }


        const char* p =
            begin + prefixLength;

        uint32_t part = 0;
        uint32_t digits = 0;


        while (p < end)
        {
            const char ch = *p++;

            if (ch < '0' || ch > '9')
                return false;


            if (part >
                (UINT32_MAX -
                    static_cast<uint32_t>(ch - '0')) / 10u)
            {
                return false;
            }


            part =
                part * 10u +
                static_cast<uint32_t>(ch - '0');

            ++digits;
        }


        if (digits == 0)
            return false;


        outPart = part;

        return true;
    }


    // ========================================================================
    // Sequence equality
    // ========================================================================

    static bool unicodeNFCTestEquals(
        const uint32_t* actual,
        size_t actualCount,
        const uint32_t* expected,
        size_t expectedCount) noexcept
    {
        if (actualCount != expectedCount)
            return false;


        for (size_t i = 0; i < actualCount; ++i) {
            if (actual[i] != expected[i])
                return false;
        }


        return true;
    }


    // ========================================================================
    // Diagnostic printing
    // ========================================================================

    static void unicodeNFCTestPrintSequence(
        const char* label,
        const uint32_t* codePoints,
        size_t count)
    {
        std::printf(
            "  %-12s",
            label);


        if (count == 0)
        {
            std::printf("<empty>\n");
            return;
        }


        for (size_t i = 0; i < count; ++i)
            std::printf(" U+%04X", codePoints[i]);


        std::printf("\n");
    }


    // ========================================================================
    // testUnicodeNFCConformance
    //
    // Test the NFC portion of Unicode NormalizationTest.txt.
    //
    // Columns:
    //
    //      c1 = source
    //      c2 = NFC
    //      c3 = NFD
    //      c4 = NFKC
    //      c5 = NFKD
    //
    //
    // Official NFC invariants:
    //
    //      c2 == NFC(c1)
    //      c2 == NFC(c2)
    //      c2 == NFC(c3)
    //
    //      c4 == NFC(c4)
    //      c4 == NFC(c5)
    //
    //
    // Part 1 contains character-by-character normalization data.
    //
    // After all explicit Part 1 records have been tested, every other Unicode
    // code-point position is also verified to remain unchanged by this runtime
    // NFC implementation.
    //
    // This goes slightly beyond the formal conformance requirement, which
    // applies the Part 1 complement rule to assigned code points.
    // ========================================================================

    static bool testUnicodeNFCConformance(
        const char* databaseFilename,
        const char* normalizationTestFilename)
    {
        auto fail = [](const char* message) -> bool {
            std::printf(
                "Unicode NFC conformance: FAIL: %s\n",
                message);

            return false;
            };


        // ====================================================================
        // Load Unicode database
        // ====================================================================

        std::ifstream databaseInput(
            databaseFilename,
            std::ios::binary |
            std::ios::ate);


        if (!databaseInput)
            return fail("unable to open Unicode database");


        const std::streampos databaseEnd =
            databaseInput.tellg();


        if (databaseEnd <= 0)
            return fail("Unicode database is empty");


        std::vector<uint8_t> databaseBytes(
            static_cast<size_t>(databaseEnd));


        databaseInput.seekg(
            0,
            std::ios::beg);


        databaseInput.read(
            reinterpret_cast<char*>(
                databaseBytes.data()),
            static_cast<std::streamsize>(
                databaseBytes.size()));


        if (!databaseInput)
            return fail("unable to read Unicode database");


        UnicodeDatabase database;


        if (!database.reset(
            ByteSpan(
                databaseBytes.data(),
                databaseBytes.size())))
        {
            return fail(
                "UnicodeDatabase rejected database");
        }


        UnicodeNormalizer normalizer(
            database);


        if (!normalizer)
            return fail("UnicodeNormalizer is unavailable");


        if (!normalizer.nfcAvailable())
            return fail("Unicode NFC normalization is unavailable");


        // ====================================================================
        // Open NormalizationTest.txt
        // ====================================================================

        std::ifstream testInput(
            normalizationTestFilename);


        if (!testInput)
            return fail(
                "unable to open NormalizationTest.txt");


        // ====================================================================
        // Test-file version
        //
        // Unicode 17 currently begins:
        //
        //      # NormalizationTest-17.0.0.txt
        //
        // The test itself does not hard-code 17.0.0. Instead it verifies that
        // the test-file version agrees exactly with the loaded database.
        // ====================================================================

        bool testVersionFound = false;

        uint32_t testMajor = 0;
        uint32_t testMinor = 0;
        uint32_t testPatch = 0;


        // ====================================================================
        // Section state
        // ====================================================================

        static constexpr uint32_t kMaximumTestParts = 32;


        bool sawAnyPart = false;
        bool sawPart1 = false;

        uint32_t currentPart =
            kMaximumTestParts;

        uint32_t highestPart = 0;

        size_t partRows[kMaximumTestParts]{};


        // ====================================================================
        // Test accounting
        // ====================================================================

        size_t totalRows = 0;
        size_t relationChecks = 0;


        // ====================================================================
        // Part 1 code-point set
        //
        // Part 1 c1 values are individual code points.
        //
        // One byte per Unicode code-point position keeps the complement test
        // deliberately simple.
        // ====================================================================

        std::vector<uint8_t> part1Listed(
            kUnicodeLimit,
            0);

        size_t part1ListedCount = 0;


        // ====================================================================
        // Reused parsing / normalization storage
        // ====================================================================

        std::vector<uint32_t> columns[5];
        std::vector<uint32_t> normalized;


        // ====================================================================
        // Relationship checker
        // ====================================================================

        size_t currentLine = 0;


        auto checkNFC =
            [&](const std::vector<uint32_t>& source,
                const std::vector<uint32_t>& expected,
                const char* relationship) -> bool
            {
                size_t requiredCapacity = 0;


                if (!UnicodeNormalizer::nfcMaximumCapacity(
                    source.size(),
                    requiredCapacity))
                {
                    std::printf(
                        "Unicode NFC conformance: FAIL\n"
                        "  Line:         %zu\n"
                        "  Relationship: %s\n"
                        "  Error:        nfcMaximumCapacity() failed\n",
                        currentLine,
                        relationship);

                    unicodeNFCTestPrintSequence(
                        "Source:",
                        source.data(),
                        source.size());

                    return false;
                }


                normalized.resize(
                    requiredCapacity);


                size_t actualCount = 0;


                if (!normalizer.nfc(
                    source.data(),
                    source.size(),
                    normalized.data(),
                    normalized.size(),
                    actualCount))
                {
                    std::printf(
                        "Unicode NFC conformance: FAIL\n"
                        "  Line:         %zu\n"
                        "  Relationship: %s\n"
                        "  Error:        nfc() failed\n",
                        currentLine,
                        relationship);

                    unicodeNFCTestPrintSequence(
                        "Source:",
                        source.data(),
                        source.size());

                    return false;
                }


                if (actualCount >
                    requiredCapacity)
                {
                    std::printf(
                        "Unicode NFC conformance: FAIL\n"
                        "  Line:         %zu\n"
                        "  Relationship: %s\n"
                        "  Error:        NFC output exceeded capacity\n",
                        currentLine,
                        relationship);

                    return false;
                }


                if (!unicodeNFCTestEquals(
                    normalized.data(),
                    actualCount,
                    expected.data(),
                    expected.size()))
                {
                    std::printf(
                        "Unicode NFC conformance: FAIL\n"
                        "  Line:         %zu\n"
                        "  Relationship: %s\n",
                        currentLine,
                        relationship);

                    unicodeNFCTestPrintSequence(
                        "Source:",
                        source.data(),
                        source.size());

                    unicodeNFCTestPrintSequence(
                        "Expected:",
                        expected.data(),
                        expected.size());

                    unicodeNFCTestPrintSequence(
                        "Actual:",
                        normalized.data(),
                        actualCount);

                    return false;
                }


                ++relationChecks;

                return true;
            };


        // ====================================================================
        // Parse NormalizationTest.txt
        // ====================================================================

        std::string line;


        while (std::getline(
            testInput,
            line))
        {
            ++currentLine;


            // ---------------------------------------------------------------
            // Capture Unicode version before stripping comments.
            // ---------------------------------------------------------------

            if (!testVersionFound)
            {
                unsigned major = 0;
                unsigned minor = 0;
                unsigned patch = 0;


                if (std::sscanf(
                    line.c_str(),
                    "# NormalizationTest-%u.%u.%u.txt",
                    &major,
                    &minor,
                    &patch) == 3)
                {
                    testMajor =
                        static_cast<uint32_t>(major);

                    testMinor =
                        static_cast<uint32_t>(minor);

                    testPatch =
                        static_cast<uint32_t>(patch);

                    testVersionFound = true;
                }
            }


            // ---------------------------------------------------------------
            // Remove comments.
            //
            // This converts:
            //
            //      @Part3 # PRI #29 Test
            //
            // into:
            //
            //      @Part3
            // ---------------------------------------------------------------

            const size_t comment =
                line.find('#');


            if (comment !=
                std::string::npos)
            {
                line.resize(comment);
            }


            const char* begin =
                line.data();

            const char* end =
                begin + line.size();


            unicodeNFCTestTrim(
                begin,
                end);


            if (begin == end)
                continue;


            // ---------------------------------------------------------------
            // @PartN directive
            // ---------------------------------------------------------------

            if (*begin == '@')
            {
                uint32_t partNumber = 0;


                if (!unicodeNFCTestParsePart(
                    begin,
                    end,
                    partNumber))
                {
                    std::printf(
                        "Unicode NFC conformance: FAIL\n"
                        "  Line:  %zu\n"
                        "  Error: invalid test directive\n",
                        currentLine);

                    return false;
                }


                if (partNumber >=
                    kMaximumTestParts)
                {
                    std::printf(
                        "Unicode NFC conformance: FAIL\n"
                        "  Line:  %zu\n"
                        "  Error: test part number too large: %u\n",
                        currentLine,
                        partNumber);

                    return false;
                }


                currentPart =
                    partNumber;

                sawAnyPart =
                    true;


                if (partNumber == 1)
                    sawPart1 = true;


                if (partNumber >
                    highestPart)
                {
                    highestPart =
                        partNumber;
                }


                continue;
            }


            // ---------------------------------------------------------------
            // Data must occur inside a Part.
            // ---------------------------------------------------------------

            if (!sawAnyPart ||
                currentPart >= kMaximumTestParts)
            {
                std::printf(
                    "Unicode NFC conformance: FAIL\n"
                    "  Line:  %zu\n"
                    "  Error: data encountered before first @Part directive\n",
                    currentLine);

                return false;
            }


            // ---------------------------------------------------------------
            // Parse exactly five semicolon-terminated columns.
            //
            // NormalizationTest.txt rows are:
            //
            //      c1;c2;c3;c4;c5;
            // ---------------------------------------------------------------

            const char* fieldBegin =
                begin;


            for (uint32_t field = 0;
                field < 5;
                ++field)
            {
                const char* semicolon =
                    fieldBegin;


                while (semicolon < end &&
                    *semicolon != ';')
                {
                    ++semicolon;
                }


                if (semicolon == end)
                {
                    std::printf(
                        "Unicode NFC conformance: FAIL\n"
                        "  Line:  %zu\n"
                        "  Error: missing field separator\n",
                        currentLine);

                    return false;
                }


                if (!unicodeNFCTestParseSequence(
                    fieldBegin,
                    semicolon,
                    columns[field]))
                {
                    std::printf(
                        "Unicode NFC conformance: FAIL\n"
                        "  Line:  %zu\n"
                        "  Field: %u\n"
                        "  Error: invalid code-point sequence\n",
                        currentLine,
                        field + 1u);

                    return false;
                }


                fieldBegin =
                    semicolon + 1;
            }


            // ---------------------------------------------------------------
            // Only whitespace may follow c5's trailing semicolon.
            // ---------------------------------------------------------------

            const char* remainderBegin =
                fieldBegin;

            const char* remainderEnd =
                end;


            unicodeNFCTestTrim(
                remainderBegin,
                remainderEnd);


            if (remainderBegin !=
                remainderEnd)
            {
                std::printf(
                    "Unicode NFC conformance: FAIL\n"
                    "  Line:  %zu\n"
                    "  Error: unexpected data after fifth field\n",
                    currentLine);

                return false;
            }


            // ---------------------------------------------------------------
            // Row accounting.
            // ---------------------------------------------------------------

            ++totalRows;
            ++partRows[currentPart];


            // ---------------------------------------------------------------
            // Part 1:
            //
            // c1 must be one individual code point.
            // ---------------------------------------------------------------

            if (currentPart == 1)
            {
                if (columns[0].size() != 1)
                {
                    std::printf(
                        "Unicode NFC conformance: FAIL\n"
                        "  Line:  %zu\n"
                        "  Error: Part 1 c1 is not one code point\n",
                        currentLine);

                    return false;
                }


                const uint32_t cp =
                    columns[0][0];


                if (part1Listed[cp] != 0)
                {
                    std::printf(
                        "Unicode NFC conformance: FAIL\n"
                        "  Line:  %zu\n"
                        "  Error: duplicate Part 1 c1 U+%04X\n",
                        currentLine,
                        cp);

                    return false;
                }


                part1Listed[cp] = 1;
                ++part1ListedCount;
            }


            // ---------------------------------------------------------------
            // Official NFC conformance relationships:
            //
            //      c2 == NFC(c1)
            //      c2 == NFC(c2)
            //      c2 == NFC(c3)
            //
            //      c4 == NFC(c4)
            //      c4 == NFC(c5)
            // ---------------------------------------------------------------

            if (!checkNFC(
                columns[0],
                columns[1],
                "NFC(c1) == c2"))
            {
                return false;
            }


            if (!checkNFC(
                columns[1],
                columns[1],
                "NFC(c2) == c2"))
            {
                return false;
            }


            if (!checkNFC(
                columns[2],
                columns[1],
                "NFC(c3) == c2"))
            {
                return false;
            }


            if (!checkNFC(
                columns[3],
                columns[3],
                "NFC(c4) == c4"))
            {
                return false;
            }


            if (!checkNFC(
                columns[4],
                columns[3],
                "NFC(c5) == c4"))
            {
                return false;
            }
        }


        if (!testInput.eof())
            return fail(
                "error while reading NormalizationTest.txt");


        // ====================================================================
        // File integrity
        // ====================================================================

        if (!testVersionFound)
            return fail(
                "NormalizationTest.txt version not found");


        if (!sawAnyPart)
            return fail(
                "NormalizationTest.txt contains no test parts");


        if (!sawPart1)
            return fail(
                "NormalizationTest.txt contains no Part 1");


        if (highestPart >=
            kMaximumTestParts)
        {
            return fail(
                "invalid highest Part number");
        }


        // --------------------------------------------------------------------
        // Require contiguous Part numbering:
        //
        //      Part 0 ... Part N
        //
        // but do not hard-code N.
        // --------------------------------------------------------------------

        for (uint32_t i = 0;
            i <= highestPart;
            ++i)
        {
            if (partRows[i] == 0)
            {
                std::printf(
                    "Unicode NFC conformance: FAIL\n"
                    "  Error: Part %u is missing or empty\n",
                    i);

                return false;
            }
        }


        // ====================================================================
        // Unicode version agreement
        // ====================================================================

        if (testMajor != database.unicodeMajor() ||
            testMinor != database.unicodeMinor() ||
            testPatch != database.unicodePatch())
        {
            std::printf(
                "Unicode NFC conformance: FAIL\n"
                "  Database Unicode version: %u.%u.%u\n"
                "  Test file version:        %u.%u.%u\n",
                database.unicodeMajor(),
                database.unicodeMinor(),
                database.unicodePatch(),
                testMajor,
                testMinor,
                testPatch);

            return false;
        }


        // ====================================================================
        // Part 1 complement
        //
        // The formal Unicode requirement says that assigned code points not
        // specifically listed in Part 1 must remain unchanged by normalization.
        //
        // As with the NFD conformance test, this low-level runtime test goes
        // further and verifies every other position in the Unicode address
        // space. Unassigned and surrogate positions are expected to remain
        // unchanged by this scalar-level NFC implementation as well.
        // ====================================================================

        size_t complementChecks = 0;


        for (uint32_t cp = 0;
            cp < kUnicodeLimit;
            ++cp)
        {
            if (part1Listed[cp] != 0)
                continue;


            UnicodeCanonicalDecomposition result;


            if (!normalizer.nfc(
                cp,
                result))
            {
                std::printf(
                    "Unicode NFC conformance: FAIL\n"
                    "  Complement check: U+%04X\n"
                    "  Error: nfc() failed\n",
                    cp);

                return false;
            }


            if (result.count != 1 ||
                result.codePoints[0] != cp)
            {
                std::printf(
                    "Unicode NFC conformance: FAIL\n"
                    "  Complement check: U+%04X\n",
                    cp);

                unicodeNFCTestPrintSequence(
                    "Expected:",
                    &cp,
                    1);

                unicodeNFCTestPrintSequence(
                    "Actual:",
                    result.codePoints,
                    result.count);

                return false;
            }


            ++complementChecks;
        }


        // ====================================================================
        // Internal accounting
        // ====================================================================

        size_t accountedRows = 0;


        for (uint32_t i = 0;
            i <= highestPart;
            ++i)
        {
            accountedRows +=
                partRows[i];
        }


        if (accountedRows != totalRows)
            return fail(
                "test row accounting is inconsistent");


        if (relationChecks !=
            totalRows * 5)
        {
            return fail(
                "NFC relationship accounting is inconsistent");
        }


        if (part1ListedCount +
            complementChecks !=
            static_cast<size_t>(kUnicodeLimit))
        {
            return fail(
                "Part 1 complement accounting is inconsistent");
        }


        // ====================================================================
        // PASS
        // ====================================================================

        std::printf(
            "Unicode NFC conformance: PASS\n"
            "  Unicode version:             %u.%u.%u\n"
            "  Test rows:                   %zu\n",
            database.unicodeMajor(),
            database.unicodeMinor(),
            database.unicodePatch(),
            totalRows);


        for (uint32_t i = 0;
            i <= highestPart;
            ++i)
        {
            std::printf(
                "    Part %u rows:              %zu\n",
                i,
                partRows[i]);
        }


        std::printf(
            "  NFC relationships checked:   %zu\n"
            "  Part 1 listed code points:   %zu\n"
            "  Complement code points:      %zu\n"
            "  Full Part 1 complement:      PASS\n",
            relationChecks,
            part1ListedCount,
            complementChecks);


        return true;
    }

} // namespace waavs