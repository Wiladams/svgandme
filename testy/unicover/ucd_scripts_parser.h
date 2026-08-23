// ucd_scripts_parser.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "core_nametable.h"
#include "core_openhashmap.h"

#include "ucd_parser.h"
#include "ucd_property_value_aliases_parser.h"
#include "unicode_coverage_builder.h"
#include "unicode_database_builder.h"


namespace waavs
{
    // ========================================================================
    // UCDScriptsParseError
    // ========================================================================

    enum class UCDScriptsParseError : uint8_t
    {
        None = 0,

        ExistingScripts,
        MissingAliases,
        MissingUnknownAlias,
        InvalidAlias,
        StorageFailed,

        InvalidRange,
        MissingScriptName,
        UnexpectedField,
        NameInternFailed,
        UnknownScriptName,
        OverlappingRange,

        CoverageFinalizeFailed,
        CoverageAddFailed,
        ScriptAddFailed
    };


    // ========================================================================
    // UCDScriptsParseResult
    // ========================================================================

    struct UCDScriptsParseResult
    {
        UCDScriptsParseError error{ UCDScriptsParseError::None };

        uint32_t lineNumber{ 0 };
        uint32_t rangeCount{ 0 };
        uint32_t scriptCount{ 0 };

        size_t explicitCodePoints{ 0 };
        size_t unknownCodePoints{ 0 };

        [[nodiscard]] bool success() const noexcept
        {
            return error == UCDScriptsParseError::None;
        }

        explicit operator bool() const noexcept
        {
            return success();
        }
    };


    // ========================================================================
    // Error text
    // ========================================================================

    static inline const char* ucdScriptsParseErrorString(UCDScriptsParseError error) noexcept
    {
        switch (error)
        {
        case UCDScriptsParseError::None:
            return "no error";

        case UCDScriptsParseError::ExistingScripts:
            return "database already contains Script records";

        case UCDScriptsParseError::MissingAliases:
            return "Script value alias table is empty";

        case UCDScriptsParseError::MissingUnknownAlias:
            return "Script alias table does not contain Unknown";

        case UCDScriptsParseError::InvalidAlias:
            return "invalid Script alias record";

        case UCDScriptsParseError::StorageFailed:
            return "failed to allocate Script parser storage";

        case UCDScriptsParseError::InvalidRange:
            return "invalid Script code-point range";

        case UCDScriptsParseError::MissingScriptName:
            return "missing Script property value";

        case UCDScriptsParseError::UnexpectedField:
            return "unexpected extra field in Scripts.txt record";

        case UCDScriptsParseError::NameInternFailed:
            return "failed to intern Script name";

        case UCDScriptsParseError::UnknownScriptName:
            return "Script name is not present in PropertyValueAliases.txt";

        case UCDScriptsParseError::OverlappingRange:
            return "Scripts.txt contains overlapping Script ranges";

        case UCDScriptsParseError::CoverageFinalizeFailed:
            return "failed to finalize Script coverage";

        case UCDScriptsParseError::CoverageAddFailed:
            return "failed to add Script coverage";

        case UCDScriptsParseError::ScriptAddFailed:
            return "failed to add Script database record";
        }

        return "unknown Scripts.txt parser error";
    }


    namespace ucd_detail
    {
        // ====================================================================
        // ScriptAccumulator
        //
        // One mutable coverage for every Script property value known through
        // PropertyValueAliases.txt.
        //
        // Seeding from the alias table means retained values with no explicit
        // Scripts.txt ranges, such as Hrkt / Katakana_Or_Hiragana, still
        // receive a database record with empty coverage.
        // ====================================================================

        struct ScriptAccumulator
        {
            InternedKey shortName{ nullptr };
            InternedKey longName{ nullptr };

            UnicodeCoverageBuilder coverage;
        };


        // ====================================================================
        // coverageIntersectsRange
        //
        // Generator-side validation helper.
        //
        // Scripts.txt is relatively small and each explicitly assigned code
        // point is tested at most once before being inserted.  This simple
        // scan keeps overlap validation independent of file ordering.
        // ====================================================================

        static inline bool coverageIntersectsRange(const UnicodeCoverageBuilder& coverage,
            uint32_t first, uint32_t last) noexcept
        {
            uint32_t cp = first;

            for (;;)
            {
                if (coverage.contains(cp))
                    return true;

                if (cp == last)
                    break;

                ++cp;
            }

            return false;
        }


