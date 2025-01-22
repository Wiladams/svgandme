#pragma once

/*
    Math routines are always very useful not only for graphics, but for
    machine learning and other domains.

    Herein lies some typical math routines to be used while fiddling about
    with graphics.  The routines here are meant to be simple and representative
    of typical usage.  If you're using an API such as p5, these routines
    will have convenient wrappers.

    Aside from the typical trig functions, there are also a few key data
    structures, such as vectors, matrices, and quaternions.  They are
    here so you don't have to invent those from scratch for each new
    program.

    Some specialization for 3D modeling is inside this file, due to convenience
    but mostly outside.

    The routines in here might not be the fastest, but they're fairly compact, 
    and should not be embarrassing.

    References
    https://github.com/nfrechette/rtm
    https://github.com/HandmadeMath/Handmade-Math/blob/master/HandmadeMath.h
    yocto-gl

    And many others

    The routines here assume columnar vectors (like OPenGL), so the 
    multiplications go one way.
*/


#include <cstdint>
#include <stdlib.h>
#include <cmath>
#include <limits>
#include <algorithm>
#include <utility>

#include "bithacks.h"


//=================================
// CONSTANT Declarations
//=================================
namespace {
    using std::pair;
}

namespace waavs {
    // Some useful Math constants
    constexpr auto pi = 3.14159265358979323846;          // PI (as a double)
    constexpr auto pif = (float)pi;                      // PI (as a float value)
    constexpr auto PiOver2 = 1.57079632679489661923;     // PI / 2
    constexpr auto PiOver4 = 0.78539816339744830961;     // PI / 4
    constexpr auto Pi2 = 6.28318530717958647693;         // 2 * PI
    constexpr auto InvPi = 0.31830988618379067154;
    constexpr auto Inv2Pi = 0.15915494309189533577;
    constexpr auto Inv4Pi = 0.07957747154594766788;      // sqrt(2.0)
	constexpr auto DegToRad = 0.017453292519943295;      // PI / 180
	constexpr auto RadToDeg = 57.29577951308232;         // 180 / PI
    constexpr auto Sqrt2 = 1.41421356237309504880;

    constexpr auto int_max = std::numeric_limits<int>::max();
    constexpr auto int_min = std::numeric_limits<int>::min();
    constexpr auto flt_max = std::numeric_limits<float>::max();
    constexpr auto flt_min = std::numeric_limits<float>::min();
    constexpr auto flt_eps = std::numeric_limits<float>::epsilon();
    constexpr auto dbl_max = std::numeric_limits<double>::max();
    constexpr auto dbl_min = std::numeric_limits<double>::min();
    constexpr auto dbl_eps = std::numeric_limits<double>::epsilon();

} 

/*
//=================================
// DECLARATIONS for singular floats
//=================================
namespace waavs
{
	INLINE double min(double a, double b) noexcept;
	INLINE double max(double a, double b) noexcept;
    
    INLINE float abs(float a);
    INLINE float acos(float a);
    INLINE float asin(float a);

    INLINE float atan(float a);
    INLINE float atan2(float y, float x);

    INLINE float clamp(float a, float min, float max);
    INLINE float cos(float a);
    INLINE float exp(float a);
    INLINE float exp2(float a);
    INLINE float fmod(float a, float b);
    INLINE float log(float a);
    INLINE float log2(float a);
    INLINE float max(float a, float b);
    INLINE float min(float a, float b);

    INLINE float pow(float a, float b);
    INLINE double powd(double a, double b);
    
    INLINE float sign(float a);
    INLINE float sin(float a);
    INLINE float sqr(float a);
    INLINE float sqrt(float a);
    INLINE float tan(float a);


    INLINE constexpr float radians(float a);
    INLINE constexpr float degrees(float a);
    INLINE constexpr float lerp(float a, float b, float u);
    INLINE void swap(float& a, float& b);
    INLINE float smoothStep(float a, float b, float u);
    INLINE float bias(float a, float bias);
    INLINE float gain(float a, float gain);

    INLINE float map(float x, float olow, float ohigh, float rlow, float rhigh);
    INLINE float floor(float a);
    INLINE float ceil(float a);
    INLINE bool isNaN(const float a);
    INLINE bool isfinite(float a);

}
*/

//==================================
// IMPLEMENTATION of FUNCTIONS
//==================================



namespace waavs
{
    INLINE float sign(float a) noexcept { return a < 0 ? -1.0f : 1.0f; }
    INLINE float max(float a, float b) noexcept { return (a > b) ? a : b; }
    INLINE float min(float a, float b) noexcept { return (a < b) ? a : b; }
    INLINE float clamp(float a, float min_, float max_) noexcept { return min(max(a, min_), max_); }
    
    INLINE float abs(float a) noexcept { return a < 0 ? -a : a; }
    INLINE float acos(float a) noexcept { return std::acos(a); }
    INLINE float asin(float a) noexcept { return std::asin(a); }

    INLINE float atan(float a) noexcept { return std::atan(a); }
    INLINE float atan2(float y, float x) noexcept { return std::atan2(y, x); }



    INLINE float cos(float a) noexcept { return std::cos(a); }
    INLINE float exp(float a) noexcept { return std::exp(a); }
    INLINE float exp2(float a) noexcept { return std::exp2(a); }
    INLINE float fmod(float a, float b) noexcept { return std::fmod(a, b); }
    INLINE float log(float a) noexcept { return std::log(a); }
    INLINE float log2(float a) noexcept { return std::log2(a); }


    INLINE float pow(float a, float b) noexcept { return std::pow(a, b); }
    INLINE double powd(double a, double b) noexcept { return std::pow(a, b); }


    INLINE float sin(float a) noexcept { return std::sin(a); }
    INLINE float sqr(float a) noexcept { return a * a; }
    INLINE float sqrt(float a) noexcept { return std::sqrt(a); }
    INLINE float tan(float a) noexcept { return std::tan(a); }

    INLINE bool isfinite(float a) { return std::isfinite(a); }
    INLINE constexpr float radiansf(float a) { return float(a * 0.017453292519943295); }
    INLINE constexpr float degreesf(float a) { return float(a * 57.29577951308232); }
    INLINE constexpr float lerp(float a, float b, float u) { return a * (1.0f - u) + b * u; }
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

    INLINE float floor(float a) { return std::floor(a); }
    INLINE float ceil(float a) { return std::ceil(a); }
    INLINE bool isNaN(const float a) { return std::isnan(a); }

}


namespace waavs
{
    INLINE double min(double  a, double b) noexcept { return a < b ? a : b; }
    INLINE double max(double a, double b) noexcept { return a > b ? a : b; }
    INLINE double abs(double a) noexcept { return a < 0 ? -a : a; }

    INLINE double atan(double a) noexcept { return std::atan(a); }
    INLINE double atan2(double y, double x) noexcept { return std::atan2(y, x); }

    
    INLINE double clamp(double a, double min_, double max_) noexcept { return min(max(a, min_), max_); }

    INLINE double radians(double a) noexcept { return (a * 0.017453292519943295); }
    INLINE double degrees(double a) noexcept { return (a * 57.29577951308232); }

    // radians_normalize()
    // Normalize a radian value to keep it in the range of 0 to 2pi
    INLINE double radians_normalize(double rad) noexcept
    {
        double v = std::fmod(rad, (2.0 * pi));
        if (v < 0.0)
        {
            return v + (2.0 * pi);
        }
        return v;
    }

    // vector_angle()
    // 
    // Calculate angle between two vectors, where the difference
    // has already been calculated
    INLINE double vector_angle(double dx, double dy) noexcept
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

//=======================================
// DECLARATIONS for singular integers
//=======================================


namespace waavs
{
    INLINE int abs(int a);
    INLINE int min(int a, int b);
    INLINE int max(int a, int b);
    INLINE int clamp(int a, int min, int max);
    INLINE int sign(int a);
    INLINE int pow2(int a);
    INLINE void swap(int& a, int& b);

    INLINE size_t min(size_t a, size_t b);
    INLINE size_t max(size_t a, size_t b);

}

// Some useful types
// Vectors, matrices, etc
// This vector stuff could all be done with clever usage of
// templates, but after many different iterations, it seems easier
// just to make it explicit for the kinds of vectors we end up
// using the most, and avoid the whole template thing.
namespace waavs {
    struct vec2f 
    {

        float x=0;
        float y=0;


        INLINE float& operator[](int i);
        INLINE const float& operator[](int i) const;
    } ;

    struct vec2d
    {
		double x = 0;
		double y = 0;

		INLINE double& operator[](int i);
		INLINE const double& operator[](int i) const;
    };

    
    struct vec3f {
        float x = 0;
        float y = 0;
        float z = 0;

        INLINE float& operator[](int i);
        INLINE const float& operator[](int i) const;
    };

    struct vec4f {
        union {
            struct {
                float x;
                float y;
                float z;
                float w;
            };
            struct {    // different from vec4b
                float b;
                float g;
                float r;
                float a;
            };

        };



        INLINE float& operator[](int i);
        INLINE const float& operator[](int i) const;
    };

    // Element access
    INLINE vec3f xyz(const vec4f& a);
    INLINE vec2f xy(const vec4f& a);
}


//=========================================
//  DECLARATION vec2f
//=========================================

namespace waavs
{
    // Vector Sequence Operations
    INLINE constexpr int          size(const vec2f& a);
    INLINE const float* begin(const vec2f& a);
    INLINE const float* end(const vec2f& a);
    INLINE float*       begin(vec2f& a);
    INLINE float*       end(vec2f& a);
    INLINE const float* data(const vec2f& a);
    INLINE float*       data(vec2f& a);

    // Comparison operators
    INLINE bool operator==(const vec2f& a, const vec2f& b);
    INLINE bool operator!=(const vec2f& a, const vec2f& b);

    // Vector operations
    INLINE vec2f operator+(const vec2f& a);
    INLINE vec2f operator-(const vec2f& a);
    INLINE vec2f operator+(const vec2f& a, const vec2f &b);
    INLINE vec2f operator+(const vec2f& a, float b);
    INLINE vec2f operator+(float a, const vec2f& b);
    INLINE vec2f operator-(const vec2f& a, const vec2f& b);
    INLINE vec2f operator-(const vec2f& a, float b);
    INLINE vec2f operator-(float a, const vec2f& b);
    INLINE vec2f operator*(const vec2f& a, const vec2f& b);
    INLINE vec2f operator*(const vec2f& a, float b);
    INLINE vec2f operator*(float a, const vec2f& b);
    INLINE vec2f operator/(const vec2f& a, const vec2f& b);
    INLINE vec2f operator/(const vec2f& a, float b);
    INLINE vec2f operator/(float a, const vec2f& b);

    // Vector assignments
    INLINE vec2f& operator +=(vec2f& a, const vec2f& b);
    INLINE vec2f& operator +=(vec2f& a, float b);
    INLINE vec2f& operator -=(vec2f& a, const vec2f& b);
    INLINE vec2f& operator -=(vec2f& a, float b);
    INLINE vec2f& operator *=(vec2f& a, const vec2f& b);
    INLINE vec2f& operator *=(vec2f& a, float b);
    INLINE vec2f& operator /=(vec2f& a, const vec2f& b);
    INLINE vec2f& operator /=(vec2f& a, float b);

    // 2D Vector Products and Length
    INLINE float dot(const vec2f& a, const vec2f& b);
    INLINE float cross(const vec2f& a, const vec2f& b);

    INLINE float length(const vec2f& a);
    INLINE float lengthSquared(const vec2f& a);
    INLINE vec2f normalize(const vec2f& a);
    INLINE float distance(const vec2f& a, const vec2f& b);
    INLINE float distanceSquared(const vec2f& a, const vec2f& b);
    INLINE float angle(const vec2f& a, const vec2f& b);

    // min & max for elements, and clamp
    INLINE vec2f max(const vec2f& a, const vec2f& b);
    INLINE vec2f min(const vec2f& a, const vec2f& b);
    INLINE vec2f max(const vec2f& a, float b);
    INLINE vec2f min(const vec2f& a, float b);
    INLINE vec2f clamp(const vec2f& a, float min, float max);
    INLINE vec2f lerp(const vec2f& a, const vec2f& b, float u);
    INLINE vec2f lerp(const vec2f& a, const vec2f& b, const vec2f& u);

    INLINE float max(const vec2f& a);
    INLINE float min(const vec2f& a);
    INLINE float sum(const vec2f& a);
    INLINE float mean(const vec2f& a);

    // Functions applied to vector elements
    INLINE vec2f abs(const vec2f& a);
    INLINE vec2f sqr(const vec2f& a);
    INLINE vec2f sqrt(const vec2f& a);
    INLINE vec2f exp(const vec2f& a);
    INLINE vec2f log(const vec2f& a);
    INLINE vec2f exp2(const vec2f& a);
    INLINE vec2f log2(const vec2f& a);

    INLINE vec2f pow(const vec2f& a, float b);
    INLINE vec2f pow(const vec2f& a, const vec2f& b);
    INLINE vec2f gain(const vec2f& a, float b);
    INLINE bool isfinite(const vec2f& a);
    INLINE void swap(vec2f& a, vec2f& b);

}


//=========================================
//  DECLARATION vec3f
//=========================================

namespace waavs
{
    // Vector Sequence Operations
    INLINE int          size(const vec3f& a);
    INLINE const float* begin(const vec3f& a);
    INLINE const float* end(const vec3f& a);
    INLINE float* begin(vec3f& a);
    INLINE float* end(vec3f& a);
    INLINE const float* data(const vec3f& a);
    INLINE float* data(vec3f& a);

    // Comparison operators
    INLINE bool operator==(const vec3f& a, const vec3f& b);
    INLINE bool operator!=(const vec3f& a, const vec3f& b);

