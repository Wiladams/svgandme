// filter_femorphology.h
#pragma once

#include <memory>

#include "definitions.h"
#include "coloring.h"
#include "wggeometry.h"
#include "surface.h"

namespace waavs
{
    struct MorphDequeScratch
    {
        int fCapacity = 0;

        std::unique_ptr<int[]>     idxA;
        std::unique_ptr<int[]>     idxR;
        std::unique_ptr<int[]>     idxG;
        std::unique_ptr<int[]>     idxB;

        std::unique_ptr<uint8_t[]> valA;
        std::unique_ptr<uint8_t[]> valR;
        std::unique_ptr<uint8_t[]> valG;
        std::unique_ptr<uint8_t[]> valB;

        bool ensureCapacity(int n) noexcept
        {
            if (n <= fCapacity)
                return true;

            std::unique_ptr<int[]>     nIdxA(new (std::nothrow) int[n]);
            std::unique_ptr<int[]>     nIdxR(new (std::nothrow) int[n]);
            std::unique_ptr<int[]>     nIdxG(new (std::nothrow) int[n]);
            std::unique_ptr<int[]>     nIdxB(new (std::nothrow) int[n]);

            std::unique_ptr<uint8_t[]> nValA(new (std::nothrow) uint8_t[n]);
            std::unique_ptr<uint8_t[]> nValR(new (std::nothrow) uint8_t[n]);
            std::unique_ptr<uint8_t[]> nValG(new (std::nothrow) uint8_t[n]);
            std::unique_ptr<uint8_t[]> nValB(new (std::nothrow) uint8_t[n]);

            if (!nIdxA || !nIdxR || !nIdxG || !nIdxB ||
                !nValA || !nValR || !nValG || !nValB)
                return false;

            idxA = std::move(nIdxA);
            idxR = std::move(nIdxR);
            idxG = std::move(nIdxG);
            idxB = std::move(nIdxB);

            valA = std::move(nValA);
            valR = std::move(nValR);
            valG = std::move(nValG);
            valB = std::move(nValB);

            fCapacity = n;
            return true;
        }
    };

    // some dequeue helpers for the morphology operations
    static INLINE void morph_push_max_u8(
        int* qidx, uint8_t* qval, int& head, int& tail, int idx, uint8_t v) noexcept
    {
        while (tail > head && qval[tail - 1] <= v)
            --tail;

        qidx[tail] = idx;
        qval[tail] = v;
        ++tail;
    }

    static INLINE void morph_push_min_u8(
        int* qidx, uint8_t* qval, int& head, int& tail, int idx, uint8_t v) noexcept
    {
        while (tail > head && qval[tail - 1] >= v)
            --tail;

        qidx[tail] = idx;
        qval[tail] = v;
        ++tail;
    }

    static INLINE void morph_expire_front(
        int* qidx, int& head, int tail, int minIdx) noexcept
    {
        while (head < tail && qidx[head] < minIdx)
            ++head;
    }



    // -----------------------------

