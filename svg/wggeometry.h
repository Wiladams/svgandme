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
    
        bool operator == (const WGRectD& other) const
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
    
        bool operator == (const WGRectI& other) const
        {
            return x == other.x && y == other.y &&
                w == other.w && h == other.h;
        }
    };

    ASSERT_POD_TYPE(WGRectI);
//}

// ---------------------------------------
// WGRectD utilities
//
namespace waavs 
{
    static INLINE bool wg_rectD_is_valid(const WGRectD& r) noexcept
    {
        return (r.w > 0.0) && (r.h > 0.0);
    }
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



    static INLINE WGRectD wg_rectD_inflate(const WGRectD& r, const double dw, const double dh) noexcept
    {
        return WGRectD{ r.x - dw, r.y - dh, r.w + 2 * dw, r.h + 2 * dh };
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

namespace waavs
{
    static INLINE bool wg_rectI_is_valid(const WGRectI& r) noexcept
    {
        return r.w > 0 && r.h > 0;
    }

    static INLINE int wg_rectI_max_x(const WGRectI& r) noexcept
    {
        return r.x + r.w - 1;
    }

    static INLINE int wg_rectI_max_y(const WGRectI& r) noexcept
    {
        return r.y + r.h - 1;
    }

    static INLINE bool wg_rectI_contains_point(const WGRectI& r, int x, int y) noexcept
    {
        return x >= r.x && y >= r.y &&
            x < r.x + r.w &&
            y < r.y + r.h;
    }

    static INLINE bool wg_rectI_contains_sample(const WGRectI& r, double x, double y) noexcept
    {
        return x >= double(r.x) &&
            y >= double(r.y) &&
            x <= double(r.x + r.w - 1) &&
            y <= double(r.y + r.h - 1);
    }

    static INLINE bool wg_rectI_contains(const WGRectI& r, const WGRectI& bounds) noexcept
    {
        return r.x >= bounds.x &&
            r.y >= bounds.y &&
            r.x + r.w <= bounds.x + bounds.w &&
            r.y + r.h <= bounds.y + bounds.h;
    }


    // Inflate a rectangle by a given amount in each direction.  
// The dw and dh parameters specify how much to inflate 
// the rectangle on each edge in the horizontal and vertical directions, respectively.
// So, the final rectangle will be: width+2dw, height+2dh, 
// and the x and y will be moved by -dw and -dh respectively.
    static INLINE WGRectI wg_rectI_inflate(const WGRectI& r, const int dw, const int dh) noexcept
    {
        return WGRectI{ r.x - dw, r.y - dh, r.w + 2 * dw, r.h + 2 * dh };
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



namespace waavs {


    // Matrix type for 2D graphics
    // This matrix is a 3x3, which means it can represent
    // all affine transformations in 2D, including translation, 
    // rotation, scaling, and skewing.
    // I can also do perspective transformations, 
    // This matrix is meant to be easily compatible with
    // the BLMatrix2D type used in Blend2D
    struct WGMatrix3x3 final 
    {
        union {
            double m[9];
            struct {
                double m00, m01, m02;
                double m10, m11, m12;
                double m20, m21, m22;
            };
        };

        // ------------------------------------------------------------------------
        // Construction
        // ------------------------------------------------------------------------

        INLINE WGMatrix3x3() noexcept
        {
            // startoff with identity matrix
            m00 = 1.0; m01 = 0.0; m02 = 0.0;
            m10 = 0.0; m11 = 1.0; m12 = 0.0;
            m20 = 0.0; m21 = 0.0; m22 = 1.0;
        }

        INLINE constexpr WGMatrix3x3(const WGMatrix3x3&) noexcept = default;
        INLINE WGMatrix3x3& operator=(const WGMatrix3x3&) noexcept = default;

        // Affine 2D constructor:
        //
        //   [ m00 m01 0 ]
        //   [ m10 m11 0 ]
        //   [ m20 m21 1 ]
        //
        INLINE constexpr WGMatrix3x3(
            double m00Value, double m01Value,
            double m10Value, double m11Value,
            double m20Value, double m21Value) noexcept
            : m00(m00Value), m01(m01Value), m02(0.0),
            m10(m10Value), m11(m11Value), m12(0.0),
            m20(m20Value), m21(m21Value), m22(1.0) {
        }

        // Full 3x3 constructor.
        INLINE constexpr WGMatrix3x3(
            double m00Value, double m01Value, double m02Value,
            double m10Value, double m11Value, double m12Value,
            double m20Value, double m21Value, double m22Value) noexcept
            : m00(m00Value), m01(m01Value), m02(m02Value),
            m10(m10Value), m11(m11Value), m12(m12Value),
            m20(m20Value), m21(m21Value), m22(m22Value) {
        }

        // ------------------------------------------------------------------------
        // Static constructors
        // ------------------------------------------------------------------------

        WG_NODISCARD
            static INLINE constexpr WGMatrix3x3 makeIdentity() noexcept {
            return WGMatrix3x3(
                1.0, 0.0, 0.0,
                0.0, 1.0, 0.0,
                0.0, 0.0, 1.0);
        }

        WG_NODISCARD
            static INLINE constexpr WGMatrix3x3 makeTranslation(double x, double y) noexcept {
            return WGMatrix3x3(
                1.0, 0.0, 0.0,
                0.0, 1.0, 0.0,
                x, y, 1.0);
        }

        WG_NODISCARD
            static INLINE constexpr WGMatrix3x3 makeScaling(double x, double y) noexcept {
            return WGMatrix3x3(
                x, 0.0, 0.0,
                0.0, y, 0.0,
                0.0, 0.0, 1.0);
        }

        WG_NODISCARD
            static INLINE constexpr WGMatrix3x3 makeScaling(double xy) noexcept {
            return makeScaling(xy, xy);
        }

        WG_NODISCARD
            static INLINE WGMatrix3x3 makeSkewing(double x, double y) noexcept {
            return WGMatrix3x3(
                1.0, std::tan(y), 0.0,
                std::tan(x), 1.0, 0.0,
                0.0, 0.0, 1.0);
        }

        WG_NODISCARD
            static INLINE WGMatrix3x3 makeSinCos(double s, double c, double tx = 0.0, double ty = 0.0) noexcept {
            return WGMatrix3x3(
                c, s, 0.0,
                -s, c, 0.0,
                tx, ty, 1.0);
        }

        WG_NODISCARD
            static INLINE WGMatrix3x3 makeRotation(double angle) noexcept
        {
            WGMatrix3x3 m;
            m.resetToRotation(angle);
            return m;
        }

        WG_NODISCARD
            static INLINE WGMatrix3x3 makeRotation(double angle, double x, double y) noexcept
        {
            WGMatrix3x3 m;
            m.resetToRotation(angle, x, y);
            return m;
        }

        // ------------------------------------------------------------------------
        // Reset
        // ------------------------------------------------------------------------

        INLINE void reset() noexcept {
            m00 = 1.0; m01 = 0.0; m02 = 0.0;
            m10 = 0.0; m11 = 1.0; m12 = 0.0;
            m20 = 0.0; m21 = 0.0; m22 = 1.0;
        }

        INLINE void reset(const WGMatrix3x3& other) noexcept {
            m00 = other.m00; m01 = other.m01; m02 = other.m02;
            m10 = other.m10; m11 = other.m11; m12 = other.m12;
            m20 = other.m20; m21 = other.m21; m22 = other.m22;
        }

        INLINE void reset(
            double m00Value, double m01Value, double m02Value,
            double m10Value, double m11Value, double m12Value,
            double m20Value, double m21Value, double m22Value) noexcept {
            m00 = m00Value; m01 = m01Value; m02 = m02Value;
            m10 = m10Value; m11 = m11Value; m12 = m12Value;
            m20 = m20Value; m21 = m21Value; m22 = m22Value;
        }

        INLINE void resetAffine(
            double m00Value, double m01Value,
            double m10Value, double m11Value,
            double m20Value, double m21Value) noexcept {
            m00 = m00Value; m01 = m01Value; m02 = 0.0;
            m10 = m10Value; m11 = m11Value; m12 = 0.0;
            m20 = m20Value; m21 = m21Value; m22 = 1.0;
        }

        INLINE void resetToTranslation(double x, double y) noexcept {
            resetAffine(1.0, 0.0,
                0.0, 1.0,
                x, y);
        }

        INLINE void resetToScaling(double x, double y) noexcept {
            resetAffine(x, 0.0,
                0.0, y,
                0.0, 0.0);
        }

        INLINE void resetToScaling(double xy) noexcept {
            resetToScaling(xy, xy);
        }

        INLINE void resetToSkewing(double x, double y) noexcept {
            *this = makeSkewing(x, y);
        }

        INLINE void resetToSinCos(double s, double c, double tx = 0.0, double ty = 0.0) noexcept {
            resetAffine(c, s,
                -s, c,
                tx, ty);
        }

        INLINE void resetToRotation(double angle) noexcept {
            const double s = std::sin(angle);
            const double c = std::cos(angle);
            resetToSinCos(s, c);
        }

        INLINE void resetToRotation(double angle, double x, double y) noexcept {
            const double s = std::sin(angle);
            const double c = std::cos(angle);

            // Rotation about point (x, y) in row-vector convention:
            // T(-x, -y) * R * T(x, y)
            resetAffine(
                c, s,
                -s, c,
                x - x * c + y * s,
                y - x * s - y * c);
        }

        // ------------------------------------------------------------------------
        // Comparison / properties
        // ------------------------------------------------------------------------

        WG_NODISCARD
            INLINE bool equals(const WGMatrix3x3& other) const noexcept {
            return dbl_eq(m00, other.m00) &&
                dbl_eq(m01, other.m01) &&
                dbl_eq(m02, other.m02) &&
                dbl_eq(m10, other.m10) &&
                dbl_eq(m11, other.m11) &&
                dbl_eq(m12, other.m12) &&
                dbl_eq(m20, other.m20) &&
                dbl_eq(m21, other.m21) &&
                dbl_eq(m22, other.m22);
        }

        WG_NODISCARD
            INLINE bool operator==(const WGMatrix3x3& other) const noexcept { return equals(other); }

        WG_NODISCARD
            INLINE bool operator!=(const WGMatrix3x3& other) const noexcept { return !equals(other); }

        WG_NODISCARD
            INLINE bool isIdentity() const noexcept {
            return m00 == 1.0 && m01 == 0.0 && m02 == 0.0 &&
                m10 == 0.0 && m11 == 1.0 && m12 == 0.0 &&
                m20 == 0.0 && m21 == 0.0 && m22 == 1.0;
        }

        WG_NODISCARD
            INLINE bool isAffine2D() const noexcept
        {
            return almost_zero(m02) &&
                almost_zero(m12) &&
                almost_zero(m22 - 1.0);
        }

        WG_NODISCARD
            INLINE double determinant() const noexcept {
            return
                m00 * (m11 * m22 - m12 * m21) -
                m01 * (m10 * m22 - m12 * m20) +
                m02 * (m10 * m21 - m11 * m20);
        }

        WG_NODISCARD
            INLINE double determinantAffine2D() const noexcept {
            return m00 * m11 - m01 * m10;
        }

        // ------------------------------------------------------------------------
        // Matrix multiply
        // ------------------------------------------------------------------------

        WG_NODISCARD
            static INLINE WGMatrix3x3 mul(const WGMatrix3x3& a, const WGMatrix3x3& b) noexcept {
            WGMatrix3x3 r;

            r.m00 = a.m00 * b.m00 + a.m01 * b.m10 + a.m02 * b.m20;
            r.m01 = a.m00 * b.m01 + a.m01 * b.m11 + a.m02 * b.m21;
            r.m02 = a.m00 * b.m02 + a.m01 * b.m12 + a.m02 * b.m22;

            r.m10 = a.m10 * b.m00 + a.m11 * b.m10 + a.m12 * b.m20;
            r.m11 = a.m10 * b.m01 + a.m11 * b.m11 + a.m12 * b.m21;
            r.m12 = a.m10 * b.m02 + a.m11 * b.m12 + a.m12 * b.m22;

            r.m20 = a.m20 * b.m00 + a.m21 * b.m10 + a.m22 * b.m20;
            r.m21 = a.m20 * b.m01 + a.m21 * b.m11 + a.m22 * b.m21;
            r.m22 = a.m20 * b.m02 + a.m21 * b.m12 + a.m22 * b.m22;

            return r;
        }

        INLINE void transform(const WGMatrix3x3& other) noexcept {
            *this = mul(other, *this);
        }

        INLINE void postTransform(const WGMatrix3x3& other) noexcept {
            *this = mul(*this, other);
        }

        // ------------------------------------------------------------------------
        // Affine-style ops matching BLMatrix2D row-vector behavior
        // ------------------------------------------------------------------------

        INLINE void translate(double x, double y) noexcept {
            // Pre-style translate under row-vector convention.
            m20 += x * m00 + y * m10;
            m21 += x * m01 + y * m11;
            m22 += x * m02 + y * m12;
        }

        INLINE void scale(double x, double y) noexcept {
            // Pre-style scale under row-vector convention.
            m00 *= x; m01 *= x; m02 *= x;
            m10 *= y; m11 *= y; m12 *= y;
        }

        INLINE void scale(double xy) noexcept {
            scale(xy, xy);
        }

        INLINE void skew(double x, double y) noexcept {
            transform(makeSkewing(x, y));
        }

        INLINE void rotate(double angle) noexcept
        {
            WGMatrix3x3 r;
            r.resetToRotation(angle);
            transform(r);
        }

        INLINE void rotate(double angle, double x, double y) noexcept
        {
            WGMatrix3x3 r;
            r.resetToRotation(angle, x, y);
            transform(r);
        }

        INLINE void postTranslate(double x, double y) noexcept {
            m20 += x;
            m21 += y;
        }

        INLINE void postScale(double x, double y) noexcept {
            m00 *= x; m01 *= y; m02 *= 1.0;
            m10 *= x; m11 *= y; m12 *= 1.0;
            m20 *= x; m21 *= y; m22 *= 1.0;
        }

        INLINE void postScale(double xy) noexcept {
            postScale(xy, xy);
        }

        INLINE void postSkew(double x, double y) noexcept {
            postTransform(makeSkewing(x, y));
        }

        INLINE void postRotate(double angle) noexcept
        {
            WGMatrix3x3 r;
            r.resetToRotation(angle);
            postTransform(r);
        }

        INLINE void postRotate(double angle, double x, double y) noexcept
        {
            WGMatrix3x3 r;
            r.resetToRotation(angle, x, y);
            postTransform(r);
        }

        // ------------------------------------------------------------------------
        // Inversion
        // ------------------------------------------------------------------------

        INLINE bool invertAffine2D() noexcept {
            const double d = determinantAffine2D();
            if (d == 0.0)
                return false;

            const double invD = 1.0 / d;

            const double a = m11 * invD;
            const double b = -m01 * invD;
            const double c = -m10 * invD;
            const double d2 = m00 * invD;
            const double tx = -(m20 * a + m21 * c);
            const double ty = -(m20 * b + m21 * d2);

            resetAffine(a, b,
                c, d2,
                tx, ty);
            return true;
        }

        INLINE bool invert() noexcept {
            const double det = determinant();
            if (det == 0.0)
                return false;

            const double invDet = 1.0 / det;

            WGMatrix3x3 r;

            r.m00 = (m11 * m22 - m12 * m21) * invDet;
            r.m01 = -(m01 * m22 - m02 * m21) * invDet;
            r.m02 = (m01 * m12 - m02 * m11) * invDet;

            r.m10 = -(m10 * m22 - m12 * m20) * invDet;
            r.m11 = (m00 * m22 - m02 * m20) * invDet;
            r.m12 = -(m00 * m12 - m02 * m10) * invDet;

            r.m20 = (m10 * m21 - m11 * m20) * invDet;
            r.m21 = -(m00 * m21 - m01 * m20) * invDet;
            r.m22 = (m00 * m11 - m01 * m10) * invDet;

            *this = r;
            return true;
        }

        WG_NODISCARD
        static INLINE bool invert(WGMatrix3x3& dst, const WGMatrix3x3& src) noexcept {
            dst = src;
            return dst.invert();
        }

        WG_NODISCARD
        static INLINE bool invertAffine2D(WGMatrix3x3& dst, const WGMatrix3x3& src) noexcept {
            dst = src;
            return dst.invertAffine2D();
        }

        // ------------------------------------------------------------------------
        // Mapping
        // ------------------------------------------------------------------------

        // Row-vector convention:
        //
        // [x y 1] * M
        //
        // x' = x*m00 + y*m10 + m20
        // y' = x*m01 + y*m11 + m21
        //
        WG_NODISCARD
        INLINE WGPointD mapPoint(double x, double y) const noexcept {
            const double xp = x * m00 + y * m10 + m20;
            const double yp = x * m01 + y * m11 + m21;
            const double wp = x * m02 + y * m12 + m22;

            if (wp != 0.0 && wp != 1.0) {
                const double invW = 1.0 / wp;
                return WGPointD{ xp * invW, yp * invW };
            }

            return WGPointD{ xp, yp };
        }

        WG_NODISCARD
        INLINE WGPointD mapPoint(const WGPointD& p) const noexcept {
            return mapPoint(p.x, p.y);
        }

        WG_NODISCARD
        INLINE WGPointD mapVector(double x, double y) const noexcept {
            const double xp = x * m00 + y * m10;
            const double yp = x * m01 + y * m11;
            return WGPointD{ xp, yp };
        }

        WG_NODISCARD
        INLINE WGPointD mapVector(const WGPointD& v) const noexcept {
            return mapVector(v.x, v.y);
        }
    };

} // namespace waavs


