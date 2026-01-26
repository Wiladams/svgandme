
// This is a simple example of how to use the SVG library
// to render an SVG file into a bitmap image using svgandme 
// with the blend2d library.  
// The SVGFactory is used to create a document from a file, 
// and then uses the SVGB2DDriver to render the document into a BLImage.
//
// The example also shows how to use the ViewportTransformer
// to scale and translate the document to fit into the 
// image.

#include <filesystem>

#include "mappedfile.h"
#include "svg.h"
#include "viewport.h"
#include "svgb2ddriver.h"


using namespace waavs;

// These determine the size of the output image
// The drawing is scaled to fit within this size
// Change it to whatever you want
#define CAN_WIDTH 1920
#define CAN_HEIGHT 1280


// The SVG library itself doesn't know anything about the
// file system, so this routines helps load fonts from a director
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

// Here we load up whatever system fonts our drawing will use
// The default example here loads the Windows system fonts
static void setupFonts()
{
	loadFontDirectory("c:\\Windows\\Fonts");
}

// createDocument
// This is a little helper that takes a filename and creates a
// SVGDocument from it.  You can create an in memory representation 
// of the documet however you like.
// Calling SVGFactory::createFromChunk() is the critical piece
//
static SVGDocumentHandle createDocument(const char *filename)
{
	auto mapped = MappedFile::create_shared(filename);
	
	// if the mapped file does not exist, return
	if (mapped == nullptr)
	{
		printf("File not found: %s\n", filename);
		return nullptr;
	}

	ByteSpan mappedSpan;
	mappedSpan.resetFromSize(mapped->data(), mapped->size());
	auto doc = SVGFactory::createFromChunk(mappedSpan, CAN_WIDTH, CAN_HEIGHT, 96.0);

	return doc;
}

static void renderImage(SVGDocumentHandle gDoc, const char* outfilename)
{
	if (gDoc == nullptr)
	{
		printf("Document is null\n");
		return;
	}

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

	// If the document does not already have a viewBox/width/height
	// getBBox() will not return a proper size.  So, the document
	// must be well formed in that way for this to work.
	// This is the extent of the document, in user units
	BLRect sceneFrame = gDoc->getBBox();
	printf("viewport: %3.0f %3.0f %3.0f %3.0f\n", sceneFrame.x, sceneFrame.y, sceneFrame.w, sceneFrame.h);


	// At this point, we could just do the render, but we go further
	// and create a viewport, which will handle the scaling and translation
	// This will essentially do a 'scale to fit'
	// You don't have to go this route.  You can simply calculate the scaling
	// and apply that to the context.
	ViewportTransformer vp{};
	vp.getViewBoxFrame(sceneFrame);		// The part of the scene we want to display
	vp.setViewportFrame(surfaceFrame);		// The surface we want to fit to 

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
	img.writeToFile(outfilename);
}


int main(int argc, char** argv)
{
	printf("argc: %d\n", argc);
	
	if (argc < 2)
    {
        printf("Usage: svgimage <svg file>  [output file]\n");
        return 1;
    }
	
	// This is a platform specific step required to get fonts setup
	// before rendering.
	setupFonts();

    // create an mmap for the specified file
    const char* infilename = argv[1];
	
	const char* outfilename = nullptr;
	if (argc >= 3)
		outfilename = argv[2];
	else
		outfilename = "output.png";

	auto gDoc = createDocument(infilename);

	renderImage(gDoc, outfilename);

    return 0;
}
