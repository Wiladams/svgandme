// unicode_normalizer.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include "unicode_canonical_decomposer.h"
#include "unicode_canonical_ordering.h"
#include "unicode_canonical_sequence_composer.h"
#include "unicode_database.h"


namespace waavs
{
    // ========================================================================
    // UnicodeNormalizer
    //
    // Unicode normalization algorithms built on the primitive runtime Unicode
    // database services.
    //
    // Currently implemented:
    //
    //      NFD - Canonical Decomposition
    //      NFC - Canonical Decomposition followed by Canonical Composition
    //
    //
    // NFD consists of:
    //
    //      input
    //        |
    //        v
    //      recursive canonical decomposition
    //        |
    //        v
    //      canonical combining-class ordering
    //        |
    //        v
    //      NFD
    //
    //
    // NFC consists of:
    //
    //      input
    //        |
    //        v
    //      NFD
    //        |
    //        v
    //      canonical sequence composition
    //        |
    //        v
    //      NFC
    //
    //
    // This class owns no Unicode data. The UnicodeDatabase supplied to reset()
    // must remain alive and unchanged for the lifetime of the normalizer.
    //
    //
    // Single-code-point normalization:
    //
    //      one input code point
    //          ->
    //      at most kUnicodeCanonicalDecompositionMaxLength code points
    //
    //
    // Sequence normalization:
    //
    //      N input code points
    //          ->
    //      at most N * kUnicodeCanonicalDecompositionMaxLength code points
    //
    //
    // No dynamic allocation is performed by UnicodeNormalizer.
    //
    // ========================================================================

    class UnicodeNormalizer
    {
    public:
        UnicodeNormalizer() noexcept = default;

        explicit UnicodeNormalizer(const UnicodeDatabase& database) noexcept {
            reset(database);
        }


        // ====================================================================
        // State
        // ====================================================================

        [[nodiscard]] bool valid() const noexcept {
            return mDecomposer.valid() && mOrdering.valid();
        }

        [[nodiscard]] bool nfcAvailable() const noexcept {
            return valid() && mComposer.valid();
        }

        explicit operator bool() const noexcept {
            return valid();
        }


        void clear() noexcept
        {
            mDecomposer.reset();
            mOrdering.clear();
            mComposer.clear();
        }


        // ====================================================================
        // reset
        //
        // Attach normalization services to a Unicode database.
        //
        // NFD requires:
        //
        //      canonical decomposition data
        //      Canonical_Combining_Class
        //
        // Both must be available.
        // ====================================================================

        bool reset(const UnicodeDatabase& database) noexcept
        {
            clear();

            if (!database.valid())
                return false;


            UnicodeCanonicalDecomposer decomposer(
                database.decomposition());

            if (!decomposer)
                return false;


            UnicodeCanonicalOrdering ordering(database);

            if (!ordering)
                return false;


            UnicodeCanonicalSequenceComposer composer;

            if (database.hasComposition() &&
                !composer.reset(database))
            {
                return false;
            }


            mDecomposer = decomposer;
            mOrdering = ordering;
            mComposer = composer;

            return true;
        }


        // ====================================================================
        // Accessors
        // ====================================================================

        [[nodiscard]]
        const UnicodeCanonicalDecomposer& decomposer() const noexcept {
            return mDecomposer;
        }


        [[nodiscard]]
        const UnicodeCanonicalOrdering& ordering() const noexcept {
            return mOrdering;
        }

        [[nodiscard]]
        const UnicodeCanonicalSequenceComposer& composer() const noexcept {
            return mComposer;
        }


        // ====================================================================
        // nfdMaximumCapacity
        //
        // Return the worst-case destination capacity required for N input code
        // points.
        //
        // For the current canonical-decomposition representation:
        //
        //      maximum output = inputCount * 4
        //
        // Returns false if the multiplication would overflow size_t.
        //
        // outCapacity is modified only on success.
        // ====================================================================