    // Vector operations
    INLINE vec3f operator+(const vec3f& a);
    INLINE vec3f operator-(const vec3f& a);
    INLINE vec3f operator+(const vec3f& a, const vec3f& b);
    INLINE vec3f operator+(const vec3f& a, float b);
    INLINE vec3f operator+(float a, const vec3f& b);
    INLINE vec3f operator-(const vec3f& a, const vec3f& b);
    INLINE vec3f operator-(const vec3f& a, float b);
    INLINE vec3f operator-(float a, const vec3f& b);
    INLINE vec3f operator*(const vec3f& a, const vec3f& b);
    INLINE vec3f operator*(const vec3f& a, float b);
    INLINE vec3f operator*(float a, const vec3f& b);
    INLINE vec3f operator/(const vec3f& a, const vec3f& b);
    INLINE vec3f operator/(const vec3f& a, float b);
    INLINE vec3f operator/(float a, const vec3f& b);

    // Vector assignments
    INLINE vec3f& operator +=(vec3f& a, const vec3f& b);
    INLINE vec3f& operator +=(vec3f& a, float b);
    INLINE vec3f& operator -=(vec3f& a, const vec3f& b);
    INLINE vec3f& operator -=(vec3f& a, float b);
    INLINE vec3f& operator *=(vec3f& a, const vec3f& b);
    INLINE vec3f& operator *=(vec3f& a, float b);
    INLINE vec3f& operator /=(vec3f& a, const vec3f& b);
    INLINE vec3f& operator /=(vec3f& a, float b);

    // 2D Vector Products and Length
    INLINE float dot(const vec3f& a, const vec3f& b);
    INLINE vec3f cross(const vec3f& a, const vec3f& b);

    INLINE float length(const vec3f& a);
    INLINE float lengthSquared(const vec3f& a);
    INLINE vec3f normalize(const vec3f& a);
    INLINE float distance(const vec3f& a, const vec3f& b);
    INLINE float distanceSquared(const vec3f& a, const vec3f& b);
    INLINE float angle(const vec3f& a, const vec3f& b);

    INLINE vec3f orthogonal(const vec3f& v);
    INLINE vec3f orthonormalize(const vec3f& a, const vec3f& b);
    INLINE vec3f reflect(const vec3f& w, const vec3f& n);
    INLINE vec3f refract(const vec3f& w, const vec3f& n, float inv_eta);
}


//=========================================
//  DECLARATION vec4f
//=========================================

namespace waavs
{
    // Vector Sequence Operations
    INLINE int          size(const vec4f& a);
    INLINE const float* begin(const vec4f& a);
    INLINE const float* end(const vec4f& a);
    INLINE float*       begin(vec4f& a);
    INLINE float*       end(vec4f& a);
    INLINE const float* data(const vec4f& a);
    INLINE float*       data(vec4f& a);

    // Comparison operators
    INLINE bool operator==(const vec4f& a, const vec4f& b);
    INLINE bool operator!=(const vec4f& a, const vec4f& b);

    // Vector operations
    INLINE vec4f operator+(const vec4f& a);
    INLINE vec4f operator-(const vec4f& a);
    INLINE vec4f operator+(const vec4f& a, const vec4f& b);
    INLINE vec4f operator+(const vec4f& a, float b);
    INLINE vec4f operator+(float a, const vec4f& b);
    INLINE vec4f operator-(const vec4f& a, const vec4f& b);
    INLINE vec4f operator-(const vec4f& a, float b);
    INLINE vec4f operator-(float a, const vec4f& b);
    INLINE vec4f operator*(const vec4f& a, const vec4f& b);
    INLINE vec4f operator*(const vec4f& a, float b);
    INLINE vec4f operator*(float a, const vec4f& b);
    INLINE vec4f operator/(const vec4f& a, const vec4f& b);
    INLINE vec4f operator/(const vec4f& a, float b);
    INLINE vec4f operator/(float a, const vec4f& b);

    // Vector assignments
    INLINE vec4f& operator +=(vec4f& a, const vec4f& b);
    INLINE vec4f& operator +=(vec4f& a, float b);
    INLINE vec4f& operator -=(vec4f& a, const vec4f& b);
    INLINE vec4f& operator -=(vec4f& a, float b);
    INLINE vec4f& operator *=(vec4f& a, const vec4f& b);
    INLINE vec4f& operator *=(vec4f& a, float b);
    INLINE vec4f& operator /=(vec4f& a, const vec4f& b);
    INLINE vec4f& operator /=(vec4f& a, float b);

    // Vector Products and Length
    INLINE float dot(const vec4f& a, const vec4f& b);

    INLINE float length(const vec4f& a);
    INLINE float lengthSquared(const vec4f& a);
    INLINE vec4f normalize(const vec4f& a);
    INLINE float distance(const vec4f& a, const vec4f& b);
    INLINE float distanceSquared(const vec4f& a, const vec4f& b);
    INLINE float angle(const vec4f& a, const vec4f& b);

    INLINE vec4f slerp(const vec4f& a, const vec4f& b, float u);

    // min & max for elements, and clamp
    INLINE vec4f max(const vec4f& a, const vec4f& b);
    INLINE vec4f min(const vec4f& a, const vec4f& b);
    INLINE vec4f max(const vec4f& a, float b);
    INLINE vec4f min(const vec4f& a, float b);
    INLINE vec4f clamp(const vec4f& a, float min, float max);
    INLINE vec4f lerp(const vec4f& a, const vec4f& b, float u);
    INLINE vec4f lerp(const vec4f& a, const vec4f& b, const vec4f& u);

    INLINE float max(const vec4f& a);
    INLINE float min(const vec4f& a);
    INLINE float sum(const vec4f& a);
    INLINE float mean(const vec4f& a);

    // Functions applied to vector elements
    INLINE vec4f abs(const vec4f& a);
    INLINE vec4f sqr(const vec4f& a);
    INLINE vec4f sqrt(const vec4f& a);
    INLINE vec4f exp(const vec4f& a);
    INLINE vec4f log(const vec4f& a);
    INLINE vec4f exp2(const vec4f& a);
    INLINE vec4f log2(const vec4f& a);

    INLINE vec4f pow(const vec4f& a, float b);
    INLINE vec4f pow(const vec4f& a, const vec4f& b);
    INLINE vec4f gain(const vec4f& a, float b);
    INLINE bool isfinite(const vec4f& a);
    INLINE void swap(vec4f& a, vec4f& b);

    // Quaternion operatons represented as xi + yj + zk + w
    // const auto identity_quat4f = vec4f{0, 0, 0, 1};
    INLINE vec4f quat_mul(const vec4f& a, float b);
    INLINE vec4f quat_mul(const vec4f& a, const vec4f& b);
    INLINE vec4f quat_conjugate(const vec4f& a);
    INLINE vec4f quat_inverse(const vec4f& a);
}


//============================================
// INTEGER VECTOR TYPES
//============================================

namespace waavs 
{
    struct vec2i
    {
        int x = 0;
        int y = 0;

        INLINE int&         operator[](int i);
        INLINE const int&   operator[](int i) const;
    };

    struct vec3i
    {
        int x = 0;
        int y = 0;
        int z = 0;

        INLINE int& operator[](int i);
        INLINE const int& operator[](int i) const;
    };

    struct vec4i
    {
        int x = 0;
        int y = 0;
        int z = 0;
        int w = 0;

        INLINE int& operator[](int i);
        INLINE const int& operator[](int i) const;
    };

    struct vec3b
    {
        union {
            struct {
                uint8_t x;
                uint8_t y;
                uint8_t z;
            };
            struct {
                uint8_t b;
                uint8_t g;
                uint8_t r;
            };
        };


        INLINE uint8_t& operator[](int i);
        INLINE const uint8_t& operator[](int i) const;
    };

    struct vec4b
    {
        union {
            struct {
                uint8_t x;
                uint8_t y;
                uint8_t z;
                uint8_t w;
            };
            struct {
                uint8_t b;
                uint8_t g;
                uint8_t r;
                uint8_t a;
            };
            uint32_t value=0;
        };


        INLINE uint8_t & operator[](int i);
        INLINE const uint8_t & operator[](int i) const;
    };

    INLINE vec3i xyz(const vec4i& a);

}

//=========================================
//  DECLARATION vec2i
//=========================================
namespace waavs
{
    // Vector sequence operations.
    INLINE int        size(const vec2i& a);
    INLINE const int* begin(const vec2i& a);
    INLINE const int* end(const vec2i& a);
    INLINE int* begin(vec2i& a);
    INLINE int* end(vec2i& a);
    INLINE const int* data(const vec2i& a);
    INLINE int* data(vec2i& a);

    // Vector comparison operations.
    INLINE bool operator==(const vec2i& a, const vec2i& b);
    INLINE bool operator!=(const vec2i& a, const vec2i& b);

    // Vector operations.
    INLINE vec2i operator+(const vec2i& a);
    INLINE vec2i operator-(const vec2i& a);
    INLINE vec2i operator+(const vec2i& a, const vec2i& b);
    INLINE vec2i operator+(const vec2i& a, int b);
    INLINE vec2i operator+(int a, const vec2i& b);
    INLINE vec2i operator-(const vec2i& a, const vec2i& b);
    INLINE vec2i operator-(const vec2i& a, int b);
    INLINE vec2i operator-(int a, const vec2i& b);
    INLINE vec2i operator*(const vec2i& a, const vec2i& b);
    INLINE vec2i operator*(const vec2i& a, int b);
    INLINE vec2i operator*(int a, const vec2i& b);
    INLINE vec2i operator/(const vec2i& a, const vec2i& b);
    INLINE vec2i operator/(const vec2i& a, int b);
    INLINE vec2i operator/(int a, const vec2i& b);

    // Vector assignments
    INLINE vec2i& operator+=(vec2i& a, const vec2i& b);
    INLINE vec2i& operator+=(vec2i& a, int b);
    INLINE vec2i& operator-=(vec2i& a, const vec2i& b);
    INLINE vec2i& operator-=(vec2i& a, int b);
    INLINE vec2i& operator*=(vec2i& a, const vec2i& b);
    INLINE vec2i& operator*=(vec2i& a, int b);
    INLINE vec2i& operator/=(vec2i& a, const vec2i& b);
    INLINE vec2i& operator/=(vec2i& a, int b);

    // Max element and clamp.
    INLINE vec2i max(const vec2i& a, int b);
    INLINE vec2i min(const vec2i& a, int b);
    INLINE vec2i max(const vec2i& a, const vec2i& b);
    INLINE vec2i min(const vec2i& a, const vec2i& b);
    INLINE vec2i clamp(const vec2i& x, int min, int max);

    INLINE int max(const vec2i& a);
    INLINE int min(const vec2i& a);
    INLINE int sum(const vec2i& a);

    // Functions applied to vector elements
    INLINE vec2i abs(const vec2i& a);
    INLINE void  swap(vec2i& a, vec2i& b);
}


//=========================================
//  DECLARATION vec3i
//=========================================

namespace waavs
{
    // Vector sequence operations.
    INLINE int        size(const vec3i& a);
    INLINE const int* begin(const vec3i& a);
    INLINE const int* end(const vec3i& a);
    INLINE int* begin(vec3i& a);
    INLINE int* end(vec3i& a);
    INLINE const int* data(const vec3i& a);
    INLINE int* data(vec3i& a);

    // Vector comparison operations.
    INLINE bool operator==(const vec3i& a, const vec3i& b);
    INLINE bool operator!=(const vec3i& a, const vec3i& b);

    // Vector operations.
    INLINE vec3i operator+(const vec3i& a);
    INLINE vec3i operator-(const vec3i& a);
    INLINE vec3i operator+(const vec3i& a, const vec3i& b);
    INLINE vec3i operator+(const vec3i& a, int b);
    INLINE vec3i operator+(int a, const vec3i& b);
    INLINE vec3i operator-(const vec3i& a, const vec3i& b);
    INLINE vec3i operator-(const vec3i& a, int b);
    INLINE vec3i operator-(int a, const vec3i& b);
    INLINE vec3i operator*(const vec3i& a, const vec3i& b);
    INLINE vec3i operator*(const vec3i& a, int b);
    INLINE vec3i operator*(int a, const vec3i& b);
    INLINE vec3i operator/(const vec3i& a, const vec3i& b);
    INLINE vec3i operator/(const vec3i& a, int b);
    INLINE vec3i operator/(int a, const vec3i& b);

    // Vector assignments
    INLINE vec3i& operator+=(vec3i& a, const vec3i& b);
    INLINE vec3i& operator+=(vec3i& a, int b);
    INLINE vec3i& operator-=(vec3i& a, const vec3i& b);
    INLINE vec3i& operator-=(vec3i& a, int b);
    INLINE vec3i& operator*=(vec3i& a, const vec3i& b);
    INLINE vec3i& operator*=(vec3i& a, int b);
    INLINE vec3i& operator/=(vec3i& a, const vec3i& b);
    INLINE vec3i& operator/=(vec3i& a, int b);

    // Max element and clamp.
    INLINE vec3i max(const vec3i& a, int b);
    INLINE vec3i min(const vec3i& a, int b);
    INLINE vec3i max(const vec3i& a, const vec3i& b);
    INLINE vec3i min(const vec3i& a, const vec3i& b);
    INLINE vec3i clamp(const vec3i& x, int min, int max);

    INLINE int max(const vec3i& a);
    INLINE int min(const vec3i& a);
    INLINE int sum(const vec3i& a);

