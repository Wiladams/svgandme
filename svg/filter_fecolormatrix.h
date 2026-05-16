// filter_fecolormatrix.h

#pragma once


#include "filter_exec.h"

namespace waavs
{

    struct ColorMatrixPrepared
    {
        WGFilterColorSpace colorSpace;
        FilterColorMatrixType type;
        float Mf[20];
        bool isIdentity;
    };



    // ------------------------------------------------------------
    // LinearRGB path: convert to linear, apply matrix in float, convert back to sRGB
    // ------------------------------------------------------------

    static INLINE float eval_row_linear_scalar(
        const float* row,
        float r, float g, float b, float a) noexcept
    {
        const float v =
            row[0] * r +
            row[1] * g +
            row[2] * b +
            row[3] * a +
            row[4];

        return clamp01f(v);
    }

    static INLINE void colormatrix_linear_prgb32_row_scalar(
        uint32_t* dst, const uint32_t* src, size_t n,
        const float Mf[20]) noexcept
    {
        for (size_t i = 0; i < n; ++i)
        {
            uint32_t ap8, rp8, gp8, bp8;
            argb32_unpack_u32(src[i], ap8, rp8, gp8, bp8);

            const float ap = dequantize0_255(ap8);

            float sr = 0.0f;
            float sg = 0.0f;
            float sb = 0.0f;

            if (ap > 0.0f)
            {
                const float invA = 1.0f / ap;

                const float r_srgb = clamp01f(dequantize0_255(rp8) * invA);
                const float g_srgb = clamp01f(dequantize0_255(gp8) * invA);
                const float b_srgb = clamp01f(dequantize0_255(bp8) * invA);

                sr = coloring_srgb_component_to_linear(r_srgb);
                sg = coloring_srgb_component_to_linear(g_srgb);
                sb = coloring_srgb_component_to_linear(b_srgb);
            }

            const float sa = ap;

            const float rr = eval_row_linear_scalar(&Mf[0], sr, sg, sb, sa);
            const float gg = eval_row_linear_scalar(&Mf[5], sr, sg, sb, sa);
            const float bb = eval_row_linear_scalar(&Mf[10], sr, sg, sb, sa);
            const float aa = eval_row_linear_scalar(&Mf[15], sr, sg, sb, sa);

            const float rrp_lin = rr * aa;
            const float ggp_lin = gg * aa;
            const float bbp_lin = bb * aa;

            const float rrp_srgb = coloring_linear_component_to_srgb(rrp_lin);
            const float ggp_srgb = coloring_linear_component_to_srgb(ggp_lin);
            const float bbp_srgb = coloring_linear_component_to_srgb(bbp_lin);

            dst[i] = argb32_pack_u8(
                quantize0_255(aa),
                quantize0_255(rrp_srgb),
                quantize0_255(ggp_srgb),
                quantize0_255(bbp_srgb));
        }
    }




    static INLINE void colormatrix_linear_prgb32_row(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixPrepared& M) noexcept
    {
        colormatrix_linear_prgb32_row_scalar(dst, src, n, M.Mf);
    }

    // ------------------------------------------------------------
    // Identity detection (in float domain)
    // ------------------------------------------------------------
    static INLINE bool isIdentity(const float Mf[20])
    {
        static constexpr float kId[20] =
        {
            1,0,0,0,0,
            0,1,0,0,0,
            0,0,1,0,0,
            0,0,0,1,0
        };

        const float epsf = 1.0e-6f;

        for (int i = 0; i < 20; ++i)
        {
            if (std::fabs(Mf[i] - kId[i]) > epsf)
            {
                return false;
            }
        }

        return true;
    }

