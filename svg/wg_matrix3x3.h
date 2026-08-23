// wg_matrix3x3.h

#pragma once

#include "core_geometry.h"

namespace waavs {

    // WGMatrix3x3
    // 
    // Matrix type for 2D graphics
    // This matrix is a 3x3, which means it can represent
    // all affine transformations in 2D, including translation, 
    // rotation, scaling, and skewing.
    // As such, it can be a drop-in replacement for the BLMatrix2D type used
    // in blend2d.  This gives us some backend independence, as well as gives
    // us the ability to do perspective transformations if the applicaiton needs that.
    // 
    // The trick is to maintain good performance for the default case of
    // affine transformations, while still supporting the general
    // case of perspective transformations.

    // This enum tells you what kind of matrix you have
    // so you can optimize certain operations based on the type of matrix.
    enum WGTransformType : uint32_t
    {
        WG_TRANSFORM_TYPE_IDENTITY      = 0x00000001u,
        WG_TRANSFORM_TYPE_TRANSLATE     = 0x00000002u,
        WG_TRANSFORM_TYPE_SCALE         = 0x00000004u,
        WG_TRANSFORM_TYPE_AFFINE        = 0x00000008u,
        WG_TRANSFORM_TYPE_PERSPECTIVE   = 0x00000010u
    };

    struct WGMatrix3x3
    {
        double m00, m01, m02;
        double m10, m11, m12;
        double m20, m21, m22;
        /*
        union {
            double m[9];
            struct {
                double m00, m01, m02;
                double m10, m11, m12;
                double m20, m21, m22;
            };
        };
        */

        // ------------------------------------------------------------------------
        // Construction
        // ------------------------------------------------------------------------

        WGMatrix3x3() noexcept
        {
            // startoff with identity matrix
            m00 = 1.0; m01 = 0.0; m02 = 0.0;
            m10 = 0.0; m11 = 1.0; m12 = 0.0;
            m20 = 0.0; m21 = 0.0; m22 = 1.0;
        }

        constexpr WGMatrix3x3(const WGMatrix3x3&) noexcept = default;
        WGMatrix3x3& operator=(const WGMatrix3x3&) noexcept = default;

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

        static WGMatrix3x3 makeIdentity() noexcept 
        {
            return WGMatrix3x3(
                1.0, 0.0, 0.0,
                0.0, 1.0, 0.0,
                0.0, 0.0, 1.0);
        }

        static WGMatrix3x3 makeTranslation(double x, double y) noexcept 
        {
            return WGMatrix3x3(
                1.0, 0.0, 0.0,
                0.0, 1.0, 0.0,
                x, y, 1.0);
        }

        static WGMatrix3x3 makeScaling(double x, double y) noexcept 
        {
            return WGMatrix3x3(
                x, 0.0, 0.0,
                0.0, y, 0.0,
                0.0, 0.0, 1.0);
        }

        static WGMatrix3x3 makeScaling(double xy) noexcept 
        {
            return makeScaling(xy, xy);
        }

        static WGMatrix3x3 makeSkewing(double x, double y) noexcept 
        {
            return WGMatrix3x3(
                1.0, std::tan(y), 0.0,
                std::tan(x), 1.0, 0.0,
                0.0, 0.0, 1.0);
        }

        
        static WGMatrix3x3 makeSinCos(double s, double c, double tx = 0.0, double ty = 0.0) noexcept 
        {
            return WGMatrix3x3(
                c, s, 0.0,
                -s, c, 0.0,
                tx, ty, 1.0);
        }

        
        static WGMatrix3x3 makeRotation(double angle) noexcept
        {
            return makeRotation(angle, 0.0, 0.0);
        }

        
        static WGMatrix3x3 makeRotation(double angle, double cx, double cy) noexcept
        {
            WGMatrix3x3 m;
            m.resetToRotation(angle, cx, cy);
            return m;
        }

        static WGMatrix3x3 makeRotationAboutPoint(
            double angle,
            double px,
            double py) noexcept
        {
            const double s = std::sin(angle);
            const double c = std::cos(angle);

            return WGMatrix3x3(
                c, s, 0.0,
                -s, c, 0.0,
                px - c * px + s * py,
                py - s * px - c * py,
                1.0);
        }

        // ----------------------------------------
        // 
        
        INLINE WGTransformType getType() const noexcept
        {
            if (m02 != 0.0 || m12 != 0.0 || m22 != 1.0)
                return WG_TRANSFORM_TYPE_PERSPECTIVE;

            if (m00 == 1.0 && m01 == 0.0 &&
                m10 == 0.0 && m11 == 1.0) {
                if (m20 == 0.0 && m21 == 0.0)
                    return WG_TRANSFORM_TYPE_IDENTITY;

                return WG_TRANSFORM_TYPE_TRANSLATE;
            }

            if (m01 == 0.0 && m10 == 0.0) {
                if (m20 == 0.0 && m21 == 0.0)
                    return WG_TRANSFORM_TYPE_SCALE;

                return WG_TRANSFORM_TYPE_AFFINE;
            }

            return WG_TRANSFORM_TYPE_AFFINE;
        }

