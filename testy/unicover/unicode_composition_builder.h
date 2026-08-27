// unicode_composition_builder.h

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "unicode_composition_data.h"
#include "unicode_coverage_builder.h"


namespace waavs
{
    // ========================================================================
    // UnicodeCompositionBuildError
    // ========================================================================

    enum class UnicodeCompositionBuildError : uint8_t
    {
        None = 0,

        InvalidCodePoint,
        DuplicatePair,
        ConflictingPair
    };


    // ========================================================================
    // UnicodeCompositionBuildResult
    //
    // candidatePairCount:
    //      Number of valid direct canonical pair mappings submitted to the
    //      builder.
    //
    // excludedPairCount:
    //      Number discarded because the composite code point belongs to
    //      Full_Composition_Exclusion.
    //
    // compositionPairCount:
    //      Number emitted into the final composition table.
    //
    //
    // On DuplicatePair or ConflictingPair:
    //
    //      first
    //      second
    //      existingComposite
    //      incomingComposite
    //
    // identify the offending pair.
    // ========================================================================

    struct UnicodeCompositionBuildResult
    {
        UnicodeCompositionBuildError error{
            UnicodeCompositionBuildError::None };

        size_t candidatePairCount{ 0 };
        size_t excludedPairCount{ 0 };
        size_t compositionPairCount{ 0 };

        uint32_t first{ 0 };
        uint32_t second{ 0 };
        uint32_t existingComposite{ 0 };
        uint32_t incomingComposite{ 0 };


        [[nodiscard]]
        bool success() const noexcept {
            return error == UnicodeCompositionBuildError::None;
        }


        explicit operator bool() const noexcept {
            return success();
        }
    };


    // ========================================================================
    // Error text
    // ========================================================================

    static inline const char* unicodeCompositionBuildErrorString(
        UnicodeCompositionBuildError error) noexcept
    {
        switch (error)
        {
        case UnicodeCompositionBuildError::None:
            return "no error";

        case UnicodeCompositionBuildError::InvalidCodePoint:
            return "composition mapping contains an invalid Unicode code point";

        case UnicodeCompositionBuildError::DuplicatePair:
            return "duplicate canonical composition pair";

        case UnicodeCompositionBuildError::ConflictingPair:
            return "canonical composition pair maps to multiple composites";
        }

        return "unknown Unicode composition builder error";
    }


    // ========================================================================
    // UnicodeCompositionBuilder
    //
    // Generator-side construction of canonical composition records.
    //
    //
    // Input:
    //
    //      direct canonical decomposition:
    //
    //          composite -> first second
    //
    //      plus:
    //
    //          Full_Composition_Exclusion
    //
    //
    // Output:
    //
    //      sorted UnicodeCompositionRecord[]
    //
    //          (first, second) -> composite
    //
    //
    // Important:
    //
    //      The input pair must be the DIRECT canonical decomposition from
    //      UnicodeData.txt.
    //
    // It must NOT be the recursively flattened decomposition used by NFD.
    //
    //
    // Example:
    //
    //      U+01FA -> U+00C5 U+0301
    //
    // emits:
    //
    //      U+00C5 + U+0301 -> U+01FA
    //
    // rather than:
    //
    //      U+0041 + U+030A ... etc.
    //
    //
    // Full_Composition_Exclusion is generator-only here. Excluded mappings are
    // simply omitted from the final runtime table.
    //
    //
    // finalize() is transactional with respect to outRecords.
    // ========================================================================

    class UnicodeCompositionBuilder
    {
    public:
        UnicodeCompositionBuilder() = default;


        // ====================================================================
        // clear
        // ====================================================================

        void clear() noexcept
        {
            mRecords.clear();

            mError =
                UnicodeCompositionBuildError::None;

            mCandidatePairCount = 0;
            mExcludedPairCount = 0;

            mErrorFirst = 0;
            mErrorSecond = 0;
            mErrorComposite = 0;
        }


        // ====================================================================
        // reserve
        // ====================================================================

        void reserve(size_t count)
        {
            mRecords.reserve(count);
        }


        // ====================================================================
        // State
        // ====================================================================

        [[nodiscard]]
        bool failed() const noexcept {
            return mError != UnicodeCompositionBuildError::None;
        }


        [[nodiscard]]
        UnicodeCompositionBuildError error() const noexcept {
            return mError;
        }


        [[nodiscard]]
        size_t candidatePairCount() const noexcept {
            return mCandidatePairCount;
        }


        [[nodiscard]]
        size_t excludedPairCount() const noexcept {
            return mExcludedPairCount;
        }


        [[nodiscard]]
        size_t retainedPairCount() const noexcept {
            return mRecords.size();
        }


        [[nodiscard]]
        bool empty() const noexcept {
            return mRecords.empty();
        }


        // ====================================================================
        // addCanonicalPair
        //
        // Add one DIRECT canonical pair mapping:
        //
        //      composite -> first second
        //
        // as the reverse composition:
        //
        //      first + second -> composite
        //
        //
        // If composite is present in Full_Composition_Exclusion, the mapping
        // is intentionally discarded and counted as excluded.
        //
        // Pair uniqueness is checked during finalize(), after sorting.
        //
        // Returns false only for invalid input or if the builder was already
        // placed into a failed state.
        // ====================================================================

