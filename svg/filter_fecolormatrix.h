#pragma once

#include "definitions.h"
#include "coloring.h"
#include "pixeling.h"
#include "wggeometry.h"
#include "surface.h"

namespace waavs
{
    struct ColorMatrixQ88
    {
        int32_t c[20];
    };


    static INLINE uint32_t unpremul_byte_q8(uint32_t cp, uint32_t a, const uint16_t* invA) noexcept
    {
        if (a == 0)
            return 0;

        uint32_t v = (cp * invA[a] + 128u) >> 8;
        return v > 255u ? 255u : v;
    }

    static INLINE uint32_t eval_row_q88_scalar(
        const int32_t* row,
        uint32_t r, uint32_t g, uint32_t b, uint32_t a) noexcept
    {
        int32_t acc =
            row[0] * int32_t(r) +
            row[1] * int32_t(g) +
            row[2] * int32_t(b) +
            row[3] * int32_t(a) +
            row[4];

        return clamp0_255_i32((acc + 128) >> 8);
    }

    static INLINE uint32_t mul255_round_u8(uint32_t x, uint32_t a) noexcept
    {
        uint32_t t = x * a + 128u;
        return (t + (t >> 8)) >> 8;
    }

    static void colorMatrix_q88_row_scalar(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixQ88& Mq,
        const uint16_t* invA) noexcept
    {
        for (size_t i = 0; i < n; ++i)
        {
            uint32_t ap, rp, gp, bp;
            argb32_unpack_u32(src[i], ap, rp, gp, bp);

            const uint32_t r = unpremul_byte_q8(rp, ap, invA);
            const uint32_t g = unpremul_byte_q8(gp, ap, invA);
            const uint32_t b = unpremul_byte_q8(bp, ap, invA);
            const uint32_t a = ap;

            const uint32_t rr = eval_row_q88_scalar(&Mq.c[0], r, g, b, a);
            const uint32_t gg = eval_row_q88_scalar(&Mq.c[5], r, g, b, a);
            const uint32_t bb = eval_row_q88_scalar(&Mq.c[10], r, g, b, a);
            const uint32_t aa = eval_row_q88_scalar(&Mq.c[15], r, g, b, a);

            const uint32_t rrp = mul255_round_u8(rr, aa);
            const uint32_t ggp = mul255_round_u8(gg, aa);
            const uint32_t bbp = mul255_round_u8(bb, aa);

            dst[i] = argb32_pack_u32(aa, rrp, ggp, bbp);
        }
    }


#if defined(__aarch64__) && defined(__ARM_NEON)
#include <arm_neon.h>

    static INLINE int32x4_t eval_row_q88_neon_4(
        const int32x4_t r,
        const int32x4_t g,
        const int32x4_t b,
        const int32x4_t a,
        const int32_t* row) noexcept
    {
        int32x4_t acc = vdupq_n_s32(row[4]);

        acc = vmlaq_n_s32(acc, r, row[0]);
        acc = vmlaq_n_s32(acc, g, row[1]);
        acc = vmlaq_n_s32(acc, b, row[2]);
        acc = vmlaq_n_s32(acc, a, row[3]);

        acc = vaddq_s32(acc, vdupq_n_s32(128));
        acc = vshrq_n_s32(acc, 8);

        acc = vmaxq_s32(acc, vdupq_n_s32(0));
        acc = vminq_s32(acc, vdupq_n_s32(255));

        return acc;
    }

    static void colorMatrix_q88_row_neon(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixQ88& Mq,
        const uint16_t* invA) noexcept
    {
        size_t i = 0;

        for (; i + 4 <= n; i += 4)
        {
            int32_t r4[4];
            int32_t g4[4];
            int32_t b4[4];
            int32_t a4[4];

            for (int lane = 0; lane < 4; ++lane)
            {
                uint32_t ap, rp, gp, bp;
                argb32_unpack_u32(src[i + (size_t)lane], ap, rp, gp, bp);

                r4[lane] = (int32_t)unpremul_byte_q8(rp, ap, invA);
                g4[lane] = (int32_t)unpremul_byte_q8(gp, ap, invA);
                b4[lane] = (int32_t)unpremul_byte_q8(bp, ap, invA);
                a4[lane] = (int32_t)ap;
            }

            const int32x4_t rv = vld1q_s32(r4);
            const int32x4_t gv = vld1q_s32(g4);
            const int32x4_t bv = vld1q_s32(b4);
            const int32x4_t av = vld1q_s32(a4);

            const int32x4_t rrv = eval_row_q88_neon_4(rv, gv, bv, av, &Mq.c[0]);
            const int32x4_t ggv = eval_row_q88_neon_4(rv, gv, bv, av, &Mq.c[5]);
            const int32x4_t bbv = eval_row_q88_neon_4(rv, gv, bv, av, &Mq.c[10]);
            const int32x4_t aav = eval_row_q88_neon_4(rv, gv, bv, av, &Mq.c[15]);

            int32_t rr4[4];
            int32_t gg4[4];
            int32_t bb4[4];
            int32_t aa4[4];

            vst1q_s32(rr4, rrv);
            vst1q_s32(gg4, ggv);
            vst1q_s32(bb4, bbv);
            vst1q_s32(aa4, aav);

            for (int lane = 0; lane < 4; ++lane)
            {
                const uint32_t rr = (uint32_t)rr4[lane];
                const uint32_t gg = (uint32_t)gg4[lane];
                const uint32_t bb = (uint32_t)bb4[lane];
                const uint32_t aa = (uint32_t)aa4[lane];

                const uint32_t rrp = mul255_round_u8(rr, aa);
                const uint32_t ggp = mul255_round_u8(gg, aa);
                const uint32_t bbp = mul255_round_u8(bb, aa);

                dst[i + (size_t)lane] = argb32_pack_u32(aa, rrp, ggp, bbp);
            }
        }

        if (i < n)
        {
            colorMatrix_q88_row_scalar(dst + i, src + i, n - i, Mq, invA);
        }
    }
#endif

    static INLINE void colorMatrix_q88_row(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixQ88& Mq,
        const uint16_t* invA) noexcept
    {
#if defined(__aarch64__) && defined(__ARM_NEON)
        colorMatrix_q88_row_neon(dst, src, n, Mq, invA);
#else
        colorMatrix_q88_row_scalar(dst, src, n, Mq, invA);
#endif
    }


}
