

#include "app/mappedfile.h"
#include "app/xmlutil.h"

#include "svg/xmlscan.h"

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
    auto mapped = MappedFile::create_shared(filename);

    if (nullptr == mapped)
        return 0;


    waavs::ByteSpan s(mapped->data(), mapped->size());
    
    XmlElementIterator iter(s);

    
    // iterate over the elements
    // printing each one
    // There is no regard to hierarchy here, just raw
    // element output
    // Printing an element will print its attributes though
	// and printing a pure content node will print its content
	// and printing a comment will print the comment
    while (iter.next())
    {   
        waavs::printXmlElement(*iter);
    }

    // close the mapped file
    mapped->close();


    return 0;
}
