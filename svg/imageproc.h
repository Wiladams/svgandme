#pragma once

// Image processing routines
// The various image processing routines, implemented against the PixelArray interface
// These should be called by the filter program executor.



#include "surface.h"
#include "pixeling.h"

/*
namespace waavs {


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
*/