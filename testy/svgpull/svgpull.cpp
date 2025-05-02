/*
    Just a general file for testing out stuff
*/

#include <iostream>

#include "mappedfile.h"
#include "xmlutil.h"


#include "svgdatatypes.h"
#include "svgpath.h"
#include "svgportal.h"
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

static void testAViewport(const char *xmlattrs)
{
    // Setup an XMLAttribute collection from a static string
    XmlAttributeCollection attrs{};
	attrs.scanAttributes(xmlattrs);
    
    // Create a viewport, and load the attributes
    SVGPortal vp{};
	vp.loadFromAttributes(attrs);

    // Create a render context so the viewport can be bound to it
	IRenderSVG ctx;
	
	// Bind the viewport to the context
	vp.bindToContext(&ctx, nullptr);
	
    
    // Print out the viewport's bounding box
    printf("================\n%s\n-----------------\n", xmlattrs);
	printRect(vp.viewportFrame());
	printRect(vp.viewBoxFrame());
	printTransform(vp.viewBoxToViewportTransform());
}

static void testViewport()
{
    testAViewport("width='100' height='100' viewBox='0 0 200 200' preserveAspectRatio='xMidYMid meet'");
    testAViewport("x='10' y='15' width='100' height='100' viewBox='0 0 200 200' preserveAspectRatio='xMidYMid meet'");
    testAViewport("viewBox='0 0 80 20'");
    testAViewport("id = 'myDot' width = '10' height = '10' viewBox = '0 0 2 2'");
}

static void testPathParse1()
{
    BLPath aPath{};

    parsePath("M 10 10 L 90 90", aPath);
}


static void testPathParse()
{

    SVGPathSegmentIterator iter("M 10, 50Q 25, 25 40, 50t 30, 0 30, 0 30, 0 30, 0 30, 0");

    SVGSegmentParseState seg;
    while (iter.nextSegment(seg))
    {
		printf("%c ", seg.fSegmentKind);
        int n = strlen(seg.fArgTypes);
        switch (n) {
        case 7:
            printf("%3.2f ", seg.args[n - 7]);
        case 6:
            printf("%3.2f ", seg.args[n - 6]);
        case 5:
            printf("%3.2f ", seg.args[n - 5]);
        case 4:
            printf("%3.2f ", seg.args[n - 4]);
        case 3:
            printf("%3.2f ", seg.args[n - 3]);
        case 2:
            printf("%3.2f ", seg.args[n - 2]);
        case 1:
            printf("%3.2f ", seg.args[n - 1]);
        default:
            printf("\n");
        }
    }
}


int main(int argc, char** argv)
{

    // create an mmap for the specified file
    if (argc > 1) {
        const char* filename = argv[1];
        //testFile(filename);
    }


    //testViewport();
    testPathParse();
    
    return 0;
}
