// maths_base.h - Base math functions and definitions for the waavs library.

#pragma once


#include <cstdint>
#include <stdlib.h>
#include <cmath>
#include <limits>
#include <algorithm>
#include <utility>

#include "definitions.h"
#include "bit_hacks.h"

namespace waavs
{
    // Most Common Math constants
    constexpr auto kPi = 3.14159265358979323846;          // PI (as a double)
    constexpr auto kPif = (float)kPi;                      // PI (as a float value)
    constexpr auto PiOver2 = 1.57079632679489661923;     // PI / 2
    constexpr auto PiOver4 = 0.78539816339744830961;     // PI / 4
    constexpr auto Pi2 = 6.28318530717958647693;         // 2 * PI
    constexpr auto InvPi = 0.31830988618379067154;
    constexpr auto Inv2Pi = 0.15915494309189533577;
    constexpr auto Inv4Pi = 0.07957747154594766788;      // sqrt(2.0)
    constexpr auto DegToRad = 0.017453292519943295;      // PI / 180
    constexpr auto RadToDeg = 57.29577951308232;         // 180 / PI
    constexpr auto Sqrt2 = 1.41421356237309504880;
    
    constexpr float kInv255f = 1.0f / 255.0f;
    constexpr double kInv255 = 1.0 / 255.0;


    // Limits for various types
    constexpr int32_t int32_max = std::numeric_limits<int32_t>::max();
    constexpr int32_t int32_min = std::numeric_limits<int32_t>::min();

    constexpr auto int_max = std::numeric_limits<int>::max();
    constexpr auto int_min = std::numeric_limits<int>::min();

    constexpr auto flt_max = std::numeric_limits<float>::max();
    constexpr auto flt_min = std::numeric_limits<float>::min();
    constexpr auto flt_eps = std::numeric_limits<float>::epsilon();

    constexpr auto dbl_max = std::numeric_limits<double>::max();
    constexpr auto dbl_min = std::numeric_limits<double>::min();
    constexpr auto dbl_eps = std::numeric_limits<double>::epsilon();
    constexpr double dbl_eps_scaled = 1e-12;

}

//=======================================
// IMPLEMENTATION for  integers
//=======================================

namespace waavs
{
    static constexpr INLINE int min(const int a, const int b) noexcept { return (a < b) ? a : b; }
    static constexpr INLINE int max(const int a, const int b) noexcept { return (a > b) ? a : b; }
    static constexpr INLINE int clamp(const int a, const int min_, const int max_) noexcept { return min(max(a, min_), max_); }

    static constexpr INLINE int abs(int a) noexcept { return a < 0 ? -a : a; }
    static constexpr INLINE int sign(int a) noexcept { return a < 0 ? -1 : 1; }
    static constexpr INLINE int pow2(int a) noexcept { return 1 << a; }
    static constexpr INLINE void swap(int& a, int& b) noexcept { return std::swap(a, b); };

}

//=======================================
// IMPLEMENTATION for  size_t
//=======================================
namespace waavs
{

    static constexpr INLINE size_t min(const size_t a, const size_t b) noexcept { return (a < b) ? a : b; }
    static constexpr INLINE size_t max(const size_t a, const size_t b) noexcept { return (a > b) ? a : b; }
}

//=================================
// IMPLEMENTATION for floats
//=================================
namespace waavs
{
    static constexpr INLINE float max(const float a, const float b) noexcept { return (a > b) ? a : b; }
    static constexpr INLINE float min(const float a, const float b) noexcept { return (a < b) ? a : b; }
    static constexpr INLINE float clamp(const float a, const float min_, const float max_) noexcept { return min(max(a, min_), max_); }
    static constexpr INLINE float abs(float a) noexcept { return a < 0 ? -a : a; }
    static constexpr INLINE float sign(const float a) noexcept { return a < 0 ? -1.0f : 1.0f; }


