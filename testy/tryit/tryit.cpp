#include "svg.h"
#include "viewport.h"

#include "svgb2ddriver.h"
#include "curves.h"
#include "curvefx.h"
#include "SVGAPIPrinter.h"
#include "morse.h"

using namespace waavs;

#define CAN_WIDTH 1920
#define CAN_HEIGHT 1280


SVGB2DDriver ctx;


static bool builder(BLPath& path, IProduce<CurveSegment>& input, size_t segCount = 20) 
{
	CurveSegment seg;
	bool started = false;

	while (input.next(seg)) {
		CurveParametricSegmentGenerator gen(*seg.curve, segCount, seg.t0, seg.t1);
		Point2d pt;
		double t;

		bool segStarted = false;
		while (gen.next(pt, t)) {
			if (!started) {
				path.moveTo(pt.x, pt.y);
				started = true;
			}
			else if (!segStarted) {
				path.lineTo(pt.x, pt.y);  // Move to start of this segment
				segStarted = true;
			}
			else {
				path.lineTo(pt.x, pt.y);
			}
		}
	}

	return started;
}

static void testCurveSource()
{
	auto curve = std::make_shared<CubicCurve>(Point2d{ 50, 250 }, Point2d{ 150, 50 }, Point2d{ 250, 450 }, Point2d{ 350, 250 });
	PVXCurveSource source(curve);

	CurveSegment seg;
	while (source.next(seg)) {
		printf("Segment t0=%.2f, t1=%.2f\n", seg.t0, seg.t1);
	}

}

static void testFxChain()
{
	BLPath path{};
	auto curve = std::make_shared<CubicCurve>(Point2d{ 50, 250 }, Point2d{ 150, 50 }, Point2d{ 250, 450 }, Point2d{ 350, 250 });
	auto source = std::make_shared<PVXCurveSource>(curve);
	auto dasher = std::make_shared<PVXDashFilter>(source, std::vector<double>{ 5, 4, 5, 4, 5, 4, 10, 4, 10, 4, 10, 4, 5, 4, 5, 4, 5, 4 });

	auto widthWarp = std::make_shared<ParametricStopMap<double>>();
	widthWarp->addStop(0.0, 2.0);
	widthWarp->addStop(0.5, 10.0);
	widthWarp->addStop(1.0, 2.0);

	// generate the curve into a BLPath
	//if (builder(path, source(curve)))

	// Split the curve up according to the dash pattern
	//if (builder(path,
	//	dasher({ 20, 10 },
	//		source(curve))))

	// Split according to pattern, and reshape each segment
	//	if (builder(path,
	//		    brushVariableWidth(widthWarp,
	//				dasher({ 20, 10 },
	//					source(curve)))))

	// Vary the width of the entire curve
	//if (builder(path,
	//	brushVariableWidth(widthWarp,
	//		source(curve))))

//	if (builder(path,
//		    brushVariableWidth(widthWarp,
//				dasher({ 20, 10 },
//					source(curve)))))


	if (builder(path,*dasher))
	{
		SVGAPIPrinter printer{};

		printer.noFill();
		printer.stroke(BLRgba32(0xffffff00));
		printer.drawShape(path);
	}
	else
	{
		printf("No segments\n");
	};

}



