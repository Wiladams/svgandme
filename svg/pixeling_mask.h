#pragma once

#include "pixeling.h"
#include "surface.h"
#include "surface_traversal.h"

namespace waavs
{
    static INLINE uint32_t mask_coverage_alpha_PRGB32(Pixel_ARGB32 m) noexcept
    {
        return argb32_unpack_alpha_u32(m);
    }

    // Compute the luminance value of a pixel
    static INLINE uint32_t mask_coverage_luminance_PRGB32(Pixel_ARGB32 m) noexcept
    {
        uint8_t a;
        uint8_t r;
        uint8_t g;
        uint8_t b;

        // Mask surface is PRGB32, so recover straight sRGB RGB first.
        argb32_unpack_unpremul_u8(m, a, r, g, b);

        if (a == 0)
            return 0;

        // Rec.709 / sRGB luminance coefficients, scaled by 65536.
        const uint32_t lum =
            (13933u * uint32_t(r) +
                46871u * uint32_t(g) +
                4732u * uint32_t(b) +
                32768u) >> 16;

        // SVG luminance mask coverage is luminance * alpha.
        return mul255_round_u8(uint32_t(a), lum);
    }

    // Apply the mask opacity to a pixel
    static INLINE Pixel_ARGB32 prgb32_apply_mask_coverage(
        Pixel_ARGB32 p,
        uint32_t coverage) noexcept
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
    static INLINE void wg_hspan_mask_alpha_PRGB32(
        Pixel_ARGB32* dst,
        const Pixel_ARGB32* msk,
        int w) noexcept
    {
        for (int x = 0; x < w; ++x)
        {
            const uint32_t coverage = mask_coverage_alpha_PRGB32(msk[x]);
            dst[x] = prgb32_apply_mask_coverage(dst[x], coverage);
        }
    }

    // Mask a span using luminance
    static INLINE void wg_hspan_mask_luminance_PRGB32(
        Pixel_ARGB32* dst,
        const Pixel_ARGB32* msk,
        int w) noexcept
    {
        for (int x = 0; x < w; ++x)
        {
            const uint32_t coverage = mask_coverage_luminance_PRGB32(msk[x]);
            dst[x] = prgb32_apply_mask_coverage(dst[x], coverage);
        }
    }



    static INLINE WGResult wg_surface_mask_unchecked(
        Surface_ARGB32& dstView,
        const Surface_ARGB32& maskView,
        MaskTypeKind maskType) noexcept
    {
        if (dstView.width <= 0 || dstView.height <= 0)
            return WG_SUCCESS;

        switch (maskType)
        {
        case MaskTypeKind::MASKTYPE_ALPHA:
            return wg_surface_rows_apply_unary_unchecked(
                dstView,
                maskView,
                wg_hspan_mask_alpha_PRGB32);

        case MaskTypeKind::MASKTYPE_LUMINANCE:
        default:
            return wg_surface_rows_apply_unary_unchecked(
                dstView,
                maskView,
                wg_hspan_mask_luminance_PRGB32);
        }
    }

    static INLINE WGResult wg_surface_mask(
        Surface_ARGB32& dst,
        const Surface_ARGB32& mask,
        MaskTypeKind maskType) noexcept
    {
        if (!dst.data || !mask.data)
            return WG_ERROR_Invalid_Argument;

        if (dst.width != mask.width ||
            dst.height != mask.height)
        {
            return WG_ERROR_Invalid_Argument;
        }

        return wg_surface_mask_unchecked(
            dst,
            mask,
            maskType);
    }
    
}
