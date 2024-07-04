#pragma comment(lib, "blend2d.lib")

#include "app/apphost.h"

#include <filesystem>

#include "svg.h"
#include "mappedfile.h"
#include "recorder.h"
#include "svgdomdocument.h"

using namespace waavs;

// Create one of these first, so factory constructor will run
SVGFactory gSVG;

FontHandler gFontHandler{};

// Reference to currently active document
//std::shared_ptr<SVGDocument> gDoc{ nullptr };

Recorder gRecorder{ nullptr };


static std::shared_ptr<SVGDOMDocument> docFromFilename(const char* filename)
{
	auto mapped = waavs::MappedFile::create_shared(filename);

	// if the mapped file does not exist, return
	if (mapped == nullptr)
	{
		printf("File not found: %s\n", filename);
		return nullptr;
	}

	ByteSpan aspan(mapped->data(), mapped->size());
	std::shared_ptr<SVGDocument> aDoc = SVGDocument::createFromChunk(aspan, &gFontHandler);

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
		gDoc = docFromFilename(fde.filenames[i].c_str());


		//double endTime = seconds();
		//printf("== fileDrop: SVGDocument::createFromFilename took %f seconds\n", endTime - startTime);

		if (gDoc != nullptr)
		{
			//resetView();

			//auto objFr = gDoc->sceneFrame();

			// Set the initial viewport
			//gViewPort.surfaceFrame({ 0, 0, (double)canvasWidth, (double)canvasHeight });
			//gViewPort.surfaceFrame(objFr);
			//gViewPort.sceneFrame(objFr);

			//refreshDoc();
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
	//loadFontDirectory("c:\\windows\\fonts");
	//loadFontDirectory("..\\resources");


	// resize app window
	createAppWindow(1024, 768, "SVG DOM");

	gRecorder.reset(&appFrameBuffer().image(), "frame", 15, 0);

	// register to receive various events
	dropFiles();
	subscribe(onFileDrop);
	
	//subscribe(onFrameEvent);
	//subscribe(onMouseEvent);
	//subscribe(onResizeEvent);
	//subscribe(onKeyboardEvent);

	// clear the buffer to white to start
	appFrameBuffer().setAllPixels(vec4b{ 0xFF,0xff,0xff,0xff });

}

