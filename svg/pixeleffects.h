// pixeleffects.h
//
// WAAVS - SVG Filter Pixel Kernels (Surface-only API)
// ---------------------------------------------------
// This header defines the *raw* pixel effect kernels for SVG filter primitives,
// operating ONLY on Surface_ARGB32 from pixeling.h.
//
// No PixelArray appears in any public function signature.
// Your executor/binder can adapt PixelArray <-> Surface_ARGB32 in one place.
//
// Dependencies:
//   - coloring.h  : linear/sRGB conversions + Porter-Duff "over"
//   - pixeling.h  : ARGB32<->linear PRGBA pack/unpack, sampling, basic fills
//
// Notes:
//   - Surface_ARGB32 holds packed ARGB32 pixels in logical layout:
//       (A<<24)|(R<<16)|(G<<8)|B
//   - All math is performed in linear premultiplied space (ColorPRGBA).
//   - Parameters are assumed already converted into TILE PIXELS by the caller.
//
// ---------------------------------------------------

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>
#include <algorithm>

#include "definitions.h"
#include "nametable.h"  // InternedKey, PSNameTable::INTERN
#include "coloring.h"
#include "pixeling.h"
#include "wggeometry.h"



// If we don't want nametable.h here, replace InternedKey with enum
// for channel selectors (R,G,B,A) and drop the INTERN usage.


namespace waavs {
    /*
    // ------------------------------------------------------------
    // Rect + kernel context
    // ------------------------------------------------------------

    struct PixelKernelCtx {
        // ROI in tile pixel space; if roi.w/h <= 0 => full image.
        WGRectI roi{ 0,0,0,0 };

        // For effects that may sample at fractional coordinates (offset/displacement).
        bool allowSubpixel{ true };
    };



    static INLINE WGRectI roiOrFull(const PixelKernelCtx& k, const Surface_ARGB32& s) noexcept
    {
        if (k.roi.w > 0 && k.roi.h > 0)
            return intersection(k.roi, Surface_ARGB32_bounds(&s));

        return Surface_ARGB32_bounds(&s);
    }

    // ------------------------------------------------------------
    // Basic surface ops (ARGB32 packed)
    // ------------------------------------------------------------

    // surface_argb32_clear
    // 
    // Clear to transparent black (0x00000000)
    // You might be tempted to just memset width*height*4, but
    // but, the stride may be larger than width*4, because this surface
    // may be a sub-region of a larger image. So we need to clear row by row.
    // 
    // Note: memset is likely SIMD-optimized in libc, so this should be reasonably fast.
    //
    static INLINE void surface_argb32_clear(Surface_ARGB32* s) noexcept {
        for (int y = 0; y < s->height; ++y) {
            uint32_t* row = Surface_ARGB32_row_pointer(s, y);
            std::memset(row, 0, s->stride);
        }
    }

    // surface_copy_ARGB32
    //
    // Copy src to dst, assuming same dimensions.  Like clear, we need to copy row by row
    // 
    // Note: std::memcpy is likely SIMD-optimized in libc, so this should be reasonably fast.
    //
    static INLINE void surface_copy_ARGB32(Surface_ARGB32* dst, const Surface_ARGB32* src) noexcept
    {
        WAAVS_ASSERT(dst->width == src->width && dst->height == src->height);
        for (int y = 0; y < dst->height; ++y) {
            uint32_t* d = Surface_ARGB32_row_pointer(dst, y);
            const uint32_t* s = Surface_ARGB32_row_pointer_const(src, y);
            std::memcpy(d, s, (size_t)dst->width * 4u);
        }
    }

    static INLINE void surface_copy_rect_ARGB32(Surface_ARGB32* dst, const Surface_ARGB32* src, WGRectI r) noexcept
    {
        // clamp r to dst/src (assumed same dims)
        if (r.x < 0) { r.w += r.x; r.x = 0; }
        if (r.y < 0) { r.h += r.y; r.y = 0; }
        if (r.x + r.w > dst->width)  r.w = dst->width - r.x;
        if (r.y + r.h > dst->height) r.h = dst->height - r.y;
        if (r.w <= 0 || r.h <= 0) return;

        for (int y = 0; y < r.h; ++y) {
            uint32_t* d = Surface_ARGB32_row_pointer(dst, r.y + y) + r.x;
            const uint32_t* s = Surface_ARGB32_row_pointer_const(src, r.y + y) + r.x;
            std::memcpy(d, s, (size_t)r.w * 4u);
        }
    }

    // ------------------------------------------------------------
    // Channel selector helper (for feDisplacementMap)
    // ------------------------------------------------------------
    static INLINE float selectChannel01_linearStraight(ColorLinear c, InternedKey ch) noexcept {
        static InternedKey kR = PSNameTable::INTERN("R");
        static InternedKey kG = PSNameTable::INTERN("G");
        static InternedKey kB = PSNameTable::INTERN("B");
        static InternedKey kA = PSNameTable::INTERN("A");

        if (ch == kR) return clamp01f(c.r);
        if (ch == kG) return clamp01f(c.g);
        if (ch == kB) return clamp01f(c.b);
        if (ch == kA) return clamp01f(c.a);

        return clamp01f(c.a);
    }
    */
}


