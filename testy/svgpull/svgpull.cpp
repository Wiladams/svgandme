

#include "mappedfile.h"
#include "xmlutil.h"

#include "svgdomdocument.h"
#include "svgdatatypes.h"

using namespace waavs;

static void testParseNumber()
{
	const char* str = "-2.34e3M10,20";
    ByteSpan s(str);
    double num{ 0 };
    
    while (parseNextNumber(s, num))
    {
        printf("NUM: %f\n", num);
    }

}

int main(int argc, char** argv)
{
    testParseNumber();
    
    if (argc < 2)
    {
        printf("Usage: xmlpull <xml file>  [output file]\n");
        return 1;
    }

    // create an mmap for the specified file
    const char* filename = argv[1];
    auto mapped = MappedFile::create_shared(filename);

    if (nullptr == mapped)
        return 0;


    waavs::ByteSpan s(mapped->data(), mapped->size());

	auto doc = SVGDOMDocument::createFromChunk(s);
    

    // close the mapped file
    mapped->close();


    return 0;
}
