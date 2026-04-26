#pragma once

#include "filter_types.h"
#include "surface.h"
#include "surface_traversal.h"

namespace waavs
{
    static float applyTransferFunc(const ComponentFunc& f, float x) noexcept
    {
        x = clamp01f(x);

        switch (f.type)
        {
        default:
        case FILTER_TRANSFER_IDENTITY:
            return x;

        case FILTER_TRANSFER_LINEAR:
            return clamp01f(f.p0 * x + f.p1);

        case FILTER_TRANSFER_GAMMA:
            return clamp01f(f.p0 * std::pow(x, f.p1) + f.p2);

        case FILTER_TRANSFER_TABLE:
        {
            if (!f.table.p || f.table.n == 0)
                return x;

            if (f.table.n == 1)
                return clamp01f(f.table.p[0]);

            if (x >= 1.0f)
                return clamp01f(f.table.p[f.table.n - 1]);

            const float pos = x * float(f.table.n - 1);
            uint32_t i0 = (uint32_t)std::floor(pos);
            if (i0 >= f.table.n - 1)
                i0 = f.table.n - 2;

            const uint32_t i1 = i0 + 1;
            const float t = pos - float(i0);

            return clamp01f(f.table.p[i0] * (1.0f - t) + f.table.p[i1] * t);
        }

        case FILTER_TRANSFER_DISCRETE:
        {
            if (!f.table.p || f.table.n == 0)
                return x;

            if (x >= 1.0f)
                return clamp01f(f.table.p[f.table.n - 1]);

            uint32_t idx = (uint32_t)std::floor(x * float(f.table.n));
            if (idx >= f.table.n)
                idx = f.table.n - 1;

            return clamp01f(f.table.p[idx]);
        }
        }
    }


    static INLINE void componenttransfer_srgb_row_scalar(
        uint32_t* dst,
        const uint32_t* src,
        int x0, int x1,
        const ComponentFunc& rF,
        const ComponentFunc& gF,
        const ComponentFunc& bF,
        const ComponentFunc& aF) noexcept
    {
        for (int x = x0; x < x1; ++x)
        {
            const uint32_t px = src[x];

            const float a_p = dequantize0_255((px >> 24) & 0xFFu);
            const float r_p = dequantize0_255((px >> 16) & 0xFFu);
            const float g_p = dequantize0_255((px >> 8) & 0xFFu);
            const float b_p = dequantize0_255((px >> 0) & 0xFFu);

            float a = a_p;
            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;

            if (a_p > 0.0f)
            {
                const float invA = 1.0f / a_p;
                r = clamp01f(r_p * invA);
                g = clamp01f(g_p * invA);
                b = clamp01f(b_p * invA);
            }

            float rr = applyTransferFunc(rF, r);
            float gg = applyTransferFunc(gF, g);
            float bb = applyTransferFunc(bF, b);
            float aa = applyTransferFunc(aF, a);

            rr = clamp01f(rr);
            gg = clamp01f(gg);
            bb = clamp01f(bb);
            aa = clamp01f(aa);

            rr *= aa;
            gg *= aa;
            bb *= aa;

            dst[x] = argb32_pack_u8(
                quantize0_255(aa),
                quantize0_255(rr),
                quantize0_255(gg),
                quantize0_255(bb));
        }
    }


