// filter_fecomposite.h
#pragma once

#include "definitions.h"

#include "filterprogramexec.h"   // FilterProgramExecutor + IAmFroot<T>
#include "pixeling.h"
#include "pixeling_porterduff.h" // Porter-Duff pixel ops

namespace waavs
{
    // Numeric helpers for arithmetic composite operator classification 
    // and row kernel specialization.
    static INLINE bool arithmetic_is_one(int32_t v, int shift) noexcept
    {
        return v == (1 << shift);
    }

    static INLINE bool arithmetic_is_zero(int32_t v) noexcept
    {
        return v == 0;
    }

    // ------------------------------------------------------------
    // Porter-Duff Row helpers (PARGB32)
    // ------------------------------------------------------------
    // They all have the same signature, so we can use them 
    // as function pointers in the program executor.
    template <typename PixelOp>
    static INLINE void composite_binary_prgb32_row_scalar(
        uint32_t* d,
        const uint32_t* s1,
        const uint32_t* s2,
        int w,
        PixelOp op) noexcept
    {
        for (int x = 0; x < w; ++x)
            d[x] = op(s1[x], s2[x]);
    }

    // Operator: in
    //
    static  INLINE void composite_in_prgb32_row(uint32_t* d, const uint32_t* s1, const uint32_t* s2, int w) noexcept
    {
#if WAAVS_HAS_NEON
        composite_in_prgb32_row_neon(d, s1, s2, w);
#else
        composite_binary_prgb32_row_scalar(d, s1, s2, w, composite_in_prgb32_pixel);
#endif
    }

    // Operator: over
    //
    static INLINE void composite_over_prgb32_row(uint32_t* d, const uint32_t* s1, const uint32_t* s2, int w) noexcept
    {
#if WAAVS_HAS_NEON
        composite_over_prgb32_row_neon(d, s1, s2, w);
#else
        composite_binary_prgb32_row_scalar(d, s1, s2, w, composite_over_prgb32_pixel);
#endif
    }

    // Operator: out
    //
    static INLINE void composite_out_prgb32_row(uint32_t* d, const uint32_t* s1, const uint32_t* s2, int w) noexcept
    {
#if WAAVS_HAS_NEON
        composite_out_prgb32_row_neon(d, s1, s2, w);
#else
        composite_binary_prgb32_row_scalar(d, s1, s2, w, composite_out_prgb32_pixel);
#endif
    }


    // Operator: atop
    //
    static INLINE void composite_atop_prgb32_row(uint32_t* d, const uint32_t* s1, const uint32_t* s2, int w) noexcept
    {
        composite_binary_prgb32_row_scalar(d, s1, s2, w, composite_atop_prgb32_pixel);
    }

    // Operator: xor
    static INLINE void composite_xor_prgb32_row(uint32_t* d, const uint32_t* s1, const uint32_t* s2, int w) noexcept
    {
        composite_binary_prgb32_row_scalar(d, s1, s2, w, composite_xor_prgb32_pixel);
    }
}


namespace waavs
{

    //-----------------------------------------
    // onComposite() 
    // Type: binary
    //-----------------------------------------

    // This structure holds the arithmetic coefficients in fixed-point
    // format for the row kernel to use.    
    struct ArithmeticCoeffFx
    {
        int32_t k1;
        int32_t k2;
        int32_t k3;
        int32_t k4;
        int32_t shift;
    };

    static INLINE ArithmeticCoeffFx makeArithmeticCoeffFx(
        float k1,
        float k2,
        float k3,
        float k4) noexcept
    {
        ArithmeticCoeffFx fx{};

        // Fixed-point format:
        //
        //   shift = 15
        //   scale = 1 << shift
        //
        // We want the hot loop to evaluate:
        //
        //   out8 = clamp255(
        //       round(k1 * a * b / 255 +
        //             k2 * a +
        //             k3 * b +
        //             255 * k4))
        //
        // where a,b are 8-bit channel values in [0..255].
        //
        // To keep the row kernel simple, we pre-fold the constant factors:
        //
        //   fx.k1 stores k1 / 255 in Q15
        //   fx.k2 stores k2       in Q15
        //   fx.k3 stores k3       in Q15
        //   fx.k4 stores 255*k4   in Q15
        //
        // Then the row kernel can do:
        //
        //   v = (fx.k1 * a * b + fx.k2 * a + fx.k3 * b + fx.k4 + round) >> shift
        //
        // with a single final clamp to [0,255].

        constexpr int32_t kShift = 15;
        constexpr double  kScale = double(1u << kShift);

        fx.shift = kShift;
        fx.k1 = saturate_round_i32(double(k1) * (kScale * kInv255));
        fx.k2 = saturate_round_i32(double(k2) * kScale);
        fx.k3 = saturate_round_i32(double(k3) * kScale);
        fx.k4 = saturate_round_i32(double(k4) * (kScale * 255.0));

        return fx;
    }


