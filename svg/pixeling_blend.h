// pixeling_blend.h

#pragma once

// blend modes for pixel art
#include "definitions.h"

#include "coloring.h"
#include "surface_info.h"
#include "surface.h"

namespace waavs
{
    // Blend modes for pixel art. These are designed to be simple and fast, 
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
        const ColorPRGBA bp = coloring_ARGB32_to_prgba(backdrop);
        const ColorPRGBA sp = coloring_ARGB32_to_prgba(source);

        const ColorLinear b = coloring_linear_unpremultiply(bp);
        const ColorLinear s = coloring_linear_unpremultiply(sp);

        const float ab = clamp01f(b.a);
        const float as = clamp01f(s.a);

        const float outA = as + ab * (1.0f - as);
        if (!(outA > 0.0f))
            return 0u;

        const float cb_r = clamp01f(b.r);
        const float cb_g = clamp01f(b.g);
        const float cb_b = clamp01f(b.b);

        const float cs_r = clamp01f(s.r);
        const float cs_g = clamp01f(s.g);
        const float cs_b = clamp01f(s.b);

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

        ColorLinear out{};
        out.a = outA;
        out.r = clamp01f(outR_p / outA);
        out.g = clamp01f(outG_p / outA);
        out.b = clamp01f(outB_p / outA);

