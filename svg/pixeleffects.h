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
//   - imageproc.h : (optional) your box blur kernels + boxesForGauss
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
#include "imageproc.h"  // boxesForGauss, boxBlurH_PRGB32, boxBlurV_PRGB32 (optional)

// If we don't want nametable.h here, replace InternedKey with enum
// for channel selectors (R,G,B,A) and drop the INTERN usage.

namespace waavs {

    // ------------------------------------------------------------
    // Rect + kernel context
    // ------------------------------------------------------------




    struct PixelKernelCtx {
        // ROI in tile pixel space; if roi.w/h <= 0 => full image.
        WGRectI roi{ 0,0,0,0 };

        // For effects that may sample at fractional coordinates (offset/displacement).
        bool allowSubpixel{ true };
    };

    // rectangle convenience
    static INLINE WGRectI rectI_full(const Surface_ARGB32& s) noexcept 
    {
        return WGRectI{ 0, 0, s.width, s.height };
    }

    static INLINE WGRectI roiOrFull(const PixelKernelCtx& k, const Surface_ARGB32& s) noexcept 
    {
        if (k.roi.w > 0 && k.roi.h > 0) 
            return intersection(k.roi, rectI_full(s));
        
        return rectI_full(s);
    }

    // ------------------------------------------------------------
    // Basic surface ops (ARGB32 packed)
    // ------------------------------------------------------------

    // surface_clear_ARGB32
    // 
    // Clear to transparent black (0x00000000)
    // You might be tempted to just memset width*height*4, but
    // but, the stride may be larger than width*4, because this surface
    // may be a sub-region of a larger image. So we need to clear row by row.
    // 
    // Note: memset is likely SIMD-optimized in libc, so this should be reasonably fast.
    //
    static INLINE void surface_clear_ARGB32(Surface_ARGB32* s) noexcept {
        for (int y = 0; y < s->height; ++y) {
            uint32_t* row = pixeling_ARGB32_row_ptr(s, y);
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
            uint32_t* d = pixeling_ARGB32_row_ptr(dst, y);
            const uint32_t* s = pixeling_ARGB32_row_ptr_const(src, y);
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
            uint32_t* d = pixeling_ARGB32_row_ptr(dst, r.y + y) + r.x;
            const uint32_t* s = pixeling_ARGB32_row_ptr_const(src, r.y + y) + r.x;
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

        if (ch == kR) return WAAVS_CLAMP01(c.r);
        if (ch == kG) return WAAVS_CLAMP01(c.g);
        if (ch == kB) return WAAVS_CLAMP01(c.b);
        if (ch == kA) return WAAVS_CLAMP01(c.a);
        return WAAVS_CLAMP01(c.a);
    }

    // ------------------------------------------------------------
    // Porter-Duff helpers (linear PRGBA)
    // ------------------------------------------------------------
    static INLINE ColorPRGBA prgba_in(ColorPRGBA s, ColorPRGBA d) noexcept {
        const float a = d.a;
        return ColorPRGBA{ s.r * a, s.g * a, s.b * a, s.a * a };
    }
    static INLINE ColorPRGBA prgba_out(ColorPRGBA s, ColorPRGBA d) noexcept {
        const float ia = 1.0f - d.a;
        return ColorPRGBA{ s.r * ia, s.g * ia, s.b * ia, s.a * ia };
    }
    static INLINE ColorPRGBA prgba_atop(ColorPRGBA s, ColorPRGBA d) noexcept {
        const float Ad = d.a;
        const float iAs = 1.0f - s.a;
        return ColorPRGBA{
          s.r * Ad + d.r * iAs,
          s.g * Ad + d.g * iAs,
          s.b * Ad + d.b * iAs,
          s.a * Ad + d.a * iAs
        };
    }
    static INLINE ColorPRGBA prgba_xor(ColorPRGBA s, ColorPRGBA d) noexcept {
        const float iAd = 1.0f - d.a;
        const float iAs = 1.0f - s.a;
        return ColorPRGBA{
          s.r * iAd + d.r * iAs,
          s.g * iAd + d.g * iAs,
          s.b * iAd + d.b * iAs,
          s.a * iAd + d.a * iAs
        };
    }

    // ============================================================
    // feFlood
    // ============================================================
    static INLINE bool feFlood(const PixelKernelCtx& k,
        Surface_ARGB32* out,
        Pixel_ARGB32 argb32_premul) noexcept
    {
        (void)k;
        for (int y = 0; y < out->height; ++y) {
            uint32_t* row = pixeling_ARGB32_row_ptr(out, y);
            for (int x = 0; x < out->width; ++x) row[x] = argb32_premul;
        }
        return true;
    }

    // ============================================================
    // feOffset
    // dxPx/dyPx in TILE PIXELS
    // ============================================================
    static INLINE bool feOffset(const PixelKernelCtx& k,
        const Surface_ARGB32* in,
        Surface_ARGB32* out,
        double dxPx, double dyPx) noexcept
    {
        surface_clear_ARGB32(out);

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
                const uint32_t* s = pixeling_ARGB32_row_ptr_const(in, srcY0 + y) + srcX0;
                uint32_t* d = pixeling_ARGB32_row_ptr(out, dstY0 + y) + dstX0;
                std::memcpy(d, s, (size_t)cw * 4u);
            }
            return true;
        }

