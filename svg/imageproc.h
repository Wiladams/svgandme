#pragma once

// Image processing routines
// The various image processing routines, implemented against the PixelArray interface
// These should be called by the filter program executor.


#include "maths.h"
#include "surface.h"
#include "wggeometry.h"

namespace waavs
{
    static INLINE float clamp01f(float v) noexcept
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    static INLINE uint8_t clamp_u8f(float v) noexcept
    {
        if (v <= 0.0f) return 0;
        if (v >= 255.0f) return 255;
        return (uint8_t)std::lround(v);
    }

    struct FColor4
    {
        float r;
        float g;
        float b;
        float a;
    };
}


namespace waavs
{
    // --------------------------------------------------------
    // Filter enum families for closed vocabularies
    // These values are emitted into mem[] as u32.
    // --------------------------------------------------------

    enum FilterBlendMode : uint32_t
    {
        FILTER_BLEND_NORMAL = 0,
        FILTER_BLEND_MULTIPLY,
        FILTER_BLEND_SCREEN,
        FILTER_BLEND_DARKEN,
        FILTER_BLEND_LIGHTEN,
        FILTER_BLEND_OVERLAY,
        FILTER_BLEND_COLOR_DODGE,
        FILTER_BLEND_COLOR_BURN,
        FILTER_BLEND_HARD_LIGHT,
        FILTER_BLEND_SOFT_LIGHT,
        FILTER_BLEND_DIFFERENCE,
        FILTER_BLEND_EXCLUSION
    };
}


// ============================================================================
// feGaussianBlur reference impl (3 box blurs), PixelArray based
// ============================================================================

namespace waavs {

    // Pack averaged channels back to PRGB32 (premultiplied stays premultiplied)
    static INLINE uint32_t packAvg(int sa, int sr, int sg, int sb, int div) noexcept
    {
        // Rounded divide
        const int half = div >> 1;
        int a = (sa + half) / div;
        int r = (sr + half) / div;
        int g = (sg + half) / div;
        int b = (sb + half) / div;

        a = waavs::clamp(a, 0, 255);
        r = waavs::clamp(r, 0, 255);
        g = waavs::clamp(g, 0, 255);
        b = waavs::clamp(b, 0, 255);

        return (uint32_t)((a << 24) | (r << 16) | (g << 8) | (b));
    }

    static INLINE void unpack(uint32_t p, int& a, int& r, int& g, int& b) noexcept
    {
        a = (int)((p >> 24) & 0xFF);
        r = (int)((p >> 16) & 0xFF);
        g = (int)((p >> 8) & 0xFF);
        b = (int)((p) & 0xFF);
    }

    static INLINE uint32_t pxRead32_Clamped(const Surface& img, int x, int y) noexcept
    {
        const int W = (int)img.width();
        const int H = (int)img.height();
        if (W <= 0 || H <= 0) return 0u;

        x = waavs::clamp(x, 0, W - 1);
        y = waavs::clamp(y, 0, H - 1);

        const uint32_t* row = (const uint32_t*)img.rowPointer((size_t)y);
        return row[(size_t)x];
    }

    static INLINE void pxWrite32_Unsafe(Surface& img, int x, int y, uint32_t v) noexcept
    {
        uint32_t* row = (uint32_t*)img.rowPointer((size_t)y);
        row[(size_t)x] = v;
    }

    // Convert desired sigma to N box sizes (odd), using common approximation.
    // See: "Fast almost-Gaussian filtering" (box blur approximation).
    static INLINE void boxesForGauss(double sigma, int n, int* sizesOut) noexcept
    {
        // Guard
        if (!(sigma > 0.0)) {
            for (int i = 0; i < n; ++i) sizesOut[i] = 1;
            return;
        }

        // Ideal box width
        const double wIdeal = std::sqrt((12.0 * sigma * sigma / (double)n) + 1.0);
        int wl = (int)std::floor(wIdeal);
        if ((wl & 1) == 0) wl--;                 // make odd
        int wu = wl + 2;

        const double mIdeal =
            (12.0 * sigma * sigma - (double)n * wl * wl - 4.0 * (double)n * wl - 3.0 * (double)n) /
            (-4.0 * wl - 4.0);

        int m = (int)std::round(mIdeal);
        m = waavs::clamp(m, 0, n);

        for (int i = 0; i < n; ++i)
            sizesOut[i] = (i < m) ? wl : wu;
    }

