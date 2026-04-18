#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>
#include <algorithm>

#include "definitions.h"
#include "coloring.h"
#include "wggeometry.h"
#include "surface.h"

namespace waavs
{


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

    struct BoxBlurDivInfo
    {
        uint32_t div;
        uint32_t half;
        uint32_t recip32;
    };

    // Precompute division info for a given box blur radius, 
    // to avoid doing expensive division in the inner loop.
    static INLINE BoxBlurDivInfo makeBoxBlurDivInfo(int div) noexcept
    {
        if (div <= 0)
            div = 1;

        BoxBlurDivInfo di{};
        di.div = (uint32_t)div;
        di.half = (uint32_t)(div >> 1);

        // floor(2^32 / div)
        di.recip32 = (uint32_t)(((uint64_t)1 << 32) / (uint64_t)di.div);

        return di;
    }

    static INLINE uint32_t pxRead32_Clamped(const Surface& img, int x, int y) noexcept
    {
        const int W = (int)img.width();
        const int H = (int)img.height();
        if (W <= 0 || H <= 0) return 0u;

        x = clamp(x, 0, W - 1);
        y = clamp(y, 0, H - 1);

        const uint32_t* row = (const uint32_t*)img.rowPointer((size_t)y);
        return row[(size_t)x];
    }

    static INLINE void pxWrite32_Unsafe(Surface& img, int x, int y, uint32_t v) noexcept
    {
        uint32_t* row = (uint32_t*)img.rowPointer((size_t)y);
        row[(size_t)x] = v;
    }

    // Pack averaged channels back to PRGB32 (premultiplied stays premultiplied)
    static INLINE uint32_t packAvgDivInfo(
        int sa, int sr, int sg, int sb,
        const BoxBlurDivInfo& di) noexcept
    {
        const int a = (sa + (int)di.half) / (int)di.div;
        const int r = (sr + (int)di.half) / (int)di.div;
        const int g = (sg + (int)di.half) / (int)di.div;
        const int b = (sb + (int)di.half) / (int)di.div;

        return argb32_pack_u8(
            clamp0_255_i64(a),
            clamp0_255_i64(r),
            clamp0_255_i64(g),
            clamp0_255_i64(b));
    }

    /*
    // Keep this around a little while until we
    // are sure the optimized version is correct and gives the same results.
    static INLINE uint32_t packAvgDivInfo_ref(
        int sa, int sr, int sg, int sb,
        const BoxBlurDivInfo& di) noexcept
    {
        const int a = (sa + (int)di.half) / (int)di.div;
        const int r = (sr + (int)di.half) / (int)di.div;
        const int g = (sg + (int)di.half) / (int)di.div;
        const int b = (sb + (int)di.half) / (int)di.div;

        return pack_argb32(
            waavs::clamp_u8(a),
            waavs::clamp_u8(r),
            waavs::clamp_u8(g),
            waavs::clamp_u8(b));
    }
    */
    
    // -------------------------------------------
///*
// if NEON portability is a concern, use the following
// which avoids the use of vdivq_s32 (which is not supported
// on all ARMv7 targets, and is slow on ARMv8).

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
static INLINE void store4_packAvgDivInfo_neon(
    uint32_t* dst,
    const int32x4_t va,
    const int32x4_t vr,
    const int32x4_t vg,
    const int32x4_t vb,
    const BoxBlurDivInfo& di) noexcept
{
    int32_t sa4[4];
    int32_t sr4[4];
    int32_t sg4[4];
    int32_t sb4[4];

    vst1q_s32(sa4, va);
    vst1q_s32(sr4, vr);
    vst1q_s32(sg4, vg);
    vst1q_s32(sb4, vb);

    dst[0] = packAvgDivInfo(sa4[0], sr4[0], sg4[0], sb4[0], di);
    dst[1] = packAvgDivInfo(sa4[1], sr4[1], sg4[1], sb4[1], di);
    dst[2] = packAvgDivInfo(sa4[2], sr4[2], sg4[2], sb4[2], di);
    dst[3] = packAvgDivInfo(sa4[3], sr4[3], sg4[3], sb4[3], di);
}
#endif
    //*/



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
        m = clamp(m, 0, n);

