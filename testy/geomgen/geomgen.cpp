/*
gcdl_types.h
gcdl_program.h
gcdl_parser.h


gcdl_svg_export.h
*/

#include "gcdl_resolver.h"
#include "gcdl_eval.h"
#include "gcdl_svg_export.h"
#include "gcdl_parse_xml.h"

#include <cstdio>

using namespace waavs;

static void printPoint(const char* name, const GCDLPoint& p)
{
    std::printf("%s: %.3f %.3f\n", name, p.x, p.y);
}

static void printCircle(const char* name, const GCDLCircle& c)
{
    std::printf("%s: center %.3f %.3f radius %.3f\n",
        name, c.c.x, c.c.y, c.r);
}

static int test_case_01()
{
    GCDLProgram prog;

    InternedKey opPointXY = PSNameTable::INTERN("point.xy");
    InternedKey opLineFromPoints = PSNameTable::INTERN("line.fromPoints");
    InternedKey opLineDivide = PSNameTable::INTERN("line.divide");
    InternedKey opCircleCenterPoint = PSNameTable::INTERN("circle.centerPoint");
    InternedKey opIntersectCircleCircle = PSNameTable::INTERN("intersect.circleCircle");
    InternedKey opArcCircleFromTo = PSNameTable::INTERN("arc.circleFromTo");

    InternedKey A = PSNameTable::INTERN("A");
    InternedKey B = PSNameTable::INTERN("B");
    InternedKey base = PSNameTable::INTERN("base");
    InternedKey baseParts = PSNameTable::INTERN("baseParts");
    InternedKey leftCircle = PSNameTable::INTERN("leftCircle");
    InternedKey rightCircle = PSNameTable::INTERN("rightCircle");
    InternedKey apexCandidates = PSNameTable::INTERN("apexCandidates");
    InternedKey leftArc = PSNameTable::INTERN("leftArc");
    InternedKey rightArc = PSNameTable::INTERN("rightArc");

    GCDLNode n;

    n = {};
    n.id = A;
    n.op = opPointXY;
    n.nums = { 0.0, 0.0 };
    prog.addNode(n);

    n = {};
    n.id = B;
    n.op = opPointXY;
    n.nums = { 800.0, 0.0 };
    prog.addNode(n);

    n = {};
    n.id = base;
    n.op = opLineFromPoints;
    n.refs = { { A, 0 }, { B, 0 } };
    prog.addNode(n);

    n = {};
    n.id = baseParts;
    n.op = opLineDivide;
    n.refs = { { base, 0 } };
    n.nums = { 8.0 };
    prog.addNode(n);

    // Equilateral pointed arch:
    // The left visible arc runs from A to the upper intersection,
    // but its circle center is B.
    n = {};
    n.id = leftCircle;
    n.op = opCircleCenterPoint;
    n.refs = { { B, 0 }, { A, 0 } };
    prog.addNode(n);

    // The right visible arc runs from the upper intersection to B,
    // but its circle center is A.
    n = {};
    n.id = rightCircle;
    n.op = opCircleCenterPoint;
    n.refs = { { A, 0 }, { B, 0 } };
    prog.addNode(n);

    n = {};
    n.id = apexCandidates;
    n.op = opIntersectCircleCircle;
    n.refs = { { leftCircle, 0 }, { rightCircle, 0 } };
    prog.addNode(n);

    // Use apexCandidates[0] if your circle-circle function returns the upper
    // point first. If it returns the lower point first, change this slot to 1.
    const uint32_t apexSlot = 0;

    n = {};
    n.id = leftArc;
    n.op = opArcCircleFromTo;
    n.refs = { { leftCircle, 0 }, { A, 0 }, { apexCandidates, apexSlot } };
    n.nums = { 0.0 };
    prog.addNode(n);

    n = {};
    n.id = rightArc;
    n.op = opArcCircleFromTo;
    n.refs = { { rightCircle, 0 }, { apexCandidates, apexSlot }, { B, 0 } };
    n.nums = { 0.0 };
    prog.addNode(n);

    GCDLResolveResult rr = resolveGCDLProgram(prog);
    if (!rr.ok) {
        std::printf("resolve failed\n");
        return 1;
    }

    GCDLEvalContext ctx;
    if (!evalGCDLProgram(prog, ctx)) {
        std::printf("eval failed\n");
        return 1;
    }

    GCDLPoint p;
    GCDLCircle c;
    GCDLArc a;

    if (ctx.getPoint({ A, 0 }, p))
        printPoint("A", p);

    if (ctx.getPoint({ B, 0 }, p))
        printPoint("B", p);

    if (ctx.getPoint({ baseParts, 3 }, p))
        printPoint("baseParts[3]", p);

    if (ctx.getPoint({ baseParts, 5 }, p))
        printPoint("baseParts[5]", p);

    if (ctx.getPoint({ apexCandidates, 0 }, p))
        printPoint("apexCandidates[0]", p);

    if (ctx.getPoint({ apexCandidates, 1 }, p))
        printPoint("apexCandidates[1]", p);

    if (ctx.getCircle({ leftCircle, 0 }, c))
        printCircle("leftCircle", c);

    if (ctx.getCircle({ rightCircle, 0 }, c))
        printCircle("rightCircle", c);

    if (ctx.getArc({ leftArc, 0 }, a)) {
        std::printf(
            "leftArc: center %.3f %.3f radius %.3f angles %.3f %.3f ccw %d\n",
            a.c.x, a.c.y, a.r, a.a0, a.a1, a.ccw ? 1 : 0);
    }

    if (ctx.getArc({ rightArc, 0 }, a)) {
        std::printf(
            "rightArc: center %.3f %.3f radius %.3f angles %.3f %.3f ccw %d\n",
            a.c.x, a.c.y, a.r, a.a0, a.a1, a.ccw ? 1 : 0);
    }

    if (!writeGCDLSVG("gcdl_arch_test.svg", ctx)) {
        std::printf("svg export failed\n");
        return 1;
    }

    std::printf("wrote gcdl_arch_test.svg\n");

    return 0;
}

