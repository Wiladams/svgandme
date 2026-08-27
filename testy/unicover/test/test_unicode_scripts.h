#include "test_core.h"
#include "ucd_scripts_parser.h"

// ============================================================================
// findScriptIndex
// ============================================================================

static bool findScriptIndex(
    const waavs::UnicodeDatabase& database,
    waavs::InternedKey scriptName,
    uint32_t& outIndex)
{
    using namespace waavs;

    if (!scriptName)
        return false;

    for (uint32_t i = 0; i < database.scriptCount(); ++i)
    {
        InternedKey name = database.scriptName(i);

        if (name == scriptName)
        {
            outIndex = i;
            return true;
        }
    }

    return false;
}


// ============================================================================
// verifyScriptCodePoint
// ============================================================================

static bool verifyScriptCodePoint(
    const waavs::UnicodeDatabase& database,
    const char* scriptNameText,
    uint32_t codePoint,
    bool expected)
{
    using namespace waavs;

    InternedKey scriptName =
        WSNameSet::INTERN(scriptNameText);

    if (!scriptName)
    {
        printf(
            "FAILED: could not intern Script name '%s'\n",
            scriptNameText);

        return false;
    }

    uint32_t scriptIndex = 0;

    if (!findScriptIndex(
        database,
        scriptName,
        scriptIndex))
    {
        printf(
            "FAILED: Script '%s' not found in runtime database\n",
            scriptNameText);

        return false;
    }

    UnicodeCoverage coverage =
        database.scriptCoverage(scriptIndex);

    if (!coverage.valid())
    {
        printf(
            "FAILED: Script '%s' has invalid coverage\n",
            scriptNameText);

        return false;
    }

    const bool actual =
        coverage.contains(codePoint);

    if (actual != expected)
    {
        printf(
            "FAILED: %-22s U+%04X expected %s, got %s\n",
            scriptNameText,
            codePoint,
            expected ? "YES" : "NO",
            actual ? "YES" : "NO");

        return false;
    }

    printf(
        "  %-22s U+%04X  %-3s  PASS\n",
        scriptNameText,
        codePoint,
        expected ? "YES" : "NO");

    return true;
}


// ============================================================================
// verifyScriptIdentity
//
// Verify both the long Script name and ISO 15924 short name.
// ============================================================================

static bool verifyScriptIdentity(
    const waavs::UnicodeDatabase& database,
    const char* scriptNameText,
    const char* iso15924Text)
{
    using namespace waavs;

    InternedKey expectedName =
        WSNameSet::INTERN(scriptNameText);

    InternedKey expectedISO =
        WSNameSet::INTERN(iso15924Text);

    if (!expectedName || !expectedISO)
    {
        printf(
            "FAILED: could not intern Script identity %s / %s\n",
            scriptNameText,
            iso15924Text);

        return false;
    }

    uint32_t scriptIndex = 0;

    if (!findScriptIndex(
        database,
        expectedName,
        scriptIndex))
    {
        printf(
            "FAILED: Script '%s' not found\n",
            scriptNameText);

        return false;
    }

    InternedKey actualName =
        database.scriptName(scriptIndex);

    InternedKey actualISO =
        database.scriptISO15924(scriptIndex);

    if (actualName != expectedName)
    {
        printf(
            "FAILED: Script name mismatch for '%s'\n",
            scriptNameText);

        return false;
    }

    if (actualISO != expectedISO)
    {
        printf(
            "FAILED: ISO 15924 mismatch for '%s': expected %s, got %s\n",
            scriptNameText,
            iso15924Text,
            actualISO ? actualISO : "<null>");

        return false;
    }

    printf(
        "  %-22s <-> %-4s  PASS\n",
        scriptNameText,
        iso15924Text);

    return true;
}


// ============================================================================
// verifyEmptyScriptCoverage
// ============================================================================

