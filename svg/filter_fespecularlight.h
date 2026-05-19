// filter_fespecularlight.h

#pragma once


#include "filter_types.h"
#include "filter_lighting.h"

namespace waavs
{

    // =============================================
    // feSpecularLighting helpers
    // =============================================

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
    static INLINE Pixel_ARGB32 packSpecularLightingPixel_lut(
        float lcR, float lcG, float lcB,
        float lit,
        FilterColorInterpolation interp,
        const ColorCodecLUT& lut) noexcept
    {
        const float r = clamp01f(lcR * lit);
        const float g = clamp01f(lcG * lit);
        const float b = clamp01f(lcB * lit);
        const float a = max(r, max(g, b));

        return packLightingPixel_lut(
            r, g, b, a,
            interp,
            lut);
    }

    static INLINE void specularLighting_row_lut(
        Pixel_ARGB32* dst,
        const Surface_ARGB32& src,
        int y,
        int x0,
        int x1,
        const PixelToFilterUserMap& map,
        const LightPayload& light,
        uint32_t lightType,
        float lcR,
        float lcG,
        float lcB,
        float surfaceScale,
        float specularConstant,
        float specularExponent,
        float dux,
        float duy,
        FilterColorInterpolation interp,
        const ColorCodecLUT& lut) noexcept
    {
        for (int x = x0; x < x1; ++x)
        {
            const LightingHeight3x3 h = sampleLightingHeight3x3(src, x, y);

            float nx, ny, nz;
            computeLightingNormalFromHeights(
                h.h00, h.h10, h.h20,
                h.h01, h.h11, h.h21,
                h.h02, h.h12, h.h22,
                surfaceScale,
                dux, duy,
                nx, ny, nz);

            float ux, uy;
            pixelCenterToFilterUserStandalone(map, x, y, ux, uy);

            const float surfaceZ = surfaceScale * h.h11;

            float lx, ly, lz;
            computeSurfaceToLightVector(
                lightType,
                light,
                ux, uy, surfaceZ,
                lx, ly, lz);

            float lightFactor = 1.0f;
            if (lightType == FILTER_LIGHT_SPOT)
                lightFactor = computeLightingSpotConeFactor(light, ux, uy, surfaceZ);

            const float lit = computeSpecularTerm(
                nx, ny, nz,
                lx, ly, lz,
                specularConstant,
                specularExponent,
                lightFactor);

            dst[x] = packSpecularLightingPixel_lut(
                lcR, lcG, lcB,
                lit,
                interp,
                lut);
        }
    }
}
