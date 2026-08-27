// unicode_value_table8.h

#pragma once

#include <cassert>
#include <cstdint>

#include "unicode_value_table8_data.h"


namespace waavs
{
    // ========================================================================
    // UnicodeValueTable8Pools
    //
    // Non-owning view of the shared page pools used by UnicodeValueTable8Data.
    //
    // Master-page and value-page references occupy independent 16-bit address
    // spaces.
    //
    // The memory backing these pools must remain valid for the lifetime of
    // every UnicodeValueTable8 that references them.
    //
    // ========================================================================

    struct UnicodeValueTable8Pools
    {
        const UnicodeValueMasterPage8* masterPages{ nullptr };
        const UnicodeValuePage8* valuePages{ nullptr };

        uint32_t masterPageCount{ 0 };
        uint32_t valuePageCount{ 0 };


        [[nodiscard]]
        constexpr bool valid() const noexcept
        {
            return
                (masterPageCount == 0 || masterPages != nullptr) &&
                (valuePageCount == 0 || valuePages != nullptr);
        }


        [[nodiscard]]
        const UnicodeValueMasterPage8& masterPage(
            UnicodeValueMasterPage8Ref ref) const noexcept
        {
            assert(!unicodeValue8RefIsUniform(ref));
            assert(ref < masterPageCount);

            return masterPages[ref];
        }


        [[nodiscard]]
        const UnicodeValuePage8& valuePage(
            UnicodeValuePage8Ref ref) const noexcept
        {
            assert(!unicodeValue8RefIsUniform(ref));
            assert(ref < valuePageCount);

            return valuePages[ref];
        }
    };


    // ========================================================================
    // UnicodeValueTable8
    //
    // Immutable, non-owning runtime view of:
    //
    //      Unicode code point -> uint8_t
    //
    //
    // A table consists of:
    //
    //      UnicodeValueTable8Data
    //
    //          masters[]
    //              |
    //              v
    //      shared UnicodeValueMasterPage8 pool
    //
    //          sub[]
    //              |
    //              v
    //      shared UnicodeValuePage8 pool
    //
    //
    // Uniform master regions and uniform 1024-code-point pages are represented
    // directly by inline references and require no physical page lookup.
    //
    // UnicodeValueTable8 contains no allocated storage and performs no
    // lifetime management. It can refer directly into a validated persistent
    // Unicode database.
    //
    // ========================================================================

    class UnicodeValueTable8
    {
    public:
        constexpr UnicodeValueTable8() noexcept = default;


        constexpr UnicodeValueTable8(
            const UnicodeValueTable8Data* data,
            const UnicodeValueTable8Pools* pools) noexcept
            : mData(data)
            , mPools(pools)
        {
        }


        // ====================================================================
        // Basic state
        // ====================================================================

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept
        {
            return mData != nullptr &&
                mPools != nullptr;
        }


        [[nodiscard]]
        constexpr bool valid() const noexcept
        {
            return
                mData != nullptr &&
                mPools != nullptr &&
                mPools->valid();
        }


        [[nodiscard]]
        constexpr const UnicodeValueTable8Data* data() const noexcept
        {
            return mData;
        }


        [[nodiscard]]
        constexpr const UnicodeValueTable8Pools* pools() const noexcept
        {
            return mPools;
        }


        // ====================================================================
        // value
        //
        // Return the value associated with one Unicode code point.
        //
        // Code points outside the Unicode address space return zero.
        //
        // Lookup has three possible paths:
        //
        //      uniform master:
        //
        //          table -> inline value
        //
        //      uniform sub-page:
        //
        //          table -> master -> inline value
        //
        //      nonuniform sub-page:
        //
        //          table -> master -> value page -> byte
        //
        // ====================================================================

        [[nodiscard]]
        uint8_t value(uint32_t cp) const noexcept
        {
            if (!mData ||
                !mPools ||
                cp >= kUnicodeLimit)
            {
                return 0;
            }


            const uint32_t masterIndex =
                cp >> kUnicodeMasterShift;


            const UnicodeValueMasterPage8Ref masterRef =
                mData->masters[masterIndex];


            // ---------------------------------------------------------------
            // Entire 32768-code-point master region has one value.
            // ---------------------------------------------------------------

            if (unicodeValue8RefIsUniform(masterRef))
            {
                return
                    unicodeValue8RefUniformValue(masterRef);
            }


            assert(masterRef < mPools->masterPageCount);


            const UnicodeValueMasterPage8& master =
                mPools->masterPage(masterRef);


            const uint32_t subIndex =
                (cp >> kUnicodeSubShift) &
                (kUnicodeSubsPerMaster - 1u);


            const UnicodeValuePage8Ref pageRef =
                master.sub[subIndex];


            // ---------------------------------------------------------------
            // Entire 1024-code-point sub-page has one value.
            // ---------------------------------------------------------------

            if (unicodeValue8RefIsUniform(pageRef))
            {
                return
                    unicodeValue8RefUniformValue(pageRef);
            }


            assert(pageRef < mPools->valuePageCount);


            const UnicodeValuePage8& page =
                mPools->valuePage(pageRef);


            const uint32_t valueIndex =
                cp &
                (kUnicodeSubSize - 1u);


            return page.values[valueIndex];
        }


        // ====================================================================
        // operator[]
        //
        // Convenience equivalent of value().
        // ====================================================================

        [[nodiscard]]
        uint8_t operator[](uint32_t cp) const noexcept
        {
            return value(cp);
        }


    private:
        const UnicodeValueTable8Data* mData{ nullptr };
        const UnicodeValueTable8Pools* mPools{ nullptr };
    };


    // ========================================================================
    // Convenience relationships
    //
    // Equality means identity of the backing persistent data and pools.
    // It does not perform semantic comparison of all Unicode values.
    // ========================================================================

    [[nodiscard]]
    inline bool operator==(
        const UnicodeValueTable8& a,
        const UnicodeValueTable8& b) noexcept
    {
        return
            a.data() == b.data() &&
            a.pools() == b.pools();
    }


    [[nodiscard]]
    inline bool operator!=(
        const UnicodeValueTable8& a,
        const UnicodeValueTable8& b) noexcept
    {
        return !(a == b);
    }

} // namespace waavs
