//
// This is a junk file used to try out various things.
// Typically just things that don't require any UI
// tear down and rebuild as needed.
// 


#include "app_mappedfile.h"
#include "lang_span.h"
#include "svgtransform.h"
#include "svg/xmlscan.h"
#include "svg/svgcss.h"

#include "wg_matrix3x3.h"


using namespace waavs;




#include <cstdio>
#include <cmath>



static double round_half_up(double x) { return floor(x + 0.49999999999999994); }

static void test_round()
{
    double t1 = round_half_up(2.5); // 3
    double t2 = round_half_up(2.4); // 2
    double t3 = round_half_up(-2.5); // -2
    double t4 = round_half_up(-2.6); // -3

    printf("round_half_up(2.5) = %f\n", t1);
    printf("round_half_up(2.4) = %f\n", t2);
    printf("round_half_up(-2.5) = %f\n", t3);
    printf("round_half_up(-2.6) = %f\n", t4);
}



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
    attrs.fAttributes.forEach([](InternedKey name, const ByteSpan& value)
        {
            printf("[%s] = [%.*s]\n",
                name ? name : "",
                (int)value.size(),
                value.data());
        });
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