    //----------------------------
    // Row kernels
    // ------------------------------------------

    static INLINE bool arith_fits_i32_after_round(int64_t worstMagnitude, int shift) noexcept
    {
        const int64_t rounder = int64_t(1) << (shift - 1);
        return worstMagnitude + rounder <= int64_t(int32_max);
    }

    // Copy a span
    static INLINE void copy_prgb32_row(uint32_t* dst, const uint32_t* src, size_t n) noexcept
    {
        memcpy(dst, src, n * sizeof(uint32_t));
    }

    // set a span to a specified value
    static INLINE void arithmetic_fill_prgb32_row(uint32_t* dst, size_t n, uint32_t px) noexcept
    {
        memset_l(dst, px, n);
    }

    // Set a span to zeros
    static INLINE void arithmetic_zero_prgb32_row(uint32_t* dst, size_t n) noexcept
    {
        memset(dst, 0, n * sizeof(uint32_t));
    }



    // ------------------------------------------


    static INLINE uint8_t arith_linear_only_u8(uint8_t x, int32_t kfx, int shift) noexcept
    {
        const int64_t v = (int64_t(kfx) * int64_t(x) +
            (int64_t(1) << (shift - 1))) >> shift;

        return clamp0_255_i64(v);
    }

    // Operator: linear
    static INLINE void arithmetic_linear_only_prgb32_row_scalar(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        int32_t kfx,
        int shift) noexcept
    {
        for (size_t i = 0; i < n; ++i)
        {
            uint8_t a, r, g, b;
            argb32_unpack_u8(src[i], a, r, g, b);

            dst[i] = argb32_pack_u8(
                arith_linear_only_u8(a, kfx, shift),
                arith_linear_only_u8(r, kfx, shift),
                arith_linear_only_u8(g, kfx, shift),
                arith_linear_only_u8(b, kfx, shift));
        }
    }

    static INLINE bool arith_k1_only_can_use_neon(const ArithmeticCoeffFx& fx) noexcept
    {
        return arith_fits_i32_after_round(int64_t(abs(fx.k1)) * 255 * 255, fx.shift);
    }

    static INLINE uint8_t arith_k1_only_u8(uint8_t a, uint8_t b, int32_t k1fx, int shift) noexcept
    {
        int64_t v = (int64_t(k1fx) * int64_t(a) * int64_t(b) +
            (int64_t(1) << (shift - 1))) >> shift;

        return clamp0_255_i64(v);
    }

