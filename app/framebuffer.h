#pragma once


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <blend2d.h>

#include <vector>
#include <memory>


namespace waavs {
    //
    // AFrameBuffer - A structure that combines a GDI DIBSection with a blend2d BLImage
    // You can get either a blend2d context out of it, 
    // or a GDI32 Drawing Context
    // The BLImage is used to represent the image data for ease of use
    //
    struct AFrameBuffer
    {
        uint8_t* fFrameBufferData{ nullptr };
        size_t fBytesPerRow{ 0 };

        
        BITMAPINFO fGDIBMInfo{ {0} };          // retain bitmap info for  future usage
        HBITMAP fGDIDIBHandle = nullptr;       // Handle to the dibsection to be created
        HDC     fGDIBitmapDC = nullptr;        // DeviceContext dib is selected into

        // Blend2d image and context
        BLContext fB2dContext{};
        BLImage fB2dImage{};


        AFrameBuffer() = default;
        
        AFrameBuffer(int w, int h)
        {
            reset(w, h);
        }

        virtual ~AFrameBuffer()
        {
            fB2dContext.end();

            ::DeleteDC(fGDIBitmapDC);
            ::DeleteObject(fGDIDIBHandle);
        }

        void* data() const { return fFrameBufferData; }
        size_t stride() const { return fBytesPerRow; }
		int width() const { return fGDIBMInfo.bmiHeader.biWidth; }
		int height() const { return std::abs(fGDIBMInfo.bmiHeader.biHeight); }
        
        BLImage& getBlend2dImage() { return fB2dImage; }
        BLContext& getBlend2dContext() { return fB2dContext; }

        const BITMAPINFO& bitmapInfo() const { return fGDIBMInfo; }
        HDC getGDIContext() { return fGDIBitmapDC; }

        


        void resetGDIDC()
        {
            if (fGDIBitmapDC)
                ::DeleteDC(fGDIBitmapDC);

            // Create a GDI Device Context
            fGDIBitmapDC = ::CreateCompatibleDC(nullptr);

            // Do some setup to the DC to make it suitable
            // for drawing with GDI if we choose to do that
            ::SetGraphicsMode(fGDIBitmapDC, GM_ADVANCED);
            ::SetBkMode(fGDIBitmapDC, TRANSPARENT);        // for GDI text rendering

        }

        void resetGDIDibSection(int w, int h)
        {
            // The framebuffer is based on a DIBSection
            if (fGDIDIBHandle)
            {
                // destroy current DIB
                DeleteObject(fGDIDIBHandle);
                fGDIDIBHandle = nullptr;
            }

            fBytesPerRow = 4 * w;
            fGDIBMInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            fGDIBMInfo.bmiHeader.biWidth = (LONG)w;
            fGDIBMInfo.bmiHeader.biHeight = -(LONG)h;	// top-down DIB Section
            fGDIBMInfo.bmiHeader.biPlanes = 1;
            fGDIBMInfo.bmiHeader.biClrImportant = 0;
            fGDIBMInfo.bmiHeader.biClrUsed = 0;
            fGDIBMInfo.bmiHeader.biCompression = BI_RGB;
            fGDIBMInfo.bmiHeader.biBitCount = 32;
            fGDIBMInfo.bmiHeader.biSizeImage = DWORD(fBytesPerRow * h);


            fGDIDIBHandle = ::CreateDIBSection(nullptr, &fGDIBMInfo, DIB_RGB_COLORS, (void**)&fFrameBufferData, nullptr, 0);

            if ((fGDIBitmapDC != nullptr) && (fGDIDIBHandle != nullptr))
                ::SelectObject(fGDIBitmapDC, fGDIDIBHandle);
        }

        void resetB2d(int w, int h)
        {
            // Then we tie a BLImage to the same so
            // we can use both drawing APIs
            if (!fB2dImage.empty())
                fB2dImage.reset();

            BLResult br = fB2dImage.createFromData(static_cast<int>(w), static_cast<int>(h), BL_FORMAT_PRGB32, fFrameBufferData, fBytesPerRow);

            // Create a context for the image
            BLContextCreateInfo ctxInfo{};
            ctxInfo.threadCount = 4;

            fB2dContext.begin(fB2dImage, &ctxInfo);
        }

        void reset(int w, int h)
        {
            fB2dContext.end();

            resetGDIDC();
            resetGDIDibSection(w, h);
            resetB2d(w, h);
        }

        // Some drawing
        // clear the contents of the framebuffer to all '0' values
        void clear() 
        {
			memset(fFrameBufferData, 0, fBytesPerRow * height());
        }
    };

    // struct SwapChain
    // A structure consisting of a specified number of AFrameBuffer
    // rendering targets.  They can be swapped using the 'swap()' method.
    //
    struct ASwapChain {
        std::vector<std::unique_ptr<AFrameBuffer>> fBuffers{};

        size_t fNumBuffers{ 0 };
        size_t fFrontBufferIndex{ 0 };

        ASwapChain(size_t sz)
            : ASwapChain(10, 10, sz)
        {}

        ASwapChain(int w, int h, size_t sz)
            : fNumBuffers(sz)
        {
            reset(w, h);
        }

        void reset(int w, int h)
        {
            fBuffers.clear();
            for (size_t i = 0; i < fNumBuffers; i++)
            {
                auto fb = std::make_unique<AFrameBuffer>(w, h);

                fBuffers.push_back(std::move(fb));
            }

            fFrontBufferIndex = 0;
        }

        size_t swap()
        {
            fFrontBufferIndex = (fFrontBufferIndex + 1) % fNumBuffers;
            return fFrontBufferIndex;
        }

        AFrameBuffer* getNthBuffer(size_t n)
        {
            size_t realIndex = (fFrontBufferIndex + n) % fNumBuffers;
            return fBuffers[realIndex].get();
        }

        AFrameBuffer* getFrontBuffer()
        {
            return getNthBuffer(0);
        }

        AFrameBuffer* getNextBuffer()
        {
            return getNthBuffer(1);
        }

    };

} // namespace waavs