    // Horizontal box blur for a rect, clamped sampling.
    static void boxBlurH_PRGB32(Surface& dst, const Surface& src, int radius, const WGRectI& area) noexcept
    {
        if (radius <= 0) {
            // Copy only the area
            for (int y = area.y; y < area.y + area.h; ++y) {
                const uint32_t* srow = (const uint32_t*)src.rowPointer((size_t)y);
                uint32_t* drow = (uint32_t*)dst.rowPointer((size_t)y);
                std::memcpy(drow + area.x, srow + area.x, (size_t)area.w * 4u);
            }
            return;
        }

        const int div = 2 * radius + 1;

        for (int y = area.y; y < area.y + area.h; ++y)
        {
            uint32_t* drow = (uint32_t*)dst.rowPointer((size_t)y);

            int sa = 0, sr = 0, sg = 0, sb = 0;

            // init window for x = area.x
            const int x0 = area.x;
            for (int i = -radius; i <= radius; ++i) {
                int a, r, g, b;
                unpack(pxRead32_Clamped(src, x0 + i, y), a, r, g, b);
                sa += a; sr += r; sg += g; sb += b;
            }

            pxWrite32_Unsafe(dst, x0, y, packAvg(sa, sr, sg, sb, div));

            // slide
            for (int x = x0 + 1; x < area.x + area.w; ++x)
            {
                int aL, rL, gL, bL;
                int aR, rR, gR, bR;

                unpack(pxRead32_Clamped(src, x - radius - 1, y), aL, rL, gL, bL);
                unpack(pxRead32_Clamped(src, x + radius, y), aR, rR, gR, bR);

                sa += (aR - aL);
                sr += (rR - rL);
                sg += (gR - gL);
                sb += (bR - bL);

                pxWrite32_Unsafe(dst, x, y, packAvg(sa, sr, sg, sb, div));
            }
        }
    }

    // Vertical box blur for a rect, clamped sampling.
    static void boxBlurV_PRGB32(Surface& dst, const Surface& src, int radius, const WGRectI& area) noexcept
    {
        if (radius <= 0) {
            for (int y = area.y; y < area.y + area.h; ++y) {
                const uint32_t* srow = (const uint32_t*)src.rowPointer((size_t)y);
                uint32_t* drow = (uint32_t*)dst.rowPointer((size_t)y);
                std::memcpy(drow + area.x, srow + area.x, (size_t)area.w * 4u);
            }
            return;
        }

        const int div = 2 * radius + 1;

        // Process column-wise (but still write row-wise)
        for (int x = area.x; x < area.x + area.w; ++x)
        {
            int sa = 0, sr = 0, sg = 0, sb = 0;

            const int y0 = area.y;
            for (int i = -radius; i <= radius; ++i) {
                int a, r, g, b;
                unpack(pxRead32_Clamped(src, x, y0 + i), a, r, g, b);
                sa += a; sr += r; sg += g; sb += b;
            }

            pxWrite32_Unsafe(dst, x, y0, packAvg(sa, sr, sg, sb, div));

            for (int y = y0 + 1; y < area.y + area.h; ++y)
            {
                int aT, rT, gT, bT;
                int aB, rB, gB, bB;

                unpack(pxRead32_Clamped(src, x, y - radius - 1), aT, rT, gT, bT);
                unpack(pxRead32_Clamped(src, x, y + radius), aB, rB, gB, bB);

                sa += (aB - aT);
                sr += (rB - rT);
                sg += (gB - gT);
                sb += (bB - bT);

                pxWrite32_Unsafe(dst, x, y, packAvg(sa, sr, sg, sb, div));
            }
        }
    }


