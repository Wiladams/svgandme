#ifndef PIXELING_H_INCLUDED
#define PIXELING_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <math.h>       /* fmaf; link with -lm on *nix */

#include "definitions.h"
#include "coloring.h"
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
    // a fast multiply of two 8-bit values, returning an 8-bit result, with rounding
    static INLINE uint32_t mul255(uint32_t x, uint32_t y) noexcept
    {
        uint32_t t = x * y + 128;
        return (t + (t >> 8)) >> 8;
    }

    static INLINE uint32_t wg_div255_u32(uint32_t x) noexcept
    {
        return (x + 128u + ((x + 128u) >> 8)) >> 8;
    }


#if defined(_M_ARM64) || defined(__aarch64__)

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

    // pack_argb32()
    // 
    // Convert the given float values into 8-bit components and
    // stuff them into a uint32_t
    // 
    // NOTE:  This routine does NOT do any pre-multiplication.  It 
    // assumes any pre-multiplication has already occured before calling
    // into here
    static INLINE uint32_t pack_argb32(float a, float r, float g, float b) noexcept
    {
        // clamp to range [0,1] before converting to 8-bit integers.
        r = clamp01(r);
        g = clamp01(g);
        b = clamp01(b);
        a = clamp01(a);

        const uint32_t A = (uint32_t)float_to_u8(a);
        const uint32_t R = (uint32_t)float_to_u8(r);
        const uint32_t G = (uint32_t)float_to_u8(g);
        const uint32_t B = (uint32_t)float_to_u8(b);

        return (A << 24) | (R << 16) | (G << 8) | B;
    }

    static INLINE uint32_t pack_argb32(uint8_t a, uint8_t r, uint8_t g, uint8_t b) noexcept
    {
        return (uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
    }

    static INLINE uint32_t pack_argb32(uint32_t a, uint32_t r, uint32_t g, uint32_t b) noexcept
    {
        return ((a) << 24) | ((r) << 16) | ((g) << 8) | (b);
    }




    static INLINE void argb32_unpack(uint32_t px, uint8_t& a, uint8_t& r, uint8_t& g, uint8_t& b) noexcept
    {
        a = (uint8_t)((px >> 24) & 0xFF);
        r = (uint8_t)((px >> 16) & 0xFF);
        g = (uint8_t)((px >> 8) & 0xFF);
        b = (uint8_t)(px & 0xFF);
    }

    static INLINE void argb32_unpack(uint32_t px, int& a, int& r, int& g, int& b) noexcept
    {
        a = (int)((px >> 24) & 0xFF);
        r = (int)((px >> 16) & 0xFF);
        g = (int)((px >> 8) & 0xFF);
        b = (int)(px & 0xFF);
    }

    static INLINE void argb32_unpack(uint32_t px, uint32_t& a, uint32_t& r, uint32_t& g, uint32_t& b) noexcept
    {
        a = (uint32_t)((px >> 24) & 0xFF);
        r = (uint32_t)((px >> 16) & 0xFF);
        g = (uint32_t)((px >> 8) & 0xFF);
        b = (uint32_t)(px & 0xFF);
    }

    // Get only the alpha channel from a PRGB32 pixel 
    // Useful for some operations where we only care about alpha
    static INLINE uint32_t argb32_unpack_alpha(uint32_t p) noexcept
    {
        return (p >> 24) & 0xFF;
    }
}

//------------------------------------------------------------------------- 
// Pack/unpack between PRGBA (linear floats) and ARGB32/RGBA32 (sRGB premul) 
// ------------------------------------------------------------------------- 


