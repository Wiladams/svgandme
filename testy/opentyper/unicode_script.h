#pragma once

#include <cstddef>
#include <cstdint>

#include "core_nametable.h"
#include "unicode_coverage.h"

namespace waavs
{
    // ========================================================================
    // UnicodeScript
    //
    // Identifies a Unicode Script property value.
    //
    // This enum will eventually be generated from the Unicode Character
    // Database PropertyValueAliases.txt / Scripts.txt data.
    //
    // Common, Inherited, and Unknown are genuine Unicode Script property
    // values and are therefore represented explicitly.
    //
    // ========================================================================

    enum class UnicodeScript : uint16_t
    {
        Unknown = 0,
        Common,
        Inherited,

        Adlam,
        Ahom,
        AnatolianHieroglyphs,
        Arabic,
        Armenian,
        Avestan,

        Balinese,
        Bamum,
        BassaVah,
        Batak,
        Bengali,
        BeriaErfe,
        Bhaiksuki,
        Bopomofo,
        Brahmi,
        Braille,
        Buginese,
        Buhid,

        CanadianAboriginal,
        Carian,
        CaucasianAlbanian,
        Chakma,
        Cham,
        Cherokee,
        Chorasmian,
        Cuneiform,
        Cypriot,
        CyproMinoan,
        Cyrillic,

        Deseret,
        Devanagari,
        DivesAkuru,
        Dogra,
        Duployan,

        EgyptianHieroglyphs,
        Elbasan,
        Elymaic,
        Ethiopic,

        Garay,
        Georgian,
        Glagolitic,
        Gothic,
        Grantha,
        Greek,
        Gujarati,
        GunjalaGondi,
        Gurmukhi,
        GurungKhema,

        Han,
        Hangul,
        HanifiRohingya,
        Hanunoo,
        Hatran,
        Hebrew,
        Hiragana,

        ImperialAramaic,
        InscriptionalPahlavi,
        InscriptionalParthian,

        Javanese,

        Kaithi,
        Kannada,
        Katakana,
        Kawi,
        KayahLi,
        Kharoshthi,
        KhitanSmallScript,
        Khmer,
        Khojki,
        Khudawadi,
        KiratRai,

        Lao,
        Latin,
        Lepcha,
        Limbu,
        LinearA,
        LinearB,
        Lisu,
        Lycian,
        Lydian,

        Mahajani,
        Makasar,
        Malayalam,
        Mandaic,
        Manichaean,
        Marchen,
        MasaramGondi,
        Medefaidrin,
        MeeteiMayek,
        MendeKikakui,
        MeroiticCursive,
        MeroiticHieroglyphs,
        Miao,
        Modi,
        Mongolian,
        Mro,
        Multani,
        Myanmar,

        Nabataean,
        NagMundari,
        Nandinagari,
        NewTaiLue,
        Newa,
        Nko,
        Nushu,
        NyiakengPuachueHmong,

        Ogham,
        OlChiki,
        OlOnal,
        OldHungarian,
        OldItalic,
        OldNorthArabian,
        OldPermic,
        OldPersian,
        OldSogdian,
        OldSouthArabian,
        OldTurkic,
        OldUyghur,
        Oriya,
        Osage,
        Osmanya,

        PahawhHmong,
        Palmyrene,
        PauCinHau,
        PhagsPa,
        Phoenician,
        PsalterPahlavi,

        Rejang,
        Runic,

        Samaritan,
        Saurashtra,
        Sharada,
        Shavian,
        Siddham,
        Sidetic,
        SignWriting,
        Sinhala,
        Sogdian,
        SoraSompeng,
        Soyombo,
        Sundanese,
        Sunuwar,
        SylotiNagri,
        Syriac,

        Tagalog,
        Tagbanwa,
        TaiLe,
        TaiTham,
        TaiViet,
        TaiYo,
        Takri,
        Tamil,
        Tangsa,
        Tangut,
        Telugu,
        Thaana,
        Thai,
        Tibetan,
        Tifinagh,
        Tirhuta,
        Todhri,
        TolongSiki,
        Toto,
        TuluTigalari,

        Ugaritic,

        Vai,
        Vithkuqi,

