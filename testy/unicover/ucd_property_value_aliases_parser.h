// ucd_property_value_aliases_parser.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include "core_nametable.h"
#include "core_openhashmap.h"

#include "ucd_parser.h"


namespace waavs
{
    // ========================================================================
    // UCDScriptValueAlias
    //
    // Preferred aliases for one Script property value.
    //
    // Example:
    //
    //      sc ; Latn ; Latin
    //
    //      shortName = "Latn"
    //      longName  = "Latin"
    //
    // ========================================================================

    struct UCDScriptValueAlias
    {
        InternedKey shortName{ nullptr };
        InternedKey longName{ nullptr };
    };


    // ========================================================================
    // UCDScriptValueAliases
    //
    // Generator-side lookup table for Script property value aliases.
    //
    // Provides lookup in either direction:
    //
    //      Latin -> Latn
    //      Latn  -> Latin
    //
    // All names are canonical InternedKey values.
    //
    // ========================================================================

    class UCDScriptValueAliases
    {
    public:
        UCDScriptValueAliases() = default;

        UCDScriptValueAliases(const UCDScriptValueAliases&) = delete;
        UCDScriptValueAliases& operator=(const UCDScriptValueAliases&) = delete;

        UCDScriptValueAliases(UCDScriptValueAliases&&) noexcept = default;
        UCDScriptValueAliases& operator=(UCDScriptValueAliases&&) noexcept = default;


        void clear() noexcept
        {
            mAliases.clear();
            mByShortName.clear();
            mByLongName.clear();
        }


        bool reserve(size_t count)
        {
            if (count > std::numeric_limits<uint32_t>::max())
                return false;

            if (!mByShortName.reserve(count))
                return false;

            if (!mByLongName.reserve(count))
                return false;

            mAliases.reserve(count);
            return true;
        }


        [[nodiscard]]
        size_t size() const noexcept
        {
            return mAliases.size();
        }


        [[nodiscard]]
        bool empty() const noexcept
        {
            return mAliases.empty();
        }


        [[nodiscard]]
        const std::vector<UCDScriptValueAlias>& aliases() const noexcept
        {
            return mAliases;
        }


        [[nodiscard]]
        const UCDScriptValueAlias* alias(uint32_t index) const noexcept
        {
            if (index >= mAliases.size())
                return nullptr;

            return &mAliases[index];
        }


        [[nodiscard]]
        const UCDScriptValueAlias* findByShortName(InternedKey shortName) const noexcept
        {
            if (!shortName)
                return nullptr;

            const uint32_t* index = mByShortName.getRef(shortName);

            if (!index || *index >= mAliases.size())
                return nullptr;

            return &mAliases[*index];
        }


        [[nodiscard]]
        const UCDScriptValueAlias* findByLongName(InternedKey longName) const noexcept
        {
            if (!longName)
                return nullptr;

            const uint32_t* index = mByLongName.getRef(longName);

            if (!index || *index >= mAliases.size())
                return nullptr;

            return &mAliases[*index];
        }


        [[nodiscard]]
        InternedKey shortNameForLongName(InternedKey longName) const noexcept
        {
            const UCDScriptValueAlias* value = findByLongName(longName);
            return value ? value->shortName : nullptr;
        }


        [[nodiscard]]
        InternedKey longNameForShortName(InternedKey shortName) const noexcept
        {
            const UCDScriptValueAlias* value = findByShortName(shortName);
            return value ? value->longName : nullptr;
        }


        bool add(InternedKey shortName, InternedKey longName)
        {
            if (!shortName || !*shortName || !longName || !*longName)
                return false;

            if (mByShortName.contains(shortName))
                return false;

            if (mByLongName.contains(longName))
                return false;

            if (mAliases.size() >= std::numeric_limits<uint32_t>::max())
                return false;

            const uint32_t index = static_cast<uint32_t>(mAliases.size());

            UCDScriptValueAlias value{};
            value.shortName = shortName;
            value.longName = longName;

            mAliases.push_back(value);

            if (!mByShortName.put(shortName, index))
            {
                mAliases.pop_back();
                return false;
            }

            if (!mByLongName.put(longName, index))
            {
                mByShortName.remove(shortName);
                mAliases.pop_back();
                return false;
            }

            return true;
        }


    private:
        std::vector<UCDScriptValueAlias> mAliases;

        WSOpenHashMap<InternedKey, uint32_t> mByShortName;
        WSOpenHashMap<InternedKey, uint32_t> mByLongName;
    };


    // ========================================================================
    // UCDPropertyValueAliasesParseError
    // ========================================================================

    enum class UCDPropertyValueAliasesParseError : uint8_t
    {
        None = 0,

        MissingProperty,
        MissingShortName,
        MissingLongName,
        NameInternFailed,
        DuplicateScriptAlias,
        StorageFailed
    };


    // ========================================================================
    // UCDPropertyValueAliasesParseResult
    // ========================================================================

    struct UCDPropertyValueAliasesParseResult
    {
        UCDPropertyValueAliasesParseError error{ UCDPropertyValueAliasesParseError::None };

        uint32_t lineNumber{ 0 };
        uint32_t scriptAliasCount{ 0 };


        [[nodiscard]]
        bool success() const noexcept
        {
            return error == UCDPropertyValueAliasesParseError::None;
        }


        explicit operator bool() const noexcept
        {
            return success();
        }
    };


    // ========================================================================
    // Error text
    // ========================================================================