namespace waavs {
    /*
    // ============================================================
    // feFlood
    // ============================================================
    static INLINE bool feFlood(const PixelKernelCtx& k, Surface_ARGB32* out, Pixel_ARGB32 argb32_premul) noexcept
    {
        (void)k;
        for (int y = 0; y < out->height; ++y) {
            uint32_t* row = Surface_ARGB32_row_pointer(out, y);
            for (int x = 0; x < out->width; ++x) row[x] = argb32_premul;
        }
        return true;
    }
    */

    /*
    // ============================================================
    // feOffset
    // dxPx/dyPx in TILE PIXELS
    // ============================================================
    static INLINE bool feOffset(const PixelKernelCtx& k,
        const Surface_ARGB32* in,
        Surface_ARGB32* out,
        double dxPx, double dyPx) noexcept
    {
        surface_argb32_clear(out);

        const int W = in->width;
        const int H = in->height;

        const double fracX = dxPx - std::floor(dxPx);
        const double fracY = dyPx - std::floor(dyPx);

        const bool doSub =
            k.allowSubpixel &&
            ((std::fabs(fracX) > 1e-9) || (std::fabs(fracY) > 1e-9));

        if (!doSub) {
            const int dx = (int)std::lround(dxPx);
            const int dy = (int)std::lround(dyPx);

            int dstX0 = dx, dstY0 = dy;
            int srcX0 = 0, srcY0 = 0;
            int cw = W, ch = H;

            if (dstX0 < 0) { srcX0 = -dstX0; cw -= srcX0; dstX0 = 0; }
            if (dstY0 < 0) { srcY0 = -dstY0; ch -= srcY0; dstY0 = 0; }
            if (dstX0 + cw > W) cw = W - dstX0;
            if (dstY0 + ch > H) ch = H - dstY0;

            if (cw <= 0 || ch <= 0) return true;

            for (int y = 0; y < ch; ++y) {
                const uint32_t* s = Surface_ARGB32_row_pointer_const(in, srcY0 + y) + srcX0;
                uint32_t* d = Surface_ARGB32_row_pointer(out, dstY0 + y) + dstX0;
                std::memcpy(d, s, (size_t)cw * 4u);
            }
            return true;
        }

        // Bilinear path:
        // out(x,y) = sample(in, x - dx, y - dy)
        const float invWm1 = (W > 1) ? (1.0f / (float)(W - 1)) : 0.0f;
        const float invHm1 = (H > 1) ? (1.0f / (float)(H - 1)) : 0.0f;

        for (int y = 0; y < H; ++y) {
            uint32_t* drow = Surface_ARGB32_row_pointer(out, y);
            for (int x = 0; x < W; ++x) {
                const float sx = (float)((double)x - dxPx);
                const float sy = (float)((double)y - dyPx);
                const float u = (W > 1) ? (sx * invWm1) : 0.0f;
                const float v = (H > 1) ? (sy * invHm1) : 0.0f;

                const ColorPRGBA p = surface_ARGB32_sample_bilinear_prgba(in, u, v);
                drow[x] = pixeling_prgba_pack_ARGB32(p);
            }
        }

        return true;
    }
    */