        Wancho,
        WarangCiti,

        Yezidi,
        Yi,

        ZanabazarSquare,

        Count
    };


    // ========================================================================
    // UnicodeScriptRange
    //
    // One contiguous range carrying a Unicode Script property value.
    //
    // first and last are inclusive.
    //
    // The generated table will come directly from Scripts.txt and will be
    // sorted by first code point.
    //
    // ========================================================================

    struct UnicodeScriptRange
    {
        uint32_t first;
        uint32_t last;
        UnicodeScript script;

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
    // UnicodeScriptInfo
    //
    // Metadata associated with a Unicode Script property value.
    //
    // name:
    //     Unicode long property value name.
    //
    // iso15924:
    //     Four-character ISO 15924 script code, which is also used as the
    //     Unicode short property value alias for Script.
    //
    // NOTE:
    //
    // These ISO 15924 values must not be confused with OpenType ScriptList
    // tags.  Although often related, they are separate namespaces.
    //
    // ========================================================================

    struct UnicodeScriptInfo
    {
        UnicodeScript script;

        InternedKey name;
        InternedKey iso15924;
    };


    // ========================================================================
    // Script metadata
    //
    // These names are interned once during static initialization.
    //
    // Eventually this table should be generated from
    // PropertyValueAliases.txt.
    //
    // For now this contains a representative subset corresponding to the
    // hand-written range sample below.
    //
    // ========================================================================

    inline const UnicodeScriptInfo kUnicodeScripts[] =
    {
        {
            UnicodeScript::Unknown,
            WSNameSet::INTERN("Unknown"),
            WSNameSet::INTERN("Zzzz")
        },
        {
            UnicodeScript::Common,
            WSNameSet::INTERN("Common"),
            WSNameSet::INTERN("Zyyy")
        },
        {
            UnicodeScript::Inherited,
            WSNameSet::INTERN("Inherited"),
            WSNameSet::INTERN("Zinh")
        },

        {
            UnicodeScript::Latin,
            WSNameSet::INTERN("Latin"),
            WSNameSet::INTERN("Latn")
        },
        {
            UnicodeScript::Greek,
            WSNameSet::INTERN("Greek"),
            WSNameSet::INTERN("Grek")
        },
        {
            UnicodeScript::Cyrillic,
            WSNameSet::INTERN("Cyrillic"),
            WSNameSet::INTERN("Cyrl")
        },
        {
            UnicodeScript::Armenian,
            WSNameSet::INTERN("Armenian"),
            WSNameSet::INTERN("Armn")
        },
        {
            UnicodeScript::Hebrew,
            WSNameSet::INTERN("Hebrew"),
            WSNameSet::INTERN("Hebr")
        },
        {
            UnicodeScript::Arabic,
            WSNameSet::INTERN("Arabic"),
            WSNameSet::INTERN("Arab")
        },
        {
            UnicodeScript::Syriac,
            WSNameSet::INTERN("Syriac"),
            WSNameSet::INTERN("Syrc")
        },

        {
            UnicodeScript::Devanagari,
            WSNameSet::INTERN("Devanagari"),
            WSNameSet::INTERN("Deva")
        },
        {
            UnicodeScript::Bengali,
            WSNameSet::INTERN("Bengali"),
            WSNameSet::INTERN("Beng")
        },
        {
            UnicodeScript::Gurmukhi,
            WSNameSet::INTERN("Gurmukhi"),
            WSNameSet::INTERN("Guru")
        },
        {
            UnicodeScript::Gujarati,
            WSNameSet::INTERN("Gujarati"),
            WSNameSet::INTERN("Gujr")
        },
        {
            UnicodeScript::Oriya,
            WSNameSet::INTERN("Oriya"),
            WSNameSet::INTERN("Orya")
        },
        {
            UnicodeScript::Tamil,
            WSNameSet::INTERN("Tamil"),
            WSNameSet::INTERN("Taml")
        },
        {
            UnicodeScript::Telugu,
            WSNameSet::INTERN("Telugu"),
            WSNameSet::INTERN("Telu")
        },
        {
            UnicodeScript::Kannada,
            WSNameSet::INTERN("Kannada"),
            WSNameSet::INTERN("Knda")
        },
        {
            UnicodeScript::Malayalam,
            WSNameSet::INTERN("Malayalam"),
            WSNameSet::INTERN("Mlym")
        },

        {
            UnicodeScript::Thai,
            WSNameSet::INTERN("Thai"),
            WSNameSet::INTERN("Thai")
        },
        {
            UnicodeScript::Lao,
            WSNameSet::INTERN("Lao"),
            WSNameSet::INTERN("Laoo")
        },

        {
            UnicodeScript::Han,
            WSNameSet::INTERN("Han"),
            WSNameSet::INTERN("Hani")
        },
        {
            UnicodeScript::Hiragana,
            WSNameSet::INTERN("Hiragana"),
            WSNameSet::INTERN("Hira")
        },
        {
            UnicodeScript::Katakana,
            WSNameSet::INTERN("Katakana"),
            WSNameSet::INTERN("Kana")
        },
        {
            UnicodeScript::Hangul,
            WSNameSet::INTERN("Hangul"),
            WSNameSet::INTERN("Hang")
        },

        {
            UnicodeScript::Braille,
            WSNameSet::INTERN("Braille"),
            WSNameSet::INTERN("Brai")
        }
    };