    // Functions applied to vector elements
    INLINE vec3i abs(const vec3i& a);
    INLINE void  swap(vec3i& a, vec3i& b);
}

//=========================================
//  DECLARATION vec3b
//=========================================
namespace waavs
{
    INLINE int        size(const vec3b& a);
    INLINE const uint8_t* begin(const vec3b& a);
    INLINE const uint8_t* end(const vec3b& a);
    INLINE uint8_t* begin(vec3b& a);
    INLINE uint8_t* end(vec3b& a);
    INLINE const uint8_t* data(const vec3b& a);
    INLINE uint8_t* data(vec3b& a);

}


//=========================================
//  DECLARATION vec4b
//=========================================
namespace waavs
{
    INLINE int        size(const vec4b& a);
    INLINE const uint8_t* begin(const vec4b& a);
    INLINE const uint8_t* end(const vec4b& a);
    INLINE uint8_t* begin(vec4b& a);
    INLINE uint8_t* end(vec4b& a);
    INLINE const uint8_t* data(const vec4b& a);
    INLINE uint8_t* data(vec4b& a);

}

//=========================================
//  DECLARATION vec4i
//=========================================

namespace waavs
{
    // Vector sequence operations.
    INLINE int        size(const vec4i& a);
    INLINE const int* begin(const vec4i& a);
    INLINE const int* end(const vec4i& a);
    INLINE int* begin(vec4i& a);
    INLINE int* end(vec4i& a);
    INLINE const int* data(const vec4i& a);
    INLINE int* data(vec4i& a);

    // Vector comparison operations.
    INLINE bool operator==(const vec4i& a, const vec4i& b);
    INLINE bool operator!=(const vec4i& a, const vec4i& b);

    // Vector operations.
    INLINE vec4i operator+(const vec4i& a);
    INLINE vec4i operator-(const vec4i& a);
    INLINE vec4i operator+(const vec4i& a, const vec4i& b);
    INLINE vec4i operator+(const vec4i& a, int b);
    INLINE vec4i operator+(int a, const vec4i& b);
    INLINE vec4i operator-(const vec4i& a, const vec4i& b);
    INLINE vec4i operator-(const vec4i& a, int b);
    INLINE vec4i operator-(int a, const vec4i& b);
    INLINE vec4i operator*(const vec4i& a, const vec4i& b);
    INLINE vec4i operator*(const vec4i& a, int b);
    INLINE vec4i operator*(int a, const vec4i& b);
    INLINE vec4i operator/(const vec4i& a, const vec4i& b);
    INLINE vec4i operator/(const vec4i& a, int b);
    INLINE vec4i operator/(int a, const vec4i& b);

    // Vector assignments
    INLINE vec4i& operator+=(vec4i& a, const vec4i& b);
    INLINE vec4i& operator+=(vec4i& a, int b);
    INLINE vec4i& operator-=(vec4i& a, const vec4i& b);
    INLINE vec4i& operator-=(vec4i& a, int b);
    INLINE vec4i& operator*=(vec4i& a, const vec4i& b);
    INLINE vec4i& operator*=(vec4i& a, int b);
    INLINE vec4i& operator/=(vec4i& a, const vec4i& b);
    INLINE vec4i& operator/=(vec4i& a, int b);

    // Max element and clamp.
    INLINE vec4i max(const vec4i& a, int b);
    INLINE vec4i min(const vec4i& a, int b);
    INLINE vec4i max(const vec4i& a, const vec4i& b);
    INLINE vec4i min(const vec4i& a, const vec4i& b);
    INLINE vec4i clamp(const vec4i& x, int min, int max);

    INLINE int max(const vec4i& a);
    INLINE int min(const vec4i& a);
    INLINE int sum(const vec4i& a);

    // Functions applied to vector elements
    INLINE vec4i abs(const vec4i& a);
    INLINE void  swap(vec4i& a, vec4i& b);

    // Vector comparison operations.
    INLINE bool operator==(const vec3b& a, const vec3b& b);
    INLINE bool operator!=(const vec3b& a, const vec3b& b);
    
    // Vector comparison operations.
    INLINE bool operator==(const vec4b& a, const vec4b& b);
    INLINE bool operator!=(const vec4b& a, const vec4b& b);
}


//=========================================
// DECLARATION - MATRICES
//=========================================

namespace waavs
{
    // Small Fixed-size matrices stored in column major format.
    struct mat2f {
        vec2f x = { 1, 0 };
        vec2f y = { 0, 1 };

        INLINE vec2f& operator[](int i);
        INLINE const vec2f& operator[](int i) const;
    };

    // Small Fixed-size matrices stored in column major format.
    struct mat3f {
        vec3f x = { 1, 0, 0 };
        vec3f y = { 0, 1, 0 };
        vec3f z = { 0, 0, 1 };

        INLINE vec3f& operator[](int i);
        INLINE const vec3f& operator[](int i) const;
    };

    // Small Fixed-size matrices stored in column major format.
    struct mat4f {
        vec4f x = { 1, 0, 0, 0 };
        vec4f y = { 0, 1, 0, 0 };
        vec4f z = { 0, 0, 1, 0 };
        vec4f w = { 0, 0, 0, 1 };

        INLINE vec4f& operator[](int i);
        INLINE const vec4f& operator[](int i) const;
    };

}


namespace waavs
{
    // Matrix comparisons.
    INLINE bool operator==(const mat2f& a, const mat2f& b);
    INLINE bool operator!=(const mat2f& a, const mat2f& b);

    // Matrix operations.
    INLINE mat2f operator+(const mat2f& a, const mat2f& b);
    INLINE mat2f operator*(const mat2f& a, float b);
    INLINE vec2f operator*(const mat2f& a, const vec2f& b);
    INLINE vec2f operator*(const vec2f& a, const mat2f& b);
    INLINE mat2f operator*(const mat2f& a, const mat2f& b);

    // Matrix assignments.
    INLINE mat2f& operator+=(mat2f& a, const mat2f& b);
    INLINE mat2f& operator*=(mat2f& a, const mat2f& b);
    INLINE mat2f& operator*=(mat2f& a, float b);

    // Matrix diagonals and transposes.
    INLINE vec2f diagonal(const mat2f& a);
    INLINE mat2f transpose(const mat2f& a);

    // Matrix adjoints, determinants and inverses.
    INLINE float determinant(const mat2f& a);
    INLINE mat2f adjoint(const mat2f& a);
    INLINE mat2f inverse(const mat2f& a);

    // Matrix comparisons.
    INLINE bool operator==(const mat3f& a, const mat3f& b);
    INLINE bool operator!=(const mat3f& a, const mat3f& b);

    // Matrix operations.
    INLINE mat3f operator+(const mat3f& a, const mat3f& b);
    INLINE mat3f operator*(const mat3f& a, float b);
    INLINE vec3f operator*(const mat3f& a, const vec3f& b);
    INLINE vec3f operator*(const vec3f& a, const mat3f& b);
    INLINE mat3f operator*(const mat3f& a, const mat3f& b);

    // Matrix assignments.
    INLINE mat3f& operator+=(mat3f& a, const mat3f& b);
    INLINE mat3f& operator*=(mat3f& a, const mat3f& b);
    INLINE mat3f& operator*=(mat3f& a, float b);

    // Matrix diagonals and transposes.
    INLINE vec3f diagonal(const mat3f& a);
    INLINE mat3f transpose(const mat3f& a);

    // Matrix adjoints, determinants and inverses.
    INLINE float determinant(const mat3f& a);
    INLINE mat3f adjoint(const mat3f& a);
    INLINE mat3f inverse(const mat3f& a);

    // Constructs a basis from a direction
    INLINE mat3f basis_fromz(const vec3f& v);

    // Matrix comparisons.
    INLINE bool operator==(const mat4f& a, const mat4f& b);
    INLINE bool operator!=(const mat4f& a, const mat4f& b);

    // Matrix operations.
    INLINE mat4f operator+(const mat4f& a, const mat4f& b);
    INLINE mat4f operator*(const mat4f& a, float b);
    INLINE vec4f operator*(const mat4f& a, const vec4f& b);
    INLINE vec4f operator*(const vec4f& a, const mat4f& b);
    INLINE mat4f operator*(const mat4f& a, const mat4f& b);

    // Matrix assignments.
    INLINE mat4f& operator+=(mat4f& a, const mat4f& b);
    INLINE mat4f& operator*=(mat4f& a, const mat4f& b);
    INLINE mat4f& operator*=(mat4f& a, float b);

    // Matrix diagonals and transposes.
    INLINE vec4f diagonal(const mat4f& a);
    INLINE mat4f transpose(const mat4f& a);
}

//===================================================
// DECLARATIONS - RIGID BODY TRANSFORMS/FRAMES
//===================================================

namespace waavs
{
    // Rigid frames stored as a column-major affine transform matrix.
    struct frame2f {
        vec2f x = { 1, 0 };
        vec2f y = { 0, 1 };
        vec2f o = { 0, 0 };

        INLINE vec2f& operator[](int i);
        INLINE const vec2f& operator[](int i) const;
    };

    // Rigid frames stored as a column-major affine transform matrix.
    struct frame3f {
        vec3f x = { 1, 0, 0 };
        vec3f y = { 0, 1, 0 };
        vec3f z = { 0, 0, 1 };
        vec3f o = { 0, 0, 0 };

        INLINE vec3f& operator[](int i);
        INLINE const vec3f& operator[](int i) const;
    };

    // Frame properties
    INLINE mat2f rotation(const frame2f& a);
    INLINE vec2f translation(const frame2f& a);

    // Frame construction
    INLINE frame2f make_frame(const mat2f& m, const vec2f& t);

    // Conversion between frame and mat
    INLINE mat3f   frame_to_mat(const frame2f& f);
    INLINE frame2f mat_to_frame(const mat3f& ma);

    // Frame comparisons.
    INLINE bool operator==(const frame2f& a, const frame2f& b);
    INLINE bool operator!=(const frame2f& a, const frame2f& b);

    // Frame composition, equivalent to affine matrix product.
    INLINE frame2f  operator*(const frame2f& a, const frame2f& b);
    INLINE frame2f& operator*=(frame2f& a, const frame2f& b);

    // Frame inverse, equivalent to rigid affine inverse.
    INLINE frame2f inverse(const frame2f& a, bool non_rigid = false);

    // Frame properties
    INLINE mat3f rotation(const frame3f& a);
    INLINE vec3f translation(const frame3f& a);

    // Frame construction
    INLINE frame3f make_frame(const mat3f& m, const vec3f& t);

    // Conversion between frame and mat
    INLINE mat4f   frame_to_mat(const frame3f& f);
    INLINE frame3f mat_to_frame(const mat4f& m);

    // Frame comparisons.
    INLINE bool operator==(const frame3f& a, const frame3f& b);
    INLINE bool operator!=(const frame3f& a, const frame3f& b);

    // Frame composition, equivalent to affine matrix product.
    INLINE frame3f  operator*(const frame3f& a, const frame3f& b);
    INLINE frame3f& operator*=(frame3f& a, const frame3f& b);

    // Frame inverse, equivalent to rigid affine inverse.
    INLINE frame3f inverse(const frame3f& a, bool non_rigid = false);

    // Frame construction from axis.
    INLINE frame3f frame_fromz(const vec3f& o, const vec3f& v);
    INLINE frame3f frame_fromzx(const vec3f& o, const vec3f& z_, const vec3f& x_);
}

//=============================================
// DECLERATION - QUATERNIONS
//=============================================

namespace waavs
{
    // Quaternions to represent rotations
    struct quat4f {
        float x = 0;
        float y = 0;
        float z = 0;
        float w = 1;
    };

    // Quaternion operatons
    INLINE quat4f operator+(const quat4f& a, const quat4f& b);
    INLINE quat4f operator*(const quat4f& a, float b);
    INLINE quat4f operator/(const quat4f& a, float b);
    INLINE quat4f operator*(const quat4f& a, const quat4f& b);

    // Quaterion operations
    INLINE float  dot(const quat4f& a, const quat4f& b);
    INLINE float  length(const quat4f& a);
    INLINE quat4f normalize(const quat4f& a);
    INLINE quat4f conjugate(const quat4f& a);
    INLINE quat4f inverse(const quat4f& a);
    INLINE float  uangle(const quat4f& a, const quat4f& b);
    INLINE quat4f lerp(const quat4f& a, const quat4f& b, float t);
    INLINE quat4f nlerp(const quat4f& a, const quat4f& b, float t);
    INLINE quat4f slerp(const quat4f& a, const quat4f& b, float t);
}

//=============================================
// TRANSFORMS
//=============================================

namespace waavs
{
    // Transforms points, vectors and directions by matrices.
    INLINE vec2f transform_point(const mat3f& a, const vec2f& b);
    INLINE vec2f transform_vector(const mat3f& a, const vec2f& b);
    INLINE vec2f transform_direction(const mat3f& a, const vec2f& b);
    INLINE vec2f transform_normal(const mat3f& a, const vec2f& b);
    INLINE vec2f transform_vector(const mat2f& a, const vec2f& b);
    INLINE vec2f transform_direction(const mat2f& a, const vec2f& b);
    INLINE vec2f transform_normal(const mat2f& a, const vec2f& b);

    INLINE vec3f transform_point(const mat4f& a, const vec3f& b);
    INLINE vec3f transform_vector(const mat4f& a, const vec3f& b);
    INLINE vec3f transform_direction(const mat4f& a, const vec3f& b);
    INLINE vec3f transform_vector(const mat3f& a, const vec3f& b);
    INLINE vec3f transform_direction(const mat3f& a, const vec3f& b);
    INLINE vec3f transform_normal(const mat3f& a, const vec3f& b);

    // Transforms points, vectors and directions by frames.
    INLINE vec2f transform_point(const frame2f& a, const vec2f& b);
    INLINE vec2f transform_vector(const frame2f& a, const vec2f& b);
    INLINE vec2f transform_direction(const frame2f& a, const vec2f& b);
    INLINE vec2f transform_normal(
        const frame2f& a, const vec2f& b, bool non_rigid = false);

    // Transforms points, vectors and directions by frames.
    INLINE vec3f transform_point(const frame3f& a, const vec3f& b);
    INLINE vec3f transform_vector(const frame3f& a, const vec3f& b);
    INLINE vec3f transform_direction(const frame3f& a, const vec3f& b);
    INLINE vec3f transform_normal(
        const frame3f& a, const vec3f& b, bool non_rigid = false);

