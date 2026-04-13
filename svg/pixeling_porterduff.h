// pixeling_porterduff.h
#pragma once

#include "pixeling.h"

namespace waavs
{
    // ----------------------------------------
    // Porter-Duff Pixel helpers (premultiplied ARGB32)
    // -------------------------------------------
    static INLINE uint32_t composite_over_prgb32_pixel(uint32_t p1, uint32_t p2) noexcept
    {
        // Unpack the source pixel into components.
        // we only need alpha initially to do a quick return
        // but, we unpack them all at once anyway as it's easy.
        uint32_t sa, sr, sg, sb;
        argb32_unpack_u32(p1, sa, sr, sg, sb);

        if (sa == 255)
            return p1;
        if (sa == 0)
            return p2;

        // unpack the rgb values of the destination
        uint32_t da, dr, dg, db;
        argb32_unpack_u32(p2, da, dr, dg, db);

        const uint32_t isa = 255 - sa;

        // construct new pre-multiplied pixel values by multiplying the source color values
        const uint32_t oa = sa + mul0_255(da, isa);
        const uint32_t orr = sr + mul0_255(dr, isa);
        const uint32_t og = sg + mul0_255(dg, isa);
        const uint32_t ob = sb + mul0_255(db, isa);

        return argb32_pack_u32(oa, orr, og, ob);
    }

    static INLINE Pixel_ARGB32 composite_in_prgb32_pixel(Pixel_ARGB32 p1, Pixel_ARGB32 p2) noexcept
    {
        const uint32_t da = argb32_unpack_alpha_u32(p2);
        if (da == 255)
            return p1;
        if (da == 0)
            return 0u;

        uint32_t sa, sr, sg, sb;
        argb32_unpack_u32(p1, sa, sr, sg, sb);

        const uint32_t oa = mul0_255(sa, da);
        const uint32_t orr = mul0_255(sr, da);
        const uint32_t og = mul0_255(sg, da);
        const uint32_t ob = mul0_255(sb, da);

        return argb32_pack_u32(oa, orr, og, ob);
    }

    static INLINE Pixel_ARGB32 composite_out_prgb32_pixel(uint32_t p1, uint32_t p2) noexcept
    {
        const uint32_t da = argb32_unpack_alpha_u32(p2);
        if (da == 255)
            return 0u;
        if (da == 0)
            return p1;

        uint32_t sa, sr, sg, sb;
        argb32_unpack_u32(p1, sa, sr, sg, sb);

        const uint32_t ida = 255 - da;

        const uint32_t oa = mul0_255(sa, ida);
        const uint32_t orr = mul0_255(sr, ida);
        const uint32_t og = mul0_255(sg, ida);
        const uint32_t ob = mul0_255(sb, ida);

        return argb32_pack_u32(oa, orr, og, ob);
    }

    static INLINE Pixel_ARGB32 composite_atop_prgb32_pixel(Pixel_ARGB32 p1, Pixel_ARGB32 p2) noexcept
    {
        // Src ATOP Dst:
        //   C = Cs * Ad + Cd * (1 - As)
        //   A = Ad
        uint32_t sa, sr, sg, sb;
        argb32_unpack_u32(p1, sa, sr, sg, sb);

        uint32_t da, dr, dg, db;
        argb32_unpack_u32(p2, da, dr, dg, db);

        if (da == 0)
            return 0u;

        if (sa == 255)
        {
            const uint32_t oa = da;
            const uint32_t orr = mul0_255(sr, da);
            const uint32_t og = mul0_255(sg, da);
            const uint32_t ob = mul0_255(sb, da);
            return argb32_pack_u32(oa, orr, og, ob);
        }

        const uint32_t isa = 255 - sa;

        const uint32_t oa = da;
        const uint32_t orr = mul0_255(sr, da) + mul0_255(dr, isa);
        const uint32_t og = mul0_255(sg, da) + mul0_255(dg, isa);
        const uint32_t ob = mul0_255(sb, da) + mul0_255(db, isa);

        return argb32_pack_u32(oa, orr, og, ob);
    }

