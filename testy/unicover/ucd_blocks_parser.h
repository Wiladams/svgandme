// ucd_blocks_parser.h

#pragma once

#include <cstdint>

#include "core_nametable.h"

#include "ucd_parser.h"
#include "unicode_coverage_builder.h"
#include "unicode_database_builder.h"


namespace waavs
{
    // ========================================================================
    // UCDBlocksParseError
    // ========================================================================

    enum class UCDBlocksParseError : uint8_t
    {
        None = 0,

        InvalidRange,
        MissingBlockName,
        UnexpectedField,
        InvalidBlockOrder,
        NameInternFailed,
        CoverageFinalizeFailed,
        CoverageAddFailed,
        BlockAddFailed
    };


    // ========================================================================
    // UCDBlocksParseResult
    //
    // lineNumber:
    //
    //      Physical source line on which an error occurred.
    //      Zero on successful completion.
    //
    // blockCount:
    //
    //      Number of blocks successfully added.
    //
    // ========================================================================

    struct UCDBlocksParseResult
    {
        UCDBlocksParseError error{ UCDBlocksParseError::None };

        uint32_t lineNumber{ 0 };
        uint32_t blockCount{ 0 };


        [[nodiscard]]
        bool success() const noexcept
        {
            return error == UCDBlocksParseError::None;
        }


        explicit operator bool() const noexcept
        {
            return success();
        }
    };


    // ========================================================================
    // ucdBlocksParseErrorString
    //
    // Diagnostic text without imposing any logging or reporting policy.
    // ========================================================================

    static inline const char* ucdBlocksParseErrorString(UCDBlocksParseError error) noexcept
    {
        switch (error)
        {
        case UCDBlocksParseError::None:
            return "no error";

        case UCDBlocksParseError::InvalidRange:
            return "invalid Unicode block range";

        case UCDBlocksParseError::MissingBlockName:
            return "missing Unicode block name";

        case UCDBlocksParseError::UnexpectedField:
            return "unexpected extra field in Unicode block record";

        case UCDBlocksParseError::InvalidBlockOrder:
            return "Unicode blocks are not ordered or overlap";

        case UCDBlocksParseError::NameInternFailed:
            return "failed to intern Unicode block name";

        case UCDBlocksParseError::CoverageFinalizeFailed:
            return "failed to finalize Unicode block coverage";

        case UCDBlocksParseError::CoverageAddFailed:
            return "failed to add Unicode block coverage";

        case UCDBlocksParseError::BlockAddFailed:
            return "failed to add Unicode block record";
        }

        return "unknown Unicode block parser error";
    }


    // ========================================================================
    // ucdParseBlocks
    //
    // Parse Blocks.txt and add its contents to UnicodeDatabaseBuilder.
    //
    // Expected meaningful line syntax:
    //
    //      range ; block-name
    //
    // Examples:
    //
    //      0000..007F; Basic Latin
    //      0080..00FF; Latin-1 Supplement
    //      0100..017F; Latin Extended-A
    //
    // Comments and blank lines are already handled by UCDParser.
    //
    //
    // For every block:
    //
    //      parse range
    //          |
    //          v
    //      UnicodeCoverageBuilder
    //          |
    //          v
    //      UnicodeCoverageData
    //          |
    //          v
    //      database.addCoverage()
    //          |
    //          v
    //      database.addBlock()
    //
    //
    // On failure the database builder may contain records produced before
    // the error.  Generation should therefore be considered failed and the
    // builder discarded or cleared.
    //
    // ========================================================================

    static inline bool ucdParseBlocks(const ByteSpan& source, UnicodeDatabaseBuilder& database, UCDBlocksParseResult& outResult)
    {
        outResult = {};


        UCDParser parser(source);

        UCDLine line;


        uint32_t previousLast = 0;
        bool havePrevious = false;


        while (parser.next(line))
        {
            ByteSpan fields =
                line.data;


            // ---------------------------------------------------------------
            // Field 1: code-point range
            // ---------------------------------------------------------------

            UCDCodePointRange range;


            if (!ucdReadCodePointRange(fields, range))
            {
                outResult.error =
                    UCDBlocksParseError::InvalidRange;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ---------------------------------------------------------------
            // Field 2: block name
            // ---------------------------------------------------------------

            ByteSpan blockName;


            if (!ucdReadField(fields, blockName) ||
                !blockName)
            {
                outResult.error =
                    UCDBlocksParseError::MissingBlockName;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ---------------------------------------------------------------
            // Blocks.txt should contain exactly two fields.
            // ---------------------------------------------------------------

            bspan_trim_spaces(fields);


            if (fields)
            {
                outResult.error =
                    UCDBlocksParseError::UnexpectedField;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ---------------------------------------------------------------
            // Blocks must be ordered and non-overlapping.
            //
            // UnicodeDatabaseBuilder also enforces this invariant, but doing
            // it here produces a precise parser-level error.
            // ---------------------------------------------------------------

            if (havePrevious &&
                range.first <= previousLast)
            {
                outResult.error =
                    UCDBlocksParseError::InvalidBlockOrder;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ---------------------------------------------------------------
            // Canonical block name
            // ---------------------------------------------------------------

            InternedKey name = WSNameSet::INTERN(blockName);


            if (!name)
            {
                outResult.error =
                    UCDBlocksParseError::NameInternFailed;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ---------------------------------------------------------------
            // Build coverage for this block.
            //
            // Blocks are contiguous ranges, so each block requires exactly
            // one addRange() operation.
            // ---------------------------------------------------------------

            UnicodeCoverageBuilder coverageBuilder;


            coverageBuilder.addRange(
                range.first,
                range.last);


            UnicodeCoverageData coverageData{};


            if (!coverageBuilder.finalize(
                database.pagePool(),
                coverageData))
            {
                outResult.error =
                    UCDBlocksParseError::CoverageFinalizeFailed;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ---------------------------------------------------------------
            // Intern complete coverage into the database coverage table.
            // ---------------------------------------------------------------

            UnicodeCoverageIndex coverageIndex;


            if (!database.addCoverage(
                coverageData,
                coverageIndex))
            {
                outResult.error =
                    UCDBlocksParseError::CoverageAddFailed;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            // ---------------------------------------------------------------
            // Add block metadata.
            // ---------------------------------------------------------------

            if (!database.addBlock(
                range.first,
                range.last,
                name,
                coverageIndex))
            {
                outResult.error =
                    UCDBlocksParseError::BlockAddFailed;

                outResult.lineNumber =
                    line.lineNumber;

                return false;
            }


            previousLast =
                range.last;

            havePrevious =
                true;


            ++outResult.blockCount;
        }


        return true;
    }


    // ========================================================================
    // Convenience overload
    //
    // Parse when detailed diagnostics are not required.
    // ========================================================================

    static inline bool ucdParseBlocks(const ByteSpan& source, UnicodeDatabaseBuilder& database)
    {
        UCDBlocksParseResult result;

        return
            ucdParseBlocks(
                source,
                database,
                result);
    }

} // namespace waavs
