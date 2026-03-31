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

    static INLINE float fade(float t) noexcept
    {
        // Perlin fade curve: 6t^5 - 15t^4 + 10t^3
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }



    // ------------------------------------------------------------
    // SVG Spec-style PRNG (Park-Miller)
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
            if (seed <= 0)
            {
                seed = -seed + 1;
                if (seed <= 0)
                    seed = 1;
            }

            if (seed >= kM)
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
    // Gradient basis
    // ------------------------------------------------------------

    struct TurbulenceGradientTable
    {
        float gx[256];
        float gy[256];
    };

    static INLINE void buildGradientTable(TurbulenceGradientTable& g, SVGParkMillerRNG& rng) noexcept
    {
        static constexpr float kTwoPi = (float)Pi2;

        for (int i = 0; i < 256; ++i)
        {
            const float a = rng.nextUnitFloat() * kTwoPi;
            g.gx[i] = cosf(a);
            g.gy[i] = sinf(a);
        }
    }

    static INLINE float gradDot(
        const TurbulenceGradientTable& g,
        int hash,
        float x,
        float y) noexcept
    {
        const int h = hash & 255;
        return g.gx[h] * x + g.gy[h] * y;
    }

    // ------------------------------------------------------------
    // Permutation table
    // ------------------------------------------------------------

    static INLINE void buildPermutation(uint8_t perm[512], SVGParkMillerRNG& rng) noexcept
    {
        uint8_t p[256];
        for (int i = 0; i < 256; ++i)
            p[i] = (uint8_t)i;

        for (int i = 255; i > 0; --i)
        {
            const int j = rng.nextInt() % (i + 1);

            const uint8_t tmp = p[i];
            p[i] = p[j];
            p[j] = tmp;
        }

        for (int i = 0; i < 256; ++i)
        {
            perm[i] = p[i];
            perm[i + 256] = p[i];
        }
    }

    // ------------------------------------------------------------
    // Base Perlin sample, non-periodic
    // ------------------------------------------------------------

    // Returns a signed value roughly in [-1, 1].
    static INLINE float perlin2(
        float x,
        float y,
        const TurbulenceGradientTable& g,
        const uint8_t perm[512]) noexcept
    {
        const int ix0 = floorI(x);
        const int iy0 = floorI(y);

        const float fx0 = x - (float)ix0;
        const float fy0 = y - (float)iy0;
        const float fx1 = fx0 - 1.0f;
        const float fy1 = fy0 - 1.0f;

        const int X = ix0 & 255;
        const int Y = iy0 & 255;

        const float u = fade(fx0);
        const float v = fade(fy0);

        const int aa = perm[perm[X] + Y];
        const int ab = perm[perm[X] + ((Y + 1) & 255)];
        const int ba = perm[perm[(X + 1) & 255] + Y];
        const int bb = perm[perm[(X + 1) & 255] + ((Y + 1) & 255)];

        const float x1 = lerp(
            gradDot(g, aa, fx0, fy0),
            gradDot(g, ba, fx1, fy0),
            u);

        const float x2 = lerp(
            gradDot(g, ab, fx0, fy1),
            gradDot(g, bb, fx1, fy1),
            u);

        // sqrt(2)/2 basis normalization.
        return lerp(x1, x2, v) * 0.70710678f;
    }

    // ------------------------------------------------------------
    // Periodic helpers for stitchTiles="stitch"
    // ------------------------------------------------------------

    static INLINE int positive_mod(int x, int m) noexcept
    {
        if (m <= 0)
            return x;

        x %= m;
        if (x < 0)
            x += m;
        return x;
    }

    static INLINE int periodicIndex(int i, int period) noexcept
    {
        if (period > 0)
            return positive_mod(i, period);
        return i;
    }

    // Periodic Perlin sample using the same gradient/permutation tables.
    // periodX/periodY are lattice periods, not pixel counts.
    static INLINE float perlin2_periodic(
        float x,
        float y,
        int32_t periodX,
        int32_t periodY,
        const TurbulenceGradientTable& g,
        const uint8_t perm[512]) noexcept
    {
        const int ix0_raw = floorI(x);
        const int iy0_raw = floorI(y);
        const int ix1_raw = ix0_raw + 1;
        const int iy1_raw = iy0_raw + 1;

        const float fx0 = x - (float)ix0_raw;
        const float fy0 = y - (float)iy0_raw;
        const float fx1 = fx0 - 1.0f;
        const float fy1 = fy0 - 1.0f;

        const int ix0 = periodicIndex(ix0_raw, periodX);
        const int iy0 = periodicIndex(iy0_raw, periodY);
        const int ix1 = periodicIndex(ix1_raw, periodX);
        const int iy1 = periodicIndex(iy1_raw, periodY);

        const int X0 = ix0 & 255;
        const int Y0 = iy0 & 255;
        const int X1 = ix1 & 255;
        const int Y1 = iy1 & 255;

        const float u = fade(fx0);
        const float v = fade(fy0);

        const int aa = perm[perm[X0] + Y0];
        const int ab = perm[perm[X0] + Y1];
        const int ba = perm[perm[X1] + Y0];
        const int bb = perm[perm[X1] + Y1];

        const float x1 = lerp(
            gradDot(g, aa, fx0, fy0),
            gradDot(g, ba, fx1, fy0),
            u);

        const float x2 = lerp(
            gradDot(g, ab, fx0, fy1),
            gradDot(g, bb, fx1, fy1),
            u);

        return lerp(x1, x2, v) * 0.70710678f;
    }

    // ------------------------------------------------------------
    // Turbulence parameter block
    // ------------------------------------------------------------

    struct TurbulenceNoiseParams
    {
        float baseFreqX{ 0.01f };
        float baseFreqY{ 0.01f };
        uint32_t octaves{ 1 };
        uint64_t seed{ 1 };
    };

    static INLINE int32_t safe_period_from_extent(float extent, float baseFreq) noexcept
    {
        if (!(extent > 0.0f) || !(baseFreq > 0.0f))
            return 1;

        int32_t period = (int32_t)lroundf(extent * baseFreq);
        if (period < 1)
            period = 1;

        return period;
    }

    // ------------------------------------------------------------
    // Per-channel turbulence state
    // ------------------------------------------------------------

    struct TurbulenceChannelState
    {
        TurbulenceGradientTable grads{};
        uint8_t perm[512]{};
    };

    static INLINE void buildTurbulenceChannelState(
        TurbulenceChannelState& s,
        SVGParkMillerRNG& rng) noexcept
    {
        buildGradientTable(s.grads, rng);
        buildPermutation(s.perm, rng);
    }

    static INLINE void buildTurbulenceChannelStates(
        TurbulenceChannelState states[4],
        int32_t seed) noexcept
    {
        SVGParkMillerRNG rng;
        rng.init(seed);

        // Build channels in R, G, B, A order.
        for (int i = 0; i < 4; ++i)
            buildTurbulenceChannelState(states[i], rng);
    }

    // ------------------------------------------------------------
    // Non-stitched accumulation
    // ------------------------------------------------------------

    // Returns a signed result. Map later with fractal_to_unit().
    static INLINE float fractalNoise2(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        const TurbulenceChannelState& s) noexcept
    {
        float sum = 0.0f;
        float amp = 1.0f;
        float freqX = p.baseFreqX;
        float freqY = p.baseFreqY;

        for (uint32_t o = 0; o < p.octaves; ++o)
        {
            const float n = perlin2(x * freqX, y * freqY, s.grads, s.perm);
            sum += n * amp;

            freqX *= 2.0f;
            freqY *= 2.0f;
            amp *= 0.5f;
        }

        return sum;
    }

    // Returns a non-negative result. Map later with turbulence_to_unit().
    static INLINE float turbulence2(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        const TurbulenceChannelState& s) noexcept
    {
        float sum = 0.0f;
        float amp = 1.0f;
        float freqX = p.baseFreqX;
        float freqY = p.baseFreqY;

        for (uint32_t o = 0; o < p.octaves; ++o)
        {
            float n = perlin2(x * freqX, y * freqY, s.grads, s.perm);
            n = fabsf(n);
            sum += n * amp;

            freqX *= 2.0f;
            freqY *= 2.0f;
            amp *= 0.5f;
        }

        return sum;
    }

    // ------------------------------------------------------------
    // Stitched accumulation
    // ------------------------------------------------------------

    static INLINE float fractalNoise2_stitch(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        int32_t basePeriodX,
        int32_t basePeriodY,
        const TurbulenceChannelState& s) noexcept
    {
        float sum = 0.0f;
        float amp = 1.0f;
        float freqX = p.baseFreqX;
        float freqY = p.baseFreqY;

        int32_t periodX = basePeriodX;
        int32_t periodY = basePeriodY;

        for (uint32_t o = 0; o < p.octaves; ++o)
        {
            const float n = perlin2_periodic(
                x * freqX,
                y * freqY,
                periodX,
                periodY,
                s.grads,
                s.perm);

            sum += n * amp;

            freqX *= 2.0f;
            freqY *= 2.0f;
            amp *= 0.5f;

            if (periodX > 0) periodX *= 2;
            if (periodY > 0) periodY *= 2;
        }

        return sum;
    }

    static INLINE float turbulence2_stitch(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        int32_t basePeriodX,
        int32_t basePeriodY,
        const TurbulenceChannelState& s) noexcept
    {
        float sum = 0.0f;
        float amp = 1.0f;
        float freqX = p.baseFreqX;
        float freqY = p.baseFreqY;

        int32_t periodX = basePeriodX;
        int32_t periodY = basePeriodY;

        for (uint32_t o = 0; o < p.octaves; ++o)
        {
            float n = perlin2_periodic(
                x * freqX,
                y * freqY,
                periodX,
                periodY,
                s.grads,
                s.perm);

            n = fabsf(n);
            sum += n * amp;

            freqX *= 2.0f;
            freqY *= 2.0f;
            amp *= 0.5f;

            if (periodX > 0) periodX *= 2;
            if (periodY > 0) periodY *= 2;
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
    // Convenience helpers for sampling RGBA
    // ------------------------------------------------------------

    static INLINE float sampleFractalChannel(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        const TurbulenceChannelState& s,
        bool stitchTiles,
        int32_t basePeriodX,
        int32_t basePeriodY) noexcept
    {
        if (stitchTiles)
            return fractal_to_unit(fractalNoise2_stitch(x, y, p, basePeriodX, basePeriodY, s));

        return fractal_to_unit(fractalNoise2(x, y, p, s));
    }

    static INLINE float sampleTurbulenceChannel(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        const TurbulenceChannelState& s,
        bool stitchTiles,
        int32_t basePeriodX,
        int32_t basePeriodY) noexcept
    {
        if (stitchTiles)
            return turbulence_to_unit(turbulence2_stitch(x, y, p, basePeriodX, basePeriodY, s));

        return turbulence_to_unit(turbulence2(x, y, p, s));
    }
}