        [[nodiscard]]
        static bool nfdMaximumCapacity(
            size_t inputCount,
            size_t& outCapacity) noexcept
        {
            constexpr size_t maxExpansion =
                kUnicodeCanonicalDecompositionMaxLength;

            if (inputCount >
                std::numeric_limits<size_t>::max() / maxExpansion)
            {
                return false;
            }

            outCapacity =
                inputCount * maxExpansion;

            return true;
        }

        // ====================================================================
        // nfcMaximumCapacity
        //
        // NFC first produces NFD and then compacts that sequence through
        // canonical composition.
        //
        // Composition never increases sequence length, so the NFD worst-case
        // capacity is also sufficient for NFC.
        // ====================================================================

        [[nodiscard]]
        static bool nfcMaximumCapacity(size_t inputCount, size_t& outCapacity) noexcept {
            return nfdMaximumCapacity(inputCount, outCapacity);
        }

        // ====================================================================
        // nfd - single code point
        //
        // Normalize one Unicode code point to NFD.
        //
        // Recursive decomposition produces at most four code points and
        // canonical ordering does not change that count, so the existing
        // UnicodeCanonicalDecomposition result is sufficient.
        //
        // outResult is modified only on success.
        // ====================================================================

        [[nodiscard]]
        bool nfd(
            uint32_t cp,
            UnicodeCanonicalDecomposition& outResult) const noexcept
        {
            if (!valid())
                return false;


            UnicodeCanonicalDecomposition result;


            if (!mDecomposer.decompose(
                cp,
                result))
            {
                return false;
            }


            if (!mOrdering.order(
                result.codePoints,
                result.count))
            {
                return false;
            }


            outResult = result;

            return true;
        }

        // ====================================================================
// nfc - single code point
//
// Normalize one Unicode code point to NFC.
//
// The code point is first normalized to NFD, then canonically composed
// in place.
//
// NFD produces at most four code points and composition can only
// preserve or reduce that count, so UnicodeCanonicalDecomposition is
// sufficient for the result.
//
// outResult is modified only on success.
// ====================================================================

        [[nodiscard]]
        bool nfc(uint32_t cp, UnicodeCanonicalDecomposition& outResult) const noexcept
        {
            if (!nfcAvailable())
                return false;


            UnicodeCanonicalDecomposition result;

            if (!nfd(cp, result))
                return false;


            size_t count =
                static_cast<size_t>(result.count);


            if (!mComposer.compose(
                result.codePoints,
                count))
            {
                return false;
            }


            result.count =
                static_cast<uint32_t>(count);

            outResult = result;

            return true;
        }

        // ====================================================================
        // nfdLength
        //
        // Determine the exact number of code points produced by NFD for an
        // input sequence.
        //
        // Canonical ordering never changes sequence length, so this requires
        // only recursive decomposition.
        //
        // This function is useful when the caller wants to allocate exactly
        // enough destination storage rather than the 4N worst-case capacity.
        //
        // count == 0 permits input == nullptr.
        //
        // outCount is modified only on success.
        // ====================================================================

        [[nodiscard]]
        bool nfdLength(
            const uint32_t* input,
            size_t inputCount,
            size_t& outCount) const noexcept
        {
            if (!valid())
                return false;


            if (inputCount == 0) {
                outCount = 0;
                return true;
            }


            if (!input)
                return false;


            size_t total = 0;


            for (size_t i = 0; i < inputCount; ++i)
            {
                UnicodeCanonicalDecomposition decomposition;


                if (!mDecomposer.decompose(
                    input[i],
                    decomposition))
                {
                    return false;
                }


                const size_t expansion =
                    static_cast<size_t>(
                        decomposition.count);


                if (total >
                    std::numeric_limits<size_t>::max() - expansion)
                {
                    return false;
                }


                total += expansion;
            }


            outCount = total;

            return true;
        }


