#pragma once

#include "definitions.h"

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

    static INLINE float noise_lerp(float a, float b, float t) noexcept
    {
        return a + (b - a) * t;
    }

    static INLINE int fastFloor(float x) noexcept
    {
        int i = (int)x;
        return (x < (float)i) ? (i - 1) : i;
    }

    // ------------------------------------------------------------
    // Classic Perlin helpers
    // ------------------------------------------------------------

    static INLINE float grad(int hash, float x, float y) noexcept
    {
        // 2D gradient selection returning a scalar dot product.
        switch (hash & 7)
        {
        default:
        case 0: return  x + y;
        case 1: return -x + y;
        case 2: return  x - y;
        case 3: return -x - y;
        case 4: return  x;
        case 5: return -x;
        case 6: return  y;
        case 7: return -y;
        }
    }

    static INLINE void buildPermutation(uint8_t perm[512], uint32_t seed) noexcept
    {
        uint8_t p[256];
        for (uint32_t i = 0; i < 256; ++i)
            p[i] = (uint8_t)i;

        // Deterministic Fisher-Yates using a simple LCG.
        uint32_t s = seed ? seed : 1u;
        for (int i = 255; i > 0; --i)
        {
            s = s * 1664525u + 1013904223u;
            const uint32_t j = s % (uint32_t)(i + 1);

            const uint8_t tmp = p[i];
            p[i] = p[j];
            p[j] = tmp;
        }

        for (uint32_t i = 0; i < 256; ++i)
        {
            perm[i] = p[i];
            perm[i + 256] = p[i];
        }
    }

    // Single scalar 2D Perlin-style noise sample.
    // Output is roughly in [-1, 1].
    static INLINE float perlin2(float x, float y, const uint8_t perm[512]) noexcept
    {
        const int ix0 = fastFloor(x);
        const int iy0 = fastFloor(y);

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

        const float x1 = noise_lerp(
            grad(aa, fx0, fy0),
            grad(ba, fx1, fy0),
            u);

        const float x2 = noise_lerp(
            grad(ab, fx0, fy1),
            grad(bb, fx1, fy1),
            u);

        // Normalize the chosen gradient basis to a reasonable range.
        return noise_lerp(x1, x2, v) * 0.70710678f;
    }

    // ------------------------------------------------------------
    // Periodic hash helpers for stitchTiles="stitch"
    // ------------------------------------------------------------

    static INLINE uint64_t splitmix64(uint64_t x) noexcept
    {
        x += 0x9E3779B97F4A7C15ull;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        x ^= (x >> 31);
        return x;
    }

    static INLINE uint32_t hash2_u32(int32_t x, int32_t y, uint64_t seed) noexcept
    {
        uint64_t h = seed;
        h ^= splitmix64((uint64_t)(uint32_t)x + 0x632BE59BD9B4E019ull);
        h ^= splitmix64((uint64_t)(uint32_t)y + 0x8CB92BA72F3D8DD7ull);
        h = splitmix64(h);
        return (uint32_t)(h & 0xFFFFFFFFu);
    }

    static INLINE uint32_t hash2_periodic(
        int32_t x,
        int32_t y,
        int32_t periodX,
        int32_t periodY,
        uint64_t seed) noexcept
    {
        if (periodX > 0) {
            x %= periodX;
            if (x < 0)
                x += periodX;
        }

        if (periodY > 0) {
            y %= periodY;
            if (y < 0)
                y += periodY;
        }

        return hash2_u32(x, y, seed);
    }

    static INLINE float grad2_dot_periodic(uint32_t h, float x, float y) noexcept
    {
        return grad((int)h, x, y);
    }

    // Periodic gradient noise for stitchTiles="stitch".
    // Output is roughly in [-1, 1].
    static INLINE float perlin2_periodic(
        float x,
        float y,
        int32_t periodX,
        int32_t periodY,
        uint64_t seed) noexcept
    {
        const int32_t ix0 = fastFloor(x);
        const int32_t iy0 = fastFloor(y);
        const int32_t ix1 = ix0 + 1;
        const int32_t iy1 = iy0 + 1;

        const float fx0 = x - (float)ix0;
        const float fy0 = y - (float)iy0;
        const float fx1 = fx0 - 1.0f;
        const float fy1 = fy0 - 1.0f;

        const float u = fade(fx0);
        const float v = fade(fy0);

        const uint32_t aa = hash2_periodic(ix0, iy0, periodX, periodY, seed);
        const uint32_t ab = hash2_periodic(ix0, iy1, periodX, periodY, seed);
        const uint32_t ba = hash2_periodic(ix1, iy0, periodX, periodY, seed);
        const uint32_t bb = hash2_periodic(ix1, iy1, periodX, periodY, seed);

        const float x1 = noise_lerp(
            grad2_dot_periodic(aa, fx0, fy0),
            grad2_dot_periodic(ba, fx1, fy0),
            u);

        const float x2 = noise_lerp(
            grad2_dot_periodic(ab, fx0, fy1),
            grad2_dot_periodic(bb, fx1, fy1),
            u);

        return noise_lerp(x1, x2, v) * 0.70710678f;
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
    // Non-stitched accumulation using classic Perlin noise
    // ------------------------------------------------------------

    static INLINE float fractalNoise2(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        uint64_t channelSeed) noexcept
    {
        uint8_t perm[512];
        buildPermutation(perm, (uint32_t)(channelSeed & 0xFFFFFFFFu));

        float sum = 0.0f;
        float amp = 1.0f;
        float freqX = p.baseFreqX;
        float freqY = p.baseFreqY;
        float norm = 0.0f;

        for (uint32_t o = 0; o < p.octaves; ++o)
        {
            const float n = perlin2(x * freqX, y * freqY, perm);

            sum += n * amp;
            norm += amp;

            freqX *= 2.0f;
            freqY *= 2.0f;
            amp *= 0.5f;
        }

        if (norm > 0.0f)
            sum /= norm;

        return sum;
    }

    static INLINE float turbulence2(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        uint64_t channelSeed) noexcept
    {
        uint8_t perm[512];
        buildPermutation(perm, (uint32_t)(channelSeed & 0xFFFFFFFFu));

        float sum = 0.0f;
        float amp = 1.0f;
        float freqX = p.baseFreqX;
        float freqY = p.baseFreqY;
        float norm = 0.0f;

        for (uint32_t o = 0; o < p.octaves; ++o)
        {
            float n = perlin2(x * freqX, y * freqY, perm);
            n = fabsf(n);

            sum += n * amp;
            norm += amp;

            freqX *= 2.0f;
            freqY *= 2.0f;
            amp *= 0.5f;
        }

        if (norm > 0.0f)
            sum /= norm;

        return sum;
    }

    // ------------------------------------------------------------
    // Stitched accumulation using periodic Perlin noise
    // ------------------------------------------------------------

    static INLINE float fractalNoise2_stitch(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        int32_t basePeriodX,
        int32_t basePeriodY,
        uint64_t channelSeed) noexcept
    {
        float sum = 0.0f;
        float amp = 1.0f;
        float freqX = p.baseFreqX;
        float freqY = p.baseFreqY;
        float norm = 0.0f;

        int32_t periodX = basePeriodX;
        int32_t periodY = basePeriodY;

        for (uint32_t o = 0; o < p.octaves; ++o)
        {
            const float n = perlin2_periodic(
                x * freqX,
                y * freqY,
                periodX,
                periodY,
                channelSeed);

            sum += n * amp;
            norm += amp;

            freqX *= 2.0f;
            freqY *= 2.0f;
            amp *= 0.5f;

            if (periodX > 0) periodX *= 2;
            if (periodY > 0) periodY *= 2;
        }

        if (norm > 0.0f)
            sum /= norm;

        return sum;
    }

    static INLINE float turbulence2_stitch(
        float x,
        float y,
        const TurbulenceNoiseParams& p,
        int32_t basePeriodX,
        int32_t basePeriodY,
        uint64_t channelSeed) noexcept
    {
        float sum = 0.0f;
        float amp = 1.0f;
        float freqX = p.baseFreqX;
        float freqY = p.baseFreqY;
        float norm = 0.0f;

        int32_t periodX = basePeriodX;
        int32_t periodY = basePeriodY;

        for (uint32_t o = 0; o < p.octaves; ++o)
        {
            float n = perlin2_periodic(
                x * freqX,
                y * freqY,
                periodX,
                periodY,
                channelSeed);

            n = fabsf(n);

            sum += n * amp;
            norm += amp;

            freqX *= 2.0f;
            freqY *= 2.0f;
            amp *= 0.5f;

            if (periodX > 0) periodX *= 2;
            if (periodY > 0) periodY *= 2;
        }

        if (norm > 0.0f)
            sum /= norm;

        return sum;
    }
}