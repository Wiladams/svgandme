// ==============================================================================
// Yet another representation of pixel data.
// This one is different than PixelArray or FrameBuffer in that
// It's plain, without any reliance on any other API, such as blend2d or Windows
// Just plain raw memory allocation, and conforming to the Surface_ARGB32 interface
//
// ==============================================================================

#pragma once


#include "maths.h"
#include "pixeling.h"
#include "membuff.h"
#include "wggeometry.h"

namespace waavs
{
    struct Surface
    {
    private:
        // We don't want anything touching this internal data
        // container, only through interfaces.
        // The data buffer is only active when this surface owns
        // the data that has been allocated.
        // The surface can also serve as a wrapper for other storage
        // In those cases, the data buffer will be empty, and this
        // surface is not responsible for allocat/de-allocate
        MemBuff fDataBuffer;   

    public:
        // Storage for the essential information that makes up 
        // the surface (width, height, stride, data pointer)
        Surface_ARGB32 fInfo;



    public:
        Surface() = default;

        Surface(int w, int h)
        {
            (void)reset(w, h);
        }

        bool reset(int w, int h)
        {
            if (w<= 0 || h <= 0)
                return false;

            // clear existing memory
            fDataBuffer.reset();

            fInfo.width = w;
            fInfo.height = h;
            fInfo.stride = w * 4; // ARGB32 is 4 bytes per pixel

            size_t totalBytes = (size_t)fInfo.stride * (size_t)fInfo.height;
            fDataBuffer.resetFromSize(totalBytes);
            fInfo.data = fDataBuffer.begin();
            
            return true;
        }


        Surface_ARGB32 info() const noexcept { return fInfo; }

        // Convenience accessors
        // Two forms of bounds: integer and double.  The double form is useful for geometry calculations, 
        // and the integer form is useful for pixel loops.
        WGRectI boundsI() const noexcept { return WGRectI{ 0, 0, (int)width(), (int)height() }; }
        WGRectD boundsD() const noexcept { return WGRectD{ 0.0, 0.0, (double)width(), (double)height() }; }

        size_t width() const noexcept { return (size_t)fInfo.width; }
        size_t height() const noexcept { return (size_t)fInfo.height; }
        size_t stride() const noexcept { return (size_t)fInfo.stride; }
        const uint8_t* data() const noexcept { return fInfo.data; }
        uint8_t* data() noexcept { return fInfo.data; }

        const uint32_t* rowPointer(int y) const noexcept { return pixeling_ARGB32_row_ptr_const(&fInfo, y); }
        uint32_t* rowPointer(int y) noexcept { return pixeling_ARGB32_row_ptr(&fInfo, y); }

        // createFromData
        // 
        // Initialize a surface to wrap externally allocated data.
        // The surface will not take ownership of the data, and will 
        // not attempt to free it.
        // The caller is responsible for ensuring that the data remains valid
        // for the lifetime of the surface, and that the data is properly
        // allocated with the correct stride and format.
        int createFromData(size_t w, size_t h, size_t stride, uint8_t* data)
        {
            if (!data || w <= 0 || h <= 0 || stride < w * 4)
                return -1;

            fDataBuffer.reset(); // clear existing memory
            fInfo.width = (int)w;
            fInfo.height = (int)h;
            fInfo.stride = (int)stride;
            fInfo.data = data;

            return 0;
        }

        // getSubRegion
        //
        // Retrieve a sub-region of this surface as a new surface that wraps the same data.
        INLINE bool getSubRegion(const WGRectI& r, Surface& out) const noexcept
        {
            // Reject empty source or empty requested rect.
            if (!fInfo.data || fInfo.width <= 0 || fInfo.height <= 0)
                return false;

            if (r.w <= 0 || r.h <= 0)
                return false;

            // Clip requested region to this surface.
            const WGRectI clipped = intersection(r, boundsI());
            if (clipped.w <= 0 || clipped.h <= 0)
                return false;

            // Compute top-left of clipped region using the existing row helper.
            const uint32_t* row = rowPointer(clipped.y);
            if (!row)
                return false;

            uint8_t* subData = (uint8_t*)(row + clipped.x);

            // Wrap as a non-owning proxy with parent stride preserved.
            return out.createFromData(
                (size_t)clipped.w,
                (size_t)clipped.h,
                stride(),
                subData) == 0;
        }