    static INLINE void arithmetic_k1_only_prgb32_row_scalar(
        uint32_t* dst,
        const uint32_t* s1,
        const uint32_t* s2,
        size_t n,
        const ArithmeticCoeffFx& fx) noexcept
    {
        for (size_t i = 0; i < n; ++i)
        {
            const uint32_t p1 = s1[i];
            const uint32_t p2 = s2[i];

            uint8_t a1, r1, g1, b1;
            uint8_t a2, r2, g2, b2;

            argb32_unpack_u8(p1, a1, r1, g1, b1);
            argb32_unpack_u8(p2, a2, r2, g2, b2);

            const uint8_t a = arith_k1_only_u8(a1, a2, fx.k1, fx.shift);
            const uint8_t r = arith_k1_only_u8(r1, r2, fx.k1, fx.shift);
            const uint8_t g = arith_k1_only_u8(g1, g2, fx.k1, fx.shift);
            const uint8_t b = arith_k1_only_u8(b1, b2, fx.k1, fx.shift);

            dst[i] = argb32_pack_u8(a, r, g, b);
        }
    }


#if WAAVS_HAS_NEON
    static INLINE void arithmetic_linear_only_prgb32_row_neon(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        int32_t kfx,
        int shift) noexcept
    {
        const uint32x4_t kMaskFF = vdupq_n_u32(0xFFu);
        const int32x4_t kZero = vdupq_n_s32(0);
        const int32x4_t k255 = vdupq_n_s32(255);
        const int32x4_t kRound = vdupq_n_s32(int32_t(1) << (shift - 1));

        size_t i = 0;

        for (; i + 4 <= n; i += 4)
        {
            const uint32x4_t p = vld1q_u32(src + i);

            const uint32x4_t au = vshrq_n_u32(p, 24);
            const uint32x4_t ru = vandq_u32(vshrq_n_u32(p, 16), kMaskFF);
            const uint32x4_t gu = vandq_u32(vshrq_n_u32(p, 8), kMaskFF);
            const uint32x4_t bu = vandq_u32(p, kMaskFF);

            const int32x4_t a = vreinterpretq_s32_u32(au);
            const int32x4_t r = vreinterpretq_s32_u32(ru);
            const int32x4_t g = vreinterpretq_s32_u32(gu);
            const int32x4_t b = vreinterpretq_s32_u32(bu);

            int32x4_t ao = vmlaq_n_s32(kRound, a, kfx);
            int32x4_t ro = vmlaq_n_s32(kRound, r, kfx);
            int32x4_t go = vmlaq_n_s32(kRound, g, kfx);
            int32x4_t bo = vmlaq_n_s32(kRound, b, kfx);

            if (shift == 15)
            {
                ao = vshrq_n_s32(ao, 15);
                ro = vshrq_n_s32(ro, 15);
                go = vshrq_n_s32(go, 15);
                bo = vshrq_n_s32(bo, 15);
            }
            else
            {
                ao = vshlq_s32(ao, vdupq_n_s32(-shift));
                ro = vshlq_s32(ro, vdupq_n_s32(-shift));
                go = vshlq_s32(go, vdupq_n_s32(-shift));
                bo = vshlq_s32(bo, vdupq_n_s32(-shift));
            }

            ao = vmaxq_s32(ao, kZero);
            ro = vmaxq_s32(ro, kZero);
            go = vmaxq_s32(go, kZero);
            bo = vmaxq_s32(bo, kZero);

            ao = vminq_s32(ao, k255);
            ro = vminq_s32(ro, k255);
            go = vminq_s32(go, k255);
            bo = vminq_s32(bo, k255);

            const uint32x4_t aout = vshlq_n_u32(vreinterpretq_u32_s32(ao), 24);
            const uint32x4_t rout = vshlq_n_u32(vreinterpretq_u32_s32(ro), 16);
            const uint32x4_t gout = vshlq_n_u32(vreinterpretq_u32_s32(go), 8);
            const uint32x4_t bout = vreinterpretq_u32_s32(bo);

            const uint32x4_t out =
                vorrq_u32(vorrq_u32(aout, rout),
                    vorrq_u32(gout, bout));

            vst1q_u32(dst + i, out);
        }

        if (i < n)
            arithmetic_linear_only_prgb32_row_scalar(dst + i, src + i, n - i, kfx, shift);
    }
#endif


#if WAAVS_HAS_NEON
    static INLINE void arithmetic_k1_only_prgb32_row_neon(
        uint32_t* dst,
        const uint32_t* s1,
        const uint32_t* s2,
        size_t n,
        const ArithmeticCoeffFx& fx) noexcept
    {
        WAAVS_ASSERT(fx.shift == 15);

        const uint32x4_t kMaskFF = vdupq_n_u32(0xFFu);
        const int32x4_t kZero = vdupq_n_s32(0);
        const int32x4_t k255 = vdupq_n_s32(255);
        const int32x4_t kRound = vdupq_n_s32(1 << (fx.shift - 1));

        const int32_t k1fx = fx.k1;
        size_t i = 0;

        for (; i + 4 <= n; i += 4)
        {
            const uint32x4_t p1 = vld1q_u32(s1 + i);
            const uint32x4_t p2 = vld1q_u32(s2 + i);

            const uint32x4_t a1u = vshrq_n_u32(p1, 24);
            const uint32x4_t r1u = vandq_u32(vshrq_n_u32(p1, 16), kMaskFF);
            const uint32x4_t g1u = vandq_u32(vshrq_n_u32(p1, 8), kMaskFF);
            const uint32x4_t b1u = vandq_u32(p1, kMaskFF);

            const uint32x4_t a2u = vshrq_n_u32(p2, 24);
            const uint32x4_t r2u = vandq_u32(vshrq_n_u32(p2, 16), kMaskFF);
            const uint32x4_t g2u = vandq_u32(vshrq_n_u32(p2, 8), kMaskFF);
            const uint32x4_t b2u = vandq_u32(p2, kMaskFF);

            const int32x4_t a1 = vreinterpretq_s32_u32(a1u);
            const int32x4_t r1 = vreinterpretq_s32_u32(r1u);
            const int32x4_t g1 = vreinterpretq_s32_u32(g1u);
            const int32x4_t b1 = vreinterpretq_s32_u32(b1u);

            const int32x4_t a2 = vreinterpretq_s32_u32(a2u);
            const int32x4_t r2 = vreinterpretq_s32_u32(r2u);
            const int32x4_t g2 = vreinterpretq_s32_u32(g2u);
            const int32x4_t b2 = vreinterpretq_s32_u32(b2u);

            int32x4_t ao = vmulq_s32(a1, a2);
            int32x4_t ro = vmulq_s32(r1, r2);
            int32x4_t go = vmulq_s32(g1, g2);
            int32x4_t bo = vmulq_s32(b1, b2);

            ao = vmlaq_n_s32(kRound, ao, k1fx);
            ro = vmlaq_n_s32(kRound, ro, k1fx);
            go = vmlaq_n_s32(kRound, go, k1fx);
            bo = vmlaq_n_s32(kRound, bo, k1fx);

            ao = vshrq_n_s32(ao, 15);
            ro = vshrq_n_s32(ro, 15);
            go = vshrq_n_s32(go, 15);
            bo = vshrq_n_s32(bo, 15);

            ao = vmaxq_s32(ao, kZero);
            ro = vmaxq_s32(ro, kZero);
            go = vmaxq_s32(go, kZero);
            bo = vmaxq_s32(bo, kZero);

            ao = vminq_s32(ao, k255);
            ro = vminq_s32(ro, k255);
            go = vminq_s32(go, k255);
            bo = vminq_s32(bo, k255);

            const uint32x4_t aout = vshlq_n_u32(vreinterpretq_u32_s32(ao), 24);
            const uint32x4_t rout = vshlq_n_u32(vreinterpretq_u32_s32(ro), 16);
            const uint32x4_t gout = vshlq_n_u32(vreinterpretq_u32_s32(go), 8);
            const uint32x4_t bout = vreinterpretq_u32_s32(bo);

            const uint32x4_t out =
                vorrq_u32(vorrq_u32(aout, rout),
                    vorrq_u32(gout, bout));

            vst1q_u32(dst + i, out);
        }

        if (i < n)
            arithmetic_k1_only_prgb32_row_scalar(dst + i, s1 + i, s2 + i, n - i, fx);
    }
#endif



