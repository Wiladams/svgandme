#pragma once

#include "definitions.h"
#include "maths.h"

#include <math.h>
#include <stdint.h>

namespace waavs
{
    // ------------------------------------------------------------
    // Basic interpolation helpers
    // ------------------------------------------------------------

    static INLINE float cubic_smoothstep(float t) noexcept
    {
        // Cubic smoothstep: 3t^2 - 2t^3
        return t * t * (3.0f - 2.0f * t);
    }

    // ------------------------------------------------------------
    // SVG spec-style PRNG (Park-Miller)
    // ------------------------------------------------------------

    struct SVGParkMillerRNG
    {
        static constexpr int32_t kM = 2147483647;
        static constexpr int32_t kA = 16807;
        static constexpr int32_t kQ = 127773;
        static constexpr int32_t kR = 2836;

        int32_t fState{ 1 };

        INLINE void init(int32_t seed) noexcept
        {
            // Match the classic SVG/resvg normalization:
            // if (seed <= 0) seed = -seed % (kM - 1) + 1;
            // if (seed >  kM - 1) seed = kM - 1;
            if (seed <= 0)
                seed = (-seed % (kM - 1)) + 1;

            if (seed > (kM - 1))
                seed = kM - 1;

            fState = seed;
        }

        INLINE int32_t nextInt() noexcept
        {
            const int32_t hi = fState / kQ;
            const int32_t lo = fState % kQ;

            int32_t test = kA * lo - kR * hi;
            if (test <= 0)
                test += kM;

            fState = test;
            return fState;
        }

        INLINE float nextUnitFloat() noexcept
        {
            return (float)nextInt() / (float)kM;
        }
    };

    // ------------------------------------------------------------
    // Constants matching the classic SVG turbulence implementation
    // ------------------------------------------------------------

    static constexpr int32_t kTurbulenceTableSize = 256;
    static constexpr int32_t kTurbulenceTableMask = 255;
    static constexpr int32_t kPerlinN = 0x1000;

    // ------------------------------------------------------------
    // Gradient basis
    // ------------------------------------------------------------

    struct TurbulenceGradientTable
    {
        float gx[kTurbulenceTableSize];
        float gy[kTurbulenceTableSize];
    };

    static INLINE void buildGradientTable(TurbulenceGradientTable& g, SVGParkMillerRNG& rng) noexcept
    {
        for (int i = 0; i < kTurbulenceTableSize; ++i)
        {
            float x = float((rng.nextInt() % 512) - 256) * (1.0f / 256.0f);
            float y = float((rng.nextInt() % 512) - 256) * (1.0f / 256.0f);

            if (x == 0.0f && y == 0.0f)
                x = 1.0f;

            const float len = std::sqrt(x * x + y * y);
            g.gx[i] = x / len;
            g.gy[i] = y / len;
        }
    }

    static INLINE float gradDot(
        const TurbulenceGradientTable& g,
        int hash,
        float x,
        float y) noexcept
    {
        const int h = hash & kTurbulenceTableMask;
        return g.gx[h] * x + g.gy[h] * y;
    }

    // ------------------------------------------------------------
    // Shared permutation table
    // ------------------------------------------------------------

    struct TurbulencePermutationTable
    {
        uint8_t perm[512]{};
    };

    static INLINE void buildPermutation(
        TurbulencePermutationTable& p,
        SVGParkMillerRNG& rng) noexcept
    {
        for (int i = 0; i < kTurbulenceTableSize; ++i)
            p.perm[i] = (uint8_t)i;

        for (int i = kTurbulenceTableSize - 1; i > 0; --i)
        {
            const uint8_t tmp = p.perm[i];
            const int j = rng.nextInt() % kTurbulenceTableSize;
            p.perm[i] = p.perm[j];
            p.perm[j] = tmp;
        }

        for (int i = 0; i < kTurbulenceTableSize; ++i)
            p.perm[kTurbulenceTableSize + i] = p.perm[i];
    }

