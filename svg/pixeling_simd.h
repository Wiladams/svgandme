#ifndef PIXELING_SIMD_H_INCLUDED
#define PIXELING_SIMD_H_INCLUDED

/*
 * pixeling_simd.h — SIMD-accelerated pixel ops for WAAVS
 *
 * Accuracy note:
 *   Functions here that operate *entirely in 8-bit sRGB premultiplied*
 *   (e.g., ARGB32 OVER ARGB32) are FAST but not linear-light correct.
 *   They are useful for UI, previews, or where exact photometric correctness
 *   isn't required. For correctness, unpack to linear PRGBA floats, blend,
 *   then pack (see coloring.h + pixeling.h).
 *
 * Naming: pixeling_<SOURCE>_<operation>_<TARGET>[_backend]
 *   - SOURCE/TARGET are pixel word layouts by bit order, e.g., ARGB32:
 *       ARGB32 = (A<<24)|(R<<16)|(G<<8)|B
 *
 * Backends included:
 *   - Scalar fallback (always available)
 *   - SSE2  (x86/x64)   : process 4 pixels per iter
 *   - NEON  (ARM/AArch64): process 8 pixels per iter
 *
 * Requires:
 *   - coloring.h and pixeling.h for types/helpers if you intermix with linear path
 */

#include <stdint.h>
#include <stddef.h>

 // ------------------------------------------
 // Fast OVER in sRGB premultiplied 8-bit path 
 // out = src + dst*(1 - src.a)                
 // All channels treated uniformly (including A)
 // ------------------------------------------

 // Scalar reference implementation (always built)
static inline void pixeling_ARGB32_over_span_ARGB32_fast_scalar(uint32_t src_px,
    uint32_t* dst, int n)
{
    const uint32_t srcA = (src_px >> 24) & 0xFF;
    const uint32_t invA = 255u - srcA;

    for (int i = 0; i < n; ++i) {
        uint32_t d = dst[i];

        // per-channel: out = src + round(dst*invA/255) 
        uint32_t dA = (d >> 24) & 0xFF;
        uint32_t dR = (d >> 16) & 0xFF;
        uint32_t dG = (d >> 8) & 0xFF;
        uint32_t dB = d & 0xFF;

        uint32_t tA = dA * invA; tA = (tA + 128u); tA = (tA + (tA >> 8)) >> 8; // ~ round(/255) 
        uint32_t tR = dR * invA; tR = (tR + 128u); tR = (tR + (tR >> 8)) >> 8;
        uint32_t tG = dG * invA; tG = (tG + 128u); tG = (tG + (tG >> 8)) >> 8;
        uint32_t tB = dB * invA; tB = (tB + 128u); tB = (tB + (tB >> 8)) >> 8;

        uint32_t oA = srcA + tA; if (oA > 255u) oA = 255u;
        uint32_t oR = ((src_px >> 16) & 0xFF) + tR; if (oR > 255u) oR = 255u;
        uint32_t oG = ((src_px >> 8) & 0xFF) + tG; if (oG > 255u) oG = 255u;
        uint32_t oB = (src_px & 0xFF) + tB; if (oB > 255u) oB = 255u;

        dst[i] = (oA << 24) | (oR << 16) | (oG << 8) | oB;
    }
}