    static INLINE void arithmetic_k1_only_prgb32_row(
        uint32_t* dst,
        const uint32_t* s1,
        const uint32_t* s2,
        size_t n,
        const ArithmeticCoeffFx& fx) noexcept
    {
#if WAAVS_HAS_NEON
        if (arith_k1_only_can_use_neon(fx))
        {
            arithmetic_k1_only_prgb32_row_neon(dst, s1, s2, n, fx);
            return;
        }
#endif

        arithmetic_k1_only_prgb32_row_scalar(dst, s1, s2, n, fx);
    }

    // ------------------------------------------
    static INLINE bool arith_k2_only_can_use_neon(const ArithmeticCoeffFx& fx) noexcept
    {
        return arith_fits_i32_after_round(int64_t(abs(fx.k2)) * 255, fx.shift);
    }

    static INLINE void arithmetic_k2_only_prgb32_row_scalar(
        uint32_t* dst,
        const uint32_t* s1,
        size_t n,
        const ArithmeticCoeffFx& fx) noexcept
    {
        arithmetic_linear_only_prgb32_row_scalar(dst, s1, n, fx.k2, fx.shift);
    }

#if WAAVS_HAS_NEON
    static INLINE void arithmetic_k2_only_prgb32_row_neon(
        uint32_t* dst,
        const uint32_t* s1,
        size_t n,
        const ArithmeticCoeffFx& fx) noexcept
    {
        arithmetic_linear_only_prgb32_row_neon(dst, s1, n, fx.k2, fx.shift);
    }
#endif