    // ------------------------------------------------------------
    // Stitch helpers
    // ------------------------------------------------------------

    struct TurbulenceStitchInfo
    {
        int32_t width{ 0 };
        int32_t height{ 0 };
        int32_t wrapX{ 0 };
        int32_t wrapY{ 0 };
    };

    static INLINE float snap_stitch_frequency(float baseFreq, float extent) noexcept
    {
        if (!(baseFreq > 0.0f) || !(extent > 0.0f))
            return baseFreq;

        const float scaled = extent * baseFreq;
        const float lo = floorf(scaled) / extent;
        const float hi = ceilf(scaled) / extent;

        if (!(lo > 0.0f))
            return hi;

        if (!(hi > 0.0f))
            return lo;

        return (baseFreq / lo < hi / baseFreq) ? lo : hi;
    }

    static INLINE TurbulenceStitchInfo make_stitch_info(
        float tileX,
        float tileY,
        float tileWidth,
        float tileHeight,
        float baseFreqX,
        float baseFreqY) noexcept
    {
        TurbulenceStitchInfo s{};
        s.width = (int32_t)floorf(tileWidth * baseFreqX + 0.5f);
        s.height = (int32_t)floorf(tileHeight * baseFreqY + 0.5f);
        s.wrapX = (int32_t)floorf(tileX * baseFreqX + (float)kPerlinN + (float)s.width);
        s.wrapY = (int32_t)floorf(tileY * baseFreqY + (float)kPerlinN + (float)s.height);
        return s;
    }

    static INLINE void update_stitch_info_for_next_octave(TurbulenceStitchInfo& s) noexcept
    {
        s.width *= 2;
        s.height *= 2;
        s.wrapX = 2 * s.wrapX - kPerlinN;
        s.wrapY = 2 * s.wrapY - kPerlinN;
    }

    // ------------------------------------------------------------
    // Base Perlin sample, matching classic SVG/resvg layout
    // ------------------------------------------------------------

    static INLINE float perlin2(
        float x,
        float y,
        const TurbulenceGradientTable& g,
        const TurbulencePermutationTable& p) noexcept
    {
        float t = x + (float)kPerlinN;
        int32_t bx0 = (int32_t)t;
        int32_t bx1 = bx0 + 1;
        float rx0 = t - (float)((int64_t)bx0);
        float rx1 = rx0 - 1.0f;

        t = y + (float)kPerlinN;
        int32_t by0 = (int32_t)t;
        int32_t by1 = by0 + 1;
        float ry0 = t - (float)((int64_t)by0);
        float ry1 = ry0 - 1.0f;

        bx0 &= kTurbulenceTableMask;
        bx1 &= kTurbulenceTableMask;
        by0 &= kTurbulenceTableMask;
        by1 &= kTurbulenceTableMask;

        const int i = p.perm[bx0];
        const int j = p.perm[bx1];

        const int b00 = p.perm[i + by0];
        const int b10 = p.perm[j + by0];
        const int b01 = p.perm[i + by1];
        const int b11 = p.perm[j + by1];

        const float sx = cubic_smoothstep(rx0);
        const float sy = cubic_smoothstep(ry0);

        const float u00 = gradDot(g, b00, rx0, ry0);
        const float u10 = gradDot(g, b10, rx1, ry0);
        const float a = lerp(u00, u10, sx);

        const float u01 = gradDot(g, b01, rx0, ry1);
        const float u11 = gradDot(g, b11, rx1, ry1);
        const float b = lerp(u01, u11, sx);

        return lerp(a, b, sy);
    }

