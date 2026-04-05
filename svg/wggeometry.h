#pragma once

#include "definitions.h"
#include "maths.h"

//namespace waavs
//{
    // Very core low level geometry types.  Use type assertions here
    // so they remain usable in C interfaces, and to ensure they are 
    // POD types that can be safely memcpy'd and used in unions and the like.
    // Don't want to introduce any virtual functions or anything like that here, 
    // as these types are meant to be lightweight and efficient, and used in hot code paths.
    //

    struct WGPointD final { 
        double x, y; 
        
        WGPointD() : x(0), y(0) {}
        WGPointD(const double x, const double y) : x(x), y(y) {}

        WGPointD operator-(const WGPointD& rhs) const { return { x - rhs.x, y - rhs.y }; }
        WGPointD operator+(const WGPointD& rhs) const { return { x + rhs.x, y + rhs.y }; }
        WGPointD operator*(double s) const { return { x * s, y * s }; }
        
#ifdef __cplusplus
        // Calculate the midpoint between two points
        WGPointD midpoint(const WGPointD& b) const { return (*this + b) * 0.5; }

        // Treat the point as a vector, and convert to a unit
        // vector (divide by length)
        void normalize() {
            double len = std::sqrt(x * x + y * y);
            if (len > waavs::dbl_eps) {
                x /= len;
                y /= len;
            }
        }
#endif
    };
    
    ASSERT_POD_TYPE(WGPointD);


    struct WGPointI { 
        int x, y; 

        WGPointI() : x(0), y(0) {}
        WGPointI(const int x, const int y) : x(x), y(y) {}


    };

    ASSERT_POD_TYPE(WGPointI);


    struct WGRectD { 
        double x, y, w, h; 

        WGRectD() : x(0), y(0), w(0), h(0) {}
        WGRectD(const double x, const double y, const double w, const double h) : x(x), y(y), w(w), h(h) {}
    
        bool operator == (const WGRectD& other)
        {
            return x == other.x && y == other.y &&
                w == other.w && h == other.h;
        }

        bool isEmpty() const { return w <= 0.0 || h <= 0.0; }

    };

    ASSERT_POD_TYPE(WGRectD);

    struct WGRectI { 
        int x{}, y{}, w{}, h{}; 
        WGRectI() : x(0), y(0), w(0), h(0) {}
        WGRectI(const int x, const int y, const int w, const int h) : x(x), y(y), w(w), h(h) {}
    
        bool operator == (const WGRectI& other)
        {
            return x == other.x && y == other.y &&
                w == other.w && h == other.h;
        }
    };

    ASSERT_POD_TYPE(WGRectI);
//}


namespace waavs 
{
    static INLINE void wg_rectD_reset(WGRectD& r) { r.x = 0.0; r.y = 0.0; r.w = 0.0; r.h = 0.0; }

    static INLINE double right(const WGRectD& r) { return r.x + r.w; }
    static INLINE double left(const WGRectD& r) { return r.x; }
    static INLINE double top(const WGRectD& r) { return r.y; }
    static INLINE double bottom(const WGRectD& r) { return r.y + r.h; }
    static INLINE WGPointD center(const WGRectD& r) { return { r.x + (r.w / 2),r.y + (r.h / 2) }; }

    static INLINE void moveBy(WGRectD& r, double dx, double dy) { r.x += dx; r.y += dy; }
    static INLINE void moveBy(WGRectD& r, const WGPointD& dxy) { r.x += dxy.x; r.y += dxy.y; }


    static INLINE bool containsRect(const WGRectD& a, double x, double y)
    {
        return (x >= a.x && x < a.x + a.w && y >= a.y && y < a.y + a.h);
    }

    static INLINE bool containsRect(const WGRectD& a, const WGPointD& pt)
    {
        return containsRect(a, pt.x, pt.y);
    }

    static INLINE WGRectI intersection(const WGRectI& a, const WGRectI& b) noexcept {
        const int x0 = (a.x > b.x) ? a.x : b.x;
        const int y0 = (a.y > b.y) ? a.y : b.y;
        const int x1 = ((a.x + a.w) < (b.x + b.w)) ? (a.x + a.w) : (b.x + b.w);
        const int y1 = ((a.y + a.h) < (b.y + b.h)) ? (a.y + a.h) : (b.y + b.h);
        const int w = x1 - x0;
        const int h = y1 - y0;
        if (w <= 0 || h <= 0) return WGRectI{ 0,0,0,0 };
        return WGRectI{ x0, y0, w, h };
    }
    
    // return the intersection between two rectangles
    // if they don't intersect, then return an empty rect
    //
    static INLINE WGRectD intersection(const WGRectD& a, const WGRectD& b) noexcept 
    {
        const double x0 = (a.x > b.x) ? a.x : b.x;
        const double y0 = (a.y > b.y) ? a.y : b.y;
        const double x1 = ((a.x + a.w) < (b.x + b.w)) ? (a.x + a.w) : (b.x + b.w);
        const double y1 = ((a.y + a.h) < (b.y + b.h)) ? (a.y + a.h) : (b.y + b.h);
        const double w = x1 - x0;
        const double h = y1 - y0;

        if (w <= 0 || h <= 0) 
            return WGRectD{ 0,0,0,0 };
        
        return WGRectD{ x0, y0, w, h };
    }

    // mergeRect()
    // 
    // Perform a union operation between a WGRectD and a WGPointD
    // Return a new WGRectD that represents the union.
    static INLINE WGRectD wg_rectd_merge_point(const WGRectD& a, const WGPointD& b)
    {
        // return a BLRect that is the union of BLRect a 
        // and BLPoint b using local temporary variables
        double x1 = std::min(a.x, b.x);
        double y1 = std::min(a.y, b.y);
        double x2 = std::max(a.x + a.w, b.x);
        double y2 = std::max(a.y + a.h, b.y);

        return { x1, y1, x2 - x1, y2 - y1 };
    }

