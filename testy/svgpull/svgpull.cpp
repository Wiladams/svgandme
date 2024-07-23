

#include <iostream>

#include "mappedfile.h"
#include "xmlutil.h"

#include "svgdomdocument.h"
#include "svgdatatypes.h"
#include "svgpath.h"

using namespace waavs;

static void printRect(const BLRect& aRect)
{
	printf("x=%f, y=%f, w=%f, h=%f\n", aRect.x, aRect.y, aRect.w, aRect.h);
}

using cHash = std::hash<ByteSpan>;

static void testSomething()
{
    // test the hash values of the SegmentCommand enum
    // using the hashPathCommand function
    std::cout << "hashPathCommand(MoveTo) = " << svgPathCommandHash(SegmentCommand::MoveTo) << std::endl;
    std::cout << "hashPathCommand(MoveBy) = " << svgPathCommandHash(SegmentCommand::MoveBy) << std::endl;

	std::cout << "hashPathCommand(LineTo) = " << svgPathCommandHash(SegmentCommand::LineTo) << std::endl;
    std::cout << "hashPathCommand(LineBy) = " << svgPathCommandHash(SegmentCommand::LineBy) << std::endl;
    
	std::cout << "hashPathCommand(HLineTo) = " << svgPathCommandHash(SegmentCommand::HLineTo) << std::endl;
	std::cout << "hashPathCommand(HLineBy) = " << svgPathCommandHash(SegmentCommand::HLineBy) << std::endl;

	std::cout << "hashPathCommand(VLineTo) = " << svgPathCommandHash(SegmentCommand::VLineTo) << std::endl;
	std::cout << "hashPathCommand(VLineBy) = " << svgPathCommandHash(SegmentCommand::VLineBy) << std::endl;
    
    std::cout << "hashPathCommand(CubicTo) = " << svgPathCommandHash(SegmentCommand::CubicTo) << std::endl;
	std::cout << "hashPathCommand(CubicBy) = " << svgPathCommandHash(SegmentCommand::CubicBy) << std::endl;
    
    std::cout << "hashPathCommand(SCubicTo) = " << svgPathCommandHash(SegmentCommand::SCubicTo) << std::endl;
	std::cout << "hashPathCommand(SCubicBy) = " << svgPathCommandHash(SegmentCommand::SCubicBy) << std::endl;
    
	std::cout << "hashPathCommand(QuadTo) = " << svgPathCommandHash(SegmentCommand::QuadTo) << std::endl;
	std::cout << "hashPathCommand(QuadBy) = " << svgPathCommandHash(SegmentCommand::QuadBy) << std::endl;

	std::cout << "hashPathCommand(SQuadTo) = " << svgPathCommandHash(SegmentCommand::SQuadTo) << std::endl;
	std::cout << "hashPathCommand(SQuadBy) = " << svgPathCommandHash(SegmentCommand::SQuadBy) << std::endl;
    
    std::cout << "hashPathCommand(ArcTo) = " << svgPathCommandHash(SegmentCommand::ArcTo) << std::endl;
	std::cout << "hashPathCommand(ArcBy) = " << svgPathCommandHash(SegmentCommand::ArcBy) << std::endl;
    
    std::cout << "hashPathCommand(CloseTo) = " << svgPathCommandHash(SegmentCommand::CloseTo) << std::endl;
	std::cout << "hashPathCommand(CloseBy) = " << svgPathCommandHash(SegmentCommand::CloseBy) << std::endl;


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