    static bool dilate_row_scalar(
        uint32_t* dst,
        const uint32_t* src,
        int x0,
        int x1,
        int W,
        int radius,
        MorphDequeScratch& scratch) noexcept
    {
        const int n = x1 - x0 + 1;
        if (n <= 0)
            return true;


        if (radius <= 0)
        {
            for (int x = x0; x <= x1; ++x)
            {
                const uint32_t px = src[x];
                uint8_t a, r, g, b;
                argb32_unpack_unpremul_u8(px, a, r, g, b);

                dst[x] = argb32_pack_u8(a, r, g, b);
            }

            return true;
        }

        const int window = radius * 2 + 1;
        const int streamLen = n + radius * 2;

        if (!scratch.ensureCapacity(streamLen))
            return false;

        int headA = 0, tailA = 0;
        int headR = 0, tailR = 0;
        int headG = 0, tailG = 0;
        int headB = 0, tailB = 0;

        for (int s = 0; s < streamLen; ++s)
        {
            int sx = x0 - radius + s;
            if (sx < 0) sx = 0;
            if (sx >= W) sx = W - 1;

            const uint32_t px = src[sx];
            uint8_t a, r, g, b;
            argb32_unpack_unpremul_u8(px, a, r, g, b);

            morph_push_max_u8(scratch.idxA.get(), scratch.valA.get(), headA, tailA, s, a);
            morph_push_max_u8(scratch.idxR.get(), scratch.valR.get(), headR, tailR, s, r);
            morph_push_max_u8(scratch.idxG.get(), scratch.valG.get(), headG, tailG, s, g);
            morph_push_max_u8(scratch.idxB.get(), scratch.valB.get(), headB, tailB, s, b);

            const int winStart = s - window + 1;
            if (winStart < 0)
                continue;

            morph_expire_front(scratch.idxA.get(), headA, tailA, winStart);
            morph_expire_front(scratch.idxR.get(), headR, tailR, winStart);
            morph_expire_front(scratch.idxG.get(), headG, tailG, winStart);
            morph_expire_front(scratch.idxB.get(), headB, tailB, winStart);

            const int x = x0 + winStart;
            //dst[x] = argb32_pack_premul_u8(
            dst[x] = argb32_pack_u8(
                scratch.valA[headA],
                scratch.valR[headR],
                scratch.valG[headG],
                scratch.valB[headB]);
        }

        return true;
    }

    static bool erode_row_scalar(
        uint32_t* dst,
        const uint32_t* src,
        int x0,
        int x1,
        int W,
        int radius,
        MorphDequeScratch& scratch) noexcept
    {
        const int n = x1 - x0 + 1;
        if (n <= 0)
            return true;

        
        if (radius <= 0)
        {
            for (int x = x0; x <= x1; ++x)
            {
                const uint32_t px = src[x];
                uint8_t a, r, g, b;
                argb32_unpack_unpremul_u8(px, a, r, g, b);

                dst[x] = argb32_pack_u8(a, r, g, b);
            }

            return true;
        }
        

        const int window = radius * 2 + 1;
        const int streamLen = n + radius * 2;

        if (!scratch.ensureCapacity(streamLen))
            return false;

        int headA = 0, tailA = 0;
        int headR = 0, tailR = 0;
        int headG = 0, tailG = 0;
        int headB = 0, tailB = 0;

        for (int s = 0; s < streamLen; ++s)
        {
            int sx = x0 - radius + s;
            if (sx < 0) sx = 0;
            if (sx >= W) sx = W - 1;

            const uint32_t px = src[sx];
            uint8_t a, r, g, b;
            argb32_unpack_unpremul_u8(px, a, r, g, b);

            morph_push_min_u8(scratch.idxA.get(), scratch.valA.get(), headA, tailA, s, a);
            morph_push_min_u8(scratch.idxR.get(), scratch.valR.get(), headR, tailR, s, r);
            morph_push_min_u8(scratch.idxG.get(), scratch.valG.get(), headG, tailG, s, g);
            morph_push_min_u8(scratch.idxB.get(), scratch.valB.get(), headB, tailB, s, b);

            const int winStart = s - window + 1;
            if (winStart < 0)
                continue;

            morph_expire_front(scratch.idxA.get(), headA, tailA, winStart);
            morph_expire_front(scratch.idxR.get(), headR, tailR, winStart);
            morph_expire_front(scratch.idxG.get(), headG, tailG, winStart);
            morph_expire_front(scratch.idxB.get(), headB, tailB, winStart);

            const int x = x0 + winStart;
            //dst[x] = argb32_pack_premul_u8(
            dst[x] = argb32_pack_u8(
                scratch.valA[headA],
                scratch.valR[headR],
                scratch.valG[headG],
                scratch.valB[headB]);
        }

        return true;
    }

