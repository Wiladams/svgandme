// opentype_features.h
#pragma once

#include "core_nametable.h"
#include "core_openhashmap.h"
#include "opentype_types.h"

namespace waavs {
    namespace opentype {

        // ============================================================================
        // Feature Bit Definitions
        // ============================================================================

        enum CommonFeatureBit : uint64_t {
            // Ligatures
            CF_LIGA = 1ULL << 0,
            CF_DLIG = 1ULL << 1,
            CF_CLIG = 1ULL << 2,
            CF_HLIG = 1ULL << 3,

            // Small Caps
            CF_SMCP = 1ULL << 4,
            CF_C2SC = 1ULL << 5,
            CF_PCAP = 1ULL << 6,
            CF_C2PC = 1ULL << 7,

            // Figures
            CF_LNUM = 1ULL << 8,
            CF_ONUM = 1ULL << 9,
            CF_PNUM = 1ULL << 10,
            CF_TNUM = 1ULL << 11,
            CF_ZERO = 1ULL << 12,

            // Vertical Positioning
            CF_SUPS = 1ULL << 13,
            CF_SUBS = 1ULL << 14,
            CF_SINF = 1ULL << 15,
            CF_ORDN = 1ULL << 16,

            // Alternates
            CF_SALT = 1ULL << 17,
            CF_AALT = 1ULL << 18,
            CF_SWSH = 1ULL << 19,
            CF_CSWH = 1ULL << 20,
            CF_HIST = 1ULL << 21,

            // Fractions
            CF_FRAC = 1ULL << 22,
            CF_AFRC = 1ULL << 23,

            // Stylistic Sets (first 20)
            CF_SS01 = 1ULL << 24,
            CF_SS02 = 1ULL << 25,
            CF_SS03 = 1ULL << 26,
            CF_SS04 = 1ULL << 27,
            CF_SS05 = 1ULL << 28,
            CF_SS06 = 1ULL << 29,
            CF_SS07 = 1ULL << 30,
            CF_SS08 = 1ULL << 31,
            CF_SS09 = 1ULL << 32,
            CF_SS10 = 1ULL << 33,
            CF_SS11 = 1ULL << 34,
            CF_SS12 = 1ULL << 35,
            CF_SS13 = 1ULL << 36,
            CF_SS14 = 1ULL << 37,
            CF_SS15 = 1ULL << 38,
            CF_SS16 = 1ULL << 39,
            CF_SS17 = 1ULL << 40,
            CF_SS18 = 1ULL << 41,
            CF_SS19 = 1ULL << 42,
            CF_SS20 = 1ULL << 43,

            // Case
            CF_CASE = 1ULL << 44,
            CF_CPSP = 1ULL << 45,
            CF_TITL = 1ULL << 46,

            // Other common
            CF_ORNM = 1ULL << 47,
            CF_LOCL = 1ULL << 48,
            CF_KERN = 1ULL << 49,
            CF_MARK = 1ULL << 50,
            CF_MKMK = 1ULL << 51,
            CF_RLIG = 1ULL << 52,

            // Complex script (common)
            CF_INIT = 1ULL << 53,
            CF_MEDI = 1ULL << 54,
            CF_FINA = 1ULL << 55,
            CF_ISOL = 1ULL << 56,
            CF_NUKT = 1ULL << 57,
            CF_AKHN = 1ULL << 58,

            // That's 59 bits, we have room for 5 more common features
        };

        // ============================================================================
        // Feature Tag Mapper - Uses WSNameMap to map tag -> bit at parse time
        // ============================================================================

        class FeatureTagMapper
        {
        private:
            static WSOpenHashMap<Tag, uint64_t, WSHash32> sTagMap;
            static bool sInitialized;

