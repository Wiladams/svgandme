#pragma once

#ifndef Surface_h
#define Surface_h

//#pragma once

/*
    The mating between a DIBSection and a BLImage Graphics and BLContext

    This one object creates a UserPixelMap to be the
    backing storage for graphics.

    It then mates this with a BLContext so that we can
    do drawing with both GDI as well as blend2d.

    The object inherits from GLBraphics, so you can do drawing
    on the object directly.
*/

#include "svg/irendersvg.h"
#include "viewport.h"


#include <cstdio>


namespace waavs
{
    // A ready made surface for rendering svg graphics
    // it marries a BLImage with a BLContext, and adds 
    // a couple of extras.
    class SVGSurface : public IRenderSVG, public ISVGDrawable
    {
        // For interacting with blend2d
        BLImage fImage{};
        ViewPort fViewport{};

    public:
        SVGSurface(FontHandler* fontHandler)
            :IRenderSVG(fontHandler)
        {}
        
        SVGSurface(FontHandler* fontHandler, int w, int h, uint32_t threadCount = 0)
            :IRenderSVG(fontHandler)
        {
            makePixelArray(w, h, threadCount);
        }
        
		ViewPort& viewport() { return fViewport; }

        BLRect frame() { return BLRect(0, 0, getWidth(), getHeight()); }

        
        bool attachContext(uint32_t threadCount = 0)
        {
            // BUGBUG - with multi-thread, flush/sync doesn't quite seem
            // to be in sync, causing tearing.
            // Initialize the BLContext
            BLContextCreateInfo createInfo{};
            createInfo.commandQueueLimit = 255;
            createInfo.threadCount = threadCount;

            // Connect the context to the image
            BLContext::begin(fImage, createInfo);
            BLContext::clearAll();

            return true;
        }
        
        // Create an independent pixel array
        bool makePixelArray(int w, int h, uint32_t threadCount = 0)
        {
            fImage.reset();
			auto res = blImageInitAs(&fImage, w, h, BL_FORMAT_PRGB32);
            if (res != BL_SUCCESS)
            {
                printf("blImageInitAs failed with %d\n", res);
                return false;
            }
            
			// Create a context for the image
            attachContext(threadCount);

            return true;
        }
        

        
		// Attach to an already existant pixel array
        void attachPixelArray(PixelArray& pixmap, uint32_t threadCount = 0)
        {
            // Reset the BLImage so we can initialize it anew
            fImage.reset();

            // Initialize the BLImage
            // MUST use the PRGB32 in order for Win32 SRC_OVER operations to work correctly
            auto res = blImageInitAsFromData(&fImage, (int)pixmap.width(), (int)pixmap.height(), BL_FORMAT_PRGB32, pixmap.data(), (intptr_t)pixmap.stride(), BLDataAccessFlags::BL_DATA_ACCESS_RW, nullptr, nullptr);
            if (res != 0)
            {
                printf("Surface::attachPixelArray - FAILURE: %d\n", res);
                return;
            }

            attachContext();
            
        }


        virtual ~SVGSurface()
        {
            fImage.reset();
        }

        long getWidth() { BLImageData info{};  fImage.getData(&info); return info.size.w; }
        long getHeight() { BLImageData info{};  fImage.getData(&info); return info.size.h; }
        ptrdiff_t getStride() { BLImageData info{};  fImage.getData(&info); return ptrdiff_t(info.stride); }
        BLRgba32* getPixels() { BLImageData info{};  fImage.getData(&info); return (BLRgba32*)info.pixelData; }

        // Calculate whether a point is whithin our bounds
        bool containsPoint(int x, int y)
        {
            return (
                (x >= 0) && (x < fImage.width()) &&
                (y >= 0) && (y < fImage.height())
                );
        }

        BLImage& getImage() { return fImage; }

        // BUGBUG - need to take stride into account
        inline int pixelOffset(int x, int y)
        {
            return (y * fImage.width()) + x;
        }

        // Get a single pixel
        BLRgba32 get(int x, int y)
        {
            x = (int)clamp(x, 0, fImage.width() - 1);
            y = (int)clamp(y, 0, fImage.height() - 1);

            // Get data from BLContext
            int offset = pixelOffset(x, y);
            return ((BLRgba32 *)getPixels())[offset];
        }

        // Set a single pixel
        void set(int x, int y, const BLRgba32& c)
        {
            int offset = pixelOffset(x, y);

            ((BLRgba32*)getPixels())[offset] = c;
        }

        //===================================
        // ISVGDrawable
        // We are a drawable as well as a renderer
        // So, drawing will copy our image into 
        // the given context
		//===================================
        void draw(IRenderSVG* ctx)
        {
            ctx->image(fImage, 0, 0);
        }
    };

}

#endif  // Surface_h


