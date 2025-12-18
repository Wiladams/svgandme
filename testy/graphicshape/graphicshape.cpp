#pragma comment(lib, "blend2d.lib")

#include <filesystem>
#include <ranges>
#include <algorithm>

#include "mappedfile.h"
#include "svguiapp.h"
#include "agraphic.h"

#include "viewnavigator.h"
#include "b2dpath.h"
#include "ocspan.h"
#include "wsenum.h"

using namespace waavs;

// Drawing context used for drawing document
static IRenderSVG gDrawingContext{};


// Reference to currently active document
SVGDocumentHandle gDoc{ nullptr };

static const char* goSquare_s = "M 1,1  h 10 v 10 h -10 z";
static const char* goDown_s = "M14,11V5c0-1.105-0.895-2-2-2h-2C8.895,3,8,3.895,8,5v6H3l8,8l8-8H14z";
static const char* goUp_s = "M11,3l-8,8h5v6c0,1.105,0.895,2,2,2h2c1.105,0,2-0.895,2-2v-6h5L11,3z";
static const char* goLeft_s = "M17,8h-6V3l-8,8l8,8v-5h6c1.105,0,2-0.895,2-2v-2C19,8.895,18.105,8,17,8z";
static const char* goRight_s = "M5,8h6v5l8-8l-8-8v5H5c-1.105,0-2,0.895-2,2v2C3,7.105,3.895,8,5,8z";

static const char* figures_s = R"||(
M22-49c4.9-2.7-27.1-0.8-58.8,14.1c-13.7,6.4-52.7,24.7-52.7,24.7l7.3,45.3c-2.3-1.6,45.9,0.7,70.7,6
			c2.8,0.6,24-1.3,7-9.2c-9.3-4.3-31-7.8-31-7.8s8.8,0.2,14-0.3c19.3-1.8,25-7.5,25-7.5s-4-9.7-15-14c-4.2-1.7-15-4.5-15-4.5
			s17.3-1.8,23.3-3C8.6-7.6,18.6-16.5,18-19.1c0,0-5.6-6.3-17.9-7.7C-4.7-27.3-25-24.7-25-24.7s22.3-7,27.8-9.8
			C14.1-40.1,22-49,22-49z
)||";



static WSEnum WSPathCmdEnum = {
	{"MOVE", BL_PATH_CMD_MOVE},
	{"ON", BL_PATH_CMD_ON},
	{"QUAD", BL_PATH_CMD_QUAD},
	{"CONIC", BL_PATH_CMD_CONIC},
	{"CUBIC", BL_PATH_CMD_CUBIC},
	{"CLOSE", BL_PATH_CMD_CLOSE},
	{"WEIGHT", BL_PATH_CMD_WEIGHT}
};



