// filter_feblend.h - implementation of feBlend filter primitive

#pragma once

#include "filter_types.h"
#include "filter_program_exec.h"   // FilterProgramExecutor + IAmFroot<T>
#include "pixeling_porterduff.h"

namespace waavs
{
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

    // ------------------------------------------------
    // Blend two ColorLinear colors according to the 
    // given blend mode, and return a straight ColorLinear result.
    //----------------------------------------------------
    static INLINE ColorLinear feBlendColorLinear(
        FilterBlendMode mode,
        const ColorLinear& backdrop,
        const ColorLinear& source) noexcept
    {
        const float ab = clamp01f(backdrop.a);
        const float as = clamp01f(source.a);

        const float outA = as + ab * (1.0f - as);

        if (!(outA > 0.0f))
            return ColorLinear{ 0.0f, 0.0f, 0.0f, 0.0f };

        // Inputs are expected to already be straight linear colors.
        const float cb_r = clamp01f(backdrop.r);
        const float cb_g = clamp01f(backdrop.g);
        const float cb_b = clamp01f(backdrop.b);

        const float cs_r = clamp01f(source.r);
        const float cs_g = clamp01f(source.g);
        const float cs_b = clamp01f(source.b);

        // Blend function result in straight linear color space.
        const float br = blend_channel(mode, cb_r, cs_r);
        const float bg = blend_channel(mode, cb_g, cs_g);
        const float bb = blend_channel(mode, cb_b, cs_b);

        // W3C separable blend compositing form.
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

        return out;
    }
    


    //----------------------------------------------------

    static INLINE void feBlendPRGB32_normal_row(
        uint32_t* dst,
        const uint32_t* in1,
        const uint32_t* in2,
        int count) noexcept
    {
        for (int i = 0; i < count; ++i)
            dst[i] = composite_over_prgb32_pixel(in1[i], in2[i]);
    }

    static INLINE uint32_t feBlendPRGB32_pixel(FilterBlendMode mode, Pixel_ARGB32 backdrop, Pixel_ARGB32 source) noexcept
    {
        const ColorPRGBA bp = coloring_ARGB32_to_prgba(backdrop);
        const ColorPRGBA sp = coloring_ARGB32_to_prgba(source);

        const ColorLinear b = coloring_linear_unpremultiply(bp);
        const ColorLinear s = coloring_linear_unpremultiply(sp);

        const ColorLinear o = feBlendColorLinear(mode, b, s);

        return coloring_prgba_to_ARGB32(coloring_linear_premultiply(o));
    }


}