    static INLINE void arithmetic_k2_only_prgb32_row(
        uint32_t* dst,
        const uint32_t* s1,
        size_t n,
        const ArithmeticCoeffFx& fx) noexcept
    {
#if WAAVS_HAS_NEON
        if (arith_k2_only_can_use_neon(fx))
        {
            arithmetic_k2_only_prgb32_row_neon(dst, s1, n, fx);
            return;
        }
#endif

        arithmetic_k2_only_prgb32_row_scalar(dst, s1, n, fx);
    }

    // ------------------------------------------

    static INLINE bool arith_k3_only_can_use_neon(const ArithmeticCoeffFx& fx) noexcept
    {
        return arith_fits_i32_after_round(int64_t(abs(fx.k3)) * 255, fx.shift);
    }

    static INLINE void arithmetic_k3_only_prgb32_row_scalar(uint32_t* dst, const uint32_t* s2, size_t n,
        const ArithmeticCoeffFx& fx) noexcept
    {
        arithmetic_linear_only_prgb32_row_scalar(dst, s2, n, fx.k3, fx.shift);
    }

#if WAAVS_HAS_NEON
    static INLINE void arithmetic_k3_only_prgb32_row_neon(
        uint32_t* dst,
        const uint32_t* s2,
        size_t n,
        const ArithmeticCoeffFx& fx) noexcept
    {
        arithmetic_linear_only_prgb32_row_neon(dst, s2, n, fx.k3, fx.shift);
    }
#endif



    static INLINE void arithmetic_k3_only_prgb32_row(
        uint32_t* dst,
        const uint32_t* s2,
        size_t n,
        const ArithmeticCoeffFx& fx) noexcept
    {
#if WAAVS_HAS_NEON
        if (arith_k3_only_can_use_neon(fx))
        {
            arithmetic_k3_only_prgb32_row_neon(dst, s2, n, fx);
            return;
        }
#endif

        arithmetic_k3_only_prgb32_row_scalar(dst, s2, n, fx);
    }

    // ------------------------------------------
    static INLINE uint8_t arith_k4_only_u8(int32_t k4fx, int shift) noexcept
    {
        int64_t v = (int64_t(k4fx) + (int64_t(1) << (shift - 1))) >> shift;
        return clamp0_255_i64(v);
    }

    // ------------------------------------------
    static INLINE bool arith_k2_k3_can_use_neon(const ArithmeticCoeffFx& fx) noexcept
    {
        return arith_fits_i32_after_round(
            int64_t(abs(fx.k2)) * 255 +
            int64_t(abs(fx.k3)) * 255,
            fx.shift);
    }


    static INLINE uint8_t arith_k2_k3_u8(
        uint8_t a,
        uint8_t b,
        int32_t k2fx,
        int32_t k3fx,
        int shift) noexcept
    {
        int64_t v =
            (k2fx * int32_t(a) +
                k3fx * int32_t(b) +
                (int64_t(1) << (shift - 1))) >> shift;

        return clamp0_255_i64(v);
    }

