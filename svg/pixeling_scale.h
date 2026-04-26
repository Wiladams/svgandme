#pragma once

#include "surface_info.h"

namespace waavs
{
    enum WGScaleFilter : uint32_t
    {
        WG_BLIT_FILTER_Nearest = 0,
        WG_BLIT_FILTER_Bilinear = 1
    };

    // -------------------------------------------
    // Some experimental stuff

    static INLINE uint32_t wg_sample_nearest_argb32(const Surface_ARGB32& src, int sx, int sy) noexcept
    {
        sx = clamp(sx, 0, src.width - 1);
        sy = clamp(sy, 0, src.height - 1);

        const uint32_t* row = Surface_ARGB32_row_pointer((Surface_ARGB32*)&src, sy);
        return row[sx];
    }

    static INLINE uint32_t wg_sample_bilinear_argb32_16(const Surface_ARGB32& src, int fx16, int fy16) noexcept
    {
        // fx16/fy16 are 16.16 source coordinates measured in pixel space.
        // We sample around floor(coord), using the next pixel as needed.
        int x0 = fx16 >> 16;
        int y0 = fy16 >> 16;
        int wx = (fx16 & 0xFFFF) >> 8; // 0..255
        int wy = (fy16 & 0xFFFF) >> 8; // 0..255

        int x1 = x0 + 1;
        int y1 = y0 + 1;

        x0 = clamp(x0, 0, src.width - 1);
        y0 = clamp(y0, 0, src.height - 1);
        x1 = clamp(x1, 0, src.width - 1);
        y1 = clamp(y1, 0, src.height - 1);

        const uint32_t* row0 = Surface_ARGB32_row_pointer_const(&src, y0);
        const uint32_t* row1 = Surface_ARGB32_row_pointer_const(&src, y1);

        const uint32_t c00 = row0[x0];
        const uint32_t c10 = row0[x1];
        const uint32_t c01 = row1[x0];
        const uint32_t c11 = row1[x1];

        const uint32_t w00 = (uint32_t)(256 - wx) * (uint32_t)(256 - wy);
        const uint32_t w10 = (uint32_t)(wx) * (uint32_t)(256 - wy);
        const uint32_t w01 = (uint32_t)(256 - wx) * (uint32_t)(wy);
        const uint32_t w11 = (uint32_t)(wx) * (uint32_t)(wy);

        const uint32_t a =
            (((c00 >> 24) & 0xFF) * w00 +
                ((c10 >> 24) & 0xFF) * w10 +
                ((c01 >> 24) & 0xFF) * w01 +
                ((c11 >> 24) & 0xFF) * w11 + 32768) >> 16;

        const uint32_t r =
            (((c00 >> 16) & 0xFF) * w00 +
                ((c10 >> 16) & 0xFF) * w10 +
                ((c01 >> 16) & 0xFF) * w01 +
                ((c11 >> 16) & 0xFF) * w11 + 32768) >> 16;

        const uint32_t g =
            (((c00 >> 8) & 0xFF) * w00 +
                ((c10 >> 8) & 0xFF) * w10 +
                ((c01 >> 8) & 0xFF) * w01 +
                ((c11 >> 8) & 0xFF) * w11 + 32768) >> 16;

        const uint32_t b =
            (((c00) & 0xFF) * w00 +
                ((c10) & 0xFF) * w10 +
                ((c01) & 0xFF) * w01 +
                ((c11) & 0xFF) * w11 + 32768) >> 16;

        return (a << 24) | (r << 16) | (g << 8) | b;
    }