    /*
    // ============================================================
    // feMerge (N -> 1)
    // ============================================================
    struct SurfaceSpan {
        const Surface_ARGB32* const* p{};
        uint32_t n{};
    };

    static INLINE bool feMerge(const PixelKernelCtx& k, SurfaceSpan inputs, Surface_ARGB32* out) noexcept {
        (void)k;
        surface_argb32_clear(out);

        const int W = out->width;
        const int H = out->height;

        for (uint32_t i = 0; i < inputs.n; ++i) {
            const Surface_ARGB32* src = inputs.p[i];
            if (!src) continue;
            WAAVS_ASSERT(src->width == W && src->height == H);

            for (int y = 0; y < H; ++y) {
                const uint32_t* srow = Surface_ARGB32_row_pointer_const(src, y);
                uint32_t* drow = Surface_ARGB32_row_pointer(out, y);
                for (int x = 0; x < W; ++x) {
                    const ColorPRGBA s = pixeling_ARGB32_unpack_prgba(srow[x]);
                    const ColorPRGBA d = pixeling_ARGB32_unpack_prgba(drow[x]);
                    const ColorPRGBA o = coloring_prgba_over(s, d);
                    drow[x] = pixeling_prgba_pack_ARGB32(o);
                }
            }
        }

        return true;
    }
    */

/*
    // ============================================================
    // feTile
    // ============================================================
    static INLINE bool feTile(const PixelKernelCtx& k, const Surface_ARGB32* in, Surface_ARGB32* out) noexcept {
        (void)k;
        const int W = out->width, H = out->height;
        const int SW = in->width, SH = in->height;
        if (SW <= 0 || SH <= 0) { surface_argb32_clear(out); return true; }

        for (int y = 0; y < H; ++y) {
            uint32_t* drow = Surface_ARGB32_row_pointer(out, y);
            const uint32_t* srow = Surface_ARGB32_row_pointer_const(in, y % SH);
            for (int x = 0; x < W; ++x)
                drow[x] = srow[x % SW];
        }
        return true;
    }
    */

