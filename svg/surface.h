// ==============================================================================
// Yet another representation of pixel data.
// This one is different than PixelArray or FrameBuffer in that
// It's plain, without any reliance on any other API, such as blend2d or Windows
// Just plain raw memory allocation, and conforming to the Surface_ARGB32 interface
//
// ==============================================================================

#pragma once



#include "membuff.h"
#include "idrawpixels.h"

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
            fInfo.contiguous = true;    // rows are contiguous with no padding

            size_t totalBytes = (size_t)fInfo.stride * (size_t)fInfo.height;
            fDataBuffer.resetFromSize(totalBytes);
            fInfo.data = fDataBuffer.begin();
            
            // Start out with a clean slate by default
            wg_clear_all(fInfo);

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
        uint32_t createFromData(size_t w, size_t h, size_t stride, uint8_t* data)
        {
            if (!data || w <= 0 || h <= 0 || stride < w * 4)
                return -1;

            fDataBuffer.reset(); // clear existing memory
            fInfo.width = (int)w;
            fInfo.height = (int)h;
            fInfo.stride = (int)stride;
            fInfo.data = data;
            fInfo.contiguous = (stride == w * 4);

            return WG_SUCCESS;
        }

        // getSubSurface
        //
        // Retrieve a sub-region of this surface as a new surface that wraps the same data.
        bool getSubSurface(const WGRectI& r, Surface& out) const noexcept
        {
            if (!wg_get_subarea(fInfo, r, out.fInfo))
                return false;

            // Wrap as a non-owning proxy with parent stride preserved.
            return out.createFromData(
                (size_t)out.fInfo.width,
                (size_t)out.fInfo.height,
                stride(),
                out.fInfo.data) == WG_SUCCESS;
        }

        // clearAll()
        // 
        // Set all pixels to a transparent black (0x00000000)
        // This should work, even if the surface is a sub-region 
        // of a larger surface.
        void clearAll() noexcept 
        {
            (void)wg_clear_all(fInfo);
        }
        

        // fillAll()
        //
        // Fill the entire surface with a single premultiplied ARGB color.
        //
        void fillAll(Pixel_ARGB32 rgbaPremul) noexcept
        {
            wg_fill_all(fInfo, rgbaPremul);
        }

        //
        // fillRect
        //
        // Fill a sub-region with a given pre-multiplied pixel value
        // This will handle clipping the rectangle to fit the bounds
        // of the surface.
        void fillRect(const WGRectI& r, Pixel_ARGB32 rgbaPremul) const
        {
            Surface sub;
            if (!getSubSurface(r, sub))
                return;

            wg_fill_all(sub.fInfo, rgbaPremul);
        }

        void fillRect(const WGRectD& r, Pixel_ARGB32 rgbaPremul) const
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
        // Big caveat: This routine uses memcpy, so if the source and destination regions 
        // overlap, the behavior is undefined.
        void blit(const Surface& src, int dstX, int dstY) noexcept
        {
            (void)wg_blit(fInfo, src.fInfo, dstX, dstY);
        }

    };
}