    static INLINE Pixel_ARGB32 composite_xor_prgb32_pixel(Pixel_ARGB32 p1, Pixel_ARGB32 p2) noexcept
    {
        // Src XOR Dst:
        //   C = Cs * (1 - Ad) + Cd * (1 - As)
        //   A = As * (1 - Ad) + Ad * (1 - As)
        uint32_t sa, sr, sg, sb;
        argb32_unpack_u32(p1, sa, sr, sg, sb);

        uint32_t da, dr, dg, db;
        argb32_unpack_u32(p2, da, dr, dg, db);

        if (sa == 0)
            return p2;
        if (da == 0)
            return p1;

        if (sa == 255 && da == 255)
            return 0u;

        const uint32_t isa = 255 - sa;
        const uint32_t ida = 255 - da;

        const uint32_t oa = mul0_255(sa, ida) + mul0_255(da, isa);
        const uint32_t orr = mul0_255(sr, ida) + mul0_255(dr, isa);
        const uint32_t og = mul0_255(sg, ida) + mul0_255(dg, isa);
        const uint32_t ob = mul0_255(sb, ida) + mul0_255(db, isa);

        return argb32_pack_u32(oa, orr, og, ob);
    }
}

// -----------------------------------------
// NEON-optimized row Porter-Duff compositing for PRGB32 pixels
// -----------------------------------------

namespace waavs {

#if WAAVS_HAS_NEON

    static INLINE void composite_in_prgb32_row_neon(uint32_t* d, const uint32_t* s1, const uint32_t* s2, int w) noexcept
    {
        int x = 0;

        for (; x + 4 <= w; x += 4)
        {
            // Memory byte order on little-endian ARM64:
            //   B G R A per pixel
            const uint8x16_t src8 = vld1q_u8((const uint8_t*)(s1 + x));
            const uint8x16_t dst8 = vld1q_u8((const uint8_t*)(s2 + x));

            const uint16x8_t srcLo = vmovl_u8(vget_low_u8(src8));
            const uint16x8_t srcHi = vmovl_u8(vget_high_u8(src8));
            const uint16x8_t dstLo = vmovl_u8(vget_low_u8(dst8));
            const uint16x8_t dstHi = vmovl_u8(vget_high_u8(dst8));

            const uint16x8_t aLo = neon_splat_alpha_bgra_u16(dstLo);
            const uint16x8_t aHi = neon_splat_alpha_bgra_u16(dstHi);

            const uint16x8_t outLo = neon_mul255_u16(srcLo, aLo);
            const uint16x8_t outHi = neon_mul255_u16(srcHi, aHi);

            const uint8x8_t outLo8 = vmovn_u16(outLo);
            const uint8x8_t outHi8 = vmovn_u16(outHi);
            const uint8x16_t out8 = vcombine_u8(outLo8, outHi8);

            vst1q_u8((uint8_t*)(d + x), out8);
        }

        for (; x < w; ++x)
            d[x] = composite_in_prgb32_pixel(s1[x], s2[x]);
    }

#endif

#if WAAVS_HAS_NEON

    static INLINE void composite_out_prgb32_row_neon(uint32_t* d, const uint32_t* s1, const uint32_t* s2, int w) noexcept
    {
        int x = 0;

        for (; x + 4 <= w; x += 4)
        {
            const uint8x16_t src8 = vld1q_u8((const uint8_t*)(s1 + x));
            const uint8x16_t dst8 = vld1q_u8((const uint8_t*)(s2 + x));

            const uint16x8_t srcLo = vmovl_u8(vget_low_u8(src8));
            const uint16x8_t srcHi = vmovl_u8(vget_high_u8(src8));
            const uint16x8_t dstLo = vmovl_u8(vget_low_u8(dst8));
            const uint16x8_t dstHi = vmovl_u8(vget_high_u8(dst8));

            const uint16x8_t aLo = neon_splat_inv_alpha_bgra_u16(dstLo);
            const uint16x8_t aHi = neon_splat_inv_alpha_bgra_u16(dstHi);

            const uint16x8_t outLo = neon_mul255_u16(srcLo, aLo);
            const uint16x8_t outHi = neon_mul255_u16(srcHi, aHi);

            const uint8x8_t outLo8 = vmovn_u16(outLo);
            const uint8x8_t outHi8 = vmovn_u16(outHi);
            const uint8x16_t out8 = vcombine_u8(outLo8, outHi8);

            vst1q_u8((uint8_t*)(d + x), out8);
        }

        for (; x < w; ++x)
            d[x] = composite_out_prgb32_pixel(s1[x], s2[x]);
    }

#endif



#if WAAVS_HAS_NEON

