#pragma once

#include "definitions.h"

#include "filterprogramexec.h"   // FilterProgramExecutor + IAmFroot<T>
//#include "filter_noise.h"
//#include "svggraphicselement.h"
//#include "viewport.h"
#include "pixeling.h"
#include "coloring.h"

namespace waavs
{



    static INLINE Color4f unpack_prgb32_to_float(uint32_t px) noexcept
    {
        Color4f c{};
        c.a = float((px >> 24) & 0xFF) * (1.0f / 255.0f);
        c.r = float((px >> 16) & 0xFF) * (1.0f / 255.0f);
        c.g = float((px >> 8) & 0xFF) * (1.0f / 255.0f);
        c.b = float((px >> 0) & 0xFF) * (1.0f / 255.0f);
        return c;
    }

    static INLINE void unpremultiply_in_place(Color4f & c) noexcept
    {
        if (c.a > 0.0f)
        {
            const float invA = 1.0f / c.a;
            c.r = clamp01f(c.r * invA);
            c.g = clamp01f(c.g * invA);
            c.b = clamp01f(c.b * invA);
        }
        else
        {
            c.r = 0.0f;
            c.g = 0.0f;
            c.b = 0.0f;
        }
    }

    static INLINE uint32_t pack_float_to_prgb32(const Color4f & c_) noexcept
    {
        Color4f c = c_;
        c.a = clamp01f(c.a);
        c.r = clamp01f(c.r);
        c.g = clamp01f(c.g);
        c.b = clamp01f(c.b);

        const float pr = c.r * c.a;
        const float pg = c.g * c.a;
        const float pb = c.b * c.a;

        const uint8_t A = clamp_u8f(c.a * 255.0f);
        const uint8_t R = clamp_u8f(pr * 255.0f);
        const uint8_t G = clamp_u8f(pg * 255.0f);
        const uint8_t B = clamp_u8f(pb * 255.0f);

        return (uint32_t(A) << 24) | (uint32_t(R) << 16) | (uint32_t(G) << 8) | uint32_t(B);
    }

    static INLINE float blend_channel_normal(float cb, float cs) noexcept
    {
        (void)cb;
        return cs;
    }

    static INLINE float blend_channel_multiply(float cb, float cs) noexcept
    {
        return cb * cs;
    }

    static INLINE float blend_channel_screen(float cb, float cs) noexcept
    {
        return cb + cs - cb * cs;
    }

    static INLINE float blend_channel_darken(float cb, float cs) noexcept
    {
        return (cb < cs) ? cb : cs;
    }

    static INLINE float blend_channel_lighten(float cb, float cs) noexcept
    {
        return (cb > cs) ? cb : cs;
    }

    static INLINE float blend_channel_overlay(float cb, float cs) noexcept
    {
        return (cb <= 0.5f) ? (2.0f * cb * cs)
            : (1.0f - 2.0f * (1.0f - cb) * (1.0f - cs));
    }

    static INLINE float blend_channel_hard_light(float cb, float cs) noexcept
    {
        return (cs <= 0.5f) ? (2.0f * cb * cs)
            : (1.0f - 2.0f * (1.0f - cb) * (1.0f - cs));
    }

    static INLINE float blend_channel_color_dodge(float cb, float cs) noexcept
    {
        if (cs >= 1.0f) return 1.0f;
        return clamp01f(cb / (1.0f - cs));
    }

    static INLINE float blend_channel_color_burn(float cb, float cs) noexcept
    {
        if (cs <= 0.0f) return 0.0f;
        return 1.0f - clamp01f((1.0f - cb) / cs);
    }

