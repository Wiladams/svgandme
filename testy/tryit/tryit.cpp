//
// This is a junk file used to try out various things.
// Typically just things that don't require any UI
// tear down and rebuild as needed.
// 
#include "blend2d.h"
#include "blend2d/imagedecoder.h"

#include "app/mappedfile.h"
#include "bspan.h"

#pragma comment(lib, "blend2d.lib")

using namespace waavs;

#if defined(_M_ARM64)
# pragma message("_M_ARM64 defined")
#endif

#if defined(__aarch64__)
# pragma message("__aarch64__ defined")
#endif

#if defined(_M_X64)
# pragma message("_M_X64 defined")
#endif

#if defined(__x86_64__)
# pragma message("__x86_64__ defined")
#endif

#if defined(__SSE2__)
# pragma message("__SSE2__ defined")
#endif

#if defined(__SSSE3__)
# pragma message("__SSSE3__ defined")
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
# pragma message("__ARM_NEON defined")
#endif

#if WAAVS_HAS_NEON
#pragma message("WAAVS_HAS_NEON")
#endif

int main(int argc, char** argv)
{
    printf("MAIN\n");

    // Get a chunk of memory that has the file in it
    //     
    const char* filename = "X:\\svgarchive\\lottie\\bouncing_beach_ball.png";
    auto mapped = MappedFile::create_shared(filename);

    if (nullptr == mapped)
        return 0;


    BLImageCodec codec;
    BLResult res = codec.findByData(mapped->data(), mapped->size());
    printf("Codec: %d\n", res);

    if (res != BL_SUCCESS)
        return 0;

    // Get a decoder from the codec.
    BLImageDecoder decoder;
    res = codec.createDecoder(&decoder);
    printf("Decoder: %d\n", res);

    if (res != BL_SUCCESS)
        return 0;




    // Try to get the image info so we can see how many frames
    // and what the dimensions are.
    BLImageInfo info{};
    info.reset();
    res = decoder.readInfo(info, (const uint8_t*)mapped->data(), mapped->size());
    if (res != BL_SUCCESS)
        return 0;


    // Get images until done.  Apparently, it will loop around to
    // the beginning, so if you want single shot, pay attention
    // to 'frameIndex()'

    BLImage img{};
    img.create(info.size.w, info.size.h, BL_FORMAT_PRGB32);

    do
    {
        printf("frame: %zd\n", decoder.frameIndex());
        res = decoder.readFrame(img, (const uint8_t*)mapped->data(), mapped->size());

    } while (res == BL_SUCCESS);


    return 0;
}
