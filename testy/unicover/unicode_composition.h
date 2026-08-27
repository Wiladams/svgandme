// unicode_composition.h

#pragma once

#include <cstddef>
#include <cstdint>

#include "unicode_composition_data.h"
#include "unicode_coverage_data.h"


namespace waavs
{
    // ========================================================================
    // UnicodeComposition
    //
    // Immutable, non-owning runtime view of explicit canonical composition:
    //
    //      first + second -> composite
    //
    // The backing UnicodeCompositionRecord array is stored strictly in
    // lexicographic order by:
    //
    //      first
    //      second
    //
    // allowing direct binary search over persistent database memory.
    //
    // This view represents only explicit canonical composition records stored
    // in the Unicode database.
    //
    // Hangul composition is not represented here. It is handled
    // algorithmically by the canonical-composition layer above this primitive.
    //
    // The backing memory must remain valid for the lifetime of this view.
    // ========================================================================

    class UnicodeComposition
    {
    public:
        constexpr UnicodeComposition() noexcept = default;

        constexpr UnicodeComposition(const UnicodeCompositionRecord* records,
            uint32_t recordCount) noexcept
            : mRecords(records)
            , mRecordCount(recordCount)
        {
        }


        // ====================================================================
        // Basic state
        // ====================================================================

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept {
            return valid();
        }


        [[nodiscard]]
        constexpr bool valid() const noexcept {
            return mRecords != nullptr && mRecordCount != 0;
        }


        [[nodiscard]]
        constexpr bool empty() const noexcept {
            return mRecordCount == 0;
        }


        [[nodiscard]]
        constexpr uint32_t size() const noexcept {
            return mRecordCount;
        }


        [[nodiscard]]
        constexpr const UnicodeCompositionRecord* data() const noexcept {
            return mRecords;
        }


        // ====================================================================
        // recordAt
        //
        // Return one physical composition record by persistent array index.
        //
        // Returns nullptr when index is outside the view.
        // ====================================================================

        [[nodiscard]]
        const UnicodeCompositionRecord* recordAt(uint32_t index) const noexcept
        {
            if (!mRecords || index >= mRecordCount)
                return nullptr;

            return &mRecords[index];
        }


        // ====================================================================
        // record
        //
        // Find the explicit canonical composition record:
        //
        //      first + second -> composite
        //
        // Returns nullptr when:
        //
        //      - this view is unattached
        //      - either input is outside Unicode
        //      - no explicit composition mapping exists
        //
        // The returned pointer refers directly into persistent database memory.
        //
        // Hangul composition is deliberately not performed here.
        // ====================================================================

        [[nodiscard]]
        const UnicodeCompositionRecord* record(uint32_t first,
            uint32_t second) const noexcept
        {
            if (!valid() ||
                first >= kUnicodeLimit ||
                second >= kUnicodeLimit)
            {
                return nullptr;
            }


            uint32_t low = 0;
            uint32_t high = mRecordCount;


            while (low < high)
            {
                const uint32_t middle =
                    low + ((high - low) >> 1);


                const UnicodeCompositionRecord& candidate =
                    mRecords[middle];


                if (candidate.first < first ||
                    (candidate.first == first &&
                        candidate.second < second))
                {
                    low = middle + 1u;
                }
                else
                {
                    high = middle;
                }
            }


            if (low >= mRecordCount)
                return nullptr;


            const UnicodeCompositionRecord& candidate =
                mRecords[low];


            if (candidate.first != first ||
                candidate.second != second)
            {
                return nullptr;
            }


            return &candidate;
        }


        // ====================================================================
        // hasMapping
        // ====================================================================

        [[nodiscard]]
        bool hasMapping(uint32_t first, uint32_t second) const noexcept
        {
            return record(first, second) != nullptr;
        }


        // ====================================================================
        // composite
        //
        // Return the explicit composite for:
        //
        //      first + second
        //
        // kUnicodeCompositionNone means that no explicit canonical composition
        // mapping exists.
        //
        // Hangul composition is deliberately not performed here.
        // ====================================================================

        [[nodiscard]]
        uint32_t composite(uint32_t first, uint32_t second) const noexcept
        {
            const UnicodeCompositionRecord* mapping =
                record(first, second);

            return mapping
                ? mapping->composite
                : kUnicodeCompositionNone;
        }


    private:
        const UnicodeCompositionRecord* mRecords{ nullptr };
        uint32_t mRecordCount{ 0 };
    };


    // ========================================================================
    // Convenience relationships
    //
    // Equality means identity of the backing persistent record array.
    // It does not perform semantic comparison of all mappings.
    // ========================================================================

    [[nodiscard]]
    inline bool operator==(const UnicodeComposition& a,
        const UnicodeComposition& b) noexcept
    {
        return
            a.data() == b.data() &&
            a.size() == b.size();
    }


    [[nodiscard]]
    inline bool operator!=(const UnicodeComposition& a,
        const UnicodeComposition& b) noexcept
    {
        return !(a == b);
    }

} // namespace waavs
