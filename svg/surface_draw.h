// surface_draw.h

#pragma once

#include "surface_info.h"
#include "surface_traversal.h"
#include "surface_area_resolver.h"

#include "pixeling.h"
#include "pixeling_porterduff.h"
#include "pixeling_blend.h"

#include "membuff.h" // for memset_u32

//
// Drawing routines for Surface_ARGB32.  These are all implemented in terms 
// of the basic surface info and row accessors defined in surface_info.h, 
// and the pixel compositing functions defined in pixeling_porterduff.h.  
// The drawing routines perform clipping and boundary checks, then call 
// into the pixel compositing functions to do the actual work.
//
namespace waavs
{

    // --------------------------------------
    // wg_fill_hspan_unchecked()
    // 
    // Assumes all boundary checks have already been performed by caller.
    // Fills a horizontal span of pixels with a constant premultiplied ARGB color.
    // SRCCOPY semantics: the color is directly copied to the destination, without blending.
    static INLINE void wg_hspan_fill_unchecked(uint32_t* dst, size_t len, Pixel_ARGB32 rgbaPremul) noexcept
    {
        memset_u32((uint32_t*)dst, (int32_t)rgbaPremul, (size_t)len);
    }

    // wg_fill_hspan()
    //
    // Fill a horizontal span of pixels with a constant premultiplied ARGB color.
    // This routine performs boundary checks and clipping, then calls wg_fill_hspan_raw() 
    // to do the actual filling.  Call this interface from higher-level code that may
    // have out-of-bounds coordinates, and let this routine handle the clipping.
    static INLINE WGResult wg_hspan_fill(Surface_ARGB32& dst,
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
        wg_hspan_fill_unchecked(row + x0, clippedLen, rgbaPremul);

        return WG_SUCCESS;
    }

    // --------------------------------------
    // wg_hspan_copy_unchecked()
    //
    static INLINE void wg_hspan_copy_unchecked(
        uint32_t* dst,
        const uint32_t* src,
        size_t count) noexcept
    {
        std::memcpy(dst,src,count * sizeof(Pixel_ARGB32));
    }

    // --------------------------------------
    // wg_hspan_blend_unchecked()
    //
    static INLINE WGResult wg_hspan_blend_unchecked(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int count,
        BlendRowFn rowFn) noexcept
    {
        WAAVS_ASSERT(count >= 0);
        WAAVS_ASSERT(rowFn != nullptr);

        if (count > 0)
            rowFn(dst, backdrop, source, count);

        return WG_SUCCESS;
    }

    // Use the blend mode to figure out which blending function to use
    // then call wg_hspan_blend_unchecked() to do the actual blending.
    static INLINE WGResult wg_hspan_blend_resolve_unchecked(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int count,
        WGBlendMode mode,
        WGFilterColorSpace cs) noexcept
    {
        BlendRowFn rowFn = get_blend_row_fn(mode, cs);
        return wg_hspan_blend_unchecked(
            dst,
            backdrop,
            source,
            count,
            rowFn);
    }


    static INLINE WGResult wg_hspan_composite_unchecked(
        uint32_t* dst,
        const uint32_t* src,
        int count,
        CompositeRowFn rowFn) noexcept
    {
        WAAVS_ASSERT(count >= 0);
        WAAVS_ASSERT(rowFn != nullptr);

        if (count > 0)
            rowFn(dst, src, dst, count);

        return WG_SUCCESS;
    }


    static INLINE WGResult wg_hspan_composite_resolve_unchecked(
        uint32_t* dst,
        const uint32_t* src,
        int count,
        WGCompositeOp op) noexcept
    {
        if (op == WG_COMP_SRC_COPY)
        {
            wg_hspan_copy_unchecked(dst, src, size_t(count));
            return WG_SUCCESS;
        }

        CompositeRowFn rowFn = get_composite_row_fn(op);
        if (!rowFn)
            return WG_ERROR_Invalid_Argument;

        return wg_hspan_composite_unchecked(dst, src, count, rowFn);
    }






    // --------------------------------------
    // Bulk surface operations
    // --------------------------------------



