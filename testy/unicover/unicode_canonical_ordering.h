// unicode_canonical_ordering.h

#pragma once

#include <cstddef>
#include <cstdint>

#include "unicode_combining_class.h"
#include "unicode_database.h"


namespace waavs
{
    // ========================================================================
    // UnicodeCanonicalOrdering
    //
    // Perform canonical combining-class ordering on an already canonically
    // decomposed Unicode code-point sequence.
    //
    // Canonical ordering operates independently within each combining
    // sequence. A code point whose Canonical_Combining_Class is zero acts as
    // a boundary and is never moved.
    //
    // Non-zero combining characters are placed in ascending CCC order.
    //
    // Ordering is stable:
    //
    //      equal CCC values retain their original relative order.
    //
    // Example:
    //
    //      code points:      A      mark1   mark2   mark3
    //      CCC:              0      230     220     232
    //
    // becomes:
    //
    //      code points:      A      mark2   mark1   mark3
    //      CCC:              0      220     230     232
    //
    //
    // The algorithm uses stable insertion ordering. Combining sequences are
    // normally very short, making this preferable to a general-purpose sort:
    //
    //      - no allocation
    //      - no temporary buffer
    //      - naturally stable
    //      - never crosses CCC=0 boundaries
    //      - very small implementation
    //
    // The UnicodeDatabase supplied to this object must remain alive and must
    // not be reset while this object is in use.
    //
    // ========================================================================

    class UnicodeCanonicalOrdering
    {
    public:
        UnicodeCanonicalOrdering() noexcept = default;

        explicit UnicodeCanonicalOrdering(const UnicodeDatabase& database) noexcept {
            reset(database);
        }


        // ====================================================================
        // State
        // ====================================================================

        [[nodiscard]] bool valid() const noexcept {
            return mDatabase != nullptr && mDatabase->valid();
        }

        explicit operator bool() const noexcept {
            return valid();
        }

        void clear() noexcept {
            mDatabase = nullptr;
        }


        // ====================================================================
        // reset
        //
        // Attach to a Unicode database containing Canonical_Combining_Class.
        //
        // CCC is optional at the persistent database level. Normalization,
        // however, requires it. Therefore we explicitly verify that the
        // semantic CCC property exists rather than relying on combiningClass()
        // returning its default value of zero.
        // ====================================================================

        bool reset(const UnicodeDatabase& database) noexcept
        {
            clear();

            if (!database.valid())
                return false;

            const uint16_t property =
                static_cast<uint16_t>(
                    UnicodeValueProperty8CanonicalCombiningClass);

            for (uint32_t i = 0; i < database.valueProperty8Count(); ++i)
            {
                const UnicodeValueProperty8Record* record =
                    database.valueProperty8Record(i);

                if (record && record->property == property) {
                    mDatabase = &database;
                    return true;
                }
            }

            return false;
        }


        // ====================================================================
        // combiningClass
        //
        // Convenience access to the CCC table associated with this ordering
        // object.
        // ====================================================================

        [[nodiscard]]
        UnicodeCombiningClass combiningClass(uint32_t cp) const noexcept
        {
            if (!valid())
                return kUnicodeCombiningClassNotReordered;

            return mDatabase->combiningClass(cp);
        }


        // ====================================================================
        // order
        //
        // Canonically reorder codePoints[] in place.
        //
        // Returns false when:
        //
        //      - the ordering object is invalid
        //      - codePoints is null while count is non-zero
        //      - any input value is outside the Unicode address space
        //
        // Input validation occurs before modification, so invalid input leaves
        // the supplied sequence unchanged.
        //
        // count == 0 is valid and requires no codePoints buffer.
        // ====================================================================

        [[nodiscard]]
        bool order(uint32_t* codePoints, size_t count) const noexcept
        {
            if (!valid())
                return false;

            if (count == 0)
                return true;

            if (!codePoints)
                return false;


            // ---------------------------------------------------------------
            // Validate completely before modifying the sequence.
            // ---------------------------------------------------------------

            for (size_t i = 0; i < count; ++i) {
                if (codePoints[i] >= kUnicodeLimit)
                    return false;
            }


            // ---------------------------------------------------------------
            // Stable insertion ordering.
            //
            // For each non-starter, walk backward while the preceding CCC is
            // greater than the current CCC.
            //
            // Stop when:
            //
            //      previous CCC == 0
            //
            //          A starter is an ordering boundary.
            //
            //      previous CCC <= current CCC
            //
            //          Correct ascending position reached.
            //
            // The strict '>' comparison is what preserves stability for equal
            // combining classes.
            // ---------------------------------------------------------------

            for (size_t i = 1; i < count; ++i)
            {
                const uint32_t cp = codePoints[i];

                const UnicodeCombiningClass ccc =
                    mDatabase->combiningClass(cp);


                // CCC zero establishes a new combining sequence.
                if (ccc == kUnicodeCombiningClassNotReordered)
                    continue;


                size_t j = i;


                while (j > 0)
                {
                    const UnicodeCombiningClass previousCCC =
                        mDatabase->combiningClass(
                            codePoints[j - 1]);


                    // Never cross a starter.
                    if (previousCCC ==
                        kUnicodeCombiningClassNotReordered)
                    {
                        break;
                    }


                    // Already in ascending order.
                    //
                    // Equality deliberately stops here, preserving the
                    // original order of equal-CCC characters.
                    if (previousCCC <= ccc)
                        break;


                    codePoints[j] =
                        codePoints[j - 1];

                    --j;
                }


                if (j != i)
                    codePoints[j] = cp;
            }


            return true;
        }


        // ====================================================================
        // isOrdered
        //
        // Check whether a sequence is already in canonical order.
        //
        // Within every sequence of non-zero CCC values:
        //
        //      ccc[n] <= ccc[n + 1]
        //
        // CCC zero resets the comparison.
        //
        // As with order(), invalid input returns false.
        // ====================================================================

        [[nodiscard]]
        bool isOrdered(const uint32_t* codePoints, size_t count) const noexcept
        {
            if (!valid())
                return false;

            if (count == 0)
                return true;

            if (!codePoints)
                return false;


            UnicodeCombiningClass previousCCC =
                kUnicodeCombiningClassNotReordered;


            for (size_t i = 0; i < count; ++i)
            {
                const uint32_t cp =
                    codePoints[i];


                if (cp >= kUnicodeLimit)
                    return false;


                const UnicodeCombiningClass ccc =
                    mDatabase->combiningClass(cp);


                if (ccc ==
                    kUnicodeCombiningClassNotReordered)
                {
                    previousCCC =
                        kUnicodeCombiningClassNotReordered;

                    continue;
                }


                if (previousCCC !=
                    kUnicodeCombiningClassNotReordered &&
                    previousCCC > ccc)
                {
                    return false;
                }


                previousCCC = ccc;
            }


            return true;
        }


    private:
        const UnicodeDatabase* mDatabase{ nullptr };
    };

} // namespace waavs