        for (int i = 0; i < n; ++i)
            sizesOut[i] = (i < m) ? wl : wu;
    }

    // --------------------------------
    static INLINE void boxBlurH_PRGB32_row_edge_scalar(
        uint32_t* drow,
        const Surface& src,
        int y,
        int x0,
        int x1,
        int radius,
        int div) noexcept
    {
        if (x0 >= x1)
            return;
        
        auto divInfo = makeBoxBlurDivInfo(div);

        for (int x = x0; x < x1; ++x)
        {
            int sa = 0, sr = 0, sg = 0, sb = 0;

            for (int i = -radius; i <= radius; ++i)
            {
                uint8_t a, r, g, b;
                argb32_unpack_u8(pxRead32_Clamped(src, x + i, y), a, r, g, b);
                sa += a;
                sr += r;
                sg += g;
                sb += b;
            }

            drow[x] = packAvgDivInfo(sa, sr, sg, sb, divInfo);
        }
    }

    static INLINE void boxBlurH_PRGB32_row_middle_scalar(
        uint32_t* drow,
        const uint32_t* srow,
        int x0,
        int x1,
        int radius,
        int div) noexcept
    {
        if (x0 >= x1)
            return;

        int sa = 0, sr = 0, sg = 0, sb = 0;
        BoxBlurDivInfo divInfo = makeBoxBlurDivInfo(div);

        // Initialize window for x = x0.
        for (int i = -radius; i <= radius; ++i)
        {
            uint8_t a, r, g, b;
            argb32_unpack_u8(srow[x0 + i], a, r, g, b);
            sa += a;
            sr += r;
            sg += g;
            sb += b;
        }

        drow[x0] = packAvgDivInfo(sa, sr, sg, sb, divInfo);

        for (int x = x0 + 1; x < x1; ++x)
        {
            uint8_t aL, rL, gL, bL;
            uint8_t aR, rR, gR, bR;

            argb32_unpack_u8(srow[x - radius - 1], aL, rL, gL, bL);
            argb32_unpack_u8(srow[x + radius], aR, rR, gR, bR);

            sa += (int)aR - (int)aL;
            sr += (int)rR - (int)rL;
            sg += (int)gR - (int)gL;
            sb += (int)bR - (int)bL;

            drow[x] = packAvgDivInfo(sa, sr, sg, sb, divInfo);
        }
    }


