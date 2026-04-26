// surface.h
#pragma once

#include "membuff.h"

#include "surface_draw.h"


namespace waavs
{
    struct Surface
    {
    private:
        RefMemBuff* fMemory = nullptr;
        Surface_ARGB32 fInfo{};

        void addRef() noexcept
        {
            if (fMemory)
                fMemory->addRef();
        }

        void release() noexcept
        {
            if (fMemory)
                fMemory->release();

            fMemory = nullptr;
            fInfo = {};
        }

    public:
        Surface() = default;

        explicit Surface(int32_t w, int32_t h)
        {
            (void)reset(w, h);
        }

        Surface(const Surface& other) noexcept
            : fMemory(other.fMemory),
            fInfo(other.fInfo)
        {
            addRef();
        }

        Surface& operator=(const Surface& other) noexcept
        {
            if (this == &other)
                return *this;

            if (other.fMemory)
                other.fMemory->addRef();

            release();

            fMemory = other.fMemory;
            fInfo = other.fInfo;

            return *this;
        }

        Surface(Surface&& other) noexcept
            : fMemory(other.fMemory),
            fInfo(other.fInfo)
        {
            other.fMemory = nullptr;
            other.fInfo = {};
        }

        Surface& operator=(Surface&& other) noexcept
        {
            if (this == &other)
                return *this;

            release();

            fMemory = other.fMemory;
            fInfo = other.fInfo;

            other.fMemory = nullptr;
            other.fInfo = {};

            return *this;
        }

        ~Surface() noexcept
        {
            release();
        }

        bool reset(int32_t w, int32_t h) noexcept
        {
            if (w <= 0 || h <= 0)
                return false;

            const intptr_t stride = intptr_t(w) * 4;
            const size_t totalBytes = size_t(stride) * size_t(h);

            RefMemBuff* mem = RefMemBuff::create(totalBytes);
            if (!mem || !mem->data())
            {
                if (mem)
                    mem->release();
                return false;
            }

            release();

            fMemory = mem;

            fInfo.data = mem->data();
            fInfo.width = w;
            fInfo.height = h;
            fInfo.stride = stride;
            fInfo.contiguous = true;

            return true;
        }

        WGResult createFromData(
            size_t w,
            size_t h,
            size_t stride,
            uint8_t* data) noexcept
        {
            if (!data || w == 0 || h == 0 || stride < w * 4)
                return WG_ERROR_Invalid_Argument;

            release();

            fInfo.data = data;
            fInfo.width = int32_t(w);
            fInfo.height = int32_t(h);
            fInfo.stride = intptr_t(stride);
            fInfo.contiguous = stride == w * 4;

            return WG_SUCCESS;
        }

        WGResult getSubSurface(const WGRectI& r, Surface& out) const noexcept
        {
            Surface_ARGB32 subInfo{};

            WGResult res = Surface_ARGB32_get_subarea(fInfo, r, subInfo);
            if (res != WG_SUCCESS)
                return res;

            Surface tmp;
            tmp.fMemory = fMemory;
            tmp.fInfo = subInfo;

            if (tmp.fMemory)
                tmp.fMemory->addRef();

            out = tmp;

            return WG_SUCCESS;
        }

        Surface_ARGB32 info() const noexcept { return fInfo; }

        bool empty() const noexcept
        {
            return !fInfo.data || fInfo.width <= 0 || fInfo.height <= 0;
        }

        bool ownsMemory() const noexcept
        {
            return fMemory != nullptr;
        }

        bool isContiguous() const noexcept
        {
            return fInfo.contiguous;
        }

        WGRectI boundsI() const noexcept
        {
            return WGRectI{ 0, 0, fInfo.width, fInfo.height };
        }

        WGRectD boundsD() const noexcept
        {
            return WGRectD{
                0.0,
                0.0,
                double(fInfo.width),
                double(fInfo.height)
            };
        }

        size_t width() const noexcept { return size_t(fInfo.width); }
        size_t height() const noexcept { return size_t(fInfo.height); }
        size_t stride() const noexcept { return size_t(fInfo.stride); }

        const uint8_t* data() const noexcept { return fInfo.data; }
        uint8_t* data() noexcept { return fInfo.data; }

        const uint32_t* rowPointer(int y) const noexcept
        {
            return Surface_ARGB32_row_pointer_const(&fInfo, y);
        }

        uint32_t* rowPointer(int y) noexcept
        {
            return Surface_ARGB32_row_pointer(&fInfo, y);
        }

