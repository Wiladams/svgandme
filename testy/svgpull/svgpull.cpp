/*
    Just a general file for testing out stuff
*/

#include <iostream>

#include "mappedfile.h"
#include "xmlutil.h"


#include "svgdatatypes.h"
#include "svgpath.h"
#include "svgviewport.h"
#include "collections.h"

using namespace waavs;

static void printRect(const BLRect& aRect)
{
	printf("x=%f, y=%f, w=%f, h=%f\n", aRect.x, aRect.y, aRect.w, aRect.h);
}

static void printTransform(const BLMatrix2D& tform)
{
    printf("---- transform ----\n");
    printf("%3.2f  %3.2f\n", tform.m[0], tform.m[1]);
	printf("%3.2f  %3.2f\n", tform.m[2], tform.m[3]);
	printf("%3.2f  %3.2f\n", tform.m[4], tform.m[5]);
    
    
}

static void testFile(const char *filename)
{
    auto mapped = MappedFile::create_shared(filename);

    if (nullptr == mapped)
        return ;


    waavs::ByteSpan s(mapped->data(), mapped->size());

    //auto doc = SVGDOMDocument::createFromChunk(s);


    // close the mapped file
    mapped->close();
}

static void testViewport(const char *xmlattrs)
{
    // Setup an XMLAttribute collection from a static string
    XmlAttributeCollection attrs{};
	attrs.scanAttributes(xmlattrs);
    
    // Create a viewport, and load the attributes
	SVGViewport vp{};
	vp.loadFromAttributes(attrs);

    // Create a render context so the viewport can be bound to it
	IRenderSVG ctx(nullptr);
	
	// Bind the viewport to the context
	vp.bindToContext(&ctx, nullptr);
	
    
    // Print out the viewport's bounding box
    printf("================\n%s\n-----------------\n", xmlattrs);
	printRect(vp.fViewport.surfaceFrame());
	printRect(vp.fViewport.sceneFrame());
	printTransform(vp.fViewport.sceneToSurfaceTransform());
}

int main(int argc, char** argv)
{

    // create an mmap for the specified file
    if (argc > 1) {
        const char* filename = argv[1];
        //testFile(filename);
    }

    testViewport("width='100' height='100' viewBox='0 0 200 200' preserveAspectRatio='xMidYMid meet'");
    testViewport("x='10' y='15' width='100' height='100' viewBox='0 0 200 200' preserveAspectRatio='xMidYMid meet'");
    testViewport("viewBox='0 0 80 20'");
    testViewport("id = 'myDot' width = '10' height = '10' viewBox = '0 0 2 2'");
    
    return 0;
}