static void testMorseCode()
{
	// Create a simple cubic bezier curve
	// Then use one of the parametric curve generators
	// to generate line segments for rendering

	std::vector<double> morseCode;
	createMorseCode("HELLO WORLD", morseCode, 2);

	auto curve = std::make_shared<CubicCurve>(Point2d{ 50, 250 }, Point2d{ 150, 50 }, Point2d{ 250, 450 }, Point2d{ 350, 250 });
	//auto curve = std::make_shared<LineCurve>(Point{ 10, 20 }, Point{ 1920, 20 });
	auto source = std::make_shared<PVXCurveSource>(curve);
	auto dasher = std::make_shared<PVXDashFilter>(source, morseCode);

	CurveSegment seg;
	while (dasher->next(seg)) {
		if (seg.visible) {
			printf("apath.moveTo(%.3f, %.3f);\n", seg.start.x, seg.start.y);
			printf("apath.lineTo(%.3f, %.3f);\n", seg.end.x, seg.end.y);
		}
	}

	/*
	BLPath path{}; // Create a path to hold the segments
	if (builder(path, dasher))
	{
		SVGAPIPrinter printer{};

		printer.noFill();
		printer.stroke(BLRgba32(0xffffff00));
		printer.drawShape(path);
	}
	else
	{
		printf("No segments\n");
	};
	*/
}

static void testDashedCurve()
{
	// Here we want to test out the use of the DashedCurveGenerator
	// in relative isoltion.  The segments that come off it have a 
	// 'visible' attribute, as well as the start and stop points.
	// We're simply going to use the start/stop points to draw
	// simple line segments.
	auto curve = std::make_shared<CubicCurve>({ 50, 250 }, { 150, 50 }, { 250, 450 }, { 350, 250 });
	auto source = std::make_shared<PVXCurveSource>(curve);
	PVXDashFilter dashing(source, { 5,4,5,4,5,4,10,4,10,4,10,4,5,4,5,4,5,4 });

	CurveSegment seg;
	while (dashing.next(seg)) {
		if (seg.visible) {
			printf("apath.moveTo(%.3f, %.3f);\n", seg.start.x, seg.start.y);
			printf("apath.lineTo(%.3f, %.3f);\n", seg.end.x, seg.end.y);
		}
	}

}



static void testVariableWidth()
{
	// 1. Define a cubic curve
	auto curve = std::make_shared<CubicCurve>(
		Point2d{ 50.0, 250.0 },
		Point2d{ 150.0, 200.0 },
		Point2d{ 250.0, 200.0 },
		Point2d{ 350.0, 250.0 }
	);
	auto source = std::make_shared<PVXCurveSource>(curve);

	// 2. Create a stroke width profile
	ParametricStopMap<double> widthMap;
	widthMap.addStop(0.0, 2.0);		// start narrow
	widthMap.addStop(0.33, 10.0);	// first hump
	widthMap.addStop(0.5, 20.0);		// peak
	widthMap.addStop(0.66, 10.0);   // second hump
	widthMap.addStop(1.0, 2.0);		// end narrow

	// Optional: ease-in-out curve
	//widthMap.setEasingFunction([](double t, double) {
	//	return t * t * (3 - 2 * t);  // smoothstep easing
	//	});

	// 3. Create stroke outline generator
	auto widthMorpher = brushVariableWidth(widthMap, source, 20);

	// 4. Build the Blend2D path
	BLPath path{};
	if (builder(path, *widthMorpher))
	{
		// 5. Render the filled shape
		SVGAPIPrinter printer{};

		printer.stroke(BLRgba32(0xffffffff));
		printer.fill(BLRgba32(0xffffff00));
		printer.drawShape(path);
	}
	else
	{
		printf("No segments\n");
	};

}

static void testWidthMap()
{
	auto curve = std::make_shared<LineCurve>(Point{ 50, 250 }, Point{ 350, 250 });
	auto source = std::make_shared<PVXCurveSource>(curve);
	ParametricStopMap<double> widthMap;
	widthMap.addStop(0.0, 2.0);		// start narrow
	widthMap.addStop(0.33, 10.0);	// first hump
	widthMap.addStop(0.5, 2.0);		// second narrow
	widthMap.addStop(0.66, 10.0);   // second hump
	widthMap.addStop(1.0, 2.0);		// end narrow

	// iterate the map
	for (double t = 0.0; t <= 1.0; t += 0.1)
	{
		double width = widthMap.eval(t);
		printf("t: %.3f, width: %.3f\n", t, width);
	}
}