    static INLINE WGRectI computeAreaI(const Surface& img, const WGRectD &subr) noexcept
    {
        const int W = (int)img.width();
        const int H = (int)img.height();

        //if (!subr) return BLRectI(0, 0, W, H);

        int x = (int)std::floor(subr.x);
        int y = (int)std::floor(subr.y);
        int w = (int)std::ceil(subr.w);
        int h = (int)std::ceil(subr.h);

        // Clamp to image bounds
        if (w <= 0 || h <= 0) return WGRectI(0, 0, 0, 0);

        if (x < 0) { w += x; x = 0; }
        if (y < 0) { h += y; y = 0; }

        if (x > W) { x = W; w = 0; }
        if (y > H) { y = H; h = 0; }

        if (x + w > W) w = W - x;
        if (y + h > H) h = H - y;

        if (w <= 0 || h <= 0) return WGRectI(0, 0, 0, 0);
        return WGRectI(x, y, w, h);
    }
    



} // namespace waavs

// ======================================
// blending reference implementation
// ======================================


namespace waavs
{


    static INLINE FColor4 unpack_prgb32_to_float(uint32_t px) noexcept
    {
        FColor4 c{};
        c.a = float((px >> 24) & 0xFF) * (1.0f / 255.0f);
        c.r = float((px >> 16) & 0xFF) * (1.0f / 255.0f);
        c.g = float((px >> 8) & 0xFF) * (1.0f / 255.0f);
        c.b = float((px >> 0) & 0xFF) * (1.0f / 255.0f);
        return c;
    }

    static INLINE void unpremultiply_in_place(FColor4& c) noexcept
    {
        if (c.a > 0.0f)
        {
            const float invA = 1.0f / c.a;
            c.r = clamp01f(c.r * invA);
            c.g = clamp01f(c.g * invA);
            c.b = clamp01f(c.b * invA);
        }
        else
        {
            c.r = 0.0f;
            c.g = 0.0f;
            c.b = 0.0f;
        }
    }

    static INLINE uint32_t pack_float_to_prgb32(const FColor4& c_) noexcept
    {
        FColor4 c = c_;
        c.a = clamp01f(c.a);
        c.r = clamp01f(c.r);
        c.g = clamp01f(c.g);
        c.b = clamp01f(c.b);

        const float pr = c.r * c.a;
        const float pg = c.g * c.a;
        const float pb = c.b * c.a;

        const uint8_t A = clamp_u8f(c.a * 255.0f);
        const uint8_t R = clamp_u8f(pr * 255.0f);
        const uint8_t G = clamp_u8f(pg * 255.0f);
        const uint8_t B = clamp_u8f(pb * 255.0f);

        return (uint32_t(A) << 24) | (uint32_t(R) << 16) | (uint32_t(G) << 8) | uint32_t(B);
    }

    static INLINE float blend_channel_normal(float cb, float cs) noexcept
    {
        (void)cb;
        return cs;
    }

    static INLINE float blend_channel_multiply(float cb, float cs) noexcept
    {
        return cb * cs;
    }

    static INLINE float blend_channel_screen(float cb, float cs) noexcept
    {
        return cb + cs - cb * cs;
    }

    static INLINE float blend_channel_darken(float cb, float cs) noexcept
    {
        return (cb < cs) ? cb : cs;
    }

    static INLINE float blend_channel_lighten(float cb, float cs) noexcept
    {
        return (cb > cs) ? cb : cs;
    }

    static INLINE float blend_channel_overlay(float cb, float cs) noexcept
    {
        return (cb <= 0.5f) ? (2.0f * cb * cs)
            : (1.0f - 2.0f * (1.0f - cb) * (1.0f - cs));
    }

    static INLINE float blend_channel_hard_light(float cb, float cs) noexcept
    {
        return (cs <= 0.5f) ? (2.0f * cb * cs)
            : (1.0f - 2.0f * (1.0f - cb) * (1.0f - cs));
    }

    static INLINE float blend_channel_color_dodge(float cb, float cs) noexcept
    {
        if (cs >= 1.0f) return 1.0f;
        return clamp01f(cb / (1.0f - cs));
    }

    static INLINE float blend_channel_color_burn(float cb, float cs) noexcept
    {
        if (cs <= 0.0f) return 0.0f;
        return 1.0f - clamp01f((1.0f - cb) / cs);
    }

