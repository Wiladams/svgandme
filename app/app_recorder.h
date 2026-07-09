// recorder.h

#pragma once

/*
    Record a Surface

        Then, to generate video

        ffmpeg -framerate 15 -i <name>%06d.bmp <outputname>.mp4
*/

#include <string>
#include <cstdio>

#include "blend2d_connect.h"
#include "surface.h"

namespace waavs
{
    class IRecordFrames
    {
    public:
        virtual ~IRecordFrames() = default;

        virtual void record() = 0;
        virtual void pause() = 0;
        virtual void resume() = 0;
        virtual void end() = 0;

        virtual bool isRecording() const noexcept = 0;
        virtual int64_t frameCount() const noexcept = 0;

        virtual bool recordFrame(const Surface& src, int64_t timeUs) = 0;
    };
}

namespace waavs
{
    // Make a series of keyframe files for recording from
    // a single surface.
    // It is assumed the surface changes over time
    struct MonoSurfaceRecorder : public IRecordFrames
    {
        Surface* fSurface{ nullptr };
        BLImage fImage{};
        BLImageCodec fCodec;

        // Recording state
        std::string fBasename;

        bool fIsRecording = false;
        bool fFrameRate = 30;   // frames per second
        int fFrameCount = 0;
        int fCurrentFrame = 0;
        int fMaxFrames = 0;

        // For now, just record frames as individual files, and then use ffmpeg to stitch them together into a video.
        // In the future, we could implement an actual video encoder here, but that is a bit more work.

        MonoSurfaceRecorder(const char* basename = "frame", double fps = 30, int maxFrames = 0)
        {
            fBasename = basename;
            fFrameRate = fps;
            fIsRecording = false;
            fCurrentFrame = 0;
            fMaxFrames = maxFrames;
            fCodec.find_by_name("QOI");
        }

        void record() override
        {
            // do whatever is necessary to begin
            // recording.
            fIsRecording = true;
        }

        void pause() override
        {
            fIsRecording = false;
        }

        void resume() override
        {
            fIsRecording = true;
        }

        void end() override
        {
            fIsRecording = false;
        }
        bool isRecording() const noexcept override
        {
            return fIsRecording;
        }
        int64_t frameCount() const noexcept override
        {
            return fFrameCount;
        }

        bool recordFrame(const Surface& src, int64_t timeUs) override
        {
            // For now, just save the frame as an image file.  In the future, we could implement an actual video encoder here.
            return true;
        }
    };
}




namespace waavs {
    class Recorder
    {
        Surface* fSurface{ nullptr };
        BLImage fImage{};
        BLImageCodec fCodec;
        
        std::string fBasename;
        int fFrameRate = 30;

        bool fIsRecording = false;
        int fCurrentFrame = 0;
        int fMaxFrames = 0;

        Recorder() = delete;    // Don't want default constructor

    public:
        Recorder(Surface * surf, const char* basename = "frame", int fps = 30, int maxFrames = 0)
            : fSurface(surf)
            , fBasename(basename)
            , fFrameRate(fps)
            , fIsRecording(false)
            , fCurrentFrame(0)
            , fMaxFrames(maxFrames)
        {
            if (surf != nullptr)
                fImage.create_from_data((int)surf->width(), (int)surf->height(), BL_FORMAT_PRGB32, surf->data(), surf->stride());
            
            fCodec.find_by_name("QOI");
        }

        void reset(Surface* surf, const char* basename = "frame", int fps = 30, int maxFrames = 0)
        {
            fSurface = surf;

            if (surf != nullptr)
            {
                fImage.create_from_data((int)surf->width(), (int)surf->height(), BL_FORMAT_PRGB32, surf->data(), surf->stride());
            }

            fBasename = basename;
            fFrameRate = fps;
            fIsRecording = false;
            fCurrentFrame = 0;
            fMaxFrames = maxFrames;
        }

        int frameCount() const { return fCurrentFrame; }

        bool isRecording() const { return fIsRecording; }
        void toggleRecording() {
            if (fIsRecording)
                pause();
            else
                record();
        }

        void saveFrame()
        {
            if (!fIsRecording || !fSurface)
                return;

            if (fMaxFrames > 0) {
                if (fCurrentFrame >= fMaxFrames) {
                    return;   // reached maximum frames, maybe stop timer
                }
            }

            char frameName[256];
            sprintf_s(frameName, 255, "%s%06d.qoi", fBasename.c_str(), fCurrentFrame);
            //BLImageCodec codec;
            //codec.findByName("QOI");
            fImage.write_to_file(frameName, fCodec);

            fCurrentFrame = fCurrentFrame + 1;
        }

        bool record()
        {
            // need to cancel existing timer
            if (fIsRecording)
                return false;

            fIsRecording = true;
            return true;
        }

        void pause()
        {
            fIsRecording = false;
        }

        void stop()
        {
            fCurrentFrame = 0;
            fIsRecording = false;
        }

    };

}