static bool verifyEmptyScriptCoverage(
    const waavs::UnicodeDatabase& database,
    const char* scriptNameText)
{
    using namespace waavs;

    InternedKey scriptName =
        WSNameSet::INTERN(scriptNameText);

    if (!scriptName)
        return false;

    uint32_t scriptIndex = 0;

    if (!findScriptIndex(
        database,
        scriptName,
        scriptIndex))
    {
        printf(
            "FAILED: Script '%s' not found\n",
            scriptNameText);

        return false;
    }

    UnicodeCoverage coverage =
        database.scriptCoverage(scriptIndex);

    if (!coverage.valid())
    {
        printf(
            "FAILED: Script '%s' has invalid coverage\n",
            scriptNameText);

        return false;
    }

    if (!coverage.empty())
    {
        const UnicodeCoverageStats stats =
            coverage.stats();

        printf(
            "FAILED: %-22s expected empty coverage, contains %zu code points\n",
            scriptNameText,
            stats.coveredCodePoints);

        return false;
    }

    printf(
        "  %-22s empty         PASS\n",
        scriptNameText);

    return true;
}

// ============================================================================
// verifyUnicodeScriptsRuntime
// ============================================================================

static bool verifyUnicodeScriptsRuntime(
    const waavs::UnicodeDatabaseBuilder& builder,
    const waavs::UCDScriptsParseResult& parseResult)
{
    using namespace waavs;


    // ========================================================================
    // Serialize database into memory
    // ========================================================================

    std::vector<uint8_t> databaseImage;

    UnicodeDatabaseWriteResult writeResult{};

    if (!UnicodeDatabaseWriter::write(
        builder,
        17,
        0,
        0,
        databaseImage,
        writeResult))
    {
        printf(
            "FAILED: database serialization: %s\n",
            unicodeDatabaseWriteErrorString(
                writeResult.error));

        return false;
    }


    // ========================================================================
    // Attach runtime database
    // ========================================================================

    ByteSpan databaseBytes(
        databaseImage.data(),
        databaseImage.size());

    UnicodeDatabase database;

    if (!database.reset(databaseBytes))
    {
        printf(
            "FAILED: UnicodeDatabase rejected generated database\n");

        return false;
    }


    printf("\nRuntime database verification\n");
    printf("------------------------------------------------------------\n");

    printf(
        "Database bytes:           %zu\n",
        databaseImage.size());

    printf(
        "Script records:           %u\n",
        database.scriptCount());

    printf(
        "Coverage records:         %u\n",
        database.coverageCount());


    // ========================================================================
    // Basic structural checks
    // ========================================================================

    if (database.scriptCount() !=
        parseResult.scriptCount)
    {
        printf(
            "FAILED: runtime Script count %u, expected %u\n",
            database.scriptCount(),
            parseResult.scriptCount);

        return false;
    }

    printf("Runtime script count:     PASS\n");


    // ========================================================================
    // Persistent Script identity
    //
    // This verifies:
    //
    //      UnicodeScriptRecord
    //      string-pool offsets
    //      runtime string lookup
    //      WSNameSet canonicalization
    //
    // ========================================================================

    printf("\nScript identity\n");
    printf("------------------------------------------------------------\n");

    if (!verifyScriptIdentity(
        database,
        "Latin",
        "Latn"))
    {
        return false;
    }

    if (!verifyScriptIdentity(
        database,
        "Arabic",
        "Arab"))
    {
        return false;
    }

    if (!verifyScriptIdentity(
        database,
        "Telugu",
        "Telu"))
    {
        return false;
    }

    if (!verifyScriptIdentity(
        database,
        "Common",
        "Zyyy"))
    {
        return false;
    }

    if (!verifyScriptIdentity(
        database,
        "Inherited",
        "Zinh"))
    {
        return false;
    }

    if (!verifyScriptIdentity(
        database,
        "Unknown",
        "Zzzz"))
    {
        return false;
    }

    if (!verifyScriptIdentity(
        database,
        "Katakana_Or_Hiragana",
        "Hrkt"))
    {
        return false;
    }


    // ========================================================================
    // Semantic coverage verification
    // ========================================================================

    printf("\nScript coverage semantics\n");
    printf("------------------------------------------------------------\n");


    // ------------------------------------------------------------------------
    // Latin
    // ------------------------------------------------------------------------

    if (!verifyScriptCodePoint(
        database,
        "Latin",
        0x0041,
        true))
    {
        return false;
    }

    if (!verifyScriptCodePoint(
        database,
        "Latin",
        0x0061,
        true))
    {
        return false;
    }

    if (!verifyScriptCodePoint(
        database,
        "Latin",
        0x0627,
        false))
    {
        return false;
    }


    // ------------------------------------------------------------------------
    // Arabic
    // ------------------------------------------------------------------------

    if (!verifyScriptCodePoint(
        database,
        "Arabic",
        0x0627,
        true))
    {
        return false;
    }

    if (!verifyScriptCodePoint(
        database,
        "Arabic",
        0x0041,
        false))
    {
        return false;
    }


    // ------------------------------------------------------------------------
    // Telugu
    //
    // U+0C15 = TELUGU LETTER KA
    // ------------------------------------------------------------------------

    if (!verifyScriptCodePoint(
        database,
        "Telugu",
        0x0C15,
        true))
    {
        return false;
    }


    // ------------------------------------------------------------------------
    // Common
    //
    // U+0020 = SPACE
    // ------------------------------------------------------------------------

    if (!verifyScriptCodePoint(
        database,
        "Common",
        0x0020,
        true))
    {
        return false;
    }


    // ------------------------------------------------------------------------
    // Inherited
    //
    // U+0301 = COMBINING ACUTE ACCENT
    // ------------------------------------------------------------------------

    if (!verifyScriptCodePoint(
        database,
        "Inherited",
        0x0301,
        true))
    {
        return false;
    }


    // ------------------------------------------------------------------------
    // Unknown
    //
    // U+0378 is unassigned.
    //
    // This is particularly important because Unknown was not explicitly
    // accumulated from Scripts.txt.  It was generated as the complement of
    // all explicit Script assignments.
    // ------------------------------------------------------------------------

    if (!verifyScriptCodePoint(
        database,
        "Unknown",
        0x0378,
        true))
    {
        return false;
    }

    if (!verifyScriptCodePoint(
        database,
        "Unknown",
        0x0041,
        false))
    {
        return false;
    }


    // ------------------------------------------------------------------------
    // Retained empty Script value.
    // ------------------------------------------------------------------------

    if (!verifyEmptyScriptCoverage(
        database,
        "Katakana_Or_Hiragana"))
    {
        return false;
    }


    // ========================================================================
    // Verify complete Unknown coverage count.
    // ========================================================================

    InternedKey unknownName =
        WSNameSet::INTERN("Unknown");

    uint32_t unknownIndex = 0;

    if (!findScriptIndex(
        database,
        unknownName,
        unknownIndex))
    {
        printf(
            "FAILED: unable to locate Unknown Script\n");

        return false;
    }

    UnicodeCoverage unknownCoverage =
        database.scriptCoverage(unknownIndex);

    UnicodeCoverageStats unknownStats =
        unknownCoverage.stats();

    if (unknownStats.coveredCodePoints !=
        parseResult.unknownCodePoints)
    {
        printf(
            "FAILED: Unknown coverage contains %zu code points, expected %zu\n",
            unknownStats.coveredCodePoints,
            parseResult.unknownCodePoints);

        return false;
    }

    printf("\nUnknown coverage\n");
    printf("------------------------------------------------------------\n");

    printf(
        "Covered code points:      %zu\n",
        unknownStats.coveredCodePoints);

    printf(
        "Expected code points:     %zu\n",
        parseResult.unknownCodePoints);

    printf(
        "Unknown coverage count:   PASS\n");


    printf("\nRuntime Script verification: PASS\n");

    return true;
}