        return coloring_prgba_to_ARGB32(coloring_linear_premultiply(out));
    }

    static INLINE Pixel_ARGB32 blend_normal_linear_prgb32_pixel(
        Pixel_ARGB32 backdrop, Pixel_ARGB32 source) noexcept
    {
        return composite_over_prgb32_pixel(source, backdrop);
    }

    static INLINE Pixel_ARGB32 blend_multiply_linear_prgb32_pixel(
        Pixel_ARGB32 b, Pixel_ARGB32 s) noexcept
    {
        return blend_separable_linear_prgb32_pixel(b, s,
            [](float cb, float cs) noexcept { return cb * cs; });
    }

    static INLINE Pixel_ARGB32 blend_screen_linear_prgb32_pixel(
        Pixel_ARGB32 b, Pixel_ARGB32 s) noexcept
    {
        return blend_separable_linear_prgb32_pixel(b, s,
            [](float cb, float cs) noexcept { return cb + cs - cb * cs; });
    }

    static INLINE Pixel_ARGB32 blend_darken_linear_prgb32_pixel(
        Pixel_ARGB32 b, Pixel_ARGB32 s) noexcept
    {
        return blend_separable_linear_prgb32_pixel(b, s,
            [](float cb, float cs) noexcept { return (cb < cs) ? cb : cs; });
    }

    static INLINE Pixel_ARGB32 blend_lighten_linear_prgb32_pixel(
        Pixel_ARGB32 b, Pixel_ARGB32 s) noexcept
    {
        return blend_separable_linear_prgb32_pixel(b, s,
            [](float cb, float cs) noexcept { return (cb > cs) ? cb : cs; });
    }

    static INLINE Pixel_ARGB32 blend_overlay_linear_prgb32_pixel(
        Pixel_ARGB32 b, Pixel_ARGB32 s) noexcept
    {
        return blend_separable_linear_prgb32_pixel(b, s,
            [](float cb, float cs) noexcept {
                return (cb <= 0.5f)
                    ? (2.0f * cb * cs)
                    : (1.0f - 2.0f * (1.0f - cb) * (1.0f - cs));
            });
    }

    static INLINE Pixel_ARGB32 blend_color_dodge_linear_prgb32_pixel(
        Pixel_ARGB32 b, Pixel_ARGB32 s) noexcept
    {
        return blend_separable_linear_prgb32_pixel(b, s,
            [](float cb, float cs) noexcept {
                if (cs >= 1.0f) return 1.0f;
                return clamp01f(cb / (1.0f - cs));
            });
    }

    static INLINE Pixel_ARGB32 blend_color_burn_linear_prgb32_pixel(
        Pixel_ARGB32 b, Pixel_ARGB32 s) noexcept
    {
        return blend_separable_linear_prgb32_pixel(b, s,
            [](float cb, float cs) noexcept {
                if (cs <= 0.0f) return 0.0f;
                return 1.0f - clamp01f((1.0f - cb) / cs);
            });
    }

    static INLINE Pixel_ARGB32 blend_hard_light_linear_prgb32_pixel(
        Pixel_ARGB32 b, Pixel_ARGB32 s) noexcept
    {
        return blend_overlay_linear_prgb32_pixel(s, b);
    }

    static INLINE Pixel_ARGB32 blend_soft_light_linear_prgb32_pixel(
        Pixel_ARGB32 b, Pixel_ARGB32 s) noexcept
    {
        return blend_separable_linear_prgb32_pixel(b, s,
            [](float cb, float cs) noexcept {
                if (cs <= 0.5f)
                    return cb - (1.0f - 2.0f * cs) * cb * (1.0f - cb);
                float d = (cb <= 0.25f)
                    ? ((16.0f * cb - 12.0f) * cb + 4.0f) * cb
                    : std::sqrt(cb);
                return cb + (2.0f * cs - 1.0f) * (d - cb);
            });
    }

    static INLINE Pixel_ARGB32 blend_difference_linear_prgb32_pixel(
        Pixel_ARGB32 b, Pixel_ARGB32 s) noexcept
    {
        return blend_separable_linear_prgb32_pixel(b, s,
            [](float cb, float cs) noexcept { return std::fabs(cb - cs); });
    }

    static INLINE Pixel_ARGB32 blend_exclusion_linear_prgb32_pixel(
        Pixel_ARGB32 b, Pixel_ARGB32 s) noexcept
    {
        return blend_separable_linear_prgb32_pixel(b, s,
            [](float cb, float cs) noexcept { return cb + cs - 2.0f * cb * cs; });
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
        const ColorPRGBA bp = coloring_ARGB32_to_prgba(backdrop);
        const ColorPRGBA sp = coloring_ARGB32_to_prgba(source);

        const float ab = clamp01f(bp.a);
        const float as = clamp01f(sp.a);

        const float outA = as + ab * (1.0f - as);
        if (!(outA > 0.0f))
            return 0u;

        const float cb_r = (ab > 0.0f) ? clamp01f(bp.r / ab) : 0.0f;
        const float cb_g = (ab > 0.0f) ? clamp01f(bp.g / ab) : 0.0f;
        const float cb_b = (ab > 0.0f) ? clamp01f(bp.b / ab) : 0.0f;

        const float cs_r = (as > 0.0f) ? clamp01f(sp.r / as) : 0.0f;
        const float cs_g = (as > 0.0f) ? clamp01f(sp.g / as) : 0.0f;
        const float cs_b = (as > 0.0f) ? clamp01f(sp.b / as) : 0.0f;

        const float br = blendFn(cb_r, cs_r);
        const float bg = blendFn(cb_g, cs_g);
        const float bb = blendFn(cb_b, cs_b);

        const float outR_p =
            (1.0f - as) * bp.r +
            (1.0f - ab) * sp.r +
            ab * as * br;

        const float outG_p =
            (1.0f - as) * bp.g +
            (1.0f - ab) * sp.g +
            ab * as * bg;

        const float outB_p =
            (1.0f - as) * bp.b +
            (1.0f - ab) * sp.b +
            ab * as * bb;

        ColorPRGBA out{};
        out.a = outA;
        out.r = clamp01f(outR_p);
        out.g = clamp01f(outG_p);
        out.b = clamp01f(outB_p);

        return coloring_prgba_to_ARGB32(out);
    }

    static INLINE Pixel_ARGB32 blend_normal_srgb_prgb32_pixel(
        Pixel_ARGB32 backdrop,
        Pixel_ARGB32 source) noexcept
    {
        return composite_over_prgb32_pixel(source, backdrop);
    }

    static INLINE Pixel_ARGB32 blend_multiply_srgb_prgb32_pixel(
        Pixel_ARGB32 backdrop,
        Pixel_ARGB32 source) noexcept
    {
        return blend_separable_srgb_prgb32_pixel(
            backdrop,
            source,
            [](float cb, float cs) noexcept
            {
                return cb * cs;
            });
    }

    static INLINE Pixel_ARGB32 blend_screen_srgb_prgb32_pixel(
        Pixel_ARGB32 backdrop,
        Pixel_ARGB32 source) noexcept
    {
        return blend_separable_srgb_prgb32_pixel(
            backdrop,
            source,
            [](float cb, float cs) noexcept
            {
                return cb + cs - cb * cs;
            });
    }

    static INLINE Pixel_ARGB32 blend_darken_srgb_prgb32_pixel(
        Pixel_ARGB32 backdrop,
        Pixel_ARGB32 source) noexcept
    {
        return blend_separable_srgb_prgb32_pixel(
            backdrop,
            source,
            [](float cb, float cs) noexcept
            {
                return (cb < cs) ? cb : cs;
            });
    }

    static INLINE Pixel_ARGB32 blend_lighten_srgb_prgb32_pixel(
        Pixel_ARGB32 backdrop,
        Pixel_ARGB32 source) noexcept
    {
        return blend_separable_srgb_prgb32_pixel(
            backdrop,
            source,
            [](float cb, float cs) noexcept
            {
                return (cb > cs) ? cb : cs;
            });
    }

    static INLINE Pixel_ARGB32 blend_overlay_srgb_prgb32_pixel(
        Pixel_ARGB32 backdrop,
        Pixel_ARGB32 source) noexcept
    {
        return blend_separable_srgb_prgb32_pixel(
            backdrop,
            source,
            [](float cb, float cs) noexcept
            {
                return (cb <= 0.5f)
                    ? (2.0f * cb * cs)
                    : (1.0f - 2.0f * (1.0f - cb) * (1.0f - cs));
            });
    }

    static INLINE Pixel_ARGB32 blend_color_dodge_srgb_prgb32_pixel(
        Pixel_ARGB32 backdrop,
        Pixel_ARGB32 source) noexcept
    {
        return blend_separable_srgb_prgb32_pixel(
            backdrop,
            source,
            [](float cb, float cs) noexcept
            {
                if (cs >= 1.0f)
                    return 1.0f;

                return clamp01f(cb / (1.0f - cs));
            });
    }

    static INLINE Pixel_ARGB32 blend_color_burn_srgb_prgb32_pixel(
        Pixel_ARGB32 backdrop,
        Pixel_ARGB32 source) noexcept
    {
        return blend_separable_srgb_prgb32_pixel(
            backdrop,
            source,
            [](float cb, float cs) noexcept
            {
                if (cs <= 0.0f)
                    return 0.0f;

                return 1.0f - clamp01f((1.0f - cb) / cs);
            });
    }

    static INLINE Pixel_ARGB32 blend_hard_light_srgb_prgb32_pixel(
        Pixel_ARGB32 backdrop,
        Pixel_ARGB32 source) noexcept
    {
        return blend_separable_srgb_prgb32_pixel(
            backdrop,
            source,
            [](float cb, float cs) noexcept
            {
                return (cs <= 0.5f)
                    ? (2.0f * cb * cs)
                    : (1.0f - 2.0f * (1.0f - cb) * (1.0f - cs));
            });
    }

    static INLINE Pixel_ARGB32 blend_soft_light_srgb_prgb32_pixel(
        Pixel_ARGB32 backdrop,
        Pixel_ARGB32 source) noexcept
    {
        return blend_separable_srgb_prgb32_pixel(
            backdrop,
            source,
            [](float cb, float cs) noexcept
            {
                if (cs <= 0.5f)
                {
                    return cb - (1.0f - 2.0f * cs) * cb * (1.0f - cb);
                }

                float d;
                if (cb <= 0.25f)
                    d = ((16.0f * cb - 12.0f) * cb + 4.0f) * cb;
                else
                    d = std::sqrt(cb);

                return cb + (2.0f * cs - 1.0f) * (d - cb);
            });
    }

    static INLINE Pixel_ARGB32 blend_difference_srgb_prgb32_pixel(
        Pixel_ARGB32 backdrop,
        Pixel_ARGB32 source) noexcept
    {
        return blend_separable_srgb_prgb32_pixel(
            backdrop,
            source,
            [](float cb, float cs) noexcept
            {
                return std::fabs(cb - cs);
            });
    }

    static INLINE Pixel_ARGB32 blend_exclusion_srgb_prgb32_pixel(
        Pixel_ARGB32 backdrop,
        Pixel_ARGB32 source) noexcept
    {
        return blend_separable_srgb_prgb32_pixel(
            backdrop,
            source,
            [](float cb, float cs) noexcept
            {
                return cb + cs - 2.0f * cb * cs;
            });
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
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_normal_linear_prgb32_pixel);
#else
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_normal_linear_prgb32_pixel);
#endif
    }

    static INLINE void blend_multiply_linear_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
