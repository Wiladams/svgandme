#pragma comment(lib, "blend2d.lib")

#include "app/apphost.h"

#include <filesystem>


#include "mappedfile.h"
#include "svguiapp.h"
#include "svgwaavs.h"
#include "svgicons.h"
#include "viewnavigator.h"
#include "svgb2ddriver.h"

using namespace waavs;


// Reference to currently active document
std::shared_ptr<SVGDocument> gDoc{ nullptr };
ViewNavigator gNavigator{};


// Animation management
bool gAnimate{ false };
bool gPerformTransform{ true };
bool gAutoGrow{ false };

double gRecordingStart{ 0 };

// retrieve a pointer to a unique drawing context
static IRenderSVG *getDrawingContext()
{
	static std::unique_ptr<SVGB2DDriver> sDrawingContext = std::make_unique<SVGB2DDriver>();

	return sDrawingContext.get();
}

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

	
	ByteSpan aspan;
	aspan.resetFromSize(mapped->data(), mapped->size());
	std::shared_ptr<SVGDocument> aDoc = SVGFactory::createFromChunk(aspan, FontHandler::getFontHandler(), appFrameWidth, appFrameHeight, physicalDpi);
	
	return aDoc;
}



static void drawBackground()
{
	getDrawingContext()->renew();
}

static void drawForeground()
{
}

static void drawDocument()
{
	// setup any transform
	if (gPerformTransform)
		getDrawingContext()->transform(gNavigator.sceneToSurfaceTransform());


	// draw the document into the ctx
	if (gDoc != nullptr)
		gDoc->draw(getDrawingContext(), gDoc.get(), appFrameWidth, appFrameHeight);

	getDrawingContext()->flush();
}



static void draw()
{	
	double startTime = seconds();

	drawBackground();
	drawDocument();
	drawForeground();
}

static void resetView()
{
	gNavigator.resetNavigator();
	gNavigator.setFrame(BLRect(0, 0, appFrameWidth, appFrameHeight));
	gNavigator.setBounds(BLRect(0, 0, appFrameWidth, appFrameHeight));

}

static void handleChange(const bool& changeOccured)
{
	if (gDoc == nullptr)
        return;

	if (gAnimate)
	{
		gDoc->update(gDoc.get());
	}
	
	//printf("svgviewer::handleChange\n");
	draw();
	
	// we do this screenRefresh here because mouse dragging
	// runs the window in a modal way, starving us of regular
	// redraw messages, based on timing, so we force a redraw
	// message through the message queue
	refreshScreenNow();

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
			//IRenderSVG ctx(FontHandler::getFontHandler());
			//ctx.setContainerFrame(BLRect(0, 0, canvasWidth, canvasHeight));
			//gDoc->draw(&ctx, gDoc.get());
			
			BLRect objFr = gDoc->getBBox();
			//auto objFr = gDoc->frame();
			
			resetView();


			
			// Set the initial viewport
			gNavigator.setFrame({ 0, 0, (double)appFrameWidth, (double)appFrameHeight });
			gNavigator.setBounds(objFr);
			
			handleChange(true);

			break;
		}

	}
}

// Create a routine to respond to frameevents
// which are sent at the frame rate specified
static void onFrameEvent(const FrameCountEvent& fe)
{
	//printf("frameEvent: %d\n", (int)fe.frameCount);
	//printf("Actual Frame Rate: %d\n", (int)(fe.frameCount / seconds()));
	

	handleChange(true);

	refreshScreenNow();


	getRecorder()->saveFrame();
}



static void onResizeEvent(const ResizeEvent& re)
{
	gNavigator.setFrame(BLRect(0, 0, appFrameWidth, appFrameHeight));
	
	//printf("onResizeEvent: %d x %d\n", re.width, re.height);
	BLContextCreateInfo ctxInfo{};
	ctxInfo.threadCount = 4;
	//ctxInfo.threadCount = 0;

	getDrawingContext()->attach(getAppFrameBuffer()->getBlend2dImage(), &ctxInfo);
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
			case 'G':
				gAutoGrow = !gAutoGrow;
			break;
			
			case VK_PLAY:
			case VK_PAUSE:
			case 'R':
				getRecorder()->toggleRecording();
				if (getRecorder()->isRecording())
				{
					gRecordingStart = seconds();
				}
				else
				{
					double duration = seconds() - gRecordingStart;
					printf("Recording Frames: %d  Duration: %3.2f  FPS: %f\n", getRecorder()->frameCount(), duration, getRecorder()->frameCount() / duration);
				}
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
	//loadFontDirectory("c:\\windows\\fonts");
	loadDefaultFonts();
	//loadFontDirectory("d:\\commonfonts");
	//loadFontDirectory("..\\resources");

}


// called once before main loop is running
void setup()
{
    //printf("setup()\n");

	// register to receive various events
	subscribe(onFileDrop);
	subscribe(onResizeEvent);
	//subscribe(onFrameEvent);
	subscribe(onMouseEvent);
	subscribe(onKeyboardEvent);


	setupFonts();
	
	setFrameRate(30);
	
    dropFiles();
	
	// set app window size and title
	createAppWindow(1280, 1024, "SVGViewer");
	
	getRecorder()->reset(&getAppFrameBuffer()->getBlend2dImage(), "frame", 15, 0);
	
	
	// Set the initial viewport
	//gViewPort.surfaceFrame({0, 0, (double)canvasWidth, (double)canvasHeight});
	gNavigator.setFrame({ 0, 0, (double)appFrameWidth, (double) appFrameHeight });
	gNavigator.subscribe(handleChange);
	
	// Load extension elements
	DisplayCaptureElement::registerFactory();
	//SVGScriptElement::registerFactory();
}
