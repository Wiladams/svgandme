// unicode_database_generator.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <utility>

#include "ucd_blocks_parser.h"
#include "ucd_bidi_class_parser.h"
#include "ucd_combining_class_parser.h"
#include "ucd_extended_pictographic_parser.h"
#include "ucd_general_category_parser.h"
#include "ucd_grapheme_cluster_break_parser.h"
#include "ucd_indic_conjunct_break_parser.h"
#include "ucd_property_value_aliases_parser.h"
#include "ucd_normalization_props_parser.h"
#include "ucd_scripts_parser.h"
#include "ucd_unicode_data_parser.h"

#include "unicode_database.h"
#include "unicode_database_builder.h"
#include "unicode_database_writer.h"
#include "unicode_decomposition_builder.h"
#include "unicode_composition_builder.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    // ========================================================================
    // UCDSourceFile
    //
    // Small generator-side file holder.
    //
    // The vector owns the bytes. span() is valid as long as this object
    // remains alive and its vector is not modified.
    // ========================================================================

    struct UCDSourceFile
    {
        std::vector<uint8_t> bytes;


        [[nodiscard]]
        ByteSpan span() const noexcept
        {
            return bytes.empty()
                ? ByteSpan{}
            : ByteSpan(bytes.data(), bytes.size());
        }
    };


    // ========================================================================
    // ucdJoinPath
    // ========================================================================

    static inline std::string ucdJoinPath(
        const char* root,
        const char* relative)
    {
        std::string path =
            root ? root : "";


        if (!path.empty() &&
            path.back() != '/' &&
            path.back() != '\\')
        {
            path.push_back('/');
        }


        path +=
            relative ? relative : "";


        return path;
    }


    // ========================================================================
    // ucdLoadFile
    // ========================================================================

    static inline bool ucdLoadFile(
        const std::string& filename,
        UCDSourceFile& outFile)
    {
        outFile.bytes.clear();


        std::ifstream input(
            filename,
            std::ios::binary |
            std::ios::ate);


        if (!input)
        {
            std::printf(
                "Unable to open UCD file: %s\n",
                filename.c_str());

            return false;
        }


        const std::streampos end =
            input.tellg();


        if (end <= 0)
        {
            std::printf(
                "UCD file is empty or unreadable: %s\n",
                filename.c_str());

            return false;
        }


        outFile.bytes.resize(
            static_cast<size_t>(end));


        input.seekg(
            0,
            std::ios::beg);


        input.read(
            reinterpret_cast<char*>(
                outFile.bytes.data()),
            static_cast<std::streamsize>(
                outFile.bytes.size()));


        if (!input)
        {
            std::printf(
                "Unable to read UCD file: %s\n",
                filename.c_str());

            outFile.bytes.clear();
            return false;
        }


        return true;
    }


    // ========================================================================
    // verifyWrittenUnicodeDatabase
    //
    // Read the completed .ucdb image back from disk and attach the real
    // runtime UnicodeDatabase reader.
    //
    // This catches:
    //
    //      - file I/O problems
    //      - writer layout problems
    //      - structural-validation failures
    //      - missing semantic sections
    //
    // ========================================================================

    static inline bool verifyWrittenUnicodeDatabase(const char* filename)
    {
        UCDSourceFile file;


        if (!ucdLoadFile(
            filename,
            file))
        {
            return false;
        }


        UnicodeDatabase database;


        if (!database.reset(
            file.span()))
        {
            std::printf(
                "Written database failed runtime validation: %s\n",
                filename);

            return false;
        }


        if (database.unicodeMajor() != 17 ||
            database.unicodeMinor() != 0 ||
            database.unicodePatch() != 0)
        {
            std::printf("Written database has unexpected Unicode version\n");

            return false;
        }


        if (!database.hasGeneralCategory())
        {
            std::printf("Written database has no General_Category table\n");

            return false;
        }


        if (!database.hasCombiningClass())
        {
            std::printf("Written database has no Canonical_Combining_Class table\n");

            return false;
        }

        if (!database.hasBidiClass())
        {
            std::printf("Written database has no Bidi_Class table\n");

            return false;
        }

        if (!database.hasGraphemeClusterBreak())
        {
            std::printf( "Written database has no Grapheme_Cluster_Break table\n");

            return false;
        }

        if (!database.hasExtendedPictographic())
        {
            std::printf( "Written database has no Extended_Pictographic property\n");

            return false;
        }

        if (!database.hasDecomposition())
        {
            std::printf(
                "Written database has no canonical decomposition table\n");

            return false;
        }


        if (database.blockCount() != 346)
        {
            std::printf(
                "Unexpected block count: %u\n",
                database.blockCount());

            return false;
        }


        if (database.scriptCount() != 176)
        {
            std::printf(
                "Unexpected script count: %u\n",
                database.scriptCount());

            return false;
        }


        if (database.decompositionRecordCount() != 2081)
        {
            std::printf(
                "Unexpected decomposition record count: %u\n",
                database.decompositionRecordCount());

            return false;
        }


        // --------------------------------------------------------------------
        // Small semantic smoke test.
        // --------------------------------------------------------------------

        if (database.generalCategory(0x0041) !=
            UnicodeGeneralCategory::UppercaseLetter)
        {
            std::printf(
                "General_Category smoke test failed for U+0041\n");

            return false;
        }


        if (database.combiningClass(0x0301) != 230)
        {
            std::printf(
                "CCC smoke test failed for U+0301\n");

            return false;
        }


        UnicodeDecomposition decomposition =
            database.decomposition();


        const UnicodeDecompositionRecord* record =
            decomposition.record(0x00E9);


        if (!record ||
            record->first != 0x0065 ||
            record->second != 0x0301)
        {
            std::printf(
                "Canonical decomposition smoke test failed for U+00E9\n");

            return false;
        }

        // grapheme cluster break smoke test
        if (database.graphemeClusterBreak(0x0041) !=
            UnicodeGraphemeClusterBreak::Other)
        {
            std::printf(
                "Grapheme_Cluster_Break smoke test failed for U+0041\n");

            return false;
        }


        if (database.graphemeClusterBreak(0x0301) !=
            UnicodeGraphemeClusterBreak::Extend)
        {
            std::printf(
                "Grapheme_Cluster_Break smoke test failed for U+0301\n");

            return false;
        }


        if (database.graphemeClusterBreak(0x200D) !=
            UnicodeGraphemeClusterBreak::ZWJ)
        {
            std::printf(
                "Grapheme_Cluster_Break smoke test failed for U+200D\n");

            return false;
        }


        if (database.graphemeClusterBreak(0x1F1E6) !=
            UnicodeGraphemeClusterBreak::RegionalIndicator)
        {
            std::printf(
                "Grapheme_Cluster_Break smoke test failed for U+1F1E6\n");

            return false;
        }


        if (database.graphemeClusterBreak(0xAC00) !=
            UnicodeGraphemeClusterBreak::LV)
        {
            std::printf(
                "Grapheme_Cluster_Break smoke test failed for U+AC00\n");

            return false;
        }


        std::printf(
            "Runtime validation: PASS\n"
            "  Database bytes:          %zu\n"
            "  Blocks:                  %u\n"
            "  Scripts:                 %u\n"
            "  VALUE8 properties:       %u\n"
            "  VALUE8 tables:           %u\n"
            "  Decomposition masters:   %u\n"
            "  Decomposition pages:     %u\n"
            "  Decomposition records:   %u\n",
            file.bytes.size(),
            database.blockCount(),
            database.scriptCount(),
            database.valueProperty8Count(),
            database.valueTable8Count(),
            database.decompositionMasterPageCount(),
            database.decompositionPageCount(),
            database.decompositionRecordCount());


        return true;
    }


    // ========================================================================
    // buildUnicodeDatabase17
    //
    // Build the complete Unicode 17.0.0 database currently supported by
    // unicover and write it to disk.
    //
    // Expected directory structure beneath ucdRoot:
    //
    //      Blocks.txt
    //      PropertyValueAliases.txt
    //      Scripts.txt
    //      UnicodeData.txt
    //
    //      extracted/
    //          DerivedGeneralCategory.txt
    //          DerivedCombiningClass.txt
    //
    // ========================================================================

    static inline bool buildUnicodeDatabase17(
        const char* ucdRoot,
        const char* outputFilename)
    {
        if (!ucdRoot ||
            !outputFilename)
        {
            return false;
        }


        // ====================================================================
        // Database
        // ====================================================================

        UnicodeDatabaseBuilder database;


        // Some inexpensive generator-side reservations.
        database.reserveBlocks(346);
        database.reserveProperties(1);
        database.reserveScripts(176);
        database.reserveValueProperties8(5);
        database.reserveValueTables8(5);


        // ====================================================================
        // Blocks.txt
        // ====================================================================

        {
            const std::string filename =
                ucdJoinPath(
                    ucdRoot,
                    "Blocks.txt");


            UCDSourceFile source;


            if (!ucdLoadFile(
                filename,
                source))
            {
                return false;
            }


            UCDBlocksParseResult result;


            if (!ucdParseBlocks(
                source.span(),
                database,
                result))
            {
                std::printf(
                    "Blocks.txt parse failed\n"
                    "  Error: %s\n"
                    "  Line:  %u\n",
                    ucdBlocksParseErrorString(
                        result.error),
                    result.lineNumber);

                return false;
            }


            std::printf(
                "Blocks.txt: PASS\n"
                "  Blocks: %u\n",
                result.blockCount);
        }


        // ====================================================================
        // PropertyValueAliases.txt
        //
        // Currently used to establish the complete Script value set and
        // preferred ISO 15924 aliases before Scripts.txt is parsed.
        // ====================================================================

        UCDScriptValueAliases scriptAliases;


        {
            const std::string filename =
                ucdJoinPath(
                    ucdRoot,
                    "PropertyValueAliases.txt");


            UCDSourceFile source;


            if (!ucdLoadFile(
                filename,
                source))
            {
                return false;
            }


            UCDPropertyValueAliasesParseResult result;


            if (!ucdParseScriptValueAliases(
                source.span(),
                scriptAliases,
                result))
            {
                std::printf(
                    "PropertyValueAliases.txt parse failed\n"
                    "  Error: %s\n"
                    "  Line:  %u\n",
                    ucdPropertyValueAliasesParseErrorString(
                        result.error),
                    result.lineNumber);

                return false;
            }


            std::printf(
                "PropertyValueAliases.txt: PASS\n"
                "  Script aliases: %u\n",
                result.scriptAliasCount);
        }

        // ====================================================================
        // Extended_Pictographic
        // ====================================================================

        {
            const std::string filename = ucdJoinPath(ucdRoot, "emoji/emoji-data.txt");

            UCDSourceFile source;

            if (!ucdLoadFile(filename, source))
                return false;


            UnicodeCoverageBuilder coverage;
            UCDExtendedPictographicParseResult result;

            if (!ucdParseExtendedPictographic(source.span(), coverage, result))
            {
                std::printf(
                    "emoji-data.txt Extended_Pictographic parse failed\n"
                    "  Error: %s\n"
                    "  Line:  %u\n",
                    ucdExtendedPictographicParseErrorString(result.error),
                    result.lineNumber);

                return false;
            }


            UnicodeCoverageData coverageData{};

            if (!coverage.finalize(database.pagePool(), coverageData))
            {
                std::printf(
                    "Extended_Pictographic coverage finalization failed\n");

                return false;
            }


            UnicodeCoverageIndex coverageIndex;

            if (!database.addCoverage(coverageData, coverageIndex))
            {
                std::printf(
                    "Unable to add Extended_Pictographic coverage\n");

                return false;
            }


            InternedKey name = WSNameSet::INTERN("Extended_Pictographic");

            if (!database.addProperty(
                name, coverageIndex, UnicodePropertySourceEmojiData))
            {
                std::printf(
                    "Unable to register Extended_Pictographic property\n");

                return false;
            }


            std::printf(
                "emoji-data.txt Extended_Pictographic: PASS\n"
                "  Ranges:      %u\n"
                "  Codepoints:  %zu\n",
                result.rangeCount,
                result.codePoints);
        }

        // ====================================================================
        // Scripts.txt
        //
        // Scripts are SET-valued database records in the current design:
        //
        //      Script value -> UnicodeCoverage
        //
        // ====================================================================

        {
            const std::string filename =
                ucdJoinPath(
                    ucdRoot,
                    "Scripts.txt");


            UCDSourceFile source;


            if (!ucdLoadFile(
                filename,
                source))
            {
                return false;
            }


            UCDScriptsParseResult result;


            if (!ucdParseScripts(
                source.span(),
                scriptAliases,
                database,
                result))
            {
                std::printf(
                    "Scripts.txt parse failed\n"
                    "  Error: %s\n"
                    "  Line:  %u\n",
                    ucdScriptsParseErrorString(
                        result.error),
                    result.lineNumber);

                return false;
            }


            std::printf(
                "Scripts.txt: PASS\n"
                "  Ranges:              %u\n"
                "  Scripts:             %u\n"
                "  Explicit codepoints: %zu\n"
                "  Unknown codepoints:  %zu\n",
                result.rangeCount,
                result.scriptCount,
                result.explicitCodePoints,
                result.unknownCodePoints);
        }


        // ====================================================================
        // General_Category
        //
        // Build VALUE8 table #1.
        // ====================================================================

        {
            const std::string filename =
                ucdJoinPath(
                    ucdRoot,
                    "extracted/DerivedGeneralCategory.txt");


            UCDSourceFile source;


            if (!ucdLoadFile(
                filename,
                source))
            {
                return false;
            }


            auto values =
                std::make_unique<UnicodeValueTable8Builder>();


            UCDGeneralCategoryParseResult result;


            if (!ucdParseGeneralCategory(
                source.span(),
                *values,
                result))
            {
                std::printf(
                    "DerivedGeneralCategory.txt parse failed\n"
                    "  Error: %s\n"
                    "  Line:  %u\n",
                    ucdGeneralCategoryParseErrorString(
                        result.error),
                    result.lineNumber);

                return false;
            }


            UnicodeValueTable8Data table{};


            if (!values->finalize(
                database.valuePagePool8(),
                table))
            {
                std::printf(
                    "General_Category VALUE8 finalization failed\n");

                return false;
            }


            UnicodeValueTable8Index tableIndex;


            if (!database.addValueTable8(
                table,
                tableIndex))
            {
                std::printf(
                    "Unable to add General_Category VALUE8 table\n");

                return false;
            }


            if (!database.addValueProperty8(
                UnicodeValueProperty8GeneralCategory,
                tableIndex))
            {
                std::printf(
                    "Unable to register General_Category property\n");

                return false;
            }


            std::printf(
                "DerivedGeneralCategory.txt: PASS\n"
                "  Ranges:            %u\n"
                "  Assigned codepoints: %zu\n",
                result.rangeCount,
                result.assignedCodePoints);
        }


        // ====================================================================
        // Canonical_Combining_Class
        //
        // Build VALUE8 table #2 using the SAME VALUE8 page pool.
        // ====================================================================

        {
            const std::string filename =
                ucdJoinPath(
                    ucdRoot,
                    "extracted/DerivedCombiningClass.txt");


            UCDSourceFile source;


            if (!ucdLoadFile(
                filename,
                source))
            {
                return false;
            }


            auto values =
                std::make_unique<UnicodeValueTable8Builder>();


            UCDCombiningClassParseResult result;


            if (!ucdParseCombiningClass(
                source.span(),
                *values,
                result))
            {
                std::printf(
                    "DerivedCombiningClass.txt parse failed\n"
                    "  Error: %s\n"
                    "  Line:  %u\n",
                    ucdCombiningClassParseErrorString(
                        result.error),
                    result.lineNumber);

                return false;
            }


            UnicodeValueTable8Data table{};


            if (!values->finalize(
                database.valuePagePool8(),
                table))
            {
                std::printf(
                    "Canonical_Combining_Class VALUE8 finalization failed\n");

                return false;
            }


            UnicodeValueTable8Index tableIndex;


            if (!database.addValueTable8(
                table,
                tableIndex))
            {
                std::printf(
                    "Unable to add Canonical_Combining_Class VALUE8 table\n");

                return false;
            }


            if (!database.addValueProperty8(
                UnicodeValueProperty8CanonicalCombiningClass,
                tableIndex))
            {
                std::printf(
                    "Unable to register Canonical_Combining_Class property\n");

                return false;
            }


            std::printf(
                "DerivedCombiningClass.txt: PASS\n"
                "  Ranges:               %u\n"
                "  Explicit codepoints:  %zu\n"
                "  Non-zero codepoints:  %zu\n",
                result.rangeCount,
                result.explicitCodePoints,
                result.nonZeroCodePoints);
        }

        // ====================================================================
        // Bidi_Class
        //
        // Build VALUE8 table #3 using the same shared VALUE8 page pool.
        // ====================================================================

        {
            const std::string filename =
                ucdJoinPath(
                    ucdRoot,
                    "extracted/DerivedBidiClass.txt");


            UCDSourceFile source;


            if (!ucdLoadFile(
                filename,
                source))
            {
                return false;
            }


            auto values =
                std::make_unique<UnicodeValueTable8Builder>();


            UCDBidiClassParseResult result;


            if (!ucdParseBidiClass(
                source.span(),
                *values,
                result))
            {
                std::printf(
                    "DerivedBidiClass.txt parse failed\n"
                    "  Error: %s\n"
                    "  Line:  %u\n",
                    ucdBidiClassParseErrorString(
                        result.error),
                    result.lineNumber);

                return false;
            }


            UnicodeValueTable8Data table{};


            if (!values->finalize(
                database.valuePagePool8(),
                table))
            {
                std::printf(
                    "Bidi_Class VALUE8 finalization failed\n");

                return false;
            }


            UnicodeValueTable8Index tableIndex;


            if (!database.addValueTable8(
                table,
                tableIndex))
            {
                std::printf(
                    "Unable to add Bidi_Class VALUE8 table\n");

                return false;
            }


            if (!database.addValueProperty8(
                UnicodeValueProperty8BidiClass,
                tableIndex))
            {
                std::printf(
                    "Unable to register Bidi_Class property\n");

                return false;
            }


            std::printf(
                "DerivedBidiClass.txt: PASS\n"
                "  Ranges:               %u\n"
                "  @missing ranges:      %u\n"
                "  Explicit codepoints:  %zu\n"
                "  Defaulted codepoints: %zu\n",
                result.rangeCount,
                result.missingRangeCount,
                result.explicitCodePoints,
                result.defaultedCodePoints);
        }

        // ========================================================================
        // Grapheme_Cluster_Break
        //
        // Build VALUE8 table using the shared VALUE8 page pool.
        // ========================================================================

        {
            const std::string filename =
                ucdJoinPath(ucdRoot, "auxiliary/GraphemeBreakProperty.txt");

            UCDSourceFile source;

            if (!ucdLoadFile(filename, source))
                return false;


            auto values = std::make_unique<UnicodeValueTable8Builder>();

            UCDGraphemeClusterBreakParseResult result;

            if (!ucdParseGraphemeClusterBreak(source.span(), *values, result))
            {
                std::printf(
                    "GraphemeBreakProperty.txt parse failed\n"
                    "  Error: %s\n"
                    "  Line:  %u\n",
                    ucdGraphemeClusterBreakParseErrorString(result.error),
                    result.lineNumber);

                return false;
            }


            UnicodeValueTable8Data table{};

            if (!values->finalize(database.valuePagePool8(), table))
            {
                std::printf(
                    "Grapheme_Cluster_Break VALUE8 finalization failed\n");

                return false;
            }


            UnicodeValueTable8Index tableIndex;

            if (!database.addValueTable8(table, tableIndex))
            {
                std::printf(
                    "Unable to add Grapheme_Cluster_Break VALUE8 table\n");

                return false;
            }


            if (!database.addValueProperty8(
                UnicodeValueProperty8GraphemeClusterBreak, tableIndex))
            {
                std::printf(
                    "Unable to register Grapheme_Cluster_Break property\n");

                return false;
            }


            std::printf(
                "GraphemeBreakProperty.txt: PASS\n"
                "  Ranges:                 %u\n"
                "  Explicit codepoints:    %zu\n"
                "  Defaulted codepoints:   %zu\n",
                result.rangeCount,
                result.explicitCodePoints,
                result.defaultedCodePoints);
        }


        // ====================================================================
// Indic_Conjunct_Break
//
// DerivedCoreProperties.txt contains multiple properties. The parser
// extracts only InCB records.
//
// Build VALUE8 table #5 using the shared VALUE8 page pool.
// ====================================================================

        {
            const std::string filename =
                ucdJoinPath(
                    ucdRoot,
                    "DerivedCoreProperties.txt");


            UCDSourceFile source;

            if (!ucdLoadFile(
                filename,
                source))
            {
                return false;
            }


            auto values =
                std::make_unique<UnicodeValueTable8Builder>();


            UCDIndicConjunctBreakParseResult result;

            if (!ucdParseIndicConjunctBreak(
                source.span(),
                *values,
                result))
            {
                std::printf(
                    "DerivedCoreProperties.txt Indic_Conjunct_Break parse failed\n"
                    "  Error: %s\n"
                    "  Line:  %u\n",
                    ucdIndicConjunctBreakParseErrorString(
                        result.error),
                    result.lineNumber);

                return false;
            }


            UnicodeValueTable8Data table{};

            if (!values->finalize(
                database.valuePagePool8(),
                table))
            {
                std::printf(
                    "Indic_Conjunct_Break VALUE8 finalization failed\n");

                return false;
            }


            UnicodeValueTable8Index tableIndex;

            if (!database.addValueTable8(
                table,
                tableIndex))
            {
                std::printf(
                    "Unable to add Indic_Conjunct_Break VALUE8 table\n");

                return false;
            }


            if (!database.addValueProperty8(
                UnicodeValueProperty8IndicConjunctBreak,
                tableIndex))
            {
                std::printf(
                    "Unable to register Indic_Conjunct_Break property\n");

                return false;
            }


            std::printf(
                "DerivedCoreProperties.txt Indic_Conjunct_Break: PASS\n"
                "  Ranges:                 %u\n"
                "  Explicit codepoints:    %zu\n"
                "  Defaulted codepoints:   %zu\n",
                result.rangeCount,
                result.explicitCodePoints,
                result.defaultedCodePoints);
        }


        // ====================================================================
        // Canonical decomposition
        //
        // UnicodeData.txt supplies direct canonical decomposition records.
        // Compatibility decompositions are parsed but not stored.
        // ====================================================================

        {
            const std::string filename =
                ucdJoinPath(
                    ucdRoot,
                    "UnicodeData.txt");


            UCDSourceFile source;


            if (!ucdLoadFile(
                filename,
                source))
            {
                return false;
            }


            auto decomposition = std::make_unique<UnicodeDecompositionBuilder>();

            decomposition->reserveRecords(2200);


            UCDUnicodeDataParseResult result;


            if (!ucdParseUnicodeData(
                source.span(),
                *decomposition,
                result))
            {
                std::printf(
                    "UnicodeData.txt parse failed\n"
                    "  Error: %s\n"
                    "  Line:  %u\n",
                    ucdUnicodeDataParseErrorString(
                        result.error),
                    result.lineNumber);

                return false;
            }


            UnicodeDecompositionData data{};


            if (!decomposition->finalize(
                database.decompositionPagePool(),
                data))
            {
                std::printf(
                    "Canonical decomposition finalization failed\n");

                return false;
            }


            if (!database.setDecomposition(
                data,
                decomposition->records()))
            {
                std::printf(
                    "Unable to register canonical decomposition\n");

                return false;
            }


            std::printf(
                "UnicodeData.txt: PASS\n"
                "  Records:                %zu\n"
                "  Canonical mappings:     %zu\n"
                "  Singleton mappings:     %zu\n"
                "  Pair mappings:          %zu\n"
                "  Compatibility mappings: %zu\n",
                result.recordCount,
                result.canonicalMappingCount,
                result.singletonMappingCount,
                result.pairMappingCount,
                result.compatibilityMappingCount);

            // ====================================================================
// Full_Composition_Exclusion
//
// DerivedNormalizationProps.txt identifies canonical decomposition
// mappings which must not participate in NFC composition.
//
// This coverage is generator-only. It is used to construct the final
// composition table and is not persisted in the database.
// ====================================================================

            UnicodeCoverageBuilder fullCompositionExclusion;

            {
                const std::string normalizationFilename =
                    ucdJoinPath(
                        ucdRoot,
                        "DerivedNormalizationProps.txt");


                UCDSourceFile normalizationSource;

                if (!ucdLoadFile(
                    normalizationFilename,
                    normalizationSource))
                {
                    return false;
                }


                UCDNormalizationPropsParseResult normalizationResult;

                if (!ucdParseFullCompositionExclusion(
                    normalizationSource.span(),
                    fullCompositionExclusion,
                    normalizationResult))
                {
                    std::printf(
                        "DerivedNormalizationProps.txt parse failed\n"
                        "  Line: %u\n",
                        normalizationResult.lineNumber);

                    return false;
                }


                std::printf(
                    "DerivedNormalizationProps.txt: PASS\n"
                    "  Full_Composition_Exclusion ranges: %u\n"
                    "  Excluded codepoints:               %zu\n",
                    normalizationResult.rangeCount,
                    normalizationResult.codePointCount);
            }


            // ====================================================================
            // Canonical composition
            //
            // The decomposition builder contains the DIRECT canonical mappings
            // parsed from UnicodeData.txt:
            //
            //      composite -> first [second]
            //
            // Composition is the reverse of direct two-code-point mappings:
            //
            //      first + second -> composite
            //
            // Singleton decompositions are not composition candidates.
            //
            // Full_Composition_Exclusion is applied by UnicodeCompositionBuilder.
            // Hangul remains algorithmic and is not stored here.
            // ====================================================================

            UnicodeCompositionBuilder composition;
            composition.reserve(1100);


            for (uint32_t cp = 0; cp < kUnicodeLimit; ++cp)
            {
                const UnicodeDecompositionRecord* record =
                    decomposition->record(cp);

                if (!record)
                    continue;

                if (record->second ==
                    kUnicodeDecompositionSecondNone)
                {
                    continue;
                }


                if (!composition.addCanonicalPair(
                    cp,
                    record->first,
                    record->second,
                    fullCompositionExclusion))
                {
                    std::printf(
                        "Canonical composition construction failed\n"
                        "  Composite: U+%04X\n"
                        "  First:     U+%04X\n"
                        "  Second:    U+%04X\n",
                        cp,
                        record->first,
                        record->second);

                    return false;
                }
            }


            // ====================================================================
            // Finalize canonical composition
            // ====================================================================

            std::vector<UnicodeCompositionRecord> compositionRecords;
            UnicodeCompositionBuildResult compositionResult;


            if (!composition.finalize(
                compositionRecords,
                compositionResult))
            {
                std::printf(
                    "Canonical composition finalization failed\n"
                    "  Error: %s\n",
                    unicodeCompositionBuildErrorString(
                        compositionResult.error));

                return false;
            }


            if (!database.setComposition(
                std::move(compositionRecords)))
            {
                std::printf(
                    "Unable to register canonical composition\n");

                return false;
            }


            std::printf(
                "Canonical composition: PASS\n"
                "  Candidate pairs: %zu\n"
                "  Excluded pairs:  %zu\n"
                "  Stored pairs:    %zu\n",
                compositionResult.candidatePairCount,
                compositionResult.excludedPairCount,
                compositionResult.compositionPairCount);


        }


        // ====================================================================
        // Builder summary before serialization
        // ====================================================================

        const UnicodeDatabaseBuilderStats& databaseStats =
            database.stats();


        const UnicodePagePoolStats& setStats =
            database.pagePool().stats();


        const UnicodeValuePagePoolStats8& valueStats =
            database.valuePagePool8().stats();


        const UnicodeDecompositionPagePoolStats& decompositionStats =
            database.decompositionPagePool().stats();


        std::printf(
            "\nDatabase construction complete\n"
            "  Blocks:                   %zu\n"
            "  Scripts:                  %zu\n"
            "  Binary properties:        %zu\n"
            "  Coverages:                %zu\n"
            "  VALUE8 properties:        %zu\n"
            "  VALUE8 tables:            %zu\n"
            "  Decomposition records:    %zu\n"
            "  Composition records:      %zu\n"
            "  String pool bytes:        %zu\n",
            database.blockCount(),
            database.scriptCount(),
            database.propertyCount(),
            database.coverageCount(),
            database.valueProperty8Count(),
            database.valueTable8Count(),
            database.decompositionRecordCount(),
            database.compositionRecordCount(),
            database.stringPoolSize());


        std::printf(
            "  SET master pages:         %zu\n"
            "  SET bit pages:            %zu\n"
            "  VALUE8 master pages:      %zu\n"
            "  VALUE8 value pages:       %zu\n"
            "  Decomposition masters:    %zu\n"
            "  Decomposition pages:      %zu\n",
            database.pagePool().masterPageCount(),
            database.pagePool().bitPageCount(),
            database.valuePagePool8().masterPageCount(),
            database.valuePagePool8().valuePageCount(),
            database.decompositionPagePool().masterPageCount(),
            database.decompositionPagePool().pageCount());


        std::printf(
            "  Coverage requests:        %zu\n"
            "  Coverage reused:          %zu\n"
            "  Unique coverages:         %zu\n"
            "  VALUE8 root requests:     %zu\n"
            "  VALUE8 roots reused:      %zu\n"
            "  Unique VALUE8 roots:      %zu\n",
            databaseStats.coverageRequests,
            databaseStats.coverageReused,
            databaseStats.uniqueCoverages,
            databaseStats.valueTable8Requests,
            databaseStats.valueTable8Reused,
            databaseStats.uniqueValueTables8);


        // Silence unused warnings if some stats are not printed yet.
        (void)setStats;
        (void)valueStats;
        (void)decompositionStats;


        // ====================================================================
        // Serialize real .ucdb file
        // ====================================================================

        UnicodeDatabaseWriteResult writeResult;


        if (!UnicodeDatabaseWriter::writeFile(
            outputFilename,
            database,
            17,
            0,
            0,
            writeResult))
        {
            std::printf(
                "Unicode database write failed\n"
                "  Error: %s\n",
                unicodeDatabaseWriteErrorString(
                    writeResult.error));

            return false;
        }


        std::printf(
            "\nDatabase written: %s\n"
            "  Bytes: %u\n",
            outputFilename,
            writeResult.databaseSize);


        // ====================================================================
        // Reload the actual file from disk.
        //
        // Do not merely validate an in-memory writer buffer here. The goal of
        // this entry point is specifically to prove that the final on-disk
        // artifact is usable by the runtime reader.
        // ====================================================================

        if (!verifyWrittenUnicodeDatabase( outputFilename))
        {
            return false;
        }


        std::printf(
            "\nUnicode 17.0.0 database generation: PASS\n");


        return true;
    }

} // namespace waavs