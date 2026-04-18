// maths_transcend.h - transcendental functions

#pragma once

#include <cmath>

#include "definitions.h"


namespace waavs
{
    INLINE float COS(float a) noexcept { return std::cos(a); }
    INLINE float ACOS(float a) noexcept { return std::acos(a); }
    INLINE float SIN(float a) noexcept { return std::sin(a); }
    INLINE float ASIN(float a) noexcept { return std::asin(a); }
    INLINE float TAN(float a) noexcept { return std::tan(a); }
    INLINE float ATAN(float a) noexcept { return std::atan(a); }
    INLINE float ATAN2(float y, float x) noexcept { return std::atan2(y, x); }


    INLINE float EXP(float a) noexcept { return std::exp(a); }
    INLINE float EXP2(float a) noexcept { return std::exp2(a); }
    INLINE float FMOD(float a, float b) noexcept { return std::fmod(a, b); }
    INLINE float LOG(float a) noexcept { return std::log(a); }
    INLINE float LOG2(float a) noexcept { return std::log2(a); }

    INLINE float POW(float a, float b) noexcept { return std::pow(a, b); }
    INLINE double POWD(double a, double b) noexcept { return std::pow(a, b); }

    INLINE float SQR(float a) noexcept { return a * a; }
    INLINE float SQRT(float a) noexcept { return std::sqrt(a); }
}