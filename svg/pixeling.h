#ifndef PIXELING_H_INCLUDED
#define PIXELING_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <math.h>       /* fmaf; link with -lm on *nix */

#include "coloring.h"


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

    return (A8 << 24) | (R8 << 16) | (G8 << 8) | B8; // [A:R:G:B] 
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

/* ------------------------------------------------------------- */
/* Straight sRGBA8 helpers (no premultiplication; asset handling)*/
/* ------------------------------------------------------------- */

typedef uint32_t Pixel_SRGBA8_ARGB32; /* [A:R:G:B] straight sRGB */
typedef uint32_t Pixel_SRGBA8_RGBA32; /* [R:G:B:A] straight sRGB */

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
    uint8_t* data;   // base pointer
    int      width;  // in pixels
    int      height; // in pixels
    int      stride; // in bytes between rows
} Surface_ARGB32;

static inline uint32_t* pixeling_ARGB32_row_ptr(Surface_ARGB32* s, int y) {
    return (uint32_t*)(s->data + (size_t)y * (size_t)s->stride);
}

static inline const uint32_t* pixeling_ARGB32_row_ptr_const(const Surface_ARGB32* s, int y) {
    return (const uint32_t*)(s->data + (size_t)y * (size_t)s->stride);
}

/* Fill a span with a constant PRGBA color */
static inline void pixeling_prgba_fill_span_ARGB32(uint32_t* dst, int n, const ColorPRGBA c)
{
    const uint32_t px = pixeling_prgba_pack_ARGB32(c);
    for (int i = 0; i < n; ++i) dst[i] = px;
}

/* Fill a rectangle (clamped) with a constant PRGBA color */
static inline void pixeling_prgba_fill_rect_ARGB32(Surface_ARGB32* s,
    int x, int y, int w, int h,
    const ColorPRGBA c)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > s->width)  w = s->width - x;
    if (y + h > s->height) h = s->height - y;
    if (w <= 0 || h <= 0) return;

    const uint32_t px = pixeling_prgba_pack_ARGB32(c);
    for (int j = 0; j < h; ++j) {
        uint32_t* row = pixeling_ARGB32_row_ptr(s, y + j) + x;
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

/* ---------------------------------------------------- */
/* Sampling and resampling                              */
/* ---------------------------------------------------- */

/* Bilinear sample from ARGB32 surface (normalized 0..1 UV), returns linear PRGBA */
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

    const uint32_t* r0 = pixeling_ARGB32_row_ptr_const(s, y0);
    const uint32_t* r1 = pixeling_ARGB32_row_ptr_const(s, y1);

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

        const uint32_t* r0 = pixeling_ARGB32_row_ptr_const(src, sy);
        const uint32_t* r1 = pixeling_ARGB32_row_ptr_const(src, sy1);
        uint32_t* rd = pixeling_ARGB32_row_ptr(dst, y);

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

/* ---------------------------------------------------- */
/* Optional LUT path for faster sRGB->linear for 8-bit  */
/* ---------------------------------------------------- */

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


#endif // PIXELING_H_INCLUDED
