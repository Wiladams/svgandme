#pragma once

#include "framebuffer.h"



namespace waavs
{

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
        {
        }

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

}