static int test_case_02_xml()
{
    static const char* xmlText =
        "<gcdl version=\"1.0\" name=\"equilateral-pointed-arch\">"
        "  <nodes>"
        "    <point id=\"A\" x=\"0\" y=\"0\"/>"
        "    <point id=\"B\" x=\"800\" y=\"0\"/>"
        "    <line id=\"base\" from=\"A\" to=\"B\"/>"
        "    <divide id=\"baseParts\" line=\"base\" parts=\"8\"/>"
        "    <circle id=\"leftCircle\" center=\"B\" through=\"A\"/>"
        "    <circle id=\"rightCircle\" center=\"A\" through=\"B\"/>"
        "    <intersect id=\"apexCandidates\" kind=\"circleCircle\" a=\"leftCircle\" b=\"rightCircle\"/>"
        "    <arc id=\"leftArc\" circle=\"leftCircle\" from=\"A\" to=\"apexCandidates[0]\" ccw=\"false\"/>"
        "    <arc id=\"rightArc\" circle=\"rightCircle\" from=\"apexCandidates[0]\" to=\"B\" ccw=\"false\"/>"
        "  </nodes>"
        "</gcdl>";

    ByteSpan src;
    src.resetFromSize((const unsigned char*)xmlText, strlen(xmlText));

    GCDLProgram prog;
    if (!parseGCDLDocument(src, prog)) {
        std::printf("parse failed\n");
        return 1;
    }

    GCDLResolveResult rr = resolveGCDLProgram(prog);
    if (!rr.ok) {
        std::printf("resolve failed\n");
        return 1;
    }

    GCDLEvalContext ctx;
    if (!evalGCDLProgram(prog, ctx)) {
        std::printf("eval failed\n");
        return 1;
    }

    GCDLPoint p;
    GCDLCircle c;
    GCDLArc a;

    InternedKey A = PSNameTable::INTERN("A");
    InternedKey B = PSNameTable::INTERN("B");
    InternedKey baseParts = PSNameTable::INTERN("baseParts");
    InternedKey apexCandidates = PSNameTable::INTERN("apexCandidates");
    InternedKey leftCircle = PSNameTable::INTERN("leftCircle");
    InternedKey rightCircle = PSNameTable::INTERN("rightCircle");
    InternedKey leftArc = PSNameTable::INTERN("leftArc");
    InternedKey rightArc = PSNameTable::INTERN("rightArc");

    if (ctx.getPoint({ A, 0 }, p))
        printPoint("A", p);

    if (ctx.getPoint({ B, 0 }, p))
        printPoint("B", p);

    if (ctx.getPoint({ baseParts, 3 }, p))
        printPoint("baseParts[3]", p);

    if (ctx.getPoint({ baseParts, 5 }, p))
        printPoint("baseParts[5]", p);

    if (ctx.getPoint({ apexCandidates, 0 }, p))
        printPoint("apexCandidates[0]", p);

    if (ctx.getPoint({ apexCandidates, 1 }, p))
        printPoint("apexCandidates[1]", p);

    if (ctx.getCircle({ leftCircle, 0 }, c))
        printCircle("leftCircle", c);

    if (ctx.getCircle({ rightCircle, 0 }, c))
        printCircle("rightCircle", c);

    if (ctx.getArc({ leftArc, 0 }, a)) {
        std::printf(
            "leftArc: center %.3f %.3f radius %.3f angles %.3f %.3f ccw %d\n",
            a.c.x, a.c.y, a.r, a.a0, a.a1, a.ccw ? 1 : 0);
    }

    if (ctx.getArc({ rightArc, 0 }, a)) {
        std::printf(
            "rightArc: center %.3f %.3f radius %.3f angles %.3f %.3f ccw %d\n",
            a.c.x, a.c.y, a.r, a.a0, a.a1, a.ccw ? 1 : 0);
    }

    if (!writeGCDLSVG("gcdl_arch_xml_test.svg", ctx)) {
        std::printf("svg export failed\n");
        return 1;
    }

    std::printf("wrote gcdl_arch_xml_test.svg\n");

    return 0;
}