    static constexpr INLINE bool isfinite(float a)  noexcept { return std::isfinite(a); }
    static constexpr INLINE float radiansf(float a)  noexcept { return float(a * 0.017453292519943295); }
    static constexpr INLINE float degreesf(float a)  noexcept { return float(a * 57.29577951308232); }
    static constexpr INLINE float lerp(float a, float b, float u) noexcept { return fmaf((b - a), u, a); }

    static constexpr INLINE  void swap(float& a, float& b)  noexcept { return std::swap(a, b); }
    static constexpr INLINE float smoothStep(float a, float b, float u)  noexcept {
        auto t = clamp((u - a) / (b - a), 0.0f, 1.0f);
        return t * t * (3 - 2 * t);
    }
    static constexpr INLINE float bias(float a, float bias)  noexcept {
        return a / ((1.0f / bias - 2) * (1.0f - a) + 1);
    }
    static constexpr INLINE float gain(float a, float gain) noexcept
    {
        return (a < 0.5f) ? bias(a * 2.0f, gain) / 2.0f
            : bias(a * 2.0f - 1, 1.0f - gain) / 2.0f + 0.5f;
    }

    static constexpr INLINE float map(float x, float olow, float ohigh, float rlow, float rhigh) noexcept
    {
        return rlow + (x - olow) * ((rhigh - rlow) / (ohigh - olow));
    }

    static constexpr INLINE int iroundf_fast(float v) noexcept
    {
        return int(v + (v >= 0.0f ? 0.5f : -0.5f));
    }
    static constexpr INLINE float floor(float a) noexcept { return std::floor(a); }
    static constexpr INLINE int floorI(float x) noexcept
    {
        int i = (int)x;
        return (x < (float)i) ? (i - 1) : i;
    }
    static constexpr INLINE float ceil(float a) noexcept { return std::ceil(a); }
    static constexpr INLINE bool isNaN(const float a) noexcept { return std::isnan(a); }

}

//=================================
// IMPLEMENTATION for doubles
//=================================
namespace waavs
{
    static constexpr INLINE double min(const double  a, const double b) noexcept { return a < b ? a : b; }
    static constexpr INLINE double max(const double a, const double b) noexcept { return a > b ? a : b; }
    static constexpr INLINE double clamp(const double a, const double minValue, const double maxValue) noexcept { return min(max(a, minValue), maxValue); }
    static constexpr INLINE double abs(const double a) noexcept { return a < 0 ? -a : a; }

    static constexpr INLINE double atan(double a) noexcept { return std::atan(a); }
    static constexpr INLINE double atan2(double y, double x) noexcept { return std::atan2(y, x); }

    static constexpr INLINE double degrees(double a) noexcept { return (a * 57.29577951308232); }
    static constexpr INLINE double radians(double a) noexcept { return (a * 0.017453292519943295); }
    // radians_normalize()
    // Normalize a radian value to keep it in the range of 0 to 2pi
    static INLINE double radians_normalize(double rad) noexcept
    {
        double v = std::fmod(rad, Pi2);
        if (v < 0.0)
        {
            return v + Pi2;
        }
        return v;
    }

    // dbl_eq
    // Are two double values 'close enough' to each other to call 
    // them 'equal'
    static INLINE bool dbl_eq(double a, double b) noexcept {
        const double diff = std::fabs(a - b);
        const double scale = std::max({ 1.0, std::fabs(a), std::fabs(b) });
        return diff <= dbl_eps_scaled * scale;
    }

    // almost_zero
    // Is a double value 'close enough' to zero that it
    // should just be considered to be zero?
    static INLINE bool almost_zero(double x) noexcept {
        const double scale = std::max(1.0, std::fabs(x));
        return std::fabs(x) <= dbl_eps * 64.0 * scale;
    }