        // Bilinear path:
        // out(x,y) = sample(in, x - dx, y - dy)
        const float invWm1 = (W > 1) ? (1.0f / (float)(W - 1)) : 0.0f;
        const float invHm1 = (H > 1) ? (1.0f / (float)(H - 1)) : 0.0f;

        for (int y = 0; y < H; ++y) {
            uint32_t* drow = pixeling_ARGB32_row_ptr(out, y);
            for (int x = 0; x < W; ++x) {
                const float sx = (float)((double)x - dxPx);
                const float sy = (float)((double)y - dyPx);
                const float u = (W > 1) ? (sx * invWm1) : 0.0f;
                const float v = (H > 1) ? (sy * invHm1) : 0.0f;

                const ColorPRGBA p = pixeling_ARGB32_sample_bilinear_prgba(in, u, v);
                drow[x] = pixeling_prgba_pack_ARGB32(p);
            }
        }

        return true;
    }

    // ============================================================
    // feGaussianBlur
    //
    // Reference: copy-through outside ROI, blur only ROI.
    // Uses your existing boxBlurH_PRGB32/boxBlurV_PRGB32 which take PixelArray.
    //
    // To keep Surface-only API, this function instead provides a SURFACE-ONLY
    // fallback separable box blur implementation (not the fastest, but correct).
    //
    // If you want to keep using your existing optimized PixelArray blur code,
    // do that in the adapter layer (PixelArray->Surface) and keep this kernel
    // as the fallback/reference. This is fully isolated.
    // ============================================================

    static INLINE uint8_t clampU8(int v) noexcept {
        if (v < 0) return 0;
        if (v > 255) return 255;
        return (uint8_t)v;
    }

    // 1D box blur over ARGB32 packed by unpacking to PRGBA linear and averaging.
    // This is intentionally simple (O(r) per pixel). Replace with your fast blur
    // in the adapter layer if desired.
    static INLINE void boxBlurH_linearPRGBA(Surface_ARGB32* dst, const Surface_ARGB32* src,
        int r, WGRectI area) noexcept
    {
        const int W = src->width;
        for (int y = area.y; y < area.y + area.h; ++y) {
            const uint32_t* srow = pixeling_ARGB32_row_ptr_const(src, y);
            uint32_t* drow = pixeling_ARGB32_row_ptr(dst, y);

            for (int x = area.x; x < area.x + area.w; ++x) {
                ColorPRGBA acc{ 0,0,0,0 };
                int n = 0;
                const int x0 = (x - r < 0) ? 0 : (x - r);
                const int x1 = (x + r >= W) ? (W - 1) : (x + r);
                for (int xx = x0; xx <= x1; ++xx) {
                    const ColorPRGBA p = pixeling_ARGB32_unpack_prgba(srow[xx]);
                    acc.r += p.r; acc.g += p.g; acc.b += p.b; acc.a += p.a;
                    ++n;
                }
                const float inv = 1.0f / (float)n;
                acc.r *= inv; acc.g *= inv; acc.b *= inv; acc.a *= inv;
                drow[x] = pixeling_prgba_pack_ARGB32(acc);
            }
        }
    }

    static INLINE void boxBlurV_linearPRGBA(Surface_ARGB32* dst, const Surface_ARGB32* src,
        int r, WGRectI area) noexcept
    {
        const int H = src->height;
        for (int y = area.y; y < area.y + area.h; ++y) {
            uint32_t* drow = pixeling_ARGB32_row_ptr(dst, y);

            const int y0 = (y - r < 0) ? 0 : (y - r);
            const int y1 = (y + r >= H) ? (H - 1) : (y + r);

            for (int x = area.x; x < area.x + area.w; ++x) {
                ColorPRGBA acc{ 0,0,0,0 };
                int n = 0;
                for (int yy = y0; yy <= y1; ++yy) {
                    const uint32_t* srow = pixeling_ARGB32_row_ptr_const(src, yy);
                    const ColorPRGBA p = pixeling_ARGB32_unpack_prgba(srow[x]);
                    acc.r += p.r; acc.g += p.g; acc.b += p.b; acc.a += p.a;
                    ++n;
                }
                const float inv = 1.0f / (float)n;
                acc.r *= inv; acc.g *= inv; acc.b *= inv; acc.a *= inv;
                drow[x] = pixeling_prgba_pack_ARGB32(acc);
            }
        }
    }

    static INLINE bool feGaussianBlur(const PixelKernelCtx& k,
        const Surface_ARGB32* in,
        Surface_ARGB32* out,
        double sigmaXpx, double sigmaYpx) noexcept
    {
        // Copy-through first.
        surface_copy_ARGB32(out, in);

        const double sigma = (sigmaXpx > sigmaYpx) ? sigmaXpx : sigmaYpx;
        if (!(sigma > 0.0))
            return true;

        WGRectI area = roiOrFull(k, *in);
        if (area.w <= 0 || area.h <= 0)
            return true;

        int box[3];
        waavs::boxesForGauss(sigma, 3, box);

        // Temporary full-size surfaces (simple reference)
        // Allocate as raw buffers (caller can provide scratch in future).
        const size_t bytes = (size_t)in->stride * (size_t)in->height;
        uint8_t* mem0 = (uint8_t*)malloc(bytes);
        uint8_t* mem1 = (uint8_t*)malloc(bytes);
        if (!mem0 || !mem1) { if (mem0) free(mem0); if (mem1) free(mem1); return false; }

        Surface_ARGB32 tmp0{ mem0, in->width, in->height, in->stride };
        Surface_ARGB32 tmp1{ mem1, in->width, in->height, in->stride };

        // Start from input
        const Surface_ARGB32* curSrc = in;
        Surface_ARGB32* curDst = &tmp0;

        for (int pass = 0; pass < 3; ++pass) {
            const int r = (box[pass] - 1) / 2;

            boxBlurH_linearPRGBA(curDst, curSrc, r, area);

            curSrc = curDst;
            curDst = (curDst == &tmp0) ? &tmp1 : &tmp0;

            boxBlurV_linearPRGBA(curDst, curSrc, r, area);

            curSrc = curDst;
            curDst = (curDst == &tmp0) ? &tmp1 : &tmp0;
        }

        // Copy ROI from blurred temp into out.
        for (int y = area.y; y < area.y + area.h; ++y) {
            const uint32_t* srow = pixeling_ARGB32_row_ptr_const(curSrc, y);
            uint32_t* drow = pixeling_ARGB32_row_ptr(out, y);
            std::memcpy(drow + area.x, srow + area.x, (size_t)area.w * 4u);
        }

        free(mem0);
        free(mem1);
        return true;
    }

    // ============================================================
    // feMerge (N -> 1)
    // ============================================================
    struct SurfaceSpan {
        const Surface_ARGB32* const* p{};
        uint32_t n{};
    };

    static INLINE bool feMerge(const PixelKernelCtx& k, SurfaceSpan inputs, Surface_ARGB32* out) noexcept {
        (void)k;
        surface_clear_ARGB32(out);

        const int W = out->width;
        const int H = out->height;

        for (uint32_t i = 0; i < inputs.n; ++i) {
            const Surface_ARGB32* src = inputs.p[i];
            if (!src) continue;
            WAAVS_ASSERT(src->width == W && src->height == H);

            for (int y = 0; y < H; ++y) {
                const uint32_t* srow = pixeling_ARGB32_row_ptr_const(src, y);
                uint32_t* drow = pixeling_ARGB32_row_ptr(out, y);
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

    // ============================================================
    // feTile
    // ============================================================
    static INLINE bool feTile(const PixelKernelCtx& k, const Surface_ARGB32* in, Surface_ARGB32* out) noexcept {
        (void)k;
        const int W = out->width, H = out->height;
        const int SW = in->width, SH = in->height;
        if (SW <= 0 || SH <= 0) { surface_clear_ARGB32(out); return true; }

        for (int y = 0; y < H; ++y) {
            uint32_t* drow = pixeling_ARGB32_row_ptr(out, y);
            const uint32_t* srow = pixeling_ARGB32_row_ptr_const(in, y % SH);
            for (int x = 0; x < W; ++x)
                drow[x] = srow[x % SW];
        }
        return true;
    }

    // ============================================================
    // feColorMatrix (4x5)
    // ============================================================
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
            const uint32_t* srow = pixeling_ARGB32_row_ptr_const(in, y);
            uint32_t* drow = pixeling_ARGB32_row_ptr(out, y);

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

    // ============================================================
    // feComponentTransfer
    // ============================================================
    struct ComponentFunc {
        InternedKey typeKey{};
        float p0{}, p1{}, p2{};
        const float* table{};
        uint32_t tableN{};
    };

    static INLINE float applyTransferFunc(float v, const ComponentFunc& f) noexcept
    {
        static InternedKey kIdentity = PSNameTable::INTERN("identity");
        static InternedKey kTable = PSNameTable::INTERN("table");
        static InternedKey kDiscrete = PSNameTable::INTERN("discrete");
        static InternedKey kLinear = PSNameTable::INTERN("linear");
        static InternedKey kGamma = PSNameTable::INTERN("gamma");

        v = WAAVS_CLAMP01(v);
        const InternedKey t = f.typeKey;

        if (!t || t == kIdentity) return v;

        if (t == kLinear) {
            return WAAVS_CLAMP01(WAAVS_FMAF(f.p0, v, f.p1)); // slope, intercept
        }

        if (t == kGamma) {
            const float p = (v <= 0.0f) ? 0.0f : powf(v, f.p1); // exponent
            return WAAVS_CLAMP01(WAAVS_FMAF(f.p0, p, f.p2));    // amplitude, offset
        }

        if ((t == kTable || t == kDiscrete) && f.table && f.tableN) {
            if (f.tableN == 1) return WAAVS_CLAMP01(f.table[0]);

            const float tt = v * (float)(f.tableN - 1);
            if (t == kDiscrete) {
                const uint32_t idx = (uint32_t)(tt);
                const uint32_t i = (idx < f.tableN) ? idx : (f.tableN - 1);
                return WAAVS_CLAMP01(f.table[i]);
            }
            else {
                const uint32_t i0 = (uint32_t)std::floor(tt);
                const uint32_t i1 = (i0 + 1 < f.tableN) ? (i0 + 1) : i0;
                const float frac = tt - (float)i0;
                const float a = f.table[i0], b = f.table[i1];
                return WAAVS_CLAMP01(WAAVS_FMAF((b - a), frac, a));
            }
        }

        return v;
    }

    static INLINE bool feComponentTransfer(const PixelKernelCtx& k,
        const Surface_ARGB32* in,
        Surface_ARGB32* out,
        const ComponentFunc& fr,
        const ComponentFunc& fg,
        const ComponentFunc& fb,
        const ComponentFunc& fa) noexcept
    {
        (void)k;
        const int W = in->width;
        const int H = in->height;

        for (int y = 0; y < H; ++y) {
            const uint32_t* srow = pixeling_ARGB32_row_ptr_const(in, y);
            uint32_t* drow = pixeling_ARGB32_row_ptr(out, y);

            for (int x = 0; x < W; ++x) {
                const ColorPRGBA sp = pixeling_ARGB32_unpack_prgba(srow[x]);
                const ColorLinear s = coloring_linear_unpremultiply(sp);

                ColorLinear o{};
                o.r = applyTransferFunc(s.r, fr);
                o.g = applyTransferFunc(s.g, fg);
                o.b = applyTransferFunc(s.b, fb);
                o.a = applyTransferFunc(s.a, fa);

                drow[x] = pixeling_prgba_pack_ARGB32(coloring_linear_premultiply(o));
            }
        }

        return true;
    }

    // ============================================================
    // feBlend
    // ============================================================
    enum BlendMode : uint8_t {
        BLEND_NORMAL,
        BLEND_MULTIPLY,
        BLEND_SCREEN,
        BLEND_OVERLAY,
        BLEND_DARKEN,
        BLEND_LIGHTEN
    };

    static INLINE float blendFunc(float cb, float cs, BlendMode m) noexcept {
        switch (m) {
        case BLEND_MULTIPLY: return cb * cs;
        case BLEND_SCREEN:   return cb + cs - cb * cs;
        case BLEND_DARKEN:   return (cb < cs) ? cb : cs;
        case BLEND_LIGHTEN:  return (cb > cs) ? cb : cs;
        case BLEND_OVERLAY:  return (cb <= 0.5f) ? (2.0f * cb * cs) : (1.0f - 2.0f * (1.0f - cb) * (1.0f - cs));
        case BLEND_NORMAL:
        default:             return cs;
        }
    }

    static INLINE bool feBlend(const PixelKernelCtx& k,
        const Surface_ARGB32* in1 /* backdrop */,
        const Surface_ARGB32* in2 /* source */,
        Surface_ARGB32* out,
        BlendMode mode) noexcept
    {
        (void)k;
        const int W = out->width;
        const int H = out->height;

        for (int y = 0; y < H; ++y) {
            const uint32_t* bRow = pixeling_ARGB32_row_ptr_const(in1, y);
            const uint32_t* sRow = pixeling_ARGB32_row_ptr_const(in2, y);
            uint32_t* oRow = pixeling_ARGB32_row_ptr(out, y);

            for (int x = 0; x < W; ++x) {
                const ColorPRGBA bp = pixeling_ARGB32_unpack_prgba(bRow[x]);
                const ColorPRGBA sp = pixeling_ARGB32_unpack_prgba(sRow[x]);

                const ColorLinear b = coloring_linear_unpremultiply(bp);
                const ColorLinear s = coloring_linear_unpremultiply(sp);

                const float Ab = WAAVS_CLAMP01(b.a);
                const float As = WAAVS_CLAMP01(s.a);

                const float Br = blendFunc(b.r, s.r, mode);
                const float Bg = blendFunc(b.g, s.g, mode);
                const float Bb = blendFunc(b.b, s.b, mode);

                const float Ar = As + Ab - As * Ab;
                const float t0 = (1.0f - As);
                const float t1 = (1.0f - Ab);
                const float t2 = As * Ab;

                ColorLinear o{};
                o.a = WAAVS_CLAMP01(Ar);
                o.r = WAAVS_CLAMP01(t0 * b.r + t1 * s.r + t2 * Br);
                o.g = WAAVS_CLAMP01(t0 * b.g + t1 * s.g + t2 * Bg);
                o.b = WAAVS_CLAMP01(t0 * b.b + t1 * s.b + t2 * Bb);

                oRow[x] = pixeling_prgba_pack_ARGB32(coloring_linear_premultiply(o));
            }
        }

        return true;
    }

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
        const Surface_ARGB32* in1 /* source */,
        const Surface_ARGB32* in2 /* backdrop */,
        Surface_ARGB32* out,
        CompositeOp op,
        float k1, float k2, float k3, float k4) noexcept
    {
        (void)k;

        const int W = out->width;
        const int H = out->height;

        for (int y = 0; y < H; ++y) {
            const uint32_t* sRow = pixeling_ARGB32_row_ptr_const(in1, y);
            const uint32_t* dRow = pixeling_ARGB32_row_ptr_const(in2, y);
            uint32_t* oRow = pixeling_ARGB32_row_ptr(out, y);

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
            const uint32_t* mRow = pixeling_ARGB32_row_ptr_const(in2, y);
            uint32_t* oRow = pixeling_ARGB32_row_ptr(out, y);

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
            uint32_t* oRow = pixeling_ARGB32_row_ptr(out, y);

            const int y0 = (y - ryPx < 0) ? 0 : (y - ryPx);
            const int y1 = (y + ryPx >= H) ? (H - 1) : (y + ryPx);

            for (int x = 0; x < W; ++x) {
                const int x0 = (x - rxPx < 0) ? 0 : (x - rxPx);
                const int x1 = (x + rxPx >= W) ? (W - 1) : (x + rxPx);

                ColorPRGBA best{};
                if (op == MORPH_ERODE) best = ColorPRGBA{ 1,1,1,1 };
                else                  best = ColorPRGBA{ 0,0,0,0 };

                for (int yy = y0; yy <= y1; ++yy) {
                    const uint32_t* sRow = pixeling_ARGB32_row_ptr_const(in, yy);
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

    // ============================================================
    // feImage kernel is just copy (binding resolves href -> surface)
    // ============================================================
    static INLINE bool feImage(const PixelKernelCtx& k, const Surface_ARGB32* src, Surface_ARGB32* out) noexcept {
        (void)k;
        surface_copy_ARGB32(out, src);
        return true;
    }

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

    static INLINE bool feTurbulence(const PixelKernelCtx&,
        Surface_ARGB32*,
        InternedKey, float, float,
        uint32_t, float, bool) noexcept
    {
        return false;
    }

    struct LightPayload { float L[8]{}; };

    static INLINE bool feDiffuseLighting(const PixelKernelCtx&,
        const Surface_ARGB32*,
        Surface_ARGB32*,
        uint32_t, float, float,
        float, float,
        uint32_t, const LightPayload&) noexcept
    {
        return false;
    }

    static INLINE bool feSpecularLighting(const PixelKernelCtx&,
        const Surface_ARGB32*,
        Surface_ARGB32*,
        uint32_t, float, float, float,
        float, float,
        uint32_t, const LightPayload&) noexcept
    {
        return false;
    }

    static INLINE bool feDropShadow(const PixelKernelCtx&,
        const Surface_ARGB32*,
        Surface_ARGB32*,
        double, double,
        double, double,
        Pixel_ARGB32) noexcept
    {
        return false;
    }

} // namespace waavs