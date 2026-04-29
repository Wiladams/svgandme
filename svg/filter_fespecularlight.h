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
    static INLINE uint32_t packSpecularLightingPixel(
        float lcR, float lcG, float lcB,
        float lit) noexcept
    {
        const float pr = clamp01f(lcR * lit);
        const float pg = clamp01f(lcG * lit);
        const float pb = clamp01f(lcB * lit);

        float a = max(pr, max(pg, pb)); // pr;

        return argb32_pack_u8(
            quantize0_255(a),
            quantize0_255(pr),
            quantize0_255(pg),
            quantize0_255(pb));
    }


}
