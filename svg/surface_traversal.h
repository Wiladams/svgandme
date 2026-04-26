// surface_traversal.h
#pragma once

#include "surface_info.h"

namespace waavs
{
    // RowFn:
    //   void rowFn(uint32_t* dst, int w, int y) noexcept;
    template <typename RowFn>
    static INLINE WGResult wg_surface_rows_apply_unchecked(
        Surface_ARGB32& dst,
        RowFn rowFn) noexcept
    {
        if (dst.width <= 0 || dst.height <= 0)
            return WG_SUCCESS;

        uint8_t* dRow = dst.data;
        const ptrdiff_t dStride = dst.stride;

        const int w = dst.width;
        const int h = dst.height;

        for (int y = 0; y < h; ++y)
        {
            rowFn(reinterpret_cast<uint32_t*>(dRow), w, y);
            dRow += dStride;
        }

        return WG_SUCCESS;
    }


    // RowFn:
    //   void rowFn(uint32_t* dst, const uint32_t* src, int w) noexcept;
    template <typename RowFn>
    static INLINE WGResult wg_surface_rows_apply_unary_unchecked(
        Surface_ARGB32& dst,
        const Surface_ARGB32& src,
        RowFn rowFn) noexcept
    {
        if (dst.width <= 0 || dst.height <= 0)
            return WG_SUCCESS;

        uint8_t* dRow = dst.data;
        const uint8_t* sRow = src.data;

        const ptrdiff_t dStride = dst.stride;
        const ptrdiff_t sStride = src.stride;

        const int w = dst.width;
        const int h = dst.height;

        for (int y = 0; y < h; ++y)
        {
            rowFn(
                reinterpret_cast<uint32_t*>(dRow),
                reinterpret_cast<const uint32_t*>(sRow),
                w);

            dRow += dStride;
            sRow += sStride;
        }

        return WG_SUCCESS;
    }


    // RowFn:
    //   void rowFn(uint32_t* dst,
    //              const uint32_t* a,
    //              const uint32_t* b,
    //              int w) noexcept;
    template <typename RowFn>
    static INLINE WGResult wg_surface_rows_apply_binary_unchecked(
        Surface_ARGB32& dst,
        const Surface_ARGB32& a,
        const Surface_ARGB32& b,
        RowFn rowFn) noexcept
    {
        if (dst.width <= 0 || dst.height <= 0)
            return WG_SUCCESS;

        uint8_t* dRow = dst.data;
        const uint8_t* aRow = a.data;
        const uint8_t* bRow = b.data;

        const ptrdiff_t dStride = dst.stride;
        const ptrdiff_t aStride = a.stride;
        const ptrdiff_t bStride = b.stride;

        const int w = dst.width;
        const int h = dst.height;

        for (int y = 0; y < h; ++y)
        {
            rowFn(
                reinterpret_cast<uint32_t*>(dRow),
                reinterpret_cast<const uint32_t*>(aRow),
                reinterpret_cast<const uint32_t*>(bRow),
                w);

            dRow += dStride;
            aRow += aStride;
            bRow += bStride;
        }

        return WG_SUCCESS;
    }


    // RowFn:
    //   void rowFn(uint32_t* dst,
    //              const uint32_t* const* srcRows,
    //              int srcCount,
    //              int w) noexcept;
    //
    // srcRowsScratch must have room for srcCount entries.
    template <typename RowFn>
    static INLINE WGResult wg_surface_rows_apply_multi_unchecked(
        Surface_ARGB32& dst,
        const Surface_ARGB32* const* srcs,
        int srcCount,
        const uint32_t** srcRowsScratch,
        RowFn rowFn) noexcept
    {
        if (dst.width <= 0 || dst.height <= 0)
            return WG_SUCCESS;

        if (!srcs || srcCount < 0 || (srcCount > 0 && !srcRowsScratch))
            return WG_ERROR_Invalid_Argument;

        uint8_t* dRow = dst.data;
        const ptrdiff_t dStride = dst.stride;

        const int w = dst.width;
        const int h = dst.height;

        for (int y = 0; y < h; ++y)
        {
            for (int i = 0; i < srcCount; ++i)
            {
                const Surface_ARGB32& src = *srcs[i];

                srcRowsScratch[i] =
                    reinterpret_cast<const uint32_t*>(
                        src.data + ptrdiff_t(y) * src.stride);
            }

            rowFn(
                reinterpret_cast<uint32_t*>(dRow),
                srcRowsScratch,
                srcCount,
                w);

            dRow += dStride;
        }

        return WG_SUCCESS;
    }
}


