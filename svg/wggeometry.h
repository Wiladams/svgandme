#pragma once

#include "definitions.h"
#include "maths.h"

namespace waavs
{
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
		
        // Calculate the midpoint between two points
        WGPointD midpoint(const WGPointD& b) const { return (*this + b) * 0.5; }

        // Treat the point as a vector, and convert to a unit
        // vector (divide by length)
        void normalize() {
			double len = std::sqrt(x * x + y * y);
			if (len > dbl_eps) {
				x /= len;
				y /= len;
			}
		}
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
    };

    ASSERT_POD_TYPE(WGRectD);

    struct WGRectI { 
        int x{}, y{}, w{}, h{}; 
        WGRectI() : x(0), y(0), w(0), h(0) {}
        WGRectI(const int x, const int y, const int w, const int h) : x(x), y(y), w(w), h(h) {}
    };

    ASSERT_POD_TYPE(WGRectI);
}


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
    static INLINE WGRectD rectMerge(const WGRectD& a, const WGPointD& b)
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
    // firts the original (a) as well as the new one (b)
    // this is a union operation.
    static INLINE WGRectD rectMerge(const WGRectD& a, const WGRectD& b)
    {
        // return a WGRectD that is the union of WGRectD a
        // and WGRectD b, using local temporary variables
        double x1 = std::min(a.x, b.x);
        double y1 = std::min(a.y, b.y);
        double x2 = std::max(a.x + a.w, b.x + b.w);
        double y2 = std::max(a.y + a.h, b.y + b.h);

        return { x1, y1, x2 - x1, y2 - y1 };
    }

    static INLINE void expandRect(WGRectD& a, const WGPointD& b) { a = rectMerge(a, b); }
    static INLINE void wg_rectD_union(WGRectD& a, const WGRectD& b) { a = rectMerge(a, b); }
}