        void clearAll() noexcept
        {
            (void)wg_surface_clear(fInfo);
        }

        void fillAll(Pixel_ARGB32 rgbaPremul) noexcept
        {
            wg_surface_fill(fInfo, rgbaPremul);
        }

        void fillRect(const WGRectI& r, Pixel_ARGB32 rgbaPremul) noexcept
        {
            Surface sub;
            if (getSubSurface(r, sub) != WG_SUCCESS)
                return;

            wg_surface_fill(sub.fInfo, rgbaPremul);
        }

        void fillRect(const WGRectD& r, Pixel_ARGB32 rgbaPremul) noexcept
        {
            int x = int(waavs::floor(r.x));
            int y = int(waavs::floor(r.y));
            int w = int(waavs::ceil(r.x + r.w)) - x;
            int h = int(waavs::ceil(r.y + r.h)) - y;

            fillRect(WGRectI{ x, y, w, h }, rgbaPremul);
        }

        void blit(const Surface& src, int dstX, int dstY) noexcept
        {
            (void)wg_blit_copy(fInfo, src.fInfo, dstX, dstY);
        }

        void compositeBlit(
            const Surface& src,
            int dstX,
            int dstY,
            WGCompositeOp op) noexcept
        {
            (void)wg_blit_composite(fInfo, src.fInfo, dstX, dstY, op);
        }
    };

/*
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
            //wg_clear_all(fInfo);

            return true;
        }


        Surface_ARGB32 info() const noexcept { return fInfo; }

        // Convenience accessors
        bool isContiguous() const noexcept { return fInfo.contiguous; }

        // Two forms of bounds: integer and double.  The double form is useful for geometry calculations, 
        // and the integer form is useful for pixel loops.
        WGRectI boundsI() const noexcept { return WGRectI{ 0, 0, (int)width(), (int)height() }; }
        WGRectD boundsD() const noexcept { return WGRectD{ 0.0, 0.0, (double)width(), (double)height() }; }

        size_t width() const noexcept { return (size_t)fInfo.width; }
        size_t height() const noexcept { return (size_t)fInfo.height; }
        size_t stride() const noexcept { return (size_t)fInfo.stride; }
        const uint8_t* data() const noexcept { return fInfo.data; }
        uint8_t* data() noexcept { return fInfo.data; }

        const uint32_t* rowPointer(int y) const noexcept { return Surface_ARGB32_row_pointer_const(&fInfo, y); }
        uint32_t* rowPointer(int y) noexcept { return Surface_ARGB32_row_pointer(&fInfo, y); }

        // createFromData
        // 
        // Initialize a surface to wrap externally allocated data.
        // The surface will not take ownership of the data, and will 
        // not attempt to free it.
        // The caller is responsible for ensuring that the data remains valid
        // for the lifetime of the surface, and that the data is properly
        // allocated with the correct stride and format.
        WGResult createFromData(size_t w, size_t h, size_t stride, uint8_t* data)
        {
            if (!data || w <= 0 || h <= 0 || stride < w * 4)
                return WG_ERROR_Invalid_Argument;

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
        WGResult getSubSurface(const WGRectI& r, Surface& out) const noexcept
        {
            WGErrorCode res = Surface_ARGB32_get_subarea(fInfo, r, out.fInfo);
            if (res != WG_SUCCESS)
                return res;


            // Wrap as a non-owning proxy with parent stride preserved.
            return out.createFromData(
                (size_t)out.fInfo.width,
                (size_t)out.fInfo.height,
                stride(),
                out.fInfo.data);
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
            // need to find the intersection of the src rect with
            // the surface bounds, then fill that intersection area.
            Surface sub;
            if (getSubSurface(r, sub) != WG_SUCCESS)
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
        // Note: blit() routines DO NOT do scaling.
        void blit(const Surface& src, int dstX, int dstY) noexcept
        {
            (void)wg_blit_copy(fInfo, src.fInfo, dstX, dstY);
        }
        
        // compositeBlit()
        //
        // Composite a source surface into this surface 
        // at the given destination coordinates, using the 
        // specified compositing operator.
        // Big caveat: This routine does not handle overlapping 
        // source and destination regions.
        //
        void compositeBlit(const Surface& src, int dstX, int dstY, WGCompositeOp op) noexcept
        {
            (void)wg_blit_composite(fInfo, src.fInfo, dstX, dstY, op);
        }
    };
    */
}