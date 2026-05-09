#pragma once

#include "pixeling.h"
#include "surface.h"
#include "surface_traversal.h"

namespace waavs
{

    // Apply the mask opacity to a pixel
    static INLINE Pixel_ARGB32 prgb32_apply_clip_coverage( Pixel_ARGB32 p, uint32_t coverage) noexcept
    {
        uint32_t a;
        uint32_t r;
        uint32_t g;
        uint32_t b;

        argb32_unpack_u32(p, a, r, g, b);

        a = mul255_round_u8(a, coverage);
        r = mul255_round_u8(r, coverage);
        g = mul255_round_u8(g, coverage);
        b = mul255_round_u8(b, coverage);

        return argb32_pack_u32(a, r, g, b);
    }

    // mask a span using supplied alpha
    static INLINE void wg_hspan_clip_PRGB32(
        Pixel_ARGB32* dst,
        const Pixel_ARGB32* clip,
        int w) noexcept
    {
        for (int x = 0; x < w; ++x)
        {
            const uint32_t coverage = argb32_unpack_alpha_u32(clip[x]);
            //if (coverage > 0)
            //    printf("c: %04d", coverage);

            dst[x] = prgb32_apply_clip_coverage(dst[x], coverage);
        }
    }


    static INLINE WGResult wg_surface_clip_unchecked(
        Surface_ARGB32& dstView,
        const Surface_ARGB32& maskView ) noexcept
    {
        if (dstView.width <= 0 || dstView.height <= 0)
            return WG_SUCCESS;


        return wg_surface_rows_apply_unary_unchecked( dstView, maskView,
            wg_hspan_clip_PRGB32);
    }

    static INLINE WGResult wg_surface_clip(
        Surface_ARGB32& dst,
        const Surface_ARGB32& clip) noexcept
    {
        if (!dst.data || !clip.data)
            return WG_ERROR_Invalid_Argument;

        if (dst.width != clip.width ||
            dst.height != clip.height)
        {
            return WG_ERROR_Invalid_Argument;
        }

        return wg_surface_clip_unchecked( dst, clip);
    }

}

