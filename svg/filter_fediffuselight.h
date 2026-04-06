#pragma once

#include "filter_util.h"

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

        // Same sign convention that now matches your corrected specular path.
        nx = -surfaceScale * dHx;
        ny = -surfaceScale * dHy;
        nz = 1.0f;
        normalize3(nx, ny, nz);
    }

    static INLINE void computeDiffuseLightVector(
        uint32_t lightType,
        const LightPayload& light,
        float ux, float uy, float h,
        float& lx, float& ly, float& lz) noexcept
    {
        const float kPi = waavs::pif;

        if (lightType == FILTER_LIGHT_DISTANT)
        {
            const float az = light.L[0] * (kPi / 180.0f);
            const float el = light.L[1] * (kPi / 180.0f);

            lx = std::cos(el) * std::cos(az);
            ly = std::cos(el) * std::sin(az);
            lz = std::sin(el);
            normalize3(lx, ly, lz);
            return;
        }

        if (lightType == FILTER_LIGHT_POINT || lightType == FILTER_LIGHT_SPOT)
        {
            lx = light.L[0] - ux;
            ly = light.L[1] - uy;
            lz = light.L[2] - h;
            normalize3(lx, ly, lz);
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
        const float kPi = waavs::pif;

        float ax = light.L[3] - light.L[0];
        float ay = light.L[4] - light.L[1];
        float az = light.L[5] - light.L[2];

        float sx = ux - light.L[0];
        float sy = uy - light.L[1];
        float sz = h - light.L[2];

        const bool axisOk = normalize3(ax, ay, az);
        const bool rayOk = normalize3(sx, sy, sz);

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
        // Match your existing behavior: fully opaque lit result.
        const float a = 1.0f;
        const float pr = clamp01f(lcR * lit);
        const float pg = clamp01f(lcG * lit);
        const float pb = clamp01f(lcB * lit);

        return pack_argb32(a, pr, pg, pb);
    }
}