static void testSimplePath()
{
	// Create a simple cubic bezier curve
	// Then use one of the parametric curve generators
	// to generate line segments for rendering
	CubicCurve gen{ { 0, 0 }, { 100, 100 }, { 200, 100 }, { 300, 0 } };
	CurveArcLengthSegmentGenerator gen2{ gen, 100 };

	Point pt{};
	double t{};

	BLPath apath{};

	bool firstOne = true;
	while (gen2.next(pt, t))
	{
		if (firstOne)
		{
			firstOne = false;
			printf("apath.moveTo(%.3f, %.3f);\n", pt.x, pt.y);
		}
		else {
			printf("apath.lineTo(%.3f, %.3f);\n", pt.x, pt.y);
		}

	}
}

int main(int argc, char** argv)
{
	//printf("argc: %d\n", argc);

	// Create an image that we will draw into
	BLImage img(CAN_WIDTH, CAN_HEIGHT, BL_FORMAT_PRGB32);

	//if (argc < 2)
	//{
	//	printf("Usage: svgimage <xml file>  [output file]\n");
	//	return 1;
	//}

	//testSimplePath();
	
	testMorseCode();
	//testDashedCurve();
	
	//testWidthMap();
	//testVariableWidth();
	
	//testFxChain();

	/*
	setupFonts();

	// create an mmap for the specified file
	const char* filename = argv[1];

	auto mapped = MappedFile::create_shared(filename);

	// if the mapped file does not exist, return
	if (mapped == nullptr)
	{
		printf("File not found: %s\n", filename);
		return 1;
	}

	ByteSpan mappedSpan(mapped->data(), mapped->size());
	gDoc = SVGFactory::createFromChunk(mappedSpan, FontHandler::getFontHandler(), CAN_WIDTH, CAN_HEIGHT, 96.0);

	if (gDoc == nullptr)
		return 1;


	// Attach the drawing context to the image
	// We MUST do this before we perform any other
	// operations, including the transform
	BLContextCreateInfo createInfo{};
	createInfo.threadCount = 4;
	ctx.attach(img, &createInfo);

	// setup for drawing.  Not strictly necessary.
	// you can setup the context however you want before actually
	// drawing.
	ctx.background(BLRgba32(0xFFFFFFFF));	// white background
	ctx.renew();

	// We are doing 'fit to canvas' drawing, so we need
	// to calculate the scaling factors for the x and y 
	// axes.  The easiest way to do that is to create a viewport
	// Create a rectangle the size of the BLImage we want to render into
	BLRect surfaceFrame{ 0, 0, CAN_WIDTH, CAN_HEIGHT };

	// Now that we've processed the document, we have correct sizing
	// Get the frame size from the document
	// This is the extent of the document, in user units
	BLRect sceneFrame = gDoc->getBBox();
	printf("viewport: %3.0f %3.0f %3.0f %3.0f\n", sceneFrame.x, sceneFrame.y, sceneFrame.w, sceneFrame.h);


	// At this point, we could just do the render, but we go further
	// and create a viewport, which will handle the scaling and translation
	// This will essentially do a 'scale to fit'
	ViewportTransformer vp{};
	vp.viewBoxFrame(sceneFrame);		// The part of the scene we want to display
	vp.viewportFrame(surfaceFrame);		// The surface we want to fit to 

	// apply the viewport's sceneToSurface transform to the context
	BLMatrix2D tform = vp.viewBoxToViewportTransform();
	ctx.transform(tform);

	// Render the document into the context
	gDoc->draw(&ctx, gDoc.get());

	// detach automatically does a final flush, 
	// so we're ready to save the image after that
	ctx.detach();

	// Save the image from the drawing context out to a file
	// or do whatever you're going to do with it
	const char* outfilename = nullptr;


	if (argc >= 3)
		outfilename = argv[2];
	else
		outfilename = "output.png";

	img.writeToFile(outfilename);
	*/

	return 0;
}