    /*
    // ============================================================
    // feColorMatrix (4x5)
    // ============================================================




        static INLINE int16_t clamp_i16(int v) noexcept
        {
            if (v < -32768) return -32768;
            if (v > 32767) return  32767;
            return (int16_t)v;
        }

        static INLINE int32_t clamp_i32(long long v) noexcept
        {
            if (v < -2147483648LL) return -2147483648LL;
            if (v > 2147483647LL) return  2147483647LL;
            return (int32_t)v;
        }

        static INLINE ColorMatrixQ88 quantizeColorMatrixQ88(const float m[20]) noexcept
        {
            ColorMatrixQ88 q{};

            for (int row = 0; row < 4; ++row)
            {
                const int base = row * 5;

                q.c[base + 0] = clamp_i16((int)std::lround(m[base + 0] * 256.0f));
                q.c[base + 1] = clamp_i16((int)std::lround(m[base + 1] * 256.0f));
                q.c[base + 2] = clamp_i16((int)std::lround(m[base + 2] * 256.0f));
                q.c[base + 3] = clamp_i16((int)std::lround(m[base + 3] * 256.0f));

                // Bias is in normalized color space in SVG-like math.
                // Convert bias to byte domain first, then to Q8.8.
                q.c[base + 4] = (int16_t)clamp_i32((long long)std::lround(m[base + 4] * 255.0f * 256.0f));
            }

            return q;
        }

        static INLINE uint32_t packARGB32(uint32_t a, uint32_t r, uint32_t g, uint32_t b) noexcept
        {
            return (a << 24) | (r << 16) | (g << 8) | b;
        }

        static INLINE uint32_t colorMatrixPixel_scalar_q88(uint32_t px, const ColorMatrixQ88& m) noexcept
        {
            const int r = (int)((px >> 16) & 0xFFu);
            const int g = (int)((px >> 8) & 0xFFu);
            const int b = (int)((px) & 0xFFu);
            const int a = (int)((px >> 24) & 0xFFu);

            auto evalRow = [&](int base) -> uint32_t
                {
                    int acc =
                        int(m.c[base + 0]) * r +
                        int(m.c[base + 1]) * g +
                        int(m.c[base + 2]) * b +
                        int(m.c[base + 3]) * a +
                        int(m.c[base + 4]);

                    // Round from Q8.8 to integer.
                    acc = (acc + 128) >> 8;

                    if (acc < 0)   acc = 0;
                    if (acc > 255) acc = 255;
                    return (uint32_t)acc;
                };

            const uint32_t rr = evalRow(0);
            const uint32_t gg = evalRow(5);
            const uint32_t bb = evalRow(10);
            const uint32_t aa = evalRow(15);

            return packARGB32(aa, rr, gg, bb);
        }

#if defined(__aarch64__) && defined(__ARM_NEON)

        static INLINE int32x4_t evalRow4_q88_neon(
            const int16x4_t r4,
            const int16x4_t g4,
            const int16x4_t b4,
            const int16x4_t a4,
            const int16_t* row) noexcept
        {
            int32x4_t acc = vdupq_n_s32((int32_t)row[4]);

            acc = vmlal_n_s16(acc, r4, row[0]);
            acc = vmlal_n_s16(acc, g4, row[1]);
            acc = vmlal_n_s16(acc, b4, row[2]);
            acc = vmlal_n_s16(acc, a4, row[3]);

            // Round and shift Q8.8 -> integer.
            acc = vaddq_s32(acc, vdupq_n_s32(128));
            acc = vshrq_n_s32(acc, 8);

            return acc;
        }

        static INLINE uint32x4_t packARGB4_u32(
            const uint32x4_t aa,
            const uint32x4_t rr,
            const uint32x4_t gg,
            const uint32x4_t bb) noexcept
        {
            uint32x4_t out = vshlq_n_u32(aa, 24);
            out = vorrq_u32(out, vshlq_n_u32(rr, 16));
            out = vorrq_u32(out, vshlq_n_u32(gg, 8));
            out = vorrq_u32(out, bb);
            return out;
        }

        static INLINE void colorMatrixRow_neon_q88(
            uint32_t* dst,
            const uint32_t* src,
            size_t count,
            const ColorMatrixQ88& m) noexcept
        {
            size_t i = 0;

            for (; i + 8 <= count; i += 8)
            {
                const uint32x4_t p0 = vld1q_u32(src + i + 0);
                const uint32x4_t p1 = vld1q_u32(src + i + 4);

                // Extract channels from packed ARGB32.
                const uint32x4_t a0_u32 = vshrq_n_u32(p0, 24);
                const uint32x4_t r0_u32 = vandq_u32(vshrq_n_u32(p0, 16), vdupq_n_u32(0xFF));
                const uint32x4_t g0_u32 = vandq_u32(vshrq_n_u32(p0, 8), vdupq_n_u32(0xFF));
                const uint32x4_t b0_u32 = vandq_u32(p0, vdupq_n_u32(0xFF));

                const uint32x4_t a1_u32 = vshrq_n_u32(p1, 24);
                const uint32x4_t r1_u32 = vandq_u32(vshrq_n_u32(p1, 16), vdupq_n_u32(0xFF));
                const uint32x4_t g1_u32 = vandq_u32(vshrq_n_u32(p1, 8), vdupq_n_u32(0xFF));
                const uint32x4_t b1_u32 = vandq_u32(p1, vdupq_n_u32(0xFF));

                // Narrow 32 -> 16. Values are guaranteed 0..255.
                const uint16x4_t a0_u16 = vmovn_u32(a0_u32);
                const uint16x4_t r0_u16 = vmovn_u32(r0_u32);
                const uint16x4_t g0_u16 = vmovn_u32(g0_u32);
                const uint16x4_t b0_u16 = vmovn_u32(b0_u32);

                const uint16x4_t a1_u16 = vmovn_u32(a1_u32);
                const uint16x4_t r1_u16 = vmovn_u32(r1_u32);
                const uint16x4_t g1_u16 = vmovn_u32(g1_u32);
                const uint16x4_t b1_u16 = vmovn_u32(b1_u32);

                // Combine low/high groups to 8 lanes, then reinterpret as signed.
                const int16x8_t a_u16 = vreinterpretq_s16_u16(vcombine_u16(a0_u16, a1_u16));
                const int16x8_t r_u16 = vreinterpretq_s16_u16(vcombine_u16(r0_u16, r1_u16));
                const int16x8_t g_u16 = vreinterpretq_s16_u16(vcombine_u16(g0_u16, g1_u16));
                const int16x8_t b_u16 = vreinterpretq_s16_u16(vcombine_u16(b0_u16, b1_u16));

                const int16x4_t r_lo = vget_low_s16(r_u16);
                const int16x4_t g_lo = vget_low_s16(g_u16);
                const int16x4_t b_lo = vget_low_s16(b_u16);
                const int16x4_t a_lo = vget_low_s16(a_u16);

                const int16x4_t r_hi = vget_high_s16(r_u16);
                const int16x4_t g_hi = vget_high_s16(g_u16);
                const int16x4_t b_hi = vget_high_s16(b_u16);
                const int16x4_t a_hi = vget_high_s16(a_u16);

                int32x4_t rr_lo = evalRow4_q88_neon(r_lo, g_lo, b_lo, a_lo, &m.c[0]);
                int32x4_t gg_lo = evalRow4_q88_neon(r_lo, g_lo, b_lo, a_lo, &m.c[5]);
                int32x4_t bb_lo = evalRow4_q88_neon(r_lo, g_lo, b_lo, a_lo, &m.c[10]);
                int32x4_t aa_lo = evalRow4_q88_neon(r_lo, g_lo, b_lo, a_lo, &m.c[15]);

                int32x4_t rr_hi = evalRow4_q88_neon(r_hi, g_hi, b_hi, a_hi, &m.c[0]);
                int32x4_t gg_hi = evalRow4_q88_neon(r_hi, g_hi, b_hi, a_hi, &m.c[5]);
                int32x4_t bb_hi = evalRow4_q88_neon(r_hi, g_hi, b_hi, a_hi, &m.c[10]);
                int32x4_t aa_hi = evalRow4_q88_neon(r_hi, g_hi, b_hi, a_hi, &m.c[15]);

                // Clamp signed 32 -> unsigned 16.
                const uint16x4_t rr0 = vqmovun_s32(rr_lo);
                const uint16x4_t gg0 = vqmovun_s32(gg_lo);
                const uint16x4_t bb0 = vqmovun_s32(bb_lo);
                const uint16x4_t aa0 = vqmovun_s32(aa_lo);

                const uint16x4_t rr1 = vqmovun_s32(rr_hi);
                const uint16x4_t gg1 = vqmovun_s32(gg_hi);
                const uint16x4_t bb1 = vqmovun_s32(bb_hi);
                const uint16x4_t aa1 = vqmovun_s32(aa_hi);

                const uint32x4_t out0 = packARGB4_u32(
                    vmovl_u16(aa0),
                    vmovl_u16(rr0),
                    vmovl_u16(gg0),
                    vmovl_u16(bb0));

                const uint32x4_t out1 = packARGB4_u32(
                    vmovl_u16(aa1),
                    vmovl_u16(rr1),
                    vmovl_u16(gg1),
                    vmovl_u16(bb1));

                vst1q_u32(dst + i + 0, out0);
                vst1q_u32(dst + i + 4, out1);
            }

            for (; i < count; ++i)
                dst[i] = colorMatrixPixel_scalar_q88(src[i], m);
        }

#endif


    static INLINE bool feColorMatrix(const PixelKernelCtx& k,
        const Surface_ARGB32* in,
        Surface_ARGB32* out,
        const float* m20, uint32_t count) noexcept
    {
        (void)k;
        if (!m20 || count != 20) { surface_copy_ARGB32(out, in); return true; }

        const int W = in->width;
        const int H = in->height;

        for (int y = 0; y < H; ++y) {
            const uint32_t* srow = Surface_ARGB32_row_pointer_const(in, y);
            uint32_t* drow = Surface_ARGB32_row_pointer(out, y);

            for (int x = 0; x < W; ++x) {
                const ColorPRGBA sp = pixeling_ARGB32_unpack_prgba(srow[x]);
                const ColorLinear s = coloring_linear_unpremultiply(sp);

                const float r = WAAVS_FMAF(m20[0], s.r, WAAVS_FMAF(m20[1], s.g, WAAVS_FMAF(m20[2], s.b, WAAVS_FMAF(m20[3], s.a, m20[4]))));
                const float g = WAAVS_FMAF(m20[5], s.r, WAAVS_FMAF(m20[6], s.g, WAAVS_FMAF(m20[7], s.b, WAAVS_FMAF(m20[8], s.a, m20[9]))));
                const float b = WAAVS_FMAF(m20[10], s.r, WAAVS_FMAF(m20[11], s.g, WAAVS_FMAF(m20[12], s.b, WAAVS_FMAF(m20[13], s.a, m20[14]))));
                const float a = WAAVS_FMAF(m20[15], s.r, WAAVS_FMAF(m20[16], s.g, WAAVS_FMAF(m20[17], s.b, WAAVS_FMAF(m20[18], s.a, m20[19]))));

                ColorLinear o{};
                o.r = WAAVS_CLAMP01(r);
                o.g = WAAVS_CLAMP01(g);
                o.b = WAAVS_CLAMP01(b);
                o.a = WAAVS_CLAMP01(a);

                drow[x] = pixeling_prgba_pack_ARGB32(coloring_linear_premultiply(o));
            }
        }

        return true;
    }
    */



