//
// This is a junk file used to try out various things.
// Typically just things that don't require any UI
// tear down and rebuild as needed.
// 


#include "app/mappedfile.h"
#include "bspan.h"
#include "svgtransform.h"
#include "svg/xmlscan.h"
#include "svg/svgcss.h"

//#pragma comment(lib, "blend2d.lib")

using namespace waavs;

#if defined(_M_ARM64)
# pragma message("_M_ARM64 defined")
#endif

#if defined(__aarch64__)
# pragma message("__aarch64__ defined")
#endif

#if defined(_M_X64)
# pragma message("_M_X64 defined")
#endif

#if defined(__x86_64__)
# pragma message("__x86_64__ defined")
#endif

#if defined(__SSE2__)
# pragma message("__SSE2__ defined")
#endif

#if defined(__SSSE3__)
# pragma message("__SSSE3__ defined")
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
# pragma message("__ARM_NEON defined")
#endif

#if WAAVS_HAS_NEON
#pragma message("WAAVS_HAS_NEON")
#endif


#include <cstdio>
#include <cmath>

namespace waavs
{
    static bool nearlyEqual(double a, double b, double eps = 1e-9) noexcept
    {
        return std::abs(a - b) <= eps;
    }

    static bool checkPoint(
        const WGMatrix3x3& m,
        double x, double y,
        double ex, double ey,
        const char* label) noexcept
    {
        WGPointD p = m.mapPoint(x, y);

        bool ok = nearlyEqual(p.x, ex) && nearlyEqual(p.y, ey);

        std::printf(
            "    %-10s in=(%9.4f,%9.4f) actual=(%12.6f,%12.6f) expected=(%12.6f,%12.6f) %s\n",
            label,
            x, y,
            p.x, p.y,
            ex, ey,
            ok ? "PASS" : "FAIL");

        return ok;
    }

    static bool runTransformCase(
        const char* text,
        double x0, double y0, double ex0, double ey0,
        double x1, double y1, double ex1, double ey1,
        double x2, double y2, double ex2, double ey2) noexcept
    {
        WGMatrix3x3 m;
        ByteSpan s(text);

        bool parsed = parseTransform(s, m);

        std::printf("\n%s\n", text);
        std::printf("  parse: %s\n", parsed ? "true" : "false");

        if (!parsed)
            return false;

        bool ok = true;

        ok = checkPoint(m, x0, y0, ex0, ey0, "origin") && ok;
        ok = checkPoint(m, x1, y1, ex1, ey1, "x-axis") && ok;
        ok = checkPoint(m, x2, y2, ex2, ey2, "point") && ok;

        return ok;
    }

    static void test_transform()
    {
        int passCount = 0;
        int failCount = 0;

#define RUN_TRANSFORM_TEST(text, x0, y0, ex0, ey0, x1, y1, ex1, ey1, x2, y2, ex2, ey2) \
        do { \
            bool ok = runTransformCase( \
                text, \
                x0, y0, ex0, ey0, \
                x1, y1, ex1, ey1, \
                x2, y2, ex2, ey2); \
            if (ok) \
                ++passCount; \
            else \
                ++failCount; \
        } while (0)

        RUN_TRANSFORM_TEST(
            "translate(10,20)",
            0, 0, 10, 20,
            1, 0, 11, 20,
            2, 3, 12, 23);

        RUN_TRANSFORM_TEST(
            "translate(10)",
            0, 0, 10, 0,
            1, 0, 11, 0,
            2, 3, 12, 3);

        RUN_TRANSFORM_TEST(
            "scale(2)",
            0, 0, 0, 0,
            1, 0, 2, 0,
            2, 3, 4, 6);

        RUN_TRANSFORM_TEST(
            "scale(2,3)",
            0, 0, 0, 0,
            1, 0, 2, 0,
            2, 3, 4, 9);

        RUN_TRANSFORM_TEST(
            "matrix(1,0,0,1,10,20)",
            0, 0, 10, 20,
            1, 0, 11, 20,
            2, 3, 12, 23);

        RUN_TRANSFORM_TEST(
            "matrix(2,0,0,3,10,20)",
            0, 0, 10, 20,
            1, 0, 12, 20,
            2, 3, 14, 29);

        RUN_TRANSFORM_TEST(
            "rotate(90)",
            0, 0, 0, 0,
            1, 0, 0, 1,
            2, 3, -3, 2);

        RUN_TRANSFORM_TEST(
            "rotate(90,10,20)",
            10, 20, 10, 20,
            11, 20, 10, 21,
            10, 21, 9, 20);

        RUN_TRANSFORM_TEST(
            "skewX(45)",
            0, 0, 0, 0,
            1, 0, 1, 0,
            2, 3, 5, 3);

        RUN_TRANSFORM_TEST(
            "skewY(45)",
            0, 0, 0, 0,
            1, 0, 1, 1,
            2, 3, 2, 5);

        RUN_TRANSFORM_TEST(
            "translate(10,20) scale(2)",
            0, 0, 20, 40,
            1, 0, 22, 40,
            2, 3, 24, 46);

        RUN_TRANSFORM_TEST(
            "scale(2) translate(10,20)",
            0, 0, 10, 20,
            1, 0, 12, 20,
            2, 3, 14, 26);

#undef RUN_TRANSFORM_TEST

        std::printf("\ntransform tests: pass=%d fail=%d\n", passCount, failCount);
    }
}

static void dumpCssAttributes(const XmlAttributeCollection& attrs) noexcept
{
    for (const auto& attr : attrs.values())
    {
        const char* name = attr.first;
        const ByteSpan& value = attr.second;

        printf("[%s] = [%.*s]\n",
            name ? name : "",
            (int)value.size(),
            value.data());
    }
}

void test_css_parse()
{
    ByteSpan css = R"(
fill:red;
stroke:black;
font-family:"Times; New Roman";
transform:matrix(1,0,0,1,10,20);
custom:func("a;b;c", url(foo;bar));
fill: rgba(255, 0, 0, 0.5)
)";

    XmlAttributeCollection attrs;
    gatherCssAttributes(css, attrs);
    dumpCssAttributes(attrs);
}


int main(int argc, char** argv)
{
    printf("MAIN\n");

    //test_transform();
    test_css_parse();

    return 0;
}
