// unicode_value_table8_builder.h

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

#include "unicode_value_page_pool_builder8.h"
#include "unicode_value_table8_data.h"


namespace waavs
{
    // ========================================================================
    // UnicodeValueTable8Builder
    //
    // Mutable generator-side representation of:
    //
    //      Unicode code point -> uint8_t
    //
    // The construction representation is deliberately dense:
    //
    //      1 byte per Unicode code point
    //      1,114,112 bytes total
    //
    // This keeps generator-side construction simple:
    //
    //      - direct code-point lookup
    //      - simple range assignment
    //      - no temporary sparse hierarchy
    //      - no page-allocation bookkeeping
    //
    // finalize() converts this dense mutable representation into the
    // canonical persistent UnicodeValueTable8Data representation using a
    // shared UnicodeValuePagePoolBuilder8.
    //
    // The resulting hierarchy is:
    //
    //      UnicodeValueTable8Data
    //              |
    //              v
    //      shared UnicodeValueMasterPage8 pool
    //              |
    //              v
    //      shared UnicodeValuePage8 pool
    //
    // Uniform pages and uniform master regions are collapsed to inline
    // references by UnicodeValuePagePoolBuilder8.
    //
    // ========================================================================

    class UnicodeValueTable8Builder
    {
    public:
        // ====================================================================
        // Construction
        //
        // Default construction initializes the complete Unicode address space
        // to value zero.
        // ====================================================================

        UnicodeValueTable8Builder() = default;


        explicit UnicodeValueTable8Builder(uint8_t defaultValue)
        {
            clear(defaultValue);
        }


        // ====================================================================
        // clear
        //
        // Set the complete Unicode address space to one value.
        // ====================================================================

        void clear(uint8_t value = 0) noexcept
        {
            mValues.fill(value);
        }


        // ====================================================================
        // set
        //
        // Assign one Unicode code point.
        //
        // Values outside the Unicode code-point address space are ignored.
        // ====================================================================

        void set(uint32_t cp, uint8_t value) noexcept
        {
            if (cp >= kUnicodeLimit)
                return;

            mValues[cp] = value;
        }


        // ====================================================================
        // setRange
        //
        // Assign one value to the inclusive range:
        //
        //      first ... last
        //
        // The range is clipped to the Unicode code-point address space.
        // ====================================================================

        void setRange(uint32_t first, uint32_t last, uint8_t value) noexcept
        {
            if (first > last)
                return;

            if (first >= kUnicodeLimit)
                return;

            if (last >= kUnicodeLimit)
                last = kUnicodeLimit - 1u;


            std::fill(
                mValues.begin() + first,
                mValues.begin() + last + 1u,
                value);
        }


        // ====================================================================
        // value
        //
        // Query the mutable builder directly.
        //
        // Primarily useful during parsing, validation, and testing.
        //
        // Code points outside the Unicode address space return zero.
        // ====================================================================

        [[nodiscard]]
        uint8_t value(uint32_t cp) const noexcept
        {
            if (cp >= kUnicodeLimit)
                return 0;

            return mValues[cp];
        }


        // ====================================================================
        // data
        //
        // Direct generator-side access to the dense value array.
        //
        // This can be useful for validation or bulk analysis.  It is not part
        // of the persistent runtime representation.
        // ====================================================================

        [[nodiscard]]
        const uint8_t* data() const noexcept
        {
            return mValues.data();
        }


        [[nodiscard]]
        uint8_t* data() noexcept
        {
            return mValues.data();
        }


        // ====================================================================
        // size
        // ====================================================================

        [[nodiscard]]
        static constexpr size_t size() noexcept
        {
            return kUnicodeLimit;
        }


        // ====================================================================
        // finalize
        //
        // Convert this mutable dense value table into its canonical persistent
        // representation.
        //
        //
        // For each logical 1024-code-point page:
        //
        //      dense values
        //          |
        //          v
        //      UnicodeValuePage8
        //          |
        //          v
        //      pool.internValuePage()
        //          |
        //          v
        //      UnicodeValuePage8Ref
        //
        //
        // Groups of 32 resulting page references form:
        //
        //      UnicodeValueMasterPage8
        //          |
        //          v
        //      pool.internMasterPage()
        //          |
        //          v
        //      UnicodeValueMasterPage8Ref
        //
        //
        // The 34 master references finally form:
        //
        //      UnicodeValueTable8Data
        //
        //
        // The pool builder is responsible for:
        //
        //      - collapsing uniform value pages
        //      - collapsing uniform master regions
        //      - deduplicating physical value pages
        //      - deduplicating physical master pages
        //
        //
        // outTable is modified only after the complete table has been
        // successfully finalized.
        //
        // As with UnicodeCoverageBuilder, the shared pool itself may contain
        // pages successfully interned before a later failure.
        // ====================================================================

        [[nodiscard]]
        bool finalize(UnicodeValuePagePoolBuilder8& pool,
            UnicodeValueTable8Data& outTable) const
        {
            UnicodeValueTable8Data result{};


            for (uint32_t mi = 0; mi < kUnicodeMasterCount; ++mi)
            {
                UnicodeValueMasterPage8 masterPage{};


                const uint32_t masterStart =
                    mi << kUnicodeMasterShift;


                // -----------------------------------------------------------
                // Finalize the 32 logical value pages belonging to this
                // master region.
                // -----------------------------------------------------------

                for (uint32_t si = 0; si < kUnicodeSubsPerMaster; ++si)
                {
                    const uint32_t subStart =
                        masterStart +
                        (si << kUnicodeSubShift);


                    UnicodeValuePage8 valuePage{};


                    std::memcpy(
                        valuePage.values,
                        mValues.data() + subStart,
                        sizeof(valuePage.values));


                    UnicodeValuePage8Ref pageRef;


                    if (!pool.internValuePage(valuePage, pageRef))
                        return false;


                    masterPage.sub[si] =
                        pageRef;
                }


                // -----------------------------------------------------------
                // Intern the complete master page.
                //
                // If all 32 sub-pages contain the same uniform value, the
                // pool builder collapses the entire master region to an inline
                // uniform reference.
                // -----------------------------------------------------------

                UnicodeValueMasterPage8Ref masterRef;


                if (!pool.internMasterPage(masterPage, masterRef))
                    return false;


                result.masters[mi] =
                    masterRef;
            }


            // ---------------------------------------------------------------
            // Commit only after complete success.
            // ---------------------------------------------------------------

            outTable = result;

            return true;
        }


    private:
        // ====================================================================
        // Dense generator-side storage
        //
        // UnicodeValueTable8Builder is intentionally approximately 1.1 MiB.
        // This is generator-side memory only and is never serialized.
        // ====================================================================

        std::array<uint8_t, kUnicodeLimit> mValues{};
    };

} // namespace waavs