static int test_case_horseshoe_arch()
{
    static const char* xmlText = R"||(
<gcdl version="1.0" name="horseshoe-arch">
  <nodes>
    <point id="A" x="0" y="0"/>
    <point id="B" x="800" y="0"/>

    <line id="base" from="A" to="B"/>

    <!-- centers moved inward -->
    <divide id="baseParts" line="base" parts="10"/>

    <circle id="leftCircle"
            center="baseParts[3]"
            through="A"/>

    <circle id="rightCircle"
            center="baseParts[7]"
            through="B"/>

    <intersect id="apex"
               kind="circleCircle"
               a="leftCircle"
               b="rightCircle"/>

    <arc id="leftArc"
         circle="leftCircle"
         from="A"
         to="apex[0]"
         ccw="false"/>

    <arc id="rightArc"
         circle="rightCircle"
         from="apex[0]"
         to="B"
         ccw="false"/>

  </nodes>
</gcdl>
)||";

    ByteSpan src;
    src.resetFromSize((const unsigned char*)xmlText, strlen(xmlText));

    GCDLProgram prog;
    if (!parseGCDLDocument(src, prog)) {
        std::printf("parse failed\n");
        return 1;
    }

    // Check to make sure it's a valid program before trying to evaluate it. 
    GCDLResolveResult rr = resolveGCDLProgram(prog);
    if (!rr.ok) {
        std::printf("resolve failed\n");
        return 1;
    }

    GCDLEvalContext ctx;
    if (!evalGCDLProgram(prog, ctx)) {
        std::printf("eval failed\n");
        return 1;
    }



    if (!writeGCDLSVG("gcdl_horseshoe_arch.svg", ctx)) {
        std::printf("svg export failed\n");
        return 1;
    }

    std::printf("wrote gcdl_horseshoe_arch.svg.svg\n");

    return 0;
}