    /*
    // ============================================================
    // feComposite
    // ============================================================

    enum CompositeOp : uint8_t {
        COMP_OVER,
        COMP_IN,
        COMP_OUT,
        COMP_ATOP,
        COMP_XOR,
        COMP_ARITHMETIC
    };

    static INLINE bool feComposite(const PixelKernelCtx& k,
        const Surface_ARGB32* in1,      // source
        const Surface_ARGB32* in2,     // backdrop
        Surface_ARGB32* out,
        CompositeOp op,
        float k1, float k2, float k3, float k4) noexcept
    {
        (void)k;

        const int W = out->width;
        const int H = out->height;

        for (int y = 0; y < H; ++y) {
            const uint32_t* sRow = Surface_ARGB32_row_pointer_const(in1, y);
            const uint32_t* dRow = Surface_ARGB32_row_pointer_const(in2, y);
            uint32_t* oRow = Surface_ARGB32_row_pointer(out, y);

            for (int x = 0; x < W; ++x) {
                const ColorPRGBA sP = pixeling_ARGB32_unpack_prgba(sRow[x]);
                const ColorPRGBA dP = pixeling_ARGB32_unpack_prgba(dRow[x]);

                ColorPRGBA oP{};

                if (op == COMP_ARITHMETIC) {
                    // arithmetic is defined on *straight* RGBA per component
                    const ColorLinear s = coloring_linear_unpremultiply(sP);
                    const ColorLinear d = coloring_linear_unpremultiply(dP);

                    ColorLinear o{};
                    o.r = WAAVS_CLAMP01(k1 * s.r * d.r + k2 * s.r + k3 * d.r + k4);
                    o.g = WAAVS_CLAMP01(k1 * s.g * d.g + k2 * s.g + k3 * d.g + k4);
                    o.b = WAAVS_CLAMP01(k1 * s.b * d.b + k2 * s.b + k3 * d.b + k4);
                    o.a = WAAVS_CLAMP01(k1 * s.a * d.a + k2 * s.a + k3 * d.a + k4);

                    oP = coloring_linear_premultiply(o);
                }
                else {
                    switch (op) {
                    case COMP_OVER: oP = coloring_prgba_over(sP, dP); break;
                    case COMP_IN:   oP = prgba_in(sP, dP); break;
                    case COMP_OUT:  oP = prgba_out(sP, dP); break;
                    case COMP_ATOP: oP = prgba_atop(sP, dP); break;
                    case COMP_XOR:  oP = prgba_xor(sP, dP); break;
                    default:        oP = coloring_prgba_over(sP, dP); break;
                    }
                }

                oRow[x] = pixeling_prgba_pack_ARGB32(oP);
            }
        }

        return true;
    }
    */

/*
    // ============================================================
    // feDisplacementMap
    // scalePx in TILE PIXELS
    // x/yChannel: InternedKey "R","G","B","A"
    // ============================================================
    static INLINE bool feDisplacementMap(const PixelKernelCtx& k,
        const Surface_ARGB32* in1,
        const Surface_ARGB32* in2,
        Surface_ARGB32* out,
        float scalePx,
        InternedKey xChannel,
        InternedKey yChannel) noexcept
    {
        (void)k;
        const int W = out->width;
        const int H = out->height;

        const float invWm1 = (W > 1) ? (1.0f / (float)(W - 1)) : 0.0f;
        const float invHm1 = (H > 1) ? (1.0f / (float)(H - 1)) : 0.0f;

        for (int y = 0; y < H; ++y) {
            const uint32_t* mRow = Surface_ARGB32_row_pointer_const(in2, y);
            uint32_t* oRow = Surface_ARGB32_row_pointer(out, y);

            for (int x = 0; x < W; ++x) {
                const ColorPRGBA mp = pixeling_ARGB32_unpack_prgba(mRow[x]);
                const ColorLinear ms = coloring_linear_unpremultiply(mp);

                const float cx = selectChannel01_linearStraight(ms, xChannel);
                const float cy = selectChannel01_linearStraight(ms, yChannel);

                const float dx = (cx - 0.5f) * scalePx;
                const float dy = (cy - 0.5f) * scalePx;

                const float sx = (float)x + dx;
                const float sy = (float)y + dy;

                const float u = (W > 1) ? (sx * invWm1) : 0.0f;
                const float v = (H > 1) ? (sy * invHm1) : 0.0f;

                const ColorPRGBA p = pixeling_ARGB32_sample_bilinear_prgba(in1, u, v);
                oRow[x] = pixeling_prgba_pack_ARGB32(p);
            }
        }

        return true;
    }
    */

/*
    // ============================================================
    // feMorphology (erode/dilate), simple reference
    // ============================================================
    enum MorphologyOp : uint8_t { MORPH_ERODE, MORPH_DILATE };

    static INLINE bool feMorphology(const PixelKernelCtx& k,
        const Surface_ARGB32* in,
        Surface_ARGB32* out,
        MorphologyOp op,
        int rxPx, int ryPx) noexcept
    {
        (void)k;
        rxPx = (rxPx < 0) ? 0 : rxPx;
        ryPx = (ryPx < 0) ? 0 : ryPx;

        if (rxPx == 0 && ryPx == 0) { surface_copy_ARGB32(out, in); return true; }

        const int W = in->width;
        const int H = in->height;

        for (int y = 0; y < H; ++y) {
            uint32_t* oRow = Surface_ARGB32_row_pointer(out, y);

            const int y0 = (y - ryPx < 0) ? 0 : (y - ryPx);
            const int y1 = (y + ryPx >= H) ? (H - 1) : (y + ryPx);

            for (int x = 0; x < W; ++x) {
                const int x0 = (x - rxPx < 0) ? 0 : (x - rxPx);
                const int x1 = (x + rxPx >= W) ? (W - 1) : (x + rxPx);

                ColorPRGBA best{};
                if (op == MORPH_ERODE) best = ColorPRGBA{ 1,1,1,1 };
                else                  best = ColorPRGBA{ 0,0,0,0 };

                for (int yy = y0; yy <= y1; ++yy) {
                    const uint32_t* sRow = Surface_ARGB32_row_pointer_const(in, yy);
                    for (int xx = x0; xx <= x1; ++xx) {
                        const ColorPRGBA p = pixeling_ARGB32_unpack_prgba(sRow[xx]);
                        if (op == MORPH_ERODE) {
                            best.r = (p.r < best.r) ? p.r : best.r;
                            best.g = (p.g < best.g) ? p.g : best.g;
                            best.b = (p.b < best.b) ? p.b : best.b;
                            best.a = (p.a < best.a) ? p.a : best.a;
                        }
                        else {
                            best.r = (p.r > best.r) ? p.r : best.r;
                            best.g = (p.g > best.g) ? p.g : best.g;
                            best.b = (p.b > best.b) ? p.b : best.b;
                            best.a = (p.a > best.a) ? p.a : best.a;
                        }
                    }
                }

                oRow[x] = pixeling_prgba_pack_ARGB32(best);
            }
        }

        return true;
    }
    */


