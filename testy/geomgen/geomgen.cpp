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

#include "gcdl_program_authored_xml_parse.h"
#include "gcdl_program_normalizer.h"
#include "gcdl_program_flattener.h"

#include "app/mappedfile.h"
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

    GCDLPrimitiveNode n;

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



static int test_case_span(const ByteSpan src)
{

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


static const char* gcdl_design_role_name(GCDLDesignRole role)
{
    switch (role) {
    case GCDLDesignRole::None: return "none";
    case GCDLDesignRole::Construction: return "construction";
    case GCDLDesignRole::Finished: return "finished";
    case GCDLDesignRole::VoidBoundary: return "void-boundary";
    case GCDLDesignRole::SolidBoundary: return "solid-boundary";
    case GCDLDesignRole::Decoration: return "decoration";
    case GCDLDesignRole::Dimension: return "dimension";
    case GCDLDesignRole::Label: return "label";
    case GCDLDesignRole::Debug: return "debug";
    default: return "unknown";
    }
}

static const char* gcdl_fab_role_name(GCDLFabricationRole role)
{
    switch (role) {
    case GCDLFabricationRole::None: return "none";
    case GCDLFabricationRole::Cut: return "cut";
    case GCDLFabricationRole::Score: return "score";
    case GCDLFabricationRole::Engrave: return "engrave";
    case GCDLFabricationRole::Drill: return "drill";
    case GCDLFabricationRole::Pocket: return "pocket";
    default: return "unknown";
    }
}

static void printNormalizedNode(const GCDLNormalizedNode& n, size_t index)
{
    std::printf(
        "[%zu] id=%s op=%s refs=%zu nums=%zu\n",
        index,
        n.id ? n.id : "(null)",
        n.op ? n.op : "(null)",
        n.refs.size(),
        n.nums.size());

    std::printf(
        "    meta: step=%s motif=%s source=%s design=%s fab=%s\n",
        n.meta.sourceStep ? n.meta.sourceStep : "(none)",
        n.meta.sourceMotif ? n.meta.sourceMotif : "(none)",
        n.meta.sourceNode ? n.meta.sourceNode : "(none)",
        gcdl_design_role_name(n.meta.designRole),
        gcdl_fab_role_name(n.meta.fabricationRole));

    for (size_t r = 0; r < n.refs.size(); ++r) {
        std::printf(
            "    ref[%zu] = %s[%u]\n",
            r,
            n.refs[r].id ? n.refs[r].id : "(null)",
            n.refs[r].slot);
    }

    for (size_t a = 0; a < n.nums.size(); ++a) {
        std::printf(
            "    num[%zu] = %.6f\n",
            a,
            n.nums[a]);
    }
}


static int test_case_normalized(const ByteSpan src)
{
    GCDLAuthoredProgram authored;
    if (!gcdl_create_authored_program_from_xml(src, authored)) {
        std::printf("authored xml parse failed\n");
        return 1;
    }

    GCDLNormalizedProgram normalized;
    if (!gcdl_normalize_program(authored, normalized)) {
        std::printf("normalize failed\n");
        return 1;
    }

    std::printf("normalized nodes: %zu\n", normalized.nodes.size());

    for (size_t i = 0; i < normalized.nodes.size(); ++i) {
        const GCDLNormalizedNode& n = normalized.nodes[i];

        printNormalizedNode(n, i);
    }

    return 0;
}

static int test_case_flattened(const ByteSpan src)
{
    GCDLAuthoredProgram authored;
    if (!gcdl_create_authored_program_from_xml(src, authored)) {
        std::printf("authored xml parse failed\n");
        return 1;
    }

    GCDLNormalizedProgram normalized;
    if (!gcdl_normalize_program(authored, normalized)) {
        std::printf("normalize failed\n");
        return 1;
    }

    GCDLProgram primitive;
    if (!gcdl_flatten_program(normalized, primitive)) {
        std::printf("flatten failed\n");
        return 1;
    }

    std::printf("normalized nodes: %zu\n", normalized.nodes.size());
    std::printf("primitive nodes: %zu\n", primitive.nodes.size());

    GCDLResolveResult rr = resolveGCDLProgram(primitive);
    if (!rr.ok) {
        std::printf("resolve failed\n");
        return 1;
    }

    GCDLEvalContext ctx;
    if (!evalGCDLProgram(primitive, ctx)) {
        std::printf("eval failed\n");
        return 1;
    }

    if (!writeGCDLSVG("gcdl_testcase_flattened.svg", ctx)) {
        std::printf("svg export failed\n");
        return 1;
    }

    std::printf("wrote gcdl_testcase_flattened.svg\n");

    return 0;
}


int main(int argc, char **argv)
{
    if (argc > 1) {
        const char* filename = argv[1];
        auto mapped = MappedFile::create_shared(filename);


        if (nullptr == mapped)
            return 0;

        ByteSpan src;
        src.resetFromSize(mapped->data(), mapped->size());

        //test_case_span(src);
        //test_case_normalized(src);
        test_case_flattened(src);

        return 1;
    }

    // Using the API directly
    //test_case_01();
    //test_case_02_xml();

    return 0;
}