static void quickDraw(IRenderSVG* ctx)
{
	ctx->renew();
	ctx->push();

	// Construct a graphic
	AGraphicShape SDown11(goDown_s);
	AGraphicShape SDown12(goDown_s);
	AGraphicShape SDown13(goDown_s);


	SDown11.setFillStyle(BLVar(BLRgba32(0xffff0000)));
	SDown11.setPreserveAspectRatio("xMinYMin meet");
	SDown11.setFrame({ 0,0,32,50 });

	SDown12.setFillStyle(BLVar(BLRgba32(0xffff0000)));
	SDown12.setPreserveAspectRatio("xMidYMin meet");
	SDown12.setFrame({ 50,0,32,50 });

	SDown13.setFillStyle(BLVar(BLRgba32(0xffff0000)));
	SDown13.setPreserveAspectRatio("xMaxYMin meet");
	SDown13.setFrame({ 100,0,32,50 });

	SDown11.draw(ctx);
	SDown12.draw(ctx);
	SDown13.draw(ctx);


	//
	// Row 2
	AGraphicShape SDown21(goDown_s);
	AGraphicShape SDown22(goDown_s);
	AGraphicShape SDown23(goDown_s);

	SDown21.setStyle("viewBox:0 0 22 22; portal:0 50 44 44; fill:green; preserveAspectRatio:xMinYMid meet; stroke:black");
	SDown22.setStyle("viewBox:0 0 22 22; portal:50 50 44 44; fill:green; preserveAspectRatio:xMidYMid meet; stroke:black");
	SDown23.setStyle("viewBox:0 0 22 22; portal:100 50 44 44; fill:green; preserveAspectRatio:xMidYMid meet; stroke:black");

	SDown21.draw(ctx);
	SDown22.draw(ctx);
	SDown23.draw(ctx);


	//
	// Row 3
	AGraphicShape SDown31(goDown_s);
	AGraphicShape SDown32(goDown_s);
	AGraphicShape SDown33(goDown_s);

	SDown31.setAttribute("fill", "blue");
	SDown31.setAttribute("preserveAspectRatio", "xMinYMax meet");
	SDown31.setAttribute("portal", "0, 100, 22, 44");
	SDown31.setAttribute("viewBox", "0 0 22 22");

	SDown32.setAttribute("fill", "blue");
	SDown32.setAttribute("preserveAspectRatio", "xMidYMax meet");
	SDown32.setAttribute("portal", "50, 100, 22, 44");
	SDown32.setAttribute("viewBox", "0 0 22 22");

	SDown33.setAttribute("fill", "blue");
	SDown33.setAttribute("preserveAspectRatio", "xMaxYMax meet");
	SDown33.setAttribute("portal", "100, 100, 22, 44");
	SDown33.setAttribute("viewBox", "0 0 22 22");

	SDown31.draw(ctx);
	SDown32.draw(ctx);
	SDown33.draw(ctx);


	ctx->flush();
	ctx->pop();
}

static void quickDraw2(IRenderSVG* ctx)
{
	ctx->renew();
	ctx->push();

	//ByteSpan pathSpan("M 10,10  h 100 0 V 100 H 10 Z");
	ByteSpan pathSpan("M 10,10  h 90 V 100 Z");
	BLPath apath{};

	if (parsePath(pathSpan, apath)) {
		ctx->strokeWidth(4);
		ctx->stroke(BLRgba32(0xff0000ff));
		ctx->fill(BLRgba32(0xffff0000));
		ctx->drawShape(apath);
	}


	ctx->flush();
	ctx->pop();
}



static void drawCommand(uint8_t cmd, const BLPoint& currentPt, IRenderSVG* ctx)
{
	static constexpr double ptRadius = 0.5;

	// Some colors to play with
	static BLRgba32 beginC{ 0xff00ff00 };		// color of beginning
	static BLRgba32 onC{ 0xff0000ff };			// color of mid-points
	static BLRgba32 controlC{ 0xffff0000 };		// color of control points
	static BLRgba32 lineC{ 0xff000000 };		// color of lines
	static BLRgba32 closeC{ 0xffffff00 };		// color of close points

	//if (!cmdState.isValid())
	//	return;

	static BLPoint lastMv{};

	switch (cmd)
	{
	case BL_PATH_CMD_MOVE:
		lastMv = currentPt;
		ctx->fillCircle(currentPt.x, currentPt.y, 2.0 * ptRadius, beginC);
		break;

	case BL_PATH_CMD_ON:
		// Points that are definitely ON the line
		// Typically moveto, lineto, curve(s)to, poly, etc
		ctx->fillCircle(currentPt.x, currentPt.y, ptRadius, onC);
		break;

	case BL_PATH_CMD_CLOSE:
		// On a close, draw a black vertex at the starting point
		// to confirm which point close will be using to close the figure
		ctx->fillCircle(lastMv.x, lastMv.y, 0.75 * ptRadius, closeC);
		break;

	case BL_PATH_CMD_QUAD:
	case BL_PATH_CMD_CONIC:
	case BL_PATH_CMD_CUBIC:
		// These are control points for various curve types
		// They represent what the original curve was transformed
		// into, rather than the original control points
		ctx->fillCircle(currentPt.x, currentPt.y, ptRadius, controlC);
		break;

	case BL_PATH_CMD_WEIGHT:
		// do nothing with this one
		// it is for internal use
		//printf("BL_PATH_CMD_WEIGHT\n");
		break;

	default:
		return;
		break;
	}
}

