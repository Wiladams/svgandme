// filter_fespecularlight.h

#pragma once


#include "filter_types.h"

namespace waavs
{

    // =============================================
    // feSpecularLighting helpers
    // =============================================



    static INLINE void computeSpecularLocalPixelStep(
        const PixelToFilterUserMap& map,
        int /*px*/, int /*py*/,
        float& duxOut, float& duyOut) noexcept
    {
        duxOut = (map.uxPerPixel > 0.0f) ? map.uxPerPixel : 1.0f;
        duyOut = (map.uyPerPixel > 0.0f) ? map.uyPerPixel : 1.0f;
    }



    static INLINE void computeSpecularNormalFromHeights(
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

    static INLINE void computeSpecularLightVector(
        uint32_t lightType,
        const LightPayload& light,
        float ux, float uy, float h,
        float& lx, float& ly, float& lz) noexcept
    {
        const float kPi = waavs::kPif;

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

    static INLINE float computeSpecularSpotFactor(const LightPayload& light,
        float ux, float uy, float h) noexcept
    {
        const float kPi = waavs::kPif;

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

    static INLINE float computeSpecularTerm(
        float nx, float ny, float nz,
        float lx, float ly, float lz,
        float specularConstant,
        float specularExponent,
        float lightFactor) noexcept
    {
        const float ex = 0.0f;
        const float ey = 0.0f;
        const float ez = 1.0f;

        float hx = lx + ex;
        float hy = ly + ey;
        float hz = lz + ez;

        if (!vec3_normalize(hx, hy, hz))
            return 0.0f;

        float ndoth = nx * hx + ny * hy + nz * hz;
        if (ndoth <= 0.0f)
            return 0.0f;

        const float lit = specularConstant * std::pow(ndoth, specularExponent) * lightFactor;
        return clamp01f(lit);
    }


    // Pack the specular lighting result into an ARGB32 pixel, 
    // where the RGB components are the lit color and the alpha 
    // is the maximum of the RGB components (for later compositing).
    static INLINE uint32_t packSpecularLightingPixel(
        float lcR, float lcG, float lcB,
        float lit) noexcept
    {
        const float pr = clamp01f(lcR * lit);
        const float pg = clamp01f(lcG * lit);
        const float pb = clamp01f(lcB * lit);

        float a = pr;
        if (pg > a) a = pg;
        if (pb > a) a = pb;

        return argb32_pack_u8(
            quantize0_255(a),
            quantize0_255(pr),
            quantize0_255(pg),
            quantize0_255(pb));
    }


}
