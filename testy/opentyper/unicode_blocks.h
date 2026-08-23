#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "core_nametable.h"
#include "unicode_coverage.h"

namespace waavs
{
    // ========================================================================
    // UnicodeBlock
    //
    // Identifies a named Unicode block.
    //
    // This enum will eventually be generated from the Unicode Blocks.txt
    // database.  For now, only a representative subset is included.
    //
    // ========================================================================

    enum class UnicodeBlock : uint16_t
    {
        Unknown = 0,

        BasicLatin,
        Latin1Supplement,
        LatinExtendedA,
        LatinExtendedB,

        GreekAndCoptic,
        Cyrillic,
        CyrillicSupplement,

        Hebrew,
        Arabic,
        ArabicSupplement,

        Devanagari,
        Bengali,
        Gurmukhi,
        Gujarati,
        Oriya,
        Tamil,
        Telugu,
        Kannada,
        Malayalam,

        Thai,

        Hiragana,
        Katakana,
        HangulSyllables,

        MiscellaneousSymbols,
        Dingbats,

        MiscellaneousSymbolsAndPictographs,
        Emoticons,
        TransportAndMapSymbols,
        SupplementalSymbolsAndPictographs,
        SymbolsAndPictographsExtendedA,

        Count
    };


    // ========================================================================
    // UnicodeBlockInfo
    //
    // Static description of one Unicode block.
    //
    // first and last are inclusive.
    //
    // ========================================================================

    struct UnicodeBlockInfo
    {
        UnicodeBlock block;
        uint32_t first;
        uint32_t last;
        InternedKey name;

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
    // Initial block database
    //
    // This should eventually be generated directly from Blocks.txt.
    //
    // Keep this sorted by code-point range.  Besides making the generated
    // output predictable, this will allow blockOf() to use binary search.
    //
    // ========================================================================

    inline constexpr UnicodeBlockInfo kUnicodeBlocks[] =
    {
        {
            UnicodeBlock::BasicLatin,
            0x0000,
            0x007F,
            "Basic Latin"
        },
        {
            UnicodeBlock::Latin1Supplement,
            0x0080,
            0x00FF,
            "Latin-1 Supplement"
        },
        {
            UnicodeBlock::LatinExtendedA,
            0x0100,
            0x017F,
            "Latin Extended-A"
        },
        {
            UnicodeBlock::LatinExtendedB,
            0x0180,
            0x024F,
            "Latin Extended-B"
        },

        {
            UnicodeBlock::GreekAndCoptic,
            0x0370,
            0x03FF,
            "Greek and Coptic"
        },
        {
            UnicodeBlock::Cyrillic,
            0x0400,
            0x04FF,
            "Cyrillic"
        },
        {
            UnicodeBlock::CyrillicSupplement,
            0x0500,
            0x052F,
            "Cyrillic Supplement"
        },

        {
            UnicodeBlock::Hebrew,
            0x0590,
            0x05FF,
            "Hebrew"
        },
        {
            UnicodeBlock::Arabic,
            0x0600,
            0x06FF,
            "Arabic"
        },

        {
            UnicodeBlock::Devanagari,
            0x0900,
            0x097F,
            "Devanagari"
        },
        {
            UnicodeBlock::Bengali,
            0x0980,
            0x09FF,
            "Bengali"
        },
        {
            UnicodeBlock::Gurmukhi,
            0x0A00,
            0x0A7F,
            "Gurmukhi"
        },
        {
            UnicodeBlock::Gujarati,
            0x0A80,
            0x0AFF,
            "Gujarati"
        },
        {
            UnicodeBlock::Oriya,
            0x0B00,
            0x0B7F,
            "Oriya"
        },
        {
            UnicodeBlock::Tamil,
            0x0B80,
            0x0BFF,
            "Tamil"
        },
        {
            UnicodeBlock::Telugu,
            0x0C00,
            0x0C7F,
            "Telugu"
        },
        {
            UnicodeBlock::Kannada,
            0x0C80,
            0x0CFF,
            "Kannada"
        },
        {
            UnicodeBlock::Malayalam,
            0x0D00,
            0x0D7F,
            "Malayalam"
        },

        {
            UnicodeBlock::Thai,
            0x0E00,
            0x0E7F,
            "Thai"
        },

        {
            UnicodeBlock::Hiragana,
            0x3040,
            0x309F,
            "Hiragana"
        },
        {
            UnicodeBlock::Katakana,
            0x30A0,
            0x30FF,
            "Katakana"
        },

        {
            UnicodeBlock::HangulSyllables,
            0xAC00,
            0xD7AF,
            "Hangul Syllables"
        },

        {
            UnicodeBlock::MiscellaneousSymbols,
            0x2600,
            0x26FF,
            "Miscellaneous Symbols"
        },
        {
            UnicodeBlock::Dingbats,
            0x2700,
            0x27BF,
            "Dingbats"
        },

        {
            UnicodeBlock::ArabicSupplement,
            0x0750,
            0x077F,
            "Arabic Supplement"
        },

        {
            UnicodeBlock::MiscellaneousSymbolsAndPictographs,
            0x1F300,
            0x1F5FF,
            "Miscellaneous Symbols and Pictographs"
        },
        {
            UnicodeBlock::Emoticons,
            0x1F600,
            0x1F64F,
            "Emoticons"
        },
        {
            UnicodeBlock::TransportAndMapSymbols,
            0x1F680,
            0x1F6FF,
            "Transport and Map Symbols"
        },
        {
            UnicodeBlock::SupplementalSymbolsAndPictographs,
            0x1F900,
            0x1F9FF,
            "Supplemental Symbols and Pictographs"
        },
        {
            UnicodeBlock::SymbolsAndPictographsExtendedA,
            0x1FA70,
            0x1FAFF,
            "Symbols and Pictographs Extended-A"
        },
    };


