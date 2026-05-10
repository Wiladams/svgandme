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


    static INLINE bool colorMatrixTypeHasMatrix(FilterColorMatrixType t) noexcept
    {
        return t == FILTER_COLOR_MATRIX_MATRIX;
    }


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

    /*
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

        float Mf[20]{};

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

            Mf[0] = 0.213f + 0.787f * s;  Mf[1] = 0.715f - 0.715f * s;  Mf[2] = 0.072f - 0.072f * s;  Mf[3] = 0.0f;  Mf[4] = 0.0f;
            Mf[5] = 0.213f - 0.213f * s;  Mf[6] = 0.715f + 0.285f * s;  Mf[7] = 0.072f - 0.072f * s;  Mf[8] = 0.0f;  Mf[9] = 0.0f;
            Mf[10] = 0.213f - 0.213f * s;  Mf[11] = 0.715f - 0.715f * s;  Mf[12] = 0.072f + 0.928f * s;  Mf[13] = 0.0f;  Mf[14] = 0.0f;
            Mf[15] = 0.0f;                 Mf[16] = 0.0f;                 Mf[17] = 0.0f;                 Mf[18] = 1.0f;  Mf[19] = 0.0f;
        }
        else if (type == FILTER_COLOR_MATRIX_HUE_ROTATE)
        {
            const float a = param * 3.14159265358979323846f / 180.0f;
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
        else
        {
            // luminanceToAlpha
            Mf[0] = 0.0f;    Mf[1] = 0.0f;    Mf[2] = 0.0f;    Mf[3] = 0.0f;    Mf[4] = 0.0f;
            Mf[5] = 0.0f;    Mf[6] = 0.0f;    Mf[7] = 0.0f;    Mf[8] = 0.0f;    Mf[9] = 0.0f;
            Mf[10] = 0.0f;    Mf[11] = 0.0f;    Mf[12] = 0.0f;    Mf[13] = 0.0f;    Mf[14] = 0.0f;
            Mf[15] = 0.2126f; Mf[16] = 0.7152f; Mf[17] = 0.0722f; Mf[18] = 0.0f;    Mf[19] = 0.0f;
        }

        for (int i = 0; i < 20; ++i)
            out.Mf[i] = Mf[i];

        // Identity detection: check if Mf is approximately equal to the identity matrix.
        out.isIdentity = isIdentity(Mf);

        for (int row = 0; row < 4; ++row)
        {
            const int b = row * 5;

            out.Mq.c[b + 0] = (int32_t)std::lround(Mf[b + 0] * 256.0f);
            out.Mq.c[b + 1] = (int32_t)std::lround(Mf[b + 1] * 256.0f);
            out.Mq.c[b + 2] = (int32_t)std::lround(Mf[b + 2] * 256.0f);
            out.Mq.c[b + 3] = (int32_t)std::lround(Mf[b + 3] * 256.0f);
            out.Mq.c[b + 4] = (int32_t)std::lround(Mf[b + 4] * 255.0f * 256.0f);
        }

        static uint16_t sInvAlphaQ8[256];
        static bool sInvAlphaInit = false;

        if (!sInvAlphaInit)
        {
            sInvAlphaQ8[0] = 0;
            for (int a = 1; a < 256; ++a)
                sInvAlphaQ8[a] = (uint16_t)((255u << 8) / (uint32_t)a);

            sInvAlphaInit = true;
        }

        out.invAlphaQ8 = sInvAlphaQ8;
        return true;
    }
    */

    using ColorMatrixRowFn = void(*)(uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixPrepared& M) noexcept;

    // Get the appropriate row processing function for the given color space.
    /*
    static  ColorMatrixRowFn get_colormatrix_row_fn(
        WGFilterColorSpace cs) noexcept
    {
        switch (cs)
        {
        case WG_FILTER_COLORSPACE_LINEAR_RGB:
            return colormatrix_linear_prgb32_row;

        case WG_FILTER_COLORSPACE_SRGB:
            return colormatrix_linear_prgb32_row;
            //return colormatrix_srgb_prgb32_row;

        default:
            return nullptr;
        }
    }
    */

    static INLINE void colormatrix_float_prgb32_row(
        uint32_t* dst,
        const uint32_t* src,
        size_t n,
        const ColorMatrixPrepared& M) noexcept
    {
        for (size_t i = 0; i < n; ++i)
        {
            uint32_t ap8, rp8, gp8, bp8;
            argb32_unpack_u32(src[i], ap8, rp8, gp8, bp8);

            const float ap = dequantize0_255(ap8);

            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;

            if (ap > 0.0f)
            {
                const float invA = 1.0f / ap;

                float rs = clamp01f(dequantize0_255(rp8) * invA);
                float gs = clamp01f(dequantize0_255(gp8) * invA);
                float bs = clamp01f(dequantize0_255(bp8) * invA);

                if (M.colorSpace == WG_FILTER_COLORSPACE_LINEAR_RGB ||
                    M.type == FILTER_COLOR_MATRIX_LUMINANCE_TO_ALPHA)
                {
                    rs = coloring_srgb_component_to_linear(rs);
                    gs = coloring_srgb_component_to_linear(gs);
                    bs = coloring_srgb_component_to_linear(bs);
                }

                r = rs;
                g = gs;
                b = bs;
            }

            const float a = ap;

            const float rr = eval_row_linear_scalar(&M.Mf[0], r, g, b, a);
            const float gg = eval_row_linear_scalar(&M.Mf[5], r, g, b, a);
            const float bb = eval_row_linear_scalar(&M.Mf[10], r, g, b, a);
            const float aa = eval_row_linear_scalar(&M.Mf[15], r, g, b, a);

            const float aa_clamped = clamp01f(aa);

            float ro = rr;
            float go = gg;
            float bo = bb;

            if (M.colorSpace == WG_FILTER_COLORSPACE_LINEAR_RGB ||
                M.type == FILTER_COLOR_MATRIX_LUMINANCE_TO_ALPHA)
            {
                ro = coloring_linear_component_to_srgb(ro);
                go = coloring_linear_component_to_srgb(go);
                bo = coloring_linear_component_to_srgb(bo);
            }

            const float rrp = ro * aa_clamped;
            const float ggp = go * aa_clamped;
            const float bbp = bo * aa_clamped;

            dst[i] = argb32_pack_u8(
                quantize0_255(aa_clamped),
                quantize0_255(rrp),
                quantize0_255(ggp),
                quantize0_255(bbp));

            /*
            float rrp = rr * aa_clamped;
            float ggp = gg * aa_clamped;
            float bbp = bb * aa_clamped;

            if (M.colorSpace == WG_FILTER_COLORSPACE_LINEAR_RGB ||
                M.type == FILTER_COLOR_MATRIX_LUMINANCE_TO_ALPHA)
            {
                rrp = coloring_linear_component_to_srgb(rrp);
                ggp = coloring_linear_component_to_srgb(ggp);
                bbp = coloring_linear_component_to_srgb(bbp);
            }

            dst[i] = argb32_pack_u8(
                quantize0_255(aa_clamped),
                quantize0_255(rrp),
                quantize0_255(ggp),
                quantize0_255(bbp));
                */
        }
    }


    static INLINE void colormatrix_row_scalar(
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

        ColorMatrixRowFn rowFn = nullptr;
        rowFn = colormatrix_float_prgb32_row;

        // luminanceToAlpha should use the linear coefficients path.
        //if (M.type == FILTER_COLOR_MATRIX_LUMINANCE_TO_ALPHA)
        //    rowFn = colormatrix_linear_prgb32_row;
        //else
        //    rowFn = get_colormatrix_row_fn(M.colorSpace);

        //if (!rowFn)
        //    return;

        rowFn(dst, src, n, M);
    }


    // ------------------------------------------------
    // Workhorse routines
    // ------------------------------------------------
    INLINE WGResult wg_colormatrix_rect(
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
                colormatrix_row_scalar(d, s, size_t(w), M);
            });
    }


    // Apply a color matrix to the specified area of the source image,
    // writing results to the destination image.
    static INLINE WGResult wg_colormatrix_rect(
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