        // clearAll()
        // 
        // Set all pixels to a transparent black (0x00000000)
        // This should work, even if the surface is a sub-region 
        // of a larger surface.
        void clearAll() noexcept 
        {
            for (int row = 0; row < height(); ++row)
            {
                uint32_t* rPtr = rowPointer(row);
                memset(rPtr, 0, width() * 4);
            }
        }

        // fillAll()
        //
        // Fill the entire surface with a single premultiplied ARGB color.
        // This should work, even if the surface is a sub-region of a larger surface.
        //
        void fillAll(uint32_t rgbaPremul) noexcept
        {
            for (int row = 0; row < height(); ++row)
            {
                uint32_t* rPtr = rowPointer(row);
                memset_l(rPtr, (int32_t)rgbaPremul, width());
            }
        }

        //
        // fillRect
        //
        // Fill a sub-region with a given pre-multiplied pixel value
        // This will handle clipping the rectangle to fit the bounds
        // of the surface.
        void fillRect(const WGRectI& r, uint32_t rgbaPremul) const
        {
            Surface sub;
            if (!getSubRegion(r, sub))
                return;

            sub.fillAll(rgbaPremul);
        }

        void fillRect(const WGRectD& r, uint32_t rgbaPremul) const
        {
            int x = (int)waavs::floor(r.x);
            int y = (int)waavs::floor(r.y);
            int w = (int)waavs::ceil(r.x + r.w) - x;
            int h = (int)waavs::ceil(r.y + r.h) - y;

            fillRect(WGRectI{ x, y, w, h }, rgbaPremul);
        }

        // blit()
        //
        // Copy a source surface into this surface at the given destination coordinates.
        // If you want a sub-region of the source, simply 'getSubRegion()' that region 
        // into a new surface, and blit that.
        // Big caveat: This routine uses memcpy, so if the source and destination regions 
        // overlap, the behavior is undefined.
        void blitSurface(const Surface& src, int dstX, int dstY) noexcept
        {
            // Reject empty source or destination.
            if (!data() || width() == 0 || height() == 0)
                return;

            if (!src.data() || src.width() == 0 || src.height() == 0)
                return;

            // Where the full source would land in destination coordinates.
            const WGRectI dstPlacement{ dstX, dstY, (int)src.width(), (int)src.height() };

            // Clip that placement against destination bounds.
            const WGRectI dstClipped = intersection(dstPlacement, boundsI());
            if (dstClipped.w <= 0 || dstClipped.h <= 0)
                return;

            // Compute corresponding source subregion.
            // If dstPlacement was clipped on the left/top, shift source origin by same amount.
            const WGRectI srcRect{
                dstClipped.x - dstPlacement.x,
                dstClipped.y - dstPlacement.y,
                dstClipped.w,
                dstClipped.h
            };

            Surface srcView;
            if (!src.getSubRegion(srcRect, srcView))
                return;

            Surface dstView;
            if (!getSubRegion(dstClipped, dstView))
                return;

            // Copy row by row. Since both views have same width/height now,
            // each row is a straight memcpy.
            const size_t rowBytes = dstView.width() * 4;

            for (size_t y = 0; y < dstView.height(); ++y)
            {
                uint8_t* dstRow = (uint8_t*)dstView.rowPointer((int)y);
                const uint8_t* srcRow = (const uint8_t*)srcView.rowPointer((int)y);
                memcpy(dstRow, srcRow, rowBytes);
            }
        }
    };
}