static int test_case_sixpointstar()
{
    static const char* xmlText = R"||(
<gcdl version="1.0" name="six-point-star">

  <nodes>

    <!-- Main circle -->
    <point id="C" x="0" y="0"/>
    <point id="P0" x="400" y="0"/>

    <circle id="mainCircle"
            center="C"
            through="P0"/>

    <!-- Step around the circumference using equal circles -->

    <circle id="c1"
            center="P0"
            through="C"/>

    <intersect id="v01"
               kind="circleCircle"
               a="mainCircle"
               b="c1"/>

    <!-- Top hexagon point -->
    <line id="hexSide"
          from="v01[0]"
          to="v01[1]"/>

  </nodes>

</gcdl>
)||";

    ByteSpan src;
    src.resetFromSize((const unsigned char*)xmlText, strlen(xmlText));

    GCDLProgram prog;
    if (!parseGCDLDocument(src, prog)) {
        std::printf("parse failed\n");
        return 1;
    }

    // Check to make sure it's a valid program before trying to evaluate it. 
    GCDLResolveResult rr = resolveGCDLProgram(prog);
    if (!rr.ok) {
        std::printf("resolve failed\n");
        return 1;
    }

    GCDLEvalContext ctx;
    if (!evalGCDLProgram(prog, ctx)) {
        std::printf("eval failed\n");
        return 1;
    }



    if (!writeGCDLSVG("gcdl_sixpointstar.svg", ctx)) {
        std::printf("svg export failed\n");
        return 1;
    }

    std::printf("wrote gcdl_sixpointstar.svg\n");

    return 0;
}


static int test_case_tall_pointed_arch()
{
    static const char* xmlText = R"||(
<gcdl version="1.0" name="tall-pointed-arch">
  <nodes>
    <point id="A" x="0" y="0"/>
    <point id="B" x="1000" y="0"/>

    <line id="base" from="A" to="B"/>
    <divide id="baseParts" line="base" parts="12"/>

    <circle id="leftCircle" center="baseParts[10]" through="A"/>
    <circle id="rightCircle" center="baseParts[2]" through="B"/>

    <intersect id="apex"
               kind="circleCircle"
               a="leftCircle"
               b="rightCircle"/>

    <arc id="leftArc"
         circle="leftCircle"
         from="A"
         to="apex[0]"
         ccw="false"/>

    <arc id="rightArc"
         circle="rightCircle"
         from="apex[0]"
         to="B"
         ccw="false"/>
  </nodes>
</gcdl>
)||";

    ByteSpan src;
    src.resetFromSize((const unsigned char*)xmlText, strlen(xmlText));

    GCDLProgram prog;
    if (!parseGCDLDocument(src, prog)) {
        std::printf("parse failed\n");
        return 1;
    }

    // Check to make sure it's a valid program before trying to evaluate it. 
    GCDLResolveResult rr = resolveGCDLProgram(prog);
    if (!rr.ok) {
        std::printf("resolve failed\n");
        return 1;
    }

    GCDLEvalContext ctx;
    if (!evalGCDLProgram(prog, ctx)) {
        std::printf("eval failed\n");
        return 1;
    }



    if (!writeGCDLSVG("gcdl_tallpointedarch.svg", ctx)) {
        std::printf("svg export failed\n");
        return 1;
    }

    std::printf("wrote gcdl_tallpointedarch.svg\n");

    return 0;
}