    /*
    // ============================================================
    // feImage kernel is just copy (binding resolves href -> surface)
    // ============================================================
    static INLINE bool feImage(const PixelKernelCtx& k, const Surface_ARGB32* src, Surface_ARGB32* out) noexcept {
        (void)k;
        surface_copy_ARGB32(out, src);
        return true;
    }
    */

/*
    // ============================================================
    // Stubs for larger primitives (keep signatures Surface-only)
    // ============================================================
    static INLINE bool feConvolveMatrix(const PixelKernelCtx&,
        const Surface_ARGB32*,
        Surface_ARGB32*,
        uint32_t, uint32_t,
        const float*, uint32_t,
        float, float,
        uint32_t, uint32_t,
        InternedKey,
        float, float,
        bool) noexcept
    {
        return false;
    }
    */

/*
    static INLINE bool feTurbulence(const PixelKernelCtx&,
        Surface_ARGB32*,
        InternedKey, float, float,
        uint32_t, float, bool) noexcept
    {
        return false;
    }
    */

/*
    //struct LightPayload { float L[8]{}; };

    static INLINE bool feDiffuseLighting(const PixelKernelCtx&,
        const Surface_ARGB32*,
        Surface_ARGB32*,
        uint32_t, float, float,
        float, float,
        uint32_t, const LightPayload&) noexcept
    {
        return false;
    }
    */

/*
    static INLINE bool feSpecularLighting(const PixelKernelCtx&,
        const Surface_ARGB32*,
        Surface_ARGB32*,
        uint32_t, float, float, float,
        float, float,
        uint32_t, const LightPayload&) noexcept
    {
        return false;
    }
    */

/*
    static INLINE bool feDropShadow(const PixelKernelCtx&,
        const Surface_ARGB32*,
        Surface_ARGB32*,
        double, double,
        double, double,
        Pixel_ARGB32) noexcept
    {
        return false;
    }
    */
} // namespace waavs