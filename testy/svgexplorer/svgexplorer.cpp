#pragma comment(lib, "blend2d.lib")

#include "app/apphost.h"

#include <filesystem>


#include "mappedfile.h"
#include "svguiapp.h"
#include "svgwaavs.h"

#include "svgdocumentbrowser.h"
#include "svgfilelistview.h"
#include "bgselector.h"
#include "svgicons.h"
#include "svgb2ddriver.h"


using namespace waavs;


constexpr int APP_WIDTH = 2560;
constexpr int APP_HEIGHT = 1440;
constexpr int APP_HMARGIN = 10;
constexpr int APP_VMARGIN = 10;
constexpr int APP_TOOL_MARGIN = 64;

constexpr int EXPLORER_LEFT = APP_HMARGIN;
constexpr int EXPLORER_TOP = APP_VMARGIN;
constexpr int EXPLORER_WIDTH = 256;
constexpr int EXPLORER_HEIGHT = APP_HEIGHT - APP_VMARGIN - APP_TOOL_MARGIN;

constexpr int BROWSER_LEFT = 280;
constexpr int BROWSER_TOP = APP_VMARGIN;
constexpr int BROWSER_WIDTH = APP_WIDTH - 256 - APP_HMARGIN - APP_HMARGIN - APP_HMARGIN;
constexpr int BROWSER_HEIGHT = APP_HEIGHT - APP_VMARGIN - APP_TOOL_MARGIN;

constexpr int BROWSER_TOOL_TOP = BROWSER_TOP + BROWSER_HEIGHT + APP_HMARGIN;
constexpr int BROWSER_TOOL_WIDTH = BROWSER_WIDTH;
constexpr int BROWSER_TOOL_HEIGHT = 64;

// Create one of these first, so factory constructor will run
static SVGFactory gSVG;


// Drawing context used for drawing document
SVGB2DDriver gDrawingContext;


// Animation management
bool gAnimate = false;
bool gPerformTransform = true;
bool gCheckerBackground = true;



SVGBrowsingView gBrowsingView(BLRect(BROWSER_LEFT, BROWSER_TOP, BROWSER_WIDTH, BROWSER_HEIGHT));
SVGFileListView gFileListView(BLRect(EXPLORER_LEFT, EXPLORER_TOP, EXPLORER_WIDTH, EXPLORER_HEIGHT));
BackgroundSelector gBrowserTool(BLRect(BROWSER_LEFT, BROWSER_TOOL_TOP, BROWSER_TOOL_WIDTH, BROWSER_TOOL_HEIGHT));


static void drawDocument()
{
	// draw the document into the ctx
	gBrowsingView.draw(&gDrawingContext);
	gFileListView.draw(&gDrawingContext);
	gBrowserTool.draw(&gDrawingContext);

	gDrawingContext.flush();
}

static void refreshDoc()
{
	drawDocument();
	refreshScreenNow();
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

	ByteSpan aspan;
	aspan.resetFromSize(mapped->data(), mapped->size());
	auto doc = SVGFactory::createFromChunk(aspan, FontHandler::getFontHandler(), appFrameWidth, appFrameHeight, physicalDpi);
	
	gBrowsingView.resetFromDocument(doc);
	refreshDoc();
}




//
// onFileDrop
//
// We do two things here.  You can drop a file directly on the viewing
// panel, or you can drop files onto the explorer panel.
//
static void onFileDrop(const FileDropEvent& fde)
{
	if (gFileListView.contains(fde.x, fde.y))
	{
		gFileListView.onFileDrop(fde);
		refreshDoc();
	}
	else {

		// assuming there's at least one file that 
		// has been dropped.
		for (int i = 0; i < fde.filenames.size(); i++)
		{
			loadDocFromFilename(fde.filenames[i].c_str());

			refreshDoc();
			break;
		}
	}
}

// Create a routine to respond to frameevents
static void onFrameEvent(const FrameCountEvent& fe)
{
	//printf("frameEvent: %d\n", (int)fe.frameCount);

	// update current document
	gBrowsingView.onFrameEvent(fe);
	
	if (gAnimate)
	{
		refreshDoc();
	}
	else {
		refreshScreenNow();
	}

	getRecorder()->saveFrame();
}

static void onResizeEvent(const ResizeEvent& re)
{
	//printf("onResizeEvent: %d x %d\n", re.width, re.height);
	gDrawingContext.attach(getAppFrameBuffer()->getBlend2dImage());
	refreshDoc();
}





static void portalChanged(const bool& changed)
{
	refreshDoc();
}

static void fileSelected(const FileIcon& fIcon)
{
	gBrowsingView.resetFromDocument(fIcon.document());
	refreshDoc();
}

static void onMouseEvent(const MouseEvent& e)
{
	//printf("svgexplorer::onMouseEvent: %d XButton1: %d XButtons2: %d %3f %3f\n", e.activity, e.xbutton1, e.xbutton2, e.x, e.y);

	
	if (gBrowsingView.contains(e.x, e.y))
		gBrowsingView.onMouseEvent(e);
	else if (gFileListView.contains(e.x, e.y))
		gFileListView.onMouseEvent(e);
}

static double recordBeginTime{};

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
			if (!getRecorder()->isRecording())
				recordBeginTime = seconds();
			else
			{
				double duration = seconds() - recordBeginTime;
				double fps = getRecorder()->frameCount() / duration;

				printf("Recording - Seconds: %f  Frames: %d  FPS: %f\n", duration, getRecorder()->frameCount(), fps);
			}
			getRecorder()->toggleRecording();
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
	//loadFontDirectory("x:\\Fonts\\Fonts");
	loadFontDirectory("x:\\Fonts\\commonfonts");

	//gDrawingContext.fontHandler(FontHandler::getFontHandler());
}




// called once before main loop is running
static void setup()
{
	//printf("setup()\n");

	// Setup runtime specific stuff
	createAppWindow(APP_WIDTH, APP_HEIGHT, "File Explorer");
	dropFiles();
	setFrameRate(30);

	// register to receive various events
	subscribe(onFileDrop);
	subscribe(onFrameEvent);
	subscribe(onMouseEvent);
	subscribe(onResizeEvent);
	subscribe(onKeyboardEvent);


	// Setup application specific items
	setupFonts();

	getRecorder()->reset(&getAppFrameBuffer()->getBlend2dImage(), "frame", 15, 0);

	// clear the buffer to white to start

	BLContextCreateInfo ctxInfo{};
	ctxInfo.threadCount = 4;
	//ctxInfo.threadCount = 0;
	gDrawingContext.attach(getAppFrameBuffer()->getBlend2dImage(), &ctxInfo);
	gDrawingContext.background(BLRgba32(0xffffffff));
	
	// Set the initial viewport
	gBrowsingView.subscribe(portalChanged);
	gFileListView.Topic<bool>::subscribe(portalChanged);
	gFileListView.Topic<FileIcon>::subscribe(fileSelected);

	DisplayCaptureElement::registerFactory();
	
	refreshDoc();
}
