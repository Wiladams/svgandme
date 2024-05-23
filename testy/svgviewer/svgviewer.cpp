#pragma comment(lib, "blend2d.lib")

#include "app/apphost.h"

#include "svg.h"
#include "fonthandler.h"
#include "filestreamer.h"


using namespace waavs;

// Create one of these first, so factory constructor will run
SVGFactory gSVG;

FontHandler gFontHandler{};

// Reference to currently active document
std::shared_ptr<SVGDocument> gDoc{ nullptr };


// Typography
static BLFontFace loadFont(const char* filename)
{
    BLFontFace ff = gFontHandler.loadFontFace(filename);
    return ff;
}

static void loadFontDirectory(const char* dir)
{
    gFontHandler.loadDirectoryOfFonts(dir);
}

static void loadDefaultFonts()
{
    gFontHandler.loadDefaultFonts();
}

static void loadFontFiles(std::vector<const char*> filenames)
{
    gFontHandler.loadFonts(filenames);
}



static std::shared_ptr<SVGDocument> docFromFilename(const char* filename)
{
	auto mapped = FileStreamer::createFromFilename(filename);
	
	// if the mapped file does not exist, return
	if (mapped == nullptr)
	{
		printf("File not found: %s\n", filename);
		return nullptr;
	}

	std::shared_ptr<SVGDocument> aDoc = SVGDocument::createFromChunk(mapped->span(), &gFontHandler);
	return aDoc;
}

static void drawDocument(std::shared_ptr<SVGDocument> doc)
{
	auto rootNode = doc->documentElement();

	if (rootNode == nullptr)
		return ;

	
	BLRect r = rootNode->viewport();
	printf("viewport: %3.0f %3.0f %3.0f %3.0f\n", r.x, r.y, r.w, r.h);

	auto objFr = gDoc->sceneFrame();
	printf("gDoc::sceneFrame: %f,%f %f,%f\n", objFr.x, objFr.y, objFr.w, objFr.h);

	
	// Create a SvgDrawingContext for the canvas
	SvgDrawingContext ctx(&gFontHandler);
	ctx.begin(appFrameBuffer().image());

	// setup any transform

	// draw the document into the ctx
	//appFrameBuffer().setAllPixels(vec4b{ 0x00,0x00,0xff,0xff });
	doc->draw(&ctx);
	//ctx.background(BLRgba32(0xff0000ff));
	
	// force a screen refresh
	ctx.flush();
	

}

static void onFileDrop(const FileDropEvent& fde)
{
	// assuming there's at least one file that 
	// has been dropped.
	for (int i = 0; i < fde.filenames.size(); i++)
	{
		// Create a new SVGDocument for each file
		// And create a window to display each document
		double startTime = seconds();
		gDoc = docFromFilename(fde.filenames[i].c_str());


		double endTime = seconds();
		printf("== fileDrop: SVGDocument::createFromFilename took %f seconds\n", endTime - startTime);

		drawDocument(gDoc);

	}
}

// Create a routine to respond to frameevents
static void onFrameEvent(const FrameCountEvent& fe)
{
	printf("frameEvent: %d\n", (int)fe.frameCount);
	//appFrameBuffer().setAllPixels(vec4b{ 0x00,0xff,0x00,0xff });
	
	screenRefresh();
}

static void drawTest()
{
	for (int i = 0; i < 200; i++)
	{
		appFrameBuffer().setPixel(i, i, vec4b{ 0xff,0x00,0x00,0xff });
	}

	// Create a BLImage from the canvas
	BLImage img;
	img.createFromData(canvasWidth, canvasHeight, BL_FORMAT_PRGB32, canvasPixelData, canvasStride);

	// Create a BLContext and begin on the image
	SvgDrawingContext ctx(&gFontHandler);
	//BLContext ctx{};

	// Draw a rectangle
	ctx.begin(img);
	ctx.setFillStyle(BLRgba32(0xff0000ff));
	ctx.fillRect(10, 10, 100, 100);
	//ctx.flush(BLContextFlushFlags::BL_CONTEXT_FLUSH_SYNC);
	ctx.flush();


}

// called once before main loop is running
void onLoad()
{
    printf("onLoad\n");
    
    loadDefaultFonts();
    dropFiles();


	layered();
	// resize app window
	//createAppWindow(1024, 768, "SVGViewer");
	
	// register onFileDrop for file drop events
	subscribe(onFileDrop);
	subscribe(onFrameEvent);

	// clear the buffer to white to start
	appFrameBuffer().setAllPixels(vec4b{ 0xFF,0xff,0xff,0xff });
	
	//drawTest();
}
