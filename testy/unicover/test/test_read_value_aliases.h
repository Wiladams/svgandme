
#include "test_core.h"

#include "ucd_property_value_aliases_parser.h"





static bool checkScriptAlias(const waavs::UCDScriptValueAliases& aliases,
    const char* longNameText, const char* shortNameText)
{
    using namespace waavs;

    InternedKey longName = WSNameSet::INTERN(longNameText);
    InternedKey shortName = WSNameSet::INTERN(shortNameText);

    if (!longName || !shortName)
    {
        printf("FAILED: unable to intern alias names\n");
        return false;
    }

    InternedKey foundShort = aliases.shortNameForLongName(longName);

    if (!foundShort)
    {
        printf("FAILED: no short alias for '%s'\n", longNameText);
        return false;
    }

    if (foundShort != shortName)
    {
        printf("FAILED: incorrect short alias for '%s'\n", longNameText);
        printf("  expected: %s\n", shortNameText);
        printf("  actual:   %s\n", foundShort);
        return false;
    }

    InternedKey foundLong = aliases.longNameForShortName(shortName);

    if (!foundLong)
    {
        printf("FAILED: no long alias for '%s'\n", shortNameText);
        return false;
    }

    if (foundLong != longName)
    {
        printf("FAILED: incorrect long alias for '%s'\n", shortNameText);
        printf("  expected: %s\n", longNameText);
        printf("  actual:   %s\n", foundLong);
        return false;
    }

    printf("  %-12s <-> %-4s  PASS\n", longNameText, shortNameText);

    return true;
}


static bool testScriptValueAliases(const char* filename)
{
    using namespace waavs;

    printf("\n");
    printf("============================================================\n");
    printf("Unicode Script Value Aliases\n");
    printf("============================================================\n");

    // ========================================================================
    // Read PropertyValueAliases.txt
    // ========================================================================

    std::vector<uint8_t> fileData;

    if (!readFileData(filename, fileData))
    {
        printf("FAILED: unable to read file\n");
        printf("  file: %s\n", filename);
        return false;
    }

    printf("File:  %s\n", filename);
    printf("Bytes: %zu\n", fileData.size());


    // ========================================================================
    // Parse Script aliases
    // ========================================================================

    ByteSpan source(fileData.data(), fileData.size());

    UCDScriptValueAliases aliases;
    UCDPropertyValueAliasesParseResult result;

    if (!ucdParseScriptValueAliases(source, aliases, result))
    {
        printf("\nFAILED: alias parser\n");
        printf("  line:  %u\n", result.lineNumber);
        printf("  error: %s\n",
            ucdPropertyValueAliasesParseErrorString(result.error));

        return false;
    }

    printf("\nScript aliases parsed: %zu\n", aliases.size());

    if (aliases.size() != result.scriptAliasCount)
    {
        printf("FAILED: parser count does not match alias table\n");
        printf("  parser: %u\n", result.scriptAliasCount);
        printf("  table:  %zu\n", aliases.size());
        return false;
    }


    // ========================================================================
    // Unicode 17 sanity check
    //
    // 172 encoded scripts + Common + Inherited + Unknown = 175.
    // ========================================================================

    if (aliases.size() != 176)
    {
        printf("FAILED: unexpected Unicode 17 Script alias count\n");
        printf("  expected: 176\n");
        printf("  actual:   %zu\n", aliases.size());
        return false;
    }

    printf("Unicode 17 alias count: PASS\n");


    // ========================================================================
    // Spot checks
    // ========================================================================

    printf("\nSpot checks\n");
    printf("------------------------------------------------------------\n");

    if (!checkScriptAlias(aliases, "Latin", "Latn"))
        return false;

    if (!checkScriptAlias(aliases, "Arabic", "Arab"))
        return false;

    if (!checkScriptAlias(aliases, "Telugu", "Telu"))
        return false;

    if (!checkScriptAlias(aliases, "Common", "Zyyy"))
        return false;

    if (!checkScriptAlias(aliases, "Inherited", "Zinh"))
        return false;

    if (!checkScriptAlias(aliases, "Unknown", "Zzzz"))
        return false;


    // ========================================================================
    // A less common script just to make sure we're not only exercising the
    // handful of ubiquitous values.
    // ========================================================================

    if (!checkScriptAlias(aliases, "Old_Italic", "Ital"))
        return false;


    // ========================================================================
    // Complete
    // ========================================================================

    printf("\n");
    printf("Script value aliases: PASS\n");

    return true;
}
