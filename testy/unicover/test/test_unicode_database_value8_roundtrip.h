// test_unicode_database_value8_roundtrip.h

#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>


#include "test_core.h"

#include "ucd_combining_class_parser.h"
#include "ucd_general_category_parser.h"

#include "unicode_combining_class.h"
#include "unicode_database.h"
#include "unicode_database_builder.h"
#include "unicode_database_writer.h"
#include "unicode_general_category.h"
#include "unicode_value_table8_builder.h"


namespace waavs
{
    // ========================================================================
    // testUnicodeDatabaseValue8RoundTrip
    //
    // End-to-end VALUE8 persistence test:
    //
    //      DerivedGeneralCategory.txt
    //      DerivedCombiningClass.txt
    //              |
    //              v
    //      UnicodeValueTable8Builder
    //              |
    //              v
    //      shared UnicodeDatabaseBuilder VALUE8 pool
    //              |
    //              v
    //      UnicodeDatabaseWriter
    //              |
    //              v
    //      serialized UCDB image
    //              |
    //              v
    //      UnicodeDatabase::reset()
    //              |
    //              v
    //      generalCategory()
    //      combiningClass()
    //
    // ========================================================================

    static bool testUnicodeDatabaseValue8RoundTrip(
        const ByteSpan& generalCategorySource,
        const ByteSpan& combiningClassSource)
    {
        auto fail = [](const char* message) -> bool {
            std::printf(
                "VALUE8 database round-trip: FAIL: %s\n",
                message);

            return false;
            };


        // ====================================================================
        // Database builder
        // ====================================================================

        UnicodeDatabaseBuilder database;


        // UnicodeValueTable8Builder is approximately 1.1 MiB. Reuse one
        // heap-allocated builder for both properties.
        auto values =
            std::make_unique<UnicodeValueTable8Builder>();


        // ====================================================================
        // General_Category
        // ====================================================================

        UCDGeneralCategoryParseResult generalCategoryResult;


        if (!ucdParseGeneralCategory(
            generalCategorySource,
            *values,
            generalCategoryResult))
        {
            std::printf(
                "VALUE8 database round-trip: FAIL: General_Category parse\n"
                "  Error:           %s\n"
                "  Line:            %u\n"
                "  Ranges:          %u\n"
                "  Code points:     %zu\n",
                ucdGeneralCategoryParseErrorString(
                    generalCategoryResult.error),
                generalCategoryResult.lineNumber,
                generalCategoryResult.rangeCount,
                generalCategoryResult.assignedCodePoints);

            return false;
        }


        UnicodeValueTable8Data generalCategoryData{};


        if (!values->finalize(
            database.valuePagePool8(),
            generalCategoryData))
        {
            return fail(
                "unable to finalize General_Category");
        }


        UnicodeValueTable8Index generalCategoryIndex =
            kUnicodeValueTable8IndexInvalid;


        if (!database.addValueTable8(
            generalCategoryData,
            generalCategoryIndex))
        {
            return fail(
                "unable to add General_Category VALUE8 table");
        }


        if (!database.addValueProperty8(
            UnicodeValueProperty8GeneralCategory,
            generalCategoryIndex))
        {
            return fail(
                "unable to add General_Category VALUE8 property");
        }


        // ====================================================================
        // Canonical_Combining_Class
        //
        // ucdParseCombiningClass() clears the logical VALUE8 builder before
        // populating it, so the same dense builder can be reused.
        //
        // The database VALUE8 page pool is NOT cleared. Thus GC and CCC share
        // one physical persistent pool.
        // ====================================================================

        UCDCombiningClassParseResult combiningClassResult;


        if (!ucdParseCombiningClass(
            combiningClassSource,
            *values,
            combiningClassResult))
        {
            std::printf(
                "VALUE8 database round-trip: FAIL: CCC parse\n"
                "  Error:                %s\n"
                "  Line:                 %u\n"
                "  Ranges:               %u\n"
                "  Explicit code points: %zu\n"
                "  Non-zero code points: %zu\n",
                ucdCombiningClassParseErrorString(
                    combiningClassResult.error),
                combiningClassResult.lineNumber,
                combiningClassResult.rangeCount,
                combiningClassResult.explicitCodePoints,
                combiningClassResult.nonZeroCodePoints);

            return false;
        }


        UnicodeValueTable8Data combiningClassData{};


        if (!values->finalize(
            database.valuePagePool8(),
            combiningClassData))
        {
            return fail(
                "unable to finalize Canonical_Combining_Class");
        }


        UnicodeValueTable8Index combiningClassIndex =
            kUnicodeValueTable8IndexInvalid;


        if (!database.addValueTable8(
            combiningClassData,
            combiningClassIndex))
        {
            return fail(
                "unable to add Canonical_Combining_Class VALUE8 table");
        }


        if (!database.addValueProperty8(
            UnicodeValueProperty8CanonicalCombiningClass,
            combiningClassIndex))
        {
            return fail(
                "unable to add Canonical_Combining_Class VALUE8 property");
        }


        // ====================================================================
        // Generator-side structural checks
        // ====================================================================

        if (database.valueProperty8Count() != 2)
            return fail("database does not contain two VALUE8 properties");


        if (database.valueTable8Count() != 2)
            return fail("database does not contain two VALUE8 tables");


        if (generalCategoryIndex ==
            combiningClassIndex)
        {
            return fail(
                "General_Category and CCC unexpectedly deduplicated to one table");
        }


        // ====================================================================
        // Serialize
        // ====================================================================

        std::vector<uint8_t> image;

        UnicodeDatabaseWriteResult writeResult;


        if (!UnicodeDatabaseWriter::write(
            database,
            17,
            0,
            0,
            image,
            writeResult))
        {
            std::printf(
                "VALUE8 database round-trip: FAIL: writer\n"
                "  Error: %s\n",
                unicodeDatabaseWriteErrorString(
                    writeResult.error));

            return false;
        }


        if (image.empty())
            return fail("writer produced an empty database image");


        if (writeResult.databaseSize != image.size())
            return fail("reported database size does not match image size");


        // ====================================================================
        // Attach runtime database
        // ====================================================================

        ByteSpan databaseSpan(
            image.data(),
            image.size());


        UnicodeDatabase runtime;


        if (!runtime.reset(databaseSpan))
            return fail("UnicodeDatabase rejected serialized image");


        if (!runtime.valid())
            return fail("runtime database is not valid");


        // ====================================================================
        // Header / version
        // ====================================================================

        const UnicodeDatabaseHeader* header =
            runtime.header();


        if (!header)
            return fail("runtime database has no header");


        if (header->formatMajor !=
            kUnicodeDatabaseFormatMajor)
        {
            return fail("incorrect database format major version");
        }


        if (header->formatMinor !=
            kUnicodeDatabaseFormatMinor)
        {
            return fail("incorrect database format minor version");
        }


        if (runtime.unicodeMajor() != 17 ||
            runtime.unicodeMinor() != 0 ||
            runtime.unicodePatch() != 0)
        {
            return fail("incorrect Unicode source version");
        }


        // ====================================================================
        // Persisted VALUE8 structure
        // ====================================================================

        if (runtime.valueProperty8Count() != 2)
            return fail("runtime VALUE8 property count is not two");


        if (runtime.valueTable8Count() != 2)
            return fail("runtime VALUE8 table count is not two");


        if (!runtime.hasGeneralCategory())
            return fail("runtime General_Category property is missing");


        if (!runtime.hasCombiningClass())
            return fail("runtime Canonical_Combining_Class property is missing");


        if (runtime.valueMasterPage8Count() !=
            database.valuePagePool8().masterPageCount())
        {
            return fail("VALUE8 master-page count changed during serialization");
        }


        if (runtime.valuePage8Count() !=
            database.valuePagePool8().valuePageCount())
        {
            return fail("VALUE8 value-page count changed during serialization");
        }


        // ====================================================================
        // Property directory validation
        //
        // Do not depend on table numbers. Verify that both semantic property
        // identifiers survived serialization and point to valid tables.
        // ====================================================================

        bool foundGeneralCategory = false;
        bool foundCombiningClass = false;


        for (uint32_t i = 0;
            i < runtime.valueProperty8Count();
            ++i)
        {
            const UnicodeValueProperty8Record* record =
                runtime.valueProperty8Record(i);


            if (!record)
                return fail("unable to read VALUE8 property record");


            if (record->tableIndex >=
                runtime.valueTable8Count())
            {
                return fail("VALUE8 property has invalid table index");
            }


            switch (
                static_cast<UnicodeValueProperty8>(
                    record->property))
            {
            case UnicodeValueProperty8GeneralCategory:
                foundGeneralCategory = true;
                break;


            case UnicodeValueProperty8CanonicalCombiningClass:
                foundCombiningClass = true;
                break;


            default:
                return fail("unexpected VALUE8 property identifier");
            }
        }


        if (!foundGeneralCategory)
            return fail("General_Category directory entry did not survive");


        if (!foundCombiningClass)
            return fail("CCC directory entry did not survive");


        // ====================================================================
        // General_Category semantic checks
        // ====================================================================

        auto checkGeneralCategory =
            [&](uint32_t cp,
                UnicodeGeneralCategory expected,
                const char* description) -> bool
            {
                const UnicodeGeneralCategory actual =
                    runtime.generalCategory(cp);


                if (actual != expected)
                {
                    std::printf(
                        "VALUE8 database round-trip: FAIL: GC %s\n"
                        "  U+%04X expected=%u actual=%u\n",
                        description,
                        cp,
                        static_cast<unsigned>(expected),
                        static_cast<unsigned>(actual));

                    return false;
                }


                return true;
            };


        if (!checkGeneralCategory(
            0x0041,
            UnicodeGeneralCategory::UppercaseLetter,
            "Latin capital A"))
        {
            return false;
        }


        if (!checkGeneralCategory(
            0x0061,
            UnicodeGeneralCategory::LowercaseLetter,
            "Latin small a"))
        {
            return false;
        }


        if (!checkGeneralCategory(
            0x0030,
            UnicodeGeneralCategory::DecimalNumber,
            "digit zero"))
        {
            return false;
        }


        if (!checkGeneralCategory(
            0x0020,
            UnicodeGeneralCategory::SpaceSeparator,
            "space"))
        {
            return false;
        }


        if (!checkGeneralCategory(
            0x0301,
            UnicodeGeneralCategory::NonspacingMark,
            "combining acute accent"))
        {
            return false;
        }


        if (!checkGeneralCategory(
            0x0C15,
            UnicodeGeneralCategory::OtherLetter,
            "Telugu KA"))
        {
            return false;
        }


        if (!checkGeneralCategory(
            0x200D,
            UnicodeGeneralCategory::Format,
            "zero width joiner"))
        {
            return false;
        }


        if (!checkGeneralCategory(
            0x0378,
            UnicodeGeneralCategory::Unassigned,
            "unassigned code point"))
        {
            return false;
        }


        if (!checkGeneralCategory(
            0xD800,
            UnicodeGeneralCategory::Surrogate,
            "high surrogate"))
        {
            return false;
        }


        if (!checkGeneralCategory(
            0xE000,
            UnicodeGeneralCategory::PrivateUse,
            "private-use code point"))
        {
            return false;
        }


        // ====================================================================
        // Canonical_Combining_Class semantic checks
        // ====================================================================

        auto checkCombiningClass =
            [&](uint32_t cp,
                UnicodeCombiningClass expected,
                const char* description) -> bool
            {
                const UnicodeCombiningClass actual =
                    runtime.combiningClass(cp);


                if (actual != expected)
                {
                    std::printf(
                        "VALUE8 database round-trip: FAIL: CCC %s\n"
                        "  U+%04X expected=%u actual=%u\n",
                        description,
                        cp,
                        static_cast<unsigned>(expected),
                        static_cast<unsigned>(actual));

                    return false;
                }


                return true;
            };


        if (!checkCombiningClass(
            0x0041,
            0,
            "Latin capital A"))
        {
            return false;
        }


        if (!checkCombiningClass(
            0x0300,
            230,
            "combining grave accent"))
        {
            return false;
        }


        if (!checkCombiningClass(
            0x0301,
            230,
            "combining acute accent"))
        {
            return false;
        }


        if (!checkCombiningClass(
            0x0315,
            232,
            "combining comma above right"))
        {
            return false;
        }


        if (!checkCombiningClass(
            0x0327,
            202,
            "combining cedilla"))
        {
            return false;
        }


        if (!checkCombiningClass(
            0x0345,
            240,
            "combining Greek ypogegrammeni"))
        {
            return false;
        }


        if (!checkCombiningClass(
            0x0378,
            0,
            "unassigned code point"))
        {
            return false;
        }


        if (!checkCombiningClass(
            0xD800,
            0,
            "high surrogate"))
        {
            return false;
        }


        // ====================================================================
        // Out-of-range behavior
        // ====================================================================

        if (runtime.generalCategory(kUnicodeLimit) !=
            UnicodeGeneralCategory::Unassigned)
        {
            return fail(
                "out-of-range General_Category lookup did not return default");
        }


        if (runtime.combiningClass(kUnicodeLimit) !=
            kUnicodeCombiningClassNotReordered)
        {
            return fail(
                "out-of-range CCC lookup did not return default");
        }


        // ====================================================================
        // Diagnostics
        // ====================================================================

        const UnicodeValuePagePoolStats8& poolStats =
            database.valuePagePool8().stats();


        const UnicodeDatabaseBuilderStats& databaseStats =
            database.stats();


        std::printf(
            "VALUE8 database round-trip: PASS\n"
            "  Database bytes:            %zu\n"
            "  VALUE8 properties:         %u\n"
            "  VALUE8 tables:             %u\n",
            image.size(),
            runtime.valueProperty8Count(),
            runtime.valueTable8Count());


        std::printf(
            "  Value page requests:       %zu\n"
            "  Uniform value pages:       %zu\n"
            "  Reused value pages:        %zu\n"
            "  Unique value pages:        %zu\n"
            "  Value hash collisions:     %zu\n",
            poolStats.valuePageRequests,
            poolStats.valuePageUniform,
            poolStats.valuePageReused,
            poolStats.uniqueValuePages,
            poolStats.valuePageHashCollisions);


        std::printf(
            "  Master page requests:      %zu\n"
            "  Uniform master pages:      %zu\n"
            "  Reused master pages:       %zu\n"
            "  Unique master pages:       %zu\n"
            "  Master hash collisions:    %zu\n",
            poolStats.masterPageRequests,
            poolStats.masterPageUniform,
            poolStats.masterPageReused,
            poolStats.uniqueMasterPages,
            poolStats.masterPageHashCollisions);


        std::printf(
            "  VALUE8 root requests:      %zu\n"
            "  Reused VALUE8 roots:       %zu\n"
            "  Unique VALUE8 roots:       %zu\n"
            "  Root hash collisions:      %zu\n"
            "  Stored VALUE8 page bytes:  %zu\n",
            databaseStats.valueTable8Requests,
            databaseStats.valueTable8Reused,
            databaseStats.uniqueValueTables8,
            databaseStats.valueTable8HashCollisions,
            poolStats.storedBytes());


        return true;
    }


    static bool testUnicodeDatabaseValue8RoundTrip(
        const char * generalCategoryFilename,
        const char * combiningClassFilename)
    {
        std::vector<uint8_t> categoryFileData;

        if (!readFileData(generalCategoryFilename, categoryFileData))
        {
            printf("FAILED: unable to read file\n");
            printf("  file: %s\n", generalCategoryFilename);
            return false;
        }

        const ByteSpan generalCategorySource = ByteSpan(categoryFileData.data(), categoryFileData.size());


        std::vector<uint8_t> classFileData;

        if (!readFileData(combiningClassFilename, classFileData))
        {
            printf("FAILED: unable to read file\n");
            printf("  file: %s\n", combiningClassFilename);
            return false;
        }

        const ByteSpan combiningClassSource = ByteSpan(classFileData.data(), classFileData.size());

        return testUnicodeDatabaseValue8RoundTrip(
            generalCategorySource,
            combiningClassSource);

    }
} // namespace waavs