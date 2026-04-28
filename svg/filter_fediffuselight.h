// filter_fediffuselight.h

#pragma once

#include "filter_types.h"

namespace waavs
{
    // =============================================
    // feDiffuseLighting helpers
    // =============================================

    static INLINE void computeDiffuseNormalFromHeights(
        float h00, float h10, float h20,
        float h01, float h11, float h21,
        float h02, float h12, float h22,
        float surfaceScale,
        float dux, float duy,
        float& nx, float& ny, float& nz) noexcept
    {
        float dHx = 0.0f;
        float dHy = 0.0f;

        if (dux > 0.0f)
        {
            dHx =
                ((h20 + 2.0f * h21 + h22) -
                    (h00 + 2.0f * h01 + h02)) / (8.0f * dux);
        }

        if (duy > 0.0f)
        {
            dHy =
                ((h02 + 2.0f * h12 + h22) -
                    (h00 + 2.0f * h10 + h20)) / (8.0f * duy);
        }

        nx = -surfaceScale * dHx;
        ny = -surfaceScale * dHy;
        nz = 1.0f;
        vec3_normalize(nx, ny, nz);
    }

    static INLINE void computeDiffuseLightVector(
        uint32_t lightType,
        const LightPayload& light,
        float ux, float uy, float h,
        float& lx, float& ly, float& lz) noexcept
    {
        const float kPi = kPif;

        if (lightType == FILTER_LIGHT_DISTANT)
        {
            const float az = light.L[0] * (kPi / 180.0f);
            const float el = light.L[1] * (kPi / 180.0f);

            lx = std::cos(el) * std::cos(az);
            ly = std::cos(el) * std::sin(az);
            lz = std::sin(el);
            vec3_normalize(lx, ly, lz);
            return;
        }

        if (lightType == FILTER_LIGHT_POINT || lightType == FILTER_LIGHT_SPOT)
        {
            lx = light.L[0] - ux;
            ly = light.L[1] - uy;
            lz = light.L[2] - h;
            vec3_normalize(lx, ly, lz);
            return;
        }

        lx = 0.0f;
        ly = 0.0f;
        lz = 1.0f;
    }

    static INLINE float computeDiffuseSpotFactor(
        const LightPayload& light,
        float ux, float uy, float h) noexcept
    {
        const float kPi = kPif;

        float ax = light.L[3] - light.L[0];
        float ay = light.L[4] - light.L[1];
        float az = light.L[5] - light.L[2];

        float sx = ux - light.L[0];
        float sy = uy - light.L[1];
        float sz = h - light.L[2];

        const bool axisOk = vec3_normalize(ax, ay, az);
        const bool rayOk = vec3_normalize(sx, sy, sz);

        if (!axisOk || !rayOk)
            return 0.0f;

        float cosAng = ax * sx + ay * sy + az * sz;

        const float limitDeg = light.L[7];
        if (limitDeg > 0.0f)
        {
            const float limitCos = std::cos(limitDeg * (kPi / 180.0f));
            if (cosAng < limitCos)
                return 0.0f;
        }

        cosAng = clamp01f(cosAng);

        float spotExp = light.L[6];
        if (!(spotExp > 0.0f))
            spotExp = 1.0f;

        return std::pow(cosAng, spotExp);
    }

    static INLINE float computeDiffuseTerm(
        float nx, float ny, float nz,
        float lx, float ly, float lz,
        float diffuseConstant,
        float lightFactor) noexcept
    {
        float ndotl = nx * lx + ny * ly + nz * lz;
        if (ndotl <= 0.0f)
            return 0.0f;

        return clamp01f(diffuseConstant * ndotl * lightFactor);
    }