#if defined(__ARM_NEON) || defined(__ARM_NEON__)

    template<int Lane>
    static INLINE void blur_lane_insert(
        int& sa, int& sr, int& sg, int& sb,
        int32x4_t& va, int32x4_t& vr, int32x4_t& vg, int32x4_t& vb,
        const uint32_t* srow, int x, int radius) noexcept
    {
        uint8_t aL, rL, gL, bL;
        uint8_t aR, rR, gR, bR;

        argb32_unpack_u8(srow[x - radius - 1], aL, rL, gL, bL);
        argb32_unpack_u8(srow[x + radius], aR, rR, gR, bR);

        sa += (int)aR - (int)aL;
        sr += (int)rR - (int)rL;
        sg += (int)gR - (int)gL;
        sb += (int)bR - (int)bL;

        va = vsetq_lane_s32(sa, va, Lane);
        vr = vsetq_lane_s32(sr, vr, Lane);
        vg = vsetq_lane_s32(sg, vg, Lane);
        vb = vsetq_lane_s32(sb, vb, Lane);
    }

    static INLINE void boxBlurH_PRGB32_row_middle_neon(
        uint32_t* drow,
        const uint32_t* srow,
        int x0,
        int x1,
        int radius,
        int div) noexcept
    {
        if (x0 >= x1)
            return;

        BoxBlurDivInfo divInfo = makeBoxBlurDivInfo(div);

        int sa = 0;
        int sr = 0;
        int sg = 0;
        int sb = 0;

        // Initialize window for x = x0.
        for (int i = -radius; i <= radius; ++i)
        {
            uint8_t a, r, g, b;
            argb32_unpack_u8(srow[x0 + i], a, r, g, b);
            sa += a;
            sr += r;
            sg += g;
            sb += b;
        }

        drow[x0] = packAvgDivInfo(sa, sr, sg, sb, divInfo);

        int x = x0 + 1;

        for (; x + 3 < x1; x += 4)
        {
            int32x4_t va = vdupq_n_s32(0);
            int32x4_t vr = vdupq_n_s32(0);
            int32x4_t vg = vdupq_n_s32(0);
            int32x4_t vb = vdupq_n_s32(0);

            blur_lane_insert<0>(sa, sr, sg, sb, va, vr, vg, vb, srow, x + 0, radius);
            blur_lane_insert<1>(sa, sr, sg, sb, va, vr, vg, vb, srow, x + 1, radius);
            blur_lane_insert<2>(sa, sr, sg, sb, va, vr, vg, vb, srow, x + 2, radius);
            blur_lane_insert<3>(sa, sr, sg, sb, va, vr, vg, vb, srow, x + 3, radius);

            store4_packAvgDivInfo_neon(drow + x, va, vr, vg, vb, divInfo);
        }

        for (; x < x1; ++x)
        {
            uint8_t aL, rL, gL, bL;
            uint8_t aR, rR, gR, bR;

            argb32_unpack_u8(srow[x - radius - 1], aL, rL, gL, bL);
            argb32_unpack_u8(srow[x + radius], aR, rR, gR, bR);

            sa += (int)aR - (int)aL;
            sr += (int)rR - (int)rL;
            sg += (int)gR - (int)gL;
            sb += (int)bR - (int)bL;

            drow[x] = packAvgDivInfo(sa, sr, sg, sb, divInfo);
        }
    }
#endif


    // Horizontal box blur for a rect, clamped sampling.
    static INLINE void boxBlurH_PRGB32_row_scalar(
        uint32_t* drow,
        const Surface& src,
        int y,
        int xBeg,
        int xEnd,
        int radius) noexcept
    {
        if (xBeg >= xEnd)
            return;

        const int W = (int)src.width();
        const int div = 2 * radius + 1;

        if (radius <= 0)
        {
            const uint32_t* srow = (const uint32_t*)src.rowPointer((size_t)y);
            std::memcpy(drow + xBeg, srow + xBeg, (size_t)(xEnd - xBeg) * 4u);
            return;
        }

        const int midMin = radius;
        const int midMax = W - radius;

        if (midMin >= midMax)
        {
            // No middle region, all edge.
            boxBlurH_PRGB32_row_edge_scalar(drow, src, y, xBeg, xEnd, radius, div);
            return;
        }

        // If the middle region exists, we can do the optimized 
        // sliding window for it.
        const int xMidBeg = clamp(xBeg, midMin, midMax);
        const int xMidEnd = clamp(xEnd, midMin, midMax);

        // Left edge: [xBeg, xMidBeg)
        boxBlurH_PRGB32_row_edge_scalar(drow, src, y, xBeg, xMidBeg, radius, div);

        // Middle: [xMidBeg, xMidEnd)
        if (xMidBeg < xMidEnd)
        {
            const uint32_t* srow = (const uint32_t*)src.rowPointer((size_t)y);
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
            boxBlurH_PRGB32_row_middle_neon(drow, srow, xMidBeg, xMidEnd, radius, div);
#else
            boxBlurH_PRGB32_row_middle_scalar(drow, srow, xMidBeg, xMidEnd, radius, div);
#endif
        }

        // Right edge: [xMidEnd, xEnd)
        boxBlurH_PRGB32_row_edge_scalar(drow, src, y, xMidEnd, xEnd, radius, div);
    }



    // Surface level horizontal box blur for a rect, clamped sampling.
    static void boxBlurH_PRGB32_scalar(
        Surface& dst,
        const Surface& src,
        int radius,
        const WGRectI& area) noexcept
    {
        if (radius <= 0)
        {
            for (int y = area.y; y < area.y + area.h; ++y)
            {
                const uint32_t* srow = (const uint32_t*)src.rowPointer((size_t)y);
                uint32_t* drow = (uint32_t*)dst.rowPointer((size_t)y);
                std::memcpy(drow + area.x, srow + area.x, (size_t)area.w * 4u);
            }
            return;
        }

        const int xBeg = area.x;
        const int xEnd = area.x + area.w;

        for (int y = area.y; y < area.y + area.h; ++y)
        {
            uint32_t* drow = (uint32_t*)dst.rowPointer((size_t)y);
            boxBlurH_PRGB32_row_scalar(drow, src, y, xBeg, xEnd, radius);
        }
    }





    // -------------------------------------------

    static void boxBlurH_PRGB32(Surface& dst, const Surface& src, int radius, const WGRectI& area) noexcept
    {
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
        boxBlurH_PRGB32_scalar(dst, src, radius, area);
#else
        boxBlurH_PRGB32_scalar(dst, src, radius, area);
#endif
    }

    // ---------------------------------------------
    // 

