#pragma once

#include "filter_types.h"

//
// Common lighting helpers for feDiffuseLighting and feSpecularLighting.
// These are shared between the two filters to keep behavior consistent,
// and to avoid code duplication since the two filters share a lot of 
// common code for computing normals, light vectors, etc.
//
// Lighting coordinate convention:
//
// ux, uy, h are local filter-space surface coordinates.
// Point and spot light x/y and pointsAtX/Y must be localized by subtracting
// filterRectUS.x/y before calling these helpers.
//
// computeLightingVectorToLight():
//   returns normalize(lightPosition - surfacePoint)
//
// computeLightingSpotConeFactor():
//   uses:
//     spotAxis = pointsAt - lightPosition
//     spotRay  = surfacePoint - lightPosition
//
// Do not use vectorToLight for spot cone testing.

namespace waavs
{
    // pixelHeightFromAlpha()
    //
    // Interprets the alpha channel of a pixel as a height value in [0,1].
    //
    static INLINE float lightingHeightFromAlpha(const Surface& s, int x, int y) noexcept
    {
        const int W = (int)s.width();
        const int H = (int)s.height();

        if (W <= 0 || H <= 0)
            return 0.0f;

        if (x < 0) x = 0;
        else if (x >= W) x = W - 1;

        if (y < 0) y = 0;
        else if (y >= H) y = H - 1;

        const uint32_t* row = (const uint32_t*)s.rowPointer((size_t)y);
        const uint32_t px = row[x];

        return argb32_unpack_alpha_norm(px);
    }

    // ---------------------------------------------
    //
    static INLINE void computeLightingLocalPixelStep(
        const PixelToFilterUserMap& map,
        int px, 
        int py,
        float& duxOut, float& duyOut) noexcept
    {
        (void)px;
        (void)py;

        duxOut = (map.uxPerPixel > 0.0f) ? map.uxPerPixel : 1.0f;
        duyOut = (map.uyPerPixel > 0.0f) ? map.uyPerPixel : 1.0f;
    }

    // ---------------------------------------------
    //
    static INLINE void computeLightingNormalFromHeights(
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

    // ---------------------------------------------
    //
    static INLINE float computeLightingSpotConeFactor(
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



    // ---------------------------------------------
    //
    static INLINE void computeSurfaceToLightVector(
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

    /*
    static INLINE void computeSpecularLightVector(
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
    */
}