    static inline const char* ucdPropertyValueAliasesParseErrorString(
        UCDPropertyValueAliasesParseError error) noexcept
    {
        switch (error)
        {
        case UCDPropertyValueAliasesParseError::None:
            return "no error";

        case UCDPropertyValueAliasesParseError::MissingProperty:
            return "missing property field";

        case UCDPropertyValueAliasesParseError::MissingShortName:
            return "missing Script short-name field";

        case UCDPropertyValueAliasesParseError::MissingLongName:
            return "missing Script long-name field";

        case UCDPropertyValueAliasesParseError::NameInternFailed:
            return "failed to intern Script property value alias";

        case UCDPropertyValueAliasesParseError::DuplicateScriptAlias:
            return "duplicate Script property value alias";

        case UCDPropertyValueAliasesParseError::StorageFailed:
            return "failed to store Script property value alias";
        }

        return "unknown PropertyValueAliases parser error";
    }


    // ========================================================================
    // ucdFieldEquals
    //
    // Exact ASCII comparison for known UCD field identifiers.
    //
    // The official input uses "sc" for Script.  Loose matching is not needed
    // when consuming the canonical UCD source file itself.
    //
    // ========================================================================

    static inline bool ucdFieldEquals(const ByteSpan& field, const char* text) noexcept
    {
        if (!text)
            return false;

        const size_t length = std::strlen(text);

        if (field.size() != length)
            return false;

        return std::memcmp(field.data(), text, length) == 0;
    }


    // ========================================================================
    // ucdParseScriptValueAliases
    //
    // Extract preferred Script aliases from PropertyValueAliases.txt.
    //
    // Relevant input:
    //
    //      sc ; Arab ; Arabic
    //      sc ; Latn ; Latin
    //      sc ; Zinh ; Inherited ; Qaai
    //
    // Only records whose first field is exactly "sc" are consumed.
    //
    // For Script records:
    //
    //      field 1 = sc
    //      field 2 = preferred short alias
    //      field 3 = preferred long alias
    //
    // Additional fields are legal aliases and are deliberately ignored here.
    //
    // ========================================================================

    static inline bool ucdParseScriptValueAliases(const ByteSpan& source,
        UCDScriptValueAliases& aliases,
        UCDPropertyValueAliasesParseResult& outResult)
    {
        outResult = {};

        UCDParser parser(source);
        UCDLine line;


        while (parser.next(line))
        {
            ByteSpan fields = line.data;

            // ---------------------------------------------------------------
            // Property
            // ---------------------------------------------------------------

            ByteSpan property;

            if (!ucdReadField(fields, property))
            {
                outResult.error = UCDPropertyValueAliasesParseError::MissingProperty;
                outResult.lineNumber = line.lineNumber;
                return false;
            }


            // ---------------------------------------------------------------
            // We currently care only about Script property values.
            // ---------------------------------------------------------------

            if (!ucdFieldEquals(property, "sc"))
                continue;


            // ---------------------------------------------------------------
            // Preferred short alias
            // ---------------------------------------------------------------

            ByteSpan shortNameText;

            if (!ucdReadField(fields, shortNameText) || !shortNameText)
            {
                outResult.error = UCDPropertyValueAliasesParseError::MissingShortName;
                outResult.lineNumber = line.lineNumber;
                return false;
            }


            // ---------------------------------------------------------------
            // Preferred long alias
            // ---------------------------------------------------------------

            ByteSpan longNameText;

            if (!ucdReadField(fields, longNameText) || !longNameText)
            {
                outResult.error = UCDPropertyValueAliasesParseError::MissingLongName;
                outResult.lineNumber = line.lineNumber;
                return false;
            }


            // ---------------------------------------------------------------
            // Additional fields are legal aliases.
            //
            // Examples include:
            //
            //      Copt ; Coptic    ; Qaac
            //      Zinh ; Inherited ; Qaai
            //
            // We need only the preferred short/long pair for Scripts.txt.
            // ---------------------------------------------------------------


            // ---------------------------------------------------------------
            // Canonicalize names.
            // ---------------------------------------------------------------

            InternedKey shortName = WSNameSet::INTERN(shortNameText);
            InternedKey longName = WSNameSet::INTERN(longNameText);

            if (!shortName || !longName)
            {
                outResult.error = UCDPropertyValueAliasesParseError::NameInternFailed;
                outResult.lineNumber = line.lineNumber;
                return false;
            }


            // ---------------------------------------------------------------
            // Duplicates are considered malformed input.
            // ---------------------------------------------------------------

            if (aliases.findByShortName(shortName) || aliases.findByLongName(longName))
            {
                outResult.error = UCDPropertyValueAliasesParseError::DuplicateScriptAlias;
                outResult.lineNumber = line.lineNumber;
                return false;
            }


            if (!aliases.add(shortName, longName))
            {
                outResult.error = UCDPropertyValueAliasesParseError::StorageFailed;
                outResult.lineNumber = line.lineNumber;
                return false;
            }


            ++outResult.scriptAliasCount;
        }


        return true;
    }


    // ========================================================================
    // Convenience overload
    // ========================================================================

    static inline bool ucdParseScriptValueAliases(const ByteSpan& source,
        UCDScriptValueAliases& aliases)
    {
        UCDPropertyValueAliasesParseResult result;
        return ucdParseScriptValueAliases(source, aliases, result);
    }

} // namespace waavs