    // -----------------------------------
    // wg_blit_copy_unchecked()
    // 
    // Copy a source surface into a destination surface, 
    // assuming the source and destination views have already been 
    // resolved and clipped by wg_blit_resolve_views(), or equivalent.
    static INLINE WGResult wg_blit_copy_unchecked(
        Surface_ARGB32& dstView,
        const Surface_ARGB32& srcView) noexcept
    {
        if (dstView.width <= 0 || dstView.height <= 0)
            return WG_SUCCESS;

        const size_t countPerRow = size_t(dstView.width);

        if (srcView.contiguous && dstView.contiguous)
        {
            const size_t totalCount =
                countPerRow * size_t(dstView.height);

            wg_hspan_copy_unchecked(
                reinterpret_cast<uint32_t*>(dstView.data),
                reinterpret_cast<const uint32_t*>(srcView.data),
                totalCount);

            return WG_SUCCESS;
        }

        return wg_surface_rows_apply_unary_unchecked(
            dstView,
            srcView,
            [](uint32_t* d, const uint32_t* s, int w) noexcept
            {
                wg_hspan_copy_unchecked(d, s, size_t(w));
            });
    }

    // wg_blit_copy()
    //
    // Copy a source surface into a destination surface at the given destination coordinates.
    // Big caveat: This routine uses memcpy, so if the source and destination regions 
    // overlap, the behavior is undefined.
    static INLINE WGResult wg_blit_copy(
        Surface_ARGB32& dst,
        const Surface_ARGB32& src,
        int dstX,
        int dstY) noexcept
    {
        Surface_ARGB32 srcView{};
        Surface_ARGB32 dstView{};

        WGResult res = wg_blit_resolve_views(src, dst, dstX, dstY, srcView, dstView);
        if (res != WG_SUCCESS)
            return res;

        return wg_blit_copy_unchecked(dstView, srcView);
    }

    // --------------------------------
    // wg_blit_composite_unchecked()
    // 
    // Assumes:
    // - srcView and dstView are valid resolved views
    // - srcView and dstView have matching width/height
    // - bounds/clipping have already been handled
    //
    static INLINE WGResult wg_blit_composite_unchecked(
        Surface_ARGB32& dstView,
        const Surface_ARGB32& srcView,
        WGCompositeOp op) noexcept
    {
        if (dstView.width <= 0 || dstView.height <= 0)
            return WG_SUCCESS;

        if (op == WG_COMP_SRC_COPY)
            return wg_blit_copy_unchecked(dstView, srcView);

        CompositeRowFn rowFn = get_composite_row_fn(op);
        if (!rowFn)
            return WG_ERROR_Invalid_Argument;

        return wg_surface_rows_apply_unary_unchecked(
            dstView,
            srcView,
            [rowFn](uint32_t* d, const uint32_t* s, int w) noexcept
            {
                wg_hspan_composite_unchecked(d, s, w, rowFn);
            });
    }




    // wg_blit_composite()
    // 
    // Perform a pixel transfer applying a composite operation along the way.
    // The composite operation is applied per pixel, so this is not accelerated 
    // like a GPU blit, but it does allow for blending and other compositing operations.
    static INLINE WGResult wg_blit_composite(
        Surface_ARGB32& dst,
        const Surface_ARGB32& src,
        int dstX,
        int dstY,
        WGCompositeOp op) noexcept
    {
        Surface_ARGB32 srcView{};
        Surface_ARGB32 dstView{};

        WGResult res = wg_blit_resolve_views(src, dst, dstX, dstY, srcView, dstView);
        if (res != WG_SUCCESS)
            return res;

        return wg_blit_composite_unchecked(dstView, srcView, op);
    }

    // --------------------------------------
    // wg_blit_blend_unchecked()
    //
    static INLINE WGResult wg_blit_blend_unchecked(
        Surface_ARGB32& dstView,
        const Surface_ARGB32& backdropView,
        const Surface_ARGB32& sourceView,
        WGBlendMode mode,
        WGFilterColorSpace cs) noexcept
    {
        if (dstView.width <= 0 || dstView.height <= 0)
            return WG_SUCCESS;

        BlendRowFn rowFn = get_blend_row_fn(mode, cs);
        if (!rowFn)
            return WG_ERROR_Invalid_Argument;

        uint8_t* dRow = dstView.data;
        const uint8_t* bRow = backdropView.data;
        const uint8_t* sRow = sourceView.data;

        const ptrdiff_t dStride = dstView.stride;
        const ptrdiff_t bStride = backdropView.stride;
        const ptrdiff_t sStride = sourceView.stride;

        const int width = dstView.width;
        const int height = dstView.height;

        for (int y = 0; y < height; ++y)
        {
            wg_hspan_blend_unchecked(
                reinterpret_cast<uint32_t*>(dRow),
                reinterpret_cast<const uint32_t*>(bRow),
                reinterpret_cast<const uint32_t*>(sRow),
                width,
                rowFn);

            dRow += dStride;
            bRow += bStride;
            sRow += sStride;
        }

        return WG_SUCCESS;
    }
    
