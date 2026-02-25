#pragma once

#include <memory>

#include "blend2d.h"
#include "framesource.h"
#include "stopwatch.h"
#include "pixelaccessor.h"   // PixelArray
#include "nametable.h"
#include "charset.h"
#include "bspan.h"
#include "app/mappedfile.h"


namespace waavs 
{

    class Blend2DImageSource final :  public IFrameSource
    {
        static constexpr double kDefaultFrameRate = 15.0;

        bool fIsValild = false;

        // Hold onto the mapped file so it stays alive as long as we need the data
        std::shared_ptr<MappedFile> fMappedFile{};

        // Output pixels (what SVGFramesElement will render)
        PixelArray fPixels{};

        // Name of file we're displaying
        InternedKey fSourceKey{ nullptr };

        // Blend2D codec for decoding the image file
        BLImageCodec fCodec;
        BLImageDecoder fDecoder;
        BLImageInfo fImageInfo{};

        size_t fCurrentFrame = 0;
        size_t fFrameCount = 0;


        int64_t fCropX = 0;
        int64_t fCropY = 0;
        int64_t fCropWidth = 0;
        int64_t fCropHeight = 0;

        // Capture throttling
        StopWatch fTimer;
        double fMinInterval = 0.0;
        double fLastCaptureTime = 0;


    public:
        Blend2DImageSource() = default;

        void setMaxFrameRate(double fps)
        {
            fMinInterval = (fps > 0.0) ? (1.0 / fps) : 0.0;
        }

        void clampCropToImageBounds()
        {
            int sw = fImageInfo.size.w;
            int sh = fImageInfo.size.h;

            if (fCropX < 0) fCropX = 0;
            if (fCropY < 0) fCropY = 0;
            if (fCropWidth < 0) fCropWidth = 0;
            if (fCropHeight < 0) fCropHeight = 0;

            if (fCropX > sw) fCropX = sw;
            if (fCropY > sh) fCropY = sh;
            if (fCropX + fCropWidth > sw) fCropWidth = sw - fCropX;
            if (fCropY + fCropHeight > sh) fCropHeight = sh - fCropY;
        }

        bool reset(const FrameSourceDesc& desc) noexcept override
        {
            if (desc.src.empty())
                return false;

            // The 'src' field is the path to the image file.  
            // We will use Blend2D to decode it.
            fSourceKey = PSNameTable::INTERN(desc.src);

            fMappedFile = MappedFile::create_shared(fSourceKey);

            if (nullptr == fMappedFile)
                return false;

            // We have successfully mapped the file into memory.
            // Now we try to use codec API to decode it.
            BLResult res = fCodec.findByData(fMappedFile->data(), fMappedFile->size());
            //printf("Codec: %d\n", res);

            if (res != BL_SUCCESS)
                return false;

            // Get a decoder from the codec.
            
            res = fCodec.createDecoder(&fDecoder);
            //printf("Decoder: %d\n", res);

            if (res != BL_SUCCESS)
                return false;

            // Try to get the image info so we can see the size and frame count
            fImageInfo.reset();
            res = fDecoder.readInfo(fImageInfo, (const uint8_t*)fMappedFile->data(), fMappedFile->size());
            if (res != BL_SUCCESS)
                return false;

            // hold onto the frame count
            fFrameCount = fImageInfo.frameCount;

            // If we've gotten to here, we have successfully decoded the image info.  
            // We can use this to determine how many frames there are and what the dimensions are.
            
            fCropX = (int64_t)bindNumberOrPercent(desc.cropX, fImageInfo.size.w, 0);
            fCropY = (int64_t)bindNumberOrPercent(desc.cropY, fImageInfo.size.h, 0);
            fCropWidth = (int64_t)bindNumberOrPercent(desc.cropW, fImageInfo.size.w, fImageInfo.size.w);
            fCropHeight = (int64_t)bindNumberOrPercent(desc.cropH, fImageInfo.size.w, fImageInfo.size.w);

            clampCropToImageBounds();
            if (fCropWidth <= 0 || fCropHeight <= 0)
                return false;


            // Initialize the backing store
            if (!fPixels.reset(fCropWidth, fCropHeight))
                return false;

            fIsValild = true;

            // Set frames per second if specified, otherwise use default
            if (desc.maxFps > 0.0)
                setMaxFrameRate(desc.maxFps);
            else
                setMaxFrameRate(kDefaultFrameRate);  // default to 15 fps if not specified

            fLastCaptureTime = 0;

            // Get at least the first frame
            update();

            return true;
        }



        // take a snapshot
        bool update() noexcept override
        {
            if (!fIsValild)
                return false;

            // Peform rudimentary frame rate throttling
            // If the time since the last capture is less than the minimum interval, skip this capture
            double currentTime = fTimer.seconds();
            double currentInterval = currentTime - fLastCaptureTime;
            if (currentInterval < fMinInterval)
                return false;

            // Get the next frame in sequence
            fCurrentFrame = fDecoder.frameIndex();
            BLResult res = fDecoder.readFrame(fPixels.image(), (const uint8_t*)fMappedFile->data(), fMappedFile->size());

            // make not of current time as last capture time
            fLastCaptureTime = fTimer.seconds();


            if (res != BL_SUCCESS)
                return false;

            return true;
        }

        PixelArray& pixels() noexcept override { return fPixels; }
        const PixelArray& pixels() const noexcept override { return fPixels; }

    };


 

} // namespace waavs

