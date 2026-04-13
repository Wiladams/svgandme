// surface.h
#pragma once

#include "membuff.h"
#include "pixeling.h"

namespace waavs
{
    // ----------------------------------------------------
    // Surface type + rows + basic fills/blends for ARGB32 
    // ----------------------------------------------------
    typedef struct {
        uint8_t* data;          // base pointer
        int32_t  width;         // in pixels
        int32_t  height;        // in pixels
        intptr_t stride;        // in bytes between rows
        bool     contiguous;    // whether the memory is contiguous (no gap between rows)
    } Surface_ARGB32;

    // rectangle convenience
    static INLINE WGRectI Surface_ARGB32_bounds(const Surface_ARGB32* s) noexcept
    {
        return WGRectI{ 0, 0, s->width, s->height };
    }

    static  INLINE uint32_t* Surface_ARGB32_row_pointer(const Surface_ARGB32* s, int y) {
        return (uint32_t*)(s->data + ((size_t)y * (size_t)s->stride));
    }

    static  INLINE const uint32_t* Surface_ARGB32_row_pointer_const(const Surface_ARGB32* s, int y) {
        return (const uint32_t*)(s->data + (size_t)y * (size_t)s->stride);
    }
}

namespace waavs
{


    enum WGBlitFilter : uint32_t
    {
        WG_BLIT_FILTER_Nearest = 0,
        WG_BLIT_FILTER_Bilinear = 1
    };


    // Forward declaration
    // Surface management
    static INLINE WGErrorCode wg_get_subarea(const Surface_ARGB32& src, const WGRectI& area, Surface_ARGB32& subarea) noexcept;

    // Low level pixel ops
    static INLINE WGErrorCode wg_fill_hspan_raw(Pixel_ARGB32* dst, int len, Pixel_ARGB32 rgbaPremul) noexcept;
    static INLINE WGErrorCode wg_fill_hspan(Surface_ARGB32& dst, int y, int x, int len, Pixel_ARGB32 color) noexcept;

    // blending
    static INLINE WGErrorCode wg_blend_hspan_mask8(Surface_ARGB32& dst, int y, int x, int len, Pixel_ARGB32 color, const uint8_t* mask) noexcept;


    // Whole surface drawing
    static INLINE WGResult wg_clear_all(Surface_ARGB32& s) noexcept;
    static INLINE WGResult wg_fill_all(Surface_ARGB32& s, const Pixel_ARGB32 rgbaPremul) noexcept;
    static INLINE WGResult wg_blit(Surface_ARGB32& dst, const Surface_ARGB32& src, int dstX, int dstY) noexcept;


    // Implementations

    // wg_get_subarea()
    //
    // Get a subarea view of the given source surface. 
    // The subarea shares memory with the original surface, 
    // so changes to the subarea will affect the original surface.
    // This is a 'view' onto the surface.  Efficient to create and 
    // use with 'whole surface' operations like fill_all() or clear_all(), 
    // or to read/write pixels within the subarea.
    //
    // This routine will perform boundary clipping and return a subarea
    // that actually fits within the source surface.
    //
    static  WGErrorCode wg_get_subarea(const Surface_ARGB32& src, const WGRectI& area, Surface_ARGB32& subarea) noexcept
    {
        if (!src.data || src.width == 0 || src.height == 0)
            return WG_ERROR_Invalid_Argument;
        if (area.w <= 0 || area.h <= 0)
            return WG_ERROR_Invalid_Argument;
        if (area.x < 0 || area.y < 0 || area.x + area.w >(int)src.width || area.y + area.h >(int)src.height)
            return WG_ERROR_Invalid_Argument;


        subarea.data = src.data + (size_t)area.y * (size_t)src.stride + (size_t)area.x * 4;
        subarea.width = area.w;
        subarea.height = area.h;
        subarea.stride = src.stride;
        subarea.contiguous = false; // Subareas might not contiguous if they don't.
        //. cover the whole area

        return WG_SUCCESS;
    }