    static INLINE void componenttransfer_linearRGB_row_scalar(
        uint32_t* dst,
        const uint32_t* src,
        int x0, int x1,
        const ComponentFunc& rF,
        const ComponentFunc& gF,
        const ComponentFunc& bF,
        const ComponentFunc& aF) noexcept
    {
        for (int x = x0; x < x1; ++x)
        {
            const uint32_t px = src[x];

            const float a_p = dequantize0_255((px >> 24) & 0xFFu);
            const float r_p = dequantize0_255((px >> 16) & 0xFFu);
            const float g_p = dequantize0_255((px >> 8) & 0xFFu);
            const float b_p = dequantize0_255((px >> 0) & 0xFFu);

            float a = a_p;
            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;

            if (a_p > 0.0f)
            {
                const float invA = 1.0f / a_p;
                r = clamp01f(r_p * invA);
                g = clamp01f(g_p * invA);
                b = clamp01f(b_p * invA);
            }

            // Convert to linearRGB
            r = clamp01f(coloring_srgb_component_to_linear(r));
            g = clamp01f(coloring_srgb_component_to_linear(g));
            b = clamp01f(coloring_srgb_component_to_linear(b));

            float rr = applyTransferFunc(rF, r);
            float gg = applyTransferFunc(gF, g);
            float bb = applyTransferFunc(bF, b);
            float aa = applyTransferFunc(aF, a);

            rr = clamp01f(rr);
            gg = clamp01f(gg);
            bb = clamp01f(bb);
            aa = clamp01f(aa);

            // Convert back to sRGB for storage
            rr = clamp01f(coloring_linear_component_to_srgb(rr));
            gg = clamp01f(coloring_linear_component_to_srgb(gg));
            bb = clamp01f(coloring_linear_component_to_srgb(bb));

            rr *= aa;
            gg *= aa;
            bb *= aa;

            dst[x] = argb32_pack_u8(
                quantize0_255(aa),
                quantize0_255(rr),
                quantize0_255(gg),
                quantize0_255(bb));
        }
    }

    INLINE WGResult wg_rect_componenttransfer(
        Surface dst,
        const Surface src,
        const WGRectI& area,
        const ComponentFunc& rF,
        const ComponentFunc& gF,
        const ComponentFunc& bF,
        const ComponentFunc& aF,
        WGFilterColorSpace cs) noexcept
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

        const bool useLinear =
            (cs == WG_FILTER_COLORSPACE_LINEAR_RGB);

        if (useLinear)
        {
            return wg_surface_rows_apply_unary_unchecked(
                dstView,
                srcView,
                [&](uint32_t* d, const uint32_t* s, int w) noexcept
                {
                    componenttransfer_linearRGB_row_scalar(
                        d, s, 0, w, rF, gF, bF, aF);
                });
        }
        else
        {
            return wg_surface_rows_apply_unary_unchecked(
                dstView,
                srcView,
                [&](uint32_t* d, const uint32_t* s, int w) noexcept
                {
                    componenttransfer_srgb_row_scalar(
                        d, s, 0, w, rF, gF, bF, aF);
                });
        }
    }

    /*
    INLINE WGResult wg_rect_componenttransfer(
        Surface dst,
        const Surface src,
        const WGRectI& area,
        const ComponentFunc& rF,
        const ComponentFunc& gF,
        const ComponentFunc& bF,
        const ComponentFunc& aF,
        WGFilterColorSpace cs) noexcept
    {
        const int W = (int)src.width();
        const int H = (int)src.height();

        const int x0 = max(area.x, 0);
        const int y0 = max(area.y, 0);
        const int x1 = min(area.x + area.w, W);
        const int y1 = min(area.y + area.h, H);

        if (x0 >= x1 || y0 >= y1)
            return WG_SUCCESS;

        const bool useLinear = (cs == WG_FILTER_COLORSPACE_LINEAR_RGB);

        for (int y = y0; y < y1; ++y)
        {
            const uint32_t* srow = src.rowPointer(y); // (const uint32_t*)src.rowPointer((size_t)y);
            uint32_t* drow = dst.rowPointer(y); // (uint32_t*)dst.rowPointer((size_t)y);

            if (useLinear)
            {
                componenttransfer_linearRGB_row_scalar(
                    drow, srow, x0, x1, rF, gF, bF, aF);
            }
            else
            {
                componenttransfer_srgb_row_scalar(
                    drow, srow, x0, x1, rF, gF, bF, aF);
            }
        }

        return WG_SUCCESS;
    }
    */

}
