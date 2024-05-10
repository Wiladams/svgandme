#include "svg/xmlscan.h"

#include "filestreamer.h"
#include "xmlutil.h"

using namespace waavs;

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("Usage: xmlpull <xml file>  [output file]\n");
        return 1;
    }

    // create an mmap for the specified file
    const char* filename = argv[1];
    auto mapped = FileStreamer::createFromFilename(filename);

    if (mapped == nullptr)
        return 0;


    // 
    // Parse the mapped file as XML
    // printing out the con
    waavs::ByteSpan s = mapped->span();
    
    XmlElementIterator iter(s);

    
    // iterate over the elements
    // printing each one
    // There is no regard to hierarchy here, just raw
    // element output
    // Printing an element will print its attributes though
	// nad printing a pure content node will print its content
	// and printing a comment will print the comment
    while (iter.next())
    {   
        waavs::printXmlElement(*iter);
    }

    // close the mapped file
    mapped->close();


    return 0;
}
