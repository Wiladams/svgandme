#pragma once

#include "filter_types.h"
#include "surface.h"
#include "surface_traversal.h"

namespace waavs
{
    // ---------------------------------
    // ComponentTransferProgram
    // 
    // Encapsulates the details of a feComponentTransfer filter program, 
    // including the transfer functions for each channel and the color 
    // space in which to apply them.
    struct ComponentTransferProgram
    {
        ComponentFunc r;
        ComponentFunc g;
        ComponentFunc b;
        ComponentFunc a;

        WGFilterColorSpace colorSpace;
        bool allIdentity{ false };
        bool hasU8Lut{ false };

        uint8_t rLut[256];
        uint8_t gLut[256];
        uint8_t bLut[256];
        uint8_t aLut[256];
    };

    static INLINE bool component_func_is_identity(const ComponentFunc& f) noexcept
    {
        return f.type == FILTER_TRANSFER_IDENTITY;
    }

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

    // --------------------------------------
    //
    static INLINE void build_component_lut_u8(uint8_t lut[256], const ComponentFunc& f) noexcept
    {
        for (uint32_t i = 0; i < 256; ++i)
        {
            const float x = dequantize0_255(i);
            const float y = applyTransferFunc(f, x);
            lut[i] = quantize0_255(y);
        }
    }

    static INLINE void build_component_lut_linearRGB_u8(
        uint8_t lut[256],
        const ComponentFunc& f) noexcept
    {
        for (uint32_t i = 0; i < 256; ++i)
        {
            const float srgbIn = dequantize0_255(i);
            const float linIn = coloring_srgb_component_to_linear(srgbIn);
            const float linOut = applyTransferFunc(f, linIn);
            const float srgbOut = coloring_linear_component_to_srgb(linOut);

            lut[i] = quantize0_255(srgbOut);
        }
    }

    static INLINE void build_component_lut_alpha_u8(
        uint8_t lut[256],
        const ComponentFunc& f) noexcept
    {
        for (uint32_t i = 0; i < 256; ++i)
        {
            lut[i] = quantize0_255(applyTransferFunc(f, dequantize0_255(i)));
        }
    }


    // --------------------------------------
    //
    static INLINE ComponentTransferProgram prepare_componenttransfer_program(
        const ComponentFunc& rF,
        const ComponentFunc& gF,
        const ComponentFunc& bF,
        const ComponentFunc& aF,
        WGFilterColorSpace cs) noexcept
    {
        ComponentTransferProgram p{};
        p.r = rF;
        p.g = gF;
        p.b = bF;
        p.a = aF;
        p.colorSpace = cs;
        p.hasU8Lut = false;

        p.allIdentity =
            component_func_is_identity(rF) &&
            component_func_is_identity(gF) &&
            component_func_is_identity(bF) &&
            component_func_is_identity(aF);

        // build LUT
        if (!p.allIdentity)
        {
            if (cs == WG_FILTER_COLORSPACE_LINEAR_RGB)
            {
                build_component_lut_linearRGB_u8(p.rLut, rF);
                build_component_lut_linearRGB_u8(p.gLut, gF);
                build_component_lut_linearRGB_u8(p.bLut, bF);
                build_component_lut_alpha_u8(p.aLut, aF);
            }
            else 
            {
                build_component_lut_u8(p.rLut, rF);
                build_component_lut_u8(p.gLut, gF);
                build_component_lut_u8(p.bLut, bF);
                build_component_lut_u8(p.aLut, aF);
            }
            p.hasU8Lut = true;

        }

        return p;
    }
}


