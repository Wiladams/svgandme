
#include <string>
#include <iostream>


#include "gcodeprogram.h"
#include "gcodeprogram_builder.h"
#include "gcodeprogram_dispatch.h"
#include "gcodeprogram_text_sink_debug.h"


#include "pathattribute_parser.h"
#include "pathprogram_builder.h"
#include "toolpathpolicy_router.h"
#include "pathprogram_to_gcodeprogram.h"

#include "dragknife_compensate_sink.h"
#include "toolpathpolicy_dragknife.h"



using namespace waavs;

// Helper function to convert a PathProgram to a GCodeProgram using 
// the RouterToolPathPolicy.
static INLINE bool pathprogram_to_gcodeprogram_router(
	const PathProgram& src,
	GCodeProgram& dst,
	const RouterToolPathOptions& opt = RouterToolPathOptions{},
	double flatness = 0.25,
	int maxDepth = 16) noexcept
{
	dst.clear();

	GCodeProgramBuilder builder(dst);
	RouterToolPathPolicy policy(builder, opt);

	return pathprogram_to_gcodeprogram(src, policy, flatness, maxDepth);
}




namespace waavs
{
    static void test_dragknife_sink_pipeline() noexcept
    {
        // A star with strong corners plus a zig-zag open subpath.
        const char* pathText =
            "M 100 10 "
            "L 123 78 "
            "L 195 78 "
            "L 136 122 "
            "L 158 190 "
            "L 100 148 "
            "L 42 190 "
            "L 64 122 "
            "L 5 78 "
            "L 77 78 "
            "Z "
            "M 230 40 "
            "L 280 80 "
            "L 240 120 "
            "L 290 160 "
            "L 250 200";

        PathProgram src;
        if (!parsePathProgram(pathText, src)) {
            std::cout << "Failed to parse SVG path\n";
            return;
        }

        // ------------------------------------------------------------
        // Stage 1:
        // source PathProgram -> flattener -> dragknife compensator -> PathProgram
        // ------------------------------------------------------------
        PathProgram compensated;

        PathProgramBuilder compensatedBuilder;
        compensatedBuilder.reserve(src.ops.size() * 4u, src.args.size() * 8u);

        DragKnifeCompensator<PathProgramBuilder> comp(compensatedBuilder);
        comp.opt.bladeOffset = 8.0;
        comp.opt.cornerAngleDegrees = 35.0;
        comp.opt.compensateOpenPaths = true;
        comp.opt.compensateClosedPaths = true;
        comp.opt.insertCornerBridge = true;
        comp.opt.preserveClose = true;

        PathFlattener<DragKnifeCompensator<PathProgramBuilder>> flat(comp);
        flat.opt.flatness = 0.5;
        flat.opt.maxDepth = 16;

        if (!pathprogram_dispatch(src, flat)) {
            std::cout << "Failed in flatten + compensate pipeline\n";
            return;
        }

        compensated = std::move(compensatedBuilder.prog);

        // ------------------------------------------------------------
        // Stage 2:
        // compensated PathProgram -> dragknife tool policy -> GCodeProgram
        // ------------------------------------------------------------
        GCodeProgram gprog;
        GCodeProgramBuilder gbuild(gprog);

        DragKnifeToolPathOptions toolOpt;
        toolOpt.metric = true;
        toolOpt.knifeUpZ = 5.0;
        toolOpt.knifeDownZ = 0.0;
        toolOpt.travelFeed = 3000.0;
        toolOpt.cutFeed = 900.0;
        toolOpt.closePathWithLine = true;

        DragKnifeToolPathPolicy toolPolicy(gbuild, toolOpt);

        if (!pathprogram_to_gcodeprogram_flattened(compensated, toolPolicy)) {
            std::cout << "Failed to lower compensated path to GCodeProgram\n";
            return;
        }

        // ------------------------------------------------------------
        // Stage 3:
        // GCodeProgram -> text
        // ------------------------------------------------------------
        GCodeDebugTextSink sink(3);

        if (!sink.writeProgram(gprog)) {
            std::cout << "Failed to serialize GCodeProgram\n";
            return;
        }

        std::cout << "===== SOURCE PATH =====\n";
        std::cout << pathText << "\n\n";

        std::cout << "===== GCODE OUTPUT =====\n";
        std::cout << sink.str() << "\n";
    }

    static INLINE void test_path_to_gcode() noexcept
    {
        // Example SVG path string
        const char* pathStr =
            "M 0 0 "
            "L 10 0 "
            "L 10 10 "
            "L 0 10 "
            "Z";

        // Step 1: parse SVG path -> PathProgram
        PathProgram pathProg;
        if (!parsePathProgram(pathStr, pathProg)) {
            std::cout << "Failed to parse path\n";
            return;
        }

        PathProgram flattenedProg;
        double flatness = 0.25;
        int maxDepth = 16;
        if (!flattenPathProgram(pathProg, flattenedProg, flatness, maxDepth))
            return;


        // Step 2: convert PathProgram -> GCodeProgram
        GCodeProgram gprog;

        RouterToolPathOptions opt;
        opt.metric = true;
        opt.safeZ = 5.0;
        opt.cutZ = 0.0;
        opt.plungeFeed = 200.0;
        opt.cutFeed = 800.0;
        opt.spindleRPM = 10000.0;
        opt.spindleDwellSeconds = 0.5;

        if (!pathprogram_to_gcodeprogram_router(flattenedProg, gprog, opt)) {
            std::cout << "Failed to convert to GCodeProgram\n";
            return;
        }

        // Step 3: serialize GCodeProgram -> text
        GCodeDebugTextSink sink(3);

        if (!sink.writeProgram(gprog)) {
            std::cout << "Failed to serialize GCodeProgram\n";
            return;
        }

        std::string gcode = sink.str();

        // Output result
        std::cout << "==== GCODE OUTPUT ====\n";
        std::cout << gcode << "\n";
    }
}


static void test_gcode_builder()
{
	GCodeProgram prog;
	GCodeProgramBuilder b(prog);

	b.unitsMM();
	b.absolute();
	b.planeXY();
	b.rapidTo(0.0, 0.0, 5.0);
	b.setFeed(300.0);
	b.lineTo(10.0, 0.0, 0.0);
	b.lineTo(10.0, 10.0, 0.0);
	b.endProgram();
	b.end();

	GCodeDebugTextSink sink;
	if (sink.writeProgram(prog)) {
		std::string s = sink.str();

		printf("%s\n", s.c_str());
	}
}

int main() 
{
	//test_gcode_builder();
    //test_path_to_gcode();
    test_dragknife_sink_pipeline();


	return 0;
}
