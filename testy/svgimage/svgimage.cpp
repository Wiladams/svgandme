
#include <filesystem>


#include "svg.h"
#include "viewport.h"

#include "mappedfile.h"


using namespace waavs;

// Create one of these first, so factory constructor will run
//SVGFactory gSVG;

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
				bool success = gFontHandler.loadFontFace(dir_entry.path().generic_string().c_str(), ff);
			}
			
		}
	}
}


static void setupFonts()
{
	loadFontDirectory("c:\\Windows\\Fonts");

}



#define CAN_WIDTH 800
#define CAN_HEIGHT 600


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
    gDoc = SVGFactory::createFromChunk(mappedSpan, &gFontHandler, CAN_WIDTH, CAN_HEIGHT, 96);

    if (gDoc == nullptr)
        return 1;

	
	// Now that we've processed the document, we have correct sizing
	// Get the frame size from the document
	// This is the extent of the document, in user units
	BLRect sceneFrame = gDoc->getBBox();
	printf("viewport: %3.0f %3.0f %3.0f %3.0f\n", sceneFrame.x, sceneFrame.y, sceneFrame.w, sceneFrame.h);

	// Create a rectangle the size of the BLImage we want to render into
	BLRect surfaceFrame{ 0, 0, CAN_WIDTH, CAN_HEIGHT };
	
	
	// At this point, we could just do the render, but we go further
	// and create a viewport, which will handle the scaling and translation
	// This will essentially do a 'scale to fit'
	ViewportTransformer vp{};
	vp.viewBoxFrame(sceneFrame);
	vp.viewportFrame(surfaceFrame);


	// Now create a drawing context so we can
	// do the rendering
	IRenderSVG ctx(&gFontHandler);
	BLImage img(static_cast<int>(surfaceFrame.w), static_cast<int>(surfaceFrame.h), BL_FORMAT_PRGB32);
	
	// Attach the drawing context to the image
	// We MUST do this before we perform any other
	// operations, including the transform
	ctx.attach(img);
	ctx.clearAll();

	// apply the viewport's sceneToSurface transform to the context
	auto res = ctx.setTransform(vp.viewBoxToViewportTransform());
	//printf("setTransform RESULT: %d\n", res);
	

	
	// Render the document into the context
	//ctx.noStroke();
	gDoc->draw(&ctx, gDoc.get());
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