#if WAAVS_HAS_NEON
        //blend_multiply_linear_prgb32_row_neon(dst, backdrop, source, w);
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_multiply_linear_prgb32_pixel);
#else
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_multiply_linear_prgb32_pixel);
#endif
    }

    static INLINE void blend_screen_linear_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
#if WAAVS_HAS_NEON
        //blend_screen_linear_prgb32_row_neon(dst, backdrop, source, w);
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_screen_linear_prgb32_pixel);
#else
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_screen_linear_prgb32_pixel);
#endif
    }

    static INLINE void blend_darken_linear_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
#if WAAVS_HAS_NEON
        //blend_darken_linear_prgb32_row_neon(dst, backdrop, source, w);
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_darken_linear_prgb32_pixel);
#else
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_darken_linear_prgb32_pixel);
#endif
    }

    static INLINE void blend_lighten_linear_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
#if WAAVS_HAS_NEON
        //blend_lighten_linear_prgb32_row_neon(dst, backdrop, source, w);
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_lighten_linear_prgb32_pixel);
#else
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_lighten_linear_prgb32_pixel);
#endif
    }

    static INLINE void blend_overlay_linear_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_overlay_linear_prgb32_pixel);
    }

    static INLINE void blend_color_dodge_linear_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_color_dodge_linear_prgb32_pixel);
    }

    static INLINE void blend_color_burn_linear_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_color_burn_linear_prgb32_pixel);
    }

    static INLINE void blend_hard_light_linear_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_hard_light_linear_prgb32_pixel);
    }

    static INLINE void blend_soft_light_linear_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_soft_light_linear_prgb32_pixel);
    }

    static INLINE void blend_difference_linear_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