static int test_case_00(const ByteSpan &src)
{
    //ByteSpan src;
    //src.resetFromSize((const unsigned char*)xmlText, strlen(xmlText));

    GCDLProgram prog;
    if (!parseGCDLDocument(src, prog)) {
        std::printf("parse failed\n");
        return 1;
    }

    // Check to make sure it's a valid program before trying to evaluate it. 
    GCDLResolveResult rr = resolveGCDLProgram(prog);
    if (!rr.ok) {
        std::printf("resolve failed\n");
        return 1;
    }

    GCDLEvalContext ctx;
    if (!evalGCDLProgram(prog, ctx)) {
        std::printf("eval failed\n");
        return 1;
    }

    if (!writeGCDLSVG("gcdl_testcase_00.svg", ctx)) {
        std::printf("svg export failed\n");
        return 1;
    }

    std::printf("wrote gcdl_testcase_00.svg\n");

    return 0;
}

static const char* mirroredVesica = R"||(
<gcdl version="1.0" name="mirrored-vesica-rosette">
  <nodes>
    <point id="H0" x="-600" y="0"/>
    <point id="H1" x="600" y="0"/>
    <point id="V0" x="0" y="-600"/>
    <point id="V1" x="0" y="600"/>

    <line id="hAxis" from="H0" to="H1"/>
    <line id="vAxis" from="V0" to="V1"/>

    <intersect id="C" kind="lineLine" a="hAxis" b="vAxis"/>

    <point id="leftCenter" x="-250" y="0"/>
    <point id="radiusPoint" x="0" y="300"/>

    <mirror-point id="rightCenter" point="leftCenter" axis="vAxis"/>

    <circle id="leftCircle" center="leftCenter" through="radiusPoint"/>
    <circle id="rightCircle" center="rightCenter" through="radiusPoint"/>

    <intersect id="lensPoints"
               kind="circleCircle"
               a="leftCircle"
               b="rightCircle"/>

    <arc id="leftPetal"
         circle="leftCircle"
         from="lensPoints[0]"
         to="lensPoints[1]"
         ccw="true"/>

    <mirror-arc id="rightPetal"
                arc="leftPetal"
                axis="vAxis"/>

    <mirror-arc id="bottomLeftPetal"
                arc="leftPetal"
                axis="hAxis"/>

    <mirror-arc id="bottomRightPetal"
                arc="rightPetal"
                axis="hAxis"/>
  </nodes>
</gcdl>
)||";


static const char* horizontallens = R"||(
<gcdl version="1.0" name="four-petal-rosette">
  <nodes>
    <point id="H0" x="-600" y="0"/>
    <point id="H1" x="600" y="0"/>
    <point id="V0" x="0" y="-600"/>
    <point id="V1" x="0" y="600"/>

    <line id="hAxis" from="H0" to="H1"/>
    <line id="vAxis" from="V0" to="V1"/>

    <intersect id="C" kind="lineLine" a="hAxis" b="vAxis"/>

    <point id="topCenter" x="0" y="250"/>
    <point id="radiusPoint" x="300" y="0"/>

    <mirror-point id="bottomCenter" point="topCenter" axis="hAxis"/>

    <circle id="topCircle" center="topCenter" through="radiusPoint"/>
    <circle id="bottomCircle" center="bottomCenter" through="radiusPoint"/>

    <intersect id="sidePoints" kind="circleCircle" a="topCircle" b="bottomCircle"/>

    <arc id="topLeftArc"
         circle="topCircle"
         from="sidePoints[0]"
         to="sidePoints[1]"
         ccw="true"/>

    <mirror-arc id="topRightArc" arc="topLeftArc" axis="vAxis"/>
    <mirror-arc id="bottomLeftArc" arc="topLeftArc" axis="hAxis"/>
    <mirror-arc id="bottomRightArc" arc="topRightArc" axis="hAxis"/>
  </nodes>
</gcdl>
)||";