    static INLINE bool prepare_colormatrix(
        ColorMatrixPrepared& out,
        FilterColorMatrixType type,
        float param,
        F32Span matrix,
        WGFilterColorSpace cs) noexcept
    {
        memset(&out, 0, sizeof(out));

        out.colorSpace = cs;
        out.type = type;

        float* Mf = out.Mf;

        if (type == FILTER_COLOR_MATRIX_MATRIX)
        {
            if (!matrix.p || matrix.n != 20)
                return false;

            for (int i = 0; i < 20; ++i)
                Mf[i] = matrix.p[i];
        }
        else if (type == FILTER_COLOR_MATRIX_SATURATE)
        {
            const float s = param;

            Mf[0] = 0.213f + 0.787f * s;
            Mf[1] = 0.715f - 0.715f * s;
            Mf[2] = 0.072f - 0.072f * s;
            Mf[3] = 0.0f;
            Mf[4] = 0.0f;

            Mf[5] = 0.213f - 0.213f * s;
            Mf[6] = 0.715f + 0.285f * s;
            Mf[7] = 0.072f - 0.072f * s;
            Mf[8] = 0.0f;
            Mf[9] = 0.0f;

            Mf[10] = 0.213f - 0.213f * s;
            Mf[11] = 0.715f - 0.715f * s;
            Mf[12] = 0.072f + 0.928f * s;
            Mf[13] = 0.0f;
            Mf[14] = 0.0f;

            Mf[15] = 0.0f;
            Mf[16] = 0.0f;
            Mf[17] = 0.0f;
            Mf[18] = 1.0f;
            Mf[19] = 0.0f;
        }
        else if (type == FILTER_COLOR_MATRIX_HUE_ROTATE)
        {
            const float a = param * kPif / 180.0f;
            const float c = std::cos(a);
            const float s = std::sin(a);

            Mf[0] = 0.213f + 0.787f * c - 0.213f * s;
            Mf[1] = 0.715f - 0.715f * c - 0.715f * s;
            Mf[2] = 0.072f - 0.072f * c + 0.928f * s;
            Mf[3] = 0.0f;
            Mf[4] = 0.0f;

            Mf[5] = 0.213f - 0.213f * c + 0.143f * s;
            Mf[6] = 0.715f + 0.285f * c + 0.140f * s;
            Mf[7] = 0.072f - 0.072f * c - 0.283f * s;
            Mf[8] = 0.0f;
            Mf[9] = 0.0f;

            Mf[10] = 0.213f - 0.213f * c - 0.787f * s;
            Mf[11] = 0.715f - 0.715f * c + 0.715f * s;
            Mf[12] = 0.072f + 0.928f * c + 0.072f * s;
            Mf[13] = 0.0f;
            Mf[14] = 0.0f;

            Mf[15] = 0.0f;
            Mf[16] = 0.0f;
            Mf[17] = 0.0f;
            Mf[18] = 1.0f;
            Mf[19] = 0.0f;
        }
        else if (type == FILTER_COLOR_MATRIX_LUMINANCE_TO_ALPHA)
        {
            Mf[0] = 0.0f;
            Mf[1] = 0.0f;
            Mf[2] = 0.0f;
            Mf[3] = 0.0f;
            Mf[4] = 0.0f;

            Mf[5] = 0.0f;
            Mf[6] = 0.0f;
            Mf[7] = 0.0f;
            Mf[8] = 0.0f;
            Mf[9] = 0.0f;

            Mf[10] = 0.0f;
            Mf[11] = 0.0f;
            Mf[12] = 0.0f;
            Mf[13] = 0.0f;
            Mf[14] = 0.0f;

            Mf[15] = 0.2126f;
            Mf[16] = 0.7152f;
            Mf[17] = 0.0722f;
            Mf[18] = 0.0f;
            Mf[19] = 0.0f;
        }
        else
        {
            return false;
        }

        out.isIdentity = isIdentity(out.Mf);

        return true;
    }



    using ColorMatrixRowFn = void(*)(uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixPrepared& M) noexcept;


    static INLINE void colormatrix_luminance_to_alpha_prgb32_row_scalar(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixPrepared& M) noexcept
    {
        (void)M;

        for (size_t i = 0; i < n; ++i)
        {
            uint32_t ap8;
            uint32_t rp8;
            uint32_t gp8;
            uint32_t bp8;

            argb32_unpack_u32(src[i], ap8, rp8, gp8, bp8);

            if (ap8 == 0)
            {
                dst[i] = 0;
                continue;
            }

            // Source is PRGB32, so recover straight sRGB RGB first.
            const uint32_t r = unmul255_round_u8(rp8, ap8);
            const uint32_t g = unmul255_round_u8(gp8, ap8);
            const uint32_t b = unmul255_round_u8(bp8, ap8);

            // Rec.709 / sRGB luminance coefficients scaled by 65536.
            const uint32_t lum =
                (13933u * r +
                    46871u * g +
                    4732u * b +
                    32768u) >> 16;

            // luminanceToAlpha outputs black RGB with alpha = luminance.
            dst[i] = argb32_pack_u8(lum, 0, 0, 0);
        }
    }

#if WAAVS_HAS_NEON