    // Transforms points, vectors and directions by frames.
    INLINE vec2f transform_point_inverse(const frame2f& a, const vec2f& b);
    INLINE vec2f transform_vector_inverse(const frame2f& a, const vec2f& b);
    INLINE vec2f transform_direction_inverse(const frame2f& a, const vec2f& b);

    // Transforms points, vectors and directions by frames.
    INLINE vec3f transform_point_inverse(const frame3f& a, const vec3f& b);
    INLINE vec3f transform_vector_inverse(const frame3f& a, const vec3f& b);
    INLINE vec3f transform_direction_inverse(const frame3f& a, const vec3f& b);

    // Translation, scaling and rotations transforms.
    INLINE frame3f translation_frame(const vec3f& a);
    INLINE frame3f scaling_frame(const vec3f& a);
    INLINE frame3f rotation_frame(const vec3f& axis, float angle);
    INLINE frame3f rotation_frame(const vec4f& quat);
    INLINE frame3f rotation_frame(const quat4f& quat);
    INLINE frame3f rotation_frame(const mat3f& rot);

    // Lookat frame. Z-axis can be inverted with inv_xz.
    INLINE frame3f lookat_frame(const vec3f& eye, const vec3f& center,
        const vec3f& up, bool inv_xz = false);

    // OpenGL frustum, ortho and perspecgive matrices.
    INLINE mat4f frustum_mat(float l, float r, float b, float t, float n, float f);
    INLINE mat4f ortho_mat(float l, float r, float b, float t, float n, float f);
    INLINE mat4f ortho2d_mat(float left, float right, float bottom, float top);
    INLINE mat4f ortho_mat(float xmag, float ymag, float nearest, float farthest);
    INLINE mat4f perspective_mat(float fovy, float aspect, float nearest, float farthest);
    INLINE mat4f perspective_mat(float fovy, float aspect, float nearest);

    // Rotation conversions.
    INLINE pair<vec3f, float> rotation_axisangle(const vec4f& quat);
    INLINE vec4f              rotation_quat(const vec3f& axis, float angle);
    INLINE vec4f              rotation_quat(const vec4f& axisangle);
}


//==================================
// DECLARATION PYTHON-LIKE ITERATORS
//==================================
namespace waavs {
    // Python range. Construct an object that iterates over an integer sequence.
    template <typename T>
    constexpr auto range(T max);
    template <typename T>
    constexpr auto range(T min, T max);
    template <typename T>
    constexpr auto range(T min, T max, T step);
}



//=======================================
// IMPLEMENTATION for singular integers
//=======================================

namespace waavs
{
    INLINE int abs(int a) { return a < 0 ? -a : a; }
    INLINE int min(int a, int b) { return (a < b) ? a : b; }
    INLINE int max(int a, int b) { return (a > b) ? a : b; }
    INLINE int clamp(int a, int min_, int max_) { return min(max(a, min_), max_); }
    INLINE int sign(int a) { return a < 0 ? -1 : 1; }
    INLINE int pow2(int a) { return 1 << a; }
    INLINE void swap(int& a, int& b) { return std::swap(a, b); };

    INLINE size_t min(size_t a, size_t b) { return (a < b) ? a : b; }
    INLINE size_t max(size_t a, size_t b) { return (a > b) ? a : b; }
}



//==========================
// IMPLEMENTATION  Vectors
//==========================
namespace waavs
{
    // Vec2
    INLINE float& vec2f::operator[](int i) { return (&x)[i]; }
    INLINE const float& vec2f::operator[](int i)const { return (&x)[i]; }

    INLINE double& vec2d::operator[](int i) { return (&x)[i]; }
    INLINE const double& vec2d::operator[](int i)const { return (&x)[i]; }

    
    // Vec3
    INLINE float& vec3f::operator[](int i) { return (&x)[i]; }
    INLINE const float& vec3f::operator[](int i)const { return (&x)[i]; }

    // Vec4
    INLINE float& vec4f::operator[](int i) { return (&x)[i]; }
    INLINE const float& vec4f::operator[](int i)const { return (&x)[i]; }

    INLINE vec3f xyz(const vec4f& a) { return { a.x, a.y, a.z }; }
    INLINE vec2f xy(const vec4f& a) { return { a.x, a.y}; }
}

//==========================
//  IMPLEMENTATION vec2f
//==========================
namespace waavs
{
    // Vector Sequence Operations
    INLINE constexpr int          size(const vec2f& ) { return 2; }
    INLINE const float* begin(const vec2f& a) { return &a.x; }
    INLINE const float* end(const vec2f& a) { return &a.x + 2; }
    INLINE float* begin(vec2f& a) { return &a.x; }
    INLINE float* end(vec2f& a) { return &a.x + 2; }
    INLINE const float* data(const vec2f& a) { return &a.x; }
    INLINE float* data(vec2f& a) { return &a.x; }


    INLINE bool operator==(const vec2f& a, const vec2f& b) { return ((a.x == b.x) && (a.y == b.y)); }
    INLINE bool operator!=(const vec2f& a, const vec2f& b) { return a.x != b.x || a.y != b.y; }

    // Vector operations
    INLINE vec2f operator+(const vec2f& a) {return a;}
    INLINE vec2f operator-(const vec2f& a) { return { -a.x, -a.y }; }

    INLINE vec2f operator+(const vec2f& a, const vec2f& b) {return { a.x + b.x, a.y + b.y };}
    INLINE vec2f operator+(const vec2f& a, float b) {return { a.x + b, a.y + b };}
    INLINE vec2f operator+(float a, const vec2f& b) { return {a+b.x, a+b.y}; }

    INLINE vec2f operator-(const vec2f& a, const vec2f& b) { return { a.x - b.x, a.y - b.y }; }
    INLINE vec2f operator-(const vec2f& a, float b) { return { a.x - b, a.y - b }; }
    INLINE vec2f operator-(float a, const vec2f& b) { return { a - b.x, a - b.y }; }

    INLINE vec2f operator*(const vec2f& a, const vec2f& b) { return { a.x * b.x, a.y * b.y }; }
    INLINE vec2f operator*(const vec2f& a, float b) { return { a.x * b, a.y * b }; }
    INLINE vec2f operator*(float a, const vec2f& b) { return { a * b.x, a * b.y }; }

    INLINE vec2f operator/(const vec2f& a, const vec2f& b) { return { a.x / b.x, a.y / b.y }; }
    INLINE vec2f operator/(const vec2f& a, float b) { return { a.x / b, a.y / b }; }
    INLINE vec2f operator/(float a, const vec2f& b) { return { a / b.x, a / b.y }; }

    // Vector assignments
    INLINE vec2f& operator +=(vec2f& a, const vec2f& b) { return a = a + b; }
    INLINE vec2f& operator +=(vec2f& a, float b) { return a = a + b; }
    INLINE vec2f& operator -=(vec2f& a, const vec2f& b) { return a = a - b; }
    INLINE vec2f& operator -=(vec2f& a, float b) { return a = a - b; }
    INLINE vec2f& operator *=(vec2f& a, const vec2f& b) { return a = a * b; }
    INLINE vec2f& operator *=(vec2f& a, float b) { return a = a * b; }
    INLINE vec2f& operator /=(vec2f& a, const vec2f& b) { return a = a / b; }
    INLINE vec2f& operator /=(vec2f& a, float b) { return a = a / b; }


    // 2D Vector Products and Length
    INLINE float dot(const vec2f& a, const vec2f& b) { return a.x * b.x + a.y * b.y; }
    INLINE float cross(const vec2f& a, const vec2f& b) { return a.x * b.y - a.y * b.x; }

    INLINE float length(const vec2f& a) { return sqrt(dot(a, a)); }
    INLINE float lengthSquared(const vec2f& a) { return dot(a, a); }
    INLINE vec2f normalize(const vec2f& a) { auto len = length(a); return (len != 0) ? a / len : a; }

    INLINE float distance(const vec2f& a, const vec2f& b) { return length(a - b); }
    INLINE float distanceSquared(const vec2f& a, const vec2f& b) { return dot(a - b, a - b); }
    INLINE float angle(const vec2f& a, const vec2f& b) { return acos(clamp(dot(normalize(a), normalize(b)), (float)-1, (float)1)); }

    // min & max for elements, and clamp
    INLINE vec2f max(const vec2f& a, float b) { return { max(a.x, b), max(a.y,b) }; }
    INLINE vec2f min(const vec2f& a, float b) { return { min(a.x,b),min(a.y,b) }; }
    INLINE vec2f max(const vec2f& a, const vec2f& b) { return { max(a.x,b.x), max(a.y,b.y) }; }
    INLINE vec2f min(const vec2f& a, const vec2f& b) { return { min(a.x,b.x), min(a.y,b.y) }; }
    INLINE vec2f clamp(const vec2f& x, float min_, float max_) { return { clamp(x.x,min_,max_),clamp(x.y,min_,max_) }; }
    INLINE vec2f lerp(const vec2f& a, const vec2f& b, float u) { return a * (1.0f * u) + b * u; }
    INLINE vec2f lerp(const vec2f& a, const vec2f& b, const vec2f& u) { return a * (1.0f - u) + b * u; }

    INLINE float max(const vec2f& a) { return max(a.x, a.y); }
    INLINE float min(const vec2f& a) { return min(a.x, a.y); }
    INLINE float sum(const vec2f& a) { return a.x + a.y; }
    INLINE float mean(const vec2f& a) { return sum(a) / 2.0f; }

    // Functions applied to vector elements
    INLINE vec2f abs(const vec2f& a) { return { abs(a.x),abs(a.y) }; }
    INLINE vec2f sqr(const vec2f& a) { return { sqr(a.x), sqr(a.y) }; }
    INLINE vec2f sqrt(const vec2f& a) { return { sqrt(a.x), sqrt(a.y) }; }
    INLINE vec2f exp(const vec2f& a) { return { exp(a.x), exp(a.y) }; }
    INLINE vec2f log(const vec2f& a) { return { log(a.x), log(a.y) }; }
    INLINE vec2f exp2(const vec2f& a) { return { exp2(a.x), exp2(a.y) }; }
    INLINE vec2f log2(const vec2f& a) { return { log2(a.x), log2(a.y) }; }
    INLINE bool isfinite(const vec2f& a) { return isfinite(a.x) && isfinite(a.y); }
    INLINE vec2f pow(const vec2f& a, float b) { return { pow(a.x,b), pow(a.y,b) }; }
    INLINE vec2f pow(const vec2f& a, const vec2f& b) { return { pow(a.x,b.x), pow(a.y, b.y) }; }
    INLINE vec2f gain(const vec2f& a, float b) { return { gain(a.x,b), gain(a.y,b) }; }
    INLINE void swap(vec2f& a, vec2f& b) { std::swap(a, b); }
}

//=================================================
//  IMPLEMENTATION  vec3f
//=================================================
namespace waavs
{
    // Vector Sequence Operations
    INLINE int          size(const vec3f& ) { return 3; }
    INLINE const float* begin(const vec3f& a) { return &a.x; }
    INLINE const float* end(const vec3f& a) { return &a.x + 3; }
    INLINE float* begin(vec3f& a) { return &a.x; }
    INLINE float* end(vec3f& a) { return &a.x + 3; }
    INLINE const float* data(const vec3f& a) { return &a.x; }
    INLINE float* data(vec3f& a) { return &a.x; }


    INLINE bool operator==(const vec3f& a, const vec3f& b) { return ((a.x == b.x) && (a.y == b.y)); }
    INLINE bool operator!=(const vec3f& a, const vec3f& b) { return a.x != b.x || a.y != b.y; }

    // Vector operations
    INLINE vec3f operator+(const vec3f& a) { return a; }
    INLINE vec3f operator-(const vec3f& a) { return { -a.x, -a.y }; }

    INLINE vec3f operator+(const vec3f& a, const vec3f& b) { return { a.x + b.x, a.y + b.y }; }
    INLINE vec3f operator+(const vec3f& a, float b) { return { a.x + b, a.y + b }; }
    INLINE vec3f operator+(float a, const vec3f& b) { return { a + b.x, a + b.y }; }

    INLINE vec3f operator-(const vec3f& a, const vec3f& b) { return { a.x - b.x, a.y - b.y }; }
    INLINE vec3f operator-(const vec3f& a, float b) { return { a.x - b, a.y - b }; }
    INLINE vec3f operator-(float a, const vec3f& b) { return { a - b.x, a - b.y }; }

    INLINE vec3f operator*(const vec3f& a, const vec3f& b) { return { a.x * b.x, a.y * b.y }; }
    INLINE vec3f operator*(const vec3f& a, float b) { return { a.x * b, a.y * b }; }
    INLINE vec3f operator*(float a, const vec3f& b) { return { a * b.x, a * b.y }; }

    INLINE vec3f operator/(const vec3f& a, const vec3f& b) { return { a.x / b.x, a.y / b.y }; }
    INLINE vec3f operator/(const vec3f& a, float b) { return { a.x / b, a.y / b }; }
    INLINE vec3f operator/(float a, const vec3f& b) { return { a / b.x, a / b.y }; }

    // Vector assignments
    INLINE vec3f& operator +=(vec3f& a, const vec3f& b) { return a = a + b; }
    INLINE vec3f& operator +=(vec3f& a, float b) { return a = a + b; }
    INLINE vec3f& operator -=(vec3f& a, const vec3f& b) { return a = a - b; }
    INLINE vec3f& operator -=(vec3f& a, float b) { return a = a - b; }
    INLINE vec3f& operator *=(vec3f& a, const vec3f& b) { return a = a * b; }
    INLINE vec3f& operator *=(vec3f& a, float b) { return a = a * b; }
    INLINE vec3f& operator /=(vec3f& a, const vec3f& b) { return a = a / b; }
    INLINE vec3f& operator /=(vec3f& a, float b) { return a = a / b; }