    static INLINE void arithmetic_k2_k3_prgb32_row_scalar(
        uint32_t* dst,
        const uint32_t* s1,
        const uint32_t* s2,
        size_t n,
        const ArithmeticCoeffFx& fx) noexcept
    {
        for (size_t i = 0; i < n; ++i)
        {
            const uint32_t p1 = s1[i];
            const uint32_t p2 = s2[i];

            uint8_t a1, r1, g1, b1;
            uint8_t a2, r2, g2, b2;

            argb32_unpack_u8(p1, a1, r1, g1, b1);
            argb32_unpack_u8(p2, a2, r2, g2, b2);

            const uint8_t a = arith_k2_k3_u8(a1, a2, fx.k2, fx.k3, fx.shift);
            const uint8_t r = arith_k2_k3_u8(r1, r2, fx.k2, fx.k3, fx.shift);
            const uint8_t g = arith_k2_k3_u8(g1, g2, fx.k2, fx.k3, fx.shift);
            const uint8_t b = arith_k2_k3_u8(b1, b2, fx.k2, fx.k3, fx.shift);

            dst[i] = argb32_pack_u8(a, r, g, b);
        }
    }

#if WAAVS_HAS_NEON
    static INLINE void arithmetic_k2_k3_prgb32_row_neon(
        uint32_t* dst,
        const uint32_t* s1,
        const uint32_t* s2,
        size_t n,
        const ArithmeticCoeffFx& fx) noexcept
    {
        const uint32x4_t kMaskFF = vdupq_n_u32(0xFFu);
        const int32x4_t kZero = vdupq_n_s32(0);
        const int32x4_t k255 = vdupq_n_s32(255);
        const int32x4_t kRound = vdupq_n_s32(1 << (fx.shift - 1));

        const int32_t k2fx = fx.k2;
        const int32_t k3fx = fx.k3;

        size_t i = 0;

        for (; i + 4 <= n; i += 4)
        {
            const uint32x4_t p1 = vld1q_u32(s1 + i);
            const uint32x4_t p2 = vld1q_u32(s2 + i);

            const uint32x4_t a1u = vshrq_n_u32(p1, 24);
            const uint32x4_t r1u = vandq_u32(vshrq_n_u32(p1, 16), kMaskFF);
            const uint32x4_t g1u = vandq_u32(vshrq_n_u32(p1, 8), kMaskFF);
            const uint32x4_t b1u = vandq_u32(p1, kMaskFF);

            const uint32x4_t a2u = vshrq_n_u32(p2, 24);
            const uint32x4_t r2u = vandq_u32(vshrq_n_u32(p2, 16), kMaskFF);
            const uint32x4_t g2u = vandq_u32(vshrq_n_u32(p2, 8), kMaskFF);
            const uint32x4_t b2u = vandq_u32(p2, kMaskFF);

            const int32x4_t a1 = vreinterpretq_s32_u32(a1u);
            const int32x4_t r1 = vreinterpretq_s32_u32(r1u);
            const int32x4_t g1 = vreinterpretq_s32_u32(g1u);
            const int32x4_t b1 = vreinterpretq_s32_u32(b1u);

            const int32x4_t a2 = vreinterpretq_s32_u32(a2u);
            const int32x4_t r2 = vreinterpretq_s32_u32(r2u);
            const int32x4_t g2 = vreinterpretq_s32_u32(g2u);
            const int32x4_t b2 = vreinterpretq_s32_u32(b2u);

            int32x4_t ao = kRound;
            int32x4_t ro = kRound;
            int32x4_t go = kRound;
            int32x4_t bo = kRound;

            ao = vmlaq_n_s32(ao, a1, k2fx);
            ao = vmlaq_n_s32(ao, a2, k3fx);

            ro = vmlaq_n_s32(ro, r1, k2fx);
            ro = vmlaq_n_s32(ro, r2, k3fx);

            go = vmlaq_n_s32(go, g1, k2fx);
            go = vmlaq_n_s32(go, g2, k3fx);

            bo = vmlaq_n_s32(bo, b1, k2fx);
            bo = vmlaq_n_s32(bo, b2, k3fx);

            ao = vshrq_n_s32(ao, 15);
            ro = vshrq_n_s32(ro, 15);
            go = vshrq_n_s32(go, 15);
            bo = vshrq_n_s32(bo, 15);

            ao = vmaxq_s32(ao, kZero);
            ro = vmaxq_s32(ro, kZero);
            go = vmaxq_s32(go, kZero);
            bo = vmaxq_s32(bo, kZero);

            ao = vminq_s32(ao, k255);
            ro = vminq_s32(ro, k255);
            go = vminq_s32(go, k255);
            bo = vminq_s32(bo, k255);

            const uint32x4_t aout = vshlq_n_u32(vreinterpretq_u32_s32(ao), 24);
            const uint32x4_t rout = vshlq_n_u32(vreinterpretq_u32_s32(ro), 16);
            const uint32x4_t gout = vshlq_n_u32(vreinterpretq_u32_s32(go), 8);
            const uint32x4_t bout = vreinterpretq_u32_s32(bo);

            const uint32x4_t out =
                vorrq_u32(vorrq_u32(aout, rout),
                    vorrq_u32(gout, bout));

            vst1q_u32(dst + i, out);
        }

        if (i < n)
            arithmetic_k2_k3_prgb32_row_scalar(dst + i, s1 + i, s2 + i, n - i, fx);
    }
#endif