    static INLINE void colormatrix_luminance_to_alpha_prgb32_row_neon(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixPrepared& M) noexcept
    {
        (void)M;

        size_t i = 0;

        const uint16x8_t kR = vdupq_n_u16(13933u);
        const uint16x8_t kG = vdupq_n_u16(46871u);
        const uint16x8_t kB = vdupq_n_u16(4732u);
        const uint32x4_t kRound = vdupq_n_u32(32768u);

        for (; i + 8 <= n; i += 8)
        {
            uint8x8x4_t p = vld4_u8(reinterpret_cast<const uint8_t*>(src + i));

            // p.val[0] = B, p.val[1] = G, p.val[2] = R, p.val[3] = A
            // This assumes little-endian ARGB32 storage as bytes BGRA.

            uint16x8_t b = vmovl_u8(p.val[0]);
            uint16x8_t g = vmovl_u8(p.val[1]);
            uint16x8_t r = vmovl_u8(p.val[2]);
            uint16x8_t a = vmovl_u8(p.val[3]);

            uint16_t rb[8];
            uint16_t gb[8];
            uint16_t bb[8];
            uint16_t ab[8];

            vst1q_u16(rb, r);
            vst1q_u16(gb, g);
            vst1q_u16(bb, b);
            vst1q_u16(ab, a);

            for (int k = 0; k < 8; ++k)
            {
                if (ab[k] == 0)
                {
                    rb[k] = 0;
                    gb[k] = 0;
                    bb[k] = 0;
                    continue;
                }

                rb[k] = unmul255_round_u8(rb[k], ab[k]);
                gb[k] = unmul255_round_u8(gb[k], ab[k]);
                bb[k] = unmul255_round_u8(bb[k], ab[k]);
            }

            r = vld1q_u16(rb);
            g = vld1q_u16(gb);
            b = vld1q_u16(bb);

            uint32x4_t lumLo = kRound;
            lumLo = vmlal_u16(lumLo, vget_low_u16(r), vget_low_u16(kR));
            lumLo = vmlal_u16(lumLo, vget_low_u16(g), vget_low_u16(kG));
            lumLo = vmlal_u16(lumLo, vget_low_u16(b), vget_low_u16(kB));

            uint32x4_t lumHi = kRound;
            lumHi = vmlal_u16(lumHi, vget_high_u16(r), vget_high_u16(kR));
            lumHi = vmlal_u16(lumHi, vget_high_u16(g), vget_high_u16(kG));
            lumHi = vmlal_u16(lumHi, vget_high_u16(b), vget_high_u16(kB));

            uint16x4_t lumLo16 = vshrn_n_u32(lumLo, 16);
            uint16x4_t lumHi16 = vshrn_n_u32(lumHi, 16);
            uint16x8_t lum16 = vcombine_u16(lumLo16, lumHi16);
            uint8x8_t lum8 = vmovn_u16(lum16);

            uint8x8x4_t out{};
            out.val[0] = vdup_n_u8(0);
            out.val[1] = vdup_n_u8(0);
            out.val[2] = vdup_n_u8(0);
            out.val[3] = lum8;

            vst4_u8(reinterpret_cast<uint8_t*>(dst + i), out);
        }

        if (i < n)
        {
            colormatrix_luminance_to_alpha_prgb32_row_scalar(dst + i, src + i, n - i, M);
        }
    }

#endif