    static INLINE WGResult wg_blit_blend(
        Surface_ARGB32& dst,
        const Surface_ARGB32& backdrop,
        const Surface_ARGB32& source,
        int dstX,
        int dstY,
        WGBlendMode mode,
        WGFilterColorSpace cs) noexcept
    {
        if (source.width != backdrop.width ||
            source.height != backdrop.height)
            return WG_ERROR_Invalid_Argument;

        Surface_ARGB32 backdropView{};
        Surface_ARGB32 dstView{};

        WGResult res = wg_blit_resolve_views(
            backdrop,
            dst,
            dstX,
            dstY,
            backdropView,
            dstView);

        if (res != WG_SUCCESS)
            return res;

        if (dstView.width <= 0 || dstView.height <= 0)
            return WG_SUCCESS;

        // Because source and backdrop are same-sized and share the same placement,
        // resolve source against the same dst placement.
        Surface_ARGB32 sourceView{};
        Surface_ARGB32 discardDstView{};

        res = wg_blit_resolve_views(
            source,
            dst,
            dstX,
            dstY,
            sourceView,
            discardDstView);

        if (res != WG_SUCCESS)
            return res;

        return wg_blit_blend_unchecked(
            dstView,
            backdropView,
            sourceView,
            mode,
            cs);
    }

    static INLINE WGResult wg_blit_blend_rect(
        Surface_ARGB32& dst,
        const Surface_ARGB32& source,
        const Surface_ARGB32& backdrop,
        const WGRectI& area,
        WGBlendMode mode,
        WGFilterColorSpace cs) noexcept
    {
        Surface_ARGB32 dstView{};
        Surface_ARGB32 backdropView{};
        Surface_ARGB32 sourceView{};

        WGResult res = wg_surface_resolve_rect_binary(
            dst,
            backdrop,
            source,
            area,
            dstView,
            backdropView,
            sourceView);

        if (res != WG_SUCCESS)
            return res;

        if (dstView.width <= 0 || dstView.height <= 0)
            return WG_SUCCESS;

        return wg_blit_blend_unchecked(
            dstView,
            backdropView,
            sourceView,
            mode,
            cs);
    }

    // --------------------------------------
    // wg_surface_fill()
    // 
    // Fill the entire surface with the given pixel value.
    // 
    //
    static INLINE WGResult wg_surface_fill(
        Surface_ARGB32& s,
        Pixel_ARGB32 rgbaPremul) noexcept
    {
        if (!s.data || s.width <= 0 || s.height <= 0)
            return WG_ERROR_Invalid_Argument;

        const size_t countPerRow = size_t(s.width);

        if (s.contiguous)
        {
            WAAVS_ASSERT(s.stride == ptrdiff_t(countPerRow * sizeof(Pixel_ARGB32)));

            uint32_t* p = reinterpret_cast<uint32_t*>(s.data);
            const size_t totalCount = countPerRow * size_t(s.height);

            wg_hspan_fill_unchecked(p, totalCount, rgbaPremul);

            return WG_SUCCESS;
        }

        uint8_t* row = s.data;
        const ptrdiff_t stride = s.stride;
        const int height = s.height;

        for (int y = 0; y < height; ++y)
        {
            uint32_t* p = reinterpret_cast<uint32_t*>(row);
            wg_hspan_fill_unchecked(p, countPerRow, rgbaPremul);
            row += stride;
        }

        return WG_SUCCESS;
    }


    // ---------------------------------------
    // wg_surface_clear()
    //
    // Clear the entire surface to transparent black.
    //
    static INLINE WGResult wg_surface_clear(Surface_ARGB32& s) noexcept
    {
        return wg_surface_fill(s, Pixel_ARGB32(0));
    }



    static INLINE WGResult wg_fill_rect(Surface_ARGB32& s, const WGRectI area, const Pixel_ARGB32 rgbaPremul) noexcept
    {
        // get subarea for the rectangle, then fill it.
        Surface_ARGB32 subarea;
        if (Surface_ARGB32_get_subarea(s, area, subarea) != WG_SUCCESS)
            return WG_SUCCESS;

        return wg_surface_fill(subarea, rgbaPremul);
    }


}

