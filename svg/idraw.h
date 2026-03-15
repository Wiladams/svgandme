#pragma once

#include "definitions.h"
#include "wggeometry.h"
#include "pixeling.h"

namespace waavs
{
    enum WGErrorCode : uint32_t
    {
        WG_SUCCESS = 0,
        WG_ERROR_Invalid_Argument = 100,
        WG_ERROR_Out_Of_Memory,
        WG_ERROR_NotImplemented,
        WG_ERROR_UnknownError
    };

    enum WGBlitFilter : uint32_t
    {
        WG_BLIT_FILTER_Nearest = 0,
        WG_BLIT_FILTER_Bilinear = 1
    };


    // Forward declaration
    // Surface management
    static INLINE uint32_t wg_get_subarea(const Surface_ARGB32& src, const WGRectI& area, Surface_ARGB32& subarea) noexcept;
 
    // Low level pixel ops
    static INLINE uint32_t wg_fill_hspan_raw(uint32_t* dst, int len, Pixel_ARGB32 rgbaPremul) noexcept;
    static INLINE uint32_t wg_fill_hspan(Surface_ARGB32& dst, int y, int x, int len, Pixel_ARGB32 color) noexcept;
    static INLINE uint32_t wg_blend_hspan_mask8(Surface_ARGB32& dst, int y, int x, int len, Pixel_ARGB32 color, const uint8_t* mask) noexcept;


    // Whole surface drawing
    static INLINE uint32_t wg_clear_all(Surface_ARGB32& s) noexcept;
    static INLINE uint32_t wg_fill_all(Surface_ARGB32& s, const Pixel_ARGB32 rgbaPremul) noexcept;
    static INLINE uint32_t wg_blit(Surface_ARGB32& dst, const Surface_ARGB32& src, int dstX, int dstY) noexcept;

    // Utility functions
    static INLINE int wg_clampi(int v, int lo, int hi) noexcept
    {
        return (v < lo) ? lo : ((v > hi) ? hi : v);
    }


    // Implementations
    static INLINE uint32_t wg_get_subarea(const Surface_ARGB32& src, const WGRectI& area, Surface_ARGB32 &subarea) noexcept
    {
        if (!src.data || src.width == 0 || src.height == 0)
            return WG_ERROR_Invalid_Argument;
        if (area.w <= 0 || area.h <= 0)
            return WG_ERROR_Invalid_Argument;
        if (area.x < 0 || area.y < 0 || area.x + area.w > (int)src.width || area.y + area.h > (int)src.height)
            return WG_ERROR_Invalid_Argument;

        // Caller can use the returned subarea to read/write pixels within the specified rectangle.
        // The subarea shares memory with the original surface, so changes will affect the original.
        subarea.data = src.data + (size_t)area.y * (size_t)src.stride + (size_t)area.x * 4;
        subarea.width = area.w;
        subarea.height = area.h;
        subarea.stride = src.stride;
        subarea.contiguous = false; // Subareas are not contiguous by definition.

        return WG_SUCCESS;
    }

    // wg_fill_hspan_raw()
    // 
    // Assumes all boundary checks have already been performed by caller.
    // Fills a horizontal span of pixels with a constant premultiplied ARGB color.
    // SRCCOPY semantics: the color is directly copied to the destination, without blending.
    static INLINE uint32_t wg_fill_hspan_raw(uint32_t * dst, size_t len, Pixel_ARGB32 rgbaPremul) noexcept
    {
        memset_l((uint32_t*)dst, (int32_t)rgbaPremul, (size_t)len);
        return WG_SUCCESS;
    }