static const char* fourpetalrosette = R"||(
<gcdl version="1.0" name="four-petal-rosette">
  <nodes>
    <point id="C" x="0" y="0"/>

    <point id="N" x="0" y="250"/>
    <point id="S" x="0" y="-250"/>
    <point id="E" x="250" y="0"/>
    <point id="W" x="-250" y="0"/>

    <line id="hAxis" from="W" to="E"/>
    <line id="vAxis" from="S" to="N"/>

    <circle id="cN" center="N" through="C"/>
    <circle id="cS" center="S" through="C"/>
    <circle id="cE" center="E" through="C"/>
    <circle id="cW" center="W" through="C"/>

    <intersect id="neCandidates" kind="circleCircle" a="cN" b="cE"/>
    <intersect id="nwCandidates" kind="circleCircle" a="cN" b="cW"/>
    <intersect id="seCandidates" kind="circleCircle" a="cS" b="cE"/>
    <intersect id="swCandidates" kind="circleCircle" a="cS" b="cW"/>

    <point-pick id="NE" from="neCandidates" choose="upper"/>
    <point-pick id="NW" from="nwCandidates" choose="upper"/>
    <point-pick id="SE" from="seCandidates" choose="lower"/>
    <point-pick id="SW" from="swCandidates" choose="lower"/>

    <arc id="topPetalLeft" circle="cN" from="NW" to="C" ccw="false"/>
    <arc id="topPetalRight" circle="cN" from="C" to="NE" ccw="false"/>

    <arc id="rightPetalTop" circle="cE" from="NE" to="C" ccw="false"/>
    <arc id="rightPetalBottom" circle="cE" from="C" to="SE" ccw="false"/>

    <arc id="bottomPetalRight" circle="cS" from="SE" to="C" ccw="false"/>
    <arc id="bottomPetalLeft" circle="cS" from="C" to="SW" ccw="false"/>

    <arc id="leftPetalBottom" circle="cW" from="SW" to="C" ccw="false"/>
    <arc id="leftPetalTop" circle="cW" from="C" to="NW" ccw="false"/>
  </nodes>
</gcdl>
)||";

static const char* tenpointradialstarseed = R"||(
<gcdl version="1.0" name="ten-point-radial-star-seed">
  <nodes>
    <point id="C" x="0" y="0"/>

    <point-polar id="P0" center="C" radius="350" angle="90"/>
    <point-polar id="P1" center="C" radius="350" angle="126"/>
    <point-polar id="P2" center="C" radius="350" angle="162"/>
    <point-polar id="P3" center="C" radius="350" angle="198"/>
    <point-polar id="P4" center="C" radius="350" angle="234"/>
    <point-polar id="P5" center="C" radius="350" angle="270"/>
    <point-polar id="P6" center="C" radius="350" angle="306"/>
    <point-polar id="P7" center="C" radius="350" angle="342"/>
    <point-polar id="P8" center="C" radius="350" angle="18"/>
    <point-polar id="P9" center="C" radius="350" angle="54"/>

    <line id="L0" from="P0" to="P2"/>
    <line id="L1" from="P1" to="P3"/>
    <line id="L2" from="P2" to="P4"/>
    <line id="L3" from="P3" to="P5"/>
    <line id="L4" from="P4" to="P6"/>
    <line id="L5" from="P5" to="P7"/>
    <line id="L6" from="P6" to="P8"/>
    <line id="L7" from="P7" to="P9"/>
    <line id="L8" from="P8" to="P0"/>
    <line id="L9" from="P9" to="P1"/>
  </nodes>
</gcdl>
)||";


