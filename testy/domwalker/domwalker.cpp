#pragma comment(lib, "blend2d.lib")

#include <filesystem>

#include "apphost.h"
#include "mappedfile.h"
#include "svguiapp.h"
#include "svggraphic.h"

#include "viewnavigator.h"

using namespace waavs;

// Drawing context used for drawing document
IRenderSVG gDrawingContext(nullptr);


// Reference to currently active document
SVGDocumentHandle gDoc{ nullptr };


static void quickDraw(BLContext *ctx)
{
	// Construct a graphic
	AGraphicShape aShape;
	
	aShape.path().addBox(10, 10, 200, 200);
	aShape.setFillStyle(BLVar(BLRgba32(0xffff0000)));

	aShape.draw(ctx);
	
	ctx->flush(BLContextFlushFlags::BL_CONTEXT_FLUSH_SYNC);
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
	auto aDoc = SVGFactory::createDOM(aspan, &getFontHandler());

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
			//IRenderSVG ctx(&getFontHandler());
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
	quickDraw(&gDrawingContext);
	
	//appFrameBuffer().setAllPixels(vec4b{ 0x00,0xff,0x00,0xff });
	// update current document
	//if (gDoc != nullptr)
	//{
		//if (gAnimate)
		//{
	//		gDoc->update(gDoc.get());
		//	handleChange(true);
		//}
	//}


	screenRefresh();


	//gRecorder.saveFrame();
}


// called once before main loop is running
void setup()
{

	//printf("setup()\n");

	//setupFonts();

	frameRate(30);

	dropFiles();


	//layered();

	// set app window size and title
	createAppWindow(1920, 1080, "DOM Walker");

	gRecorder.reset(&appFrameBuffer()->getBlend2dImage(), "frame", 15, 0);

	// register to receive various events
	subscribe(onFileDrop);
	//subscribe(onFrameEvent);
	//subscribe(onMouseEvent);
	//subscribe(onResizeEvent);
	//subscribe(onKeyboardEvent);

	// clear the buffer to white to start
	//appFrameBuffer()->setAllPixels(vec4b{ 0xFF,0xff,0xff,0x00 });
	BLContextCreateInfo ctxInfo{};
	ctxInfo.threadCount = 4;
	//ctxInfo.threadCount = 0;
	gDrawingContext.begin(appFrameBuffer()->getBlend2dImage(), &ctxInfo);
	gDrawingContext.fillAll(BLRgba32(0xff00ffff));
	
	// Set the initial viewport
	//gViewPort.surfaceFrame({0, 0, (double)canvasWidth, (double)canvasHeight});
	//gNavigator.setFrame({ 0, 0, (double)canvasWidth, (double)canvasHeight });
	//gNavigator.subscribe(handleChange);

	// Load extension elements
	//DisplayCaptureElement::registerFactory();
	//SVGScriptElement::registerFactory();

	onFrameEvent(FrameCountEvent{ 0 });


}