    // wg_fill_hspan_raw()
    // 
    // Assumes all boundary checks have already been performed by caller.
    // Fills a horizontal span of pixels with a constant premultiplied ARGB color.
    // SRCCOPY semantics: the color is directly copied to the destination, without blending.
    static INLINE WGErrorCode wg_fill_hspan_raw(uint32_t* dst, size_t len, Pixel_ARGB32 rgbaPremul) noexcept
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
    static INLINE WGErrorCode wg_fill_hspan(Surface_ARGB32& dst,
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

        uint32_t* row = Surface_ARGB32_row_pointer(&dst, y);
        wg_fill_hspan_raw(row + x0, clippedLen, rgbaPremul);

        return WG_SUCCESS;

    }

    // --------------------------------------
    // Horizontal blend with 8-bit mask
    // --------------------------------------

    static INLINE uint32_t wg_scale_premul_argb32_u8(uint32_t c, uint32_t m) noexcept
    {
        uint8_t a, r, g, b;
        argb32_unpack_u8(c, a, r, g, b);

        const uint8_t sa = (uint8_t)div0_255_rounded_u32((uint32_t)a * m);
        const uint8_t sr = (uint8_t)div0_255_rounded_u32((uint32_t)r * m);
        const uint8_t sg = (uint8_t)div0_255_rounded_u32((uint32_t)g * m);
        const uint8_t sb = (uint8_t)div0_255_rounded_u32((uint32_t)b * m);

        return argb32_pack_u8(sa, sr, sg, sb);
    }