namespace waavs
{
#if WAAVS_HAS_NEON
    static INLINE void componenttransfer_row_u8_lut_neon(
        uint32_t* dst,
        const uint32_t* src,
        int w,
        const ComponentTransferProgram& p) noexcept
    {
        int x = 0;

        alignas(16) uint8_t aBuf[8];
        alignas(16) uint8_t rBuf[8];
        alignas(16) uint8_t gBuf[8];
        alignas(16) uint8_t bBuf[8];

        for (; x + 8 <= w; x += 8)
        {
            for (int i = 0; i < 8; ++i)
            {
                uint8_t A;
                uint8_t R;
                uint8_t G;
                uint8_t B;

                argb32_unpack_unpremul_u8(src[x + i], A, R, G, B);

                aBuf[i] = p.aLut[A];
                rBuf[i] = p.rLut[R];
                gBuf[i] = p.gLut[G];
                bBuf[i] = p.bLut[B];
            }

            const uint8x8_t A8 = vld1_u8(aBuf);
            const uint8x8_t R8 = vld1_u8(rBuf);
            const uint8x8_t G8 = vld1_u8(gBuf);
            const uint8x8_t B8 = vld1_u8(bBuf);

            const uint16x8_t A16 = vmovl_u8(A8);
            const uint16x8_t R16 = vmovl_u8(R8);
            const uint16x8_t G16 = vmovl_u8(G8);
            const uint16x8_t B16 = vmovl_u8(B8);

            const uint16x8_t Rp16 = neon_mul255_u16(R16, A16);
            const uint16x8_t Gp16 = neon_mul255_u16(G16, A16);
            const uint16x8_t Bp16 = neon_mul255_u16(B16, A16);

            const uint8x8_t Rp8 = vmovn_u16(Rp16);
            const uint8x8_t Gp8 = vmovn_u16(Gp16);
            const uint8x8_t Bp8 = vmovn_u16(Bp16);

            uint8x8x4_t bgra;
            bgra.val[0] = Bp8;
            bgra.val[1] = Gp8;
            bgra.val[2] = Rp8;
            bgra.val[3] = A8;

            vst4_u8(reinterpret_cast<uint8_t*>(dst + x), bgra);
 /*
            alignas(16) uint8_t outA[8];
            alignas(16) uint8_t outR[8];
            alignas(16) uint8_t outG[8];
            alignas(16) uint8_t outB[8];

            vst1_u8(outA, A8);
            vst1_u8(outR, Rp8);
            vst1_u8(outG, Gp8);
            vst1_u8(outB, Bp8);

            for (int i = 0; i < 8; ++i)
            {
                dst[x + i] = argb32_pack_u8(
                    outA[i],
                    outR[i],
                    outG[i],
                    outB[i]);
            }
*/
        }

        for (; x < w; ++x)
        {
            uint8_t A;
            uint8_t R;
            uint8_t G;
            uint8_t B;

            argb32_unpack_unpremul_u8(src[x], A, R, G, B);

            const uint8_t outA = p.aLut[A];

            dst[x] = argb32_pack_straight_to_premul_u8(
                outA,
                p.rLut[R],
                p.gLut[G],
                p.bLut[B]);
        }
    }
#endif

    // ----------------------------------------
    // componenttransfer_row_u8_scalar_lut
    // Applies the component transfer functions to a single 
    // row of pixels using precomputed 8-bit LUTs for each channel.
    //
    static INLINE void componenttransfer_row_u8_lut_scalar(
        uint32_t* dst,
        const uint32_t* src,
        int w,
        const ComponentTransferProgram& p) noexcept
    {
        for (int x = 0; x < w; ++x)
        {
            uint8_t A;
            uint8_t R;
            uint8_t G;
            uint8_t B;

            argb32_unpack_unpremul_u8(src[x], A, R, G, B);

            const uint8_t outA = p.aLut[A];

            if (outA == 0)
            {
                dst[x] = 0;
                continue;
            }

            dst[x] = argb32_pack_straight_to_premul_u8(
                outA,
                p.rLut[R],
                p.gLut[G],
                p.bLut[B]);
        }
    }

    // ------------------------------------------
    //

    static void componenttransfer_row_scalar(
        uint32_t* dst,
        const uint32_t* src,
        int w,
        const ComponentTransferProgram& p) noexcept
    {
        if (p.hasU8Lut)
        {
#if WAAVS_HAS_NEON
            componenttransfer_row_u8_lut_neon(dst, src, w, p);
#else
            componenttransfer_row_u8_lut_scalar(dst, src, w, p);
#endif
            return;
        }
    }


    static  WGResult wg_rect_componenttransfer(
        Surface dst,
        const Surface src,
        const WGRectI& area,
        const ComponentFunc& rF,
        const ComponentFunc& gF,
        const ComponentFunc& bF,
        const ComponentFunc& aF,
        WGFilterColorSpace cs) noexcept
    {
        const ComponentTransferProgram program = 
            prepare_componenttransfer_program(rF, gF, bF, aF, cs);
       
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

        // If all transfer functions are identity, we can just copy the pixels.
        if (program.allIdentity)
        {
            return wg_blit_copy_unchecked(dstView, srcView);
        }

        // apply the row kernel to each row of the surface views
        return wg_surface_rows_apply_unary_unchecked(
            dstView,
            srcView,
            [&](uint32_t* d, const uint32_t* s, int w) noexcept
            {
                componenttransfer_row_scalar(d, s, w, program);
            });

    }

}