        // ------------------------------------------------------------------------
        // Reset
        // ------------------------------------------------------------------------
        // Reset the matrix to identity
        INLINE void reset() noexcept {
            m00 = 1.0; m01 = 0.0; m02 = 0.0;
            m10 = 0.0; m11 = 1.0; m12 = 0.0;
            m20 = 0.0; m21 = 0.0; m22 = 1.0;
        }

        // Reset the matrix to the values of another matrix
        INLINE void reset(const WGMatrix3x3& other) noexcept {
            m00 = other.m00; m01 = other.m01; m02 = other.m02;
            m10 = other.m10; m11 = other.m11; m12 = other.m12;
            m20 = other.m20; m21 = other.m21; m22 = other.m22;
        }

        // Reset the matrix to the given values
        INLINE void reset(
            double m00Value, double m01Value, double m02Value,
            double m10Value, double m11Value, double m12Value,
            double m20Value, double m21Value, double m22Value) noexcept {
            m00 = m00Value; m01 = m01Value; m02 = m02Value;
            m10 = m10Value; m11 = m11Value; m12 = m12Value;
            m20 = m20Value; m21 = m21Value; m22 = m22Value;
        }

        // Reset the matrix to the given values, with m02 = m12 = 0.0 and m22 = 1.0
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

        void resetToSkewing(double x, double y) noexcept {
            *this = makeSkewing(x, y);
        }


        void resetToRotation(double angle) noexcept 
        {
            resetToRotation(angle, 0.0, 0.0);
        }

        INLINE void resetToRotation(
            double angle,
            double x,
            double y) noexcept
        {
            const double s = std::sin(angle);
            const double c = std::cos(angle);

            // Rotation about point (x, y):
            //
            //     T(-x, -y) * R * T(x, y)
            //
            // under the row-vector convention [px py 1] * M.
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




        // ------------------------------------------------------------------------
        // Affine-style ops matching BLMatrix2D row-vector behavior
        // ------------------------------------------------------------------------

        INLINE void translate(double x, double y) noexcept 
        {
            transform(makeTranslation(x, y));

            // Pre-style translate under row-vector convention.
            //m20 += x * m00 + y * m10;
            //m21 += x * m01 + y * m11;
            //m22 += x * m02 + y * m12;
        }

        INLINE void scale(double xy) noexcept
        {
            scale(xy, xy);
        }

        INLINE void scale(double x, double y) noexcept 
        {
            transform(makeScaling(x, y));

            // Pre-style scale under row-vector convention.
            //m00 *= x; m01 *= x; m02 *= x;
            //m10 *= y; m11 *= y; m12 *= y;
        }



        INLINE void skew(double x, double y) noexcept 
        {
            transform(makeSkewing(x, y));
        }

        INLINE void rotate(double angle) noexcept
        {
            rotate(angle, 0.0, 0.0);
        }

        INLINE void rotate(double angle, double px, double py) noexcept
        {
            transform(makeRotationAboutPoint(angle, px, py));

        }


        INLINE void transform(const WGMatrix3x3& other) noexcept 
        {
            *this = mul(other, *this);
        }

        // ------------------------------------------
        // Post transform
        // ----------------------------------------
        
        INLINE void postTranslate(double x, double y) noexcept 
        {
            postTransform(makeTranslation(x, y));
        }

        INLINE void postScale(double xy) noexcept
        {
            postScale(xy, xy);
        }

        INLINE void postScale(double x, double y) noexcept 
        {
            postTransform(makeScaling(x, y));
        }

        INLINE void postSkew(double x, double y) noexcept 
        {
            postTransform(makeSkewing(x, y));
        }

        INLINE void postRotate(double angle) noexcept
        {
            postRotate(angle, 0.0, 0.0);
        }

        INLINE void postRotate(double angle, double px, double py) noexcept
        {
            postTransform(makeRotationAboutPoint(angle, px, py));
        }

        INLINE void postTransform(const WGMatrix3x3& other) noexcept
        {
            *this = mul(*this, other);
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
        // 
        // mapPoint()
        // Perform affine mapping directly, without perspective division.  
        // This is the one most common for 2D graphics and does the same
        // thing as BLMatrix2D::mapPoint() under row-vector convention.
        WG_NODISCARD
            INLINE WGPointD mapPoint(double x, double y) const noexcept
        {
            return WGPointD{
                x * m00 + y * m10 + m20,
                x * m01 + y * m11 + m21
            };
        }

        WG_NODISCARD
            INLINE WGPointD mapPoint(const WGPointD& p) const noexcept
        {
            return mapPoint(p.x, p.y);
        }

        // mapPointPerspective()
        //
        // Perform mapping with perspective division.  
        // This allows for perspective transformations
        WG_NODISCARD
            INLINE WGPointD mapPointPerspective(double x, double y) const noexcept
        {
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
            INLINE WGPointD mapPointPerspective(const WGPointD& p) const noexcept {
            return mapPointPerspective(p.x, p.y);
        }


        // mapVector()
        // 
        // [x y 0] * M
        // Since it's a vector, we ignore the translation components (m20, m21) 
        // and the perspective components (m02, m12, m22).
        // So, it becomes even simpler and faster to compute.
        WG_NODISCARD
            INLINE WGPointD mapVector(double x, double y) const noexcept
        {
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