        // ====================================================================
        // buildUnknownCoverage
        //
        // Script=Unknown is the default for every code point not explicitly
        // represented in Scripts.txt.
        //
        // Walk the complete Unicode address space and add maximal unassigned
        // ranges to the Unknown coverage.
        // ====================================================================

        static inline size_t buildUnknownCoverage(const UnicodeCoverageBuilder& assigned,
            UnicodeCoverageBuilder& unknown)
        {
            size_t unknownCount = 0;
            uint32_t cp = 0;

            while (cp < kUnicodeLimit)
            {
                if (assigned.contains(cp))
                {
                    ++cp;
                    continue;
                }

                const uint32_t first = cp;

                while (cp < kUnicodeLimit && !assigned.contains(cp))
                    ++cp;

                const uint32_t last = cp - 1u;

                unknown.addRange(first, last);
                unknownCount += static_cast<size_t>(last - first + 1u);
            }

            return unknownCount;
        }
    }


    // ========================================================================
    // ucdParseScripts
    //
    // Parse Scripts.txt and append one UnicodeScriptRecord for every Script
    // property value in UCDScriptValueAliases.
    //
    //
    // Parsing:
    //
    //      Scripts.txt
    //          |
    //          +--> Common ranges ------> Common coverage
    //          +--> Latin ranges -------> Latin coverage
    //          +--> Arabic ranges ------> Arabic coverage
    //          +--> ...
    //
    // Every explicit range is also accumulated into:
    //
    //      assignedCoverage
    //
    // After parsing:
    //
    //      Unknown = Unicode address space - assignedCoverage
    //
    // Finally all aliases are finalized, including aliases with empty
    // coverage.
    //
    // ========================================================================