// PRGBA (linear) -> ARGB32 (sRGB premultiplied) 
static inline Pixel_ARGB32 pixeling_prgba_pack_ARGB32(const ColorPRGBA p)
{
    // Convert to straight sRGB, then re-apply alpha in sRGB for packing 
    const ColorLinear lin = coloring_linear_unpremultiply(p);
    const ColorSRGB   s = coloring_linear_to_srgb(lin);

    const float A = WAAVS_CLAMP01(p.a);
    const uint32_t A8 = (uint32_t)(A * 255.0f + 0.5f);
    const uint32_t R8 = (uint32_t)(WAAVS_CLAMP01(s.r) * A * 255.0f + 0.5f);
    const uint32_t G8 = (uint32_t)(WAAVS_CLAMP01(s.g) * A * 255.0f + 0.5f);
    const uint32_t B8 = (uint32_t)(WAAVS_CLAMP01(s.b) * A * 255.0f + 0.5f);

    return waavs::pack_argb32(A8 , R8 , G8 , B8); // [A:R:G:B] 
}

// ARGB32 (sRGB premultiplied) -> PRGBA (linear) 
static inline ColorPRGBA pixeling_ARGB32_unpack_prgba(const Pixel_ARGB32 px)
{
    const float inv = 1.0f / 255.0f;

    const float A = ((px >> 24) & 0xFF) * inv;
    float Rs = ((px >> 16) & 0xFF) * inv;
    float Gs = ((px >> 8) & 0xFF) * inv;
    float Bs = (px & 0xFF) * inv;

    if (A > 0.0f) { 
        Rs /= A; Gs /= A; Bs /= A; 
    } else { 
        Rs = 0.0f; Gs = 0.0f; Bs = 0.0f; 
    }

    const float Rl = coloring_srgb_component_to_linear(Rs);
    const float Gl = coloring_srgb_component_to_linear(Gs);
    const float Bl = coloring_srgb_component_to_linear(Bs);

    ColorPRGBA p = { Rl * A, Gl * A, Bl * A, A };
    return p;
}

// PRGBA (linear) -> RGBA32 (sRGB premultiplied) 
static inline Pixel_RGBA32 pixeling_prgba_pack_RGBA32(const ColorPRGBA p)
{
    const ColorLinear lin = coloring_linear_unpremultiply(p);
    const ColorSRGB   s = coloring_linear_to_srgb(lin);

    const float A = WAAVS_CLAMP01(p.a);
    const uint32_t A8 = (uint32_t)(A * 255.0f + 0.5f);
    const uint32_t R8 = (uint32_t)(WAAVS_CLAMP01(s.r) * A * 255.0f + 0.5f);
    const uint32_t G8 = (uint32_t)(WAAVS_CLAMP01(s.g) * A * 255.0f + 0.5f);
    const uint32_t B8 = (uint32_t)(WAAVS_CLAMP01(s.b) * A * 255.0f + 0.5f);

    return (R8 << 24) | (G8 << 16) | (B8 << 8) | A8; // [R:G:B:A] 
}

// RGBA32 (sRGB premultiplied) -> PRGBA (linear) 
static inline ColorPRGBA pixeling_RGBA32_unpack_prgba(const Pixel_RGBA32 px)
{
    const float inv = 1.0f / 255.0f;

    float R8 = (float)((px >> 24) & 0xFF);
    float G8 = (float)((px >> 16) & 0xFF);
    float B8 = (float)((px >> 8) & 0xFF);
    float A8 = (float)(px & 0xFF);

    const float A = A8 * inv;
    float Rs = (A > 0.0f) ? (R8 / A8) * inv : 0.0f;
    float Gs = (A > 0.0f) ? (G8 / A8) * inv : 0.0f;
    float Bs = (A > 0.0f) ? (B8 / A8) * inv : 0.0f;

    const float Rl = coloring_srgb_component_to_linear(Rs);
    const float Gl = coloring_srgb_component_to_linear(Gs);
    const float Bl = coloring_srgb_component_to_linear(Bs);

    ColorPRGBA p = { Rl * A, Gl * A, Bl * A, A };
    return p;
}

// ------------------------------------------------------------- 
// Straight sRGBA8 helpers (no premultiplication; asset handling)
// ------------------------------------------------------------- 