    inline constexpr size_t kUnicodeScriptInfoCount =
        sizeof(kUnicodeScripts) /
        sizeof(kUnicodeScripts[0]);


    // ========================================================================
    // Initial Scripts.txt-style range table.
    //
    // This is deliberately only a small sample used to establish the shape
    // of the API.
    //
    // Do not treat these ranges as the authoritative Unicode Script database.
    // The completed table will be generated directly from Scripts.txt.
    //
    // IMPORTANT:
    //
    // Keep ranges sorted by first code point.  scriptOf() relies on this.
    //
    // ========================================================================

    inline constexpr UnicodeScriptRange kUnicodeScriptRanges[] =
    {
        // --------------------------------------------------------------------
        // Common
        // --------------------------------------------------------------------

        {
            0x0000,
            0x0040,
            UnicodeScript::Common
        },

        // --------------------------------------------------------------------
        // Latin
        // --------------------------------------------------------------------

        {
            0x0041,
            0x005A,
            UnicodeScript::Latin
        },
        {
            0x005B,
            0x0060,
            UnicodeScript::Common
        },
        {
            0x0061,
            0x007A,
            UnicodeScript::Latin
        },
        {
            0x007B,
            0x00A9,
            UnicodeScript::Common
        },
        {
            0x00AA,
            0x00AA,
            UnicodeScript::Latin
        },
        {
            0x00AB,
            0x00B9,
            UnicodeScript::Common
        },
        {
            0x00BA,
            0x00BA,
            UnicodeScript::Latin
        },
        {
            0x00BB,
            0x00BF,
            UnicodeScript::Common
        },
        {
            0x00C0,
            0x00D6,
            UnicodeScript::Latin
        },
        {
            0x00D8,
            0x00F6,
            UnicodeScript::Latin
        },
        {
            0x00F8,
            0x02E4,
            UnicodeScript::Latin
        },

        // --------------------------------------------------------------------
        // Greek
        // --------------------------------------------------------------------

        {
            0x0370,
            0x0373,
            UnicodeScript::Greek
        },
        {
            0x0375,
            0x0377,
            UnicodeScript::Greek
        },
        {
            0x037A,
            0x037D,
            UnicodeScript::Greek
        },

        // --------------------------------------------------------------------
        // Cyrillic
        // --------------------------------------------------------------------

        {
            0x0400,
            0x0481,
            UnicodeScript::Cyrillic
        },
        {
            0x048A,
            0x052F,
            UnicodeScript::Cyrillic
        },

        // --------------------------------------------------------------------
        // Hebrew
        // --------------------------------------------------------------------

        {
            0x0591,
            0x05C7,
            UnicodeScript::Hebrew
        },
        {
            0x05D0,
            0x05EA,
            UnicodeScript::Hebrew
        },

        // --------------------------------------------------------------------
        // Arabic
        // --------------------------------------------------------------------

        {
            0x0600,
            0x0604,
            UnicodeScript::Arabic
        },
        {
            0x0606,
            0x060B,
            UnicodeScript::Arabic
        },
        {
            0x0610,
            0x061A,
            UnicodeScript::Arabic
        },
        {
            0x0620,
            0x063F,
            UnicodeScript::Arabic
        },
        {
            0x0641,
            0x064A,
            UnicodeScript::Arabic
        },

        // --------------------------------------------------------------------
        // Devanagari
        // --------------------------------------------------------------------

        {
            0x0900,
            0x0950,
            UnicodeScript::Devanagari
        },
        {
            0x0955,
            0x0963,
            UnicodeScript::Devanagari
        },

        // --------------------------------------------------------------------
        // Telugu
        // --------------------------------------------------------------------

        {
            0x0C00,
            0x0C04,
            UnicodeScript::Telugu
        },
        {
            0x0C05,
            0x0C0C,
            UnicodeScript::Telugu
        },
        {
            0x0C0E,
            0x0C10,
            UnicodeScript::Telugu
        },

        // --------------------------------------------------------------------
        // Hiragana
        // --------------------------------------------------------------------

        {
            0x3041,
            0x3096,
            UnicodeScript::Hiragana
        },

        // --------------------------------------------------------------------
        // Katakana
        // --------------------------------------------------------------------

        {
            0x30A1,
            0x30FA,
            UnicodeScript::Katakana
        },

        // --------------------------------------------------------------------
        // Hangul
        // --------------------------------------------------------------------

        {
            0xAC00,
            0xD7A3,
            UnicodeScript::Hangul
        }
    };