    // ---------------------------------------------
    // colormatrix_matrix_srgb_prgb32_row
    //
    static INLINE void colormatrix_matrix_srgb_prgb32_row_scalar(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixPrepared& M) noexcept
    {
        for (size_t i = 0; i < n; ++i)
        {
            uint32_t ap8, rp8, gp8, bp8;
            argb32_unpack_u32(src[i], ap8, rp8, gp8, bp8);

            const float a = dequantize0_255(ap8);

            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;

            if (ap8 != 0)
            {
                r = dequantize0_255(unmul255_round_u8(rp8, ap8));
                g = dequantize0_255(unmul255_round_u8(gp8, ap8));
                b = dequantize0_255(unmul255_round_u8(bp8, ap8));
            }

            const float rr = eval_row_linear_scalar(&M.Mf[0], r, g, b, a);
            const float gg = eval_row_linear_scalar(&M.Mf[5], r, g, b, a);
            const float bb = eval_row_linear_scalar(&M.Mf[10], r, g, b, a);
            const float aa = eval_row_linear_scalar(&M.Mf[15], r, g, b, a);

            if (aa <= 0.0f)
            {
                dst[i] = 0;
                continue;
            }

            dst[i] = argb32_pack_u8(
                quantize0_255(aa),
                quantize0_255(rr * aa),
                quantize0_255(gg * aa),
                quantize0_255(bb * aa));
        }
    }


#if WAAVS_HAS_NEON



    static INLINE float32x4_t eval_row_srgb_neon(
        const float* row,
        float32x4_t r,
        float32x4_t g,
        float32x4_t b,
        float32x4_t a) noexcept
    {
        float32x4_t v = vdupq_n_f32(row[4]);

        v = vmlaq_n_f32(v, r, row[0]);
        v = vmlaq_n_f32(v, g, row[1]);
        v = vmlaq_n_f32(v, b, row[2]);
        v = vmlaq_n_f32(v, a, row[3]);

        return clamp01q_f32(v);
    }

    static INLINE void colormatrix_matrix_srgb_prgb32_row_neon(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixPrepared& M) noexcept
    {
        size_t i = 0;

        constexpr float kInv255Local = 1.0f / 255.0f;

        for (; i + 8 <= n; i += 8)
        {
            uint8x8x4_t p = vld4_u8(reinterpret_cast<const uint8_t*>(src + i));

            // Assumes little-endian PRGB32 byte layout: B, G, R, A.
            uint16x8_t bp = vmovl_u8(p.val[0]);
            uint16x8_t gp = vmovl_u8(p.val[1]);
            uint16x8_t rp = vmovl_u8(p.val[2]);
            uint16x8_t ap = vmovl_u8(p.val[3]);

            uint16_t rb[8];
            uint16_t gb[8];
            uint16_t bb[8];
            uint16_t ab[8];

            vst1q_u16(rb, rp);
            vst1q_u16(gb, gp);
            vst1q_u16(bb, bp);
            vst1q_u16(ab, ap);

            for (int k = 0; k < 8; ++k)
            {
                if (ab[k] != 0)
                {
                    rb[k] = unmul255_round_u8(rb[k], ab[k]);
                    gb[k] = unmul255_round_u8(gb[k], ab[k]);
                    bb[k] = unmul255_round_u8(bb[k], ab[k]);
                }
                else
                {
                    rb[k] = 0;
                    gb[k] = 0;
                    bb[k] = 0;
                }
            }

            uint16x8_t r16 = vld1q_u16(rb);
            uint16x8_t g16 = vld1q_u16(gb);
            uint16x8_t b16 = vld1q_u16(bb);
            uint16x8_t a16 = ap;

            float32x4_t r0 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(r16))), kInv255Local);
            float32x4_t g0 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(g16))), kInv255Local);
            float32x4_t b0 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(b16))), kInv255Local);
            float32x4_t a0 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(a16))), kInv255Local);

            float32x4_t r1 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(r16))), kInv255Local);
            float32x4_t g1 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(g16))), kInv255Local);
            float32x4_t b1 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(b16))), kInv255Local);
            float32x4_t a1 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(a16))), kInv255Local);

            float32x4_t rr0 = eval_row_srgb_neon(&M.Mf[0], r0, g0, b0, a0);
            float32x4_t gg0 = eval_row_srgb_neon(&M.Mf[5], r0, g0, b0, a0);
            float32x4_t bb0 = eval_row_srgb_neon(&M.Mf[10], r0, g0, b0, a0);
            float32x4_t aa0 = eval_row_srgb_neon(&M.Mf[15], r0, g0, b0, a0);

            float32x4_t rr1 = eval_row_srgb_neon(&M.Mf[0], r1, g1, b1, a1);
            float32x4_t gg1 = eval_row_srgb_neon(&M.Mf[5], r1, g1, b1, a1);
            float32x4_t bb1 = eval_row_srgb_neon(&M.Mf[10], r1, g1, b1, a1);
            float32x4_t aa1 = eval_row_srgb_neon(&M.Mf[15], r1, g1, b1, a1);

            uint8x8x4_t out{};

            out.val[0] = quantize0_255q_u8(vmulq_f32(bb0, aa0), vmulq_f32(bb1, aa1));
            out.val[1] = quantize0_255q_u8(vmulq_f32(gg0, aa0), vmulq_f32(gg1, aa1));
            out.val[2] = quantize0_255q_u8(vmulq_f32(rr0, aa0), vmulq_f32(rr1, aa1));
            out.val[3] = quantize0_255q_u8(aa0, aa1);

            vst4_u8(reinterpret_cast<uint8_t*>(dst + i), out);
        }

        if (i < n)
        {
            colormatrix_matrix_srgb_prgb32_row_scalar(dst + i, src + i, n - i, M);
        }
    }