    inline constexpr size_t kUnicodeBlockCount =
        sizeof(kUnicodeBlocks) /
        sizeof(kUnicodeBlocks[0]);


    // ========================================================================
    // blockInfo
    //
    // Lookup by symbolic block identifier.
    //
    // For this initial hand-written implementation a linear lookup is fine.
    // When this table is generated we can choose either:
    //
    //   - direct enum-indexed lookup
    //   - generated switch
    //   - separate tables sorted by id and range
    //
    // ========================================================================

    [[nodiscard]]
    inline constexpr const UnicodeBlockInfo* blockInfo(UnicodeBlock block) noexcept
    {
        for (const auto& info : kUnicodeBlocks) {
            if (info.block == block)
                return &info;
        }

        return nullptr;
    }


    // ========================================================================
    // blockOf
    //
    // Return the Unicode block containing a code point.
    //
    // Unicode has gaps between blocks.  A valid Unicode code point is not
    // necessarily contained in a named block.
    //
    // ========================================================================

    [[nodiscard]]
    inline constexpr UnicodeBlock
        blockOf(uint32_t cp) noexcept
    {
        if (cp >= UnicodeCoverage::kUnicodeLimit)
            return UnicodeBlock::Unknown;

        for (const auto& info : kUnicodeBlocks) {
            if (cp < info.first)
                break;

            if (cp <= info.last)
                return info.block;
        }

        return UnicodeBlock::Unknown;
    }


    // ========================================================================
    // coverage
    //
    // Produce a UnicodeCoverage corresponding to the complete block.
    //
    // ========================================================================

    [[nodiscard]]
    inline UnicodeCoverage
        coverage(UnicodeBlock block)
    {
        const UnicodeBlockInfo* info =
            blockInfo(block);

        if (!info)
            return {};

        UnicodeCoverageBuilder builder;

        builder.addRange(
            info->first,
            info->last);

        return builder.finalize();
    }


    // ========================================================================
    // Convenience accessors
    // ========================================================================

    [[nodiscard]]
    inline InternedKey blockName(UnicodeBlock block) noexcept
    {
        const UnicodeBlockInfo* info =  blockInfo(block);

        return info ? info->name : InternedKey{};
    }


    [[nodiscard]]
    inline constexpr uint32_t
        blockFirst(UnicodeBlock block) noexcept
    {
        const UnicodeBlockInfo* info =
            blockInfo(block);

        return info ? info->first : 0;
    }


    [[nodiscard]]
    inline constexpr uint32_t
        blockLast(UnicodeBlock block) noexcept
    {
        const UnicodeBlockInfo* info =
            blockInfo(block);

        return info ? info->last : 0;
    }

} // namespace waavs