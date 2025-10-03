
#include "morse.h"
#include "generator.h"
#include "waavsgraph.h"

using namespace waavs;

static void printMorseCode(const char* str)
{

    printf("Morse code for \"%s\":\n", str);

    // get the morese code map
    const auto &map = getMorseCodeMap();

    for (size_t i = 0; str[i]; ++i) 
    {
        char ch = toupper(str[i]);
        printf("%c: ", ch);
        auto it = map.find(ch);
        if (it != map.end()) {
            printf("%s ", it->second);
        }
        else if (ch == ' ') {
            printf(" / "); // word separator
        }
        else {
            printf("? "); // unknown character
        }

        printf("\n");

    }
    printf("\n");
}

struct multiplier : public IParametricSource<double>
{
    double factor;
    multiplier(double f) : factor(f) {}

    double eval(double t) override
    {
        return factor * t;
    }
};


static void test_pipeline()
{
    multiplier m(3.0);

    printf("0.0 : %f\n", m(0.0));
    printf("0.25 : %f\n", m(0.25));
    printf("0.5 : %f\n", m(0.5));
    printf("0.75 : %f\n", m(0.75));
    printf("1.0 : %f\n", m(1.0));


    // get a producer that generates values
}

static void test_graph()
{
    Point2d a{ 0.0, 0.0 };
    Point2d b{ 10.0, 5.0 };

    Point2d c = a.midpoint(b);


    printf("a: (%f, %f)\n", a.x, a.y);
    printf("b: (%f, %f)\n", b.x, b.y);
    printf("c: (%f, %f)\n", c.x, c.y);

    double d = distanceToLine({ 5.0, 3.0 }, {0,0}, {10,0});
    printf("Distance from (5,3) to line ab: %f\n", d);
}

int main(int argc, char** argv)
{
    //printMorseCode("SOS");
    //test_pipeline();
    test_graph();

    return 0;
}