#endif


    // ----------------------------------------------
    // colormatrix_matrix_linear_prgb32_row
    //
    static INLINE void colormatrix_matrix_linear_prgb32_row_scalar(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixPrepared& M) noexcept
    {
        for (size_t i = 0; i < n; ++i)
        {
            uint32_t ap8, rp8, gp8, bp8;
            argb32_unpack_u32(src[i], ap8, rp8, gp8, bp8);

            const float a = dequantize0_255(ap8);

            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;

            if (ap8 != 0)
            {
                r = dequantize0_255(unmul255_round_u8(rp8, ap8));
                g = dequantize0_255(unmul255_round_u8(gp8, ap8));
                b = dequantize0_255(unmul255_round_u8(bp8, ap8));

                r = coloring_srgb_component_to_linear(r);
                g = coloring_srgb_component_to_linear(g);
                b = coloring_srgb_component_to_linear(b);
            }

            const float rr = eval_row_linear_scalar(&M.Mf[0], r, g, b, a);
            const float gg = eval_row_linear_scalar(&M.Mf[5], r, g, b, a);
            const float bb = eval_row_linear_scalar(&M.Mf[10], r, g, b, a);
            const float aa = eval_row_linear_scalar(&M.Mf[15], r, g, b, a);

            if (aa <= 0.0f)
            {
                dst[i] = 0;
                continue;
            }

            const float ro = coloring_linear_component_to_srgb(rr);
            const float go = coloring_linear_component_to_srgb(gg);
            const float bo = coloring_linear_component_to_srgb(bb);

            dst[i] = argb32_pack_u8(
                quantize0_255(aa),
                quantize0_255(ro * aa),
                quantize0_255(go * aa),
                quantize0_255(bo * aa));
        }
    }

