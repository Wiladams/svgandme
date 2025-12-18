#pragma comment(lib, "blend2d.lib")

#include <filesystem>
#include <ranges>
#include <algorithm>

#include "mappedfile.h"
#include "wsenum.h"


#include "svguiapp.h"
#include "svgdom.h"

#include "viewnavigator.h"
#include "b2dpath.h"



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
	refreshScreenNow();
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
	gDrawingContext.attach(getAppFrameBuffer()->getBlend2dImage(), &ctxInfo);
	gDrawingContext.background(BLRgba32(0xff00ffff));
	
	// Set the initial viewport
	//gViewPort.surfaceFrame({0, 0, (double)canvasWidth, (double)canvasHeight});
	//gNavigator.setFrame({ 0, 0, (double)canvasWidth, (double)canvasHeight });
	//gNavigator.subscribe(handleChange);

	// Load extension elements
	//DisplayCaptureElement::registerFactory();
	//SVGScriptElement::registerFactory();

	//onFrameEvent(FrameCountEvent{ 0 });
}
