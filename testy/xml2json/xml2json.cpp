

#include "appmodule.h"
#include "mappedfile.h"
#include "xmltojson.h"


using namespace waavs;


static void testXmlToJson(const ByteSpan& src)
{
    // convert the XML to JSON
    printXmlToJson(src, stdout, true);
}


static void printModuleName()
{
    ByteSpan modname = getModuleFileName();
    printf("Module Name: ");
    printChunk(modname);
    printf("\n");
}

static void printCommandLine()
{
    ByteSpan cmdLine = getModuleCommandLine();
    printf("Command Line: ");
    printChunk(cmdLine);
    printf("\n");
}

int main(int argc, char** argv)
{
    printModuleName();
    printCommandLine();

    if (argc < 2)
    {
        printf("Usage: xml2json <xml file>  [output file]\n");
        return 1;
    }

    // create an mmap for the specified file
    const char* filename = argv[1];
    auto mapped = MappedFile::create_shared(filename);

    if (nullptr == mapped)
        return 0;

    waavs::ByteSpan s(mapped->data(), mapped->size());


    testXmlToJson(s);

    // close the mapped file
    mapped->close();


    return 0;
}