    // ------------------------------
    static bool dilate_col_scalar(
        Surface& dstSurf,
        const Surface& srcSurf,
        int x0,
        int x1,
        int y0,
        int y1,
        int H,
        int radius,
        MorphDequeScratch& scratch) noexcept
    {
        const int n = y1 - y0 + 1;
        if (n <= 0)
            return true;

        
        if (radius <= 0)
        {
            for (int y = y0; y <= y1; ++y)
            {
                uint32_t* drow = (uint32_t*)dstSurf.rowPointer((size_t)y);
                const uint32_t* srow = (const uint32_t*)srcSurf.rowPointer((size_t)y);

                for (int x = x0; x <= x1; ++x)
                {
                    const uint32_t px = srow[x];
                    uint8_t a, r, g, b;
                    argb32_unpack_unpremul_u8(px, a, r, g, b);

                    drow[x] = argb32_pack_u8(a, r, g, b);


                    //drow[x] = srow[x];
                }
            }
            return true;
        }
        

        const int window = radius * 2 + 1;
        const int streamLen = n + radius * 2;

        if (!scratch.ensureCapacity(streamLen))
            return false;

        for (int x = x0; x <= x1; ++x)
        {
            int headA = 0, tailA = 0;
            int headR = 0, tailR = 0;
            int headG = 0, tailG = 0;
            int headB = 0, tailB = 0;

            for (int s = 0; s < streamLen; ++s)
            {
                int sy = y0 - radius + s;
                if (sy < 0) sy = 0;
                if (sy >= H) sy = H - 1;

                const uint32_t* srow = (const uint32_t*)srcSurf.rowPointer((size_t)sy);
                const uint32_t px = srow[x];

                uint8_t a, r, g, b;
                //argb32_unpack_unpremul_u8(px, a, r, g, b);
                argb32_unpack_u8(px, a, r, g, b);

                morph_push_max_u8(scratch.idxA.get(), scratch.valA.get(), headA, tailA, s, a);
                morph_push_max_u8(scratch.idxR.get(), scratch.valR.get(), headR, tailR, s, r);
                morph_push_max_u8(scratch.idxG.get(), scratch.valG.get(), headG, tailG, s, g);
                morph_push_max_u8(scratch.idxB.get(), scratch.valB.get(), headB, tailB, s, b);

                const int winStart = s - window + 1;
                if (winStart < 0)
                    continue;

                morph_expire_front(scratch.idxA.get(), headA, tailA, winStart);
                morph_expire_front(scratch.idxR.get(), headR, tailR, winStart);
                morph_expire_front(scratch.idxG.get(), headG, tailG, winStart);
                morph_expire_front(scratch.idxB.get(), headB, tailB, winStart);

                const int y = y0 + winStart;
                uint32_t* drow = (uint32_t*)dstSurf.rowPointer((size_t)y);
                drow[x] = argb32_pack_straight_to_premul_u8(
                    scratch.valA[headA],
                    scratch.valR[headR],
                    scratch.valG[headG],
                    scratch.valB[headB]);
            }
        }

        return true;
    }

