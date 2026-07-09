#ifndef PIXELING_SURFACE_H_INCLUDED
#define PIXELING_SURFACE_H_INCLUDED

/*
 * pixeling_surface.h — byte-stride surfaces for WAAVS
 *
 * Canonical rule: STRIDE IS IN BYTES.
 * This works for any bytes-per-pixel (bpp), e.g., RGB24 (3), ARGB32 (4).
 * Rows are padded up to an alignment boundary (e.g., 4, 16, 64 bytes).
 *
 * Provided:
 *   - Stride computation: pixeling_stride_bytes_for_width(width, bpp, align_bytes)
 *   - Owned surfaces + views for ARGB32 (4Bpp) and RGB24 (3Bpp)
 *   - Row/pixel pointer helpers (typed)
 *   - 64B-aligned allocations by default (configurable)
 *
 * Word-order reminder for ARGB32: (A<<24)|(R<<16)|(G<<8)|B (bit order, not byte order).
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "pixeling.h"   // for Pixel_ARGB32, pack/unpack if needed */
#include "waavs_mem.h" // for waavs_aligned_malloc/free




/* ---- generic stride computation (BYTES) ---- */
static inline int pixeling_stride_bytes_for_width(int width, int bytes_per_pixel, int align_bytes)
{
    size_t min_row = (size_t)width * (size_t)bytes_per_pixel;
    if (align_bytes <= 0) align_bytes = 1;
    size_t row = WAAVS_ALIGN_UP_POW2(min_row, (size_t)align_bytes);
    return (int)row;
}

/* ===================== ARGB32 (4 Bpp) ===================== */

typedef struct {
    uint8_t* data;   /* base pointer */
    int      width;  /* in pixels */
    int      height; /* in pixels */
    int      stride; /* BYTES between row starts */
} Surface_ARGB32;

typedef struct {
    void* mem;
    size_t          mem_size;
    Surface_ARGB32  view;
} SurfaceOwned_ARGB32;

static inline int pixeling_ARGB32_surface_owned_create(SurfaceOwned_ARGB32* out,
    int width, int height,
    int align_bytes, /* e.g., 64 */
    int zero_init)
{
    if (!out || width <= 0 || height <= 0) return 0;
    if (align_bytes <= 0) align_bytes = 64;

    const int stride = pixeling_stride_bytes_for_width(width, 4, align_bytes);
    const size_t size = (size_t)stride * (size_t)height;

    void* mem = waavs_aligned_malloc(size, (size_t)align_bytes);
    if (!mem) return 0;
    if (zero_init) { uint8_t* p = (uint8_t*)mem; for (size_t i = 0; i < size; i++) p[i] = 0; }

    out->mem = mem;
    out->mem_size = size;
    out->view.data = (uint8_t*)mem;
    out->view.width = width;
    out->view.height = height;
    out->view.stride = stride;
    return 1;
}

static inline void pixeling_ARGB32_surface_owned_destroy(SurfaceOwned_ARGB32* s)
{
    if (!s) return;
    waavs_aligned_free(s->mem);
    s->mem = NULL; s->mem_size = 0;
    s->view.data = NULL; s->view.width = s->view.height = s->view.stride = 0;
}

/* typed row pointers */
static inline uint32_t* pixeling_ARGB32_row_ptr(Surface_ARGB32* s, int y)
{
    return (uint32_t*)(s->data + (size_t)y * (size_t)s->stride);
}
static inline const uint32_t* pixeling_ARGB32_row_ptr_const(const Surface_ARGB32* s, int y)
{
    return (const uint32_t*)(s->data + (size_t)y * (size_t)s->stride);
}

/* ===================== RGB24 (3 Bpp) ===================== */

typedef struct {
    uint8_t* data;   /* base pointer */
    int      width;  /* in pixels */
    int      height; /* in pixels */
    int      stride; /* BYTES between row starts (aligned, e.g., to 4) */
} Surface_RGB24;

typedef struct {
    void* mem;
    size_t         mem_size;
    Surface_RGB24  view;
} SurfaceOwned_RGB24;

static inline int pixeling_RGB24_surface_owned_create(SurfaceOwned_RGB24* out,
    int width, int height,
    int align_bytes, /* e.g., 4 or 64 */
    int zero_init)
{
    if (!out || width <= 0 || height <= 0) return 0;
    if (align_bytes <= 0) align_bytes = 4; /* typical for RGB24 */

    const int stride = pixeling_stride_bytes_for_width(width, 3, align_bytes);
    const size_t size = (size_t)stride * (size_t)height;

    void* mem = waavs_aligned_malloc(size, (size_t)align_bytes);
    if (!mem) return 0;
    if (zero_init) { uint8_t* p = (uint8_t*)mem; for (size_t i = 0; i < size; i++) p[i] = 0; }

    out->mem = mem;
    out->mem_size = size;
    out->view.data = (uint8_t*)mem;
    out->view.width = width;
    out->view.height = height;
    out->view.stride = stride;
    return 1;
}

static inline void pixeling_RGB24_surface_owned_destroy(SurfaceOwned_RGB24* s)
{
    if (!s) return;
    waavs_aligned_free(s->mem);
    s->mem = NULL; s->mem_size = 0;
    s->view.data = NULL; s->view.width = s->view.height = s->view.stride = 0;
}

/* Row and pixel pointers for RGB24 */
static inline uint8_t* pixeling_RGB24_row_ptr(Surface_RGB24* s, int y)
{
    return s->data + (size_t)y * (size_t)s->stride;
}
static inline const uint8_t* pixeling_RGB24_row_ptr_const(const Surface_RGB24* s, int y)
{
    return s->data + (size_t)y * (size_t)s->stride;
}
static inline uint8_t* pixeling_RGB24_px_ptr(Surface_RGB24* s, int x, int y)
{
    return s->data + (size_t)y * (size_t)s->stride + (size_t)x * 3u;
}
static inline const uint8_t* pixeling_RGB24_px_ptr_const(const Surface_RGB24* s, int x, int y)
{
    return s->data + (size_t)y * (size_t)s->stride + (size_t)x * 3u;
}

/* Example: pack a PRGBA color to straight sRGB24 (no alpha) with rounding */
static inline void pixeling_prgba_store_RGB24(ColorPRGBA c, Surface_RGB24* s, int x, int y)
{
    const ColorSRGB sr = coloring_linear_to_srgb(coloring_linear_unpremultiply(c));
    uint8_t* p = pixeling_RGB24_px_ptr(s, x, y);
    p[0] = (uint8_t)(WAAVS_CLAMP01(sr.r) * 255.0f + 0.5f);
    p[1] = (uint8_t)(WAAVS_CLAMP01(sr.g) * 255.0f + 0.5f);
    p[2] = (uint8_t)(WAAVS_CLAMP01(sr.b) * 255.0f + 0.5f);
}

#endif /* PIXELING_SURFACE_H_INCLUDED */

