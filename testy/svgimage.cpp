
#include "svg.h"

#include "filestreamer.h"
#include "xmlutil.h"

using namespace waavs;

// Create one of these first, so factory constructor will run
SVGFactory gSVG;

// Reference to currently active document
std::shared_ptr<SVGDocument> gDoc{ nullptr };

FontHandler gFontHandler{};

void setupFonts()
{
    gFontHandler.loadDefaultFonts();

}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: svgimage <xml file>  [output file]\n");
        return 1;
    }

    // create an mmap for the specified file
    const char* filename = argv[1];
    //auto mapped = FileStreamer::createFromFilename(filename);
	gDoc = SVGDocument::createFromFilename(filename, &gFontHandler);

    if (gDoc == nullptr)
        return 1;

	auto rootNode = gDoc->documentElement();
    
    if (rootNode == nullptr)
        return 1;

    setupFonts();

    // 
    // Parse the mapped file as XML
    // printing out the con

			BLRect r = rootNode->viewport();
			printf("viewport: %3.0f %3.0f %3.0f %3.0f\n", r.x, r.y, r.w, r.h);

			
			auto objFr = gDoc->sceneFrame();
			printf("gDoc::sceneFrame: %f,%f %f,%f\n", objFr.x, objFr.y, objFr.w, objFr.h);
			
			//gCameraView->resetView();
			//gCameraView->clearAll();
			//gCameraView->scene(gDoc);

			//if (objFr.w > 0 && objFr.h > 0)
			//	gCameraView->sceneFrame(objFr);

			



    return 0;
}