static inline Pixel_SRGBA8_ARGB32 pixeling_srgba_pack_ARGB32(const ColorSRGB s)
{
    const uint32_t A = (uint32_t)(WAAVS_CLAMP01(s.a) * 255.0f + 0.5f);
    const uint32_t R = (uint32_t)(WAAVS_CLAMP01(s.r) * 255.0f + 0.5f);
    const uint32_t G = (uint32_t)(WAAVS_CLAMP01(s.g) * 255.0f + 0.5f);
    const uint32_t B = (uint32_t)(WAAVS_CLAMP01(s.b) * 255.0f + 0.5f);
    return (A << 24) | (R << 16) | (G << 8) | B;
}

static inline ColorSRGB pixeling_ARGB32_unpack_srgba(const Pixel_SRGBA8_ARGB32 px)
{
    const float inv = 1.0f / 255.0f;
    ColorSRGB s;
    s.a = ((px >> 24) & 0xFF) * inv;
    s.r = ((px >> 16) & 0xFF) * inv;
    s.g = ((px >> 8) & 0xFF) * inv;
    s.b = (px & 0xFF) * inv;
    return s;
}

static inline Pixel_SRGBA8_RGBA32 pixeling_srgba_pack_RGBA32(const ColorSRGB s)
{
    const uint32_t A = (uint32_t)(WAAVS_CLAMP01(s.a) * 255.0f + 0.5f);
    const uint32_t R = (uint32_t)(WAAVS_CLAMP01(s.r) * 255.0f + 0.5f);
    const uint32_t G = (uint32_t)(WAAVS_CLAMP01(s.g) * 255.0f + 0.5f);
    const uint32_t B = (uint32_t)(WAAVS_CLAMP01(s.b) * 255.0f + 0.5f);
    return (R << 24) | (G << 16) | (B << 8) | A;
}

static inline ColorSRGB pixeling_RGBA32_unpack_srgba(const Pixel_SRGBA8_RGBA32 px)
{
    const float inv = 1.0f / 255.0f;
    ColorSRGB s;
    s.r = ((px >> 24) & 0xFF) * inv;
    s.g = ((px >> 16) & 0xFF) * inv;
    s.b = ((px >> 8) & 0xFF) * inv;
    s.a = (px & 0xFF) * inv;
    return s;
}

// ----------------------------------------------------
// Surface type + rows + basic fills/blends for ARGB32 
// ----------------------------------------------------

typedef struct {
    uint8_t* data;          // base pointer
    int32_t  width;         // in pixels
    int32_t  height;        // in pixels
    intptr_t stride;        // in bytes between rows
    bool     contiguous;    // whether the memory is contiguous (no gap between rows)
} Surface_ARGB32;

// rectangle convenience
static INLINE WGRectI Surface_ARGB32_bounds(const Surface_ARGB32 * s) noexcept
{
    return WGRectI{ 0, 0, s->width, s->height };
}

static  INLINE uint32_t* Surface_ARGB32_row_pointer(const Surface_ARGB32* s, int y) {
    return (uint32_t*)(s->data + ((size_t)y * (size_t)s->stride));
}

static  INLINE const uint32_t* Surface_ARGB32_row_pointer_const(const Surface_ARGB32* s, int y) {
    return (const uint32_t*)(s->data + (size_t)y * (size_t)s->stride);
}

// Fill a span with a constant PRGBA color
static inline void Surface_ARGB32_fill_span(uint32_t* dst, int n, const ColorPRGBA c)
{
    const uint32_t px = pixeling_prgba_pack_ARGB32(c);
    for (int i = 0; i < n; ++i) 
        dst[i] = px;
}

/* Fill a rectangle (clamped) with a constant PRGBA color */
static inline void Surface_ARGB32_fill_rect(Surface_ARGB32* s,
    int x, int y, int w, int h,
    const ColorPRGBA c)
{
    // get the intersection of the rect with the surface bounds, 
    // and skip if empty
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > s->width)  w = s->width - x;
    if (y + h > s->height) h = s->height - y;
    if (w <= 0 || h <= 0) return;

    // Convert color value to a packed pixel
    // Then fill each row of the rectangle with that pixel value
    const uint32_t px = pixeling_prgba_pack_ARGB32(c);
    for (int j = 0; j < h; ++j) {
        uint32_t* row = Surface_ARGB32_row_pointer(s, y + j) + x;
        for (int i = 0; i < w; ++i) row[i] = px;
    }
}