    static INLINE float perlin2_stitch(
        float x,
        float y,
        const TurbulenceGradientTable& g,
        const TurbulencePermutationTable& p,
        const TurbulenceStitchInfo& stitch) noexcept
    {
        float t = x + (float)kPerlinN;
        int32_t bx0 = (int32_t)t;
        int32_t bx1 = bx0 + 1;
        float rx0 = t - (float)((int64_t)bx0);
        float rx1 = rx0 - 1.0f;

        t = y + (float)kPerlinN;
        int32_t by0 = (int32_t)t;
        int32_t by1 = by0 + 1;
        float ry0 = t - (float)((int64_t)by0);
        float ry1 = ry0 - 1.0f;

        if (bx0 >= stitch.wrapX) bx0 -= stitch.width;
        if (bx1 >= stitch.wrapX) bx1 -= stitch.width;
        if (by0 >= stitch.wrapY) by0 -= stitch.height;
        if (by1 >= stitch.wrapY) by1 -= stitch.height;

        bx0 &= kTurbulenceTableMask;
        bx1 &= kTurbulenceTableMask;
        by0 &= kTurbulenceTableMask;
        by1 &= kTurbulenceTableMask;

        const int i = p.perm[bx0];
        const int j = p.perm[bx1];

        const int b00 = p.perm[i + by0];
        const int b10 = p.perm[j + by0];
        const int b01 = p.perm[i + by1];
        const int b11 = p.perm[j + by1];

        const float sx = cubic_smoothstep(rx0);
        const float sy = cubic_smoothstep(ry0);

        const float u00 = gradDot(g, b00, rx0, ry0);
        const float u10 = gradDot(g, b10, rx1, ry0);
        const float a = lerp(u00, u10, sx);

        const float u01 = gradDot(g, b01, rx0, ry1);
        const float u11 = gradDot(g, b11, rx1, ry1);
        const float b = lerp(u01, u11, sx);

        return lerp(a, b, sy);
    }

    // ------------------------------------------------------------
    // Turbulence parameter block
    // ------------------------------------------------------------

    struct TurbulenceNoiseParams
    {
        float baseFreqX{ 0.01f };
        float baseFreqY{ 0.01f };
        float seed{ 0.0f };
        uint32_t octaves{ 1 };
    };

    // ------------------------------------------------------------
    // Per-channel turbulence state
    // ------------------------------------------------------------

    struct TurbulenceChannelState
    {
        TurbulenceGradientTable grads{};
    };

    struct TurbulenceState
    {
        TurbulencePermutationTable perm{};
        TurbulenceChannelState channels[4]{};
    };

    static INLINE void buildTurbulenceState(
        TurbulenceState& s,
        int32_t seed) noexcept
    {
        SVGParkMillerRNG rng;
        rng.init(seed);

        for (int i = 0; i < 4; ++i)
            buildGradientTable(s.channels[i].grads, rng);

        buildPermutation(s.perm, rng);
    }

    // ------------------------------------------------------------
    // Raw accumulation helpers
    // ------------------------------------------------------------

    static INLINE float fractalNoise2(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        const TurbulenceChannelState& s,
        const TurbulencePermutationTable& perm) noexcept
    {
        float sum = 0.0f;
        float ratio = 1.0f;
        float px = x * p.baseFreqX;
        float py = y * p.baseFreqY;

        for (uint32_t o = 0; o < p.octaves; ++o)
        {
            sum += perlin2(px, py, s.grads, perm) / ratio;
            px *= 2.0f;
            py *= 2.0f;
            ratio *= 2.0f;
        }

        return sum;
    }

    static INLINE float turbulence2(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        const TurbulenceChannelState& s,
        const TurbulencePermutationTable& perm) noexcept
    {
        float sum = 0.0f;
        float ratio = 1.0f;
        float px = x * p.baseFreqX;
        float py = y * p.baseFreqY;

        for (uint32_t o = 0; o < p.octaves; ++o)
        {
            sum += fabsf(perlin2(px, py, s.grads, perm)) / ratio;
            px *= 2.0f;
            py *= 2.0f;
            ratio *= 2.0f;
        }

        return sum;
    }