        [[nodiscard]]
        bool addCanonicalPair(
            uint32_t composite,
            uint32_t first,
            uint32_t second,
            const UnicodeCoverageBuilder& fullCompositionExclusion)
        {
            if (failed())
                return false;


            if (composite >= kUnicodeLimit ||
                first >= kUnicodeLimit ||
                second >= kUnicodeLimit)
            {
                mError =
                    UnicodeCompositionBuildError::InvalidCodePoint;

                mErrorFirst =
                    first;

                mErrorSecond =
                    second;

                mErrorComposite =
                    composite;

                return false;
            }


            ++mCandidatePairCount;


            // ---------------------------------------------------------------
            // Full_Composition_Exclusion is defined on the composite code
            // point, not on either decomposition component.
            // ---------------------------------------------------------------

            if (fullCompositionExclusion.contains(
                composite))
            {
                ++mExcludedPairCount;
                return true;
            }


            UnicodeCompositionRecord record{};

            record.first =
                first;

            record.second =
                second;

            record.composite =
                composite;


            mRecords.push_back(
                record);


            return true;
        }


        // ====================================================================
        // finalize
        //
        // Produce canonical composition records ordered by:
        //
        //      first
        //      second
        //
        //
        // Duplicate pair:
        //
        //      A + B -> X
        //      A + B -> X
        //
        // is rejected.
        //
        //
        // Conflicting pair:
        //
        //      A + B -> X
        //      A + B -> Y
        //
        // is also rejected.
        //
        //
        // outRecords is modified only after complete success.
        // ====================================================================

        [[nodiscard]]
        bool finalize(
            std::vector<UnicodeCompositionRecord>& outRecords,
            UnicodeCompositionBuildResult& outResult) const
        {
            outResult = {};

            outResult.candidatePairCount =
                mCandidatePairCount;

            outResult.excludedPairCount =
                mExcludedPairCount;


            // ---------------------------------------------------------------
            // Propagate an earlier input failure.
            // ---------------------------------------------------------------

            if (failed())
            {
                outResult.error =
                    mError;

                outResult.first =
                    mErrorFirst;

                outResult.second =
                    mErrorSecond;

                outResult.incomingComposite =
                    mErrorComposite;

                return false;
            }


            // ---------------------------------------------------------------
            // Work on a local copy so outRecords remains unchanged on failure.
            // ---------------------------------------------------------------

            std::vector<UnicodeCompositionRecord> records =
                mRecords;


            // ---------------------------------------------------------------
            // Canonical persistent order.
            // ---------------------------------------------------------------

            std::sort(
                records.begin(),
                records.end(),
                [](const UnicodeCompositionRecord& a,
                    const UnicodeCompositionRecord& b) noexcept
                {
                    if (a.first != b.first)
                        return a.first < b.first;

                    return a.second < b.second;
                });


            // ---------------------------------------------------------------
            // Validate pair uniqueness.
            // ---------------------------------------------------------------

            for (size_t i = 1;
                i < records.size();
                ++i)
            {
                const UnicodeCompositionRecord& previous =
                    records[i - 1];

                const UnicodeCompositionRecord& current =
                    records[i];


                if (previous.first != current.first ||
                    previous.second != current.second)
                {
                    continue;
                }


                outResult.first =
                    current.first;

                outResult.second =
                    current.second;

                outResult.existingComposite =
                    previous.composite;

                outResult.incomingComposite =
                    current.composite;


                if (previous.composite ==
                    current.composite)
                {
                    outResult.error =
                        UnicodeCompositionBuildError::DuplicatePair;
                }
                else
                {
                    outResult.error =
                        UnicodeCompositionBuildError::ConflictingPair;
                }


                return false;
            }


            // ---------------------------------------------------------------
            // Accounting consistency.
            // ---------------------------------------------------------------

            if (mCandidatePairCount <
                mExcludedPairCount)
            {
                outResult.error =
                    UnicodeCompositionBuildError::ConflictingPair;

                return false;
            }


            if (mCandidatePairCount -
                mExcludedPairCount !=
                records.size())
            {
                outResult.error =
                    UnicodeCompositionBuildError::ConflictingPair;

                return false;
            }


            // ---------------------------------------------------------------
            // Commit.
            // ---------------------------------------------------------------

            outResult.compositionPairCount =
                records.size();

            outRecords =
                std::move(records);


            return true;
        }


        // ====================================================================
        // Convenience overload
        // ====================================================================

        [[nodiscard]]
        bool finalize(
            std::vector<UnicodeCompositionRecord>& outRecords) const
        {
            UnicodeCompositionBuildResult result;

            return
                finalize(
                    outRecords,
                    result);
        }


    private:
        std::vector<UnicodeCompositionRecord> mRecords{};

        UnicodeCompositionBuildError mError{
            UnicodeCompositionBuildError::None };

        size_t mCandidatePairCount{ 0 };
        size_t mExcludedPairCount{ 0 };

        uint32_t mErrorFirst{ 0 };
        uint32_t mErrorSecond{ 0 };
        uint32_t mErrorComposite{ 0 };
    };

} // namespace waavs