/* Blend (src OVER dst) across a packed ARGB32 span with a constant PRGBA src */
static inline void pixeling_prgba_over_span_ARGB32(const ColorPRGBA src, uint32_t* dst, int n)
{
    for (int i = 0; i < n; ++i) {
        const ColorPRGBA d = pixeling_ARGB32_unpack_prgba(dst[i]);
        const ColorPRGBA o = coloring_prgba_over(src, d);
        dst[i] = pixeling_prgba_pack_ARGB32(o);
    }
}

// ----------------------------------------------------
// Sampling and resampling                             
// ----------------------------------------------------

// Bilinear sample from ARGB32 surface (normalized 0..1 UV), returns linear PRGBA */
static inline ColorPRGBA pixeling_ARGB32_sample_bilinear_prgba(const Surface_ARGB32* s,
    float u, float v)
{
    /* clamp to edge */
    if (u < 0.0f) u = 0.0f; else if (u > 1.0f) u = 1.0f;
    if (v < 0.0f) v = 0.0f; else if (v > 1.0f) v = 1.0f;

    const float fx = u * (float)(s->width - 1);
    const float fy = v * (float)(s->height - 1);
    const int x0 = (int)fx, y0 = (int)fy;
    const int x1 = (x0 + 1 < s->width) ? (x0 + 1) : x0;
    const int y1 = (y0 + 1 < s->height) ? (y0 + 1) : y0;
    const float tx = fx - (float)x0, ty = fy - (float)y0;

    const uint32_t* r0 = Surface_ARGB32_row_pointer_const(s, y0);
    const uint32_t* r1 = Surface_ARGB32_row_pointer_const(s, y1);

    const ColorPRGBA c00 = pixeling_ARGB32_unpack_prgba(r0[x0]);
    const ColorPRGBA c10 = pixeling_ARGB32_unpack_prgba(r0[x1]);
    const ColorPRGBA c01 = pixeling_ARGB32_unpack_prgba(r1[x0]);
    const ColorPRGBA c11 = pixeling_ARGB32_unpack_prgba(r1[x1]);

    /* lerp horizontally then vertically (premultiplied linear) */
    const ColorPRGBA a = coloring_prgba_lerp(c00, c10, tx);
    const ColorPRGBA b = coloring_prgba_lerp(c01, c11, tx);
    return coloring_prgba_lerp(a, b, ty);
}

/* 2x box downsample: src ARGB32 -> dst ARGB32 (sizes should be ceil(src/2)) */
static inline void pixeling_ARGB32_downsample2x_ARGB32(const Surface_ARGB32* src,
    Surface_ARGB32* dst)
{
    for (int y = 0; y < dst->height; ++y) {
        const int sy = y * 2;
        const int sy1 = (sy + 1 < src->height) ? (sy + 1) : sy;

        const uint32_t* r0 = Surface_ARGB32_row_pointer_const(src, sy);
        const uint32_t* r1 = Surface_ARGB32_row_pointer_const(src, sy1);
        uint32_t* rd = Surface_ARGB32_row_pointer(dst, y);

        for (int x = 0; x < dst->width; ++x) {
            const int sx = x * 2;
            const int sx1 = (sx + 1 < src->width) ? (sx + 1) : sx;

            const uint32_t p00 = r0[sx];
            const uint32_t p10 = r0[sx1];
            const uint32_t p01 = r1[sx];
            const uint32_t p11 = r1[sx1];

            const ColorPRGBA c00 = pixeling_ARGB32_unpack_prgba(p00);
            const ColorPRGBA c10 = pixeling_ARGB32_unpack_prgba(p10);
            const ColorPRGBA c01 = pixeling_ARGB32_unpack_prgba(p01);
            const ColorPRGBA c11 = pixeling_ARGB32_unpack_prgba(p11);

            /* average premultiplied components */
            const ColorPRGBA m = {
                0.25f * (c00.r + c10.r + c01.r + c11.r),
                0.25f * (c00.g + c10.g + c01.g + c11.g),
                0.25f * (c00.b + c10.b + c01.b + c11.b),
                0.25f * (c00.a + c10.a + c01.a + c11.a)
            };
            rd[x] = pixeling_prgba_pack_ARGB32(m);
        }
    }
}