#if WAAVS_HAS_NEON

    static INLINE void colormatrix_matrix_linear_prgb32_row_neon(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixPrepared& M) noexcept
    {
        size_t i = 0;

        constexpr float kInv255Local = kInv255f;

        for (; i + 8 <= n; i += 8)
        {
            uint8x8x4_t p = vld4_u8(reinterpret_cast<const uint8_t*>(src + i));

            // Assumes little-endian PRGB32 byte layout: B, G, R, A.
            uint16x8_t bp = vmovl_u8(p.val[0]);
            uint16x8_t gp = vmovl_u8(p.val[1]);
            uint16x8_t rp = vmovl_u8(p.val[2]);
            uint16x8_t ap = vmovl_u8(p.val[3]);

            uint16_t rb[8];
            uint16_t gb[8];
            uint16_t bb[8];
            uint16_t ab[8];

            vst1q_u16(rb, rp);
            vst1q_u16(gb, gp);
            vst1q_u16(bb, bp);
            vst1q_u16(ab, ap);

            for (int k = 0; k < 8; ++k)
            {
                if (ab[k] != 0)
                {
                    rb[k] = unmul255_round_u8(rb[k], ab[k]);
                    gb[k] = unmul255_round_u8(gb[k], ab[k]);
                    bb[k] = unmul255_round_u8(bb[k], ab[k]);
                }
                else
                {
                    rb[k] = 0;
                    gb[k] = 0;
                    bb[k] = 0;
                }
            }

            uint16x8_t r16 = vld1q_u16(rb);
            uint16x8_t g16 = vld1q_u16(gb);
            uint16x8_t b16 = vld1q_u16(bb);
            uint16x8_t a16 = ap;

            float32x4_t r0 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(r16))), kInv255Local);
            float32x4_t g0 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(g16))), kInv255Local);
            float32x4_t b0 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(b16))), kInv255Local);
            float32x4_t a0 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(a16))), kInv255Local);

            float32x4_t r1 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(r16))), kInv255Local);
            float32x4_t g1 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(g16))), kInv255Local);
            float32x4_t b1 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(b16))), kInv255Local);
            float32x4_t a1 = vmulq_n_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(a16))), kInv255Local);

            r0 = coloring_srgb_component_to_linear(r0);
            g0 = coloring_srgb_component_to_linear(g0);
            b0 = coloring_srgb_component_to_linear(b0);

            r1 = coloring_srgb_component_to_linear(r1);
            g1 = coloring_srgb_component_to_linear(g1);
            b1 = coloring_srgb_component_to_linear(b1);

            float32x4_t rr0 = eval_row_srgb_neon(&M.Mf[0], r0, g0, b0, a0);
            float32x4_t gg0 = eval_row_srgb_neon(&M.Mf[5], r0, g0, b0, a0);
            float32x4_t bb0 = eval_row_srgb_neon(&M.Mf[10], r0, g0, b0, a0);
            float32x4_t aa0 = eval_row_srgb_neon(&M.Mf[15], r0, g0, b0, a0);

            float32x4_t rr1 = eval_row_srgb_neon(&M.Mf[0], r1, g1, b1, a1);
            float32x4_t gg1 = eval_row_srgb_neon(&M.Mf[5], r1, g1, b1, a1);
            float32x4_t bb1 = eval_row_srgb_neon(&M.Mf[10], r1, g1, b1, a1);
            float32x4_t aa1 = eval_row_srgb_neon(&M.Mf[15], r1, g1, b1, a1);

            rr0 = coloring_linear_component_to_srgb(rr0);
            gg0 = coloring_linear_component_to_srgb(gg0);
            bb0 = coloring_linear_component_to_srgb(bb0);

            rr1 = coloring_linear_component_to_srgb(rr1);
            gg1 = coloring_linear_component_to_srgb(gg1);
            bb1 = coloring_linear_component_to_srgb(bb1);

            uint8x8x4_t out{};

            out.val[0] = quantize0_255q_u8(vmulq_f32(bb0, aa0), vmulq_f32(bb1, aa1));
            out.val[1] = quantize0_255q_u8(vmulq_f32(gg0, aa0), vmulq_f32(gg1, aa1));
            out.val[2] = quantize0_255q_u8(vmulq_f32(rr0, aa0), vmulq_f32(rr1, aa1));
            out.val[3] = quantize0_255q_u8(aa0, aa1);

            vst4_u8(reinterpret_cast<uint8_t*>(dst + i), out);
        }

        if (i < n)
        {
            colormatrix_matrix_linear_prgb32_row_scalar(dst + i, src + i, n - i, M);
        }
    }

