#pragma once

#include "definitions.h"

#include "wggeometry.h"
#include "pixeling.h"
#include "surface.h"

//
// Things related to filtering that need to be 
// accessed from several places
// 

namespace waavs
{


    enum FilterCompositeOp : uint32_t
    {
        FILTER_COMPOSITE_OVER = 0,
        FILTER_COMPOSITE_IN,
        FILTER_COMPOSITE_OUT,
        FILTER_COMPOSITE_ATOP,
        FILTER_COMPOSITE_XOR,
        FILTER_COMPOSITE_ARITHMETIC
    };


    enum FilterColorMatrixType : uint32_t
    {
        FILTER_COLOR_MATRIX_MATRIX = 0,
        FILTER_COLOR_MATRIX_SATURATE,
        FILTER_COLOR_MATRIX_HUE_ROTATE,
        FILTER_COLOR_MATRIX_LUMINANCE_TO_ALPHA
    };

    enum FilterTransferFuncType : uint32_t
    {
        FILTER_TRANSFER_IDENTITY = 0,
        FILTER_TRANSFER_TABLE,
        FILTER_TRANSFER_DISCRETE,
        FILTER_TRANSFER_LINEAR,
        FILTER_TRANSFER_GAMMA
    };

    enum FilterMorphologyOp : uint32_t
    {
        FILTER_MORPHOLOGY_ERODE = 0,
        FILTER_MORPHOLOGY_DILATE
    };

    enum FilterEdgeMode : uint32_t
    {
        FILTER_EDGE_DUPLICATE = 0,
        FILTER_EDGE_WRAP,
        FILTER_EDGE_NONE
    };

    enum FilterChannelSelector : uint32_t
    {
        FILTER_CHANNEL_R = 0,
        FILTER_CHANNEL_G,
        FILTER_CHANNEL_B,
        FILTER_CHANNEL_A
    };

    enum FilterTurbulenceType : uint32_t
    {
        FILTER_TURBULENCE_TURBULENCE = 0,
        FILTER_TURBULENCE_FRACTAL_NOISE
    };

    enum FilterLightType : uint32_t
    {
        FILTER_LIGHT_DISTANT = 1,
        FILTER_LIGHT_POINT = 2,
        FILTER_LIGHT_SPOT = 3
    };
}

namespace waavs
{


    // For feComposite, arithmetic operator parameters k1..k4 
// often have special cases when they are zero.
    enum ArithmeticCompositeKind : uint32_t
    {
        ARITH_ZERO = 0,
        ARITH_K1_ONLY,
        ARITH_K2_ONLY,
        ARITH_K3_ONLY,
        ARITH_K4_ONLY,
        ARITH_K2_K3,
        ARITH_K1_K2_K3,
        ARITH_GENERAL
    };

    // Classify the arithmetic composite operator based on which of k1..k4 are zero.
// Then use that later to specialize the inner loop for common cases.
    static INLINE ArithmeticCompositeKind classifyArithmetic(
        float k1, float k2, float k3, float k4) noexcept
    {
        const bool z1 = k1 == 0.0f;
        const bool z2 = k2 == 0.0f;
        const bool z3 = k3 == 0.0f;
        const bool z4 = k4 == 0.0f;

        if (z1 && z2 && z3 && z4) return ARITH_ZERO;
        if (!z1 && z2 && z3 && z4) return ARITH_K1_ONLY;
        if (z1 && !z2 && z3 && z4) return ARITH_K2_ONLY;
        if (z1 && z2 && !z3 && z4) return ARITH_K3_ONLY;
        if (z1 && z2 && z3 && !z4) return ARITH_K4_ONLY;
        if (z1 && !z2 && !z3 && z4) return ARITH_K2_K3;
        if (!z1 && !z2 && !z3 && z4) return ARITH_K1_K2_K3;

        return ARITH_GENERAL;
    }
}

namespace waavs
{
    struct LightPayload { float L[8]{}; };

    struct PixelToFilterUserMap
    {
        // Tile-local surface dimensions only.
        int surfaceW{ 0 };
        int surfaceH{ 0 };

        // Local semantic filter-space extent.
        WGRectD filterExtentUS{};

        float uxPerPixel{ 1.0f };
        float uyPerPixel{ 1.0f };
    };


    static INLINE void pixelCenterToFilterUserStandalone(
        const PixelToFilterUserMap& map,
        int px, int py,
        float& ux, float& uy) noexcept
    {
        //(void)map.surfaceW;
        //(void)map.surfaceH;

        // px/py are already tile-local pixel coordinates.
        const float fx = float(px) + 0.5f;
        const float fy = float(py) + 0.5f;

        ux = float(map.filterExtentUS.x) + fx * map.uxPerPixel;
        uy = float(map.filterExtentUS.y) + fy * map.uyPerPixel;
    }

    static INLINE bool normalize3(float& x, float& y, float& z) noexcept
    {
        const float len2 = x * x + y * y + z * z;
        if (almost_zero(len2))
        {
            x = 0.0f;
            y = 0.0f;
            z = 1.0f;
            return false;
        }

        const float invLen = 1.0f / std::sqrt(len2);
        x *= invLen;
        y *= invLen;
        z *= invLen;

        return true;
    }
}

namespace waavs
{
#if defined(__ARM_NEON) || defined(__aarch64__)
    static INLINE float32x4_t neon_clamp01_f32(float32x4_t v) noexcept
    {
        return vminq_f32(vmaxq_f32(v, vdupq_n_f32(0.0f)), vdupq_n_f32(1.0f));
    }

    static INLINE float32x4_t neon_recip_nr_f32(float32x4_t x) noexcept
    {
        float32x4_t y = vrecpeq_f32(x);
        y = vmulq_f32(y, vrecpsq_f32(x, y));
        y = vmulq_f32(y, vrecpsq_f32(x, y));
        return y;
    }
#endif
}
