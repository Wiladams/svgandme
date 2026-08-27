// unicode_canonical_composer.h

#pragma once

#include <cstdint>

#include "unicode_composition.h"
#include "unicode_coverage_data.h"


namespace waavs
{
    // ========================================================================
    // UnicodeCanonicalComposer
    //
    // Runtime primitive for composing one pair of Unicode code points:
    //
    //      first + second -> composite
    //
    // Composition is supplied by:
    //
    //      - algorithmic Hangul composition
    //      - explicit canonical composition lookup
    //
    // Explicit composition mappings have already had
    // Full_Composition_Exclusion applied by the database generator.
    //
    // This class deliberately does NOT perform sequence canonical composition.
    // In particular, it does not implement:
    //
    //      - starter tracking
    //      - Canonical_Combining_Class blocking
    //      - in-place sequence reduction
    //      - NFC
    //
    // Those operations belong to the sequence-composition layer above this
    // primitive.
    //
    // The UnicodeComposition backing storage must remain valid for the lifetime
    // of this object.
    // ========================================================================

    class UnicodeCanonicalComposer
    {
    public:
        UnicodeCanonicalComposer() noexcept = default;

        explicit UnicodeCanonicalComposer(UnicodeComposition composition) noexcept
            : mComposition(composition)
        {
        }


        // ====================================================================
        // State
        // ====================================================================

        [[nodiscard]]
        bool valid() const noexcept {
            return mComposition.valid();
        }


        explicit operator bool() const noexcept {
            return valid();
        }


        void reset(UnicodeComposition composition = {}) noexcept {
            mComposition = composition;
        }


        [[nodiscard]]
        const UnicodeComposition& composition() const noexcept {
            return mComposition;
        }


        // ====================================================================
        // compose
        //
        // Attempt to canonically compose:
        //
        //      first + second
        //
        // into one code point.
        //
        // Returns true when a composition exists and stores it in outComposite.
        //
        // Returns false when:
        //
        //      - this composer is invalid
        //      - either input lies outside Unicode
        //      - the pair has no canonical composition
        //
        // outComposite is changed only on success.
        //
        // Hangul composition is attempted algorithmically before consulting the
        // explicit persistent composition table.
        // ====================================================================

        [[nodiscard]]
        bool compose(uint32_t first, uint32_t second,
            uint32_t& outComposite) const noexcept
        {
            if (!valid() ||
                first >= kUnicodeLimit ||
                second >= kUnicodeLimit)
            {
                return false;
            }


            uint32_t composite;


            // ---------------------------------------------------------------
            // Algorithmic Hangul composition.
            // ---------------------------------------------------------------

            if (composeHangul(
                first,
                second,
                composite))
            {
                outComposite = composite;
                return true;
            }


            // ---------------------------------------------------------------
            // Explicit Unicode canonical composition.
            // ---------------------------------------------------------------

            composite =
                mComposition.composite(
                    first,
                    second);


            if (composite ==
                kUnicodeCompositionNone)
            {
                return false;
            }


            outComposite = composite;
            return true;
        }


        // ====================================================================
        // composite
        //
        // Convenience form returning:
        //
        //      composed code point
        //
        // or:
        //
        //      kUnicodeCompositionNone
        //
        // when no composition exists.
        // ====================================================================

        [[nodiscard]]
        uint32_t composite(uint32_t first, uint32_t second) const noexcept
        {
            uint32_t result;


            if (!compose(
                first,
                second,
                result))
            {
                return kUnicodeCompositionNone;
            }


            return result;
        }


        // ====================================================================
        // canCompose
        // ====================================================================

        [[nodiscard]]
        bool canCompose(uint32_t first, uint32_t second) const noexcept
        {
            uint32_t ignored;

            return compose(
                first,
                second,
                ignored);
        }


    private:
        // ====================================================================
        // Hangul constants
        //
        // Canonical Hangul composition uses the same Unicode constants as
        // canonical Hangul decomposition.
        //
        //      L + V  -> LV
        //      LV + T -> LVT
        //
        // TBase itself represents the absence of a trailing consonant and is
        // therefore not a composable T jamo.
        // ====================================================================

        static constexpr uint32_t kHangulSBase = 0xAC00u;
        static constexpr uint32_t kHangulLBase = 0x1100u;
        static constexpr uint32_t kHangulVBase = 0x1161u;
        static constexpr uint32_t kHangulTBase = 0x11A7u;

        static constexpr uint32_t kHangulLCount = 19u;
        static constexpr uint32_t kHangulVCount = 21u;
        static constexpr uint32_t kHangulTCount = 28u;

        static constexpr uint32_t kHangulNCount =
            kHangulVCount * kHangulTCount;

        static constexpr uint32_t kHangulSCount =
            kHangulLCount * kHangulNCount;


        // ====================================================================
        // composeHangul
        //
        // Algorithmically compose:
        //
        //      L + V -> LV
        //
        // and:
        //
        //      LV + T -> LVT
        //
        // Returns false for every other pair.
        //
        // outComposite is changed only on success.
        // ====================================================================

        [[nodiscard]]
        static bool composeHangul(uint32_t first, uint32_t second,
            uint32_t& outComposite) noexcept
        {
            // ---------------------------------------------------------------
            // L + V -> LV
            // ---------------------------------------------------------------

            if (first >= kHangulLBase &&
                first < kHangulLBase + kHangulLCount &&
                second >= kHangulVBase &&
                second < kHangulVBase + kHangulVCount)
            {
                const uint32_t lIndex =
                    first - kHangulLBase;

                const uint32_t vIndex =
                    second - kHangulVBase;


                const uint32_t lvIndex =
                    (lIndex * kHangulVCount + vIndex) *
                    kHangulTCount;


                outComposite =
                    kHangulSBase + lvIndex;

                return true;
            }


            // ---------------------------------------------------------------
            // LV + T -> LVT
            //
            // Only an LV syllable having no trailing consonant may accept T:
            //
            //      SIndex % TCount == 0
            //
            // Valid T jamo begin at TBase + 1.
            // ---------------------------------------------------------------

            if (first >= kHangulSBase &&
                first < kHangulSBase + kHangulSCount)
            {
                const uint32_t sIndex =
                    first - kHangulSBase;


                if ((sIndex % kHangulTCount) == 0 &&
                    second > kHangulTBase &&
                    second < kHangulTBase + kHangulTCount)
                {
                    const uint32_t tIndex =
                        second - kHangulTBase;


                    outComposite =
                        first + tIndex;

                    return true;
                }
            }


            return false;
        }


        UnicodeComposition mComposition{};
    };

} // namespace waavs