    // 2D Vector Products and Length
    INLINE float dot(const vec3f& a, const vec3f& b) { return a.x * b.x + a.y * b.y+a.z*b.z; }
    INLINE vec3f cross(const vec3f& a, const vec3f& b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }

    INLINE float length(const vec3f& a) { return sqrt(dot(a, a)); }
    INLINE float lengthSquared(const vec3f& a) { return dot(a, a); }
    INLINE vec3f normalize(const vec3f& a) { auto len = length(a); return (len != 0) ? a / len : a; }

    INLINE float distance(const vec3f& a, const vec3f& b) { return length(a - b); }
    INLINE float distanceSquared(const vec3f& a, const vec3f& b) { return dot(a - b, a - b); }
    INLINE float angle(const vec3f& a, const vec3f& b) { return acos(clamp(dot(normalize(a), normalize(b)), (float)-1, (float)1)); }

    // Some stuff for 3D specifically
    INLINE vec3f orthogonal(const vec3f& v){ return abs(v.x) > abs(v.z) ? vec3f{ -v.y, v.x, 0 } : vec3f{ 0, -v.z, v.y }; }
    INLINE vec3f orthonormalize(const vec3f& a, const vec3f& b) { return normalize(a - b * dot(a, b)); }
    INLINE vec3f reflect(const vec3f& w, const vec3f& n){ return -w + 2 * dot(n, w) * n; }
    INLINE vec3f refract(const vec3f& w, const vec3f& n, float inv_eta) {
        auto cosine = dot(n, w);
        auto k = 1 + inv_eta * inv_eta * (cosine * cosine - 1);
        if (k < 0) return { 0, 0, 0 };
        return -w * inv_eta + (inv_eta * cosine - sqrt(k)) * n;
    }


    // min & max for elements, and clamp
    INLINE vec3f max(const vec3f& a, float b) { return { max(a.x, b), max(a.y,b),max(a.z,b)}; }
    INLINE vec3f min(const vec3f& a, float b) { return { min(a.x,b),min(a.y,b),min(a.z,b)}; }
    INLINE vec3f max(const vec3f& a, const vec3f& b) { return { max(a.x,b.x), max(a.y,b.y),max(a.z,b.z)}; }
    INLINE vec3f min(const vec3f& a, const vec3f& b) { return { min(a.x,b.x), min(a.y,b.y),min(a.z,b.z)}; }
    INLINE vec3f clamp(const vec3f& x, float min_, float max_) { return { clamp(x.x,min_,max_),clamp(x.y,min_,max_), clamp(x.z,min_,max_)}; }
    INLINE vec3f lerp(const vec3f& a, const vec3f& b, float u) { return a * (1.0f * u) + b * u; }
    INLINE vec3f lerp(const vec3f& a, const vec3f& b, const vec3f& u) { return a * (1.0f - u) + b * u; }

    INLINE float max(const vec3f& a) { return max(max(a.x, a.y),a.z); }
    INLINE float min(const vec3f& a) { return min(min(a.x, a.y),a.z); }
    INLINE float sum(const vec3f& a) { return a.x + a.y+a.z; }
    INLINE float mean(const vec3f& a) { return sum(a) / 3.0f; }

    // Functions applied to vector elements
    INLINE vec3f abs(const vec3f& a) { return { abs(a.x),abs(a.y), abs(a.z)}; }
    INLINE vec3f sqr(const vec3f& a) { return { sqr(a.x), sqr(a.y), sqr(a.z)}; }
    INLINE vec3f sqrt(const vec3f& a) { return { sqrt(a.x), sqrt(a.y), sqrt(a.z)}; }
    INLINE vec3f exp(const vec3f& a) { return { exp(a.x), exp(a.y), exp(a.z)}; }
    INLINE vec3f log(const vec3f& a) { return { log(a.x), log(a.y), log(a.z)}; }
    INLINE vec3f exp2(const vec3f& a) { return { exp2(a.x), exp2(a.y),exp2(a.z)}; }
    INLINE vec3f log2(const vec3f& a) { return { log2(a.x), log2(a.y),log2(a.z)}; }
    INLINE bool isfinite(const vec3f& a) { return isfinite(a.x) && isfinite(a.y) && isfinite(a.z); }
    INLINE vec3f pow(const vec3f& a, float b) { return { pow(a.x,b), pow(a.y,b),pow(a.z,b)}; }
    INLINE vec3f pow(const vec3f& a, const vec3f& b) { return { pow(a.x,b.x), pow(a.y, b.y),pow(a.z,b.z)}; }
    INLINE vec3f gain(const vec3f& a, float b) { return { gain(a.x,b), gain(a.y,b),gain(a.z,b)}; }
    INLINE void swap(vec3f& a, vec3f& b) { std::swap(a, b); }


}

//==============================================
// IMPLEMENTATION - vec4f
//==============================================

namespace waavs
{
    // Vector sequence operations.
    INLINE int          size(const vec4f& ) { return 4; }
    INLINE const float* begin(const vec4f& a) { return &a.x; }
    INLINE const float* end(const vec4f& a) { return &a.x + 4; }
    INLINE float* begin(vec4f& a) { return &a.x; }
    INLINE float* end(vec4f& a) { return &a.x + 4; }
    INLINE const float* data(const vec4f& a) { return &a.x; }
    INLINE float* data(vec4f& a) { return &a.x; }

    // Vector comparison operations.
    INLINE bool operator==(const vec4f& a, const vec4f& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
    }
    INLINE bool operator!=(const vec4f& a, const vec4f& b) {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
    }

    // Vector operations.
    INLINE vec4f operator+(const vec4f& a) { return a; }
    INLINE vec4f operator-(const vec4f& a) { return { -a.x, -a.y, -a.z, -a.w }; }
    INLINE vec4f operator+(const vec4f& a, const vec4f& b) {
        return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    }
    INLINE vec4f operator+(const vec4f& a, float b) {
        return { a.x + b, a.y + b, a.z + b, a.w + b };
    }
    INLINE vec4f operator+(float a, const vec4f& b) {
        return { a + b.x, a + b.y, a + b.z, a + b.w };
    }
    INLINE vec4f operator-(const vec4f& a, const vec4f& b) {
        return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
    }
    INLINE vec4f operator-(const vec4f& a, float b) {
        return { a.x - b, a.y - b, a.z - b, a.w - b };
    }
    INLINE vec4f operator-(float a, const vec4f& b) {
        return { a - b.x, a - b.y, a - b.z, a - b.w };
    }
    INLINE vec4f operator*(const vec4f& a, const vec4f& b) {
        return { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
    }
    INLINE vec4f operator*(const vec4f& a, float b) {
        return { a.x * b, a.y * b, a.z * b, a.w * b };
    }
    INLINE vec4f operator*(float a, const vec4f& b) {
        return { a * b.x, a * b.y, a * b.z, a * b.w };
    }
    INLINE vec4f operator/(const vec4f& a, const vec4f& b) {
        return { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
    }
    INLINE vec4f operator/(const vec4f& a, float b) {
        return { a.x / b, a.y / b, a.z / b, a.w / b };
    }
    INLINE vec4f operator/(float a, const vec4f& b) {
        return { a / b.x, a / b.y, a / b.z, a / b.w };
    }

    // Vector assignments
    INLINE vec4f& operator+=(vec4f& a, const vec4f& b) { return a = a + b; }
    INLINE vec4f& operator+=(vec4f& a, float b) { return a = a + b; }
    INLINE vec4f& operator-=(vec4f& a, const vec4f& b) { return a = a - b; }
    INLINE vec4f& operator-=(vec4f& a, float b) { return a = a - b; }
    INLINE vec4f& operator*=(vec4f& a, const vec4f& b) { return a = a * b; }
    INLINE vec4f& operator*=(vec4f& a, float b) { return a = a * b; }
    INLINE vec4f& operator/=(vec4f& a, const vec4f& b) { return a = a / b; }
    INLINE vec4f& operator/=(vec4f& a, float b) { return a = a / b; }

    // Vector products and lengths.
    INLINE float dot(const vec4f& a, const vec4f& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }
    INLINE float length(const vec4f& a) { return sqrt(dot(a, a)); }
    INLINE float lengthSquared(const vec4f& a) { return dot(a, a); }
    INLINE vec4f normalize(const vec4f& a) {
        auto len = length(a);
        return (len != 0) ? a / len : a;
    }
    INLINE float distance(const vec4f& a, const vec4f& b) { return length(a - b); }
    INLINE float distanceSquared(const vec4f& a, const vec4f& b) {
        return dot(a - b, a - b);
    }
    INLINE float angle(const vec4f& a, const vec4f& b) {
        return acos(clamp(dot(normalize(a), normalize(b)), (float)-1, (float)1));
    }

    // https://en.wikipedia.org/wiki/Slerp
    INLINE vec4f slerp(const vec4f& a, const vec4f& b, float u) 
    {
        auto an = normalize(a), bn = normalize(b);
        auto d = dot(an, bn);
        if (d < 0) {
            bn = -bn;
            d = -d;
        }
        if (d > (float)0.9995) return normalize(an + u * (bn - an));
        auto th = acos(clamp(d, (float)-1, (float)1));
        if (th == 0) return an;
        return an * (sin(th * (1 - u)) / sin(th)) + bn * (sin(th * u) / sin(th));
    }

    // Max element and clamp.
    INLINE vec4f max(const vec4f& a, float b) {
        return { max(a.x, b), max(a.y, b), max(a.z, b), max(a.w, b) };
    }
    INLINE vec4f min(const vec4f& a, float b) {
        return { min(a.x, b), min(a.y, b), min(a.z, b), min(a.w, b) };
    }
    INLINE vec4f max(const vec4f& a, const vec4f& b) {
        return { max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w) };
    }
    INLINE vec4f min(const vec4f& a, const vec4f& b) {
        return { min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w) };
    }
    INLINE vec4f clamp(const vec4f& x, float min, float max) {
        return { clamp(x.x, min, max), clamp(x.y, min, max), clamp(x.z, min, max),
            clamp(x.w, min, max) };
    }
    INLINE vec4f lerp(const vec4f& a, const vec4f& b, float u) {
        return a * (1 - u) + b * u;
    }
    INLINE vec4f lerp(const vec4f& a, const vec4f& b, const vec4f& u) {
        return a * (1 - u) + b * u;
    }

    INLINE float max(const vec4f& a) { return max(max(max(a.x, a.y), a.z), a.w); }
    INLINE float min(const vec4f& a) { return min(min(min(a.x, a.y), a.z), a.w); }
    INLINE float sum(const vec4f& a) { return a.x + a.y + a.z + a.w; }
    INLINE float mean(const vec4f& a) { return sum(a) / 4; }

    // Functions applied to vector elements
    INLINE vec4f abs(const vec4f& a) {
        return { abs(a.x), abs(a.y), abs(a.z), abs(a.w) };
    }
    INLINE vec4f sqr(const vec4f& a) {
        return { sqr(a.x), sqr(a.y), sqr(a.z), sqr(a.w) };
    }
    INLINE vec4f sqrt(const vec4f& a) {
        return { sqrt(a.x), sqrt(a.y), sqrt(a.z), sqrt(a.w) };
    }
    INLINE vec4f exp(const vec4f& a) {
        return { exp(a.x), exp(a.y), exp(a.z), exp(a.w) };
    }
    INLINE vec4f log(const vec4f& a) {
        return { log(a.x), log(a.y), log(a.z), log(a.w) };
    }
    INLINE vec4f exp2(const vec4f& a) {
        return { exp2(a.x), exp2(a.y), exp2(a.z), exp2(a.w) };
    }
    INLINE vec4f log2(const vec4f& a) {
        return { log2(a.x), log2(a.y), log2(a.z), log2(a.w) };
    }
    INLINE vec4f pow(const vec4f& a, float b) {
        return { pow(a.x, b), pow(a.y, b), pow(a.z, b), pow(a.w, b) };
    }
    INLINE vec4f pow(const vec4f& a, const vec4f& b) {
        return { pow(a.x, b.x), pow(a.y, b.y), pow(a.z, b.z), pow(a.w, b.w) };
    }
    INLINE vec4f gain(const vec4f& a, float b) {
        return { gain(a.x, b), gain(a.y, b), gain(a.z, b), gain(a.w, b) };
    }
    INLINE bool isfinite(const vec4f& a) {
        return isfinite(a.x) && isfinite(a.y) && isfinite(a.z) && isfinite(a.w);
    }
    INLINE void swap(vec4f& a, vec4f& b) { std::swap(a, b); }
}

//===========================================
// IMPLEMENTATION - Quaternion
//===========================================
namespace waavs
{
    // Quaternion operatons represented as xi + yj + zk + w
    // const auto identity_quat4f = vec4f{0, 0, 0, 1};
    INLINE vec4f quat_mul(const vec4f& a, float b) {
        return { a.x * b, a.y * b, a.z * b, a.w * b };
    }
    INLINE vec4f quat_mul(const vec4f& a, const vec4f& b) {
        return { a.x * b.w + a.w * b.x + a.y * b.w - a.z * b.y,
            a.y * b.w + a.w * b.y + a.z * b.x - a.x * b.z,
            a.z * b.w + a.w * b.z + a.x * b.y - a.y * b.x,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z };
    }
    INLINE vec4f quat_conjugate(const vec4f& a) { return { -a.x, -a.y, -a.z, a.w }; }
    INLINE vec4f quat_inverse(const vec4f& a) {
        return quat_conjugate(a) / dot(a, a);
    }
}

//===========================================
// IMPLEMENTATION - Integer Vectors
//===========================================

namespace waavs
{
    // Vector data types
    INLINE int& vec2i::operator[](int i) { return (&x)[i]; }
    INLINE const int& vec2i::operator[](int i) const { return (&x)[i]; }

