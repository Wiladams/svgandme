#include <cstdio>
#include <cmath>

#include "wg_matrix3x3.h"
#include <blend2d/blend2d.h>

#pragma comment(lib, "blend2d.lib")

using namespace waavs;

    static bool matrixValueNear(
        double a,
        double b,
        double epsilon = 1.0e-12) noexcept
    {
        return std::abs(a - b) <= epsilon;
    }


    static void printWGMatrix(const WGMatrix3x3& m) noexcept
    {
        std::printf(
            "    WG: [% .12f % .12f % .12f]\n"
            "        [% .12f % .12f % .12f]\n"
            "        [% .12f % .12f % .12f]\n",
            m.m00, m.m01, m.m02,
            m.m10, m.m11, m.m12,
            m.m20, m.m21, m.m22);
    }


    static void printBLMatrix(const BLMatrix2D& m) noexcept
    {
        std::printf(
            "    BL: [% .12f % .12f]\n"
            "        [% .12f % .12f]\n"
            "        [% .12f % .12f]\n",
            m.m00, m.m01,
            m.m10, m.m11,
            m.m20, m.m21);
    }


    static bool compareWGAndBL(
        const WGMatrix3x3& wg,
        const BLMatrix2D& bl,
        const char* label,
        double epsilon = 1.0e-12) noexcept
    {
        bool ok =
            matrixValueNear(wg.m00, bl.m00, epsilon) &&
            matrixValueNear(wg.m01, bl.m01, epsilon) &&
            matrixValueNear(wg.m10, bl.m10, epsilon) &&
            matrixValueNear(wg.m11, bl.m11, epsilon) &&
            matrixValueNear(wg.m20, bl.m20, epsilon) &&
            matrixValueNear(wg.m21, bl.m21, epsilon) &&

            // BLMatrix2D is always affine.
            matrixValueNear(wg.m02, 0.0, epsilon) &&
            matrixValueNear(wg.m12, 0.0, epsilon) &&
            matrixValueNear(wg.m22, 1.0, epsilon);

        std::printf("\n%-36s %s\n", label, ok ? "PASS" : "FAIL");

        if (!ok)
        {
            printWGMatrix(wg);
            printBLMatrix(bl);

            std::printf(
                "    difference:\n"
                "        m00=% .12g  m01=% .12g\n"
                "        m10=% .12g  m11=% .12g\n"
                "        m20=% .12g  m21=% .12g\n",
                wg.m00 - bl.m00,
                wg.m01 - bl.m01,
                wg.m10 - bl.m10,
                wg.m11 - bl.m11,
                wg.m20 - bl.m20,
                wg.m21 - bl.m21);
        }

        return ok;
    }


    static void test_WGMatrix3x3_against_BLMatrix2D() noexcept
    {
        std::printf(
            "\n"
            "============================================================\n"
            "WGMatrix3x3 versus BLMatrix2D\n"
            "============================================================\n");

        int passCount = 0;
        int failCount = 0;

        WGMatrix3x3 wg;
        BLMatrix2D bl;

        wg.reset();
        bl.reset();

#define CHECK_MATRIX(label)                                      \
        do                                                       \
        {                                                        \
            if (compareWGAndBL(wg, bl, label))                   \
                ++passCount;                                     \
            else                                                 \
                ++failCount;                                     \
        } while (0)

        CHECK_MATRIX("identity");

        /*
         * Pre-style operations
         *
         * These should correspond directly:
         *
         *     WGMatrix3x3::translate()
         *     BLMatrix2D::translate()
         */

        wg.translate(10.0, 20.0);
        bl.translate(10.0, 20.0);
        CHECK_MATRIX("translate(10, 20)");

        wg.scale(2.0, 3.0);
        bl.scale(2.0, 3.0);
        CHECK_MATRIX("scale(2, 3)");

        wg.rotate(0.25);
        bl.rotate(0.25);
        CHECK_MATRIX("rotate(0.25)");

        wg.skew(0.15, -0.20);
        bl.skew(0.15, -0.20);
        CHECK_MATRIX("skew(0.15, -0.20)");

        wg.rotate(0.40, 30.0, 50.0);
        bl.rotate(0.40, 30.0, 50.0);
        CHECK_MATRIX("rotate(0.40, 30, 50)");

        /*
         * Post-style operations
         */

        wg.postTranslate(-7.0, 13.0);
        bl.post_translate(-7.0, 13.0);
        CHECK_MATRIX("postTranslate(-7, 13)");

        wg.postScale(0.5, 1.25);
        bl.post_scale(0.5, 1.25);
        CHECK_MATRIX("postScale(0.5, 1.25)");

        wg.postRotate(-0.35);
        bl.post_rotate(-0.35);
        CHECK_MATRIX("postRotate(-0.35)");

        wg.postSkew(0.08, 0.12);
        bl.post_skew(0.08, 0.12);
        CHECK_MATRIX("postSkew(0.08, 0.12)");

        wg.postRotate(0.50, -20.0, 40.0);
        bl.post_rotate(0.50, -20.0, 40.0);
        CHECK_MATRIX("postRotate(0.50, -20, 40)");

        /*
         * General matrix composition
         */

        {
            WGMatrix3x3 wgOther;
            wgOther.resetAffine(
                1.2, 0.3,
                -0.4, 0.8,
                17.0, -9.0);

            BLMatrix2D blOther(
                1.2, 0.3,
                -0.4, 0.8,
                17.0, -9.0);

            wg.transform(wgOther);
            bl.transform(blOther);

            CHECK_MATRIX("transform(other)");
        }

        {
            WGMatrix3x3 wgOther;
            wgOther.resetAffine(
                0.75, -0.1,
                0.25, 1.4,
                -12.0, 6.0);

            BLMatrix2D blOther(
                0.75, -0.1,
                0.25, 1.4,
                -12.0, 6.0);

            wg.postTransform(wgOther);
            bl.post_transform(blOther);

            CHECK_MATRIX("postTransform(other)");
        }

        /*
         * Inversion
         */

        {
            WGMatrix3x3 wgInverse = wg;
            BLMatrix2D blInverse = bl;

            bool wgSuccess = wgInverse.invertAffine2D();
            BLResult blResult = blInverse.invert();
            bool blSuccess = blResult == BL_SUCCESS;

            bool successMatches = wgSuccess == blSuccess;

            std::printf(
                "\n%-36s %s\n",
                "inverse success result",
                successMatches ? "PASS" : "FAIL");

            if (successMatches)
                ++passCount;
            else
                ++failCount;

            if (wgSuccess && blSuccess)
            {
                if (compareWGAndBL(
                    wgInverse,
                    blInverse,
                    "inverse matrix"))
                {
                    ++passCount;
                }
                else
                {
                    ++failCount;
                }
            }
        }

        /*
         * Reset constructors
         */

        wg.resetToTranslation(15.0, 25.0);
        bl.reset_to_translation(15.0, 25.0);
        CHECK_MATRIX("resetToTranslation(15, 25)");

        wg.resetToScaling(3.0, 4.0);
        bl.reset_to_scaling(3.0, 4.0);
        CHECK_MATRIX("resetToScaling(3, 4)");

        wg.resetToRotation(0.75);
        bl.reset_to_rotation(0.75);
        CHECK_MATRIX("resetToRotation(0.75)");

        wg.resetToRotation(0.75, 20.0, -10.0);
        bl.reset_to_rotation(0.75, 20.0, -10.0);
        CHECK_MATRIX("resetToRotation(0.75, 20, -10)");

        wg.resetToSkewing(0.30, -0.15);
        bl.reset_to_skewing(0.30, -0.15);
        CHECK_MATRIX("resetToSkewing(0.30, -0.15)");

#undef CHECK_MATRIX

        std::printf(
            "\n"
            "============================================================\n"
            "Matrix comparison results: pass=%d fail=%d\n"
            "============================================================\n",
            passCount,
            failCount);
    }

    static void test_blmatrix_rotation_construction() noexcept
    {
        constexpr double angle = 0.75;
        constexpr double px = 20.0;
        constexpr double py = -10.0;

        // Case 1:
        // Start with identity, then apply a rotation about point (px, py).
        BLMatrix2D applied = BLMatrix2D::make_identity();
        applied.rotate(angle, px, py);

        // Case 2:
        // Construct using make_rotation(angle, x, y).
        //
        // In Blend2D's current implementation, x and y become the direct
        // translation components m20 and m21. They are not treated as a pivot.
        BLMatrix2D made = BLMatrix2D::make_rotation(angle, px, py);

        BLPoint appliedPivot = applied.map_point(px, py);
        BLPoint madePivot = made.map_point(px, py);

        std::printf("\n==== BLMatrix2D rotation construction ====\n");

        std::printf("\nidentity + rotate(angle, px, py):\n");
        std::printf(
            "  [% .12f % .12f]\n"
            "  [% .12f % .12f]\n"
            "  [% .12f % .12f]\n",
            applied.m00, applied.m01,
            applied.m10, applied.m11,
            applied.m20, applied.m21);

        std::printf(
            "  pivot mapped: (% .12f, % .12f)\n",
            appliedPivot.x,
            appliedPivot.y);

        std::printf("\nmake_rotation(angle, px, py):\n");
        std::printf(
            "  [% .12f % .12f]\n"
            "  [% .12f % .12f]\n"
            "  [% .12f % .12f]\n",
            made.m00, made.m01,
            made.m10, made.m11,
            made.m20, made.m21);

        std::printf(
            "  pivot mapped: (% .12f, % .12f)\n",
            madePivot.x,
            madePivot.y);

        bool linearPartsMatch =
            matrixValueNear(applied.m00, made.m00) &&
            matrixValueNear(applied.m01, made.m01) &&
            matrixValueNear(applied.m10, made.m10) &&
            matrixValueNear(applied.m11, made.m11);

        bool translationsMatch =
            matrixValueNear(applied.m20, made.m20) &&
            matrixValueNear(applied.m21, made.m21);

        bool appliedKeepsPivot =
            matrixValueNear(appliedPivot.x, px) &&
            matrixValueNear(appliedPivot.y, py);

        bool madeKeepsPivot =
            matrixValueNear(madePivot.x, px) &&
            matrixValueNear(madePivot.y, py);

        std::printf("\nlinear portions match:       %s\n",
            linearPartsMatch ? "PASS" : "FAIL");

        std::printf("translation portions differ: %s\n",
            !translationsMatch ? "PASS" : "FAIL");

        std::printf("applied rotation keeps pivot: %s\n",
            appliedKeepsPivot ? "PASS" : "FAIL");

        std::printf("make_rotation keeps pivot:    %s\n",
            madeKeepsPivot ? "PASS" : "FAIL");

        std::printf("==== end test ====\n");
    }

int main() noexcept
{
    //test_WGMatrix3x3_against_BLMatrix2D();
    test_blmatrix_rotation_construction();

    return 0;
}