#if WAAVS_HAS_NEON
    namespace waavs
    {
        // represents 4 pixels in separate 
        // float32x4_t vectors for each channel, 
        // all unpremultiplied.
        struct BlendVec4
        {
            float32x4_t a;
            float32x4_t r;
            float32x4_t g;
            float32x4_t b;
        };





        static INLINE BlendVec4 neon_load_prgb32_4(const uint32_t* p) noexcept
        {
            const float s = 1.0f / 255.0f;

            const uint32_t p0 = p[0];
            const uint32_t p1 = p[1];
            const uint32_t p2 = p[2];
            const uint32_t p3 = p[3];

            const float a0 = float((p0 >> 24) & 0xFFu) * s;
            const float a1 = float((p1 >> 24) & 0xFFu) * s;
            const float a2 = float((p2 >> 24) & 0xFFu) * s;
            const float a3 = float((p3 >> 24) & 0xFFu) * s;

            const float r0 = float((p0 >> 16) & 0xFFu) * s;
            const float r1 = float((p1 >> 16) & 0xFFu) * s;
            const float r2 = float((p2 >> 16) & 0xFFu) * s;
            const float r3 = float((p3 >> 16) & 0xFFu) * s;

            const float g0 = float((p0 >> 8) & 0xFFu) * s;
            const float g1 = float((p1 >> 8) & 0xFFu) * s;
            const float g2 = float((p2 >> 8) & 0xFFu) * s;
            const float g3 = float((p3 >> 8) & 0xFFu) * s;

            const float b0 = float((p0 >> 0) & 0xFFu) * s;
            const float b1 = float((p1 >> 0) & 0xFFu) * s;
            const float b2 = float((p2 >> 0) & 0xFFu) * s;
            const float b3 = float((p3 >> 0) & 0xFFu) * s;

            BlendVec4 v{};
            v.a = (float32x4_t){ a0, a1, a2, a3 };
            v.r = (float32x4_t){ r0, r1, r2, r3 };
            v.g = (float32x4_t){ g0, g1, g2, g3 };
            v.b = (float32x4_t){ b0, b1, b2, b3 };

            return v;
        }

        static INLINE BlendVec4 neon_unpremultiply(const BlendVec4& c) noexcept
        {
            const float32x4_t zero = vdupq_n_f32(0.0f);
            const float32x4_t eps = vdupq_n_f32(1.0e-20f);

            const uint32x4_t hasA = vcgtq_f32(c.a, zero);
            const float32x4_t invA = neon_recip_nr_f32(vmaxq_f32(c.a, eps));

            BlendVec4 out{};
            out.a = c.a;
            out.r = vbslq_f32(hasA, neon_clamp01_f32(vmulq_f32(c.r, invA)), zero);
            out.g = vbslq_f32(hasA, neon_clamp01_f32(vmulq_f32(c.g, invA)), zero);
            out.b = vbslq_f32(hasA, neon_clamp01_f32(vmulq_f32(c.b, invA)), zero);
            return out;
        }

        static INLINE float32x4_t blend_channel_multiply_f32(float32x4_t cb, float32x4_t cs) noexcept
        {
            return vmulq_f32(cb, cs);
        }

        static INLINE float32x4_t blend_channel_screen_f32(float32x4_t cb, float32x4_t cs) noexcept
        {
            return vsubq_f32(vaddq_f32(cb, cs), vmulq_f32(cb, cs));
        }

        static INLINE float32x4_t blend_channel_darken_f32(float32x4_t cb, float32x4_t cs) noexcept
        {
            return vminq_f32(cb, cs);
        }

        static INLINE float32x4_t blend_channel_lighten_f32(float32x4_t cb, float32x4_t cs) noexcept
        {
            return vmaxq_f32(cb, cs);
        }

        static INLINE float32x4_t blend_channel_difference_f32(float32x4_t cb, float32x4_t cs) noexcept
        {
            return vabdq_f32(cb, cs);
        }

        static INLINE float32x4_t blend_channel_exclusion_f32(float32x4_t cb, float32x4_t cs) noexcept
        {
            return vsubq_f32(vaddq_f32(cb, cs), vmulq_n_f32(vmulq_f32(cb, cs), 2.0f));
        }

        static INLINE float32x4_t feBlend_apply_mode_f32(
            FilterBlendMode mode,
            float32x4_t cb,
            float32x4_t cs) noexcept
        {
            switch (mode)
            {
            default:
            case FILTER_BLEND_NORMAL:
                return cs;

            case FILTER_BLEND_MULTIPLY:
                return blend_channel_multiply_f32(cb, cs);

            case FILTER_BLEND_SCREEN:
                return blend_channel_screen_f32(cb, cs);

            case FILTER_BLEND_DARKEN:
                return blend_channel_darken_f32(cb, cs);

            case FILTER_BLEND_LIGHTEN:
                return blend_channel_lighten_f32(cb, cs);

            case FILTER_BLEND_DIFFERENCE:
                return blend_channel_difference_f32(cb, cs);

            case FILTER_BLEND_EXCLUSION:
                return blend_channel_exclusion_f32(cb, cs);
            }
        }

        static INLINE void neon_store_prgb32_4(
            Pixel_ARGB32* dst,
            float32x4_t outA,
            float32x4_t outR,
            float32x4_t outG,
            float32x4_t outB) noexcept
        {
            float a[4];
            float r[4];
            float g[4];
            float b[4];

            vst1q_f32(a, neon_clamp01_f32(outA));
            vst1q_f32(r, neon_clamp01_f32(outR));
            vst1q_f32(g, neon_clamp01_f32(outG));
            vst1q_f32(b, neon_clamp01_f32(outB));

            for (int i = 0; i < 4; ++i)
            {
                const ColorLinear c{
                    r[i],
                    g[i],
                    b[i],
                    a[i]
                };

                dst[i] = coloring_prgba_to_ARGB32(coloring_linear_premultiply(c));
            }
        }



        static INLINE void feBlendRowPRGB32_separable_neon(
            uint32_t* dst,
            const uint32_t* in1,
            const uint32_t* in2,
            int count,
            FilterBlendMode mode) noexcept
        {
            const float32x4_t one = vdupq_n_f32(1.0f);
            const float32x4_t zero = vdupq_n_f32(0.0f);
            const float32x4_t eps = vdupq_n_f32(1.0e-20f);

            int i = 0;
            for (; i + 4 <= count; i += 4)
            {
                const BlendVec4 bp = neon_load_prgb32_4(in1 + i);
                const BlendVec4 sp = neon_load_prgb32_4(in2 + i);

                const BlendVec4 b = neon_unpremultiply(bp);
                const BlendVec4 s = neon_unpremultiply(sp);

                const float32x4_t ab = bp.a;
                const float32x4_t as = sp.a;

                const float32x4_t oneMinusAs = vsubq_f32(one, as);
                const float32x4_t oneMinusAb = vsubq_f32(one, ab);

                const float32x4_t outA = vaddq_f32(as, vmulq_f32(ab, oneMinusAs));

                const float32x4_t br = feBlend_apply_mode_f32(mode, b.r, s.r);
                const float32x4_t bg = feBlend_apply_mode_f32(mode, b.g, s.g);
                const float32x4_t bb = feBlend_apply_mode_f32(mode, b.b, s.b);

                const float32x4_t ab_as = vmulq_f32(ab, as);

                const float32x4_t outR_p =
                    vaddq_f32(
                        vaddq_f32(
                            vmulq_f32(vmulq_f32(oneMinusAs, ab), b.r),
                            vmulq_f32(vmulq_f32(oneMinusAb, as), s.r)),
                        vmulq_f32(ab_as, br));

                const float32x4_t outG_p =
                    vaddq_f32(
                        vaddq_f32(
                            vmulq_f32(vmulq_f32(oneMinusAs, ab), b.g),
                            vmulq_f32(vmulq_f32(oneMinusAb, as), s.g)),
                        vmulq_f32(ab_as, bg));

                const float32x4_t outB_p =
                    vaddq_f32(
                        vaddq_f32(
                            vmulq_f32(vmulq_f32(oneMinusAs, ab), b.b),
                            vmulq_f32(vmulq_f32(oneMinusAb, as), s.b)),
                        vmulq_f32(ab_as, bb));

                const uint32x4_t hasOutA = vcgtq_f32(outA, zero);
                const float32x4_t invOutA = neon_recip_nr_f32(vmaxq_f32(outA, eps));

                const float32x4_t outR =
                    vbslq_f32(hasOutA, neon_clamp01_f32(vmulq_f32(outR_p, invOutA)), zero);
                const float32x4_t outG =
                    vbslq_f32(hasOutA, neon_clamp01_f32(vmulq_f32(outG_p, invOutA)), zero);
                const float32x4_t outB =
                    vbslq_f32(hasOutA, neon_clamp01_f32(vmulq_f32(outB_p, invOutA)), zero);

                neon_store_prgb32_4(dst + i, outA, outR, outG, outB);
            }

            for (; i < count; ++i)
                dst[i] = feBlendPRGB32_pixel(mode, in1[i], in2[i]);
        }

        static INLINE void feBlendPRGB32_normal_row_neon(
            uint32_t* dst,
            const uint32_t* in1,
            const uint32_t* in2,
            int count) noexcept
        {
            // The existing scalar row already uses the correct optimized PRGB over path.
            feBlendPRGB32_normal_row(dst, in1, in2, count);
        }

        static INLINE void feBlendRowPRGB32_multiply_neon(
            uint32_t* dst,
            const uint32_t* in1,
            const uint32_t* in2,
            int count) noexcept
        {
            feBlendRowPRGB32_separable_neon(dst, in1, in2, count, FILTER_BLEND_MULTIPLY);
        }

        static INLINE void feBlendRowPRGB32_screen_neon(
            uint32_t* dst,
            const uint32_t* in1,
            const uint32_t* in2,
            int count) noexcept
        {
            feBlendRowPRGB32_separable_neon(dst, in1, in2, count, FILTER_BLEND_SCREEN);
        }

        static INLINE void feBlendRowPRGB32_darken_neon(
            uint32_t* dst,
            const uint32_t* in1,
            const uint32_t* in2,
            int count) noexcept
        {
            feBlendRowPRGB32_separable_neon(dst, in1, in2, count, FILTER_BLEND_DARKEN);
        }

        static INLINE void feBlendRowPRGB32_lighten_neon(
            uint32_t* dst,
            const uint32_t* in1,
            const uint32_t* in2,
            int count) noexcept
        {
            feBlendRowPRGB32_separable_neon(dst, in1, in2, count, FILTER_BLEND_LIGHTEN);
        }

        static INLINE void feBlendRowPRGB32_difference_neon(
            uint32_t* dst,
            const uint32_t* in1,
            const uint32_t* in2,
            int count) noexcept
        {
            feBlendRowPRGB32_separable_neon(dst, in1, in2, count, FILTER_BLEND_DIFFERENCE);
        }

        static INLINE void feBlendRowPRGB32_exclusion_neon(
            uint32_t* dst,
            const uint32_t* in1,
            const uint32_t* in2,
            int count) noexcept
        {
            feBlendRowPRGB32_separable_neon(dst, in1, in2, count, FILTER_BLEND_EXCLUSION);
        }
    }