    static INLINE float blend_channel_soft_light(float cb, float cs) noexcept
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
    }

    static INLINE float blend_channel_difference(float cb, float cs) noexcept
    {
        return std::fabs(cb - cs);
    }

    static INLINE float blend_channel_exclusion(float cb, float cs) noexcept
    {
        return cb + cs - 2.0f * cb * cs;
    }

    static INLINE float blend_channel(FilterBlendMode mode, float cb, float cs) noexcept
    {
        switch (mode)
        {
        default:
        case FILTER_BLEND_NORMAL:      return blend_channel_normal(cb, cs);
        case FILTER_BLEND_MULTIPLY:    return blend_channel_multiply(cb, cs);
        case FILTER_BLEND_SCREEN:      return blend_channel_screen(cb, cs);
        case FILTER_BLEND_DARKEN:      return blend_channel_darken(cb, cs);
        case FILTER_BLEND_LIGHTEN:     return blend_channel_lighten(cb, cs);
        case FILTER_BLEND_OVERLAY:     return blend_channel_overlay(cb, cs);
        case FILTER_BLEND_COLOR_DODGE: return blend_channel_color_dodge(cb, cs);
        case FILTER_BLEND_COLOR_BURN:  return blend_channel_color_burn(cb, cs);
        case FILTER_BLEND_HARD_LIGHT:  return blend_channel_hard_light(cb, cs);
        case FILTER_BLEND_SOFT_LIGHT:  return blend_channel_soft_light(cb, cs);
        case FILTER_BLEND_DIFFERENCE:  return blend_channel_difference(cb, cs);
        case FILTER_BLEND_EXCLUSION:   return blend_channel_exclusion(cb, cs);
        }
    }

    // Blend two non-premultiplied colors with source-over alpha composition.
    // backdrop = in1, source = in2
    static INLINE Color4f feBlendPixel(FilterBlendMode mode, const Color4f & backdropPRGB, const Color4f & sourcePRGB) noexcept
    {
        Color4f b = backdropPRGB;
        Color4f s = sourcePRGB;

        unpremultiply_in_place(b);
        unpremultiply_in_place(s);

        const float ab = backdropPRGB.a;
        const float as = sourcePRGB.a;

        const float outA = as + ab * (1.0f - as);

        if (!(outA > 0.0f))
            return Color4f{ 0.0f, 0.0f, 0.0f, 0.0f };

        // Blend function result in straight color space.
        const float br = blend_channel(mode, b.r, s.r);
        const float bg = blend_channel(mode, b.g, s.g);
        const float bb = blend_channel(mode, b.b, s.b);

        // W3C compositing form for separable blend modes.
        const float outR_p =
            (1.0f - as) * ab * b.r +
            (1.0f - ab) * as * s.r +
            ab * as * br;

        const float outG_p =
            (1.0f - as) * ab * b.g +
            (1.0f - ab) * as * s.g +
            ab * as * bg;

        const float outB_p =
            (1.0f - as) * ab * b.b +
            (1.0f - ab) * as * s.b +
            ab * as * bb;

        Color4f out{};
        out.a = outA;
        out.r = clamp01f(outR_p / outA);
        out.g = clamp01f(outG_p / outA);
        out.b = clamp01f(outB_p / outA);
        return out;
    }

    static INLINE uint32_t feBlendPRGB32_normal_pixel(uint32_t backdrop, uint32_t source) noexcept
    {
        return composite_over_prgb32_pixel(backdrop, source);
    }

    static INLINE void feBlendPRGB32_normal_row(
        uint32_t* dst,
        const uint32_t* in1,
        const uint32_t* in2,
        int count) noexcept
    {
        for (int i = 0; i < count; ++i)
            dst[i] = composite_over_prgb32_pixel(in1[i], in2[i]);
    }

    static INLINE uint32_t feBlendPRGB32_pixel(FilterBlendMode mode, uint32_t backdrop, uint32_t source) noexcept
    {
        const Color4f b = unpack_prgb32_to_float(backdrop);
        const Color4f s = unpack_prgb32_to_float(source);
        const Color4f o = feBlendPixel(mode, b, s);
        return pack_float_to_prgb32(o);
    }

    /*
    static INLINE void feBlendRowPRGB32(
        uint32_t* dst,
        const uint32_t* in1,
        const uint32_t* in2,
        int count,
        FilterBlendMode mode) noexcept
    {
        for (int i = 0; i < count; ++i)
            dst[i] = feBlendPixelPRGB32(mode, in1[i], in2[i]);
    }
    */

    static INLINE void feBlendRowPRGB32(
        uint32_t* dst,
        const uint32_t* in1,
        const uint32_t* in2,
        int count,
        FilterBlendMode mode) noexcept
    {
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
        switch (mode)
        {
        case FILTER_BLEND_NORMAL:
            //feBlendPRGB32_normal_row_neon(dst, in1, in2, count);
            //return;
            break;

        case FILTER_BLEND_MULTIPLY:
            //feBlendRowPRGB32_multiply_neon(dst, in1, in2, count);
            //return;
            break;

        case FILTER_BLEND_SCREEN:
            //feBlendRowPRGB32_screen_neon(dst, in1, in2, count);
            //return;
            break;

        case FILTER_BLEND_DARKEN:
            //feBlendRowPRGB32_darken_neon(dst, in1, in2, count);
            //return;
            break;

        case FILTER_BLEND_LIGHTEN:
            //feBlendRowPRGB32_lighten_neon(dst, in1, in2, count);
            //return;
            break;

        case FILTER_BLEND_DIFFERENCE:
            //feBlendRowPRGB32_difference_neon(dst, in1, in2, count);
            //return;
            break;

        case FILTER_BLEND_EXCLUSION:
            //feBlendRowPRGB32_exclusion_neon(dst, in1, in2, count);
            //return;
            break;

        default:
            break;
        }
#endif

        // fallback to scalar if no SIMD support
        for (int i = 0; i < count; ++i)
            dst[i] = feBlendPRGB32_pixel(mode, in1[i], in2[i]);
    }
}
