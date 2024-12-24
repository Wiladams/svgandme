#pragma comment(lib, "blend2d.lib")

#include "app/apphost.h"

#include <filesystem>


#include "mappedfile.h"
#include "svguiapp.h"
#include "svgwaavs.h"
#include "svgicons.h"
#include "viewnavigator.h"

using namespace waavs;

// Drawing context used for drawing document
IRenderSVG gDrawingContext(nullptr);

// Reference to currently active document
std::shared_ptr<SVGDocument> gDoc{ nullptr };
ViewNavigator gNavigator{};


// For mouse management
static bool gIsDragging{ false };
static vec2f gDragPos{ 0,0 };
static double gZoomFactor = 0.1;


// Animation management
bool gAnimate{ false };
bool gPerformTransform{ true };


// Create one of these first, so factory constructor will run
static SVGFactory gSVG;



// docFromFilename
//
// Given a filename, parse the svg in the file, and return 
// a shared pointer to it.
//
static std::shared_ptr<SVGDocument> docFromFilename(const char* filename)
{
	auto mapped = waavs::MappedFile::create_shared(filename);
	
	// if the mapped file does not exist, return
	if (mapped == nullptr)
	{
		printf("File not found: %s\n", filename);
		return nullptr;
	}

	
	ByteSpan aspan(mapped->data(), mapped->size());
	std::shared_ptr<SVGDocument> aDoc = SVGDocument::createFromChunk(aspan, &getFontHandler(), canvasWidth, canvasHeight, physicalDpi);
	
	return aDoc;
}



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
		gDoc->draw(&gDrawingContext, gDoc.get());

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

static void resetView()
{
	gNavigator.resetNavigator();
	gNavigator.setFrame(BLRect(0, 0, canvasWidth, canvasHeight));
	gNavigator.setBounds(BLRect(0, 0, canvasWidth, canvasHeight));

}

static void handleChange(const bool&)
{
	//printf("svgviewer::handleChange\n");
	draw();
	
	screenRefresh();

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
		gDoc = docFromFilename(fde.filenames[i].c_str());


		//double endTime = seconds();
		//printf("== fileDrop: SVGDocument::createFromFilename took %f seconds\n", endTime - startTime);

		if (gDoc != nullptr)
		{
			// We have loaded the un-processed document
			// Draw into an empty context at least once to resolve references
			// and fix sizes.
			//IRenderSVG ctx(&getFontHandler());
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

// Create a routine to respond to frameevents
static void onFrameEvent(const FrameCountEvent& fe)
{
	//printf("frameEvent: %d\n", (int)fe.frameCount);
	//printf("Actual Frame Rate: %d\n", (int)(fe.frameCount / seconds()));
	
	//appFrameBuffer().setAllPixels(vec4b{ 0x00,0xff,0x00,0xff });
	// update current document
	if (gDoc != nullptr)
	{
		if (gAnimate)
		{
			gDoc->update(gDoc.get());
			handleChange(true);
		}
	}
	

	screenRefresh();


	gRecorder.saveFrame();
}



static void onResizeEvent(const ResizeEvent& re)
{
	//printf("onResizeEvent: %d x %d\n", re.width, re.height);
	gDrawingContext.begin(appFrameBuffer()->image());
	handleChange(true);
}



static void onMouseEvent(const MouseEvent& e)
{
	gNavigator.onMouseEvent(e);

}




static void onKeyboardEvent(const KeyboardEvent& ke)
{
	//printf("SVGViewer::onKeyboardEvent: %d\n", ke.key);
	if (ke.activity == KEYRELEASED)
	{
		switch (ke.keyCode)
		{
			case VK_PLAY:
			case VK_PAUSE:
			case 'R':
				gRecorder.toggleRecording();
			break;			

			case 'A':
				gAnimate = !gAnimate;
			break;

			case 'T':
				gPerformTransform = !gPerformTransform;
				handleChange(true);

			break;
		}
	}
}

static void setupFonts()
{
	//loadDefaultFonts();
	loadFontDirectory("c:\\windows\\fonts");
	loadFontDirectory("d:\\commonfonts");
	//loadFontDirectory("..\\resources");

	
	gDrawingContext.fontHandler(&getFontHandler());
}


// called once before main loop is running
void setup()
{
    //printf("setup()\n");
	
	setupFonts();
	
	frameRate(30);
	
    dropFiles();


	//layered();
	
	// set app window size and title
	createAppWindow(1920, 1080, "SVGViewer");
	
	gRecorder.reset(&appFrameBuffer()->image(), "frame", 15, 0);
	
	// register to receive various events
	subscribe(onFileDrop);
	subscribe(onFrameEvent);
	subscribe(onMouseEvent);
	subscribe(onResizeEvent);
	subscribe(onKeyboardEvent);
	
	// clear the buffer to white to start
	appFrameBuffer()->setAllPixels(vec4b{ 0xFF,0xff,0xff,0x00 });
	BLContextCreateInfo ctxInfo{};
	ctxInfo.threadCount = 4;
	//ctxInfo.threadCount = 0;
	gDrawingContext.begin(appFrameBuffer()->image(), &ctxInfo);
		
	// Set the initial viewport
	//gViewPort.surfaceFrame({0, 0, (double)canvasWidth, (double)canvasHeight});
	gNavigator.setFrame({ 0, 0, (double)canvasWidth, (double)canvasHeight });
	gNavigator.subscribe(handleChange);
	
	// Load extension elements
	DisplayCaptureElement::registerFactory();
	//SVGScriptElement::registerFactory();
}