    static INLINE uint32_t wg_src_over_premul_argb32(uint32_t src, uint32_t dst) noexcept
    {
        uint8_t sa, sr, sg, sb;
        uint8_t da, dr, dg, db;

        argb32_unpack_u8(src, sa, sr, sg, sb);
        argb32_unpack_u8(dst, da, dr, dg, db);

        const uint32_t invA = 255u - (uint32_t)sa;

        const uint8_t oa = (uint8_t)((uint32_t)sa + div0_255_rounded_u32((uint32_t)da * invA));
        const uint8_t or_ = (uint8_t)((uint32_t)sr + div0_255_rounded_u32((uint32_t)dr * invA));
        const uint8_t og = (uint8_t)((uint32_t)sg + div0_255_rounded_u32((uint32_t)dg * invA));
        const uint8_t ob = (uint8_t)((uint32_t)sb + div0_255_rounded_u32((uint32_t)db * invA));

        return argb32_pack_u8(oa, or_, og, ob);
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
        argb32_unpack_u8(srcColor, srcA, srcR, srcG, srcB);

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

    static INLINE WGErrorCode wg_blend_hspan_mask8(
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

        uint32_t* row = Surface_ARGB32_row_pointer(&dst, y);
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
    // Given a source surface and a destination surface, 
    // and a destination coordinate for the top-left of the source,
    // calculate the corresponding source and destination subareas 
    // that would be involved in a blit operation.
    static INLINE WGResult wg_resolve_blit_views(
        const Surface_ARGB32& src,
        Surface_ARGB32& dst,
        int dstX,
        int dstY,
        Surface_ARGB32& srcView,
        Surface_ARGB32& dstView) noexcept
    {
        // Reject empty source or destination.
        if (!dst.data || dst.width <= 0 || dst.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (!src.data || src.width <= 0 || src.height <= 0)
            return WG_ERROR_Invalid_Argument;

        // Destination bounds.
        const WGRectI dstBounds{ 0, 0, dst.width, dst.height };

        // Full source placement in destination coordinates.
        const WGRectI dstPlacement{ dstX, dstY, src.width, src.height };

        // Clip placement against destination bounds.
        const WGRectI dstClipped = intersection(dstPlacement, dstBounds);
        if (dstClipped.w <= 0 || dstClipped.h <= 0)
            return WG_SUCCESS;

        // Compute the corresponding source rectangle.
        // Any clipping on the destination side maps directly back into the source.
        const WGRectI srcRect{
            dstClipped.x - dstPlacement.x,
            dstClipped.y - dstPlacement.y,
            dstClipped.w,
            dstClipped.h
        };

        WGErrorCode res = wg_get_subarea(src, srcRect, srcView);
        if (res != WG_SUCCESS)
            return res;

        res = wg_get_subarea(dst, dstClipped, dstView);
        if (res != WG_SUCCESS)
            return res;

        return WG_SUCCESS;
    }


    static INLINE WGResult wg_clear_all(Surface_ARGB32& s) noexcept
    {
        if (!s.data || s.width <= 0 || s.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (s.contiguous)
        {
            // In this case, we're setting bytes to zero
            // across the whole buffer, so we can do one big memset.
            memset(s.data, 0, (size_t)s.stride * (size_t)s.height);
        }
        else {
            for (int row = 0; row < s.height; ++row)
            {
                uint32_t* rPtr = Surface_ARGB32_row_pointer(&s, row);
                memset(rPtr, 0, s.width * 4);
            }
        }

        return WG_SUCCESS;
    }

    static  WGResult wg_fill_all(Surface_ARGB32& s, const Pixel_ARGB32 rgbaPremul) noexcept
    {
        if (!s.data || s.width <= 0 || s.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (s.contiguous)
        {
            uint32_t* rPtr = Surface_ARGB32_row_pointer(&s, 0);
            memset_l(rPtr, rgbaPremul, s.width * s.height);
        }
        else {
            for (int row = 0; row < s.height; ++row)
            {
                uint32_t* rPtr = Surface_ARGB32_row_pointer(&s, row);
                memset_l(rPtr, (int32_t)rgbaPremul, s.width);
            }
        }

        return WG_SUCCESS;
    }

    static INLINE WGResult wg_fill_rect(Surface_ARGB32& s, const WGRectI area, const Pixel_ARGB32 rgbaPremul) noexcept
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
    static INLINE WGResult wg_blit(Surface_ARGB32& dst, const Surface_ARGB32& src, int dstX, int dstY) noexcept
    {
        Surface_ARGB32 srcView{};
        Surface_ARGB32 dstView{};

        WGResult res = wg_resolve_blit_views(src, dst, dstX, dstY, srcView, dstView);
        if (res != WG_SUCCESS)
            return res;

        // If the clipped intersection is empty, srcView/dstView will still be zero-initialized.
        if (dstView.width <= 0 || dstView.height <= 0)
            return WG_SUCCESS;

        const size_t rowBytes = (size_t)dstView.width * 4u;
        const size_t totalBytes = rowBytes * (size_t)dstView.height;

        if (srcView.contiguous && dstView.contiguous)
        {
            std::memcpy(dstView.data, srcView.data, totalBytes);
            return WG_SUCCESS;
        }

        for (int y = 0; y < dstView.height; ++y)
        {
            uint8_t* dstRow = (uint8_t*)Surface_ARGB32_row_pointer(&dstView, y);
            const uint8_t* srcRow = (const uint8_t*)Surface_ARGB32_row_pointer_const(&srcView, y);
            std::memcpy(dstRow, srcRow, rowBytes);
        }

        return WG_SUCCESS;
    }

/*
    static WGErrorCode wg_blit(Surface_ARGB32& dst, const Surface_ARGB32& src, int dstX, int dstY) noexcept
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
        const size_t rowBytes = dstView.stride;

        for (int y = 0; y < dstView.height; ++y)
        {
            uint8_t* dstRow = (uint8_t*)Surface_ARGB32_row_pointer(&dstView, y);
            const uint8_t* srcRow = (const uint8_t*)Surface_ARGB32_row_pointer(&srcView, y);
            memcpy(dstRow, srcRow, rowBytes);
        }

        return WG_SUCCESS;
    }
*/




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
        WGBlitFilter filter = WG_BLIT_FILTER_Bilinear) noexcept
    {
        const WGRectI srcRect{ 0, 0, (int)src.width, (int)src.height };
        return wg_scale_blit(dst, dstRect, src, srcRect, filter);
    }
}


namespace waavs 
{
    // There are some interesting surface routines in here
    // They are not being used for SVG at the moment, but
    // they might be the basis of future routines.
    // They're basically redundant though.
/*
    // Fill a span with a constant PRGBA color
    static inline void surface_ARGB32_fill_span(uint32_t* dst, int n, const ColorPRGBA c)
    {
        const uint32_t px = pixeling_prgba_pack_ARGB32(c);
        for (int i = 0; i < n; ++i)
            dst[i] = px;
    }
*/

/*
    // Fill a rectangle (clamped) with a constant PRGBA color 
    static inline void surface_ARGB32_fill_rect(Surface_ARGB32* s,
        int x, int y, int w, int h,
        const ColorPRGBA c)
    {
        // get the intersection of the rect with the surface bounds, 
        // and skip if empty
        if (x < 0) { w += x; x = 0; }
        if (y < 0) { h += y; y = 0; }
        if (x + w > s->width)  w = s->width - x;
        if (y + h > s->height) h = s->height - y;
        if (w <= 0 || h <= 0) return;

        // Convert color value to a packed pixel
        // Then fill each row of the rectangle with that pixel value
        const uint32_t px = pixeling_prgba_pack_ARGB32(c);
        for (int j = 0; j < h; ++j) {
            uint32_t* row = Surface_ARGB32_row_pointer(s, y + j) + x;
            for (int i = 0; i < w; ++i) row[i] = px;
        }
    }
    */


    /*
    // Blend (src OVER dst) across a packed ARGB32 span 
    // with a constant PRGBA src 
    static inline void surface_ARGB32_over_span(const ColorPRGBA src, uint32_t* dst, int n)
    {
        for (int i = 0; i < n; ++i) {
            const ColorPRGBA d = pixeling_ARGB32_unpack_prgba(dst[i]);
            const ColorPRGBA o = coloring_prgba_over(src, d);
            dst[i] = pixeling_prgba_pack_ARGB32(o);
        }
    }
    */

    // ----------------------------------------------------
    // Sampling and resampling                             
    // ----------------------------------------------------
/*
    // Bilinear sample from ARGB32 surface (normalized 0..1 UV), returns linear PRGBA
    static inline ColorPRGBA surface_ARGB32_sample_bilinear_prgba(const Surface_ARGB32* s,
        float u, float v)
    {
        // clamp to edge
        if (u < 0.0f) u = 0.0f; else if (u > 1.0f) u = 1.0f;
        if (v < 0.0f) v = 0.0f; else if (v > 1.0f) v = 1.0f;

        const float fx = u * (float)(s->width - 1);
        const float fy = v * (float)(s->height - 1);
        const int x0 = (int)fx, y0 = (int)fy;
        const int x1 = (x0 + 1 < s->width) ? (x0 + 1) : x0;
        const int y1 = (y0 + 1 < s->height) ? (y0 + 1) : y0;
        const float tx = fx - (float)x0, ty = fy - (float)y0;

        const uint32_t* r0 = Surface_ARGB32_row_pointer_const(s, y0);
        const uint32_t* r1 = Surface_ARGB32_row_pointer_const(s, y1);

        const ColorPRGBA c00 = pixeling_ARGB32_unpack_prgba(r0[x0]);
        const ColorPRGBA c10 = pixeling_ARGB32_unpack_prgba(r0[x1]);
        const ColorPRGBA c01 = pixeling_ARGB32_unpack_prgba(r1[x0]);
        const ColorPRGBA c11 = pixeling_ARGB32_unpack_prgba(r1[x1]);

        // lerp horizontally then vertically (premultiplied linear)
        const ColorPRGBA a = coloring_prgba_lerp(c00, c10, tx);
        const ColorPRGBA b = coloring_prgba_lerp(c01, c11, tx);
        return coloring_prgba_lerp(a, b, ty);
    }
    */

    /*
    //
    // 2x box downsample: src ARGB32 -> dst ARGB32 
    // (sizes should be ceil(src/2))
    //
    static inline void surface_ARGB32_downsample2x(const Surface_ARGB32* src,
        Surface_ARGB32* dst)
    {
        for (int y = 0; y < dst->height; ++y) {
            const int sy = y * 2;
            const int sy1 = (sy + 1 < src->height) ? (sy + 1) : sy;

            const uint32_t* r0 = Surface_ARGB32_row_pointer_const(src, sy);
            const uint32_t* r1 = Surface_ARGB32_row_pointer_const(src, sy1);
            uint32_t* rd = Surface_ARGB32_row_pointer(dst, y);

            for (int x = 0; x < dst->width; ++x) {
                const int sx = x * 2;
                const int sx1 = (sx + 1 < src->width) ? (sx + 1) : sx;

                const uint32_t p00 = r0[sx];
                const uint32_t p10 = r0[sx1];
                const uint32_t p01 = r1[sx];
                const uint32_t p11 = r1[sx1];

                const ColorPRGBA c00 = pixeling_ARGB32_unpack_prgba(p00);
                const ColorPRGBA c10 = pixeling_ARGB32_unpack_prgba(p10);
                const ColorPRGBA c01 = pixeling_ARGB32_unpack_prgba(p01);
                const ColorPRGBA c11 = pixeling_ARGB32_unpack_prgba(p11);

                // average premultiplied components 
                const ColorPRGBA m = {
                    0.25f * (c00.r + c10.r + c01.r + c11.r),
                    0.25f * (c00.g + c10.g + c01.g + c11.g),
                    0.25f * (c00.b + c10.b + c01.b + c11.b),
                    0.25f * (c00.a + c10.a + c01.a + c11.a)
                };
                rd[x] = pixeling_prgba_pack_ARGB32(m);
            }
        }
    }
    */
}


namespace waavs
{
    struct Surface
    {
    private:
        // We don't want anything touching this internal data
        // container, only through interfaces.
        // The data buffer is only active when this surface owns
        // the data that has been allocated.
        // The surface can also serve as a wrapper for other storage
        // In those cases, the data buffer will be empty, and this
        // surface is not responsible for allocat/de-allocate
        MemBuff fDataBuffer;   

    public:
        // Storage for the essential information that makes up 
        // the surface (width, height, stride, data pointer)
        Surface_ARGB32 fInfo;



    public:
        Surface() = default;

        Surface(int w, int h)
        {
            (void)reset(w, h);
        }

        bool reset(int w, int h)
        {
            if (w<= 0 || h <= 0)
                return false;

            // clear existing memory
            fDataBuffer.reset();

            fInfo.width = w;
            fInfo.height = h;
            fInfo.stride = w * 4; // ARGB32 is 4 bytes per pixel
            fInfo.contiguous = true;    // rows are contiguous with no padding

            size_t totalBytes = (size_t)fInfo.stride * (size_t)fInfo.height;
            fDataBuffer.resetFromSize(totalBytes);
            fInfo.data = fDataBuffer.begin();
            
            // Start out with a clean slate by default
            //wg_clear_all(fInfo);

            return true;
        }


        Surface_ARGB32 info() const noexcept { return fInfo; }

        // Convenience accessors
        bool isContiguous() const noexcept { return fInfo.contiguous; }

        // Two forms of bounds: integer and double.  The double form is useful for geometry calculations, 
        // and the integer form is useful for pixel loops.
        WGRectI boundsI() const noexcept { return WGRectI{ 0, 0, (int)width(), (int)height() }; }
        WGRectD boundsD() const noexcept { return WGRectD{ 0.0, 0.0, (double)width(), (double)height() }; }

        size_t width() const noexcept { return (size_t)fInfo.width; }
        size_t height() const noexcept { return (size_t)fInfo.height; }
        size_t stride() const noexcept { return (size_t)fInfo.stride; }
        const uint8_t* data() const noexcept { return fInfo.data; }
        uint8_t* data() noexcept { return fInfo.data; }

        const uint32_t* rowPointer(int y) const noexcept { return Surface_ARGB32_row_pointer_const(&fInfo, y); }
        uint32_t* rowPointer(int y) noexcept { return Surface_ARGB32_row_pointer(&fInfo, y); }

        // createFromData
        // 
        // Initialize a surface to wrap externally allocated data.
        // The surface will not take ownership of the data, and will 
        // not attempt to free it.
        // The caller is responsible for ensuring that the data remains valid
        // for the lifetime of the surface, and that the data is properly
        // allocated with the correct stride and format.
        WGResult createFromData(size_t w, size_t h, size_t stride, uint8_t* data)
        {
            if (!data || w <= 0 || h <= 0 || stride < w * 4)
                return WG_ERROR_Invalid_Argument;

            fDataBuffer.reset(); // clear existing memory
            fInfo.width = (int)w;
            fInfo.height = (int)h;
            fInfo.stride = (int)stride;
            fInfo.data = data;
            fInfo.contiguous = (stride == w * 4);

            return WG_SUCCESS;
        }

        // getSubSurface
        //
        // Retrieve a sub-region of this surface as a new surface that wraps the same data.
        WGResult getSubSurface(const WGRectI& r, Surface& out) const noexcept
        {
            WGErrorCode res = wg_get_subarea(fInfo, r, out.fInfo);
            if (res != WG_SUCCESS)
                return res;


            // Wrap as a non-owning proxy with parent stride preserved.
            return out.createFromData(
                (size_t)out.fInfo.width,
                (size_t)out.fInfo.height,
                stride(),
                out.fInfo.data);
        }

        // clearAll()
        // 
        // Set all pixels to a transparent black (0x00000000)
        // This should work, even if the surface is a sub-region 
        // of a larger surface.
        void clearAll() noexcept 
        {
            (void)wg_clear_all(fInfo);
        }
        

        // fillAll()
        //
        // Fill the entire surface with a single premultiplied ARGB color.
        //
        void fillAll(Pixel_ARGB32 rgbaPremul) noexcept
        {
            wg_fill_all(fInfo, rgbaPremul);
        }

        //
        // fillRect
        //
        // Fill a sub-region with a given pre-multiplied pixel value
        // This will handle clipping the rectangle to fit the bounds
        // of the surface.
        void fillRect(const WGRectI& r, Pixel_ARGB32 rgbaPremul) const
        {
            Surface sub;
            if (!getSubSurface(r, sub))
                return;

            wg_fill_all(sub.fInfo, rgbaPremul);
        }

        void fillRect(const WGRectD& r, Pixel_ARGB32 rgbaPremul) const
        {
            int x = (int)waavs::floor(r.x);
            int y = (int)waavs::floor(r.y);
            int w = (int)waavs::ceil(r.x + r.w) - x;
            int h = (int)waavs::ceil(r.y + r.h) - y;

            fillRect(WGRectI{ x, y, w, h }, rgbaPremul);
        }

        // blit()
        //
        // Copy a source surface into this surface at the given destination coordinates.
        // Big caveat: This routine uses memcpy, so if the source and destination regions 
        // overlap, the behavior is undefined.
        void blit(const Surface& src, int dstX, int dstY) noexcept
        {
            (void)wg_blit(fInfo, src.fInfo, dstX, dstY);
        }

    };
}