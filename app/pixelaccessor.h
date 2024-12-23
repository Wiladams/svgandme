#pragma once

#include <stdint.h>
#include "blend2d.h"

enum class PixelOrientation
{
    TopToBottom
    ,BottomToTop
};

//
// IContainPixels
// A generic interface to something that contains pixels
// 
// Limitations
// This structure will NOT work well for things smaller than a byte
// in size.  It assumes a minimum of one byte per pixel, so not
// great for super dense bitmap formats like a bit mask
//
struct IContainPixels
{
    size_t fWidth = 0;
    size_t fHeight = 0;
    ptrdiff_t fStride = 0;
    PixelOrientation fOrientation = PixelOrientation::TopToBottom;

    void orientation(PixelOrientation orient) { fOrientation = orient; }
    constexpr const PixelOrientation orientation() const noexcept { return fOrientation; }
    
    constexpr const size_t width() const noexcept { return fWidth; }
    constexpr const size_t height() const noexcept { return fHeight; }
    constexpr const ptrdiff_t stride() const noexcept { return fStride; }   // number of bytes to advance between rows

    virtual uint8_t* rowPointer(const size_t y) = 0;
    virtual const uint8_t* rowPointer(const size_t y) const = 0;
};



//
// PixelArray
// 
// Contains the information necessary to treat a chunk of memory
// as an array of pixels.
// 
// This object does not contain pixel type specific information.
// Pixel layout specifics are reserved for the PixelAccessor interface.
// This allows us to use the same PixelArray and access it using different
// kinds of accessors.  Perhaps you want the interface to be using HSL
// color values, instead of RGB, or whatever.
// 
// Memory is not allocated here.  So, the lifetime of the fData member
// is managed externally.  Most likely an object will either sub-class
// or encapsulate this PixelArray, and manage the memory.
// 
// This also makes it relatively easy to create sprite sheets, or
// other sub-section kinds of graphic elements.
//
struct PixelArray : public IContainPixels
{
    uint8_t* fData = nullptr;
    BLImage fImage{};                     // BLImage to hold the pixel data

    PixelArray() 
    {
	    fImage.reset();
    }
    PixelArray(void* d, const size_t w, const size_t h, const ptrdiff_t s, PixelOrientation o=PixelOrientation::TopToBottom)
    {
        reset(d, w, h, s, o);
    }


    // return reference to the fImage
	BLImage& image() { return fImage; }
	const BLImage& image() const { return fImage; }
    
    // Get a pointer to the raw data
    uint8_t* data() noexcept { return fData; }              // this one allows for editing
    const uint8_t* data() const noexcept { return fData; }  // this one's not editable

    // take the specified parameters as our own
    bool reset(void* d = nullptr, const size_t w = 0, const size_t h = 0, const ptrdiff_t s = 0, const PixelOrientation o=PixelOrientation::TopToBottom)
    {
        fData = (uint8_t*)d;
        fWidth = w;
        fHeight = h;

        fStride = s;
        fOrientation = o;

        // Create blend2d image
        //fImage.reset();
        BLResult br = fImage.createFromData(static_cast<int>(w), static_cast<int>(h), BL_FORMAT_PRGB32, fData, fStride);

        return br == BLResultCode::BL_SUCCESS;
        //printf("Result createfromData(): %d\n", br);
    }

    // Get a pointer to the beginning of a row
    uint8_t* rowPointer(const size_t y) override
    {
        if (fOrientation == PixelOrientation::TopToBottom)
            return (&fData[y * stride()]);
    
        // PixelOrientation::BottomToTop
        return(&fData[(height() - y - 1) * stride()]);
    }

    const uint8_t* rowPointer(const size_t y) const override
    {
        if (fOrientation == PixelOrientation::TopToBottom)
            return (&fData[y * stride()]);

        // PixelOrientation::BottomToTop
        return(&fData[(height() - y - 1) * stride()]);
    }


};


//
// PixelAccessor<T>
// This is a most generic pixel accessor.  It is a sub-class
// of PixelArrary for convenience of getting at the various
// attributes (width, height, stride, data).
// The real work here is to provide the set/getPixel methods
//
// Usage
// PixelAccessor<vec4b> rgbaaccessor(d,w,h,w*4);
//
template <typename TP>
struct PixelAccessor : public PixelArray
{
    PixelAccessor() { ; }

    PixelAccessor(void* d, const size_t w, const size_t h, const ptrdiff_t s, PixelOrientation o = PixelOrientation::TopToBottom)
        :PixelArray(d,w,h,s,o)
    {
    }

    virtual ~PixelAccessor() { ; }


    // Return a pointer to a specific pixel
    TP* pixelPointer(const size_t x, const size_t y)
    {
        return &((TP*)rowPointer(y))[x];
    }
    
    const INLINE TP* pixelPointer(const size_t x, const size_t y) const
    {
        return &((TP*)rowPointer(y))[x];
    }

    // Retrieve a single pixel
// This one does no bounds checking, so the behavior is undefined
// if the coordinates are beyond the boundary
    virtual TP getPixel(const size_t x, const size_t y) const
    {
        return *pixelPointer(x, y);
    }

    // Set a single pixel value
// Assume range checking has already occured
// Perform SRCCOPY operation on a pixel
    virtual void setPixel(const size_t x, const size_t y, const TP& c)
    {
        pixelPointer(x, y)[0] = c;
    }

    // Set every pixel to a specified value
// we can use this fast intrinsic to fill
// the whole area
    virtual void setAllPixels(const TP& c)
    {
        // Could use memset_l if we check size
        // of TP == 4
        for (int row = 0; row < static_cast<int>(height()); row++)
            for (size_t col = 0; col < static_cast<int>(width()); col++)
                setPixel(col, row, c);
    }
};