#if WAAVS_HAS_NEON
        //blend_difference_linear_prgb32_row_neon(dst, backdrop, source, w);
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_difference_linear_prgb32_pixel);
#else
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_difference_linear_prgb32_pixel);
#endif
    }

    static INLINE void blend_exclusion_linear_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
#if WAAVS_HAS_NEON
        //blend_exclusion_linear_prgb32_row_neon(dst, backdrop, source, w);
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_exclusion_linear_prgb32_pixel);
#else
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_exclusion_linear_prgb32_pixel);
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
        composite_over_prgb32_row_neon(dst, source, backdrop, w);
#else
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_normal_srgb_prgb32_pixel);
#endif
    }

    static INLINE void blend_multiply_srgb_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_multiply_srgb_prgb32_pixel);
    }

    static INLINE void blend_screen_srgb_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_screen_srgb_prgb32_pixel);
    }

    static INLINE void blend_darken_srgb_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_darken_srgb_prgb32_pixel);
    }

    static INLINE void blend_lighten_srgb_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_lighten_srgb_prgb32_pixel);
    }

    static INLINE void blend_overlay_srgb_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_overlay_srgb_prgb32_pixel);
    }

    static INLINE void blend_color_dodge_srgb_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_color_dodge_srgb_prgb32_pixel);
    }

    static INLINE void blend_color_burn_srgb_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_color_burn_srgb_prgb32_pixel);
    }

    static INLINE void blend_hard_light_srgb_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_hard_light_srgb_prgb32_pixel);
    }

    static INLINE void blend_soft_light_srgb_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_soft_light_srgb_prgb32_pixel);
    }

    static INLINE void blend_difference_srgb_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_difference_srgb_prgb32_pixel);
    }

    static INLINE void blend_exclusion_srgb_prgb32_row(
        uint32_t* dst,
        const uint32_t* backdrop,
        const uint32_t* source,
        int w) noexcept
    {
        blend_binary_prgb32_row_scalar(dst, backdrop, source, w, blend_exclusion_srgb_prgb32_pixel);
    }



    // Row helper dispatch table.
    // The first dimension is the color space (linear RGB or sRGB), 
    // and the second dimension is the blend mode.

    static const BlendRowFn kBlendRowTable[2][WG_BLEND_COUNT] =
    {
        // LINEAR_RGB
        {
            blend_normal_linear_prgb32_row,
            blend_multiply_linear_prgb32_row,
            blend_screen_linear_prgb32_row,
            blend_darken_linear_prgb32_row,
            blend_lighten_linear_prgb32_row,
            blend_overlay_linear_prgb32_row,
            blend_color_dodge_linear_prgb32_row,
            blend_color_burn_linear_prgb32_row,
            blend_hard_light_linear_prgb32_row,
            blend_soft_light_linear_prgb32_row,
            blend_difference_linear_prgb32_row,
            blend_exclusion_linear_prgb32_row
        },

        // SRGB
        {
            blend_normal_srgb_prgb32_row,
            blend_multiply_srgb_prgb32_row,
            blend_screen_srgb_prgb32_row,
            blend_darken_srgb_prgb32_row,
            blend_lighten_srgb_prgb32_row,
            blend_overlay_srgb_prgb32_row,
            blend_color_dodge_srgb_prgb32_row,
            blend_color_burn_srgb_prgb32_row,
            blend_hard_light_srgb_prgb32_row,
            blend_soft_light_srgb_prgb32_row,
            blend_difference_srgb_prgb32_row,
            blend_exclusion_srgb_prgb32_row
        }
    };

    static INLINE BlendRowFn get_blend_row_fn(
        WGBlendMode mode,
        WGFilterColorSpace cs) noexcept
    {
        if (mode >= WG_BLEND_COUNT)
            return nullptr;
        if (cs != WG_FILTER_COLORSPACE_LINEAR_RGB &&
            cs != WG_FILTER_COLORSPACE_SRGB)
            return nullptr;

        return kBlendRowTable[(int)cs][(int)mode];
    }


}