    // Vector data types
    INLINE int& vec3i::operator[](int i) { return (&x)[i]; }
    INLINE const int& vec3i::operator[](int i) const { return (&x)[i]; }

    // Vector data types
    INLINE int& vec4i::operator[](int i) { return (&x)[i]; }
    INLINE const int& vec4i::operator[](int i) const { return (&x)[i]; }

    // vec3b
    INLINE uint8_t& vec3b::operator[](int i) { return (&x)[i]; }
    INLINE const uint8_t& vec3b::operator[](int i) const { return (&x)[i]; }
    
    // Vector data types
    INLINE uint8_t & vec4b::operator[](int i) { return (&x)[i]; }
    INLINE const uint8_t& vec4b::operator[](int i) const { return (&x)[i]; }

    // Element access
    INLINE vec3i xyz(const vec4i& a) { return { a.x, a.y, a.z }; }

    // Vector sequence operations.
    INLINE int        size(const vec2i& ) { return 2; }
    INLINE const int* begin(const vec2i& a) { return &a.x; }
    INLINE const int* end(const vec2i& a) { return &a.x + 2; }
    INLINE int* begin(vec2i& a) { return &a.x; }
    INLINE int* end(vec2i& a) { return &a.x + 2; }
    INLINE const int* data(const vec2i& a) { return &a.x; }
    INLINE int* data(vec2i& a) { return &a.x; }

    // Vector comparison operations.
    INLINE bool operator==(const vec2i& a, const vec2i& b) {
        return a.x == b.x && a.y == b.y;
    }
    INLINE bool operator!=(const vec2i& a, const vec2i& b) {
        return a.x != b.x || a.y != b.y;
    }

    // Vector operations.
    INLINE vec2i operator+(const vec2i& a) { return a; }
    INLINE vec2i operator-(const vec2i& a) { return { -a.x, -a.y }; }
    INLINE vec2i operator+(const vec2i& a, const vec2i& b) {
        return { a.x + b.x, a.y + b.y };
    }
    INLINE vec2i operator+(const vec2i& a, int b) { return { a.x + b, a.y + b }; }
    INLINE vec2i operator+(int a, const vec2i& b) { return { a + b.x, a + b.y }; }
    INLINE vec2i operator-(const vec2i& a, const vec2i& b) {
        return { a.x - b.x, a.y - b.y };
    }
    INLINE vec2i operator-(const vec2i& a, int b) { return { a.x - b, a.y - b }; }
    INLINE vec2i operator-(int a, const vec2i& b) { return { a - b.x, a - b.y }; }
    INLINE vec2i operator*(const vec2i& a, const vec2i& b) {
        return { a.x * b.x, a.y * b.y };
    }
    INLINE vec2i operator*(const vec2i& a, int b) { return { a.x * b, a.y * b }; }
    INLINE vec2i operator*(int a, const vec2i& b) { return { a * b.x, a * b.y }; }
    INLINE vec2i operator/(const vec2i& a, const vec2i& b) {
        return { a.x / b.x, a.y / b.y };
    }
    INLINE vec2i operator/(const vec2i& a, int b) { return { a.x / b, a.y / b }; }
    INLINE vec2i operator/(int a, const vec2i& b) { return { a / b.x, a / b.y }; }

    // Vector assignments
    INLINE vec2i& operator+=(vec2i& a, const vec2i& b) { return a = a + b; }
    INLINE vec2i& operator+=(vec2i& a, int b) { return a = a + b; }
    INLINE vec2i& operator-=(vec2i& a, const vec2i& b) { return a = a - b; }
    INLINE vec2i& operator-=(vec2i& a, int b) { return a = a - b; }
    INLINE vec2i& operator*=(vec2i& a, const vec2i& b) { return a = a * b; }
    INLINE vec2i& operator*=(vec2i& a, int b) { return a = a * b; }
    INLINE vec2i& operator/=(vec2i& a, const vec2i& b) { return a = a / b; }
    INLINE vec2i& operator/=(vec2i& a, int b) { return a = a / b; }

    // Max element and clamp.
    INLINE vec2i max(const vec2i& a, int b) { return { max(a.x, b), max(a.y, b) }; }
    INLINE vec2i min(const vec2i& a, int b) { return { min(a.x, b), min(a.y, b) }; }
    INLINE vec2i max(const vec2i& a, const vec2i& b) {
        return { max(a.x, b.x), max(a.y, b.y) };
    }
    INLINE vec2i min(const vec2i& a, const vec2i& b) {
        return { min(a.x, b.x), min(a.y, b.y) };
    }
    INLINE vec2i clamp(const vec2i& x, int min, int max) {
        return { clamp(x.x, min, max), clamp(x.y, min, max) };
    }

    INLINE int max(const vec2i& a) { return max(a.x, a.y); }
    INLINE int min(const vec2i& a) { return min(a.x, a.y); }
    INLINE int sum(const vec2i& a) { return a.x + a.y; }

    // Functions applied to vector elements
    INLINE vec2i abs(const vec2i& a) { return { abs(a.x), abs(a.y) }; }
    INLINE void  swap(vec2i& a, vec2i& b) { std::swap(a, b); }

    // Vector sequence operations.
    INLINE int        size(const vec3i& ) { return 3; }
    INLINE const int* begin(const vec3i& a) { return &a.x; }
    INLINE const int* end(const vec3i& a) { return &a.x + 3; }
    INLINE int* begin(vec3i& a) { return &a.x; }
    INLINE int* end(vec3i& a) { return &a.x + 3; }
    INLINE const int* data(const vec3i& a) { return &a.x; }
    INLINE int* data(vec3i& a) { return &a.x; }

    // Vector comparison operations.
    INLINE bool operator==(const vec3i& a, const vec3i& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    INLINE bool operator!=(const vec3i& a, const vec3i& b) {
        return a.x != b.x || a.y != b.y || a.z != b.z;
    }

    // Vector operations.
    INLINE vec3i operator+(const vec3i& a) { return a; }
    INLINE vec3i operator-(const vec3i& a) { return { -a.x, -a.y, -a.z }; }
    INLINE vec3i operator+(const vec3i& a, const vec3i& b) {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }
    INLINE vec3i operator+(const vec3i& a, int b) {
        return { a.x + b, a.y + b, a.z + b };
    }
    INLINE vec3i operator+(int a, const vec3i& b) {
        return { a + b.x, a + b.y, a + b.z };
    }
    INLINE vec3i operator-(const vec3i& a, const vec3i& b) {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }
    INLINE vec3i operator-(const vec3i& a, int b) {
        return { a.x - b, a.y - b, a.z - b };
    }
    INLINE vec3i operator-(int a, const vec3i& b) {
        return { a - b.x, a - b.y, a - b.z };
    }
    INLINE vec3i operator*(const vec3i& a, const vec3i& b) {
        return { a.x * b.x, a.y * b.y, a.z * b.z };
    }
    INLINE vec3i operator*(const vec3i& a, int b) {
        return { a.x * b, a.y * b, a.z * b };
    }
    INLINE vec3i operator*(int a, const vec3i& b) {
        return { a * b.x, a * b.y, a * b.z };
    }
    INLINE vec3i operator/(const vec3i& a, const vec3i& b) {
        return { a.x / b.x, a.y / b.y, a.z / b.z };
    }
    INLINE vec3i operator/(const vec3i& a, int b) {
        return { a.x / b, a.y / b, a.z / b };
    }
    INLINE vec3i operator/(int a, const vec3i& b) {
        return { a / b.x, a / b.y, a / b.z };
    }

    // Vector assignments
    INLINE vec3i& operator+=(vec3i& a, const vec3i& b) { return a = a + b; }
    INLINE vec3i& operator+=(vec3i& a, int b) { return a = a + b; }
    INLINE vec3i& operator-=(vec3i& a, const vec3i& b) { return a = a - b; }
    INLINE vec3i& operator-=(vec3i& a, int b) { return a = a - b; }
    INLINE vec3i& operator*=(vec3i& a, const vec3i& b) { return a = a * b; }
    INLINE vec3i& operator*=(vec3i& a, int b) { return a = a * b; }
    INLINE vec3i& operator/=(vec3i& a, const vec3i& b) { return a = a / b; }
    INLINE vec3i& operator/=(vec3i& a, int b) { return a = a / b; }

    // Max element and clamp.
    INLINE vec3i max(const vec3i& a, int b) {
        return { max(a.x, b), max(a.y, b), max(a.z, b) };
    }
    INLINE vec3i min(const vec3i& a, int b) {
        return { min(a.x, b), min(a.y, b), min(a.z, b) };
    }
    INLINE vec3i max(const vec3i& a, const vec3i& b) {
        return { max(a.x, b.x), max(a.y, b.y), max(a.z, b.z) };
    }
    INLINE vec3i min(const vec3i& a, const vec3i& b) {
        return { min(a.x, b.x), min(a.y, b.y), min(a.z, b.z) };
    }
    INLINE vec3i clamp(const vec3i& x, int min, int max) {
        return { clamp(x.x, min, max), clamp(x.y, min, max), clamp(x.z, min, max) };
    }

    INLINE int max(const vec3i& a) { return max(max(a.x, a.y), a.z); }
    INLINE int min(const vec3i& a) { return min(min(a.x, a.y), a.z); }
    INLINE int sum(const vec3i& a) { return a.x + a.y + a.z; }

    // Functions applied to vector elements
    INLINE vec3i abs(const vec3i& a) { return { abs(a.x), abs(a.y), abs(a.z) }; }
    INLINE void  swap(vec3i& a, vec3i& b) { std::swap(a, b); }

    // Vector sequence operations.
    INLINE int        size(const vec3b& ) { return 4; }
    INLINE const uint8_t* begin(const vec3b& a) { return &a.x; }
    INLINE const uint8_t* end(const vec3b& a) { return &a.x + 4; }
    INLINE uint8_t* begin(vec3b& a) { return &a.x; }
    INLINE uint8_t* end(vec3b& a) { return &a.x + 4; }
    INLINE const uint8_t* data(const vec3b& a) { return &a.x; }
    INLINE uint8_t* data(vec3b& a) { return &a.x; }
    
    // Vector sequence operations.
    INLINE int        size(const vec4b& ) { return 4; }
    INLINE const uint8_t* begin(const vec4b& a) { return &a.x; }
    INLINE const uint8_t* end(const vec4b& a) { return &a.x + 4; }
    INLINE uint8_t* begin(vec4b& a) { return &a.x; }
    INLINE uint8_t* end(vec4b& a) { return &a.x + 4; }
    INLINE const uint8_t* data(const vec4b& a) { return &a.x; }
    INLINE uint8_t* data(vec4b& a) { return &a.x; }



    // Vector sequence operations.
    INLINE int        size(const vec4i& ) { return 4; }
    INLINE const int* begin(const vec4i& a) { return &a.x; }
    INLINE const int* end(const vec4i& a) { return &a.x + 4; }
    INLINE int* begin(vec4i& a) { return &a.x; }
    INLINE int* end(vec4i& a) { return &a.x + 4; }
    INLINE const int* data(const vec4i& a) { return &a.x; }
    INLINE int* data(vec4i& a) { return &a.x; }

    // Vector comparison operations.
    INLINE bool operator==(const vec4i& a, const vec4i& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
    }
    INLINE bool operator!=(const vec4i& a, const vec4i& b) {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
    }

    // Vector operations.
    INLINE vec4i operator+(const vec4i& a) { return a; }
    INLINE vec4i operator-(const vec4i& a) { return { -a.x, -a.y, -a.z, -a.w }; }
    INLINE vec4i operator+(const vec4i& a, const vec4i& b) {
        return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    }
    INLINE vec4i operator+(const vec4i& a, int b) {
        return { a.x + b, a.y + b, a.z + b, a.w + b };
    }
    INLINE vec4i operator+(int a, const vec4i& b) {
        return { a + b.x, a + b.y, a + b.z, a + b.w };
    }
    INLINE vec4i operator-(const vec4i& a, const vec4i& b) {
        return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
    }
    INLINE vec4i operator-(const vec4i& a, int b) {
        return { a.x - b, a.y - b, a.z - b, a.w - b };
    }
    INLINE vec4i operator-(int a, const vec4i& b) {
        return { a - b.x, a - b.y, a - b.z, a - b.w };
    }
    INLINE vec4i operator*(const vec4i& a, const vec4i& b) {
        return { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
    }
    INLINE vec4i operator*(const vec4i& a, int b) {
        return { a.x * b, a.y * b, a.z * b, a.w * b };
    }
    INLINE vec4i operator*(int a, const vec4i& b) {
        return { a * b.x, a * b.y, a * b.z, a * b.w };
    }
    INLINE vec4i operator/(const vec4i& a, const vec4i& b) {
        return { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
    }
    INLINE vec4i operator/(const vec4i& a, int b) {
        return { a.x / b, a.y / b, a.z / b, a.w / b };
    }
    INLINE vec4i operator/(int a, const vec4i& b) {
        return { a / b.x, a / b.y, a / b.z, a / b.w };
    }

    // Vector assignments
    INLINE vec4i& operator+=(vec4i& a, const vec4i& b) { return a = a + b; }
    INLINE vec4i& operator+=(vec4i& a, int b) { return a = a + b; }
    INLINE vec4i& operator-=(vec4i& a, const vec4i& b) { return a = a - b; }
    INLINE vec4i& operator-=(vec4i& a, int b) { return a = a - b; }
    INLINE vec4i& operator*=(vec4i& a, const vec4i& b) { return a = a * b; }
    INLINE vec4i& operator*=(vec4i& a, int b) { return a = a * b; }
    INLINE vec4i& operator/=(vec4i& a, const vec4i& b) { return a = a / b; }
    INLINE vec4i& operator/=(vec4i& a, int b) { return a = a / b; }

    // Max element and clamp.
    INLINE vec4i max(const vec4i& a, int b) {
        return { max(a.x, b), max(a.y, b), max(a.z, b), max(a.w, b) };
    }
    INLINE vec4i min(const vec4i& a, int b) {
        return { min(a.x, b), min(a.y, b), min(a.z, b), min(a.w, b) };
    }
    INLINE vec4i max(const vec4i& a, const vec4i& b) {
        return { max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w) };
    }
    INLINE vec4i min(const vec4i& a, const vec4i& b) {
        return { min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w) };
    }
    INLINE vec4i clamp(const vec4i& x, int min, int max) {
        return { clamp(x.x, min, max), clamp(x.y, min, max), clamp(x.z, min, max),
            clamp(x.w, min, max) };
    }

    INLINE int max(const vec4i& a) { return max(max(max(a.x, a.y), a.z), a.w); }
    INLINE int min(const vec4i& a) { return min(min(min(a.x, a.y), a.z), a.w); }
    INLINE int sum(const vec4i& a) { return a.x + a.y + a.z + a.w; }

    // Functions applied to vector elements
    INLINE vec4i abs(const vec4i& a) {
        return { abs(a.x), abs(a.y), abs(a.z), abs(a.w) };
    }
    INLINE void swap(vec4i& a, vec4i& b) { std::swap(a, b); }

    // vec3b comparison operations.
    INLINE bool operator==(const vec3b& a, const vec3b& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z ;
    }
    INLINE bool operator!=(const vec3b& a, const vec3b& b) {
        return a.x != b.x || a.y != b.y || a.z != b.z ;
    }
    
    // Vector comparison operations.
    INLINE bool operator==(const vec4b& a, const vec4b& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
    }
    INLINE bool operator!=(const vec4b& a, const vec4b& b) {
        return a.x != b.x || a.y != b.y || a.z != b.z || a.w != b.w;
    }
}


//==============================================
// IMPLEMENTATION - Matrices
//==============================================
namespace waavs
{
    // Small Fixed-size matrices stored in column major format.
    INLINE vec2f& mat2f::operator[](int i) { return (&x)[i]; }
    INLINE const vec2f& mat2f::operator[](int i) const { return (&x)[i]; }