    inline constexpr size_t kUnicodeScriptRangeCount =
        sizeof(kUnicodeScriptRanges) /
        sizeof(kUnicodeScriptRanges[0]);


    // ========================================================================
    // scriptInfo
    //
    // Lookup metadata for a script.
    //
    // Once the complete generated metadata table exists, the enum values can
    // be arranged so this becomes a direct indexed lookup if desired.
    //
    // ========================================================================

    [[nodiscard]]
    inline const UnicodeScriptInfo*
        scriptInfo(UnicodeScript script) noexcept
    {
        for (const auto& info : kUnicodeScripts) {
            if (info.script == script)
                return &info;
        }

        return nullptr;
    }


    // ========================================================================
    // scriptName
    // ========================================================================

    [[nodiscard]]
    inline InternedKey
        scriptName(UnicodeScript script) noexcept
    {
        const UnicodeScriptInfo* info =
            scriptInfo(script);

        return info
            ? info->name
            : nullptr;
    }


    // ========================================================================
    // scriptISO15924
    // ========================================================================

    [[nodiscard]]
    inline InternedKey
        scriptISO15924(UnicodeScript script) noexcept
    {
        const UnicodeScriptInfo* info =
            scriptInfo(script);

        return info
            ? info->iso15924
            : nullptr;
    }


    // ========================================================================
    // scriptOf
    //
    // Return the Unicode Script property for a code point.
    //
    // The generated table will contain the complete Scripts.txt data.
    //
    // A binary search will probably make sense once that table becomes
    // substantial.  The simple linear implementation is sufficient while
    // establishing the API.
    //
    // ========================================================================

    [[nodiscard]]
    inline constexpr UnicodeScript
        scriptOf(uint32_t cp) noexcept
    {
        if (cp >= UnicodeCoverage::kUnicodeLimit)
            return UnicodeScript::Unknown;

        for (const auto& range : kUnicodeScriptRanges) {
            if (cp < range.first)
                break;

            if (cp <= range.last)
                return range.script;
        }

        return UnicodeScript::Unknown;
    }


    // ========================================================================
    // coverage
    //
    // Produce the complete UnicodeCoverage represented by the Script property
    // ranges currently present in the database.
    //
    // Unlike a Unicode block, a script normally consists of many disjoint
    // ranges.
    //
    // ========================================================================

    [[nodiscard]]
    inline UnicodeCoverage
        coverage(UnicodeScript script)
    {
        UnicodeCoverageBuilder builder;

        for (const auto& range : kUnicodeScriptRanges) {
            if (range.script != script)
                continue;

            builder.addRange(
                range.first,
                range.last);
        }

        return builder.finalize();
    }

} // namespace waavs