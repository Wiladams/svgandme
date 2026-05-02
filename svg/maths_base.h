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
    INLINE int min(const int a, const int b) noexcept { return (a < b) ? a : b; }
    INLINE int max(const int a, const int b) noexcept { return (a > b) ? a : b; }
    INLINE int clamp(const int a, const int min_, const int max_) noexcept { return min(max(a, min_), max_); }

    static INLINE int abs(int a) noexcept { return a < 0 ? -a : a; }
    static INLINE int sign(int a) noexcept { return a < 0 ? -1 : 1; }
    static INLINE int pow2(int a) noexcept { return 1 << a; }
    static INLINE void swap(int& a, int& b) noexcept { return std::swap(a, b); };

}

//=======================================
// IMPLEMENTATION for  size_t
//=======================================
namespace waavs
{

    INLINE size_t min(const size_t a, const size_t b) noexcept { return (a < b) ? a : b; }
    INLINE size_t max(const size_t a, const size_t b) noexcept { return (a > b) ? a : b; }
}

//=================================
// IMPLEMENTATION for floats
//=================================
namespace waavs
{
    INLINE float max(const float a, const float b) noexcept { return (a > b) ? a : b; }
    INLINE float min(const float a, const float b) noexcept { return (a < b) ? a : b; }
    INLINE float clamp(const float a, const float min_, const float max_) noexcept { return min(max(a, min_), max_); }
    INLINE float abs(float a) noexcept { return a < 0 ? -a : a; }
    INLINE float sign(const float a) noexcept { return a < 0 ? -1.0f : 1.0f; }





    INLINE bool isfinite(float a) { return std::isfinite(a); }
    INLINE constexpr float radiansf(float a) { return float(a * 0.017453292519943295); }
    INLINE constexpr float degreesf(float a) { return float(a * 57.29577951308232); }
    INLINE constexpr float lerp(float a, float b, float u) noexcept { return fmaf((b - a), u, a); }

    INLINE  void swap(float& a, float& b) { return std::swap(a, b); }
    INLINE float smoothStep(float a, float b, float u) {
        auto t = clamp((u - a) / (b - a), 0.0f, 1.0f);
        return t * t * (3 - 2 * t);
    }
    INLINE float bias(float a, float bias) {
        return a / ((1.0f / bias - 2) * (1.0f - a) + 1);
    }
    INLINE float gain(float a, float gain)
    {
        return (a < 0.5f) ? bias(a * 2.0f, gain) / 2.0f
            : bias(a * 2.0f - 1, 1.0f - gain) / 2.0f + 0.5f;
    }

    INLINE float map(float x, float olow, float ohigh, float rlow, float rhigh)
    {
        return rlow + (x - olow) * ((rhigh - rlow) / (ohigh - olow));
    }

    INLINE int iroundf_fast(float v) noexcept
    {
        return int(v + (v >= 0.0f ? 0.5f : -0.5f));
    }
    INLINE float floor(float a) { return std::floor(a); }
    INLINE int floorI(float x) noexcept
    {
        int i = (int)x;
        return (x < (float)i) ? (i - 1) : i;
    }
    INLINE float ceil(float a) { return std::ceil(a); }
    INLINE bool isNaN(const float a) { return std::isnan(a); }

}

//=================================
// IMPLEMENTATION for doubles
//=================================
namespace waavs
{
    INLINE double min(const double  a, const double b) noexcept { return a < b ? a : b; }
    INLINE double max(const double a, const double b) noexcept { return a > b ? a : b; }
    INLINE double clamp(const double a, const double minValue, const double maxValue) noexcept { return min(max(a, minValue), maxValue); }
    INLINE double abs(const double a) noexcept { return a < 0 ? -a : a; }

    INLINE double atan(double a) noexcept { return std::atan(a); }
    INLINE double atan2(double y, double x) noexcept { return std::atan2(y, x); }

    INLINE double radians(double a) noexcept { return (a * 0.017453292519943295); }
    INLINE double degrees(double a) noexcept { return (a * 57.29577951308232); }


    INLINE bool dbl_eq(double a, double b) noexcept {
        const double diff = std::fabs(a - b);
        const double scale = std::max({ 1.0, std::fabs(a), std::fabs(b) });
        return diff <= dbl_eps_scaled * scale;
    }

    INLINE bool almost_zero(double x) noexcept {
        const double scale = std::max(1.0, std::fabs(x));
        return std::fabs(x) <= dbl_eps * 64.0 * scale;
    }

    // radians_normalize()
    // Normalize a radian value to keep it in the range of 0 to 2pi
    static INLINE double radians_normalize(double rad) noexcept
    {
        double v = std::fmod(rad, (2.0 * kPi));
        if (v < 0.0)
        {
            return v + (2.0 * kPi);
        }
        return v;
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
    INLINE float dequantize0_255(uint8_t v) noexcept
    {
        return float(v) * kInv255f;
    }

    INLINE float dequantize0_255(uint32_t v) noexcept
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
    static INLINE uint32_t mul255_round_u8(uint32_t x, uint32_t a) noexcept
    {
        uint32_t t = x * a + 128u;
        return (t + (t >> 8)) >> 8;
    }


    static INLINE uint8_t unmul255_round_u8(uint32_t x, uint32_t divisor) noexcept
    {
        if (divisor == 0)
            return 0;

        return clamp0_255_i32((int32_t)((x * 255u + (divisor >> 1)) / divisor));
    }

    static INLINE uint32_t div0_255_rounded_u32(uint32_t x) noexcept
    {
        return (x + 128u + ((x + 128u) >> 8)) >> 8;
    }

}