    static INLINE float fractalNoise2_stitch(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        TurbulenceStitchInfo stitch,
        const TurbulenceChannelState& s,
        const TurbulencePermutationTable& perm) noexcept
    {
        float sum = 0.0f;
        float ratio = 1.0f;
        float px = x * p.baseFreqX;
        float py = y * p.baseFreqY;

        for (uint32_t o = 0; o < p.octaves; ++o)
        {
            sum += perlin2_stitch(px, py, s.grads, perm, stitch) / ratio;
            px *= 2.0f;
            py *= 2.0f;
            ratio *= 2.0f;
            update_stitch_info_for_next_octave(stitch);
        }

        return sum;
    }

    static INLINE float turbulence2_stitch(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        TurbulenceStitchInfo stitch,
        const TurbulenceChannelState& s,
        const TurbulencePermutationTable& perm) noexcept
    {
        float sum = 0.0f;
        float ratio = 1.0f;
        float px = x * p.baseFreqX;
        float py = y * p.baseFreqY;

        for (uint32_t o = 0; o < p.octaves; ++o)
        {
            sum += fabsf(perlin2_stitch(px, py, s.grads, perm, stitch)) / ratio;
            px *= 2.0f;
            py *= 2.0f;
            ratio *= 2.0f;
            update_stitch_info_for_next_octave(stitch);
        }

        return sum;
    }

    // ------------------------------------------------------------
    // Final SVG-style mapping helpers
    // ------------------------------------------------------------

    static INLINE float fractal_to_unit(float v) noexcept
    {
        return clamp01(v * 0.5f + 0.5f);
    }

    static INLINE float turbulence_to_unit(float v) noexcept
    {
        return clamp01(v);
    }

    // ------------------------------------------------------------
    // Convenience helpers for preparing stitch frequencies/state
    // ------------------------------------------------------------

    static INLINE void adjust_base_frequencies_for_stitch(
        float tileWidth,
        float tileHeight,
        float& baseFreqX,
        float& baseFreqY) noexcept
    {
        if (baseFreqX > 0.0f)
            baseFreqX = snap_stitch_frequency(baseFreqX, tileWidth);

        if (baseFreqY > 0.0f)
            baseFreqY = snap_stitch_frequency(baseFreqY, tileHeight);
    }

    static INLINE TurbulenceStitchInfo prepare_stitch_info(
        float tileX,
        float tileY,
        float tileWidth,
        float tileHeight,
        float baseFreqX,
        float baseFreqY) noexcept
    {
        return make_stitch_info(tileX, tileY, tileWidth, tileHeight, baseFreqX, baseFreqY);
    }

    // ------------------------------------------------------------
    // Convenience helpers for sampling RGBA channels
    // ------------------------------------------------------------

    static INLINE float sampleFractalChannel(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        const TurbulenceState& s,
        int channel,
        bool stitchTiles,
        const TurbulenceStitchInfo* stitchInfo) noexcept
    {
        if ((unsigned)channel >= 4u)
            return 0.0f;

        if (stitchTiles && stitchInfo != nullptr)
        {
            return fractal_to_unit(
                fractalNoise2_stitch(
                    x, y, p,
                    *stitchInfo,
                    s.channels[channel],
                    s.perm));
        }

        return fractal_to_unit(
            fractalNoise2(
                x, y, p,
                s.channels[channel],
                s.perm));
    }

    static INLINE float sampleTurbulenceChannel(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        const TurbulenceState& s,
        int channel,
        bool stitchTiles,
        const TurbulenceStitchInfo* stitchInfo) noexcept
    {
        if ((unsigned)channel >= 4u)
            return 0.0f;

        if (stitchTiles && stitchInfo != nullptr)
        {
            return turbulence_to_unit(
                turbulence2_stitch(
                    x, y, p,
                    *stitchInfo,
                    s.channels[channel],
                    s.perm));
        }

        return turbulence_to_unit(
            turbulence2(
                x, y, p,
                s.channels[channel],
                s.perm));
    }
}