    static INLINE uint32_t packDiffuseLightingPixel(
        float lcR, float lcG, float lcB,
        float lit) noexcept
    {
        // For diffuse lighting, we want a fully opaque
        // pixel where the RGB channels represent the lit color, 
        // so we can composite

        // Diffuse light is supposed to be fully opaque
        // unlike specular which can be composited additively,
        // so we set alpha to 1.0.
        const float pr = clamp01f(lcR * lit);
        const float pg = clamp01f(lcG * lit);
        const float pb = clamp01f(lcB * lit);
        const float a = 1.0;    // max(pr, max(pg, pb));

        return argb32_pack_u8(
            quantize0_255(a),
            quantize0_255(pr),
            quantize0_255(pg),
            quantize0_255(pb));
    }

    // --------------------------------------------
    struct DiffuseLightingRowParams
    {
        float surfaceScale{ 1.0f };
        float diffuseConstant{ 1.0f };

        float lcR{ 1.0f };
        float lcG{ 1.0f };
        float lcB{ 1.0f };

        float dux{ 1.0f };
        float duy{ 1.0f };

        float uxPerPixel{ 1.0f };
        float uyPerPixel{ 1.0f };

        float rowUy{ 0.0f };

        uint32_t lightType{ FILTER_LIGHT_DISTANT };
        LightPayload localLight{};
    };



    static INLINE void diffuseLighting_row_scalar(
        uint32_t* dst,
        const uint32_t* row0,
        const uint32_t* row1,
        const uint32_t* row2,
        int x0,
        int count,
        int surfaceW,
        const DiffuseLightingRowParams& p) noexcept
    {
        for (int i = 0; i < count; ++i)
        {
            const int x = x0 + i;

            const int xm1 = clamp(x - 1, 0, surfaceW - 1);
            const int xp0 = clamp(x, 0, surfaceW - 1);
            const int xp1 = clamp(x + 1, 0, surfaceW - 1);

            const float h00 = argb32_unpack_alpha_norm(row0[xm1]);
            const float h10 = argb32_unpack_alpha_norm(row0[xp0]);
            const float h20 = argb32_unpack_alpha_norm(row0[xp1]);

            const float h01 = argb32_unpack_alpha_norm(row1[xm1]);
            const float h11 = argb32_unpack_alpha_norm(row1[xp0]);
            const float h21 = argb32_unpack_alpha_norm(row1[xp1]);

            const float h02 = argb32_unpack_alpha_norm(row2[xm1]);
            const float h12 = argb32_unpack_alpha_norm(row2[xp0]);
            const float h22 = argb32_unpack_alpha_norm(row2[xp1]);

            float nx, ny, nz;
            computeDiffuseNormalFromHeights(
                h00, h10, h20,
                h01, h11, h21,
                h02, h12, h22,
                p.surfaceScale,
                p.dux, p.duy,
                nx, ny, nz);

            const float ux = (float(x) + 0.5f) * p.uxPerPixel;
            const float uy = p.rowUy;

            const float h = p.surfaceScale * h11;

            float lx, ly, lz;
            computeDiffuseLightVector(p.lightType, p.localLight, ux, uy, h, lx, ly, lz);

            float lightFactor = 1.0f;
            if (p.lightType == FILTER_LIGHT_SPOT)
                lightFactor = computeDiffuseSpotFactor(p.localLight, ux, uy, h);

            const float lit = computeDiffuseTerm(
                nx, ny, nz,
                lx, ly, lz,
                p.diffuseConstant,
                lightFactor);

            dst[i] = packDiffuseLightingPixel(p.lcR, p.lcG, p.lcB, lit);
        }
    }

    //--------------------------------------------
    // NEON helpers for feDiffuseLighting.
    //---------------------------------------------

#if defined(__ARM_NEON) || defined(__aarch64__)

    //static INLINE float32x4_t neon_clamp01_f32(float32x4_t v) noexcept
    //{
    //    return vminq_f32(vmaxq_f32(v, vdupq_n_f32(0.0f)), vdupq_n_f32(1.0f));
    //}

