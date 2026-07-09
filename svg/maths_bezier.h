#pragma once

#include <cmath>

#include "definitions.h"
#include "core_geometry.h"

namespace waavs
{
    static constexpr double BEZIER_EPSILON = 1e-12;

    // Check if a t value is in the interior of the bezier curve (0..1)
    static INLINE bool bez_is_interior_t(double t) noexcept
    {
        return t > 0.0 && t < 1.0;
    }

    // ------------------------------------------------------
    // Quadratic bezier
    
    // Quadratic bezier evaluation using the standard formula:
    static INLINE double bez_quad_eval(
        double p0, double p1, double p2,
        double t) noexcept
    {
        double mt = 1.0 - t;
        return mt * mt * p0 + 2.0 * mt * t * p1 + t * t * p2;
    }

    static INLINE bool bez_quad_solve_extrema(
        double p0, double p1, double p2,
        double& t) noexcept
    {
        double d = p0 - 2.0 * p1 + p2;

        if (std::abs(d) < BEZIER_EPSILON)
            return false;

        t = (p0 - p1) / d;
        return bez_is_interior_t(t);
    }

    static INLINE void bez_quad_eval_point(
        double x0, double y0,
        double x1, double y1,
        double x2, double y2,
        double t,
        double& x, double& y) noexcept
    {
        x = bez_quad_eval(x0, x1, x2, t);
        y = bez_quad_eval(y0, y1, y2, t);
    }


    // ------------------------------------------------------
    // Cubic bezier

    static INLINE double bez_cubic_eval(
        double p0, double p1, double p2, double p3,
        double t) noexcept
    {
        double mt = 1.0 - t;
        return mt * mt * mt * p0
            + 3.0 * mt * mt * t * p1
            + 3.0 * mt * t * t * p2
            + t * t * t * p3;
    }



    static INLINE int bez_cubic_solve_extrema(
        double p0, double p1, double p2, double p3,
        double tOut[2]) noexcept
    {
        double a = -p0 + 3.0 * p1 - 3.0 * p2 + p3;
        double b = 2.0 * (p0 - 2.0 * p1 + p2);
        double c = -p0 + p1;

        if (std::abs(a) < BEZIER_EPSILON) {
            if (std::abs(b) < BEZIER_EPSILON)
                return 0;

            double t = -c / b;
            if (!bez_is_interior_t(t))
                return 0;

            tOut[0] = t;
            return 1;
        }

        double disc = b * b - 4.0 * a * c;
        if (disc < 0.0)
            return 0;

        double s = std::sqrt(disc);
        double invDenom = 1.0 / (2.0 * a);

        double t1 = (-b + s) * invDenom;
        double t2 = (-b - s) * invDenom;

        int count = 0;

        if (bez_is_interior_t(t1))
            tOut[count++] = t1;

        if (bez_is_interior_t(t2) && std::abs(t2 - t1) >= BEZIER_EPSILON)
            tOut[count++] = t2;

        return count;
    }


    static INLINE void bez_cubic_eval_point(
        double x0, double y0,
        double x1, double y1,
        double x2, double y2,
        double x3, double y3,
        double t,
        double& x, double& y) noexcept
    {
        x = bez_cubic_eval(x0, x1, x2, x3, t);
        y = bez_cubic_eval(y0, y1, y2, y3, t);
    }


}