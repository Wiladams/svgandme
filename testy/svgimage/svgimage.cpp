
#include "svg.h"

#include "filestreamer.h"


using namespace waavs;

// Create one of these first, so factory constructor will run
SVGFactory gSVG;

// Reference to currently active document
std::shared_ptr<SVGDocument> gDoc{ nullptr };

FontHandler gFontHandler{};

static void setupFonts()
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
    auto mapped = FileStreamer::createFromFilename(filename);

    
	// if the mapped file does not exist, return
	if (mapped == nullptr)
	{
		printf("File not found: %s\n", filename);
		return 1;
	}
    
    gDoc = SVGDocument::createFromChunk(mapped->span(), &gFontHandler);


    if (gDoc == nullptr)
        return 1;

	auto rootNode = gDoc->documentElement();
    
    if (rootNode == nullptr)
        return 1;

    setupFonts();


	// Get the viewport size so we can create an image
	// This is not the best thing to do, as viewport can be very large or small
	BLRect r = rootNode->viewport();
	printf("viewport: %3.0f %3.0f %3.0f %3.0f\n", r.x, r.y, r.w, r.h);

			
	auto objFr = gDoc->sceneFrame();
	printf("gDoc::sceneFrame: %f,%f %f,%f\n", objFr.x, objFr.y, objFr.w, objFr.h);
	

	// Create a drawing context to render into
	SvgDrawingContext ctx(r.w, r.h, &gFontHandler);

	// Render the document into the context
	gDoc->draw(&ctx);

			
	// Save the image from the drawing context out to a file
		const char* outfilename = argv[2];
		ctx.saveToFile("testfile.png");
	


    return 0;
}
