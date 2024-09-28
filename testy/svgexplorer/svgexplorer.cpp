#pragma comment(lib, "blend2d.lib")

#include "app/apphost.h"

#include <filesystem>


#include "mappedfile.h"
#include "svguiapp.h"
#include "svgicons.h"
#include "svgcacheddocument.h"
#include "svgnavigator.h"
#include "svgdocumentbrowser.h"

using namespace waavs;


// Create one of these first, so factory constructor will run
static SVGFactory gSVG;


// Drawing context used for drawing document
// 	SvgDrawingContext ctx(&gFontHandler);
IRenderSVG gDrawingContext(nullptr);


// Animation management
bool gAnimate = false;
bool gPerformTransform = true;
bool gCheckerBackground = true;




SVGBrowsingView gBrowsingView(BLRect(128,24,800,600));




static void drawDocument()
{
	// draw the document into the ctx
	gBrowsingView.draw(&gDrawingContext);

	gDrawingContext.flush();
}

static void refreshDoc()
{
	drawDocument();
	screenRefresh();
}


// docFromFilename
//
// Given a filename, parse the svg in the file, and return 
// a shared pointer to it.
//
static void loadDocFromFilename(const char* filename)
{
	auto mapped = waavs::MappedFile::create_shared(filename);

	// if the mapped file does not exist, return
	if (mapped == nullptr)
	{
		printf("File not found: %s\n", filename);
		return ;
	}


	ByteSpan aspan(mapped->data(), mapped->size());
	auto doc = SVGDocument::createFromChunk(aspan, &getFontHandler(), canvasWidth, canvasHeight, systemDpi);
	
	gBrowsingView.resetFromDocument(doc);
	refreshDoc();
}





static void onFileDrop(const FileDropEvent& fde)
{
	// assuming there's at least one file that 
	// has been dropped.
	for (int i = 0; i < fde.filenames.size(); i++)
	{
		loadDocFromFilename(fde.filenames[i].c_str());

		refreshDoc();
	}
}

// Create a routine to respond to frameevents
static void onFrameEvent(const FrameCountEvent& fe)
{
	//printf("frameEvent: %d\n", (int)fe.frameCount);
	//appFrameBuffer().setAllPixels(vec4b{ 0x00,0xff,0x00,0xff });
	// update current document

		if (gAnimate)
		{
			//gDoc->update(gDoc.get());
			refreshDoc();
		}

	screenRefresh();


	gRecorder.saveFrame();
}

static void onResizeEvent(const ResizeEvent& re)
{
	//printf("onResizeEvent: %d x %d\n", re.width, re.height);
	gDrawingContext.begin(appFrameBuffer().image());
	refreshDoc();
}





static void portalChanged(const bool& changed)
{
	refreshDoc();
}

static void onMouseEvent(const MouseEvent& e)
{
	if (gBrowsingView.contains(BLPoint(e.x, e.y)))
		gBrowsingView.onMouseEvent(e);
}

static void onKeyboardEvent(const KeyboardEvent& ke)
{
	
	gBrowsingView.onKeyboardEvent(ke);
	
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
			refreshDoc();
			break;
		}
	}
}

static void setupFonts()
{
	//loadDefaultFonts();
	loadFontDirectory("c:\\windows\\fonts");
	//loadFontDirectory("..\\resources");
	loadFontDirectory("d:\\commonfonts");

	gDrawingContext.fontHandler(&getFontHandler());
}




// called once before main loop is running
static void setup()
{
	//printf("setup()\n");

	// Setup runtime specific stuff
	createAppWindow(1024, 768, "SVG Explorer");
	dropFiles();
	frameRate(20);

	// register to receive various events
	subscribe(onFileDrop);
	subscribe(onFrameEvent);
	subscribe(onMouseEvent);
	subscribe(onResizeEvent);
	subscribe(onKeyboardEvent);


	// Setup application specific items
	setupFonts();

	gRecorder.reset(&appFrameBuffer().image(), "frame", 15, 0);

	// clear the buffer to white to start
	appFrameBuffer().setAllPixels(vec4b{ 0xFF,0xff,0xff,0xff });
	BLContextCreateInfo ctxInfo{};
	//ctxInfo.threadCount = 4;
	ctxInfo.threadCount = 0;
	gDrawingContext.begin(appFrameBuffer().image(), &ctxInfo);

	// Set the initial viewport
	gBrowsingView.subscribe(portalChanged);
	
	refreshDoc();
}