// Accepts a range and a context to draw into
// Ensure the type is a PathCommandState
template <std::ranges::range Range>
	requires std::same_as<std::ranges::range_value_t<Range>, PathCommandState>
static void drawCommands(const Range& commands, IRenderSVG* ctx)
{
	for (const auto& cmd : commands) {
		drawCommand(cmd.command(), cmd.point(), ctx);
	}
}



static void drawShadowPath(BLPath& apath, IRenderSVG* ctx)
{
	// put a simple command iterator on the path
	//PathIterState iterState(apath);

	// Create the shadow path with as much capacity as the
	// current path has.
	// You can NOT just 'reserve()' and copy all the values,
	// as the source path probably has some other properties
	// related to how the path is to be rendered.  So, better
	// to just make a copy of the entire object, then go alter
	// the coordinates after that.
	// bpath.reserve(apath.size());

	BLPath bpath{};
	bpath = apath;


	// Get the bounds of the path so we can do some
	// fun vertex manipulations
	BLRect pBounds = pathBounds(apath);
	BLPoint pCenter = BLPoint(pBounds.x + pBounds.w / 2.0, pBounds.y + pBounds.h / 2.0);


	// iterate the commands/points of the source, doing
	// some alteration along the way.
	//int offset = 0;
	PathCommandContainer pcc(apath);

	for (const auto& iterState : pcc)
	{
		BLPoint pt = iterState.point();
		uint8_t cmd = iterState.command();
		BLPoint pv(pt.x - pCenter.x, pt.y - pCenter.y);
		double scaleFactor = 1.0;

		// Won't work if coordinates are NAN
		// Calculate the vector from the center to the point
		vec2f v = { static_cast<float>(pv.x), static_cast<float>(pv.y) };

		// Normalize the vector
		v = normalize(v);


		switch (cmd)
		{
			// For any of the curve commands
			// move the point outward from the center
			// by a factor of 2.0
		case BL_PATH_CMD_QUAD:
		case BL_PATH_CMD_CUBIC:
			scaleFactor = 2.0;
			break;

		case BL_PATH_CMD_MOVE:
		case BL_PATH_CMD_ON:
			scaleFactor = -1.25;
			break;
		}

		if (scaleFactor != 1.0)
			pt = { pt.x + v.x * scaleFactor, pt.y + v.y * scaleFactor };


		bpath.setVertexAt(iterState.offset(), cmd, pt);
		drawCommand(cmd, pt, ctx);

		//++iterState;
		//++offset;
	}

	// Draw the shadow path in red
	ctx->stroke(BLRgba32(0xffff0000));
	ctx->strokeShape(bpath);

	// Draw the original path in black
	ctx->stroke(BLRgba32(0xff000000));
	ctx->strokeShape(apath);
}

static void printPathCommands(PathCommandContainer& pcc)
{
	// for each command in the path, print the command
	// as well as the associated point
	for (const auto& cmd : pcc)
	{
		ByteSpan key{};
		getEnumKey(WSPathCmdEnum, cmd.command(), key);
		printf("Command: ");
		writeChunk(key);
		printf("  Point : % f, % f\n", cmd.point().x, cmd.point().y);
	}
}

