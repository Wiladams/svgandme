/*
    convert a .svg file into the series of blend2d api calls
    that will be generated from it.
*/
#pragma comment(lib, "blend2d.lib") // Link with Blend2D static library, on Windows

#include <iostream>

#include "mappedfile.h"
#include "xmlutil.h"

#include "collections.h"
#include "svgfactory.h"
#include "SVGAPIPrinter.h"

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

static void printADoc(const ByteSpan &src)
{
    auto doc = SVGFactory::createDOM(src, nullptr);

    std::unique_ptr<SVGAPIPrinter> printer = std::make_unique<SVGAPIPrinter>();

    doc->draw(printer.get(), doc.get());
}

static void testFile(int argc, char **argv)
{
    // create an mmap for the specified file
    if (argc <= 1)
        return;
   
    const char* filename = argv[1];

    auto mapped = MappedFile::create_shared(filename);

    if (nullptr == mapped)
        return;


    waavs::ByteSpan s(mapped->data(), mapped->size());

    printADoc(s);

    // close the mapped file
    mapped->close();
}

static void testChunk()
{
    const char * s = R"||(
    <svg xmlns = 'http://www.w3.org/2000/svg' width = "1920" height = "1280" viewBox = "0 0 1920 1280">
        <path
        style = 'fill:#ffffff; stroke:url(#linearGradient4068); stroke-width:15; stroke-linecap:round; stroke-linejoin:miter; stroke-miterlimit:4; stroke-dasharray:30, 15; stroke-dashoffset:4.5; stroke-opacity:1"
        d = 'm 301.22749, 1104.8385 2.12222, 153.7785'
        id = 'path4050' />
    </svg>
)||";


	// create a chunk from the string
	ByteSpan chunk(s);

    printADoc(chunk);
}

static void testAPI()
{
    std::unique_ptr<SVGAPIPrinter> printer = std::make_unique<SVGAPIPrinter>();

    // make a couple of calls and see what gets 
    // printed out
    printer->rotate(radians(45));
    printer->translate(10, 20);
    printer->scale(3.5, 5.7);
}

int main(int argc, char** argv)
{
    //testAPI();
	testChunk();
    //testFile(argc, argv);

    return 0;
}