#endif



    // ----------------------------------------------
    // colormatrix_matrix_prgb32_row
    //
    // dispatches to linear or sRGB path based on color space
    //
    static INLINE void colormatrix_matrix_linear_prgb32_row(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixPrepared& M) noexcept
    {
#if WAAVS_HAS_NEON
        colormatrix_matrix_linear_prgb32_row_neon(dst, src, n, M);
#else
        colormatrix_matrix_linear_prgb32_row_scalar(dst, src, n, M);
#endif
    }

    static INLINE void colormatrix_matrix_srgb_prgb32_row(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixPrepared& M) noexcept
    {
#if WAAVS_HAS_NEON
        colormatrix_matrix_srgb_prgb32_row_neon(dst, src, n, M);
#else
        colormatrix_matrix_srgb_prgb32_row_scalar(dst, src, n, M);
#endif
    }

    static INLINE void colormatrix_matrix_prgb32_row(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixPrepared& M) noexcept
    {
        if (M.colorSpace == WG_FILTER_COLORSPACE_LINEAR_RGB)
        {
            colormatrix_matrix_linear_prgb32_row(dst, src, n, M);

            return;
        }

        colormatrix_matrix_srgb_prgb32_row(dst, src, n, M);
    }


    static void colormatrix_row(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixPrepared& M) noexcept
    {
        if (n == 0)
            return;

        if (M.isIdentity)
        {
            wg_hspan_copy_unchecked(dst, src, n);
            return;
        }


        if (M.type == FILTER_COLOR_MATRIX_LUMINANCE_TO_ALPHA)
        {
#if WAAVS_HAS_NEON
            colormatrix_luminance_to_alpha_prgb32_row_neon(dst, src, n, M);
#else
            colormatrix_luminance_to_alpha_prgb32_row_scalar(dst, src, n, M);
#endif
            return;
        }

        colormatrix_matrix_prgb32_row(dst, src, n, M);

    }


    // ------------------------------------------------
    // Workhorse routines
    // ------------------------------------------------
    static WGResult wg_colormatrix_rect(
        Surface dst,
        const Surface src,
        const WGRectI& area,
        const ColorMatrixPrepared& M) noexcept
    {
        Surface_ARGB32 dstInfo = dst.info();
        Surface_ARGB32 srcInfo = src.info();

        Surface_ARGB32 dstView{};
        Surface_ARGB32 srcView{};

        WGRectI clipped = intersection(area, Surface_ARGB32_bounds(&dstInfo));
        clipped = intersection(clipped, Surface_ARGB32_bounds(&srcInfo));

        if (clipped.w <= 0 || clipped.h <= 0)
            return WG_SUCCESS;

        if (Surface_ARGB32_get_subarea(dstInfo, clipped, dstView) != WG_SUCCESS)
            return WG_ERROR_Invalid_Argument;

        if (Surface_ARGB32_get_subarea(srcInfo, clipped, srcView) != WG_SUCCESS)
            return WG_ERROR_Invalid_Argument;

        return wg_surface_rows_apply_unary_unchecked(
            dstView,
            srcView,
            [&M](uint32_t* d, const uint32_t* s, int w) noexcept
            {
                colormatrix_row(d, s, size_t(w), M);
            });
    }


    // Apply a color matrix to the specified area of the source image,
    // writing results to the destination image.
    static  WGResult wg_colormatrix_rect(
        Surface dst,
        const Surface src,
        const WGRectI& area,
        FilterColorMatrixType type,
        float param,
        F32Span matrix,
        WGFilterColorSpace cs) noexcept
    {
        ColorMatrixPrepared M{};
        if (!prepare_colormatrix(M, type, param, matrix, cs))
            return WG_ERROR_Invalid_Argument;

        return wg_colormatrix_rect(dst, src, area, M);
    }
}


namespace waavs
{

    struct ColorMatrixPayload
    {
        FilterColorMatrixType type;
        float param;
        F32Span matrix;
    };

    struct ColorMatrixExecutor final
        : FilterPrimitiveExecutorBase<ColorMatrixExecutor,
        ColorMatrixTraits,
        ColorMatrixPayload>
    {
        FilterOpId opId() const noexcept override
        {
            return FilterOpId::FOP_COLOR_MATRIX;
        }

        bool executeTyped(FilterExecContext& ctx,
            const FilterResolvedIO& rio,
            const ColorMatrixPayload& p) const noexcept
        {
            if (rio.in1.empty() || rio.out.empty())
                return false;

            if (rio.writeAreaPx.w <= 0 || rio.writeAreaPx.h <= 0)
                return ctx.putImage(rio.outKey, rio.out);


            //static INLINE WGResult wg_colormatrix_rect(
            //    Surface dst,
            //    const Surface src,
            //    const WGRectI & area,
            //    FilterColorMatrixType type,
            //    float param,
            //    F32Span matrix,
            //    WGFilterColorSpace cs) noexcept


            WGResult r = wg_colormatrix_rect(
                rio.out,
                rio.in1,
                rio.writeAreaPx,
                p.type,
                p.param,
                p.matrix,
                to_WGFilterColorSpace(rio.colorSpace));

            if (r != WG_SUCCESS)
                return false;

            return ctx.putImage(rio.outKey, rio.out);
        }
    };
}