    // wg_fill_hspan()
    //
    // Fill a horizontal span of pixels with a constant premultiplied ARGB color.
    // This routine performs boundary checks and clipping, then calls wg_fill_hspan_raw() 
    // to do the actual filling.  Call this interface from higher-level code that may
    // have out-of-bounds coordinates, and let this routine handle the clipping.
    static INLINE uint32_t wg_fill_hspan(Surface_ARGB32& dst,
        int y,
        int x,
        int len,
        Pixel_ARGB32 rgbaPremul) noexcept
    {
        if (!dst.data || dst.width <= 0 || dst.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (len <= 0)
            return WG_ERROR_Invalid_Argument;

        if (y < 0 || y >= dst.height)
            return WG_ERROR_Invalid_Argument;

        int x0 = x;
        int x1 = x + len;

        if (x0 < 0)
            x0 = 0;
        if (x1 > dst.width)
            x1 = dst.width;

        const int clippedLen = x1 - x0;
        if (clippedLen <= 0)
            return WG_SUCCESS;

        uint32_t* row = pixeling_ARGB32_row_ptr(&dst, y);
        wg_fill_hspan_raw(row + x0, clippedLen, rgbaPremul);

        return WG_SUCCESS;

    }

    // --------------------------------------
    // Horizontal blend with 8-bit mask
    // --------------------------------------

    static INLINE uint32_t wg_scale_premul_argb32_u8(uint32_t c, uint32_t m) noexcept
    {
        uint8_t a, r, g, b;
        unpackPARGB32(c, a, r, g, b);

        const uint8_t sa = (uint8_t)wg_div255_u32((uint32_t)a * m);
        const uint8_t sr = (uint8_t)wg_div255_u32((uint32_t)r * m);
        const uint8_t sg = (uint8_t)wg_div255_u32((uint32_t)g * m);
        const uint8_t sb = (uint8_t)wg_div255_u32((uint32_t)b * m);

        return packPARGB32(sa, sr, sg, sb);
    }

    static INLINE uint32_t wg_src_over_premul_argb32(uint32_t src, uint32_t dst) noexcept
    {
        uint8_t sa, sr, sg, sb;
        uint8_t da, dr, dg, db;

        unpackPARGB32(src, sa, sr, sg, sb);
        unpackPARGB32(dst, da, dr, dg, db);

        const uint32_t invA = 255u - (uint32_t)sa;

        const uint8_t oa = (uint8_t)((uint32_t)sa + wg_div255_u32((uint32_t)da * invA));
        const uint8_t or_ = (uint8_t)((uint32_t)sr + wg_div255_u32((uint32_t)dr * invA));
        const uint8_t og = (uint8_t)((uint32_t)sg + wg_div255_u32((uint32_t)dg * invA));
        const uint8_t ob = (uint8_t)((uint32_t)sb + wg_div255_u32((uint32_t)db * invA));

        return packPARGB32(oa, or_, og, ob);
    }

    static INLINE void wg_blend_span_mask8_raw(
        uint32_t* dst,
        const uint8_t* mask,
        int len,
        Pixel_ARGB32 rgbaPremul) noexcept
    {
        const uint32_t srcColor = (uint32_t)rgbaPremul;

        if (len <= 0)
            return;

        uint8_t srcA, srcR, srcG, srcB;
        unpackPARGB32(srcColor, srcA, srcR, srcG, srcB);

        if (srcA == 0)
            return;

        for (int i = 0; i < len; ++i)
        {
            const uint32_t m = mask[i];

            if (m == 0)
                continue;

            if (m == 255)
            {
                dst[i] = wg_src_over_premul_argb32(srcColor, dst[i]);
                continue;
            }

            const uint32_t srcScaled = wg_scale_premul_argb32_u8(srcColor, m);
            dst[i] = wg_src_over_premul_argb32(srcScaled, dst[i]);
        }
    }

    static INLINE uint32_t wg_blend_hspan_mask8(
        Surface_ARGB32& dst,
        int y,
        int x,
        int len,
        Pixel_ARGB32 rgbaPremul,
        const uint8_t* mask) noexcept
    {
        if (!dst.data || dst.width <= 0 || dst.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (!mask)
            return WG_ERROR_Invalid_Argument;

        if (len <= 0)
            return WG_ERROR_Invalid_Argument;

        if (y < 0 || y >= dst.height)
            return WG_ERROR_Invalid_Argument;

        int64_t x0 = x;
        int64_t x1 = x0 + (int64_t)len;

        if (x0 < 0)
            x0 = 0;
        if (x1 > dst.width)
            x1 = dst.width;

        const int clippedLen = (int)(x1 - x0);
        if (clippedLen <= 0)
            return WG_SUCCESS;

        const int maskOffset = (int)(x0 - (int64_t)x);

        uint32_t* row = pixeling_ARGB32_row_ptr(&dst, y);
        wg_blend_span_mask8_raw(
            row + (int)x0,
            mask + maskOffset,
            clippedLen,
            rgbaPremul);

        return WG_SUCCESS;
    }


    // --------------------------------------
    // Bulk surface operations
    // --------------------------------------
    static INLINE uint32_t wg_clear_all(Surface_ARGB32 &s) noexcept
    {
        if (!s.data || s.width <= 0 || s.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (s.contiguous)
        {
            memset(s.data, 0, (size_t)s.stride * (size_t)s.height);
        }
        else {
            for (int row = 0; row < s.height; ++row)
            {
                uint32_t* rPtr = pixeling_ARGB32_row_ptr(&s, row);
                memset(rPtr, 0, s.width * 4);
            }
        }

        return WG_SUCCESS;
    }

    static INLINE uint32_t wg_fill_all(Surface_ARGB32 &s, const Pixel_ARGB32 rgbaPremul) noexcept
    {
        if (!s.data || s.width <= 0 || s.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (s.contiguous)
        {
            uint32_t* rPtr = pixeling_ARGB32_row_ptr(&s, 0);
            memset_l(rPtr, rgbaPremul, s.stride * s.height);
        }
        else {
            for (int row = 0; row < s.height; ++row)
            {
                uint32_t* rPtr = pixeling_ARGB32_row_ptr(&s, row);
                memset_l(rPtr, (int32_t)rgbaPremul, s.width);
            }
        }

        return WG_SUCCESS;
    }

    static INLINE uint32_t wg_fill_rect(Surface_ARGB32& s, const WGRectI area, const Pixel_ARGB32 rgbaPremul) noexcept
    {
        // get subarea for the rectangle, then fill it.
        Surface_ARGB32 subarea;
        if (wg_get_subarea(s, area, subarea) != WG_SUCCESS)
            return WG_ERROR_Invalid_Argument;

        return wg_fill_all(subarea, rgbaPremul);
    }

    // blit()
    //
    // Copy a source surface into a destination surface at the given destination coordinates.
    // Big caveat: This routine uses memcpy, so if the source and destination regions 
    // overlap, the behavior is undefined.
    static INLINE uint32_t wg_blit(Surface_ARGB32& dst, const Surface_ARGB32& src, int dstX, int dstY) noexcept
    {
        // Reject empty source or destination.
        if (!dst.data || dst.width == 0 || dst.height == 0)
            return WG_ERROR_Invalid_Argument;

        if (!src.data || src.width == 0 || src.height == 0)
            return WG_ERROR_Invalid_Argument;

        // Boundary for the destination surface.
        // Source pixels outside this boundary will be ignored
        const WGRectI dstBounds{ 0, 0, (int)dst.width, (int)dst.height };

        // Where the full source would land in destination coordinates.
        const WGRectI dstPlacement{ dstX, dstY, src.width, src.height };

        // Clip that placement against destination bounds.
        const WGRectI dstClipped = intersection(dstPlacement, dstBounds);
        if (dstClipped.w <= 0 || dstClipped.h <= 0)
            return WG_ERROR_Invalid_Argument;

        // Compute corresponding source subregion.
        // If dstPlacement was clipped on the left/top, shift source origin by same amount.
        const WGRectI srcRect{
            dstClipped.x - dstPlacement.x,
            dstClipped.y - dstPlacement.y,
            dstClipped.w,
            dstClipped.h
        };

        Surface_ARGB32 srcView;
        if (wg_get_subarea(src, srcRect, srcView) != WG_SUCCESS)
            return WG_ERROR_Invalid_Argument;


        Surface_ARGB32 dstView;
        if (wg_get_subarea(dst, dstClipped, dstView) != WG_SUCCESS)
            return WG_ERROR_Invalid_Argument;

        // Copy row by row. Since both views have same width/height now,
        // each row is a straight memcpy.
        const size_t rowBytes = dstView.width * 4;

        for (int y = 0; y < dstView.height; ++y)
        {
            uint8_t* dstRow = (uint8_t*)pixeling_ARGB32_row_ptr(&dstView, y);
            const uint8_t* srcRow = (const uint8_t*)pixeling_ARGB32_row_ptr(&srcView, y);
            memcpy(dstRow, srcRow, rowBytes);
        }

        return WG_SUCCESS;
    }





    static INLINE uint32_t wg_sample_nearest_argb32(const Surface_ARGB32& src,  int sx, int sy) noexcept
    {
        sx = wg_clampi(sx, 0, src.width - 1);
        sy = wg_clampi(sy, 0, src.height - 1);

        const uint32_t* row = pixeling_ARGB32_row_ptr((Surface_ARGB32*)&src, sy);
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

        x0 = wg_clampi(x0, 0, src.width - 1);
        y0 = wg_clampi(y0, 0, src.height - 1);
        x1 = wg_clampi(x1, 0, src.width - 1);
        y1 = wg_clampi(y1, 0, src.height - 1);

        const uint32_t* row0 = pixeling_ARGB32_row_ptr_const(&src, y0);
        const uint32_t* row1 = pixeling_ARGB32_row_ptr_const(&src, y1);

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
        WGBlitFilter filter = WG_BLIT_FILTER_Bilinear) noexcept
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

            if (wg_get_subarea(src, srcClip, srcView) != WG_SUCCESS)
                return WG_ERROR_Invalid_Argument;

            if (wg_get_subarea(dst, dstRect, dstView) != WG_SUCCESS)
                return WG_ERROR_Invalid_Argument;

            const size_t rowBytes = (size_t)dstView.width * 4;
            for (int y = 0; y < dstView.height; ++y)
            {
                uint8_t* dstRow = (uint8_t*)pixeling_ARGB32_row_ptr(&dstView, y);
                const uint8_t* srcRow = (const uint8_t*)pixeling_ARGB32_row_ptr(&srcView, y);
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
            uint32_t* dstRow = pixeling_ARGB32_row_ptr(&dst, dstRect.y + dy);
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
        WGBlitFilter filter = WG_BLIT_FILTER_Bilinear) noexcept
    {
        const WGRectI srcRect{ 0, 0, (int)src.width, (int)src.height };
        return wg_scale_blit(dst, dstRect, src, srcRect, filter);
    }
}