    static INLINE uint32_t wg_scale_blit(
        Surface_ARGB32& dst,
        const WGRectI& dstRectIn,
        const Surface_ARGB32& src,
        const WGRectI& srcRectIn,
        WGScaleFilter filter = WG_BLIT_FILTER_Bilinear) noexcept
    {
        if (!dst.data || dst.width <= 0 || dst.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (!src.data || src.width <= 0 || src.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (dstRectIn.w <= 0 || dstRectIn.h <= 0)
            return WG_ERROR_Invalid_Argument;

        if (srcRectIn.w <= 0 || srcRectIn.h <= 0)
            return WG_ERROR_Invalid_Argument;

        if (srcRectIn.x < 0 || srcRectIn.y < 0 ||
            srcRectIn.x + srcRectIn.w > src.width ||
            srcRectIn.y + srcRectIn.h > src.height)
            return WG_ERROR_Invalid_Argument;

        const WGRectI dstBounds{ 0, 0, (int)dst.width, (int)dst.height };
        const WGRectI dstRect = intersection(dstRectIn, dstBounds);

        if (dstRect.w <= 0 || dstRect.h <= 0)
            return WG_SUCCESS;

        // Fast path: exact copy, same size.
        if (srcRectIn.w == dstRectIn.w &&
            srcRectIn.h == dstRectIn.h)
        {
            Surface_ARGB32 srcView{};
            Surface_ARGB32 dstView{};

            const WGRectI srcClip{
                srcRectIn.x + (dstRect.x - dstRectIn.x),
                srcRectIn.y + (dstRect.y - dstRectIn.y),
                dstRect.w,
                dstRect.h
            };

            if (Surface_ARGB32_get_subarea(src, srcClip, srcView) != WG_SUCCESS)
                return WG_ERROR_Invalid_Argument;

            if (Surface_ARGB32_get_subarea(dst, dstRect, dstView) != WG_SUCCESS)
                return WG_ERROR_Invalid_Argument;

            const size_t rowBytes = (size_t)dstView.width * 4;
            for (int y = 0; y < dstView.height; ++y)
            {
                uint8_t* dstRow = (uint8_t*)Surface_ARGB32_row_pointer(&dstView, y);
                const uint8_t* srcRow = (const uint8_t*)Surface_ARGB32_row_pointer(&srcView, y);
                memcpy(dstRow, srcRow, rowBytes);
            }

            return WG_SUCCESS;
        }

        // Map destination pixel centers to source pixel centers.
        //
        // srcX = srcRect.x + ((dx + 0.5) * srcRect.w / dstRectIn.w) - 0.5
        // srcY = srcRect.y + ((dy + 0.5) * srcRect.h / dstRectIn.h) - 0.5
        //
        // Use 16.16 fixed point.
        const int64_t stepX = ((int64_t)srcRectIn.w << 16) / (int64_t)dstRectIn.w;
        const int64_t stepY = ((int64_t)srcRectIn.h << 16) / (int64_t)dstRectIn.h;

        const int clippedOffsetX = dstRect.x - dstRectIn.x;
        const int clippedOffsetY = dstRect.y - dstRectIn.y;

        const int64_t startX =
            ((int64_t)srcRectIn.x << 16) +
            (((int64_t)clippedOffsetX << 16) + 32768) * (int64_t)srcRectIn.w / (int64_t)dstRectIn.w -
            32768;

        const int64_t startY =
            ((int64_t)srcRectIn.y << 16) +
            (((int64_t)clippedOffsetY << 16) + 32768) * (int64_t)srcRectIn.h / (int64_t)dstRectIn.h -
            32768;

        int64_t fy = startY;

        for (int dy = 0; dy < dstRect.h; ++dy)
        {
            uint32_t* dstRow = Surface_ARGB32_row_pointer(&dst, dstRect.y + dy);
            int64_t fx = startX;

            if (filter == WG_BLIT_FILTER_Nearest)
            {
                // Round to nearest source pixel.
                for (int dx = 0; dx < dstRect.w; ++dx)
                {
                    const int sx = (int)((fx + 32768) >> 16);
                    const int sy = (int)((fy + 32768) >> 16);
                    dstRow[dstRect.x + dx] = wg_sample_nearest_argb32(src, sx, sy);
                    fx += stepX;
                }
            }
            else
            {
                for (int dx = 0; dx < dstRect.w; ++dx)
                {
                    dstRow[dstRect.x + dx] =
                        wg_sample_bilinear_argb32_16(src, (int)fx, (int)fy);
                    fx += stepX;
                }
            }

            fy += stepY;
        }

        return WG_SUCCESS;
    }

    static INLINE uint32_t wg_scale_blit_full(
        Surface_ARGB32& dst,
        const WGRectI& dstRect,
        const Surface_ARGB32& src,
        WGScaleFilter filter = WG_BLIT_FILTER_Bilinear) noexcept
    {
        const WGRectI srcRect{ 0, 0, (int)src.width, (int)src.height };
        return wg_scale_blit(dst, dstRect, src, srcRect, filter);
    }



}
