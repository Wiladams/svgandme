#pragma once

#include "filter_exec.h"
#include "pixel_program.h"


namespace waavs
{
    struct DisplacementMapProgram
    {
        const Surface_ARGB32* src = nullptr;

        int srcW = 0;
        int srcH = 0;

        int x0 = 0;
        int y0 = 0;

        float scaleX = 0.0f;
        float scaleY = 0.0f;

        FilterChannelSelector xChannel = FILTER_CHANNEL_A;
        FilterChannelSelector yChannel = FILTER_CHANNEL_A;
    };

    /*
    struct DisplacementMapReverseMapper
    {
        const uint32_t* map = nullptr;
        const DisplacementMapProgram* program = nullptr;

        INLINE WGPointD mapPoint(int x, int y, int i) const noexcept
        {
            const uint32_t mp = map[i];

            float mxv;
            float myv;

            argb32_premul_unpack_channels_to_straight(
                mp,
                program->xChannel,
                program->yChannel,
                mxv,
                myv);

            return {
                float(x) + (mxv - 0.5f) * program->scaleX,
                float(y) + (myv - 0.5f) * program->scaleY
            };
        }
    };
    */


    static INLINE float displacement_channel_straight_PRGB32(
        uint32_t px,
        FilterChannelSelector ch) noexcept
    {
        const uint32_t au = (px >> 24) & 0xffu;
        const float a = dequantize0_255(au);

        switch (ch)
        {
        default:
        case FILTER_CHANNEL_A:
            return a;

        case FILTER_CHANNEL_R:
            if (au == 0)
                return 0.0f;
            return clamp01f(dequantize0_255((px >> 16) & 0xffu) / a);

        case FILTER_CHANNEL_G:
            if (au == 0)
                return 0.0f;
            return clamp01f(dequantize0_255((px >> 8) & 0xffu) / a);

        case FILTER_CHANNEL_B:
            if (au == 0)
                return 0.0f;
            return clamp01f(dequantize0_255((px >> 0) & 0xffu) / a);
        }
    }


    static INLINE void displacement_channels_straight_PRGB32(
        uint32_t px,
        FilterChannelSelector xChannel,
        FilterChannelSelector yChannel,
        float& xValue,
        float& yValue) noexcept
    {
        if (xChannel == yChannel)
        {
            xValue = yValue = displacement_channel_straight_PRGB32(px, xChannel);
            return;
        }

        xValue = displacement_channel_straight_PRGB32(px, xChannel);
        yValue = displacement_channel_straight_PRGB32(px, yChannel);
    }

    static INLINE uint32_t displacementmap_sample_nearest_PRGB32(
        const DisplacementMapProgram& p,
        int x,
        int y,
        uint32_t mapPixel) noexcept
    {
        float mx;
        float my;

        displacement_channels_straight_PRGB32(
            mapPixel,
            p.xChannel,
            p.yChannel,
            mx,
            my);

        const float sxFloat = float(x) + (mx - 0.5f) * p.scaleX;
        const float syFloat = float(y) + (my - 0.5f) * p.scaleY;

        const int sx = iroundf_fast(sxFloat);
        const int sy = iroundf_fast(syFloat);

        if ((unsigned)sx >= (unsigned)p.srcW ||
            (unsigned)sy >= (unsigned)p.srcH)
        {
            return 0;
        }

        const uint32_t* srcRow = Surface_ARGB32_row_pointer_const(p.src, sy);
        return srcRow[sx];
    }

    static INLINE void displacementmap_prgb32_row_scalar(
        uint32_t* dst,
        const uint32_t* map,
        int count,
        const DisplacementMapProgram& p,
        int y) noexcept
    {
        int x = p.x0;

        for (int i = 0; i < count; ++i, ++x)
        {
            dst[i] = displacementmap_sample_nearest_PRGB32(
                p,
                x,
                y,
                map[i]);
        }
    }

}