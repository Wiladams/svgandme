#pragma once

#ifndef User32PixelMap_h
#define User32PixelMap_h



/*
    Simple representation of a bitmap we can use to draw into a window.
    We use a DIBSection because that's the easiest object within Windows 
    to get a pointer to the pixel data.

    we assume a format of 32-bit RGBA to make life very simple.

    A DC Context is also associated with the bitmap, to make it easier 
    to interact with other User32 and GDI interfaces.
*/

#include <windows.h>
#include <cstdio>


#include "maths.h"
#include "pixelaccessor.h"


namespace waavs {
    // Used to rapidly copy 32 bit values 
    static void memset_l(void* adr, int32_t val, size_t count) 
    {
		int32_t v;
		size_t i, n;
		uint32_t* p;
		p = static_cast<uint32_t *>(adr);
		v = val;

		// Do 4 at a time
		n = count >> 2;
		for (i = 0; i < n; i++) 
		{
			p[0] = v;
			p[1] = v;
			p[2] = v;
			p[3] = v;
			p += 4;
		}

		// Copy the last remaining values
		n = count & 3;
		for (i = 0; i < n; i++)
			*p++ = val;
}

    class User32PixelMap : public PixelAccessor<vec4b>
    {
        BITMAPINFO fBMInfo{ {0} };          // retain bitmap info for  future usage
        HBITMAP fDIBHandle = nullptr;       // Handle to the dibsection to be created
        HDC     fBitmapDC = nullptr;        // DeviceContext dib is selected into
        
        // A couple of constants
        static const int bitsPerPixel = 32;
        static const int alignment = 4;

        User32PixelMap(const User32PixelMap& other) = delete;

    public:
        User32PixelMap()
        {
            // Create a GDI Device Context
            fBitmapDC = ::CreateCompatibleDC(nullptr);

            // Do some setup to the DC to make it suitable
            // for drawing with GDI if we choose to do that
            ::SetGraphicsMode(fBitmapDC, GM_ADVANCED);
            ::SetBkMode(fBitmapDC, TRANSPARENT);        // for GDI text rendering
        }

        virtual ~User32PixelMap()
        {
            ;
            // and destroy it
            //::DeleteObject(fDIBHandle);
        }


        bool init(int awidth, int aheight)
        {
            // Delete the old DIBSection if it exists
			if (fDIBHandle)
			{
				::DeleteObject(fDIBHandle);
				fDIBHandle = nullptr;
			}


            size_t bytesPerRow = waavs::GetAlignedByteCount(awidth, bitsPerPixel, alignment);

            fBMInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            fBMInfo.bmiHeader.biWidth = (LONG)awidth;
            fBMInfo.bmiHeader.biHeight = -(LONG)aheight;	// top-down DIB Section
            fBMInfo.bmiHeader.biPlanes = 1;
            fBMInfo.bmiHeader.biClrImportant = 0;
            fBMInfo.bmiHeader.biClrUsed = 0;
            fBMInfo.bmiHeader.biCompression = BI_RGB;
            fBMInfo.bmiHeader.biBitCount = bitsPerPixel;
            fBMInfo.bmiHeader.biSizeImage = DWORD(bytesPerRow * aheight);

            // We'll create a DIBSection so we have an actual backing
            // storage for the context to draw into
            uint8_t* pdata = nullptr;
            fDIBHandle = ::CreateDIBSection(nullptr, &fBMInfo, DIB_RGB_COLORS, (void**)&pdata, nullptr, 0);

            if (nullptr == fDIBHandle)
                return false; 
            
            reset(pdata, awidth, aheight, bytesPerRow);

            // select the DIBSection into the memory context so we can 
            // peform GDI operations with it
            //auto bitmapobj = ::SelectObject(fBitmapDC, fDIBHandle);
            ::SelectObject(fBitmapDC, fDIBHandle);


            return true;
        }

        // Make sure all GDI drawing, if any, has completed
        void flush()
        {
            ::GdiFlush();
        }

        const BITMAPINFO& bitmapInfo() const { return fBMInfo; }
        HDC bitmapDC() const { return fBitmapDC; }

        size_t dataSize() const { return fBMInfo.bmiHeader.biSizeImage; }

        void setAllPixels(const vec4b& c) override
        {
            for (size_t row = 0; row < fHeight; row++)
                waavs::memset_l((uint8_t*)(fData)+(row * stride()), c.value, fWidth);
        }

    };
}

#endif  // User32PixelMap