    // Small Fixed-size matrices stored in column major format.
    INLINE vec3f& mat3f::operator[](int i) { return (&x)[i]; }
    INLINE const vec3f& mat3f::operator[](int i) const { return (&x)[i]; }

    // Small Fixed-size matrices stored in column major format.
    INLINE vec4f& mat4f::operator[](int i) { return (&x)[i]; }
    INLINE const vec4f& mat4f::operator[](int i) const { return (&x)[i]; }

    // Matrix comparisons.
    INLINE bool operator==(const mat2f& a, const mat2f& b) {
        return a.x == b.x && a.y == b.y;
    }
    INLINE bool operator!=(const mat2f& a, const mat2f& b) { return !(a == b); }

    // Matrix operations.
    INLINE mat2f operator+(const mat2f& a, const mat2f& b) {
        return { a.x + b.x, a.y + b.y };
    }
    INLINE mat2f operator*(const mat2f& a, float b) { return { a.x * b, a.y * b }; }
    INLINE vec2f operator*(const mat2f& a, const vec2f& b) {
        return a.x * b.x + a.y * b.y;
    }
    INLINE vec2f operator*(const vec2f& a, const mat2f& b) {
        return { dot(a, b.x), dot(a, b.y) };
    }
    INLINE mat2f operator*(const mat2f& a, const mat2f& b) {
        return { a * b.x, a * b.y };
    }

    // Matrix assignments.
    INLINE mat2f& operator+=(mat2f& a, const mat2f& b) { return a = a + b; }
    INLINE mat2f& operator*=(mat2f& a, const mat2f& b) { return a = a * b; }
    INLINE mat2f& operator*=(mat2f& a, float b) { return a = a * b; }

    // Matrix diagonals and transposes.
    INLINE vec2f diagonal(const mat2f& a) { return { a.x.x, a.y.y }; }
    INLINE mat2f transpose(const mat2f& a) {
        return { {a.x.x, a.y.x}, {a.x.y, a.y.y} };
    }

    // Matrix adjoints, determinants and inverses.
    INLINE float determinant(const mat2f& a) { return cross(a.x, a.y); }
    INLINE mat2f adjoint(const mat2f& a) {
        return { {a.y.y, -a.x.y}, {-a.y.x, a.x.x} };
    }
    INLINE mat2f inverse(const mat2f& a) {
        return adjoint(a) * (1 / determinant(a));
    }