#if defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(_M_X64)
//-------------------------
// SSE2 implementation       
//-------------------------
#include <emmintrin.h>
static inline void pixeling_ARGB32_over_span_ARGB32_fast_sse2(uint32_t src_px,
    uint32_t* dst, int n)
{
    const __m128i zero = _mm_setzero_si128();
    const __m128i src32 = _mm_set1_epi32((int)src_px);
    const __m128i src_lo = _mm_unpacklo_epi8(src32, zero); // 8x u16: [A R G B | A R G B] 
    const __m128i src_hi = _mm_unpackhi_epi8(src32, zero); // 8x u16: [A R G B | A R G B] 
    const int srcA = (int)((src_px >> 24) & 0xFF);
    const int invA = 255 - srcA;
    const __m128i invA16 = _mm_set1_epi16((short)invA);
    const __m128i bias = _mm_set1_epi16(128);
    const __m128i max255 = _mm_set1_epi16(255);

    int i = 0;
    for (; i + 4 <= n; i += 4) {
        __m128i d = _mm_loadu_si128((const __m128i*)(dst + i)); // 4 pixels 

        __m128i d_lo = _mm_unpacklo_epi8(d, zero); // 8 u16 
        __m128i d_hi = _mm_unpackhi_epi8(d, zero); // 8 u16 

        // m = dst * invA 
        __m128i m_lo = _mm_mullo_epi16(d_lo, invA16);
        __m128i m_hi = _mm_mullo_epi16(d_hi, invA16);

        // divide by 255 with rounding: (x + 128 + ((x + 128) >> 8)) >> 8 
        __m128i t_lo = _mm_add_epi16(m_lo, bias);
        __m128i t_hi = _mm_add_epi16(m_hi, bias);
        __m128i q_lo = _mm_add_epi16(t_lo, _mm_srli_epi16(t_lo, 8));
        __m128i q_hi = _mm_add_epi16(t_hi, _mm_srli_epi16(t_hi, 8));
        __m128i dst_scaled_lo = _mm_srli_epi16(q_lo, 8);
        __m128i dst_scaled_hi = _mm_srli_epi16(q_hi, 8);

        // out = src + dst_scaled 
        __m128i out_lo = _mm_add_epi16(dst_scaled_lo, src_lo);
        __m128i out_hi = _mm_add_epi16(dst_scaled_hi, src_hi);

        // clamp to 255, repack 
        out_lo = _mm_min_epi16(out_lo, max255);
        out_hi = _mm_min_epi16(out_hi, max255);
        __m128i out = _mm_packus_epi16(out_lo, out_hi);

        _mm_storeu_si128((__m128i*)(dst + i), out);
    }
    if (i < n) {
        pixeling_ARGB32_over_span_ARGB32_fast_scalar(src_px, dst + i, n - i);
    }
}
#endif // SSE2 

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
// ------------------------- 
// NEON implementation       
// ------------------------- 
#include <arm_neon.h>
static inline void pixeling_ARGB32_over_span_ARGB32_fast_neon(uint32_t src_px,
    uint32_t* dst, int n)
{
    const uint8x16_t src_dup = vreinterpretq_u8_u32(vdupq_n_u32(src_px));
    const uint8x8_t  src_dup_lo = vget_low_u8(src_dup);
    const uint8x8_t  src_dup_hi = vget_high_u8(src_dup);
    const uint16x8_t src_lo_u16 = vmovl_u8(src_dup_lo); // expand to u16 
    const uint16x8_t src_hi_u16 = vmovl_u8(src_dup_hi);

    const uint8_t srcA = (uint8_t)(src_px >> 24);
    const uint8_t invA = (uint8_t)(255 - srcA);
    const uint16x8_t invA16 = vdupq_n_u16(invA);
    const uint16x8_t bias = vdupq_n_u16(128);
    const uint16x8_t max255 = vdupq_n_u16(255);

    int i = 0;
    for (; i + 8 <= n; i += 8) {
        uint8x16_t d8 = vld1q_u8((const uint8_t*)(dst + i)); // 8 pixels 
        uint8x8_t d8_lo = vget_low_u8(d8);
        uint8x8_t d8_hi = vget_high_u8(d8);
        uint16x8_t d_lo = vmovl_u8(d8_lo);
        uint16x8_t d_hi = vmovl_u8(d8_hi);

        // m = dst * invA 
        uint16x8_t m_lo = vmulq_u16(d_lo, invA16);
        uint16x8_t m_hi = vmulq_u16(d_hi, invA16);

        // divide by 255 with rounding 
        uint16x8_t t_lo = vaddq_u16(m_lo, bias);
        uint16x8_t t_hi = vaddq_u16(m_hi, bias);
        uint16x8_t q_lo = vaddq_u16(t_lo, vshrq_n_u16(t_lo, 8));
        uint16x8_t q_hi = vaddq_u16(t_hi, vshrq_n_u16(t_hi, 8));
        uint16x8_t dst_scaled_lo = vshrq_n_u16(q_lo, 8);
        uint16x8_t dst_scaled_hi = vshrq_n_u16(q_hi, 8);

        // out = src + dst_scaled 
        uint16x8_t out_lo = vaddq_u16(dst_scaled_lo, src_lo_u16);
        uint16x8_t out_hi = vaddq_u16(dst_scaled_hi, src_hi_u16);

        // clamp and pack 
        out_lo = vminq_u16(out_lo, max255);
        out_hi = vminq_u16(out_hi, max255);
        uint8x8_t out8_lo = vqmovn_u16(out_lo);
        uint8x8_t out8_hi = vqmovn_u16(out_hi);
        vst1q_u8((uint8_t*)(dst + i), vcombine_u8(out8_lo, out8_hi));
    }
    if (i < n) {
        pixeling_ARGB32_over_span_ARGB32_fast_scalar(src_px, dst + i, n - i);
    }
}
#endif // NEON 

// ---------------------------------- 
// Front-end that picks best backend. 
// ---------------------------------- 
static inline void pixeling_ARGB32_over_span_ARGB32_fast(uint32_t src_px,
    uint32_t* dst, int n)
{
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
    pixeling_ARGB32_over_span_ARGB32_fast_neon(src_px, dst, n);
#elif defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(_M_X64)
    pixeling_ARGB32_over_span_ARGB32_fast_sse2(src_px, dst, n);
#else
    pixeling_ARGB32_over_span_ARGB32_fast_scalar(src_px, dst, n);
#endif
}

#endif // PIXELING_SIMD_H_INCLUDED 

