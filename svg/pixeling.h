// pixeling.h
#pragma once

#ifndef PIXELING_H_INCLUDED
#define PIXELING_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

#include "definitions.h"
#include "maths.h"
#include "wggeometry.h"


/*
 * pixeling.h — pixel-pack/unpack and surface ops for WAAVS
 *
 * Naming: pixeling_<SOURCE>_<operation>_<TARGET>
 *   Examples:
 *     pixeling_prgba_pack_ARGB32(ColorPRGBA) -> Pixel_ARGB32
 *     pixeling_ARGB32_unpack_prgba(Pixel_ARGB32) -> ColorPRGBA
 *     pixeling_ARGB32_sample_bilinear_prgba(...) -> ColorPRGBA
 *
 * Conventions:
 *   - ColorPRGBA is LINEAR RGB with PREMULTIPLIED alpha (see coloring.h).
 *   - "ARGB32" / "RGBA32" refer to the *bit layout in a 32-bit word*:
 *         ARGB32: (A<<24)|(R<<16)|(G<<8)|B
 *         RGBA32: (R<<24)|(G<<16)|(B<<8)|A
 *     Beware: byte order in memory varies by endianness; the names here
 *     describe the logical bit positions in the 32-bit value.
 *
 *   - Pack functions produce sRGB-ENCODED 8-bit pixels. For PRGBA, we
 *     convert linear -> sRGB and apply premultiplication in sRGB space
 *     only for packing (to match common API expectations).
 */


// In a 32-bit word: [A:R:G:B] means (A<<24)|(R<<16)|(G<<8)|B 
typedef uint32_t Pixel_ARGB32;
typedef uint32_t Pixel_RGBA32;

typedef uint32_t Pixel_SRGBA8_ARGB32; // [A:R:G:B] straight sRGB 
typedef uint32_t Pixel_SRGBA8_RGBA32; // [R:G:B:A] straight sRGB 

namespace waavs {


#if WAAVS_HAS_NEON

    static INLINE uint16x8_t neon_mul255_u16(uint16x8_t x, uint16x8_t y) noexcept
    {
        uint16x8_t t = vmulq_u16(x, y);
        t = vaddq_u16(t, vdupq_n_u16(128));
        t = vaddq_u16(t, vshrq_n_u16(t, 8));
        return vshrq_n_u16(t, 8);
    }

    // Input is 8 lanes holding 2 pixels in little-endian BGRA byte order:
    //   B0 G0 R0 A0 B1 G1 R1 A1
    // Output:
    //   A0 A0 A0 A0 A1 A1 A1 A1
    static INLINE uint16x8_t neon_splat_alpha_bgra_u16(uint16x8_t px) noexcept
    {
        const uint16_t a0 = vgetq_lane_u16(px, 3);
        const uint16_t a1 = vgetq_lane_u16(px, 7);
        uint16_t tmp[8] = { a0, a0, a0, a0, a1, a1, a1, a1 };
        return vld1q_u16(tmp);
    }

    static INLINE uint16x8_t neon_splat_inv_alpha_bgra_u16(uint16x8_t px) noexcept
    {
        return vsubq_u16(vdupq_n_u16(255), neon_splat_alpha_bgra_u16(px));
    }

#endif

} // namespace waavs

// SIMD helpers for pixel processing
#if defined(__SSE2__)
static INLINE __m128i mm_zero_si128() noexcept
{
    return _mm_setzero_si128();
}

static INLINE __m128i mm_mul255_epu16(__m128i x, __m128i y) noexcept
{
    __m128i t = _mm_mullo_epi16(x, y);
    t = _mm_add_epi16(t, _mm_set1_epi16(128));
    t = _mm_add_epi16(t, _mm_srli_epi16(t, 8));
    return _mm_srli_epi16(t, 8);
}

static INLINE __m128i mm_unpacklo_u8_u16(__m128i v) noexcept
{
    return _mm_unpacklo_epi8(v, mm_zero_si128());
}

static INLINE __m128i mm_unpackhi_u8_u16(__m128i v) noexcept
{
    return _mm_unpackhi_epi8(v, mm_zero_si128());
}
#endif

#if defined(__SSSE3__)
static INLINE __m128i mm_splat_alpha_u16(__m128i argb16) noexcept
{
    const __m128i shuf = _mm_setr_epi8(
        0, 1, 0, 1, 0, 1, 0, 1,
        8, 9, 8, 9, 8, 9, 8, 9
    );
    return _mm_shuffle_epi8(argb16, shuf);
}

static INLINE __m128i mm_splat_inv_alpha_u16(__m128i argb16) noexcept
{
    return _mm_sub_epi16(_mm_set1_epi16(255), mm_splat_alpha_u16(argb16));
}
#endif

namespace waavs {


    // raw byte packing without any clamping or conversion; 
    // useful for where we just want to pack known 8-bit values 
    // into a pixel
    static INLINE Pixel_ARGB32 argb32_pack_u32(uint32_t a, uint32_t r, uint32_t g, uint32_t b) noexcept
    {
        return ((a) << 24) | ((r) << 16) | ((g) << 8) | (b);
    }

    static INLINE Pixel_ARGB32 argb32_pack_u8(uint8_t a, uint8_t r, uint8_t g, uint8_t b) noexcept
    {
        return argb32_pack_u32(a, r, g, b);
    }

    static INLINE Pixel_ARGB32 argb32_pack_straight_to_premul_u8(uint8_t a, uint8_t r, uint8_t g, uint8_t b) noexcept
    {
        // Premultiply the color channels by alpha in sRGB space
        uint32_t rP = mul255_round_u8(r, a);
        uint32_t gP = mul255_round_u8(g, a);
        uint32_t bP = mul255_round_u8(b, a);

        return argb32_pack_u32(a, rP, gP, bP);
    }