    static INLINE float blend_channel_soft_light(float cb, float cs) noexcept
    {
        if (cs <= 0.5f)
        {
            return cb - (1.0f - 2.0f * cs) * cb * (1.0f - cb);
        }

        float d;
        if (cb <= 0.25f)
            d = ((16.0f * cb - 12.0f) * cb + 4.0f) * cb;
        else
            d = std::sqrt(cb);

        return cb + (2.0f * cs - 1.0f) * (d - cb);
    }

    static INLINE float blend_channel_difference(float cb, float cs) noexcept
    {
        return std::fabs(cb - cs);
    }

    static INLINE float blend_channel_exclusion(float cb, float cs) noexcept
    {
        return cb + cs - 2.0f * cb * cs;
    }

    static INLINE float blend_channel(FilterBlendMode mode, float cb, float cs) noexcept
    {
        switch (mode)
        {
        default:
        case FILTER_BLEND_NORMAL:      return blend_channel_normal(cb, cs);
        case FILTER_BLEND_MULTIPLY:    return blend_channel_multiply(cb, cs);
        case FILTER_BLEND_SCREEN:      return blend_channel_screen(cb, cs);
        case FILTER_BLEND_DARKEN:      return blend_channel_darken(cb, cs);
        case FILTER_BLEND_LIGHTEN:     return blend_channel_lighten(cb, cs);
        case FILTER_BLEND_OVERLAY:     return blend_channel_overlay(cb, cs);
        case FILTER_BLEND_COLOR_DODGE: return blend_channel_color_dodge(cb, cs);
        case FILTER_BLEND_COLOR_BURN:  return blend_channel_color_burn(cb, cs);
        case FILTER_BLEND_HARD_LIGHT:  return blend_channel_hard_light(cb, cs);
        case FILTER_BLEND_SOFT_LIGHT:  return blend_channel_soft_light(cb, cs);
        case FILTER_BLEND_DIFFERENCE:  return blend_channel_difference(cb, cs);
        case FILTER_BLEND_EXCLUSION:   return blend_channel_exclusion(cb, cs);
        }
    }

    // Blend two non-premultiplied colors with source-over alpha composition.
    // backdrop = in1, source = in2
    static INLINE FColor4 feBlendPixel(FilterBlendMode mode, const FColor4& backdropPRGB, const FColor4& sourcePRGB) noexcept
    {
        FColor4 b = backdropPRGB;
        FColor4 s = sourcePRGB;

        unpremultiply_in_place(b);
        unpremultiply_in_place(s);

        const float ab = backdropPRGB.a;
        const float as = sourcePRGB.a;

        const float outA = as + ab * (1.0f - as);

        if (!(outA > 0.0f))
            return FColor4{ 0.0f, 0.0f, 0.0f, 0.0f };

        // Blend function result in straight color space.
        const float br = blend_channel(mode, b.r, s.r);
        const float bg = blend_channel(mode, b.g, s.g);
        const float bb = blend_channel(mode, b.b, s.b);

        // W3C compositing form for separable blend modes.
        const float outR_p =
            (1.0f - as) * ab * b.r +
            (1.0f - ab) * as * s.r +
            ab * as * br;

        const float outG_p =
            (1.0f - as) * ab * b.g +
            (1.0f - ab) * as * s.g +
            ab * as * bg;

        const float outB_p =
            (1.0f - as) * ab * b.b +
            (1.0f - ab) * as * s.b +
            ab * as * bb;

        FColor4 out{};
        out.a = outA;
        out.r = clamp01f(outR_p / outA);
        out.g = clamp01f(outG_p / outA);
        out.b = clamp01f(outB_p / outA);
        return out;
    }

    static INLINE uint32_t feBlendPixelPRGB32(FilterBlendMode mode, uint32_t backdrop, uint32_t source) noexcept
    {
        const FColor4 b = unpack_prgb32_to_float(backdrop);
        const FColor4 s = unpack_prgb32_to_float(source);
        const FColor4 o = feBlendPixel(mode, b, s);
        return pack_float_to_prgb32(o);
    }

    static INLINE void feBlendRowPRGB32(
        uint32_t* dst,
        const uint32_t* in1,
        const uint32_t* in2,
        int count,
        FilterBlendMode mode) noexcept
    {
        for (int i = 0; i < count; ++i)
            dst[i] = feBlendPixelPRGB32(mode, in1[i], in2[i]);
    }
}