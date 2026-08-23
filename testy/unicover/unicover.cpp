#include "test_unicode_blocks.h"
#include "test_database_write.h"
#include "test_read_value_aliases.h"
#include "test_unicode_scripts.h"

static void testRoundtrip()
{
    if (testUnicodeDatabaseRoundTrip("./unicode/UCD/Blocks.txt", "./blocks.ucdb"))
        printf("Unicode database round trip: PASS\n");
    else
        printf("Unicode database round trip: FAIL\n");
}




int main()
{
    //testUnicodeBlocks("./unicode/UCD/Blocks.txt");
    //testRoundtrip();
    //testScriptValueAliases("./unicode/UCD/PropertyValueAliases.txt");
    testUnicodeScripts();

    return 0;
}