    // argb32_unpack...
    // 
    // Raw byte unpacking without any conversion; 
    // useful for where we just want to get the 8-bit 
    // components out of a pixel
    static INLINE void argb32_unpack_u32(Pixel_ARGB32 px, uint32_t& a, uint32_t& r, uint32_t& g, uint32_t& b) noexcept
    {
        a = (uint32_t)((px >> 24) & 0xFF);
        r = (uint32_t)((px >> 16) & 0xFF);
        g = (uint32_t)((px >> 8) & 0xFF);
        b = (uint32_t)(px & 0xFF);
    }
    
    static INLINE void argb32_unpack_i32(Pixel_ARGB32 px, int& a, int& r, int& g, int& b) noexcept
    {
        a = (int)((px >> 24) & 0xFF);
        r = (int)((px >> 16) & 0xFF);
        g = (int)((px >> 8) & 0xFF);
        b = (int)(px & 0xFF);
    }

    static INLINE void argb32_unpack_u8(Pixel_ARGB32 px, uint8_t& a, uint8_t& r, uint8_t& g, uint8_t& b) noexcept
    {
        a = (uint8_t)((px >> 24) & 0xFF);
        r = (uint8_t)((px >> 16) & 0xFF);
        g = (uint8_t)((px >> 8) & 0xFF);
        b = (uint8_t)(px & 0xFF);
    }

    static INLINE void argb32_unpack_unpremul_u8(
        Pixel_ARGB32 px,
        uint8_t& a, uint8_t& r, uint8_t& g, uint8_t& b) noexcept
    {
        uint8_t rp, gp, bp;
        argb32_unpack_u8(px, a, rp, gp, bp);

        r = unmul255_round_u8(rp, a);
        g = unmul255_round_u8(gp, a);
        b = unmul255_round_u8(bp, a);
    }

    // argb32_from_pargb32
    // 
    // Convert a premultiplied ARGB32 pixel to a straight ARGB32 
    // pixel by unpremultiplying the color channels in sRGB space
    // and repacking.
    static INLINE Pixel_ARGB32 argb32_from_pargb32(const Pixel_ARGB32 px) noexcept
    {
        uint8_t a, r, g, b;
        argb32_unpack_unpremul_u8(px, a, r, g, b);
    
        return argb32_pack_u8(a, r, g, b);
    }

    // argb32_unpack_alpha
    // 
    // Get only the alpha channel from a PRGB32 pixel 
    // Useful for some operations where we only care about alpha
    static INLINE uint32_t argb32_unpack_alpha_u32(Pixel_ARGB32 p) noexcept
    {
        return (p >> 24) & 0xFF;
    }

    // Get alpha as a normalized float in [0,1]
    static INLINE float argb32_unpack_alpha_norm(Pixel_ARGB32 p) noexcept
    {
        return dequantize0_255((p >> 24) & 0xFFu);
    }

    static INLINE void argb32_unpack_dequantized_prgba(Pixel_ARGB32 px,
        float& a,
        float& r,
        float& g,
        float& b) noexcept
    {
        uint8_t A;
        uint8_t R;
        uint8_t G;
        uint8_t B;

        argb32_unpack_u8(px, A, R, G, B);

        a = dequantize0_255(A);
        r = dequantize0_255(R);
        g = dequantize0_255(G);
        b = dequantize0_255(B);
    }

    // argb32_unpack_dequantized_straight
    // 
    // unpack
    // de-quantize
    // unpremultiply
    // to get straight RGB components
    // 
    static INLINE void argb32_unpack_dequantized_straight(Pixel_ARGB32 px,
        float& a,
        float& r,
        float& g,
        float& b) noexcept
    {
        float rp;
        float gp;
        float bp;

        argb32_unpack_dequantized_prgba(px, a, rp, gp, bp);

        r = 0.0f;
        g = 0.0f;
        b = 0.0f;

        if (a > 0.0f)
        {
            const float invA = 1.0f / a;
            r = clamp01f(rp * invA);
            g = clamp01f(gp * invA);
            b = clamp01f(bp * invA);
        }
    }
}


namespace waavs
{
    INLINE void memset_u32(void* dstvoid, uint32_t pixel, size_t count) noexcept
    {
        if (count == 0)
            return;

        uint32_t* dst32 = static_cast<uint32_t*>(dstvoid);

        // If all bytes of the uint32_t are the same, 
        // we can use memset for a faster fill
        // multiplying a single byte by the bit pattern 
        // 0x01010101 replicates that byte across all four 
        // bytes of the uint32_t
        const uint8_t b = uint8_t(pixel & 0xFF);
        if (pixel == repl_u8_u32(b)) {
            std::memset(dst32, b, count * sizeof(uint32_t));
            return;
        }

#if WAAS_HAS_NEON
        size_t i = 0;
        uint32x4_t v = vdupq_n_u32(pixel);

        for (; i + 16 <= count; i += 16) {
            vst1q_u32(dst32 + i + 0, v);
            vst1q_u32(dst32 + i + 4, v);
            vst1q_u32(dst32 + i + 8, v);
            vst1q_u32(dst32 + i + 12, v);
        }
        for (; i + 4 <= count; i += 4) {
            vst1q_u32(dst32 + i, v);
        }
        for (; i < count; ++i) {
            dst32[i] = pixel;
        }
#else
        // Fallback to scalar fill if there's any leftover
        // or if SIMD is not available
        for (size_t i = 0; i < count; ++i)
            dst32[i] = pixel;
#endif
    }
}

#endif // PIXELING_H_INCLUDED