    static INLINE float32x4_t neon_rsqrt_nr_f32(float32x4_t x) noexcept
    {
        float32x4_t y = vrsqrteq_f32(x);
        y = vmulq_f32(y, vrsqrtsq_f32(vmulq_f32(x, y), y));
        y = vmulq_f32(y, vrsqrtsq_f32(vmulq_f32(x, y), y));
        return y;
    }



    static INLINE float32x4_t neon_alpha_height4(
        const uint32_t* row,
        int x0, int x1, int x2, int x3) noexcept
    {
        const float h0 = dequantize0_255((row[x0] >> 24) & 0xFFu);
        const float h1 = dequantize0_255((row[x1] >> 24) & 0xFFu);
        const float h2 = dequantize0_255((row[x2] >> 24) & 0xFFu);
        const float h3 = dequantize0_255((row[x3] >> 24) & 0xFFu);

        float32x4_t v = vdupq_n_f32(0.0f);
        v = vsetq_lane_f32(h0, v, 0);
        v = vsetq_lane_f32(h1, v, 1);
        v = vsetq_lane_f32(h2, v, 2);
        v = vsetq_lane_f32(h3, v, 3);
        return v;
    }
#endif

#if defined(__ARM_NEON) || defined(__aarch64__)
    static INLINE void diffuseLighting_row_neon(
        uint32_t* dst,
        const uint32_t* row0,
        const uint32_t* row1,
        const uint32_t* row2,
        int x0,
        int count,
        int surfaceW,
        const DiffuseLightingRowParams& p) noexcept
    {
        if (count <= 0 || surfaceW <= 0)
            return;

        // First real NEON implementation:
        // - FILTER_LIGHT_DISTANT: vectorized
        // - FILTER_LIGHT_POINT:   vectorized
        // - FILTER_LIGHT_SPOT:    scalar fallback for now
        if (p.lightType == FILTER_LIGHT_SPOT)
        {
            diffuseLighting_row_scalar(dst, row0, row1, row2, x0, count, surfaceW, p);
            return;
        }

        const float32x4_t kZero = vdupq_n_f32(0.0f);
        const float32x4_t kOne = vdupq_n_f32(1.0f);
        const float32x4_t kTwo = vdupq_n_f32(2.0f);
        const float32x4_t kEight = vdupq_n_f32(8.0f);
        const float32x4_t kHalf = vdupq_n_f32(0.5f);
        const float32x4_t kSurfaceScale = vdupq_n_f32(p.surfaceScale);
        const float32x4_t kDiffuseConstant = vdupq_n_f32(p.diffuseConstant);
        const float32x4_t kUxPerPixel = vdupq_n_f32(p.uxPerPixel);
        const float32x4_t kUy = vdupq_n_f32(p.rowUy);
        const float32x4_t kLcR = vdupq_n_f32(p.lcR);
        const float32x4_t kLcG = vdupq_n_f32(p.lcG);
        const float32x4_t kLcB = vdupq_n_f32(p.lcB);

        const float inv8dux = (p.dux > 0.0f) ? (1.0f / (8.0f * p.dux)) : 0.0f;
        const float inv8duy = (p.duy > 0.0f) ? (1.0f / (8.0f * p.duy)) : 0.0f;
        const float32x4_t kInv8Dux = vdupq_n_f32(inv8dux);
        const float32x4_t kInv8Duy = vdupq_n_f32(inv8duy);

        float32x4_t kLx = kZero;
        float32x4_t kLy = kZero;
        float32x4_t kLz = kOne;

        if (p.lightType == FILTER_LIGHT_DISTANT)
        {
            float lx = 0.0f;
            float ly = 0.0f;
            float lz = 1.0f;
            computeDiffuseLightVector(FILTER_LIGHT_DISTANT, p.localLight, 0.0f, 0.0f, 0.0f, lx, ly, lz);

            kLx = vdupq_n_f32(lx);
            kLy = vdupq_n_f32(ly);
            kLz = vdupq_n_f32(lz);
        }

        int i = 0;
        for (; i + 4 <= count; i += 4)
        {
            const int xA = x0 + i + 0;
            const int xB = x0 + i + 1;
            const int xC = x0 + i + 2;
            const int xD = x0 + i + 3;

            const int xm1A = clamp(xA - 1, 0, surfaceW - 1);
            const int xm1B = clamp(xB - 1, 0, surfaceW - 1);
            const int xm1C = clamp(xC - 1, 0, surfaceW - 1);
            const int xm1D = clamp(xD - 1, 0, surfaceW - 1);

            const int xp0A = clamp(xA, 0, surfaceW - 1);
            const int xp0B = clamp(xB, 0, surfaceW - 1);
            const int xp0C = clamp(xC, 0, surfaceW - 1);
            const int xp0D = clamp(xD, 0, surfaceW - 1);

            const int xp1A = clamp(xA + 1, 0, surfaceW - 1);
            const int xp1B = clamp(xB + 1, 0, surfaceW - 1);
            const int xp1C = clamp(xC + 1, 0, surfaceW - 1);
            const int xp1D = clamp(xD + 1, 0, surfaceW - 1);

            const float32x4_t h00 = neon_alpha_height4(row0, xm1A, xm1B, xm1C, xm1D);
            const float32x4_t h10 = neon_alpha_height4(row0, xp0A, xp0B, xp0C, xp0D);
            const float32x4_t h20 = neon_alpha_height4(row0, xp1A, xp1B, xp1C, xp1D);

            const float32x4_t h01 = neon_alpha_height4(row1, xm1A, xm1B, xm1C, xm1D);
            const float32x4_t h11 = neon_alpha_height4(row1, xp0A, xp0B, xp0C, xp0D);
            const float32x4_t h21 = neon_alpha_height4(row1, xp1A, xp1B, xp1C, xp1D);

            const float32x4_t h02 = neon_alpha_height4(row2, xm1A, xm1B, xm1C, xm1D);
            const float32x4_t h12 = neon_alpha_height4(row2, xp0A, xp0B, xp0C, xp0D);
            const float32x4_t h22 = neon_alpha_height4(row2, xp1A, xp1B, xp1C, xp1D);

            // Sobel-style height gradient.
            const float32x4_t dHx_num =
                vsubq_f32(
                    vaddq_f32(vaddq_f32(h20, vmulq_f32(kTwo, h21)), h22),
                    vaddq_f32(vaddq_f32(h00, vmulq_f32(kTwo, h01)), h02));

            const float32x4_t dHy_num =
                vsubq_f32(
                    vaddq_f32(vaddq_f32(h02, vmulq_f32(kTwo, h12)), h22),
                    vaddq_f32(vaddq_f32(h00, vmulq_f32(kTwo, h10)), h20));

            const float32x4_t dHx = vmulq_f32(dHx_num, kInv8Dux);
            const float32x4_t dHy = vmulq_f32(dHy_num, kInv8Duy);

            // Normal: N = normalize(-surfaceScale*dHdx, -surfaceScale*dHdy, 1)
            float32x4_t nx = vnegq_f32(vmulq_f32(kSurfaceScale, dHx));
            float32x4_t ny = vnegq_f32(vmulq_f32(kSurfaceScale, dHy));
            float32x4_t nz = kOne;

            const float32x4_t nLen2 =
                vaddq_f32(vaddq_f32(vmulq_f32(nx, nx), vmulq_f32(ny, ny)), vmulq_f32(nz, nz));
            const float32x4_t nInvLen = neon_rsqrt_nr_f32(vmaxq_f32(nLen2, vdupq_n_f32(1.0e-20f)));

            nx = vmulq_f32(nx, nInvLen);
            ny = vmulq_f32(ny, nInvLen);
            nz = vmulq_f32(nz, nInvLen);

            float32x4_t lx = kLx;
            float32x4_t ly = kLy;
            float32x4_t lz = kLz;

            if (p.lightType == FILTER_LIGHT_POINT)
            {
                float32x4_t vx = vdupq_n_f32(0.0f);
                vx = vsetq_lane_f32(float(xA), vx, 0);
                vx = vsetq_lane_f32(float(xB), vx, 1);
                vx = vsetq_lane_f32(float(xC), vx, 2);
                vx = vsetq_lane_f32(float(xD), vx, 3);

                const float32x4_t ux = vmulq_f32(vaddq_f32(vx, kHalf), kUxPerPixel);
                const float32x4_t uy = kUy;
                const float32x4_t h = vmulq_f32(kSurfaceScale, h11);

                lx = vsubq_f32(vdupq_n_f32(p.localLight.L[0]), ux);
                ly = vsubq_f32(vdupq_n_f32(p.localLight.L[1]), uy);
                lz = vsubq_f32(vdupq_n_f32(p.localLight.L[2]), h);

                const float32x4_t lLen2 =
                    vaddq_f32(vaddq_f32(vmulq_f32(lx, lx), vmulq_f32(ly, ly)), vmulq_f32(lz, lz));
                const float32x4_t lInvLen = neon_rsqrt_nr_f32(vmaxq_f32(lLen2, vdupq_n_f32(1.0e-20f)));

                lx = vmulq_f32(lx, lInvLen);
                ly = vmulq_f32(ly, lInvLen);
                lz = vmulq_f32(lz, lInvLen);
            }

            float32x4_t ndotl =
                vaddq_f32(vaddq_f32(vmulq_f32(nx, lx), vmulq_f32(ny, ly)), vmulq_f32(nz, lz));
            ndotl = vmaxq_f32(ndotl, kZero);

            float32x4_t lit = vmulq_f32(kDiffuseConstant, ndotl);
            lit = neon_clamp01_f32(lit);

            float32x4_t pr = vmulq_f32(kLcR, lit);
            float32x4_t pg = vmulq_f32(kLcG, lit);
            float32x4_t pb = vmulq_f32(kLcB, lit);

            pr = neon_clamp01_f32(pr);
            pg = neon_clamp01_f32(pg);
            pb = neon_clamp01_f32(pb);

            float litArr[4];
            vst1q_f32(litArr, lit);

            // Pack with the existing scalar packer to keep behavior identical.
            dst[i + 0] = packDiffuseLightingPixel(p.lcR, p.lcG, p.lcB, litArr[0]);
            dst[i + 1] = packDiffuseLightingPixel(p.lcR, p.lcG, p.lcB, litArr[1]);
            dst[i + 2] = packDiffuseLightingPixel(p.lcR, p.lcG, p.lcB, litArr[2]);
            dst[i + 3] = packDiffuseLightingPixel(p.lcR, p.lcG, p.lcB, litArr[3]);

            //dst[xA] = packDiffuseLightingPixel(p.lcR, p.lcG, p.lcB, litArr[0]);
            //dst[xB] = packDiffuseLightingPixel(p.lcR, p.lcG, p.lcB, litArr[1]);
            //dst[xC] = packDiffuseLightingPixel(p.lcR, p.lcG, p.lcB, litArr[2]);
            //dst[xD] = packDiffuseLightingPixel(p.lcR, p.lcG, p.lcB, litArr[3]);
        }

        if (i < count)
            diffuseLighting_row_scalar(dst+i, row0, row1, row2, x0 + i, count - i, surfaceW, p);
    }
#endif



    static INLINE void diffuseLighting_row(
        uint32_t* dst,
        const uint32_t* row0,
        const uint32_t* row1,
        const uint32_t* row2,
        int x0,
        int count,
        int surfaceW,
        const DiffuseLightingRowParams& p) noexcept
    {
#if defined(__ARM_NEON) || defined(__aarch64__)
        diffuseLighting_row_neon(dst, row0, row1, row2, x0, count, surfaceW, p);
#else
        diffuseLighting_row_scalar(dst, row0, row1, row2, x0, count, surfaceW, p);
#endif
    }

}