// ---------------------------------------------------- */
// Optional LUT path for faster sRGB->linear for 8-bit  */
// ---------------------------------------------------- */

typedef struct {
    float toLinear[256];
} Pixeling_SRGBLUT;

static inline void pixeling_srgb_lut_init(Pixeling_SRGBLUT* L)
{
    for (int i = 0; i < 256; ++i) {
        const float s = (float)i / 255.0f;
        L->toLinear[i] = coloring_srgb_component_to_linear(s);
    }
}

static Pixeling_SRGBLUT * pixeling_srgb_lut_get_default()
{
    static Pixeling_SRGBLUT defaultLUT;
    static int initialized = 0;
    if (!initialized) {
        pixeling_srgb_lut_init(&defaultLUT);
        initialized = 1;
    }
    return &defaultLUT;
}


static inline ColorPRGBA pixeling_ARGB32_unpack_prgba_LUT(const Pixel_ARGB32 px)
{
    const Pixeling_SRGBLUT* L = pixeling_srgb_lut_get_default();

    const float inv = 1.0f / 255.0f;
    const int   A8 = (px >> 24) & 0xFF;
    const int   R8 = (px >> 16) & 0xFF;
    const int   G8 = (px >> 8) & 0xFF;
    const int   B8 = px & 0xFF;
    const float A = (float)A8 * inv;

    float Rs = (A8 ? ((float)R8 * inv) / A : 0.0f);
    float Gs = (A8 ? ((float)G8 * inv) / A : 0.0f);
    float Bs = (A8 ? ((float)B8 * inv) / A : 0.0f);

    int Ri = (int)(Rs * 255.0f + 0.5f); if (Ri < 0) Ri = 0; else if (Ri > 255) Ri = 255;
    int Gi = (int)(Gs * 255.0f + 0.5f); if (Gi < 0) Gi = 0; else if (Gi > 255) Gi = 255;
    int Bi = (int)(Bs * 255.0f + 0.5f); if (Bi < 0) Bi = 0; else if (Bi > 255) Bi = 255;

    ColorPRGBA p = { L->toLinear[Ri] * A, L->toLinear[Gi] * A, L->toLinear[Bi] * A, A };
    return p;
}

INLINE void memset_l(void* dstvoid, uint32_t pixel, size_t count) noexcept
{
    if (count == 0)
        return;

    uint32_t* dst32 = static_cast<uint32_t*>(dstvoid);
    // If all bytes of the pixel are the same, 
    // we can use memset for a faster fill
    const uint8_t b = uint8_t(pixel & 0xFF);
    if (pixel == (uint32_t(b) * 0x01010101u)) {
        std::memset(dst32, b, count * sizeof(uint32_t));
        return;
    }

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
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
    for (size_t i = 0; i < count; ++i)
        dst32[i] = pixel;
#endif
}

/*
// Used to rapidly copy 32 bit values 
static void memset_l(void* adr, uint32_t val, size_t count)
{
    uint32_t v;
    size_t i, n;
    uint32_t* p;
    p = static_cast<uint32_t*>(adr);
    v = val;

    // Do 4 at a time
    n = count >> 2;
    for (i = 0; i < n; i++)
    {
        p[0] = v;
        p[1] = v;
        p[2] = v;
        p[3] = v;
        p += 4;
    }

    // Copy the last remaining values
    n = count & 3;
    for (i = 0; i < n; i++)
        *p++ = val;
}
*/

#endif // PIXELING_H_INCLUDED
