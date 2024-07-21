#pragma comment(lib, "blend2d.lib")

#include "app/apphost.h"

#include <filesystem>


#include "mappedfile.h"
#include "svguiapp.h"


using namespace waavs;


// Reference to currently active document
std::shared_ptr<SVGDocument> gDoc{ nullptr };
ViewPort gViewPort{};




// For mouse management
static bool gIsDragging = false;
static vec2f gDragPos{ 0,0 };
static double gZoomFactor = 0.1;









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
	std::shared_ptr<SVGDocument> aDoc = SVGDocument::createFromChunk(aspan, &gFontHandler, canvasWidth, canvasHeight, systemPpi);

	
	return aDoc;
}

static void drawDocument(std::shared_ptr<SVGDocument> doc)
{
	if (doc == nullptr)
		return;
	
	auto rootNode = doc->documentElement();

	if (rootNode == nullptr)
		return ;
	
	// Create a SvgDrawingContext for the canvas
	SvgDrawingContext ctx(&gFontHandler);
	BLContextCreateInfo ctxInfo{};
	ctxInfo.threadCount = 4;
	ctx.begin(appFrameBuffer().image(), &ctxInfo);

	// setup any transform
	ctx.setTransform(gViewPort.sceneToSurfaceTransform());

	//double startTime = seconds();

	// draw the document into the ctx
	doc->draw(&ctx);
	ctx.flush();
	
	//double endTime = seconds();
	//printf("Drawing Duration: %f\n", endTime - startTime);
}

static void refreshDoc()
{
	// Clear the background to white
	appFrameBuffer().setAllPixels(vec4b{ 0xff,0xff,0xff,0xff });
	
	drawDocument(gDoc);
}

static void resetView()
{
	gViewPort.reset();
	gViewPort.sceneFrame(BLRect(0, 0, canvasWidth, canvasHeight));
	gViewPort.surfaceFrame(BLRect(0, 0, canvasWidth, canvasHeight));
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
		//printf("== fileDrop: SVGDocument::createFromFilename took %f seconds\n", endTime - startTime);

		if (gDoc != nullptr)
		{
			resetView();

			//auto objFr = gDoc->sceneFrame();
			auto objFr = gDoc->frame();
			
			// Set the initial viewport
			gViewPort.surfaceFrame({ 0, 0, (double)canvasWidth, (double)canvasHeight });
			//gViewPort.surfaceFrame(objFr);
			gViewPort.sceneFrame(objFr);
			
			refreshDoc();
			break;
		}

	}
}

// Create a routine to respond to frameevents
static void onFrameEvent(const FrameCountEvent& fe)
{
	//printf("frameEvent: %d\n", (int)fe.frameCount);
	//appFrameBuffer().setAllPixels(vec4b{ 0x00,0xff,0x00,0xff });
	
	screenRefresh();


	gRecorder.saveFrame();
}

static void onResizeEvent(const ResizeEvent& re)
{
	//printf("onResizeEvent: %d x %d\n", re.width, re.height);
	refreshDoc();
}

// Pan
// This is a translation, so it will move the viewport in the opposite direction
// of the provided values
static void pan(double dx, double dy)
{
	//printf("SVGViewer::pan(%3.2f, %3.2f)\n", dx, dy);

	gViewPort.translateBy(-dx, -dy);
	refreshDoc();
}


// zoomBy
// Zoom in or out, by a specified amount (cumulative)  
// z >  1.0 ==> zoom out, showing more of the scene
// z <  1.0 ==> zoom 'in', focusing on a smaller portion of the scene.
// The zoom can be centered around a specified point
static void zoomBy(double z, double cx = 0, double cy = 0)
{
	gViewPort.scaleBy(z, z, cx, cy);
	refreshDoc();

}

static void rotateBy(double r, double cx = 0, double cy = 0)
{
	gViewPort.rotateBy(r, cx, cy);
	refreshDoc();
}

static void onMouseEvent(const MouseEvent& e)
{
	MouseEvent lev = e;
	lev.x = float((double)e.x );
	lev.y = float((double)e.y );

	//printf("SVGViewer::mouseEvent: %f,%f\n", lev.x, lev.y);
	
	switch (e.activity)
	{
		// When the mouse is pressed, get into the 'dragging' state
		case MOUSEPRESSED:
			gIsDragging = true;
			gDragPos = { e.x, e.y };
		break;

		// When the mouse is released, get out of the 'dragging' state
		case MOUSERELEASED:
			gIsDragging = false;
		break;

		case MOUSEMOVED:
		{
			auto lastPos = gViewPort.surfaceToScene(gDragPos.x, gDragPos.y);
			auto currPos = gViewPort.surfaceToScene(e.x, e.y);

			//printf("SVGView::mouseEvent - currPos = %3.2f, %3.2f\n", currPos.x, currPos.y);

			if (gIsDragging)
			{
				double dx = currPos.x - lastPos.x;
				double dy = currPos.y - lastPos.y;

				// print mouse lastPos and currPos
				//printf("-----------------------------\n");
				//printf("lastPos = %3.2f, %3.2f\n", lastPos.x, lastPos.y);

				//printf("dx = %3.2f, dy = %3.2f\n", dx, dy);

				pan(dx, dy);
				gDragPos = { e.x, e.y };
			}
		}
		break;

		// We want to use the mouse wheel to 'zoom' in and out of the view
		// We will only zoom when the 'alt' key is pressed
		// Naked scroll might be used for something else
		case MOUSEWHEEL:
		{
			//printf("SVGView: MOUSEWHEEL\n");

			if (e.delta < 0)
				zoomBy(1.0 + gZoomFactor, lev.x, lev.y);
			else
				zoomBy(1.0 - gZoomFactor, lev.x, lev.y);
		}
		break;

		
		// Horizontal mouse wheel
		// Rotate around central point
		case MOUSEHWHEEL:
		{
			//printf("SVGView: MOUSEHWHEEL\n");
			if (e.delta < 0)
				rotateBy(waavs::radians(5.0f), lev.x, lev.y);
			else
				rotateBy(waavs::radians(-5.0f), lev.x, lev.y);
		}
		break;
		
	}
	
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

		}
	}
}

// called once before main loop is running
void onLoad()
{
    printf("onLoad\n");
    
	
	// Load extension elements
	
	
	frameRate(15);
	
    //loadDefaultFonts();
	loadFontDirectory("c:\\windows\\fonts");
	loadFontDirectory("..\\resources");
	
    dropFiles();


	//layered();
	// resize app window
	createAppWindow(1024, 768, "SVGViewer");
	
	gRecorder.reset(&appFrameBuffer().image(), "frame", 15, 0);
	
	// register to receive various events
	subscribe(onFileDrop);
	subscribe(onFrameEvent);
	subscribe(onMouseEvent);
	subscribe(onResizeEvent);
	subscribe(onKeyboardEvent);
	
	// clear the buffer to white to start
	appFrameBuffer().setAllPixels(vec4b{ 0xFF,0xff,0xff,0xff });
	
	// Set the initial viewport
	gViewPort.surfaceFrame({0, 0, (double)canvasWidth, (double)canvasHeight});
	

}
