// surface.h
#pragma once

#include "membuff.h"

#include "surface_draw.h"


namespace waavs
{
    struct Surface
    {
        static constexpr int32_t kBytesPerPixel = 4; // ARGB32

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

        bool operator==(const Surface& other)
        {
            return ((fInfo.data == other.fInfo.data) &&
                (fInfo.width == other.fInfo.width) &&
                (fInfo.height == other.fInfo.height) &&
                (fInfo.stride == other.fInfo.stride));
        }

        ~Surface() noexcept
        {
            release();
        }



        bool reset(int32_t w, int32_t h) noexcept
        {
            if (w <= 0 || h <= 0)
                return false;

            // Make sure we're not going to overflow allocation

            if (w > INT32_MAX / kBytesPerPixel)
                return false;

            const size_t astride = size_t(w) * kBytesPerPixel;

            // Again, check for overflow, before it can happen
            if (size_t(h) > SIZE_MAX / astride)
                return false;

            const size_t totalBytes = astride * size_t(h);

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
            fInfo.stride = astride;
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

        bool hasOwnedBacking() const noexcept
        {
            return fMemory != nullptr;
        }

        bool isBorrowed() const noexcept
        {
            return fInfo.data && fMemory == nullptr;
        }

        bool isContiguous() const noexcept
        {
            return fInfo.contiguous;
        }

        bool isValid() const noexcept
        {
            return fInfo.data &&
                fInfo.width > 0 &&
                fInfo.height > 0 &&
                fInfo.stride >= ptrdiff_t(fInfo.width) * kBytesPerPixel;
        
        }

        bool isView() const noexcept
        {
            return fMemory != nullptr && fInfo.data != fMemory->data();
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

}