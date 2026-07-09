// surface.h
#pragma once

#include "lang_memory.h"
#include "surface_info.h"


namespace waavs
{
    // --------------------------------------
    // Surface
    // Storage for 2D pixel data
    // --------------------------------------
    struct Surface
    {
        static constexpr int32_t kBytesPerPixel = 4; // ARGB32

    private:
        SharedMemBuff fMemory{};
        Surface_ARGB32 fInfo{};

    public:
        Surface() = default;
        Surface(const Surface& other) noexcept = default;
        Surface(Surface&& other) noexcept = default;
        explicit Surface(int32_t w, int32_t h) :Surface(createOwned(w, h)) {}
        ~Surface() noexcept = default;

        operator Surface_ARGB32& () noexcept { return fInfo; }
        operator const Surface_ARGB32& () const noexcept { return fInfo; }

        Surface& operator=(const Surface& other) noexcept = default;
        Surface& operator=(Surface&&) noexcept = default;


        bool operator==(const Surface& other)
        {
            return ((fInfo.data == other.fInfo.data) &&
                (fInfo.width == other.fInfo.width) &&
                (fInfo.height == other.fInfo.height) &&
                (fInfo.stride == other.fInfo.stride));
        }


        Surface_ARGB32& info() noexcept { return fInfo; }
        const Surface_ARGB32& info() const noexcept { return fInfo; }

        bool empty() const noexcept { return !fInfo.data || fInfo.width <= 0 || fInfo.height <= 0;}

        bool hasOwnedBacking() const noexcept { return bool(fMemory); }
        bool isOwnedRoot() const noexcept { return fMemory && fInfo.data == fMemory.data(); }
        bool isBorrowed() const noexcept { return fInfo.data && !fMemory; }
        bool isContiguous() const noexcept {return fInfo.contiguous;}
        bool isView() const noexcept { return fMemory && fInfo.data != fMemory.data(); }
        bool isValid() const noexcept
        {
            return fInfo.data &&
                fInfo.width > 0 &&
                fInfo.height > 0 &&
                fInfo.stride >= ptrdiff_t(fInfo.width) * kBytesPerPixel;
        
        }


        WGRectI boundsI() const noexcept { return WGRectI{ 0, 0, fInfo.width, fInfo.height }; }
        WGRectD boundsD() const noexcept { return WGRectD{ 0.0, 0.0, double(fInfo.width), double(fInfo.height) };}

        size_t width() const noexcept { return size_t(fInfo.width); }
        size_t height() const noexcept { return size_t(fInfo.height); }
        size_t stride() const noexcept { return size_t(fInfo.stride); }

        const uint8_t* data() const noexcept { return fInfo.data; }
        uint8_t* data() noexcept { return fInfo.data; }

        uint32_t* rowPointer(int y) noexcept { return Surface_ARGB32_row_pointer(fInfo, y); }
        const uint32_t* rowPointer(int y) const noexcept { return Surface_ARGB32_row_pointer_const(fInfo, y); }


        Surface getView() const noexcept
        {
            Surface view;
            view.fMemory = fMemory;
            view.fInfo = fInfo;

            return view;
        }

        Surface getView(const WGRectI& r) const noexcept
        {
            Surface_ARGB32 subInfo = Surface_ARGB32_subview(fInfo, r);
            if (!Surface_ARGB32_is_valid(subInfo))
                return {};

            Surface view;
            view.fMemory = fMemory;
            view.fInfo = subInfo;

            return view;
        }


        // -----------------------------------
        // Factory constructors
        static Surface createBorrowed(
            uint8_t* data,
            size_t w,
            size_t h,
            size_t stride) noexcept
        {
            Surface s;

            if (!data || w == 0 || h == 0 || stride < w * kBytesPerPixel)
                return {};

            s.fInfo.data = data;
            s.fInfo.width = int32_t(w);
            s.fInfo.height = int32_t(h);
            s.fInfo.stride = ptrdiff_t(stride);
            s.fInfo.contiguous = (stride == w * kBytesPerPixel);

            return s;
        }

        static Surface createOwned(int32_t w, int32_t h) noexcept
        {
            Surface s;

            if (w <= 0 || h <= 0)
                return {};

            if (w > INT32_MAX / kBytesPerPixel)
                return {};

            const size_t stride = size_t(w) * kBytesPerPixel;
            
            if (stride > size_t(PTRDIFF_MAX))
                return {};

            if (size_t(h) > SIZE_MAX / stride)
                return {};
            
            const size_t totalBytes = stride * size_t(h);

            if (!s.fMemory.resetFromSize(totalBytes))
                return {};

            s.fInfo.data = s.fMemory.data();
            s.fInfo.width = w;
            s.fInfo.height = h;
            s.fInfo.stride = ptrdiff_t(stride);
            s.fInfo.contiguous = true;

            return s;
        }

    };

}