            static void initMapping() noexcept
            {
                if (sInitialized)
                    return;

#define REGISTER_FEATURE(tag, bit) \
        sTagMap.put(OTAG(tag), bit);

                REGISTER_FEATURE("liga", CF_LIGA);
                REGISTER_FEATURE("dlig", CF_DLIG);
                REGISTER_FEATURE("clig", CF_CLIG);
                REGISTER_FEATURE("hlig", CF_HLIG);

                REGISTER_FEATURE("smcp", CF_SMCP);
                REGISTER_FEATURE("c2sc", CF_C2SC);
                REGISTER_FEATURE("pcap", CF_PCAP);
                REGISTER_FEATURE("c2pc", CF_C2PC);

                REGISTER_FEATURE("lnum", CF_LNUM);
                REGISTER_FEATURE("onum", CF_ONUM);
                REGISTER_FEATURE("pnum", CF_PNUM);
                REGISTER_FEATURE("tnum", CF_TNUM);
                REGISTER_FEATURE("zero", CF_ZERO);

                REGISTER_FEATURE("sups", CF_SUPS);
                REGISTER_FEATURE("subs", CF_SUBS);
                REGISTER_FEATURE("sinf", CF_SINF);
                REGISTER_FEATURE("ordn", CF_ORDN);

                REGISTER_FEATURE("salt", CF_SALT);
                REGISTER_FEATURE("aalt", CF_AALT);
                REGISTER_FEATURE("swsh", CF_SWSH);
                REGISTER_FEATURE("cswh", CF_CSWH);
                REGISTER_FEATURE("hist", CF_HIST);

                REGISTER_FEATURE("frac", CF_FRAC);
                REGISTER_FEATURE("afrc", CF_AFRC);

                REGISTER_FEATURE("ss01", CF_SS01);
                REGISTER_FEATURE("ss02", CF_SS02);
                REGISTER_FEATURE("ss03", CF_SS03);
                REGISTER_FEATURE("ss04", CF_SS04);
                REGISTER_FEATURE("ss05", CF_SS05);
                REGISTER_FEATURE("ss06", CF_SS06);
                REGISTER_FEATURE("ss07", CF_SS07);
                REGISTER_FEATURE("ss08", CF_SS08);
                REGISTER_FEATURE("ss09", CF_SS09);
                REGISTER_FEATURE("ss10", CF_SS10);
                REGISTER_FEATURE("ss11", CF_SS11);
                REGISTER_FEATURE("ss12", CF_SS12);
                REGISTER_FEATURE("ss13", CF_SS13);
                REGISTER_FEATURE("ss14", CF_SS14);
                REGISTER_FEATURE("ss15", CF_SS15);
                REGISTER_FEATURE("ss16", CF_SS16);
                REGISTER_FEATURE("ss17", CF_SS17);
                REGISTER_FEATURE("ss18", CF_SS18);
                REGISTER_FEATURE("ss19", CF_SS19);
                REGISTER_FEATURE("ss20", CF_SS20);

                REGISTER_FEATURE("case", CF_CASE);
                REGISTER_FEATURE("cpsp", CF_CPSP);
                REGISTER_FEATURE("titl", CF_TITL);

                REGISTER_FEATURE("ornm", CF_ORNM);
                REGISTER_FEATURE("locl", CF_LOCL);
                REGISTER_FEATURE("kern", CF_KERN);
                REGISTER_FEATURE("mark", CF_MARK);
                REGISTER_FEATURE("mkmk", CF_MKMK);
                REGISTER_FEATURE("rlig", CF_RLIG);

                REGISTER_FEATURE("init", CF_INIT);
                REGISTER_FEATURE("medi", CF_MEDI);
                REGISTER_FEATURE("fina", CF_FINA);
                REGISTER_FEATURE("isol", CF_ISOL);
                REGISTER_FEATURE("nukt", CF_NUKT);
                REGISTER_FEATURE("akhn", CF_AKHN);

#undef REGISTER_FEATURE

                sInitialized = true;
            }

        public:
            static uint64_t tagToBit(Tag tag) noexcept
            {
                initMapping();

                const uint64_t* bit =
                    sTagMap.getRef(tag);

                return bit ? *bit : 0;
            }

            static Tag bitToTag(uint64_t bit) noexcept
            {
                initMapping();

                Tag result = 0;

                sTagMap.forEachWhile(
                    [&](Tag key, uint64_t value)
                    {
                        if (value == bit)
                        {
                            result = key;
                            return false;
                        }

                        return true;
                    });

                return result;
            }

            static bool isCommonFeature(Tag tag) noexcept
            {
                return tagToBit(tag) != 0;
            }
        };

        // Static members
        WSOpenHashMap<Tag, uint64_t, WSHash32> FeatureTagMapper::sTagMap;
        bool FeatureTagMapper::sInitialized = false;

        // ============================================================================
        // FeatureSet - Fast bitmask with per-font extended features
        // ============================================================================

        class FeatureSet
        {
        private:
            uint64_t mCommonMask{ 0 };

            WSOpenHashMap<Tag, uint8_t, WSHash32>
                mExtendedFeatures;

        public:
            inline bool hasFeature(uint64_t bit) const noexcept
            {
                return (mCommonMask & bit) != 0;
            }

            inline bool hasFeature(Tag tag) const noexcept
            {
                uint64_t bit =
                    FeatureTagMapper::tagToBit(tag);

                if (bit != 0)
                    return hasFeature(bit);

                return mExtendedFeatures.contains(tag);
            }

            inline void addFeature(Tag tag) noexcept
            {
                uint64_t bit =
                    FeatureTagMapper::tagToBit(tag);

                if (bit != 0)
                {
                    mCommonMask |= bit;
                }
                else
                {
                    mExtendedFeatures.put(tag, 1);
                }
            }

            inline uint64_t commonMask() const noexcept
            {
                return mCommonMask;
            }

            template<typename Fn>
            void forEach(Fn&& fn) const noexcept
            {
                uint64_t mask = mCommonMask;

                while (mask)
                {
                    uint64_t bit = mask & -mask;

                    Tag tag = FeatureTagMapper::bitToTag(bit);

                    if (tag != 0)
                        fn(tag);

                    mask &= ~bit;
                }

                mExtendedFeatures.forEach(
                    [&](Tag tag, uint8_t)
                    {
                        fn(tag);
                    });
            }

            inline bool hasCommonFeatures() const noexcept
            {
                return mCommonMask != 0;
            }

            inline bool hasExtendedFeatures() const noexcept
            {
                return !mExtendedFeatures.empty();
            }

            inline size_t extendedFeatureCount() const noexcept
            {
                return mExtendedFeatures.size();
            }
        };

    } // namespace opentype
} // namespace waavs