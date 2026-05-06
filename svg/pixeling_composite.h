// pixeling_porterduff.h
#pragma once

#include "pixeling.h"

namespace waavs
{
    // Porter-Duff compositing operators 
    // Independent of SVG or whatever backend
    enum WGCompositeOp : uint8_t
    {
        WG_COMP_CLEAR = 0,
        WG_COMP_SRC_COPY,
        WG_COMP_SRC_OVER,
        WG_COMP_SRC_IN,
        WG_COMP_SRC_OUT,
        WG_COMP_SRC_ATOP,
        WG_COMP_SRC_XOR,

        // Note supported yet
        WG_COMP_DST_OVER,
        WG_COMP_DST_IN,
        WG_COMP_DST_OUT,
        WG_COMP_DST_ATOP,

    };

    // ----------------------------------------
    // Porter-Duff Pixel helpers (premultiplied ARGB32)
    // -------------------------------------------
    static INLINE uint32_t composite_clear_prgb32_pixel(uint32_t p1, uint32_t p2) noexcept
    {
        (void)p1;
        (void)p2;
        return 0u;
    }

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
        const uint32_t oa = sa + mul255_round_u8(da, isa);
        const uint32_t orr = sr + mul255_round_u8(dr, isa);
        const uint32_t og = sg + mul255_round_u8(dg, isa);
        const uint32_t ob = sb + mul255_round_u8(db, isa);

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

        const uint32_t oa = mul255_round_u8(sa, da);
        const uint32_t orr = mul255_round_u8(sr, da);
        const uint32_t og = mul255_round_u8(sg, da);
        const uint32_t ob = mul255_round_u8(sb, da);

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

        const uint32_t oa = mul255_round_u8(sa, ida);
        const uint32_t orr = mul255_round_u8(sr, ida);
        const uint32_t og = mul255_round_u8(sg, ida);
        const uint32_t ob = mul255_round_u8(sb, ida);

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
            const uint32_t orr = mul255_round_u8(sr, da);
            const uint32_t og = mul255_round_u8(sg, da);
            const uint32_t ob = mul255_round_u8(sb, da);
            return argb32_pack_u32(oa, orr, og, ob);
        }

        const uint32_t isa = 255 - sa;

        const uint32_t oa = da;
        const uint32_t orr = mul255_round_u8(sr, da) + mul255_round_u8(dr, isa);
        const uint32_t og = mul255_round_u8(sg, da) + mul255_round_u8(dg, isa);
        const uint32_t ob = mul255_round_u8(sb, da) + mul255_round_u8(db, isa);

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

        const uint32_t oa = mul255_round_u8(sa, ida) + mul255_round_u8(da, isa);
        const uint32_t orr = mul255_round_u8(sr, ida) + mul255_round_u8(dr, isa);
        const uint32_t og = mul255_round_u8(sg, ida) + mul255_round_u8(dg, isa);
        const uint32_t ob = mul255_round_u8(sb, ida) + mul255_round_u8(db, isa);

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


    using CompositeRowFn = void(*)(uint32_t* d, const uint32_t* s1, const uint32_t* s2, int w);

    // ------------------------------------------------------------
    // Porter-Duff Row helpers (PARGB32)
    // ------------------------------------------------------------
    // They all have the same signature, so we can use them 
    // as function pointers in the program executor.
    template <typename PixelOp>
    static INLINE void composite_binary_prgb32_row_scalar(
        uint32_t* dst,
        const uint32_t* src,
        const uint32_t* backdrop,
        int w,
        PixelOp op) noexcept
    {
        for (int x = 0; x < w; ++x)
            dst[x] = op(src[x], backdrop[x]);
    }

    // Operator: in
    //
    static  INLINE void composite_in_prgb32_row(uint32_t* dst, const uint32_t* src, const uint32_t* backdrop, int w) noexcept
    {
#if WAAVS_HAS_NEON
        composite_in_prgb32_row_neon(dst, src, backdrop, w);
#else
        composite_binary_prgb32_row_scalar(dst, src, backdrop, w, composite_in_prgb32_pixel);
#endif
    }

    // Operator: over
    //
    static INLINE void composite_over_prgb32_row(uint32_t* dst, const uint32_t* src, const uint32_t* backdrop, int w) noexcept
    {
#if WAAVS_HAS_NEON
        composite_over_prgb32_row_neon(dst, src, backdrop, w);
#else
        composite_binary_prgb32_row_scalar(dst, src, backdrop, w, composite_over_prgb32_pixel);
#endif
    }

    // Operator: out
    //
    static INLINE void composite_out_prgb32_row(uint32_t* dst, const uint32_t* src, const uint32_t* backdrop, int w) noexcept
    {
#if WAAVS_HAS_NEON
        composite_out_prgb32_row_neon(dst, src, backdrop, w);
#else
        composite_binary_prgb32_row_scalar(dst, src, backdrop, w, composite_out_prgb32_pixel);
#endif
    }


    // Operator: atop
    //
    static INLINE void composite_atop_prgb32_row(uint32_t* dst, const uint32_t* src, const uint32_t* backdrop, int w) noexcept
    {
        composite_binary_prgb32_row_scalar(dst, src, backdrop, w, composite_atop_prgb32_pixel);
    }

    // Operator: xor
    static INLINE void composite_xor_prgb32_row(uint32_t* dst, const uint32_t* src, const uint32_t* backdrop, int w) noexcept
    {
        composite_binary_prgb32_row_scalar(dst, src, backdrop, w, composite_xor_prgb32_pixel);
    }

    // Operator: clear
    static INLINE void composite_clear_prgb32_row(uint32_t* dst, const uint32_t* src, const uint32_t* backdrop, int w) noexcept
    {
        (void)src;
        (void)backdrop;

        if (w > 0)
            memset(dst, 0, w * sizeof(uint32_t));
    }

    // Operator: src copy
    static INLINE void composite_src_copy_prgb32_row(
        uint32_t* dst,
        const uint32_t* src,
        const uint32_t* backdrop,
        int w) noexcept
    {
        (void)backdrop;
        if (w > 0)
            memcpy(dst, src, size_t(w) * sizeof(uint32_t));
    }

    // Get a function pointer to the appropriate row function for a given operator.
    static INLINE CompositeRowFn get_composite_row_fn(WGCompositeOp op) noexcept
    {
        switch (op)
        {
            case WG_COMP_CLEAR:
                return composite_clear_prgb32_row;
            case WG_COMP_SRC_COPY:
                return composite_src_copy_prgb32_row;
            case WG_COMP_SRC_OVER:
                return composite_over_prgb32_row;
            case WG_COMP_SRC_IN:
                return composite_in_prgb32_row;
            case WG_COMP_SRC_OUT:
                return composite_out_prgb32_row;
            case WG_COMP_SRC_ATOP:
                return composite_atop_prgb32_row;
            case WG_COMP_SRC_XOR:
                return composite_xor_prgb32_row;
            
            default:
                return nullptr;
        }
    }
} 

