// surface_traversal.h
#pragma once

#include "surface_info.h"
#include "filter_types.h"

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

    //
    // PixelNeighborhood_ARGB32
    //
    // This structure is used to help traverse a surface
    // and gather neighborhood information for each pixel.
    //
    struct PixelNeighborhood_ARGB32
    {
        const Surface_ARGB32* src;


        INLINE int width() const noexcept { return src ? src->width : 0; }
        INLINE int height() const noexcept { return src ? src->height : 0; }


        // sample_zero
        INLINE uint32_t sample_zero_unchecked(int x, int y) const noexcept
        {
            const uint32_t* row = Surface_ARGB32_row_pointer_const(src, y);
            return row[x];
        }

        uint32_t sample_zero(int x, int y) const noexcept
        {
            if (!src || !src->data || src->width <= 0 || src->height <= 0)
                return 0u;

            if ((unsigned)x >= (unsigned)src->width ||
                (unsigned)y >= (unsigned)src->height)
                return 0u;

            return sample_zero_unchecked(x, y);
        }

        // sample_clamp
        INLINE uint32_t sample_clamp_unchecked(int x, int y) const noexcept
        {
            const uint32_t* row = Surface_ARGB32_row_pointer_const(src, y);
            return row[x];
        }

        uint32_t sample_clamp(int x, int y) const noexcept
        {
            if (!src || !src->data || src->width <= 0 || src->height <= 0)
                return 0u;

            if (x < 0)
                x = 0;
            else if (x >= src->width)
                x = src->width - 1;

            if (y < 0)
                y = 0;
            else if (y >= src->height)
                y = src->height - 1;

            return sample_clamp_unchecked(x, y);
        }


        // sample_wrap
        //
        INLINE uint32_t sample_wrap_unchecked(int x, int y) const noexcept
        {
            const uint32_t* row = Surface_ARGB32_row_pointer_const(src, y);
            return row[x];
        }

        uint32_t sample_wrap(int x, int y) const noexcept
        {
            if (!src || !src->data || src->width <= 0 || src->height <= 0)
                return 0u;

            const int w = src->width;
            const int h = src->height;

            x %= w;
            y %= h;

            if (x < 0)
                x += w;

            if (y < 0)
                y += h;

            return sample_wrap_unchecked(x, y);
        }



        // consumer focused sampling
        INLINE uint32_t center(int x, int y) const noexcept
        {
            return sample_clamp(x, y);
        }

        INLINE uint32_t left(int x, int y) const noexcept
        {
            return sample_clamp(x - 1, y);
        }

        INLINE uint32_t right(int x, int y) const noexcept
        {
            return sample_clamp(x + 1, y);
        }

        INLINE uint32_t up(int x, int y) const noexcept
        {
            return sample_clamp(x, y - 1);
        }

        INLINE uint32_t down(int x, int y) const noexcept
        {
            return sample_clamp(x, y + 1);
        }

        INLINE uint32_t upLeft(int x, int y) const noexcept
        {
            return sample_clamp(x - 1, y - 1);
        }

        INLINE uint32_t upRight(int x, int y) const noexcept
        {
            return sample_clamp(x + 1, y - 1);
        }

        INLINE uint32_t downLeft(int x, int y) const noexcept
        {
            return sample_clamp(x - 1, y + 1);
        }

        INLINE uint32_t downRight(int x, int y) const noexcept
        {
            return sample_clamp(x + 1, y + 1);
        }
    };

    static INLINE uint32_t sample_edge_mode(
        const PixelNeighborhood_ARGB32& nb,
        int x,
        int y,
        FilterEdgeMode edgeMode) noexcept
    {
        switch (edgeMode)
        {
        case FILTER_EDGE_NONE:
            return nb.sample_zero(x, y);

        case FILTER_EDGE_WRAP:
            return nb.sample_wrap(x, y);

        case FILTER_EDGE_DUPLICATE:
        default:
            return nb.sample_clamp(x, y);
        }
    }

    template <typename PixelFn>
    static INLINE WGResult wg_surface_rows_apply_neighborhood_unchecked(
        Surface_ARGB32& dstView,
        const Surface_ARGB32& srcFull,
        int baseX,
        int baseY,
        PixelFn pixelFn) noexcept
    {
        PixelNeighborhood_ARGB32 nb{ &srcFull };

        return wg_surface_rows_apply_unchecked(
            dstView,
            [&](uint32_t* drow, int w, int yLocal) noexcept
            {
                const int y = baseY + yLocal;

                for (int xLocal = 0; xLocal < w; ++xLocal)
                {
                    const int x = baseX + xLocal;
                    drow[xLocal] = pixelFn(nb, x, y);
                }
            });
    }
}