    static bool erode_col_scalar(
        Surface& dstSurf,
        const Surface& srcSurf,
        int x0,
        int x1,
        int y0,
        int y1,
        int H,
        int radius,
        MorphDequeScratch& scratch) noexcept
    {
        const int n = y1 - y0 + 1;
        if (n <= 0)
            return true;

        if (radius <= 0)
        {
            for (int y = y0; y <= y1; ++y)
            {
                uint32_t* drow = (uint32_t*)dstSurf.rowPointer((size_t)y);
                const uint32_t* srow = (const uint32_t*)srcSurf.rowPointer((size_t)y);

                for (int x = x0; x <= x1; ++x)
                {
                    // the source has already been unpremultiplied in the row pass, 
                    // so we need to pre-multiply it back for correct output
                    const uint32_t px = srow[x];
                    uint8_t a, r, g, b;
                    argb32_unpack_u8(px, a, r, g, b);
                    drow[x] = argb32_pack_straight_to_premul_u8(a, r, g, b);
                }
            }
            return true;
        }

        const int window = radius * 2 + 1;
        const int streamLen = n + radius * 2;

        if (!scratch.ensureCapacity(streamLen))
            return false;

        for (int x = x0; x <= x1; ++x)
        {
            int headA = 0, tailA = 0;
            int headR = 0, tailR = 0;
            int headG = 0, tailG = 0;
            int headB = 0, tailB = 0;

            for (int s = 0; s < streamLen; ++s)
            {
                int sy = y0 - radius + s;
                if (sy < 0) sy = 0;
                if (sy >= H) sy = H - 1;

                const uint32_t* srow = (const uint32_t*)srcSurf.rowPointer((size_t)sy);
                const uint32_t px = srow[x];

                uint8_t a, r, g, b;
                //argb32_unpack_unpremul_u8(px, a, r, g, b);
                argb32_unpack_u8(px, a, r, g, b);

                morph_push_min_u8(scratch.idxA.get(), scratch.valA.get(), headA, tailA, s, a);
                morph_push_min_u8(scratch.idxR.get(), scratch.valR.get(), headR, tailR, s, r);
                morph_push_min_u8(scratch.idxG.get(), scratch.valG.get(), headG, tailG, s, g);
                morph_push_min_u8(scratch.idxB.get(), scratch.valB.get(), headB, tailB, s, b);

                const int winStart = s - window + 1;
                if (winStart < 0)
                    continue;

                morph_expire_front(scratch.idxA.get(), headA, tailA, winStart);
                morph_expire_front(scratch.idxR.get(), headR, tailR, winStart);
                morph_expire_front(scratch.idxG.get(), headG, tailG, winStart);
                morph_expire_front(scratch.idxB.get(), headB, tailB, winStart);

                const int y = y0 + winStart;
                uint32_t* drow = (uint32_t*)dstSurf.rowPointer((size_t)y);
                drow[x] = argb32_pack_straight_to_premul_u8(
                    scratch.valA[headA],
                    scratch.valR[headR],
                    scratch.valG[headG],
                    scratch.valB[headB]);
            }
        }

        return true;
    }

    // ------------------------------
    // Row dispatch functions 
    // call the scalar or SIMD implementation depending 
    // on platform capabilities
    //
    static INLINE bool dilate_row(
        uint32_t* dst,
        const uint32_t* src,
        int x0,
        int x1,
        int W,
        int radius,
        MorphDequeScratch& scratch) noexcept
    {
#if WAAVS_HAS_NEON
        // TODO: dilate_row_neon(...)
        return dilate_row_scalar(dst, src, x0, x1, W, radius, scratch);
#else
        return dilate_row_scalar(dst, src, x0, x1, W, radius, scratch);
#endif
    }

    static INLINE bool erode_row(
        uint32_t* dst,
        const uint32_t* src,
        int x0,
        int x1,
        int W,
        int radius,
        MorphDequeScratch& scratch) noexcept
    {
#if WAAVS_HAS_NEON
        // TODO: erode_row_neon(...)
        return erode_row_scalar(dst, src, x0, x1, W, radius, scratch);
#else
        return erode_row_scalar(dst, src, x0, x1, W, radius, scratch);
#endif
    }

    static INLINE bool dilate_col(
        Surface& dstSurf,
        const Surface& srcSurf,
        int x0,
        int x1,
        int y0,
        int y1,
        int H,
        int radius,
        MorphDequeScratch& scratch) noexcept
    {
#if WAAVS_HAS_NEON
        // TODO: dilate_col_neon(...)
        return dilate_col_scalar(dstSurf, srcSurf, x0, x1, y0, y1, H, radius, scratch);
#else
        return dilate_col_scalar(dstSurf, srcSurf, x0, x1, y0, y1, H, radius, scratch);
#endif
    }

    static INLINE bool erode_col(
        Surface& dstSurf,
        const Surface& srcSurf,
        int x0,
        int x1,
        int y0,
        int y1,
        int H,
        int radius,
        MorphDequeScratch& scratch) noexcept
    {
#if WAAVS_HAS_NEON
        // TODO: erode_col_neon(...)
        return erode_col_scalar(dstSurf, srcSurf, x0, x1, y0, y1, H, radius, scratch);
#else
        return erode_col_scalar(dstSurf, srcSurf, x0, x1, y0, y1, H, radius, scratch);
#endif
    }

}