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


#include "user32pixelmap.h"
#include "stopwatch.h"
#include "nametable.h"
#include "framesource.h"


namespace waavs
{
    // Convenience class for managing a Windows 
    // Graphics Device Interface Context
    // This object owns the context, so it will be 
    // deleted when the object is destroyed.
    struct GraphicsDeviceContext final
    {
        HDC fDC{ nullptr };
    
        GraphicsDeviceContext() = default;
        
        ~GraphicsDeviceContext()
        {
            if (fDC != nullptr)
                ::DeleteDC(fDC);
        }

        bool resetByName(InternedKey key)
        {
            HDC dc = CreateDCA(key, nullptr, nullptr, nullptr);
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



    class ScreenSnapper final : public IFrameSource
    {
        static constexpr double kDefaultFrameRate = 15.0;

        InternedKey fDeviceKey{ nullptr };
        GraphicsDeviceContext fScreenDevice{};
        User32PixelMap fCaptureBitmap{};
        Surface fCaptureSurface{};

        int64_t fCapX = 0;
        int64_t fCapY = 0;
        int64_t fCapWidth = 0;
        int64_t fCapHeight = 0;

        // Capture throttling
        StopWatch fTimer;
        double fMinInterval = 0.0;
        double fLastCaptureTime = 0;


    public:
        ScreenSnapper() = default;

        void setMaxFrameRate(double fps)
        {
            fMinInterval = (fps > 0.0) ? (1.0 / fps) : 0.0;
        }

        bool reset(const FrameSourceDesc& desc) noexcept override
        {
            if (desc.src.empty())
                return false;

            // Need to turn the deviceName into something we can actuall open
            // Need to construct a small key name to hold the Windows
            // specific key for the device.  
            // This is typically something like "\\\\.\\DISPLAY1" for the default display, 
            // but it could be other things if you have multiple displays or want to capture 
            // a specific monitor.
            fDeviceKey = PSNameTable::INTERN(desc.src);
            if (!fScreenDevice.resetByName(fDeviceKey))
                return false;


            fCapX = (int64_t)resolveNumberOrPercent(desc.cropX, fScreenDevice.pixelWidth(), 0);
            fCapY = (int64_t)resolveNumberOrPercent(desc.cropY, fScreenDevice.pixelHeight(), 0);
            fCapWidth = (int64_t)resolveNumberOrPercent(desc.cropW, fScreenDevice.pixelWidth(), fScreenDevice.pixelWidth());
            fCapHeight = (int64_t)resolveNumberOrPercent(desc.cropH, fScreenDevice.pixelHeight(), fScreenDevice.pixelHeight());

            // Initialize the backing store
            if (!fCaptureBitmap.init(fCapWidth, fCapHeight))
                return false;

            fCaptureSurface.createFromData((size_t)fCapWidth, (size_t)fCapHeight, fCaptureBitmap.stride(), fCaptureBitmap.data());

            if (desc.maxFps > 0.0)
                setMaxFrameRate(desc.maxFps);
            else
                setMaxFrameRate(kDefaultFrameRate);  // default to 15 fps if not specified

            fLastCaptureTime = 0;
            nextFrame();

            return true;
        }


        bool nextFrame()
        {
            // Capture from the screen device into the local bitmap
            BOOL ok = ::StretchBlt(fCaptureBitmap.bitmapDC(), 0, 0, (int)fCaptureSurface.width(), (int)fCaptureSurface.height(),
                fScreenDevice.hdc(), fCapX, fCapY, fCapWidth, fCapHeight,
                SRCCOPY | CAPTUREBLT);
            
            if (ok == 0)
            {
                int err = ::GetLastError();
                printf("ScreenSnapper::next(), ERROR: 0x%x\n", err);
                return false;
            }

            return true;
        }


        // take a snapshot
        bool update() noexcept override
        {
            // Peform rudimentary frame rate throttling
            // If the time since the last capture is less than the minimum interval, skip this capture
            double currentTime = fTimer.seconds();
            double currentInterval = currentTime - fLastCaptureTime;
            if (currentInterval < fMinInterval)
                return false;

            bool success = nextFrame();

            // make not of current time as last capture time
            fLastCaptureTime = fTimer.seconds();

            return success;
        }

        Surface& pixels() noexcept override { return fCaptureSurface; }
        const Surface& pixels() const noexcept override { return fCaptureSurface; }

    };
}
