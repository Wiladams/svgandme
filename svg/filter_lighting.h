#pragma once

#include "filter_types.h"
#include "surface_info.h"

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
    static INLINE float lightingHeightFromAlpha(const Surface_ARGB32& s, int x, int y) noexcept
    {
        const int W = (int)s.width;
        const int H = (int)s.height;

        if (W <= 0 || H <= 0)
            return 0.0f;

        if (x < 0) x = 0;
        else if (x >= W) x = W - 1;

        if (y < 0) y = 0;
        else if (y >= H) y = H - 1;

        const uint32_t* row = Surface_ARGB32_row_pointer_const(&s, (size_t)y);
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
        constexpr float ksobelFactor = 4.0f; // 8.0f
        float dHx = 0.0f;
        float dHy = 0.0f;

        if (dux > 0.0f)
        {
            dHx =
                ((h20 + 2.0f * h21 + h22) -
                    (h00 + 2.0f * h01 + h02)) / (ksobelFactor * dux);
        }

        if (duy > 0.0f)
        {
            dHy =
                ((h02 + 2.0f * h12 + h22) -
                    (h00 + 2.0f * h10 + h20)) / (ksobelFactor * duy);
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

    static INLINE LightPayload localizeLightingPayload(
        const LightPayload& light,
        uint32_t lightType,
        const WGRectD& filterRectUS) noexcept
    {
        LightPayload localLight = light;

        if (lightType == FILTER_LIGHT_POINT || lightType == FILTER_LIGHT_SPOT)
        {
            localLight.L[0] -= float(filterRectUS.x);
            localLight.L[1] -= float(filterRectUS.y);
        }

        if (lightType == FILTER_LIGHT_SPOT)
        {
            localLight.L[3] -= float(filterRectUS.x);
            localLight.L[4] -= float(filterRectUS.y);
        }

        return localLight;
    }

    static INLINE PixelToFilterUserMap makeLightingPixelMap(const Surface_ARGB32& src,
        const WGRectD& filterRectUS) noexcept
    {
        PixelToFilterUserMap map;
        map.surfaceW = int(src.width);
        map.surfaceH = int(src.height);

        map.filterExtentUS = WGRectD{
            0.0,
            0.0,
            filterRectUS.w,
            filterRectUS.h
        };

        map.uxPerPixel = (map.surfaceW > 0)
            ? float(map.filterExtentUS.w / double(map.surfaceW))
            : 1.0f;

        map.uyPerPixel = (map.surfaceH > 0)
            ? float(map.filterExtentUS.h / double(map.surfaceH))
            : 1.0f;

        return map;
    }


    static  INLINE void resolveColorInterpolationRGB(
        const ColorSRGB& in,
        FilterColorInterpolation interp,
        float& r,
        float& g,
        float& b) noexcept
    {
        if (interp == FILTER_COLOR_INTERPOLATION_LINEAR_RGB)
        {
            const ColorLinear lin = coloring_srgb_to_linear(in);
            r = clamp01f(lin.r);
            g = clamp01f(lin.g);
            b = clamp01f(lin.b);
        }
        else if (interp == FILTER_COLOR_INTERPOLATION_SRGB)
        {
            r = clamp01f(in.r);
            g = clamp01f(in.g);
            b = clamp01f(in.b);
        }
    }

    // ---------------------------------------------
    //
    static INLINE void resolveLightingKernelStep(
        const PixelToFilterUserMap& map,
        const WGRectI& area,
        float kernelUnitLengthX,
        float kernelUnitLengthY,
        float& dux,
        float& duy) noexcept
    {
        float defaultDux = 1.0f;
        float defaultDuy = 1.0f;

        computeLightingLocalPixelStep(
            map,
            area.x,
            area.y,
            defaultDux,
            defaultDuy);

        dux = (kernelUnitLengthX > 0.0f) ? kernelUnitLengthX : defaultDux;
        duy = (kernelUnitLengthY > 0.0f) ? kernelUnitLengthY : defaultDuy;
    }


    // ---------------------------------------------
    // sampleLightingHeight3x3
    // Calculate a lighting height sample for a 3x3 pixel neighborhood, 
    // with clamping at edges.
    //
    struct LightingHeight3x3
    {
        float h00, h10, h20;
        float h01, h11, h21;
        float h02, h12, h22;
    };

    static INLINE LightingHeight3x3 sampleLightingHeight3x3(const Surface_ARGB32& src,
        int x, int y) noexcept
    {
        LightingHeight3x3 h;

        // Gather the pixels in a 3x3 neighborhood around (x,y), 
        // with clamping at edges, and convert to heights.
        // 
        // first row
        h.h00 = lightingHeightFromAlpha(src, x - 1, y - 1);
        h.h10 = lightingHeightFromAlpha(src, x, y - 1);
        h.h20 = lightingHeightFromAlpha(src, x + 1, y - 1);

        // middle row
        h.h01 = lightingHeightFromAlpha(src, x - 1, y);
        h.h11 = lightingHeightFromAlpha(src, x, y);
        h.h21 = lightingHeightFromAlpha(src, x + 1, y);

        // bottom row
        h.h02 = lightingHeightFromAlpha(src, x - 1, y + 1);
        h.h12 = lightingHeightFromAlpha(src, x, y + 1);
        h.h22 = lightingHeightFromAlpha(src, x + 1, y + 1);

        return h;
    }

    // ---------------------------------------------
    // Pixel packing helpers for diffuse and specular lighting results.
    //
    static INLINE Pixel_ARGB32 packLightingPixel_srgb_lut(
        float r, float g, float b,
        float a,
        const ColorCodecLUT& lut) noexcept
    {
        return ColorSRGB_to_Pixel_ARGB32_lut(r, g, b, a, lut);
    }

    static INLINE Pixel_ARGB32 packLightingPixel_linear_lut(
        float r, float g, float b,
        float a,
        const ColorCodecLUT& lut) noexcept
    {
        return ColorLinear_to_Pixel_ARGB32_lut(r, g, b, a, lut);
    }

    static INLINE Pixel_ARGB32 packLightingPixel_lut(
        float r, float g, float b,
        float a,
        FilterColorInterpolation interp,
        const ColorCodecLUT& lut) noexcept
    {
        if (interp == FILTER_COLOR_INTERPOLATION_LINEAR_RGB)
            return packLightingPixel_linear_lut(r, g, b, a, lut);

        return packLightingPixel_srgb_lut(r, g, b, a, lut);
    }

}
