#pragma once

#include <memory>

#include "blend2d.h"

#include "framesource.h"
#include "stopwatch.h"
#include "surface.h"
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
        Surface fPixels{};

        // Blend2D codec for decoding the image file
        BLImage fCaptureImage{};
        BLImageCodec fCodec;
        BLImageDecoder fDecoder;
        BLImageInfo fImageInfo{};

        size_t fCurrentFrame = 0;
        size_t fFrameCount = 0;



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
            //fCaptureWidth = fImageInfo.size.w;
            //fCaptureHeight = fImageInfo.size.h;

            // If we've gotten to here, we have successfully decoded the image info.  
            // We can use this to determine how many frames there are and what the dimensions are.
            
            fCropX = (int64_t)resolveNumberOrPercent(desc.cropX, fImageInfo.size.w, 0);
            fCropY = (int64_t)resolveNumberOrPercent(desc.cropY, fImageInfo.size.h, 0);
            fCropWidth = (int64_t)resolveNumberOrPercent(desc.cropW, fImageInfo.size.w, fImageInfo.size.w);
            fCropHeight = (int64_t)resolveNumberOrPercent(desc.cropH, fImageInfo.size.h, fImageInfo.size.h);

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

            // initialize the surface with the first frame of the image, 
            // so that we have something to draw immediately.
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
            BLResult res = fDecoder.readFrame(fCaptureImage, (const uint8_t*)fMappedFile->data(), fMappedFile->size());
            BLImageData imgData{};
            res = fCaptureImage.getData(&imgData);
            fPixels.createFromData((size_t)fCropWidth, (size_t)fCropHeight, 
                (size_t)imgData.stride, 
                (uint8_t *)imgData.pixelData);

            // make not of current time as last capture time
            fLastCaptureTime = fTimer.seconds();


            if (res != BL_SUCCESS)
                return false;

            return true;
        }

        Surface& pixels() noexcept override { return fPixels; }
        const Surface& pixels() const noexcept override { return fPixels; }

    };


 

} // namespace waavs