#endif

namespace waavs {
    ///*
    static INLINE void feBlendRowPRGB32(
        uint32_t* dst,
        const uint32_t* in1,
        const uint32_t* in2,
        int count,
        FilterBlendMode mode) noexcept
    {

#if WAAVS_HAS_NEON
        switch (mode)
        {
        case FILTER_BLEND_NORMAL:
            feBlendPRGB32_normal_row_neon(dst, in1, in2, count);
            return;
            break;

        case FILTER_BLEND_MULTIPLY:
            feBlendRowPRGB32_multiply_neon(dst, in1, in2, count);
            return;
            break;

        case FILTER_BLEND_SCREEN:
            feBlendRowPRGB32_screen_neon(dst, in1, in2, count);
            return;
            break;

        case FILTER_BLEND_DARKEN:
            feBlendRowPRGB32_darken_neon(dst, in1, in2, count);
            return;
            break;

        case FILTER_BLEND_LIGHTEN:
            feBlendRowPRGB32_lighten_neon(dst, in1, in2, count);
            return;
            break;

        case FILTER_BLEND_DIFFERENCE:
            feBlendRowPRGB32_difference_neon(dst, in1, in2, count);
            return;
            break;

        case FILTER_BLEND_EXCLUSION:
            feBlendRowPRGB32_exclusion_neon(dst, in1, in2, count);
            return;
            break;

        default:
            break;
        }
#endif

        // fallback to scalar if no SIMD support
        for (int i = 0; i < count; ++i)
            dst[i] = feBlendPRGB32_pixel(mode, in1[i], in2[i]);
    }
            //*/
}

