#include "test/test_unicode_blocks.h"
#include "test/test_database_write.h"
#include "test/test_read_value_aliases.h"
#include "test/test_unicode_scripts.h"
#include "test/test_unicode_value_table8.h"
#include "test/test_ucd_general_category.h"
#include "test/test_ucd_combining_class.h"
#include "test/test_unicode_database_value8_roundtrip.h"
#include "test/test_unicode_decomposition.h"
#include "test/test_unicode_decomposition_roundtrip.h"
#include "test/test_unicode_canonical_decomposer.h"
#include "test/test_unicode_canonical_ordering.h"
#include "test/test_unicode_normalizer.h"
#include "test/test_unicode_normalization_conformance.h"
#include "test/test_ucd_normalization_props_parser.h"
#include "test/test_unicode_composition_builder.h"
#include "test/test_unicode_composition.h"
#include "test/test_unicode_canonical_composer.h"
#include "test/test_unicode_canonical_sequence_composer_.h"
#include "test/test_unicode_nfc_conformance.h"
#include "test/test_ucd_bidi_class_parser.h"
#include "test/test_ucd_grapheme_cluster_break_parser.h"
#include "test/test_unicode_extended_pictographic_database.h"
#include "test/test_ucd_indic_conjunct_break_parser.h"


#include "unicode_database_generator.h"

using namespace waavs;


static void testRoundtrip()
{
    if (testUnicodeDatabaseRoundTrip("./unicode/UCD/Blocks.txt", "./blocks.ucdb"))
        printf("Unicode database round trip: PASS\n");
    else
        printf("Unicode database round trip: FAIL\n");
}


static void runTests()
{
    //testUnicodeBlocks("./UCD/Blocks.txt");
    //testRoundtrip();
    //testScriptValueAliases("./UCD/PropertyValueAliases.txt");
    //testUnicodeScripts();
    //testUnicodeValueTable8();
    //testUCDGeneralCategory("./UCD/extracted/DerivedGeneralCategory.txt");
    //testUCDCombiningClass("./UCD/extracted/DerivedCombiningClass.txt");
    //testUnicodeDatabaseValue8RoundTrip( "./UCD/extracted/DerivedGeneralCategory.txt", "./UCD/extracted/DerivedCombiningClass.txt");
    //testUnicodeDecomposition("./UCD/UnicodeData.txt");
    //testUnicodeDecompositionRoundTrip("./UCD/UnicodeData.txt");
    //testUnicodeCanonicalDecomposer("./unicode-17.0.0.ucdb");
    //testUnicodeCanonicalOrdering("./unicode-17.0.0.ucdb");
    //testUnicodeNormalizer("./unicode-17.0.0.ucdb");
    //testUnicodeNormalizationConformance( "./unicode-17.0.0.ucdb", "./UCD/NormalizationTest.txt");
    //testUnicodeNFCConformance("./unicode-17.0.0.ucdb", "./UCD/NormalizationTest.txt");
    //testUCDNormalizationPropsParser("./UCD/DerivedNormalizationProps.txt");
    //testUnicodeCompositionBuilder( "./UCD/UnicodeData.txt", "./UCD/DerivedNormalizationProps.txt");
    //testUnicodeComposition("./unicode-17.0.0.ucdb");
    //testUnicodeCanonicalComposer("./unicode-17.0.0.ucdb");
    //testUnicodeCanonicalSequenceComposer("./unicode-17.0.0.ucdb");
    //testUnicodeBidiClassParserFile("./UCD/extracted/DerivedBidiClass.txt");
    //testUnicodeGraphemeClusterBreakParser("./UCD/auxiliary/GraphemeBreakProperty.txt");
    //testUnicodeExtendedPictographicDatabase("./UCD/emoji/emoji-data.txt", "./unicode-17.0.0.ucdb");
    testUCDIndicConjunctBreakParser("./ucd/DerivedCoreProperties.txt");
}


int createDatabase(int argc, char **argv)
{
    const char* ucdRoot =
        argc > 1
        ? argv[1]
        : "./ucd";


    const char* outputFilename =
        argc > 2
        ? argv[2]
        : "./unicode-17.0.0.ucdb";


    std::printf(
        "Building Unicode database\n"
        "  UCD root: %s\n"
        "  Output:   %s\n\n",
        ucdRoot,
        outputFilename);


    return
        waavs::buildUnicodeDatabase17(
            ucdRoot,
            outputFilename)
        ? 0
        : 1;
}

int main(int argc, char **argv)
{
    //runTests();

    // Database generator
    createDatabase(argc, argv);

    return 0;
}