    static INLINE void arithmetic_k2_k3_prgb32_row(
        uint32_t* dst,
        const uint32_t* s1,
        const uint32_t* s2,
        size_t n,
        const ArithmeticCoeffFx& fx) noexcept
    {
#if WAAVS_HAS_NEON
        if (arith_k2_k3_can_use_neon(fx))
        {
            arithmetic_k2_k3_prgb32_row_neon(dst, s1, s2, n, fx);
            return;
        }
#endif

        arithmetic_k2_k3_prgb32_row_scalar(dst, s1, s2, n, fx);
    }


    // ------------------------------------------

    static INLINE uint8_t arith_k1_k2_k3_u8(
        uint8_t a,
        uint8_t b,
        const ArithmeticCoeffFx& fx) noexcept
    {
        int64_t v =
            int64_t(fx.k1) * int64_t(a) * int64_t(b) +
            int64_t(fx.k2) * int64_t(a) +
            int64_t(fx.k3) * int64_t(b) +
            (int64_t(1) << (fx.shift - 1));

        v >>= fx.shift;

        return clamp0_255_i64(v);
    }

    static INLINE void arithmetic_k1_k2_k3_prgb32_row(
        uint32_t* dst,
        const uint32_t* s1,
        const uint32_t* s2,
        size_t n,
        const ArithmeticCoeffFx& fx) noexcept
    {
        for (size_t i = 0; i < n; ++i)
        {
            const uint32_t p1 = s1[i];
            const uint32_t p2 = s2[i];

            uint8_t a1, r1, g1, b1;
            uint8_t a2, r2, g2, b2;

            argb32_unpack_u8(p1, a1, r1, g1, b1);
            argb32_unpack_u8(p2, a2, r2, g2, b2);

            const uint8_t a = arith_k1_k2_k3_u8(a1, a2, fx);
            const uint8_t r = arith_k1_k2_k3_u8(r1, r2, fx);
            const uint8_t g = arith_k1_k2_k3_u8(g1, g2, fx);
            const uint8_t b = arith_k1_k2_k3_u8(b1, b2, fx);

            dst[i] = argb32_pack_u8(a, r, g, b);
        }
    }

    static INLINE uint8_t arith_general_u8(uint8_t a, uint8_t b, const ArithmeticCoeffFx& fx) noexcept
    {
        int64_t v =
            int64_t(fx.k1) * int64_t(a) * int64_t(b) +
            int64_t(fx.k2) * int64_t(a) +
            int64_t(fx.k3) * int64_t(b) +
            int64_t(fx.k4) +
            (int64_t(1) << (fx.shift - 1));

        v >>= fx.shift;

        return clamp0_255_i64(v);
    }

    static INLINE void arithmetic_general_prgb32_row(
        uint32_t* dst,
        const uint32_t* s1,
        const uint32_t* s2,
        size_t n,
        const ArithmeticCoeffFx& fx) noexcept
    {
        for (size_t i = 0; i < n; ++i)
        {
            const uint32_t p1 = s1[i];
            const uint32_t p2 = s2[i];

            uint8_t a1, r1, g1, b1;
            uint8_t a2, r2, g2, b2;

            argb32_unpack_u8(p1, a1, r1, g1, b1);
            argb32_unpack_u8(p2, a2, r2, g2, b2);

            const uint8_t a = arith_general_u8(a1, a2, fx);
            const uint8_t r = arith_general_u8(r1, r2, fx);
            const uint8_t g = arith_general_u8(g1, g2, fx);
            const uint8_t b = arith_general_u8(b1, b2, fx);

            dst[i] = argb32_pack_u8(a, r, g, b);
        }
    }

}