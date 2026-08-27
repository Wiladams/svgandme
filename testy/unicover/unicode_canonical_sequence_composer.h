// unicode_canonical_sequence_composer.h

#pragma once

#include <cstddef>
#include <cstdint>

#include "unicode_canonical_composer.h"
#include "unicode_canonical_ordering.h"
#include "unicode_database.h"


namespace waavs
{
    // ========================================================================
    // UnicodeCanonicalSequenceComposer
    //
    // Perform canonical composition over an already canonically decomposed and
    // canonically ordered sequence.
    //
    // Input:
    //
    //      NFD sequence
    //
    // Output:
    //
    //      canonically composed sequence
    //
    // Composition uses:
    //
    //      - UnicodeCanonicalComposer for pair composition
    //      - Canonical_Combining_Class for blocking
    //
    // The sequence is compacted in place. Canonical composition can only
    // preserve or reduce the number of code points.
    //
    // The input sequence must already be:
    //
    //      - canonically decomposed
    //      - canonically ordered
    //
    // Canonical ordering is verified before modification. Canonical
    // decomposition is a semantic precondition and is not independently
    // verified here.
    //
    // No dynamic allocation is performed.
    //
    // The UnicodeDatabase supplied to reset() must remain alive and unchanged
    // for the lifetime of this object.
    // ========================================================================

    class UnicodeCanonicalSequenceComposer
    {
    public:
        UnicodeCanonicalSequenceComposer() noexcept = default;

        explicit UnicodeCanonicalSequenceComposer(const UnicodeDatabase& database) noexcept {
            reset(database);
        }


        // ====================================================================
        // State
        // ====================================================================

        [[nodiscard]] bool valid() const noexcept {
            return mComposer.valid() && mOrdering.valid();
        }

        explicit operator bool() const noexcept {
            return valid();
        }

        void clear() noexcept
        {
            mComposer.reset();
            mOrdering.clear();
        }


        // ====================================================================
        // reset
        //
        // Attach canonical-composition services to a Unicode database.
        //
        // Sequence composition requires:
        //
        //      explicit canonical composition data
        //      Canonical_Combining_Class
        //
        // Hangul pair composition remains algorithmic inside
        // UnicodeCanonicalComposer.
        // ====================================================================

        bool reset(const UnicodeDatabase& database) noexcept
        {
            clear();

            if (!database.valid())
                return false;


            UnicodeCanonicalComposer composer(database.composition());

            if (!composer)
                return false;


            UnicodeCanonicalOrdering ordering(database);

            if (!ordering)
                return false;


            mComposer = composer;
            mOrdering = ordering;

            return true;
        }


        // ====================================================================
        // Accessors
        // ====================================================================

        [[nodiscard]] const UnicodeCanonicalComposer& composer() const noexcept {
            return mComposer;
        }

        [[nodiscard]] const UnicodeCanonicalOrdering& ordering() const noexcept {
            return mOrdering;
        }


        // ====================================================================
        // compose
        //
        // Canonically compose an already decomposed and ordered sequence in
        // place.
        //
        // count is both input and output:
        //
        //      input:
        //          number of code points in codePoints[]
        //
        //      output:
        //          number of code points remaining after composition
        //
        // count == 0 permits codePoints == nullptr.
        //
        // Before modifying the sequence this routine verifies:
        //
        //      - this object is valid
        //      - every code point is valid Unicode
        //      - the input is canonically ordered
        //
        // Therefore failure during validation leaves codePoints[] and count
        // unchanged.
        //
        // Once composition begins, no operation can fail.
        // ====================================================================

        [[nodiscard]] bool compose(uint32_t* codePoints, size_t& count) const noexcept
        {
            if (!valid())
                return false;


            if (count == 0)
                return true;


            if (!codePoints)
                return false;


            // ---------------------------------------------------------------
            // Complete validation before modifying caller storage.
            //
            // isOrdered() also validates every code point.
            // ---------------------------------------------------------------

            if (!mOrdering.isOrdered(codePoints, count))
                return false;


            // ---------------------------------------------------------------
            // Canonical composition.
            //
            // outputPos identifies where the next unconsumed code point is
            // written. Successful composition does not advance outputPos,
            // thereby removing the second code point from the sequence.
            //
            // hasStarter is required because a canonically ordered sequence may
            // begin with one or more non-starters.
            // ---------------------------------------------------------------

            size_t outputPos = 0;
            size_t starterPos = 0;

            uint32_t starter = 0;

            UnicodeCombiningClass lastCCC =
                kUnicodeCombiningClassNotReordered;

            bool hasStarter = false;


            for (size_t inputPos = 0; inputPos < count; ++inputPos)
            {
                const uint32_t cp =
                    codePoints[inputPos];

                const UnicodeCombiningClass ccc =
                    mOrdering.combiningClass(cp);


                // -----------------------------------------------------------
                // Try to combine the current character with the active
                // starter.
                //
                // A character is blocked when a preceding character since the
                // starter has CCC >= its CCC.
                //
                // Because the input is canonically ordered, this reduces to:
                //
                //      lastCCC < ccc
                //
                // or:
                //
                //      lastCCC == 0
                //
                // The second condition also permits Hangul composition where
                // both characters have CCC zero.
                // -----------------------------------------------------------

                if (hasStarter &&
                    (lastCCC < ccc ||
                        lastCCC == kUnicodeCombiningClassNotReordered))
                {
                    uint32_t composite;


                    if (mComposer.compose(
                        starter,
                        cp,
                        composite))
                    {
                        codePoints[starterPos] =
                            composite;

                        starter =
                            composite;


                        // ---------------------------------------------------
                        // Do not update lastCCC.
                        //
                        // cp was consumed by composition and therefore does
                        // not become an intervening blocking character.
                        // ---------------------------------------------------

                        continue;
                    }
                }


                // -----------------------------------------------------------
                // The current code point remains in the output.
                //
                // CCC zero establishes a new starter. Any previous starter can
                // no longer participate in composition across this boundary.
                // -----------------------------------------------------------

                if (ccc == kUnicodeCombiningClassNotReordered)
                {
                    starterPos =
                        outputPos;

                    starter =
                        cp;

                    hasStarter =
                        true;
                }


                // -----------------------------------------------------------
                // Record the CCC of the most recently retained character.
                //
                // For a new starter this becomes zero.
                // -----------------------------------------------------------

                lastCCC =
                    ccc;


                // -----------------------------------------------------------
                // Compact in place.
                //
                // outputPos is never greater than inputPos, so this write can
                // never overwrite unread input.
                // -----------------------------------------------------------

                codePoints[outputPos++] =
                    cp;
            }


            count =
                outputPos;

            return true;
        }


    private:
        UnicodeCanonicalComposer mComposer{};
        UnicodeCanonicalOrdering mOrdering{};
    };

} // namespace waavs