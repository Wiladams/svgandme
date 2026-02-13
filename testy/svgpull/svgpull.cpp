/*
    Just a general file for testing out stuff
*/

#include <iostream>

#include "mappedfile.h"
#include "xmlutil.h"


#include "svgdatatypes.h"
#include "collections.h"
#include "b2dpathbuilder.h"
#include "pathsegmenter.h"
#include "viewport.h"


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


    waavs::ByteSpan s;
    s.resetFromSize(mapped->data(), mapped->size());

    //auto doc = SVGDOMDocument::createFromChunk(s);


    // close the mapped file
    mapped->close();
}

static void testAViewport(const char *xmlattrs)
{
    // Setup an XMLAttribute collection from a static string
    XmlAttributeCollection attrs{};
	scanAttributes(attrs, xmlattrs);
    
    // Create a viewport, and load the attributes
    DocViewportState docvps{};
    loadDocViewportState(docvps, attrs);

	
	// Bind the viewport to the context
    SVGViewportState vp{};
    BLRect containingVP{ 0, 0, 800, 600 };

    resolveViewState(containingVP, docvps, true, 96, nullptr, vp);


    
    // Print out the viewport's bounding box
    printf("================\n%s\n-----------------\n", xmlattrs);
	printRect(vp.fViewport);
	printRect(vp.fViewBox);
	printTransform(vp.viewBoxToViewportXform);
}

static void testViewport()
{
    testAViewport("width='100' height='100' viewBox='0 0 200 200' preserveAspectRatio='xMidYMid meet'");
    testAViewport("x='10' y='15' width='100' height='100' viewBox='0 0 200 200' preserveAspectRatio='xMidYMid meet'");
    testAViewport("viewBox='0 0 80 20'");
    testAViewport("id = 'myDot' width = '10' height = '10' viewBox = '0 0 2 2'");
}


int main(int argc, char** argv)
{
    // create an mmap for the specified file
    if (argc > 1) {
        const char* filename = argv[1];
        //testFile(filename);
    }

    testViewport();

    return 0;
}