    static INLINE void composite_over_prgb32_row_neon(uint32_t* d, const uint32_t* s1, const uint32_t* s2, int w) noexcept
    {
        int x = 0;

        for (; x + 4 <= w; x += 4)
        {
            const uint8x16_t src8 = vld1q_u8((const uint8_t*)(s1 + x));
            const uint8x16_t dst8 = vld1q_u8((const uint8_t*)(s2 + x));

            const uint16x8_t srcLo = vmovl_u8(vget_low_u8(src8));
            const uint16x8_t srcHi = vmovl_u8(vget_high_u8(src8));
            const uint16x8_t dstLo = vmovl_u8(vget_low_u8(dst8));
            const uint16x8_t dstHi = vmovl_u8(vget_high_u8(dst8));

            const uint16x8_t isaLo = neon_splat_inv_alpha_bgra_u16(srcLo);
            const uint16x8_t isaHi = neon_splat_inv_alpha_bgra_u16(srcHi);

            const uint16x8_t dstScaledLo = neon_mul255_u16(dstLo, isaLo);
            const uint16x8_t dstScaledHi = neon_mul255_u16(dstHi, isaHi);

            const uint16x8_t outLo = vaddq_u16(srcLo, dstScaledLo);
            const uint16x8_t outHi = vaddq_u16(srcHi, dstScaledHi);

            const uint8x8_t outLo8 = vmovn_u16(outLo);
            const uint8x8_t outHi8 = vmovn_u16(outHi);
            const uint8x16_t out8 = vcombine_u8(outLo8, outHi8);

            vst1q_u8((uint8_t*)(d + x), out8);
        }

        for (; x < w; ++x)
            d[x] = composite_over_prgb32_pixel(s1[x], s2[x]);
    }

#endif


} // namespace waavs

// Dispatch implementation of porter-duff compositing for PRGB32 pixels, 
// which may call optimized row functions or fall back to scalar pixel 
// functions.
//
// BUGBUG - we don't actually need this dispatcher at the moment
// preserve here temporarily in case we want it back
/*
namespace waavs {
    // reference scalar implementation in case the sub-class does
// not offer any specialization
    static INLINE uint32_t composite_prgb32_scalar(uint32_t p1, uint32_t p2, FilterCompositeOp op) noexcept
    {
        const uint32_t sa = (p1 >> 24) & 0xFF;
        const uint32_t sr = (p1 >> 16) & 0xFF;
        const uint32_t sg = (p1 >> 8) & 0xFF;
        const uint32_t sb = (p1 >> 0) & 0xFF;

        const uint32_t da = (p2 >> 24) & 0xFF;
        const uint32_t dr = (p2 >> 16) & 0xFF;
        const uint32_t dg = (p2 >> 8) & 0xFF;
        const uint32_t db = (p2 >> 0) & 0xFF;

        uint32_t oa, orr, og, ob;

        switch (op)
        {
        case FILTER_COMPOSITE_OVER:
        {
            const uint32_t isa = 255 - sa;
            oa = sa + mul0_255(da, isa);
            orr = sr + mul0_255(dr, isa);
            og = sg + mul0_255(dg, isa);
            ob = sb + mul0_255(db, isa);
            break;
        }

        case FILTER_COMPOSITE_IN:
            oa = mul0_255(sa, da);
            orr = mul0_255(sr, da);
            og = mul0_255(sg, da);
            ob = mul0_255(sb, da);
            break;

        case FILTER_COMPOSITE_OUT:
        {
            const uint32_t ida = 255 - da;
            oa = mul0_255(sa, ida);
            orr = mul0_255(sr, ida);
            og = mul0_255(sg, ida);
            ob = mul0_255(sb, ida);
            break;
        }

        case FILTER_COMPOSITE_ATOP:
        {
            const uint32_t isa = 255 - sa;
            oa = da;
            orr = mul0_255(sr, da) + mul0_255(dr, isa);
            og = mul0_255(sg, da) + mul0_255(dg, isa);
            ob = mul0_255(sb, da) + mul0_255(db, isa);
            break;
        }

        case FILTER_COMPOSITE_XOR:
        {
            const uint32_t ida = 255 - da;
            const uint32_t isa = 255 - sa;
            oa = mul0_255(sa, ida) + mul0_255(da, isa);
            orr = mul0_255(sr, ida) + mul0_255(dr, isa);
            og = mul0_255(sg, ida) + mul0_255(dg, isa);
            ob = mul0_255(sb, ida) + mul0_255(db, isa);
            break;
        }

        default:
            return 0;
        }
        return argb32_pack_u32(
            oa,
            orr,
            og,
            ob);
    }
}
*/