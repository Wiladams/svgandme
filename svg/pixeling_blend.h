// pixeling_blend.h

#pragma once

// blend modes for pixel art
#include "definitions.h"

#include "coloring.h"
#include "surface_info.h"
#include "surface.h"
#include "pixeling_porterduff.h"

namespace waavs
{
    enum WGFilterColorSpace : uint32_t
    {
        WG_FILTER_COLORSPACE_LINEAR_RGB = 0,
        WG_FILTER_COLORSPACE_SRGB = 1
    };
    
    // We need these assertions because these values are used as
    // table indices, and MUST NOT CHANGE
    static_assert(WG_FILTER_COLORSPACE_LINEAR_RGB == 0);
    static_assert(WG_FILTER_COLORSPACE_SRGB == 1);

    // Blend modes for pixel art. 
    // These are designed to be simple and fast, 
    // and to produce results that are visually pleasing for pixel art.
    enum WGBlendMode : uint8_t
    {
        WG_BLEND_NORMAL = 0,
        WG_BLEND_MULTIPLY,
        WG_BLEND_SCREEN,
        WG_BLEND_DARKEN,
        WG_BLEND_LIGHTEN,
        WG_BLEND_OVERLAY,
        WG_BLEND_COLOR_DODGE,
        WG_BLEND_COLOR_BURN,
        WG_BLEND_HARD_LIGHT,
        WG_BLEND_SOFT_LIGHT,
        WG_BLEND_DIFFERENCE,
        WG_BLEND_EXCLUSION,

        WG_BLEND_COUNT
    };

    // ------------------------------------------
    // Pixel Operations for linear RGB blend modes
    // ------------------------------------------
    
    static INLINE float blendop_normal(float cb, float cs) noexcept
    {
        return cs;
    }

    static INLINE float blendop_multiply(float cb, float cs)
    { 
        return cb * cs; 
    }
    
    static INLINE float blendop_screen(float cb, float cs) noexcept
    { 
        return cb + cs - cb * cs; 
    }

    static INLINE float blendop_darken(float cb, float cs) noexcept
    { 
        return (cb < cs) ? cb : cs; 
    }
    
    static INLINE float blendop_lighten(float cb, float cs) noexcept
    { 
        return (cb > cs) ? cb : cs; 
    }

    static INLINE float blendop_overlay(float cb, float cs) noexcept
    {
        return (cb <= 0.5f)
            ? (2.0f * cb * cs)
            : (1.0f - 2.0f * (1.0f - cb) * (1.0f - cs));
    }

    static INLINE float blendop_color_dodge(float cb, float cs) noexcept
    {
        if (cs >= 1.0f) return 1.0f;
        return clamp01f(cb / (1.0f - cs));
    }


    static INLINE float blendop_color_burn(float cb, float cs) noexcept
    {
        if (cs <= 0.0f) return 0.0f;
        return 1.0f - clamp01f((1.0f - cb) / cs);
    }

    static INLINE float blendop_hard_light(float cb, float cs) noexcept
    {
        return (cs <= 0.5f)
            ? (2.0f * cb * cs)
            : (1.0f - 2.0f * (1.0f - cb) * (1.0f - cs));
    }

    static INLINE float blendop_soft_light(float cb, float cs) noexcept
    {
        if (cs <= 0.5f)
            return cb - (1.0f - 2.0f * cs) * cb * (1.0f - cb);
        float d = (cb <= 0.25f)
            ? ((16.0f * cb - 12.0f) * cb + 4.0f) * cb
            : std::sqrt(cb);
        return cb + (2.0f * cs - 1.0f) * (d - cb);
    }

    static INLINE float blendop_difference(float cb, float cs) noexcept
    {
        return std::fabs(cb - cs);
    }

    static INLINE float blendop_exclusion(float cb, float cs) noexcept
    {
        return cb + cs - 2.0f * cb * cs;
    }
    
    // Helper, to decode, apply operation, repack a pixel 
    // 
    // Note: BlendFn is a function that takes two linear RGB values 
    // and returns the blended linear RGB value. Duck typing is 
    // used to allow this to be a lambda or a function pointer or whatever.
    //