static bool testUnicodeScripts()
{
    using namespace waavs;

    const char* aliasFileName =
        "./unicode/UCD/PropertyValueAliases.txt";

    const char* scriptsFileName =
        "./unicode/UCD/Scripts.txt";


    // ========================================================================
    // Read input files
    // ========================================================================

    std::vector<uint8_t> aliasBytes;
    std::vector<uint8_t> scriptBytes;

    if (!readFileData(aliasFileName, aliasBytes))
    {
        printf("FAILED: unable to read %s\n", aliasFileName);
        return false;
    }

    if (!readFileData(scriptsFileName, scriptBytes))
    {
        printf("FAILED: unable to read %s\n", scriptsFileName);
        return false;
    }

    ByteSpan aliasSource(
        aliasBytes.data(),
        aliasBytes.size());

    ByteSpan scriptSource(
        scriptBytes.data(),
        scriptBytes.size());


    printf("============================================================\n");
    printf("Unicode Scripts.txt parse\n");
    printf("============================================================\n");

    printf("Aliases:  %s\n", aliasFileName);
    printf("Bytes:    %zu\n", aliasBytes.size());

    printf("\nScripts:  %s\n", scriptsFileName);
    printf("Bytes:    %zu\n", scriptBytes.size());


    // ========================================================================
    // Parse Script value aliases
    // ========================================================================

    UCDScriptValueAliases aliases;
    UCDPropertyValueAliasesParseResult aliasResult{};

    if (!ucdParseScriptValueAliases(
        aliasSource,
        aliases,
        aliasResult))
    {
        printf("\nFAILED: PropertyValueAliases parse\n");
        printf("  line:  %u\n", aliasResult.lineNumber);
        printf("  error: %u\n",
            static_cast<unsigned>(aliasResult.error));

        return false;
    }


    printf("\nScript aliases\n");
    printf("------------------------------------------------------------\n");
    printf("Aliases parsed:           %zu\n", aliases.size());


    // Unicode 17:
    //
    // 172 encoded/supported scripts
    // Common
    // Inherited
    // Unknown
    // retained Katakana_Or_Hiragana
    //
    // Total = 176

    if (aliases.size() != 176)
    {
        printf("FAILED: expected 176 Script aliases\n");
        return false;
    }

    printf("Unicode 17 alias count:   PASS\n");


    // ========================================================================
    // Confirm a few alias identities before using them
    // ========================================================================

    struct AliasCheck
    {
        const char* longName;
        const char* shortName;
    };

    static const AliasCheck aliasChecks[] =
    {
        { "Latin",                 "Latn" },
        { "Arabic",                "Arab" },
        { "Telugu",                "Telu" },
        { "Common",                "Zyyy" },
        { "Inherited",             "Zinh" },
        { "Unknown",               "Zzzz" },
        { "Old_Italic",            "Ital" },
        { "Katakana_Or_Hiragana",  "Hrkt" }
    };


    printf("\nAlias checks\n");
    printf("------------------------------------------------------------\n");

    for (const AliasCheck& check : aliasChecks)
    {
        InternedKey longName =
            WSNameSet::INTERN(check.longName);

        InternedKey shortName =
            WSNameSet::INTERN(check.shortName);

        const UCDScriptValueAlias* byLong =
            aliases.findByLongName(longName);

        const UCDScriptValueAlias* byShort =
            aliases.findByShortName(shortName);

        if (!byLong)
        {
            printf(
                "FAILED: no alias record for '%s'\n",
                check.longName);

            return false;
        }

        if (!byShort)
        {
            printf(
                "FAILED: no alias record for '%s'\n",
                check.shortName);

            return false;
        }

        if (byLong != byShort)
        {
            printf(
                "FAILED: alias directions disagree for %s / %s\n",
                check.longName,
                check.shortName);

            return false;
        }

        if (byLong->longName != longName ||
            byLong->shortName != shortName)
        {
            printf(
                "FAILED: interned alias identity mismatch for %s / %s\n",
                check.longName,
                check.shortName);

            return false;
        }

        printf(
            "  %-22s <-> %-4s  PASS\n",
            check.longName,
            check.shortName);
    }


    // ========================================================================
    // Parse Scripts.txt
    // ========================================================================

    UnicodeDatabaseBuilder database;

    UCDScriptsParseResult scriptResult{};

    if (!ucdParseScripts(
        scriptSource,
        aliases,
        database,
        scriptResult))
    {
        printf("\nFAILED: Scripts.txt parse\n");
        printf("  line:  %u\n", scriptResult.lineNumber);
        printf("  error: %s\n",
            ucdScriptsParseErrorString(
                scriptResult.error));

        return false;
    }


    // ========================================================================
    // Basic parser results
    // ========================================================================

    printf("\nScripts parse\n");
    printf("------------------------------------------------------------\n");

    printf(
        "Explicit ranges:          %u\n",
        scriptResult.rangeCount);

    printf(
        "Scripts finalized:        %u\n",
        scriptResult.scriptCount);

    printf(
        "Explicit code points:     %zu\n",
        scriptResult.explicitCodePoints);

    printf(
        "Unknown code points:      %zu\n",
        scriptResult.unknownCodePoints);

    printf(
        "Total code points:        %zu\n",
        scriptResult.explicitCodePoints +
        scriptResult.unknownCodePoints);


    // ========================================================================
    // Structural verification
    // ========================================================================

    printf("\nStructural verification\n");
    printf("------------------------------------------------------------\n");


    // ------------------------------------------------------------------------
    // We seeded the parser from PropertyValueAliases.txt, so every Script
    // property value should have a record -- even retained empty values.
    // ------------------------------------------------------------------------

    if (scriptResult.scriptCount != aliases.size())
    {
        printf(
            "FAILED: script count %u does not match alias count %zu\n",
            scriptResult.scriptCount,
            aliases.size());

        return false;
    }

    printf("Script count == aliases:  PASS\n");


    if (database.scriptCount() != aliases.size())
    {
        printf(
            "FAILED: database contains %zu scripts, expected %zu\n",
            database.scriptCount(),
            aliases.size());

        return false;
    }

    printf("Database script count:    PASS\n");


    // ------------------------------------------------------------------------
    // There should certainly be explicit ranges.
    // ------------------------------------------------------------------------

    if (scriptResult.rangeCount == 0)
    {
        printf("FAILED: no explicit Script ranges parsed\n");
        return false;
    }

    printf("Explicit ranges present:  PASS\n");


    // ------------------------------------------------------------------------
    // Unknown is the complement of all explicit Script assignments.
    // Together they must describe the entire Unicode code-point space.
    // ------------------------------------------------------------------------

    const size_t totalCodePoints =
        scriptResult.explicitCodePoints +
        scriptResult.unknownCodePoints;

    if (totalCodePoints != kUnicodeLimit)
    {
        printf(
            "FAILED: coverage total %zu, expected %u\n",
            totalCodePoints,
            static_cast<unsigned>(kUnicodeLimit));

        return false;
    }

    printf("Full Unicode coverage:    PASS\n");


    // ------------------------------------------------------------------------
    // Unknown must actually contain something.
    //
    // This is also a useful sanity check that we didn't accidentally mark the
    // whole Unicode space assigned while building the complement.
    // ------------------------------------------------------------------------

    if (scriptResult.unknownCodePoints == 0)
    {
        printf("FAILED: Script=Unknown coverage is empty\n");
        return false;
    }

    printf("Unknown coverage present: PASS\n");


    // ========================================================================
    // Builder results
    // ========================================================================

    printf("\nDatabase builder\n");
    printf("------------------------------------------------------------\n");

    printf(
        "Script records:           %zu\n",
        database.scriptCount());

    printf(
        "Coverage records:         %zu\n",
        database.coverageCount());


    // ========================================================================
    // Page-pool statistics
    // ========================================================================
    //
    // These should be particularly interesting compared with Blocks.txt,
    // because each Script coverage is generally made from many disconnected
    // ranges.
    // ========================================================================

    const auto& pageStats =
        database.pagePool().stats();


    printf("\nBit page pool\n");
    printf("------------------------------------------------------------\n");

    printf(
        "Requests:                 %zu\n",
        pageStats.bitPageRequests);

    printf(
        "Empty sentinels:          %zu\n",
        pageStats.bitPageEmpty);

    printf(
        "Full sentinels:           %zu\n",
        pageStats.bitPageFull);

    printf(
        "Reused pages:             %zu\n",
        pageStats.bitPageReused);

    printf(
        "Unique stored pages:      %zu\n",
        pageStats.uniqueBitPages);

    printf(
        "Hash collisions:          %zu\n",
        pageStats.bitPageHashCollisions);


    printf("\nMaster page pool\n");
    printf("------------------------------------------------------------\n");

    printf(
        "Requests:                 %zu\n",
        pageStats.masterPageRequests);

    printf(
        "Empty sentinels:          %zu\n",
        pageStats.masterPageEmpty);

    printf(
        "Full sentinels:           %zu\n",
        pageStats.masterPageFull);

    printf(
        "Reused pages:             %zu\n",
        pageStats.masterPageReused);

    printf(
        "Unique stored pages:      %zu\n",
        pageStats.uniqueMasterPages);

    printf(
        "Hash collisions:          %zu\n",
        pageStats.masterPageHashCollisions);


    if (!verifyUnicodeScriptsRuntime( database, scriptResult))
    {
        return false;
    }


    // ========================================================================
    // Final result
    // ========================================================================

    printf("\nScripts.txt test: PASS\n");

    return true;
}


