#pragma once

/*
    Take a snapshot of the video display using GDI.

    Construct an instance of the object by determining
    the location and size of the capture.  Once constructed
    the size and location are immutable.  If you want
    to capture a different location, create another capture
    object.

    The location could be moved easily, but changing the size
    will require changing the size of the underlying
    bitmap.
    
    // Example taking snapshot of default display
    ScreenSnapshot ss(x,y, width, height);

    //getting an actual snapshot, call update()

    ss.update()

    And finally, if you want to get the capture as a BLImage
    you can use:

    ss.getImage()

    But, the ScreenSnapper is itself a PixelArray/PixelAccessor, so you
    can retrieve and set pixels directly as well.

    References:
    https://www.codeproject.com/articles/5051/various-methods-for-capturing-the-screen
*/


#include "User32PixelMap.h"
#include "stopwatch.h"
#include "nametable.h"


namespace waavs
{
    // Convenience class for managing the screen device context
    struct GraphicsDeviceContext
    {
        HDC fDC{ nullptr };
    
        GraphicsDeviceContext() = default;
        
        bool resetByName(const char *deviceName)
        {
            HDC dc = CreateDCA(deviceName, nullptr, nullptr, nullptr);
            if (dc == nullptr)
                return false;

            return reset(dc);
        }

        bool reset(HDC dc)
        {
            fDC = dc;
            return true;
        }

        HDC hdc() const { return fDC; }

        int pixelWidth() const { return GetDeviceCaps(fDC, HORZRES); }
        int pixelHeight() const { return GetDeviceCaps(fDC, VERTRES);}

    };

    class ScreenSnapper : public User32PixelMap
    {
        bool fHasCaptureSource = false;
        GraphicsDeviceContext fScreenDevice{};

        int fCapX = 0;
        int fCapY = 0;
        int fCapWidth = 0;
        int fCapHeight = 0;

        // Capture throttling
        waavs::StopWatch fTimer;
        double fMinInterval = 0.0;
        double fLastCaptureTime = 0;

    public:
        ScreenSnapper() = default;

        bool resetDevice(const char *deviceName)
        {
            if (!fScreenDevice.resetByName(deviceName))
                return false;

            return true;
        }

        bool reset(int capX, int capY, int capWidth, int capHeight, const char *deviceName)
        {
            // First, delete current DC if it exists
            // But really, we don't know whether this is
            // the right thing to do.  Instead, the srcDC should
            // be a unique_ptr, so we don't have to make
            // the decision of its lifetime
            // For now, we'll just leave it alone, as we don't expect
            // this to be reset very frequently
            //if (nullptr != fSourceDC)
            //    ::DeleteDC(fSourceDC);
            if (!resetDevice(deviceName))
                return false;

            //if (srcDC == nullptr)
            //    fSourceDC = CreateDCA("DISPLAY", nullptr, nullptr, nullptr);
            //else
            //    fSourceDC = srcDC;

            //fScreenDevice.reset(fSourceDC);
            printf("ScreenSnapper::reset(), device: %s, pixel size: %dx%d\n", 
                deviceName, fScreenDevice.pixelWidth(), fScreenDevice.pixelHeight());

            fCapX = capX;
            fCapY = capY;
            fCapWidth = capWidth;
            fCapHeight = capHeight;

            init(capWidth, capHeight);

            // Bind the fImage to the pixel map
            // so we can use both blend2d and GDI on the same
            // pixel buffer.
            int lWidth = (int)width();
            int lHeight = (int)height();
            intptr_t lStride = (intptr_t)stride();
            blImageInitAsFromData(&fImage, lWidth, lHeight, BL_FORMAT_PRGB32, data(), lStride, BLDataAccessFlags::BL_DATA_ACCESS_RW, nullptr, nullptr);

            setMaxFrameRate(15);
            fLastCaptureTime = 0;

            return true;
        }

        void setMaxFrameRate(double fps)
        {
            fMinInterval = (1.0 / fps);
        }

        // take a snapshot
        bool update()
        {
            // get current time
            double currentTime = fTimer.seconds();
            double currentInterval = currentTime - fLastCaptureTime;
            if (currentInterval < fMinInterval)
                return false;

            // Capture from the screen device into the local bitmap
            auto bResult = ::StretchBlt(bitmapDC(), 0, 0, width(), height(), 
                fScreenDevice.hdc(), fCapX, fCapY, fCapWidth, fCapHeight, 
                SRCCOPY | CAPTUREBLT);

            if (bResult == 0)
            {
                DWORD err = ::GetLastError();
                printf("ScreenSnapper::next(), ERROR: 0x%x\n", err);
            }

            // Update the capture time to ensure we don't capture
            // faster than the specified frame rate
            fLastCaptureTime = currentTime;

            return (bResult != 0);
        }
    };
}