    template <typename BlendFn>
    static INLINE Pixel_ARGB32 blend_separable_linear_prgb32_pixel(
        Pixel_ARGB32 backdrop,
        Pixel_ARGB32 source,
        BlendFn blendFn) noexcept
    {
        const uint32_t sa = (source >> 24) & 0xFFu;
        const uint32_t ba = (backdrop >> 24) & 0xFFu;

        if (sa == 0)
            return backdrop;

        if (ba == 0)
            return source;

        const ColorPRGBA bp = coloring_ARGB32_to_prgba(backdrop);
        const ColorPRGBA sp = coloring_ARGB32_to_prgba(source);

        const ColorLinear b = coloring_linear_unpremultiply(bp);
        const ColorLinear s = coloring_linear_unpremultiply(sp);

        const float ab = b.a;
        const float as = s.a;

        const float outA = as + ab * (1.0f - as);
        if (!(outA > 0.0f))
            return 0u;

        const float cb_r = b.r;
        const float cb_g = b.g;
        const float cb_b = b.b;

        const float cs_r = s.r;
        const float cs_g = s.g;
        const float cs_b = s.b;

        const float br = blendFn(cb_r, cs_r);
        const float bg = blendFn(cb_g, cs_g);
        const float bb = blendFn(cb_b, cs_b);

        const float oneMinusAs = 1.0f - as;
        const float oneMinusAb = 1.0f - ab;
        const float abAs = ab * as;

        const float outR_p =
            oneMinusAs * ab * cb_r +
            oneMinusAb * as * cs_r +
            abAs * br;

        const float outG_p =
            oneMinusAs * ab * cb_g +
            oneMinusAb * as * cs_g +
            abAs * bg;

        const float outB_p =
            oneMinusAs * ab * cb_b +
            oneMinusAb * as * cs_b +
            abAs * bb;

        return argb32_from_premultiplied_linear(outA, outR_p, outG_p, outB_p);
    }




    // ------------------------------------------
    // Pixel Operations for sRGB blend modes
    // ------------------------------------------

    template <typename BlendFn>
    static INLINE Pixel_ARGB32 blend_separable_srgb_prgb32_pixel(
        Pixel_ARGB32 backdrop,
        Pixel_ARGB32 source,
        BlendFn blendFn) noexcept
    {
        // Before we get any deeper, we can check for simple 
        // cases where either the source or backdrop is fully transparent
        const uint32_t sa = (source >> 24) & 0xFFu;
        const uint32_t ba = (backdrop >> 24) & 0xFFu;

        if (sa == 0)
            return backdrop;

        if (ba == 0)
            return source;

        const ColorSRGB bp = colorsrgb_from_premultiplied_Pixel_ARGB32(backdrop);
        const ColorSRGB sp = colorsrgb_from_premultiplied_Pixel_ARGB32(source);

        const float ab = bp.a;
        const float as = sp.a;

        const float outA = as + ab * (1.0f - as);
        if (!(outA > 0.0f))
            return 0u;

        const float cb_r = bp.r;
        const float cb_g = bp.g;
        const float cb_b = bp.b;

        const float cs_r = sp.r;
        const float cs_g = sp.g;
        const float cs_b = sp.b;

        const float br = blendFn(cb_r, cs_r);
        const float bg = blendFn(cb_g, cs_g);
        const float bb = blendFn(cb_b, cs_b);

        const float outR_p =
            (1.0f - as) * ab * cb_r +
            (1.0f - ab) * as * cs_r +
            ab * as * br;

        const float outG_p =
            (1.0f - as) * ab * cb_g +
            (1.0f - ab) * as * cs_g +
            ab * as * bg;

        const float outB_p =
            (1.0f - as) * ab * cb_b +
            (1.0f - ab) * as * cs_b +
            ab * as * bb;

        // The value has been pre-multiplied already
        // so just pack it int a pixel and return it
        Pixel_ARGB32 res = argb32_pack_premultiplied_srgb(outA, outR_p, outG_p, outB_p);

        return res;
    }




    // -----------------------------------------
    // Common row function type for blend modes.
    // ------------------------------------------

    using BlendRowFn = void(*)(uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept;

    // ------------------------------------------
    // Generic scalar row wrapper
    // -------------------------------------------
/*
    template <typename PixelOp>
    static INLINE void blend_binary_prgb32_row_scalar(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w,
        PixelOp op) noexcept
    {
        for (int x = 0; x < w; ++x)
            dst[x] = op(backdrop[x], source[x]);
    }
    */

    template <auto BlendOp>
    static INLINE void blend_srgb_prgb32_row_scalar(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        for (int x = 0; x < w; ++x)
            dst[x] = blend_separable_srgb_prgb32_pixel(
                backdrop[x], source[x], BlendOp);
    }

    template <auto BlendOp>
    static INLINE void blend_linear_prgb32_row_scalar(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        for (int x = 0; x < w; ++x)
            dst[x] = blend_separable_linear_prgb32_pixel(
                backdrop[x], source[x], BlendOp);
    }



    // Row helpers

    // ------------------------------------------------------------
    // LINEAR RGB row helpers
    // ------------------------------------------------------------

    static INLINE void blend_normal_linear_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
#if WAAVS_HAS_NEON
        // normal blend is just source-over
        //composite_over_prgb32_row_neon(dst, source, backdrop, w);
        blend_linear_prgb32_row_scalar<blendop_normal>(dst, backdrop, source, w);
#else
        blend_linear_prgb32_row_scalar<blendop_normal>(dst, backdrop, source, w);
#endif
    }



