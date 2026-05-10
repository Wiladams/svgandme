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