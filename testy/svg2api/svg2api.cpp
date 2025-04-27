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

    auto doc = SVGFactory::createDOM(s, nullptr);

    std::unique_ptr<SVGAPIPrinter> printer = std::make_unique<SVGAPIPrinter>();

    doc->draw(printer.get(), doc.get());

    // close the mapped file
    mapped->close();
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

    testFile(argc, argv);

    return 0;
}