static const char* decagonradialstar = R"||(
<gcdl version="1.0" name="decagon-radial-star">
  <nodes>
    <point id="C" x="0" y="0"/>

    <polygon-regular id="outerDecagon"
                     center="C"
                     radius="360"
                     sides="10"
                     start-angle="90"/>

    <polygon-regular id="innerDecagon"
                     center="C"
                     radius="210"
                     sides="10"
                     start-angle="108"/>

    <path-from-points id="outerPath"
                      points="outerDecagon"
                      closed="true"/>

    <path-from-points id="innerPath"
                      points="innerDecagon"
                      closed="true"/>

    <point-polar id="P0" center="C" radius="420" angle="90"/>
    <point-polar id="P1" center="C" radius="420" angle="126"/>
    <point-polar id="P2" center="C" radius="420" angle="162"/>
    <point-polar id="P3" center="C" radius="420" angle="198"/>
    <point-polar id="P4" center="C" radius="420" angle="234"/>

    <line-from-point-angle id="ray90"  point="C" angle="90"  length="430"/>
    <line-from-point-angle id="ray126" point="C" angle="126" length="430"/>
    <line-from-point-angle id="ray162" point="C" angle="162" length="430"/>
    <line-from-point-angle id="ray198" point="C" angle="198" length="430"/>
    <line-from-point-angle id="ray234" point="C" angle="234" length="430"/>

    <line id="starChord0" from="outerDecagon[0]" to="outerDecagon[3]"/>
    <line id="starChord1" from="outerDecagon[1]" to="outerDecagon[4]"/>
    <line id="starChord2" from="outerDecagon[2]" to="outerDecagon[5]"/>
    <line id="starChord3" from="outerDecagon[3]" to="outerDecagon[6]"/>
    <line id="starChord4" from="outerDecagon[4]" to="outerDecagon[7]"/>
    <line id="starChord5" from="outerDecagon[5]" to="outerDecagon[8]"/>
    <line id="starChord6" from="outerDecagon[6]" to="outerDecagon[9]"/>
    <line id="starChord7" from="outerDecagon[7]" to="outerDecagon[0]"/>
    <line id="starChord8" from="outerDecagon[8]" to="outerDecagon[1]"/>
    <line id="starChord9" from="outerDecagon[9]" to="outerDecagon[2]"/>

    <intersect id="centerCheck" kind="lineLine" a="ray90" b="starChord2"/>
  </nodes>
</gcdl>
)||";


static const char* decagramstar = R"||(
<gcdl version="1.0" name="decagram-star">
  <nodes>
    <point id="C" x="0" y="0"/>

    <polygon-regular id="decagon"
                     center="C"
                     radius="360"
                     sides="10"
                     start-angle="90"/>

    <path-from-points id="decagonPath"
                      points="decagon"
                      closed="true"/>

    <polygon-star id="decagram"
                  center="C"
                  radius="360"
                  points="10"
                  skip="3"
                  start-angle="90"/>

    <path-from-points id="decagramPath"
                      points="decagram"
                      closed="true"/>
  </nodes>
</gcdl>
)||";

static const char* pointproject = R"||(
<gcdl version="1.0" name="point-project-test">
  <nodes>
    <point id="A" x="-300" y="0"/>
    <point id="B" x="300" y="0"/>
    <point id="P" x="100" y="250"/>

    <line id="base" from="A" to="B"/>

    <point-project id="foot" point="P" line="base"/>
    <line id="drop" from="P" to="foot"/>
  </nodes>
</gcdl>
)||";


static const char* eightpointjaalicell = R"||(
<gcdl version="1.0" name="eight-point-jaali-cell">
  <nodes>
    <point id="C" x="0" y="0"/>

    <polygon-regular id="outerSquare"
                     center="C"
                     radius="500"
                     sides="4"
                     start-angle="45"/>

    <path-from-points id="outerSquarePath"
                      points="outerSquare"
                      closed="true"/>

    <polygon-regular id="outerOctagon"
                     center="C"
                     radius="420"
                     sides="8"
                     start-angle="22.5"/>

    <path-from-points id="outerOctagonPath"
                      points="outerOctagon"
                      closed="true"/>

    <polygon-star id="eightStar"
                  center="C"
                  radius="360"
                  points="8"
                  skip="3"
                  start-angle="90"/>

    <path-from-points id="eightStarPath"
                      points="eightStar"
                      closed="true"/>

    <polygon-regular id="innerOctagon"
                     center="C"
                     radius="180"
                     sides="8"
                     start-angle="22.5"/>

    <path-from-points id="innerOctagonPath"
                      points="innerOctagon"
                      closed="true"/>

    <line-from-point-angle id="ray0" point="C" angle="0" length="500"/>
    <line-from-point-angle id="ray45" point="C" angle="45" length="500"/>
    <line-from-point-angle id="ray90" point="C" angle="90" length="500"/>
    <line-from-point-angle id="ray135" point="C" angle="135" length="500"/>
    <line-from-point-angle id="ray180" point="C" angle="180" length="500"/>
    <line-from-point-angle id="ray225" point="C" angle="225" length="500"/>
    <line-from-point-angle id="ray270" point="C" angle="270" length="500"/>
    <line-from-point-angle id="ray315" point="C" angle="315" length="500"/>
  </nodes>