    // Matrix comparisons.
    INLINE bool operator==(const mat3f& a, const mat3f& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    INLINE bool operator!=(const mat3f& a, const mat3f& b) { return !(a == b); }

    // Matrix operations.
    INLINE mat3f operator+(const mat3f& a, const mat3f& b) {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }
    INLINE mat3f operator*(const mat3f& a, float b) {
        return { a.x * b, a.y * b, a.z * b };
    }
    INLINE vec3f operator*(const mat3f& a, const vec3f& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    INLINE vec3f operator*(const vec3f& a, const mat3f& b) {
        return { dot(a, b.x), dot(a, b.y), dot(a, b.z) };
    }
    INLINE mat3f operator*(const mat3f& a, const mat3f& b) {
        return { a * b.x, a * b.y, a * b.z };
    }

    // Matrix assignments.
    INLINE mat3f& operator+=(mat3f& a, const mat3f& b) { return a = a + b; }
    INLINE mat3f& operator*=(mat3f& a, const mat3f& b) { return a = a * b; }
    INLINE mat3f& operator*=(mat3f& a, float b) { return a = a * b; }

    // Matrix diagonals and transposes.
    INLINE vec3f diagonal(const mat3f& a) { return { a.x.x, a.y.y, a.z.z }; }
    INLINE mat3f transpose(const mat3f& a) {
        return {
            {a.x.x, a.y.x, a.z.x},
            {a.x.y, a.y.y, a.z.y},
            {a.x.z, a.y.z, a.z.z},
        };
    }

    // Matrix adjoints, determinants and inverses.
    INLINE float determinant(const mat3f& a) { return dot(a.x, cross(a.y, a.z)); }
    INLINE mat3f adjoint(const mat3f& a) {
        return transpose(mat3f{ cross(a.y, a.z), cross(a.z, a.x), cross(a.x, a.y) });
    }
    INLINE mat3f inverse(const mat3f& a) {
        return adjoint(a) * (1 / determinant(a));
    }

    // Constructs a basis from a direction
    INLINE mat3f basis_fromz(const vec3f& v) {
        // https://graphics.pixar.com/library/OrthonormalB/paper.pdf
        auto z = normalize(v);
        auto sign = copysignf(1.0f, z.z);
        auto a = -1.0f / (sign + z.z);
        auto b = z.x * z.y * a;
        auto x = vec3f{ 1.0f + sign * z.x * z.x * a, sign * b, -sign * z.x };
        auto y = vec3f{ b, sign + z.y * z.y * a, -z.y };
        return { x, y, z };
    }

    // Matrix sequencing
    INLINE int          size(const mat4f& ) { return 16; }
    INLINE const float* begin(const mat4f& a) { return begin(a.x); }
    INLINE const float* end(const mat4f& a) { return end(a.x); }

    // Matrix comparisons.
    INLINE bool operator==(const mat4f& a, const mat4f& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
    }
    INLINE bool operator!=(const mat4f& a, const mat4f& b) { return !(a == b); }

    // Matrix operations.
    INLINE mat4f operator+(const mat4f& a, const mat4f& b) {
        return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    }
    INLINE mat4f operator*(const mat4f& a, float b) {
        return { a.x * b, a.y * b, a.z * b, a.w * b };
    }
    INLINE vec4f operator*(const mat4f& a, const vec4f& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }
    INLINE vec4f operator*(const vec4f& a, const mat4f& b) {
        return { dot(a, b.x), dot(a, b.y), dot(a, b.z), dot(a, b.w) };
    }
    INLINE mat4f operator*(const mat4f& a, const mat4f& b) {
        return { a * b.x, a * b.y, a * b.z, a * b.w };
    }

    // Matrix assignments.
    INLINE mat4f& operator+=(mat4f& a, const mat4f& b) { return a = a + b; }
    INLINE mat4f& operator*=(mat4f& a, const mat4f& b) { return a = a * b; }
    INLINE mat4f& operator*=(mat4f& a, float b) { return a = a * b; }

    // Matrix diagonals and transposes.
    INLINE vec4f diagonal(const mat4f& a) { return { a.x.x, a.y.y, a.z.z, a.w.w }; }
    INLINE mat4f transpose(const mat4f& a) {
        return {
            {a.x.x, a.y.x, a.z.x, a.w.x},
            {a.x.y, a.y.y, a.z.y, a.w.y},
            {a.x.z, a.y.z, a.z.z, a.w.z},
            {a.x.w, a.y.w, a.z.w, a.w.w},
        };
    }
}


//================================================
// IMPLEMENTATION - RIGID BODY TRANSFORMS/FRAMES
//================================================

namespace waavs
{
    // Rigid frames stored as a column-major affine transform matrix.
    INLINE vec2f& frame2f::operator[](int i) { return (&x)[i]; }
    INLINE const vec2f& frame2f::operator[](int i) const { return (&x)[i]; }

    // Rigid frames stored as a column-major affine transform matrix.
    INLINE vec3f& frame3f::operator[](int i) { return (&x)[i]; }
    INLINE const vec3f& frame3f::operator[](int i) const { return (&x)[i]; }

    // Frame properties
    INLINE mat2f rotation(const frame2f& a) { return { a.x, a.y }; }
    INLINE vec2f translation(const frame2f& a) { return a.o; }

    // Frame construction
    INLINE frame2f make_frame(const mat2f& m, const vec2f& t) {
        return { m.x, m.y, t };
    }

    // Frame/mat conversion
    INLINE frame2f mat_to_frame(const mat3f& m) {
        return { {m.x.x, m.x.y}, {m.y.x, m.y.y}, {m.z.x, m.z.y} };
    }
    INLINE mat3f frame_to_mat(const frame2f& f) {
        return { {f.x.x, f.x.y, 0}, {f.y.x, f.y.y, 0}, {f.o.x, f.o.y, 1} };
    }

    // Frame comparisons.
    INLINE bool operator==(const frame2f& a, const frame2f& b) {
        return a.x == b.x && a.y == b.y && a.o == b.o;
    }
    INLINE bool operator!=(const frame2f& a, const frame2f& b) { return !(a == b); }

    // Frame composition, equivalent to affine matrix product.
    INLINE frame2f operator*(const frame2f& a, const frame2f& b) {
        return make_frame(rotation(a) * rotation(b), rotation(a) * b.o + a.o);
    }
    INLINE frame2f& operator*=(frame2f& a, const frame2f& b) { return a = a * b; }

    // Frame inverse, equivalent to rigid affine inverse.
    INLINE frame2f inverse(const frame2f& a, bool non_rigid) {
        if (non_rigid) {
            auto minv = inverse(rotation(a));
            return make_frame(minv, -(minv * a.o));
        }
        else {
            auto minv = transpose(rotation(a));
            return make_frame(minv, -(minv * a.o));
        }
    }

    // Frame properties
    INLINE mat3f rotation(const frame3f& a) { return { a.x, a.y, a.z }; }
    INLINE vec3f translation(const frame3f& a) { return a.o; }

    // Frame construction
    INLINE frame3f make_frame(const mat3f& m, const vec3f& t) {
        return { m.x, m.y, m.z, t };
    }

    // frame/mat conversion
    INLINE frame3f mat_to_frame(const mat4f& m) {
        return { {m.x.x, m.x.y, m.x.z}, {m.y.x, m.y.y, m.y.z}, {m.z.x, m.z.y, m.z.z},
            {m.w.x, m.w.y, m.w.z} };
    }
    INLINE mat4f frame_to_mat(const frame3f& f) {
        return { {f.x.x, f.x.y, f.x.z, 0}, {f.y.x, f.y.y, f.y.z, 0},
            {f.z.x, f.z.y, f.z.z, 0}, {f.o.x, f.o.y, f.o.z, 1} };
    }

    // Frame comparisons.
    INLINE bool operator==(const frame3f& a, const frame3f& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.o == b.o;
    }
    INLINE bool operator!=(const frame3f& a, const frame3f& b) { return !(a == b); }

    // Frame composition, equivalent to affine matrix product.
    INLINE frame3f operator*(const frame3f& a, const frame3f& b) {
        return make_frame(rotation(a) * rotation(b), rotation(a) * b.o + a.o);
    }
    INLINE frame3f& operator*=(frame3f& a, const frame3f& b) { return a = a * b; }

    // Frame inverse, equivalent to rigid affine inverse.
    INLINE frame3f inverse(const frame3f& a, bool non_rigid) {
        if (non_rigid) {
            auto minv = inverse(rotation(a));
            return make_frame(minv, -(minv * a.o));
        }
        else {
            auto minv = transpose(rotation(a));
            return make_frame(minv, -(minv * a.o));
        }
    }

    // Frame construction from axis.
    INLINE frame3f frame_fromz(const vec3f& o, const vec3f& v) {
        // https://graphics.pixar.com/library/OrthonormalB/paper.pdf
        auto z = normalize(v);
        auto sign = copysignf(1.0f, z.z);
        auto a = -1.0f / (sign + z.z);
        auto b = z.x * z.y * a;
        auto x = vec3f{ 1.0f + sign * z.x * z.x * a, sign * b, -sign * z.x };
        auto y = vec3f{ b, sign + z.y * z.y * a, -z.y };
        return { x, y, z, o };
    }
    INLINE frame3f frame_fromzx(const vec3f& o, const vec3f& z_, const vec3f& x_) {
        auto z = normalize(z_);
        auto x = orthonormalize(x_, z);
        auto y = normalize(cross(z, x));
        return { x, y, z, o };
    }
}

//===========================================
// IMPLEMENTATION - QUATERNIONS
//===========================================
namespace waavs
{
    // Quaternion operatons
    INLINE quat4f operator+(const quat4f& a, const quat4f& b) {
        return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
    }
    INLINE quat4f operator*(const quat4f& a, float b) {
        return { a.x * b, a.y * b, a.z * b, a.w * b };
    }
    INLINE quat4f operator/(const quat4f& a, float b) {
        return { a.x / b, a.y / b, a.z / b, a.w / b };
    }
    INLINE quat4f operator*(const quat4f& a, const quat4f& b) {
        return { a.x * b.w + a.w * b.x + a.y * b.w - a.z * b.y,
            a.y * b.w + a.w * b.y + a.z * b.x - a.x * b.z,
            a.z * b.w + a.w * b.z + a.x * b.y - a.y * b.x,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z };
    }

    // Quaterion operations
    INLINE float dot(const quat4f& a, const quat4f& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    }
    INLINE float  length(const quat4f& a) { return sqrt(dot(a, a)); }
    INLINE quat4f normalize(const quat4f& a) {
        auto l = length(a);
        return (l != 0) ? a / l : a;
    }
    INLINE quat4f conjugate(const quat4f& a) { return { -a.x, -a.y, -a.z, a.w }; }
    INLINE quat4f inverse(const quat4f& a) { return conjugate(a) / dot(a, a); }
    INLINE float  uangle(const quat4f& a, const quat4f& b) {
        auto d = dot(a, b);
        return d > 1 ? 0 : acos(d < -1 ? -1 : d);
    }
    INLINE quat4f lerp(const quat4f& a, const quat4f& b, float t) {
        return a * (1 - t) + b * t;
    }
    INLINE quat4f nlerp(const quat4f& a, const quat4f& b, float t) {
        return normalize(lerp(a, b, t));
    }
    INLINE quat4f slerp(const quat4f& a, const quat4f& b, float t) {
        auto th = uangle(a, b);
        return th == 0
            ? a
            : a * (sin(th * (1 - t)) / sin(th)) + b * (sin(th * t) / sin(th));
    }
}

//===========================================
// IMPLEMENTATION - TRANSFORMS
//===========================================

namespace waavs
{
    // Transforms points, vectors and directions by matrices.
    INLINE vec2f transform_point(const mat3f& a, const vec2f& b) {
        auto tvb = a * vec3f{ b.x, b.y, 1 };
        return vec2f{ tvb.x, tvb.y } / tvb.z;
    }
    INLINE vec2f transform_vector(const mat3f& a, const vec2f& b) {
        auto tvb = a * vec3f{ b.x, b.y, 0 };
        return vec2f{ tvb.x, tvb.y } / tvb.z;
    }
    INLINE vec2f transform_direction(const mat3f& a, const vec2f& b) {
        return normalize(transform_vector(a, b));
    }
    INLINE vec2f transform_normal(const mat3f& a, const vec2f& b) {
        return normalize(transform_vector(transpose(inverse(a)), b));
    }
    INLINE vec2f transform_vector(const mat2f& a, const vec2f& b) { return a * b; }
    INLINE vec2f transform_direction(const mat2f& a, const vec2f& b) {
        return normalize(transform_vector(a, b));
    }
    INLINE vec2f transform_normal(const mat2f& a, const vec2f& b) {
        return normalize(transform_vector(transpose(inverse(a)), b));
    }

    INLINE vec3f transform_point(const mat4f& a, const vec3f& b) {
        auto tvb = a * vec4f{ b.x, b.y, b.z, 1 };
        return vec3f{ tvb.x, tvb.y, tvb.z } / tvb.w;
    }
    INLINE vec3f transform_vector(const mat4f& a, const vec3f& b) {
        auto tvb = a * vec4f{ b.x, b.y, b.z, 0 };
        return vec3f{ tvb.x, tvb.y, tvb.z };
    }
    INLINE vec3f transform_direction(const mat4f& a, const vec3f& b) {
        return normalize(transform_vector(a, b));
    }
    INLINE vec3f transform_vector(const mat3f& a, const vec3f& b) { return a * b; }
    INLINE vec3f transform_direction(const mat3f& a, const vec3f& b) {
        return normalize(transform_vector(a, b));
    }
    INLINE vec3f transform_normal(const mat3f& a, const vec3f& b) {
        return normalize(transform_vector(transpose(inverse(a)), b));
    }

    // Transforms points, vectors and directions by frames.
    INLINE vec2f transform_point(const frame2f& a, const vec2f& b) {
        return a.x * b.x + a.y * b.y + a.o;
    }
    INLINE vec2f transform_vector(const frame2f& a, const vec2f& b) {
        return a.x * b.x + a.y * b.y;
    }
    INLINE vec2f transform_direction(const frame2f& a, const vec2f& b) {
        return normalize(transform_vector(a, b));
    }
    INLINE vec2f transform_normal(
        const frame2f& a, const vec2f& b, bool non_rigid) {
        if (non_rigid) {
            return transform_normal(rotation(a), b);
        }
        else {
            return normalize(transform_vector(a, b));
        }
    }

    // Transforms points, vectors and directions by frames.
    INLINE vec3f transform_point(const frame3f& a, const vec3f& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z + a.o;
    }
    INLINE vec3f transform_vector(const frame3f& a, const vec3f& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    INLINE vec3f transform_direction(const frame3f& a, const vec3f& b) {
        return normalize(transform_vector(a, b));
    }
    INLINE vec3f transform_normal(
        const frame3f& a, const vec3f& b, bool non_rigid) {
        if (non_rigid) {
            return transform_normal(rotation(a), b);
        }
        else {
            return normalize(transform_vector(a, b));
        }
    }

    // Transforms points, vectors and directions by frames.
    INLINE vec2f transform_point_inverse(const frame2f& a, const vec2f& b) {
        return { dot(a.x, b - a.o), dot(a.y, b - a.o) };
    }
    INLINE vec2f transform_vector_inverse(const frame2f& a, const vec2f& b) {
        return { dot(a.x, b), dot(a.y, b) };
    }
    INLINE vec2f transform_direction_inverse(const frame2f& a, const vec2f& b) {
        return normalize(transform_vector_inverse(a, b));
    }

    // Transforms points, vectors and directions by frames.
    INLINE vec3f transform_point_inverse(const frame3f& a, const vec3f& b) {
        return { dot(a.x, b - a.o), dot(a.y, b - a.o), dot(a.z, b - a.o) };
    }
    INLINE vec3f transform_vector_inverse(const frame3f& a, const vec3f& b) {
        return { dot(a.x, b), dot(a.y, b), dot(a.z, b) };
    }
    INLINE vec3f transform_direction_inverse(const frame3f& a, const vec3f& b) {
        return normalize(transform_vector_inverse(a, b));
    }

    // Translation, scaling and rotations transforms.
    INLINE frame3f translation_frame(const vec3f& a) {
        return { {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, a };
    }
    INLINE frame3f scaling_frame(const vec3f& a) {
        return { {a.x, 0, 0}, {0, a.y, 0}, {0, 0, a.z}, {0, 0, 0} };
    }
    INLINE frame3f rotation_frame(const vec3f& axis, float angle) {
        auto s = sin(angle), c = cos(angle);
        auto vv = normalize(axis);
        return { {c + (1 - c) * vv.x * vv.x, (1 - c) * vv.x * vv.y + s * vv.z,
                    (1 - c) * vv.x * vv.z - s * vv.y},
            {(1 - c) * vv.x * vv.y - s * vv.z, c + (1 - c) * vv.y * vv.y,
                (1 - c) * vv.y * vv.z + s * vv.x},
            {(1 - c) * vv.x * vv.z + s * vv.y, (1 - c) * vv.y * vv.z - s * vv.x,
                c + (1 - c) * vv.z * vv.z},
            {0, 0, 0} };
    }
    INLINE frame3f rotation_frame(const vec4f& quat) {
        vec4f v = quat;
        return { {v.w * v.w + v.x * v.x - v.y * v.y - v.z * v.z,
                    (v.x * v.y + v.z * v.w) * 2, (v.z * v.x - v.y * v.w) * 2},
            {(v.x * v.y - v.z * v.w) * 2,
                v.w * v.w - v.x * v.x + v.y * v.y - v.z * v.z,
                (v.y * v.z + v.x * v.w) * 2},
            {(v.z * v.x + v.y * v.w) * 2, (v.y * v.z - v.x * v.w) * 2,
                v.w * v.w - v.x * v.x - v.y * v.y + v.z * v.z},
            {0, 0, 0} };
    }
    INLINE frame3f rotation_frame(const quat4f& quat) {
        quat4f v = quat;
        return { {v.w * v.w + v.x * v.x - v.y * v.y - v.z * v.z,
                    (v.x * v.y + v.z * v.w) * 2, (v.z * v.x - v.y * v.w) * 2},
            {(v.x * v.y - v.z * v.w) * 2,
                v.w * v.w - v.x * v.x + v.y * v.y - v.z * v.z,
                (v.y * v.z + v.x * v.w) * 2},
            {(v.z * v.x + v.y * v.w) * 2, (v.y * v.z - v.x * v.w) * 2,
                v.w * v.w - v.x * v.x - v.y * v.y + v.z * v.z},
            {0, 0, 0} };
    }
    INLINE frame3f rotation_frame(const mat3f& rot) {
        return { rot.x, rot.y, rot.z, {0, 0, 0} };
    }

    // Lookat frame. Z-axis can be inverted with inv_xz.
    INLINE frame3f lookat_frame(
        const vec3f& eye, const vec3f& center, const vec3f& up, bool inv_xz) {
        auto w = normalize(eye - center);
        auto u = normalize(cross(up, w));
        auto v = normalize(cross(w, u));
        if (inv_xz) {
            w = -w;
            u = -u;
        }
        return { u, v, w, eye };
    }

    // OpenGL frustum, ortho and perspecgive matrices.
    INLINE mat4f frustum_mat(float l, float r, float b, float t, float n, float f) {
        return { {2 * n / (r - l), 0, 0, 0}, {0, 2 * n / (t - b), 0, 0},
            {(r + l) / (r - l), (t + b) / (t - b), -(f + n) / (f - n), -1},
            {0, 0, -2 * f * n / (f - n), 0} };
    }
    INLINE mat4f ortho_mat(float l, float r, float b, float t, float n, float f) {
        return { {2 / (r - l), 0, 0, 0}, {0, 2 / (t - b), 0, 0},
            {0, 0, -2 / (f - n), 0},
            {-(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 1} };
    }
    INLINE mat4f ortho2d_mat(float left, float right, float bottom, float top) {
        return ortho_mat(left, right, bottom, top, -1, 1);
    }
    INLINE mat4f ortho_mat(float xmag, float ymag, float nearest, float farthest) {
        return { {1 / xmag, 0, 0, 0}, {0, 1 / ymag, 0, 0}, {0, 0, 2 / (nearest - farthest), 0},
            {0, 0, (farthest + nearest) / (nearest - farthest), 1} };
    }
    INLINE mat4f perspective_mat(float fovy, float aspect, float nearest, float farthest) {
        auto tg = tan(fovy / 2);
        return { {1 / (aspect * tg), 0, 0, 0}, {0, 1 / tg, 0, 0},
            {0, 0, (farthest + nearest) / (nearest - farthest), -1},
            {0, 0, 2 * farthest * nearest / (nearest - farthest), 0} };
    }
    INLINE mat4f perspective_mat(float fovy, float aspect, float nearest) {
        auto tg = tan(fovy / 2);
        return { {1 / (aspect * tg), 0, 0, 0}, {0, 1 / tg, 0, 0}, {0, 0, -1, -1},
            {0, 0, 2 * nearest, 0} };
    }

    // Rotation conversions.
    INLINE pair<vec3f, float> rotation_axisangle(const vec4f& quat) {
        return { normalize(vec3f{quat.x, quat.y, quat.z}), 2 * acos(quat.w) };
    }
    INLINE vec4f rotation_quat(const vec3f& axis, float angle) {
        auto len = length(axis);
        if (len == 0) return { 0, 0, 0, 1 };
        return vec4f{ sin(angle / 2) * axis.x / len, sin(angle / 2) * axis.y / len,
            sin(angle / 2) * axis.z / len, cos(angle / 2) };
    }
    INLINE vec4f rotation_quat(const vec4f& axisangle) {
        return rotation_quat(
            vec3f{ axisangle.x, axisangle.y, axisangle.z }, axisangle.w);
    }
}

namespace waavs
{
    // Python `range()` equivalent. Construct an object to iterate over a sequence.
    template <typename T>
    constexpr auto range(T max) { return range((T)0, max); }

    template <typename T>
    constexpr auto range(T min, T max) {
        struct range_iterator {
            T    index;
            void operator++() { ++index; }
            bool operator!=(const range_iterator& other) const {
                return index != other.index;
            }
            T operator*() const { return index; }
        };
        struct range_helper {
            T              begin_ = 0, end_ = 0;
            range_iterator begin() const { return { begin_ }; }
            range_iterator end() const { return { end_ }; }
        };
        return range_helper{ min, max };
    }

    template <typename T>
    constexpr auto range(T min, T max, T step) {
        struct range_iterator {
            T    index;
            T    step;
            void operator++() { index += step; }
            bool operator!=(const range_iterator& other) const {
                return index != other.index;
            }
            T operator*() const { return index; }
        };
        struct range_helper {
            T              begin_ = 0, end_ = 0, step_ = 0;
            range_iterator begin() const { return { begin_, step_ }; }
            range_iterator end() const {
                return { begin_ + ((end_ - begin_) / step_) * step_, step_ };
            }
        };
        return range_helper{ min, max, step };
    }
}