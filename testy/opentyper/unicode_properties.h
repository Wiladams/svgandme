#pragma once

#include <cstddef>
#include <cstdint>

#include "core_nametable.h"
#include "unicode_coverage.h"

namespace waavs
{
    // ========================================================================
    // UnicodeProperty
    //
    // Identifies a binary Unicode character property useful to typography,
    // text processing, and font analysis.
    //
    // Binary properties answer:
    //
    //     Does code point X possess property P?
    //
    // They map naturally onto UnicodeCoverage.
    //
    // This is deliberately NOT intended to contain enumerated properties
    // such as:
    //
    //     General_Category
    //     Bidi_Class
    //     East_Asian_Width
    //     Joining_Type
    //
    // Those have values rather than simple true/false membership and should
    // be represented separately.
    //
    // ========================================================================

    enum class UnicodeProperty : uint16_t
    {
        // --------------------------------------------------------------------
        // DerivedCoreProperties.txt
        // --------------------------------------------------------------------

        Alphabetic,
        Lowercase,
        Uppercase,
        Cased,
        CaseIgnorable,

        Math,

        DefaultIgnorableCodePoint,

        GraphemeBase,
        GraphemeExtend,

        // --------------------------------------------------------------------
        // PropList.txt
        // --------------------------------------------------------------------

        WhiteSpace,

        Dash,
        Hyphen,
        QuotationMark,
        TerminalPunctuation,

        Diacritic,
        Extender,
        SoftDotted,

        Ideographic,
        UnifiedIdeograph,

        JoinControl,
        VariationSelector,
        RegionalIndicator,

        Deprecated,
        NoncharacterCodePoint,

        // --------------------------------------------------------------------
        // emoji/emoji-data.txt
        // --------------------------------------------------------------------

        Emoji,
        EmojiPresentation,
        EmojiModifier,
        EmojiModifierBase,
        EmojiComponent,
        ExtendedPictographic,

        Count
    };


    // ========================================================================
    // UnicodePropertySource
    //
    // Records the authoritative UCD source from which the property data
    // should eventually be generated.
    //
    // This is metadata only; applications normally do not need to care where
    // a property originated.
    //
    // ========================================================================

    enum class UnicodePropertySource : uint8_t
    {
        DerivedCoreProperties,
        PropList,
        EmojiData
    };


    // ========================================================================
    // UnicodePropertyInfo
    //
    // Metadata associated with a Unicode binary property.
    //
    // name is the official Unicode property name and is interned using the
    // project's normal name-table machinery.
    //
    // ========================================================================

    struct UnicodePropertyInfo
    {
        UnicodeProperty property;
        InternedKey name;
        UnicodePropertySource source;
    };


    // ========================================================================
    // UnicodePropertyRange
    //
    // One contiguous range possessing a binary property.
    //
    // Multiple properties may overlap the same code point, so unlike the
    // Script property, these ranges do NOT partition Unicode.
    //
    // Example:
    //
    //     'A'
    //
    // may simultaneously be:
    //
    //     Alphabetic
    //     Uppercase
    //     Cased
    //     GraphemeBase
    //
    // ========================================================================

    struct UnicodePropertyRange
    {
        uint32_t first;
        uint32_t last;
        UnicodeProperty property;

        [[nodiscard]]
        constexpr bool contains(uint32_t cp) const noexcept
        {
            return cp >= first && cp <= last;
        }

        [[nodiscard]]
        constexpr uint32_t size() const noexcept
        {
            return last - first + 1u;
        }
    };


    // ========================================================================
    // Property metadata
    //
    // Eventually generated from the Unicode property definitions.
    //
    // ========================================================================