    // mergeRect()
    // 
    // Expand the size of the rectangle such that the new rectangle
    // fits the original (a) as well as the new one (b)
    // this is a union operation.
    static INLINE WGRectD wg_rectd_merge_rect(const WGRectD& a, const WGRectD& b)
    {
        // return a WGRectD that is the union of WGRectD a
        // and WGRectD b, using local temporary variables
        double x1 = std::min(a.x, b.x);
        double y1 = std::min(a.y, b.y);
        double x2 = std::max(a.x + a.w, b.x + b.w);
        double y2 = std::max(a.y + a.h, b.y + b.h);

        return { x1, y1, x2 - x1, y2 - y1 };
    }

    static INLINE void wg_rectD_union_point(WGRectD& a, const WGPointD& b) { a = wg_rectd_merge_point(a, b); }
    static INLINE void wg_rectD_union(WGRectD& a, const WGRectD& b) 
    { 
        if (a.isEmpty()) { a = b; return; }

        a = wg_rectd_merge_rect(a, b); 
    }


    // Boxes
    static INLINE void bboxInit(double& minX, double& minY,
        double& maxX, double& maxY,
        double x, double y) noexcept
    {
        minX = maxX = x;
        minY = maxY = y;

        //minX = minY = std::numeric_limits<double>::infinity();
        //maxX = maxY = -std::numeric_limits<double>::infinity();
    }

    static INLINE void bboxExpand(double& minX, double& minY,
        double& maxX, double& maxY,
        double x, double y) noexcept
    {
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }

    // distanceToLine()
    // 
    // Calculate the distance from a point to a line segment.  The line segment
    // is defined by two points (a and b).  
    // The point int question is defined by pt.
    //
    static double distance_to_line(const WGPointD& pt, const WGPointD& a, const WGPointD& b)
    {
        double dx = b.x - a.x;
        double dy = b.y - a.y;
        double num = std::abs(dy * pt.x - dx * pt.y + b.x * a.y - b.y * a.x);
        double den = std::sqrt(dx * dx + dy * dy);
        return den > 0 ? num / den : 0.0;
    }
}

//
// Bounding box calculations for quadratic and cubic Bezier curves.
//
namespace waavs
{
    static INLINE void quadBounds(
        double x0, double y0,
        double x1, double y1,
        double x2, double y2,
        double& minX, double& minY,
        double& maxX, double& maxY)
    {
        bboxExpand(minX, minY, maxX, maxY, x2, y2);

        // X extrema
        double dx = x0 - 2.0 * x1 + x2;
        if (dx != 0.0) {
            double t = (x0 - x1) / dx;
            if (t > 0.0 && t < 1.0) {
                double mt = 1.0 - t;
                double x = mt * mt * x0 + 2 * mt * t * x1 + t * t * x2;
                double y = mt * mt * y0 + 2 * mt * t * y1 + t * t * y2;
                bboxExpand(minX, minY, maxX, maxY, x, y);
            }
        }

        // Y extrema
        double dy = y0 - 2.0 * y1 + y2;
        if (dy != 0.0) {
            double t = (y0 - y1) / dy;
            if (t > 0.0 && t < 1.0) {
                double mt = 1.0 - t;
                double x = mt * mt * x0 + 2 * mt * t * x1 + t * t * x2;
                double y = mt * mt * y0 + 2 * mt * t * y1 + t * t * y2;
                bboxExpand(minX, minY, maxX, maxY, x, y);
            }
        }
    }

    static inline void cubicBounds(
        double x0, double y0,
        double x1, double y1,
        double x2, double y2,
        double x3, double y3,
        double& minX, double& minY,
        double& maxX, double& maxY)
    {
        bboxExpand(minX, minY, maxX, maxY, x3, y3);

        auto solve = [&](double p0, double p1, double p2, double p3,
            bool isX)
            {
                double a = -p0 + 3 * p1 - 3 * p2 + p3;
                double b = 2 * (p0 - 2 * p1 + p2);
                double c = -p0 + p1;

                if (std::abs(a) < 1e-12) {
                    if (std::abs(b) < 1e-12) return;
                    double t = -c / b;
                    if (t > 0.0 && t < 1.0) {
                        double mt = 1.0 - t;
                        double x = mt * mt * mt * x0 + 3 * mt * mt * t * x1 + 3 * mt * t * t * x2 + t * t * t * x3;
                        double y = mt * mt * mt * y0 + 3 * mt * mt * t * y1 + 3 * mt * t * t * y2 + t * t * t * y3;
                        bboxExpand(minX, minY, maxX, maxY, x, y);
                    }
                    return;
                }

                double disc = b * b - 4 * a * c;
                if (disc < 0.0) return;

                double s = std::sqrt(disc);
                double t1 = (-b + s) / (2 * a);
                double t2 = (-b - s) / (2 * a);

                auto eval = [&](double t) {
                    if (t > 0.0 && t < 1.0) {
                        double mt = 1.0 - t;
                        double x = mt * mt * mt * x0 + 3 * mt * mt * t * x1 + 3 * mt * t * t * x2 + t * t * t * x3;
                        double y = mt * mt * mt * y0 + 3 * mt * mt * t * y1 + 3 * mt * t * t * y2 + t * t * t * y3;
                        bboxExpand(minX, minY, maxX, maxY, x, y);
                    }
                    };

                eval(t1);
                eval(t2);
            };

        solve(x0, x1, x2, x3, true);
        solve(y0, y1, y2, y3, false);
    }
}