    static inline bool ucdParseScripts(const ByteSpan& source,
        const UCDScriptValueAliases& aliases,
        UnicodeDatabaseBuilder& database,
        UCDScriptsParseResult& outResult)
    {
        outResult = {};

        // --------------------------------------------------------------------
        // Scripts should be generated once per database.
        // --------------------------------------------------------------------

        if (database.scriptCount() != 0)
        {
            outResult.error = UCDScriptsParseError::ExistingScripts;
            return false;
        }

        if (aliases.empty())
        {
            outResult.error = UCDScriptsParseError::MissingAliases;
            return false;
        }

        if (aliases.size() > std::numeric_limits<uint32_t>::max())
        {
            outResult.error = UCDScriptsParseError::StorageFailed;
            return false;
        }


        // ====================================================================
        // Seed one accumulator for every known Script property value.
        //
        // The vector order follows PropertyValueAliases.txt and becomes the
        // database Script record order.
        // ====================================================================

        std::vector<ucd_detail::ScriptAccumulator> scripts;
        scripts.reserve(aliases.size());

        WSOpenHashMap<InternedKey, uint32_t> scriptIndex;

        if (!scriptIndex.reserve(aliases.size()))
        {
            outResult.error = UCDScriptsParseError::StorageFailed;
            return false;
        }

        for (const UCDScriptValueAlias& alias : aliases.aliases())
        {
            if (!alias.shortName || !alias.longName)
            {
                outResult.error = UCDScriptsParseError::InvalidAlias;
                return false;
            }

            const uint32_t index = static_cast<uint32_t>(scripts.size());

            scripts.emplace_back();

            scripts.back().shortName = alias.shortName;
            scripts.back().longName = alias.longName;

            if (!scriptIndex.put(alias.longName, index))
            {
                outResult.error = UCDScriptsParseError::StorageFailed;
                return false;
            }
        }


        // ====================================================================
        // Locate Unknown.
        // ====================================================================

        InternedKey unknownName = WSNameSet::INTERN("Unknown");

        if (!unknownName)
        {
            outResult.error = UCDScriptsParseError::NameInternFailed;
            return false;
        }

        const uint32_t* unknownIndexPtr = scriptIndex.getRef(unknownName);

        if (!unknownIndexPtr)
        {
            outResult.error = UCDScriptsParseError::MissingUnknownAlias;
            return false;
        }

        const uint32_t unknownIndex = *unknownIndexPtr;


        // ====================================================================
        // Parse explicit Script assignments.
        // ====================================================================

        UnicodeCoverageBuilder assignedCoverage;

        UCDParser parser(source);
        UCDLine line;

        while (parser.next(line))
        {
            ByteSpan fields = line.data;


            // ----------------------------------------------------------------
            // Field 1: code-point range
            // ----------------------------------------------------------------

            UCDCodePointRange range;

            if (!ucdReadCodePointRange(fields, range))
            {
                outResult.error = UCDScriptsParseError::InvalidRange;
                outResult.lineNumber = line.lineNumber;
                return false;
            }


            // ----------------------------------------------------------------
            // Field 2: Script property value
            // ----------------------------------------------------------------

            ByteSpan scriptNameText;

            if (!ucdReadField(fields, scriptNameText) || !scriptNameText)
            {
                outResult.error = UCDScriptsParseError::MissingScriptName;
                outResult.lineNumber = line.lineNumber;
                return false;
            }


            // ----------------------------------------------------------------
            // Scripts.txt records contain exactly two fields.
            // ----------------------------------------------------------------

            bspan_trim_spaces(fields);

            if (fields)
            {
                outResult.error = UCDScriptsParseError::UnexpectedField;
                outResult.lineNumber = line.lineNumber;
                return false;
            }


            // ----------------------------------------------------------------
            // Canonical Script name.
            // ----------------------------------------------------------------

            InternedKey scriptName = WSNameSet::INTERN(scriptNameText);

            if (!scriptName)
            {
                outResult.error = UCDScriptsParseError::NameInternFailed;
                outResult.lineNumber = line.lineNumber;
                return false;
            }


            // ----------------------------------------------------------------
            // Scripts.txt should contain only values defined by the Script
            // aliases from PropertyValueAliases.txt.
            // ----------------------------------------------------------------

            const uint32_t* indexPtr = scriptIndex.getRef(scriptName);

            if (!indexPtr)
            {
                outResult.error = UCDScriptsParseError::UnknownScriptName;
                outResult.lineNumber = line.lineNumber;
                return false;
            }


            // ----------------------------------------------------------------
            // Script is a single-valued property. Explicit ranges therefore
            // must never overlap, even though Scripts.txt itself is grouped
            // by script rather than globally ordered by code point.
            // ----------------------------------------------------------------

            if (ucd_detail::coverageIntersectsRange(
                assignedCoverage, range.first, range.last))
            {
                outResult.error = UCDScriptsParseError::OverlappingRange;
                outResult.lineNumber = line.lineNumber;
                return false;
            }


            // ----------------------------------------------------------------
            // Add range to the individual script and to the global explicit
            // assignment coverage.
            // ----------------------------------------------------------------

            scripts[*indexPtr].coverage.addRange(range.first, range.last);
            assignedCoverage.addRange(range.first, range.last);

            ++outResult.rangeCount;

            outResult.explicitCodePoints +=
                static_cast<size_t>(range.last - range.first + 1u);
        }


        // ====================================================================
        // Derive Script=Unknown.
        //
        // UCDParser intentionally ignores comment-only @missing metadata.
        // For Scripts.txt the documented default is known and simple:
        //
        //      every code point not explicitly listed -> Unknown
        //
        // ====================================================================

        outResult.unknownCodePoints =
            ucd_detail::buildUnknownCoverage(
                assignedCoverage,
                scripts[unknownIndex].coverage);


        // ====================================================================
        // Sanity check: explicit + default must cover all Unicode code points.
        // ====================================================================

        if (outResult.explicitCodePoints + outResult.unknownCodePoints != kUnicodeLimit)
        {
            outResult.error = UCDScriptsParseError::OverlappingRange;
            return false;
        }


        // ====================================================================
        // Finalize every Script property value.
        //
        // This includes Script values with no explicit ranges.
        // ====================================================================

        for (ucd_detail::ScriptAccumulator& script : scripts)
        {
            UnicodeCoverageData coverageData{};

            if (!script.coverage.finalize(database.pagePool(), coverageData))
            {
                outResult.error = UCDScriptsParseError::CoverageFinalizeFailed;
                return false;
            }

            UnicodeCoverageIndex coverageIndex;

            if (!database.addCoverage(coverageData, coverageIndex))
            {
                outResult.error = UCDScriptsParseError::CoverageAddFailed;
                return false;
            }

            if (!database.addScript(script.longName, script.shortName, coverageIndex))
            {
                outResult.error = UCDScriptsParseError::ScriptAddFailed;
                return false;
            }

            ++outResult.scriptCount;
        }

        return true;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static inline bool ucdParseScripts(const ByteSpan& source,
        const UCDScriptValueAliases& aliases,
        UnicodeDatabaseBuilder& database)
    {
        UCDScriptsParseResult result;
        return ucdParseScripts(source, aliases, database, result);
    }

} // namespace waavs