    // ------------------------------------------------------------
    // SRGB row helpers
    // ------------------------------------------------------------

    static INLINE void blend_normal_srgb_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        // normal is still source-over; only the interpretation of
        // blend math changes, and normal has no separable blend math.
#if WAAVS_HAS_NEON
        blend_srgb_prgb32_row_scalar<blendop_normal>(dst, backdrop, source, w);
#else
        blend_srgb_prgb32_row_scalar<blendop_normal>(dst, backdrop, source, w);
#endif
    }







    static INLINE BlendRowFn get_blend_row_fn( WGBlendMode mode,  WGFilterColorSpace cs) noexcept
    {
        if (cs == WG_FILTER_COLORSPACE_SRGB)
        {
            switch (mode)
            {
            case WG_BLEND_NORMAL:      return blend_normal_srgb_prgb32_row;
            case WG_BLEND_MULTIPLY:    return blend_srgb_prgb32_row_scalar<blendop_multiply>;
            case WG_BLEND_SCREEN:      return blend_srgb_prgb32_row_scalar<blendop_screen>;
            case WG_BLEND_DARKEN:      return blend_srgb_prgb32_row_scalar<blendop_darken>;
            case WG_BLEND_LIGHTEN:     return blend_srgb_prgb32_row_scalar<blendop_lighten>;
            case WG_BLEND_OVERLAY:     return blend_srgb_prgb32_row_scalar<blendop_overlay>;
            case WG_BLEND_COLOR_DODGE: return blend_srgb_prgb32_row_scalar<blendop_color_dodge>;
            case WG_BLEND_COLOR_BURN:  return blend_srgb_prgb32_row_scalar<blendop_color_burn>;
            case WG_BLEND_HARD_LIGHT:  return blend_srgb_prgb32_row_scalar<blendop_hard_light>;
            case WG_BLEND_SOFT_LIGHT:  return blend_srgb_prgb32_row_scalar<blendop_soft_light>;
            case WG_BLEND_DIFFERENCE:  return blend_srgb_prgb32_row_scalar<blendop_difference>;
            case WG_BLEND_EXCLUSION:   return blend_srgb_prgb32_row_scalar<blendop_exclusion>;
            default: return nullptr;
            }
        }

        if (cs == WG_FILTER_COLORSPACE_LINEAR_RGB)
        {
            switch (mode)
            {
            case WG_BLEND_NORMAL:      return blend_normal_linear_prgb32_row;
            case WG_BLEND_MULTIPLY:    return blend_linear_prgb32_row_scalar<blendop_multiply>;
            case WG_BLEND_SCREEN:      return blend_linear_prgb32_row_scalar<blendop_screen>;
            case WG_BLEND_DARKEN:      return blend_linear_prgb32_row_scalar<blendop_darken>;
            case WG_BLEND_LIGHTEN:     return blend_linear_prgb32_row_scalar<blendop_lighten>;
            case WG_BLEND_OVERLAY:     return blend_linear_prgb32_row_scalar<blendop_overlay>;
            case WG_BLEND_COLOR_DODGE: return blend_linear_prgb32_row_scalar<blendop_color_dodge>;
            case WG_BLEND_COLOR_BURN:  return blend_linear_prgb32_row_scalar<blendop_color_burn>;
            case WG_BLEND_HARD_LIGHT:  return blend_linear_prgb32_row_scalar<blendop_hard_light>;
            case WG_BLEND_SOFT_LIGHT:  return blend_linear_prgb32_row_scalar<blendop_soft_light>;
            case WG_BLEND_DIFFERENCE:  return blend_linear_prgb32_row_scalar<blendop_difference>;
            case WG_BLEND_EXCLUSION:   return blend_linear_prgb32_row_scalar<blendop_exclusion>;
            default: return nullptr;
            }
        }

        return nullptr;
    }
}