    inline const UnicodePropertyInfo kUnicodeProperties[] =
    {
        // --------------------------------------------------------------------
        // DerivedCoreProperties.txt
        // --------------------------------------------------------------------

        {
            UnicodeProperty::Alphabetic,
            PSNameTable::INTERN("Alphabetic"),
            UnicodePropertySource::DerivedCoreProperties
        },
        {
            UnicodeProperty::Lowercase,
            PSNameTable::INTERN("Lowercase"),
            UnicodePropertySource::DerivedCoreProperties
        },
        {
            UnicodeProperty::Uppercase,
            PSNameTable::INTERN("Uppercase"),
            UnicodePropertySource::DerivedCoreProperties
        },
        {
            UnicodeProperty::Cased,
            PSNameTable::INTERN("Cased"),
            UnicodePropertySource::DerivedCoreProperties
        },
        {
            UnicodeProperty::CaseIgnorable,
            PSNameTable::INTERN("Case_Ignorable"),
            UnicodePropertySource::DerivedCoreProperties
        },
        {
            UnicodeProperty::Math,
            PSNameTable::INTERN("Math"),
            UnicodePropertySource::DerivedCoreProperties
        },
        {
            UnicodeProperty::DefaultIgnorableCodePoint,
            PSNameTable::INTERN("Default_Ignorable_Code_Point"),
            UnicodePropertySource::DerivedCoreProperties
        },
        {
            UnicodeProperty::GraphemeBase,
            PSNameTable::INTERN("Grapheme_Base"),
            UnicodePropertySource::DerivedCoreProperties
        },
        {
            UnicodeProperty::GraphemeExtend,
            PSNameTable::INTERN("Grapheme_Extend"),
            UnicodePropertySource::DerivedCoreProperties
        },

        // --------------------------------------------------------------------
        // PropList.txt
        // --------------------------------------------------------------------

        {
            UnicodeProperty::WhiteSpace,
            PSNameTable::INTERN("White_Space"),
            UnicodePropertySource::PropList
        },
        {
            UnicodeProperty::Dash,
            PSNameTable::INTERN("Dash"),
            UnicodePropertySource::PropList
        },
        {
            UnicodeProperty::Hyphen,
            PSNameTable::INTERN("Hyphen"),
            UnicodePropertySource::PropList
        },
        {
            UnicodeProperty::QuotationMark,
            PSNameTable::INTERN("Quotation_Mark"),
            UnicodePropertySource::PropList
        },
        {
            UnicodeProperty::TerminalPunctuation,
            PSNameTable::INTERN("Terminal_Punctuation"),
            UnicodePropertySource::PropList
        },
        {
            UnicodeProperty::Diacritic,
            PSNameTable::INTERN("Diacritic"),
            UnicodePropertySource::PropList
        },
        {
            UnicodeProperty::Extender,
            PSNameTable::INTERN("Extender"),
            UnicodePropertySource::PropList
        },
        {
            UnicodeProperty::SoftDotted,
            PSNameTable::INTERN("Soft_Dotted"),
            UnicodePropertySource::PropList
        },
        {
            UnicodeProperty::Ideographic,
            PSNameTable::INTERN("Ideographic"),
            UnicodePropertySource::PropList
        },
        {
            UnicodeProperty::UnifiedIdeograph,
            PSNameTable::INTERN("Unified_Ideograph"),
            UnicodePropertySource::PropList
        },
        {
            UnicodeProperty::JoinControl,
            PSNameTable::INTERN("Join_Control"),
            UnicodePropertySource::PropList
        },
        {
            UnicodeProperty::VariationSelector,
            PSNameTable::INTERN("Variation_Selector"),
            UnicodePropertySource::PropList
        },
        {
            UnicodeProperty::RegionalIndicator,
            PSNameTable::INTERN("Regional_Indicator"),
            UnicodePropertySource::PropList
        },
        {
            UnicodeProperty::Deprecated,
            PSNameTable::INTERN("Deprecated"),
            UnicodePropertySource::PropList
        },
        {
            UnicodeProperty::NoncharacterCodePoint,
            PSNameTable::INTERN("Noncharacter_Code_Point"),
            UnicodePropertySource::PropList
        },

        // --------------------------------------------------------------------
        // emoji/emoji-data.txt
        // --------------------------------------------------------------------

        {
            UnicodeProperty::Emoji,
            PSNameTable::INTERN("Emoji"),
            UnicodePropertySource::EmojiData
        },
        {
            UnicodeProperty::EmojiPresentation,
            PSNameTable::INTERN("Emoji_Presentation"),
            UnicodePropertySource::EmojiData
        },
        {
            UnicodeProperty::EmojiModifier,
            PSNameTable::INTERN("Emoji_Modifier"),
            UnicodePropertySource::EmojiData
        },
        {
            UnicodeProperty::EmojiModifierBase,
            PSNameTable::INTERN("Emoji_Modifier_Base"),
            UnicodePropertySource::EmojiData
        },
        {
            UnicodeProperty::EmojiComponent,
            PSNameTable::INTERN("Emoji_Component"),
            UnicodePropertySource::EmojiData
        },
        {
            UnicodeProperty::ExtendedPictographic,
            PSNameTable::INTERN("Extended_Pictographic"),
            UnicodePropertySource::EmojiData
        }
    };