        // ====================================================================
        // nfd - sequence
        //
        // Normalize an arbitrary sequence of Unicode code points to NFD.
        //
        // The caller owns the destination buffer.
        //
        // Processing occurs in two passes:
        //
        //      pass 1:
        //
        //          validate every input code point
        //          calculate exact decomposed length
        //          verify destination capacity
        //
        //      pass 2:
        //
        //          recursively decompose each input code point
        //          append each decomposition to one continuous output stream
        //          canonically order the complete stream in place
        //
        //
        // Building one continuous decomposed stream is required because
        // canonical ordering may move combining characters across original
        // source-code-point boundaries.
        //
        //
        // input and output must refer to non-overlapping storage.
        //
        // outCount is modified only on success.
        //
        // count == 0 permits:
        //
        //      input  == nullptr
        //      output == nullptr
        //
        // ====================================================================

        [[nodiscard]]
        bool nfd(
            const uint32_t* input,
            size_t inputCount,
            uint32_t* output,
            size_t outputCapacity,
            size_t& outCount) const noexcept
        {
            if (!valid())
                return false;


            // ---------------------------------------------------------------
            // Empty input.
            // ---------------------------------------------------------------

            if (inputCount == 0) {
                outCount = 0;
                return true;
            }


            if (!input)
                return false;


            // ---------------------------------------------------------------
            // Determine exact output size and completely validate the input
            // before modifying the caller's destination.
            // ---------------------------------------------------------------

            size_t requiredCount = 0;


            if (!nfdLength(
                input,
                inputCount,
                requiredCount))
            {
                return false;
            }


            if (requiredCount > outputCapacity)
                return false;


            if (requiredCount != 0 && !output)
                return false;


            // ---------------------------------------------------------------
            // Decompose the complete input into one continuous output stream.
            // ---------------------------------------------------------------

            size_t outputCount = 0;


            for (size_t i = 0; i < inputCount; ++i)
            {
                UnicodeCanonicalDecomposition decomposition;


                if (!mDecomposer.decompose(
                    input[i],
                    decomposition))
                {
                    return false;
                }


                for (uint32_t j = 0;
                    j < decomposition.count;
                    ++j)
                {
                    output[outputCount++] =
                        decomposition.codePoints[j];
                }
            }


            // ---------------------------------------------------------------
            // Internal consistency.
            //
            // The same immutable database and input sequence were used for
            // both passes, so these values must agree.
            // ---------------------------------------------------------------

            if (outputCount != requiredCount)
                return false;


            // ---------------------------------------------------------------
            // Canonical ordering is performed over the entire decomposed
            // stream, not independently for each source code point.
            // ---------------------------------------------------------------

            if (!mOrdering.order(
                output,
                outputCount))
            {
                return false;
            }


            outCount = outputCount;

            return true;
        }

        // ====================================================================
// nfc - sequence
//
// Normalize an arbitrary sequence of Unicode code points to NFC.
//
// Processing is:
//
//      input
//        |
//        v
//      NFD
//        |
//        v
//      canonical sequence composition
//        |
//        v
//      NFC
//
// The caller owns the destination buffer.
//
// The destination capacity required by NFD is sufficient because
// canonical composition can only preserve or reduce sequence length.
//
// input and output must refer to non-overlapping storage.
//
// outCount is modified only on success.
//
// inputCount == 0 permits:
//
//      input  == nullptr
//      output == nullptr
// ====================================================================

        [[nodiscard]]
        bool nfc(const uint32_t* input, size_t inputCount,
            uint32_t* output, size_t outputCapacity,
            size_t& outCount) const noexcept
        {
            if (!nfcAvailable())
                return false;


            size_t outputCount = 0;


            if (!nfd(
                input,
                inputCount,
                output,
                outputCapacity,
                outputCount))
            {
                return false;
            }


            if (!mComposer.compose(
                output,
                outputCount))
            {
                return false;
            }


            outCount = outputCount;

            return true;
        }

    private:
        UnicodeCanonicalDecomposer mDecomposer{};
        UnicodeCanonicalOrdering mOrdering{};
        UnicodeCanonicalSequenceComposer mComposer{};
    };

} // namespace waavs

