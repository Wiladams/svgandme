

#include <iostream>

#include "mappedfile.h"
#include "xmlutil.h"

#include "svgdomdocument.h"
#include "svgdatatypes.h"

using namespace waavs;

static void printRect(const BLRect& aRect)
{
	printf("x=%f, y=%f, w=%f, h=%f\n", aRect.x, aRect.y, aRect.w, aRect.h);
}

using cHash = std::hash<ByteSpan>;

static void testSomething()
{
	ByteSpan a("blue");
    ByteSpan b("blueviolet");

    // Get std::hash value from each of them and
    // print them out.
	std::cout << "hash(a) = " << cHash{}(a) << std::endl;
	std::cout << "hash(b) = " << cHash{}(b) << std::endl;
    


    

}

int main(int argc, char** argv)
{
    testSomething();
    
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