    inline constexpr size_t kUnicodePropertyCount =
        sizeof(kUnicodeProperties) /
        sizeof(kUnicodeProperties[0]);


    static_assert(
        kUnicodePropertyCount ==
        static_cast<size_t>(UnicodeProperty::Count));


    // ========================================================================
    // Initial property ranges
    //
    // This is ONLY representative data for exercising the API.
    //
    // The real table should be generated from:
    //
    //     DerivedCoreProperties.txt
    //     PropList.txt
    //     emoji/emoji-data.txt
    //
    // Binary properties overlap freely.
    //
    // For that reason this table may contain several entries with the same
    // or overlapping ranges.
    //
    // Keep the table globally sorted by first code point.
    //
    // ========================================================================

    inline constexpr UnicodePropertyRange kUnicodePropertyRanges[] =
    {
        // NULL..control whitespace examples

        {
            0x0009,
            0x000D,
            UnicodeProperty::WhiteSpace
        },

        // SPACE

        {
            0x0020,
            0x0020,
            UnicodeProperty::WhiteSpace
        },
        {
            0x0020,
            0x0020,
            UnicodeProperty::GraphemeBase
        },

        // ASCII quotation mark

        {
            0x0022,
            0x0022,
            UnicodeProperty::QuotationMark
        },

        // ASCII apostrophe

        {
            0x0027,
            0x0027,
            UnicodeProperty::QuotationMark
        },

        // Hyphen-minus

        {
            0x002D,
            0x002D,
            UnicodeProperty::Dash
        },
        {
            0x002D,
            0x002D,
            UnicodeProperty::Hyphen
        },

        // ASCII uppercase

        {
            0x0041,
            0x005A,
            UnicodeProperty::Alphabetic
        },
        {
            0x0041,
            0x005A,
            UnicodeProperty::Uppercase
        },
        {
            0x0041,
            0x005A,
            UnicodeProperty::Cased
        },
        {
            0x0041,
            0x005A,
            UnicodeProperty::GraphemeBase
        },

        // ASCII lowercase

        {
            0x0061,
            0x007A,
            UnicodeProperty::Alphabetic
        },
        {
            0x0061,
            0x007A,
            UnicodeProperty::Lowercase
        },
        {
            0x0061,
            0x007A,
            UnicodeProperty::Cased
        },
        {
            0x0061,
            0x007A,
            UnicodeProperty::GraphemeBase
        },

        // Combining Diacritical Marks -- representative sample

        {
            0x0300,
            0x036F,
            UnicodeProperty::GraphemeExtend
        },
        {
            0x0300,
            0x036F,
            UnicodeProperty::Diacritic
        },

        // Zero-width non-joiner / joiner

        {
            0x200C,
            0x200D,
            UnicodeProperty::JoinControl
        },
        {
            0x200C,
            0x200D,
            UnicodeProperty::DefaultIgnorableCodePoint
        },

        // Variation Selectors

        {
            0xFE00,
            0xFE0F,
            UnicodeProperty::VariationSelector
        },
        {
            0xFE00,
            0xFE0F,
            UnicodeProperty::DefaultIgnorableCodePoint
        },

        // Regional indicator symbols

        {
            0x1F1E6,
            0x1F1FF,
            UnicodeProperty::RegionalIndicator
        },
        {
            0x1F1E6,
            0x1F1FF,
            UnicodeProperty::Emoji
        },
        {
            0x1F1E6,
            0x1F1FF,
            UnicodeProperty::EmojiPresentation
        },

        // Representative pictographs / emoji

        {
            0x1F300,
            0x1F5FF,
            UnicodeProperty::ExtendedPictographic
        },

        {
            0x1F600,
            0x1F64F,
            UnicodeProperty::Emoji
        },
        {
            0x1F600,
            0x1F64F,
            UnicodeProperty::EmojiPresentation
        },
        {
            0x1F600,
            0x1F64F,
            UnicodeProperty::ExtendedPictographic
        },

        // Emoji skin-tone modifiers

        {
            0x1F3FB,
            0x1F3FF,
            UnicodeProperty::Emoji
        },
        {
            0x1F3FB,
            0x1F3FF,
            UnicodeProperty::EmojiModifier
        },
        {
            0x1F3FB,
            0x1F3FF,
            UnicodeProperty::EmojiComponent
        },

        // Supplementary variation selectors

        {
            0xE0100,
            0xE01EF,
            UnicodeProperty::VariationSelector
        },
        {
            0xE0100,
            0xE01EF,
            UnicodeProperty::DefaultIgnorableCodePoint
        }
    };