// A test case to iterate over the commands in a path
// applying filters to select specific kinds
static void quickDraw3(IRenderSVG* ctx)
{
	static constexpr double figScale = 40.0;

	// Some colors to play with
	static BLRgba32 lineC{ 0xff000000 };		// color of lines

	// Create some sort of path
	//ByteSpan pathSpan(goSquare_s);
	ByteSpan pathSpan(goDown_s);
	//ByteSpan pathSpan(goRight_s);
	//ByteSpan pathSpan(figures_s);

	BLPath apath{};

	if (!parsePath(pathSpan, apath))
		return;


	// Setup context for some drawing
	ctx->renew();
	ctx->push();
	ctx->background(BLRgba32(0xffc0c0c0));

	ctx->scale(figScale, figScale);		// make things more viewable
	ctx->strokeBeforeTransform(true);	// for skinny lines

	// enumerate points on the path
	// draw 'on' and 'control' points in different colors
	// So we can see the structure

	// Setup the command container so we
	// can get iterators
	PathCommandContainer pcc(apath);

	//printPathCommands(pcc);

	// Predicates to be used with a general filter
	auto onFilter = [](const PathCommandState& state) { return state.command() == BL_PATH_CMD_ON; };

	auto moveFilter = [](const PathCommandState& state) { return state.command() == BL_PATH_CMD_MOVE; };

	auto curveFilter = [](const PathCommandState& state) {return state.command() == BL_PATH_CMD_CUBIC || state.command() == BL_PATH_CMD_QUAD; };


	// Draw everything using container
	//drawCommands(PathCommandContainer(apath), ctx);

	// Draw only "ON" points using predicate
	//drawCommands(FilteredPathCommands(PathCommandContainer(apath), onFilter), ctx);
	// Draw control points
	//drawCommands(FilteredPathCommands(PathCommandContainer(apath), curveFilter), ctx);

	//drawCommands(FilteredPathCommands(moveFilter, PathCommandContainer(apath)), ctx);


	// draw altered points
	drawShadowPath(apath, ctx);



	// draw the path on top so we can see clearly how
	// to control points affect the path
	ctx->strokePath(apath, lineC);

	// Do some cleanup
	ctx->flush();
	ctx->pop();
}



// Test OcSpan
static void quickDraw5(IRenderSVG* ctx)
{
	OcSpan span1("Hello, World!");
	for (auto c : span1)
	{
		printf("%c", c);
	}
	printf("\n");

}

static void quickDraw6(IRenderSVG* ctx)
{
	//ByteSpan pathSpan(goSquare_s);
	//ByteSpan pathSpan(goDown_s);
	//ByteSpan pathSpan(goRight_s);
	ByteSpan pathSpan(figures_s);

	BLPath apath{};

	if (!parsePath(pathSpan, apath))
		return;


	PathCommandContainer pcc(apath);

	printPathCommands(pcc);
}

// Check to see what lastVertext reports when you close
// a path
static void quickDraw7(IRenderSVG* ctx)
{
	BLPath apath{};

	apath.moveTo(10, 10);
	apath.lineTo(20, 10);
	apath.lineTo(20, 20);
	apath.close();

	BLPoint vtxOut{};
	apath.getLastVertex(&vtxOut);

	printf("Last Vertex (10,10): %f, %f\n", vtxOut.x, vtxOut.y);
}

static void quickDraw8(IRenderSVG* ctx)
{
	BLPath apath{};
	B2DPathBuilder builder(apath);

	builder.addSegment('M', (const double[]) { 10.0, 50.0 });
	builder.addSegment('Q', (const double[]) { 25.00, 25.00, 40.00, 50.00 });
	builder.addSegment('t', (const double[]) { 30.00, 0.00 });
	builder.addSegment('t', (const double[]) { 30.00, 0.00 });
	builder.addSegment('t', (const double[]) { 30.00, 0.00 });
	builder.addSegment('t', (const double[]) { 30.00, 0.00 });
	builder.addSegment('t', (const double[]) { 30.00, 0.00 });


	ctx->renew();
	ctx->push();

	ctx->strokePath(apath, BLRgba32(0xffff0000));

	ctx->pop();
	ctx->flush();
}

