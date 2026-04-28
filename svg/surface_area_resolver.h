// surface_area_resolver.h

#pragma once

#include "surface_info.h"

// --------------------------------
// Surface resolvers
// 
// Determine the right set of surface shapes to be used for a given operation, 
// and retrieve them as views.
// wg_blit_resolve_views() - for offset placement: src placed into dst at dstX, dstY
//  wg_blit_copy
//  wg_blit_composite
//  onOffset
//  dropShadow (final offset)
//  tile ( repeated blits)
//  merge (if using full image src-over)
//
// *wg_surface_resolve_rect_unary()
//  colorMatrix
//  componentTransfer
//  guassian (copy-back)
//  displacement map (map input to output)
//  turbulence (output rect)
//  flood (fill rect)
//
// wg_surface_resolve_rect_binary()  dst(area) = f(a(area), b(area))
//  blend
//  composite arithmetic
//  composite (normal ops)
//
// *wg_surface_resolve_rect_dst()
//  flood
//  turbulence
//  image-generated rect writes
//
namespace waavs
{
    static INLINE WGResult wg_surface_resolve_rect_unary(
        Surface_ARGB32& dst,
        const Surface_ARGB32& src,
        const WGRectI& area,
        Surface_ARGB32& dstView,
        Surface_ARGB32& srcView) noexcept
    {
        if (!dst.data || dst.width <= 0 || dst.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (!src.data || src.width <= 0 || src.height <= 0)
            return WG_ERROR_Invalid_Argument;

        WGRectI clipped = intersection(area, Surface_ARGB32_bounds(&dst));
        clipped = intersection(clipped, Surface_ARGB32_bounds(&src));

        if (clipped.w <= 0 || clipped.h <= 0)
            return WG_SUCCESS;

        if (Surface_ARGB32_get_subarea(dst, clipped, dstView) != WG_SUCCESS)
            return WG_ERROR_Invalid_Argument;

        if (Surface_ARGB32_get_subarea(src, clipped, srcView) != WG_SUCCESS)
            return WG_ERROR_Invalid_Argument;

        return WG_SUCCESS;
    }


    // Given a source surface and a destination surface, 
// and a destination coordinate for the top-left of the source,
// calculate the corresponding source and destination subareas 
// that would be involved in a pixel transfer operation.
    static INLINE WGResult wg_blit_resolve_views(
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

        WGResult res = Surface_ARGB32_get_subarea(src, srcRect, srcView);
        if (res != WG_SUCCESS)
            return res;

        res = Surface_ARGB32_get_subarea(dst, dstClipped, dstView);
        if (res != WG_SUCCESS)
            return res;

        return WG_SUCCESS;
    }

    /*
    static INLINE WGResult wg_resolve_binary_views(
        const Surface_ARGB32& in1,
        const Surface_ARGB32& in2,
        Surface_ARGB32& dst,
        int dstX,
        int dstY,
        Surface_ARGB32& in1View,
        Surface_ARGB32& in2View,
        Surface_ARGB32& dstView) noexcept
    {
        // Validate inputs
        if (!dst.data || dst.width <= 0 || dst.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (!in1.data || in1.width <= 0 || in1.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (!in2.data || in2.width <= 0 || in2.height <= 0)
            return WG_ERROR_Invalid_Argument;

        // Destination bounds
        const WGRectI dstBounds{ 0, 0, dst.width, dst.height };

        // Full placement of in1 into dst
        const WGRectI dstPlacement{ dstX, dstY, in1.width, in1.height };

        // Clip against destination
        const WGRectI dstClipped = intersection(dstPlacement, dstBounds);
        if (dstClipped.w <= 0 || dstClipped.h <= 0)
            return WG_SUCCESS;

        // Corresponding rect in in1
        const WGRectI in1Rect{
            dstClipped.x - dstPlacement.x,
            dstClipped.y - dstPlacement.y,
            dstClipped.w,
            dstClipped.h
        };

        // Corresponding rect in in2 (same mapping as in1)
        const WGRectI in2Rect{
            dstClipped.x - dstPlacement.x,
            dstClipped.y - dstPlacement.y,
            dstClipped.w,
            dstClipped.h
        };

        WGResult res = Surface_ARGB32_get_subarea(in1, in1Rect, in1View);
        if (res != WG_SUCCESS)
            return res;

        res = Surface_ARGB32_get_subarea(in2, in2Rect, in2View);
        if (res != WG_SUCCESS)
            return res;

        res = Surface_ARGB32_get_subarea(dst, dstClipped, dstView);
        if (res != WG_SUCCESS)
            return res;

        return WG_SUCCESS;
    }
    */

    static INLINE WGResult wg_surface_resolve_rect_binary(
        Surface_ARGB32& dst,
        const Surface_ARGB32& a,
        const Surface_ARGB32& b,
        const WGRectI& area,
        Surface_ARGB32& dstView,
        Surface_ARGB32& aView,
        Surface_ARGB32& bView) noexcept
    {
        if (!dst.data || dst.width <= 0 || dst.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (!a.data || a.width <= 0 || a.height <= 0)
            return WG_ERROR_Invalid_Argument;

        if (!b.data || b.width <= 0 || b.height <= 0)
            return WG_ERROR_Invalid_Argument;

        WGRectI clipped = intersection(area, Surface_ARGB32_bounds(&dst));
        clipped = intersection(clipped, Surface_ARGB32_bounds(&a));
        clipped = intersection(clipped, Surface_ARGB32_bounds(&b));

        if (clipped.w <= 0 || clipped.h <= 0)
            return WG_SUCCESS;

        if (Surface_ARGB32_get_subarea(dst, clipped, dstView) != WG_SUCCESS)
            return WG_ERROR_Invalid_Argument;

        if (Surface_ARGB32_get_subarea(a, clipped, aView) != WG_SUCCESS)
            return WG_ERROR_Invalid_Argument;

        if (Surface_ARGB32_get_subarea(b, clipped, bView) != WG_SUCCESS)
            return WG_ERROR_Invalid_Argument;

        return WG_SUCCESS;
    }

}