    // vector_angle()
    // 
    // Calculate angle between two vectors, where the difference
    // has already been calculated
    static INLINE double vector_angle(double dx, double dy) noexcept
    {
        double rads = atan2(dy, dx);
        if (std::isnan(rads))
        {
            return 0.0;
        }
        else {
            return radians_normalize(rads);
        }
    }
}



namespace waavs
{
#if WAAVS_HAS_NEON
    static INLINE float32x4_t clamp01q_f32(float32x4_t v) noexcept
    {
        return vminq_f32(vmaxq_f32(v, vdupq_n_f32(0.0f)), vdupq_n_f32(1.0f));
    }
#endif

    static INLINE float clamp01f(const float v) noexcept { return clamp(v, 0.0f, 1.0f); }
    static INLINE double clamp01(double v) noexcept { return clamp(v, 0.0, 1.0); }


    static INLINE uint8_t clamp_u8(const int64_t v) noexcept = delete;
    static INLINE uint8_t clamp0_255_i32(const int32_t v) noexcept
    {
        if (v < 0)   return 0;
        if (v > 255) return 255;
        return (uint8_t)v;
    }

    static INLINE uint8_t clamp0_255_i64(const int64_t v) noexcept
    {
        if (v < 0)   return 0;
        if (v > 255) return 255;
        return (uint8_t)v;
    }

    // -------------------------------------------
    // Quantize and Dequantize
    
#if WAAVS_HAS_NEON
    static INLINE uint8x8_t quantize0_255q_u8(float32x4_t lo, float32x4_t hi) noexcept
    {
        const float32x4_t scale = vdupq_n_f32(255.0f);
        const float32x4_t half = vdupq_n_f32(0.5f);

        lo = vaddq_f32(vmulq_f32(clamp01q_f32(lo), scale), half);
        hi = vaddq_f32(vmulq_f32(clamp01q_f32(hi), scale), half);

        const uint32x4_t ulo = vcvtq_u32_f32(lo);
        const uint32x4_t uhi = vcvtq_u32_f32(hi);

        const uint16x4_t nlo = vmovn_u32(ulo);
        const uint16x4_t nhi = vmovn_u32(uhi);
        const uint16x8_t u16 = vcombine_u16(nlo, nhi);

        return vmovn_u16(u16);
    }
#endif

    // take value in range [0,1] and convert to uint8_t in range [0,255]
    static INLINE uint8_t quantize0_255(float v) noexcept
    {
        v = clamp01f(v);
        return (uint8_t)(v * 255.0f + 0.5f);
    }

    static INLINE uint8_t quantize0_255(double v) noexcept
    {
        v = clamp01(v);
        return (uint8_t)(v * 255.0 + 0.5);
    }

    // Turn a value into a normalized [0..1] float
    static INLINE float dequantize0_255(uint8_t v) noexcept
    {
        return float(v) * kInv255f;
    }

    static INLINE float dequantize0_255(uint32_t v) noexcept
    {
        return dequantize0_255(uint8_t(v & 0xFFu));
    }

    // saturated round of double to int32_t
    static INLINE int32_t saturate_round_i32(double v) noexcept
    {
        if (v >= double(int32_max))
            return int32_max;
        if (v <= double(int32_min))
            return int32_min;

        return (int32_t)std::llround(v);
    }

    // a fast multiply of two 8-bit values, 
    // returning an 8-bit result, with rounding
    static INLINE uint32_t mul255_round_u8(const uint32_t x, const uint32_t a) noexcept
    {
        uint32_t t = x * a + 128u;
        return (t + (t >> 8)) >> 8;
    }


    static INLINE uint8_t unmul255_round_u8(const uint32_t x, const uint32_t divisor) noexcept
    {
        if (divisor == 0)
            return 0;

        return clamp0_255_i32((int32_t)((x * 255u + (divisor >> 1)) / divisor));
    }

    static INLINE uint32_t div0_255_rounded_u32(const uint32_t x) noexcept
    {
        return (x + 128u + ((x + 128u) >> 8)) >> 8;
    }

}

