#pragma once

#pragma comment(lib, "blend2d.lib")

#include "blend2d.h"
#include "surface.h"

namespace waavs
{
    // surfaceFromBLImage
    // 
    // Create a Surface wrapper of a BLImage.  Using this method
    // the Surface will point at the BLImage data, but it will NOT
    // own the memory for it.  The BLImage is still reponsible for 
    // the lifetime of the pixel data, and knows nothing about 
    // the existence of the Surface wrapper.
    // This is ok for temporary usage, where the wrapper is created
    // and rapidly discarded.
    // It's also useful in cases where the Surface and BLImage are
    // in the same lifetime scope.

    static INLINE Surface surfaceFromBLImage(const BLImage& img) noexcept
    {
        BLImageData imgData{};
        img.getData(&imgData);

        Surface surf{};
        surf.createFromData((size_t)img.width(), (size_t)img.height(), imgData.stride, (uint8_t *)imgData.pixelData);

        return surf;
    }

    // blImageFromSurface
    //
    // When you've got a Surface, and you need to pass it along to 
    // a blend2d interface.
    static INLINE BLImage blImageFromSurface(const Surface& surf) noexcept
    {
        BLImage img;
        img.createFromData((int)surf.width(), (int)surf.height(), BL_FORMAT_PRGB32, (uint8_t*)surf.data(), surf.stride());
        return img;
    }
}