// docFromFilename
//
// Given a filename, parse the svg in the file, and return 
// a shared pointer to it.
//
static SVGDocumentHandle DOMFromFilename(const char* filename)
{
	auto mapped = waavs::MappedFile::create_shared(filename);

	// if the mapped file does not exist, return
	if (mapped == nullptr)
	{
		printf("File not found: %s\n", filename);
		return nullptr;
	}

	ByteSpan aspan(mapped->data(), mapped->size());
	auto aDoc = SVGFactory::createDOM(aspan, FontHandler::getFontHandler());

	return aDoc;
}


// Event responders
static void onFileDrop(const FileDropEvent& fde)
{
	// assuming there's at least one file that 
	// has been dropped.
	for (int i = 0; i < fde.filenames.size(); i++)
	{
		// Create a new SVGDocument for each file
		// And create a window to display each document
		double startTime = seconds();
		gDoc = DOMFromFilename(fde.filenames[i].c_str());

		double endTime = seconds();
		printf("== fileDrop: SVGFactory::createDOM() took %f seconds\n", endTime - startTime);

		if (gDoc != nullptr)
		{
			// We have loaded the un-processed document
			// Draw into an empty context at least once to resolve references
			// and fix sizes.
			//IRenderSVG ctx(FontHandler::getFontHandler());
			//ctx.setContainerFrame(BLRect(0, 0, canvasWidth, canvasHeight));
			//gDoc->draw(&ctx, gDoc.get());

			//BLRect objFr = gDoc->getBBox();
			//auto objFr = gDoc->frame();

			//resetView();



			// Set the initial viewport
			//gNavigator.setFrame({ 0, 0, (double)canvasWidth, (double)canvasHeight });
			//gNavigator.setBounds(objFr);

			//handleChange(true);

			break;
		}

	}
}

// Create a routine to respond to frameevents
static void onFrameEvent(const FrameCountEvent& fe)
{
	//printf("frameEvent: %d\n", (int)fe.frameCount);
	//printf("Actual Frame Rate: %d\n", (int)(fe.frameCount / seconds()));
	//quickDraw(&gDrawingContext);
	//quickDraw2(&gDrawingContext);
	//quickDraw3(&gDrawingContext);
	//quickDraw4(&gDrawingContext);

	refreshScreenNow();

	//gRecorder.saveFrame();
}


// called once before main loop is running
void setup()
{
	//printf("setup()\n");

	//setupFonts();

	setFrameRate(15);

	dropFiles();

	//layered();

	// set app window size and title
	createAppWindow(1024, 768, "DOM Walker");

	getRecorder()->reset(&getAppFrameBuffer()->getBlend2dImage(), "frame", 15, 0);

	// register to receive various events
	subscribe(onFileDrop);
	subscribe(onFrameEvent);
	//subscribe(onMouseEvent);
	//subscribe(onResizeEvent);
	//subscribe(onKeyboardEvent);

	// clear the buffer to white to start
	//appFrameBuffer()->setAllPixels(vec4b{ 0xFF,0xff,0xff,0x00 });
	BLContextCreateInfo ctxInfo{};
	ctxInfo.threadCount = 4;
	//ctxInfo.threadCount = 0;
	gDrawingContext.begin(appFrameBuffer()->getBlend2dImage(), &ctxInfo);
	gDrawingContext.background(BLRgba32(0xff00ffff));

	// Set the initial viewport
	//gViewPort.surfaceFrame({0, 0, (double)canvasWidth, (double)canvasHeight});
	//gNavigator.setFrame({ 0, 0, (double)canvasWidth, (double)canvasHeight });
	//gNavigator.subscribe(handleChange);

	// Load extension elements
	//DisplayCaptureElement::registerFactory();
	//SVGScriptElement::registerFactory();

	//onFrameEvent(FrameCountEvent{ 0 });

	printf("IS DIGIT (COMMA): %d\n", is_digit(','));

	//quickDraw3(&gDrawingContext);
	//quickDraw5(&gDrawingContext);
	//quickDraw6(&gDrawingContext);
	//quickDraw7(&gDrawingContext);
	quickDraw8(&gDrawingContext);

}
