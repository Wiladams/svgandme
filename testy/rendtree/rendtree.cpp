#pragma comment(lib, "blend2d.lib")

#include <filesystem>
#include <ranges>
#include <algorithm>

#include "mappedfile.h"
#include "svguiapp.h"
#include "agraphic.h"

#include "viewnavigator.h"
#include "svganimation.h"

using namespace waavs;

// Drawing context used for drawing document
IRenderSVG gDrawingContext;
std::shared_ptr<SVGDocument> gDoc{ nullptr };

static void drawBackground()
{
	gDrawingContext.renew();
}

static void drawDocument()
{
	//gDrawingContext.background(BLRgba32(0xffA6A6A6));
	gDrawingContext.background(BLRgba32(0xffffffff));

	double startTime = seconds();


	// setup any transform
	if (gPerformTransform)
		gDrawingContext.setTransform(gNavigator.sceneToSurfaceTransform());


	// draw the document into the ctx
	if (gDoc != nullptr)
		gDoc->draw(&gDrawingContext, gDoc.get(), canvasWidth, canvasHeight);

	gDrawingContext.flush();

	double endTime = seconds();
	//printf("Drawing Duration: %f\n", endTime - startTime);
}

static void drawForeground()
{

}

static void draw()
{
	drawBackground();
	drawDocument();
	drawForeground();
}

static void handleChange(const bool&)
{
	//if (gDoc != nullptr && gAnimate)
	//{
	//	gDoc->update(gDoc.get());
	//}

	//printf("svgviewer::handleChange\n");
	draw();

	// we do this screenRefresh here because mouse dragging
	// runs the window in a modal way, starving us of regular
	// redraw messages, based on timing, so we force a redraw
	// message through the message queue
	refreshScreenNow();

}

// domFromFilename
//
// Given a filename, parse the svg in the file, and return 
// a shared pointer to it.
//
static std::shared_ptr<SVGDocument> domFromFilename(const char* filename)
{
	auto mapped = waavs::MappedFile::create_shared(filename);

	// if the mapped file does not exist, return
	if (mapped == nullptr)
	{
		printf("File not found: %s\n", filename);
		return nullptr;
	}


	ByteSpan aspan(mapped->data(), mapped->size());
	std::shared_ptr<SVGDocument> aDoc = SVGFactory::createDOM(aspan, FontHandler::getFontHandler());

	return aDoc;
}

static void onFileDrop(const FileDropEvent& fde)
{
	// assuming there's at least one file that 
	// has been dropped.
	for (int i = 0; i < fde.filenames.size(); i++)
	{
		// Create a new SVGDocument for each file
		// And create a window to display each document
		//double startTime = seconds();
		gDoc = domFromFilename(fde.filenames[i].c_str());


		//double endTime = seconds();
		//printf("== fileDrop: SVGDocument::createFromFilename took %f seconds\n", endTime - startTime);

		if (gDoc != nullptr)
		{
			// We have loaded the un-processed document
			// Draw into an empty context at least once to resolve references
			// and fix sizes.
			//IRenderSVG ctx(FontHandler::getFontHandler());
			//ctx.setContainerFrame(BLRect(0, 0, canvasWidth, canvasHeight));
			//gDoc->draw(&ctx, gDoc.get());

			BLRect objFr = gDoc->getBBox();
			//auto objFr = gDoc->frame();

			resetView();



			// Set the initial viewport
			gNavigator.setFrame({ 0, 0, (double)canvasWidth, (double)canvasHeight });
			gNavigator.setBounds(objFr);

			handleChange(true);

			break;
		}

	}
}

// called once before main loop is running
void setup()
{
	//printf("setup()\n");

	//setupFonts();

	frameRate(15);

	// Allow files to be dropped
	dropFiles();


	// set app window size and title
	createAppWindow(1024, 768, "Render Tree");

	gRecorder.reset(&appFrameBuffer()->getBlend2dImage(), "frame", 15, 0);

	// register to receive various events
	subscribe(onFileDrop);
	//subscribe(onFrameEvent);
	//subscribe(onMouseEvent);
	//subscribe(onResizeEvent);
	//subscribe(onKeyboardEvent);

	// clear the buffer to white to start
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


}

