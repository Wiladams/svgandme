// pixel_program.h
#pragma once

#include "definitions.h"
#include "surface_info.h"
#include "maths_base.h"


namespace waavs
{


    static INLINE void argb32_premul_unpack_channels_to_straight(
        uint32_t px,
        FilterChannelSelector xChannel,
        FilterChannelSelector yChannel,
        float& mxv,
        float& myv) noexcept
    {
        // Get the alpha channel as a straight value in [0, 1]
        const float a = dequantize0_255((px >> 24) & 0xFFu);

        auto decodeOne = [&](FilterChannelSelector ch) noexcept -> float
            {
                switch (ch)
                {
                default:
                case FILTER_CHANNEL_A:
                    return a;

                case FILTER_CHANNEL_R:
                    if (a <= 0.0f)
                        return 0.0f;
                    return clamp01f((dequantize0_255((px >> 16) & 0xFFu)) / a);

                case FILTER_CHANNEL_G:
                    if (a <= 0.0f)
                        return 0.0f;
                    return clamp01((dequantize0_255((px >> 8) & 0xFFu)) / a);

                case FILTER_CHANNEL_B:
                    if (a <= 0.0f)
                        return 0.0f;
                    return clamp01((dequantize0_255((px >> 0) & 0xFFu)) / a);
                }
            };

        if (xChannel == yChannel)
        {
            mxv = myv = decodeOne(xChannel);
            return;
        }

        mxv = decodeOne(xChannel);
        myv = decodeOne(yChannel);
    }
}