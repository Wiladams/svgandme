#pragma once

#include "gcdl_types.h"

// Accumulating some maths functions
namespace waavs
{
    static inline double gcdl_deg_to_rad(double deg)
    {
        return deg * 3.1415926535897932384626433832795 / 180.0;
    }

    // calculate the angle between two points
    static inline double gcdl_angle_of(const GCDLPoint& c, const GCDLPoint& p)
    {
        return std::atan2(p.y - c.y, p.x - c.x);
    }

    // Distance between two points
    static inline double gcdl_distance(const GCDLPoint& a, const GCDLPoint& b)
    {
        const double dx = b.x - a.x;
        const double dy = b.y - a.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    static inline GCDLPoint gcdl_rotate_point(
        const GCDLPoint& p,
        const GCDLPoint& c,
        double angleDeg)
    {
        const double a = gcdl_deg_to_rad(angleDeg);
        const double ca = std::cos(a);
        const double sa = std::sin(a);

        const double dx = p.x - c.x;
        const double dy = p.y - c.y;

        GCDLPoint out;
        out.x = c.x + dx * ca - dy * sa;
        out.y = c.y + dx * sa + dy * ca;
        return out;
    }

    static inline uint32_t gcdl_gcd_u32(uint32_t a, uint32_t b)
    {
        while (b != 0) {
            uint32_t t = b;
            b = a % b;
            a = t;
        }

        return a;
    }
}
