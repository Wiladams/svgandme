
#include <filesystem>

#include "mappedfile.h"

#include "svg.h"
#include "viewport.h"

#include "svgb2ddriver.h"


using namespace waavs;

#define CAN_WIDTH 1920
#define CAN_HEIGHT 1280


// Reference to currently active document
std::shared_ptr<SVGDocument> gDoc{ nullptr };

FontHandler gFontHandler{};



static void loadFontDirectory(const char* dir)
{
	const std::filesystem::path fontPath(dir);
	
	for (const auto& dir_entry : std::filesystem::directory_iterator(fontPath))
	{
		if (dir_entry.is_regular_file())
		{
			if ((dir_entry.path().extension() == ".ttf") ||
				(dir_entry.path().extension() == ".otf") ||
				(dir_entry.path().extension() == ".TTF"))
			{
				BLFontFace ff{};
				if (!FontHandler::getFontHandler()->loadFontFace(dir_entry.path().generic_string().c_str(), ff))
				{
					printf("FontHandler::loadFontFace() failed: %s\n", dir_entry.path().generic_string().c_str());
					return;
				}
			}
		}
	}
}


static void setupFonts()
{
	loadFontDirectory("c:\\Windows\\Fonts");
}


int main(int argc, char **argv)
{

	printf("argc: %d\n", argc);
	
	if (argc < 2)
    {
        printf("Usage: svgimage <xml file>  [output file]\n");
        return 1;
    }
	
	setupFonts();

    // create an mmap for the specified file
    const char* filename = argv[1];

	auto mapped = MappedFile::create_shared(filename);
    
	// if the mapped file does not exist, return
	if (mapped == nullptr)
	{
		printf("File not found: %s\n", filename);
		return 1;
	}
    
	ByteSpan mappedSpan(mapped->data(), mapped->size());
	gDoc = SVGFactory::createFromChunk(mappedSpan, FontHandler::getFontHandler(), CAN_WIDTH, CAN_HEIGHT, 96.0);

    if (gDoc == nullptr)
		return 1;

	// Create an image that we will draw into
	BLImage img(CAN_WIDTH, CAN_HEIGHT, BL_FORMAT_PRGB32);

	// Attach the drawing context to the image
	// We MUST do this before we perform any other
	// operations, including the transform
	BLContextCreateInfo createInfo{};
	createInfo.threadCount = 4;
	SVGB2DDriver ctx;
	ctx.attach(img, &createInfo);

	// setup for drawing.  Not strictly necessary.
	// you can setup the context however you want before actually
	// drawing.
	ctx.background(BLRgba32(0xFFFFFFFF));	// white background
	ctx.renew();

	// We are doing 'fit to canvas' drawing, so we need
	// to calculate the scaling factors for the x and y 
	// axes.  The easiest way to do that is to create a viewport
	// Create a rectangle the size of the BLImage we want to render into
	BLRect surfaceFrame{ 0, 0, CAN_WIDTH, CAN_HEIGHT };

	// Now that we've processed the document, we have correct sizing
	// Get the frame size from the document
	// This is the extent of the document, in user units
	BLRect sceneFrame = gDoc->getBBox();
	printf("viewport: %3.0f %3.0f %3.0f %3.0f\n", sceneFrame.x, sceneFrame.y, sceneFrame.w, sceneFrame.h);


	// At this point, we could just do the render, but we go further
	// and create a viewport, which will handle the scaling and translation
	// This will essentially do a 'scale to fit'
	ViewportTransformer vp{};
	vp.viewBoxFrame(sceneFrame);		// The part of the scene we want to display
	vp.viewportFrame(surfaceFrame);		// The surface we want to fit to 
	
	// apply the viewport's sceneToSurface transform to the context
	BLMatrix2D tform = vp.viewBoxToViewportTransform();
	ctx.transform(tform);
	
	// Render the document into the context
	gDoc->draw(&ctx, gDoc.get());

	// detach automatically does a final flush, 
	// so we're ready to save the image after that
	ctx.detach();
	
	// Save the image from the drawing context out to a file
	// or do whatever you're going to do with it
	const char* outfilename = nullptr;

	
	if (argc >= 3)
		outfilename = argv[2];
	else 
		outfilename = "output.png";

	img.writeToFile(outfilename);


    return 0;
}