    inline constexpr size_t kUnicodePropertyRangeCount =
        sizeof(kUnicodePropertyRanges) /
        sizeof(kUnicodePropertyRanges[0]);


    // ========================================================================
    // propertyInfo
    //
    // Metadata lookup.
    //
    // Because the enum and metadata table are intentionally kept in the same
    // order, this can use direct indexing.
    //
    // ========================================================================

    [[nodiscard]]
    inline const UnicodePropertyInfo*
        propertyInfo(UnicodeProperty property) noexcept
    {
        const size_t index =
            static_cast<size_t>(property);

        if (index >= kUnicodePropertyCount)
            return nullptr;

        const UnicodePropertyInfo& info =
            kUnicodeProperties[index];

        // Defensive check while this data is still hand maintained.
        if (info.property != property)
            return nullptr;

        return &info;
    }


    // ========================================================================
    // propertyName
    // ========================================================================

    [[nodiscard]]
    inline InternedKey
        propertyName(UnicodeProperty property) noexcept
    {
        const UnicodePropertyInfo* info =
            propertyInfo(property);

        return info
            ? info->name
            : nullptr;
    }


    // ========================================================================
    // propertySource
    // ========================================================================

    [[nodiscard]]
    inline UnicodePropertySource
        propertySource(UnicodeProperty property) noexcept
    {
        const UnicodePropertyInfo* info =
            propertyInfo(property);

        return info
            ? info->source
            : UnicodePropertySource::DerivedCoreProperties;
    }


    // ========================================================================
    // hasProperty
    //
    // Test whether a code point possesses a binary Unicode property.
    //
    // This first implementation performs a linear scan.
    //
    // Once generated data is present, there are several opportunities for a
    // more compact/faster representation.  The API need not change.
    //
    // ========================================================================

    [[nodiscard]]
    inline constexpr bool
        hasProperty(
            uint32_t cp,
            UnicodeProperty property) noexcept
    {
        if (cp >= UnicodeCoverage::kUnicodeLimit)
            return false;

        for (const auto& range : kUnicodePropertyRanges) {
            if (cp < range.first)
                break;

            if (range.property == property &&
                cp <= range.last)
                return true;
        }

        return false;
    }


    // ========================================================================
    // coverage
    //
    // Produce a UnicodeCoverage containing every code point with the
    // requested property.
    //
    // ========================================================================

    [[nodiscard]]
    inline UnicodeCoverage coverage(UnicodeProperty property)
    {
        UnicodeCoverageBuilder builder;

        for (const auto& range : kUnicodePropertyRanges) {
            if (range.property != property)
                continue;

            builder.addRange(
                range.first,
                range.last);
        }

        return builder.finalize();
    }

} // namespace waavs

