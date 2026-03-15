#pragma once

// Image processing routines
// The various image processing routines, implemented against the PixelArray interface
// These should be called by the filter program executor.


#include "maths.h"
#include "surface.h"
#include "wggeometry.h"

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