// --------------------------------
// Vertical row helpers
// --------------------------------

    static INLINE void boxBlurV_PRGB32_row_edge_scalar(
        uint32_t* drow,
        const Surface& src,
        int y,
        int xBeg,
        int xEnd,
        int radius,
        int div) noexcept
    {
        if (xBeg >= xEnd)
            return;

        const int H = (int)src.height();
        BoxBlurDivInfo divInfo = makeBoxBlurDivInfo(div);

        // Small fixed-capacity row pointer cache for the active kernel rows.
        // If you expect very large radii, replace with a small dynamic container.
        const uint32_t* rows[256];

        if (div > 256)
        {
            // Fallback: no row cache, just do direct clamped access.
            for (int x = xBeg; x < xEnd; ++x)
            {
                int sa = 0, sr = 0, sg = 0, sb = 0;

                for (int k = -radius; k <= radius; ++k)
                {
                    const int yy = clamp(y + k, 0, H - 1);
                    const uint32_t* srow = (const uint32_t*)src.rowPointer((size_t)yy);

                    uint8_t a, r, g, b;
                    argb32_unpack_u8(srow[x], a, r, g, b);

                    sa += a;
                    sr += r;
                    sg += g;
                    sb += b;
                }

                drow[x] = packAvgDivInfo(sa, sr, sg, sb, divInfo);
            }
            return;
        }

        // Gather clamped row pointers once for this output row.
        for (int i = 0; i < div; ++i)
        {
            const int yy = clamp(y - radius + i, 0, H - 1);
            rows[i] = (const uint32_t*)src.rowPointer((size_t)yy);
        }

        for (int x = xBeg; x < xEnd; ++x)
        {
            int sa = 0, sr = 0, sg = 0, sb = 0;

            for (int i = 0; i < div; ++i)
            {
                uint8_t a, r, g, b;
                argb32_unpack_u8(rows[i][x], a, r, g, b);

                sa += a;
                sr += r;
                sg += g;
                sb += b;
            }

            drow[x] = packAvgDivInfo(sa, sr, sg, sb, divInfo);
        }
    }

    static INLINE void boxBlurV_PRGB32_row_middle_scalar(
        uint32_t* drow,
        const Surface& src,
        int y,
        int xBeg,
        int xEnd,
        int radius,
        int div) noexcept
    {
        if (xBeg >= xEnd)
            return;

        BoxBlurDivInfo divInfo = makeBoxBlurDivInfo(div);

        // Small fixed-capacity row pointer cache for the active kernel rows.
        // Middle rows need no clamping.
        const uint32_t* rows[256];

        if (div > 256)
        {
            // Fallback: direct access without cache.
            for (int x = xBeg; x < xEnd; ++x)
            {
                int sa = 0, sr = 0, sg = 0, sb = 0;

                for (int k = -radius; k <= radius; ++k)
                {
                    const uint32_t* srow = (const uint32_t*)src.rowPointer((size_t)(y + k));

                    uint8_t a, r, g, b;
                    argb32_unpack_u8(srow[x], a, r, g, b);

                    sa += a;
                    sr += r;
                    sg += g;
                    sb += b;
                }

                drow[x] = packAvgDivInfo(sa, sr, sg, sb, divInfo);
            }
            return;
        }

        // Gather unclamped row pointers once for this output row.
        for (int i = 0; i < div; ++i)
            rows[i] = (const uint32_t*)src.rowPointer((size_t)(y - radius + i));

        for (int x = xBeg; x < xEnd; ++x)
        {
            int sa = 0, sr = 0, sg = 0, sb = 0;

            for (int i = 0; i < div; ++i)
            {
                uint8_t a, r, g, b;
                argb32_unpack_u8(rows[i][x], a, r, g, b);

                sa += a;
                sr += r;
                sg += g;
                sb += b;
            }

            drow[x] = packAvgDivInfo(sa, sr, sg, sb, divInfo);
        }
    }

    static INLINE void boxBlurV_PRGB32_row_scalar(
        uint32_t* drow,
        const Surface& src,
        int y,
        int xBeg,
        int xEnd,
        int radius) noexcept
    {
        if (xBeg >= xEnd)
            return;

        const int H = (int)src.height();
        const int div = 2 * radius + 1;

        if (radius <= 0)
        {
            const uint32_t* srow = (const uint32_t*)src.rowPointer((size_t)y);
            std::memcpy(drow + xBeg, srow + xBeg, (size_t)(xEnd - xBeg) * 4u);
            return;
        }

        const int midMin = radius;
        const int midMax = H - radius;

        // No middle Y region exists.
        if (midMin >= midMax)
        {
            boxBlurV_PRGB32_row_edge_scalar(drow, src, y, xBeg, xEnd, radius, div);
            return;
        }

        // Edge rows clamp in Y, middle rows do not.
        if (y < midMin || y >= midMax)
        {
            boxBlurV_PRGB32_row_edge_scalar(drow, src, y, xBeg, xEnd, radius, div);
            return;
        }

        boxBlurV_PRGB32_row_middle_scalar(drow, src, y, xBeg, xEnd, radius, div);
    }

    // Surface level vertical box blur for a rect.
    static void boxBlurV_PRGB32_scalar(
        Surface& dst,
        const Surface& src,
        int radius,
        const WGRectI& area) noexcept
    {
        if (radius <= 0)
        {
            for (int y = area.y; y < area.y + area.h; ++y)
            {
                const uint32_t* srow = (const uint32_t*)src.rowPointer((size_t)y);
                uint32_t* drow = (uint32_t*)dst.rowPointer((size_t)y);
                std::memcpy(drow + area.x, srow + area.x, (size_t)area.w * 4u);
            }
            return;
        }

        const int xBeg = area.x;
        const int xEnd = area.x + area.w;
        const int yBeg = area.y;
        const int yEnd = area.y + area.h;

        for (int y = yBeg; y < yEnd; ++y)
        {
            uint32_t* drow = (uint32_t*)dst.rowPointer((size_t)y);
            boxBlurV_PRGB32_row_scalar(drow, src, y, xBeg, xEnd, radius);
        }
    }

    static void boxBlurV_PRGB32(Surface& dst, const Surface& src, int radius, const WGRectI& area) noexcept
    {
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
        boxBlurV_PRGB32_scalar(dst, src, radius, area);
#else
        boxBlurV_PRGB32_scalar(dst, src, radius, area);
#endif
    }




}