</gcdl>
)||";

static const char* tenfoldrosette = R"||(
<gcdl version="1.0" name="ten-fold-rosette">
  <nodes>
    <point id="C" x="0" y="0"/>

    <polygon-regular id="outerDecagon"
                     center="C"
                     radius="420"
                     sides="10"
                     start-angle="90"/>

    <path-from-points id="outerDecagonPath"
                      points="outerDecagon"
                      closed="true"/>

    <polygon-star id="mainDecagram"
                  center="C"
                  radius="420"
                  points="10"
                  skip="3"
                  start-angle="90"/>

    <path-from-points id="mainDecagramPath"
                      points="mainDecagram"
                      closed="true"/>

    <polygon-regular id="innerDecagon"
                     center="C"
                     radius="210"
                     sides="10"
                     start-angle="108"/>

    <path-from-points id="innerDecagonPath"
                      points="innerDecagon"
                      closed="true"/>

    <polygon-star id="innerStar"
                  center="C"
                  radius="210"
                  points="10"
                  skip="3"
                  start-angle="108"/>

    <path-from-points id="innerStarPath"
                      points="innerStar"
                      closed="true"/>

    <circle-center-radius id="outerGuide" center="C" radius="420"/>
    <circle-center-radius id="middleGuide" center="C" radius="315"/>
    <circle-center-radius id="innerGuide" center="C" radius="210"/>

    <line-from-point-angle id="ray0"   point="C" angle="0"   length="440"/>
    <line-from-point-angle id="ray36"  point="C" angle="36"  length="440"/>
    <line-from-point-angle id="ray72"  point="C" angle="72"  length="440"/>
    <line-from-point-angle id="ray108" point="C" angle="108" length="440"/>
    <line-from-point-angle id="ray144" point="C" angle="144" length="440"/>
    <line-from-point-angle id="ray180" point="C" angle="180" length="440"/>
    <line-from-point-angle id="ray216" point="C" angle="216" length="440"/>
    <line-from-point-angle id="ray252" point="C" angle="252" length="440"/>
    <line-from-point-angle id="ray288" point="C" angle="288" length="440"/>
    <line-from-point-angle id="ray324" point="C" angle="324" length="440"/>

    <arc-center-angles id="ringArc0" center="C" radius="315" start-angle="0" end-angle="36" ccw="true"/>
    <arc-center-angles id="ringArc1" center="C" radius="315" start-angle="72" end-angle="108" ccw="true"/>
    <arc-center-angles id="ringArc2" center="C" radius="315" start-angle="144" end-angle="180" ccw="true"/>
    <arc-center-angles id="ringArc3" center="C" radius="315" start-angle="216" end-angle="252" ccw="true"/>
    <arc-center-angles id="ringArc4" center="C" radius="315" start-angle="288" end-angle="324" ccw="true"/>
  </nodes>
</gcdl>
)||";


int main()
{
    //test_case_01();
    //test_case_02_xml();
    //test_case_horseshoe_arch();
    //test_case_sixpointstar();
    //test_case_tall_pointed_arch();
    //test_case_00(mirroredVesica);
    //test_case_00(horizontallens);
    //test_case_00(fourpetalrosette);
    //test_case_00(tenpointradialstarseed);
    //test_case_00(decagonradialstar);
    //test_case_00(decagramstar);
    //test_case_00(pointproject);
    //test_case_00(eightpointjaalicell);
    test_case_00(tenfoldrosette);

    return 0;
}