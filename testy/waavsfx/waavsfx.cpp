#include <filesystem>

#include "mappedfile.h"
#include "svg.h"
#include "viewport.h"
#include "svgb2ddriver.h"
#include "surface.h"
#include "nametable.h"
#include "svgatoms.h"
#include "pixel_effects.h"
#include "svgfactory.h"

using namespace waavs;

struct WaavsFxOptions
{
    const char* inputFile = nullptr;
    const char* outputFile = "output.png";
    const char* filterFile = nullptr;
    const char* resultName = nullptr;

    SVGLengthValue width{};
    SVGLengthValue height{};
    
    double dpi = 96.0;
    int threadCount = 4;


    bool fitSvgToCanvas = true;
    bool verbose = false;
    bool showHelp = false;

    // AARRGGBB
    uint32_t background = 0x00000000;
};

struct WaavsFxJob
{
    WaavsFxOptions options;

    BLImage sourceBitmap;
    BLImage resultBitmap;

    Surface source;
    Surface result;

    SVGDocumentHandle sourceSvg;

    std::shared_ptr<FilterProgramStream> filterProgram;

    InternedKey sourceGraphicKey = filter::SourceGraphic();
    InternedKey sourceAlphaKey = filter::SourceAlpha();
    InternedKey resultKey = PSNameTable::INTERN("result");
};




////////////////////////////////////////////////////////////////////////////////
//
// Interned command line keys
//
////////////////////////////////////////////////////////////////////////////////

static const InternedKey kArgInput = PSNameTable::INTERN("--input");
static const InternedKey kArgInputShort = PSNameTable::INTERN("-i");

static const InternedKey kArgOutput = PSNameTable::INTERN("--output");
static const InternedKey kArgOutputShort = PSNameTable::INTERN("-o");

static const InternedKey kArgFilter = PSNameTable::INTERN("--filter");
static const InternedKey kArgFilterShort = PSNameTable::INTERN("-f");

static const InternedKey kArgWidth = PSNameTable::INTERN("--width");
static const InternedKey kArgWidthShort = PSNameTable::INTERN("-w");

static const InternedKey kArgHeight = PSNameTable::INTERN("--height");
static const InternedKey kArgHeightShort = PSNameTable::INTERN("-h");

static const InternedKey kArgDpi = PSNameTable::INTERN("--dpi");
static const InternedKey kArgThreads = PSNameTable::INTERN("--threads");
static const InternedKey kArgResult = PSNameTable::INTERN("--result");
static const InternedKey kArgBg = PSNameTable::INTERN("--bg");

static const InternedKey kArgNoFit = PSNameTable::INTERN("--no-fit");
static const InternedKey kArgVerbose = PSNameTable::INTERN("--verbose");

static const InternedKey kArgHelp1 = PSNameTable::INTERN("--help");
static const InternedKey kArgHelp2 = PSNameTable::INTERN("-?");

////////////////////////////////////////////////////////////////////////////////

static INLINE bool requireArgValue(int argc, char** argv, int i) noexcept
{
    if (i + 1 >= argc)
    {
        printf("Missing value for argument: %s\n", argv[i]);
        return false;
    }

    return true;
}

static INLINE bool parseCliInt(const char* text, int& out) noexcept
{
    int64_t v = 0;

    if (!parse64i(text, v))
        return false;

    out = (int)v;
    return true;
}

static INLINE bool parseCliDouble(const char* text, double& out) noexcept
{
    return parseNumber(text, out);
}

static INLINE bool parseCliLength(const char* text, SVGLengthValue& out) noexcept
{
    SVGLengthValue tmp{};

    if (!parseLengthValue(text, tmp))
        return false;

    out = tmp;
    return true;
}

static INLINE bool parseCliHexU32(const char* text, uint32_t& out) noexcept
{
    ByteSpan s = text;

    if (!s)
        return false;

    if (*s == '#')
        ++s;

    if (s.size() >= 2 &&
        s[0] == '0' &&
        (s[1] == 'x' || s[1] == 'X'))
    {
        s += 2;
    }

    uint64_t v = 0;
    if (!parseHex64u(s, v))
        return false;

    out = (uint32_t)v;
    return true;
}

static INLINE int resolveOutputLength(
    const SVGLengthValue& value,
    double ref,
    double dpi,
    int fallback) noexcept
{
    if (!value.isSet())
        return fallback;

    LengthResolveCtx ctx = makeLengthCtxUser(ref, 0.0, dpi, nullptr);
    double px = resolveLengthUserUnits(value, ctx);

    if (!(px > 0.0))
        return fallback;

    return (int)std::ceil(px);
}


static void printUsage() noexcept
{
    printf(
        "Usage:\n"
        "  waavsfx input output [options]\n"
        "  waavsfx --input input --output output [options]\n"
        "\n"
        "Options:\n"
        "  -i, --input <file>       Input image or SVG\n"
        "  -o, --output <file>      Output image file\n"
        "  -f, --filter <file>      SVG filter document or fragment\n"
        "  -w, --width <length>     Output width, ex: 1920, 10in, 50%%\n"
        "  -h, --height <length>    Output height, ex: 1080, 6in, 50%%\n"
        "      --dpi <value>        SVG DPI, default 96\n"
        "      --threads <count>    Thread count, default 4\n"
        "      --result <name>      Named filter output to save\n"
        "      --bg <AARRGGBB>      Background color, default transparent\n"
        "      --no-fit             Do not fit SVG input to canvas\n"
        "      --verbose            Print diagnostic information\n"
        "      --help               Show this help\n"
    );
}


static bool parseCommandLine(int argc, char** argv, WaavsFxOptions& opt) noexcept
{
    int positionalCount = 0;

    for (int i = 1; i < argc; ++i)
    {
        const char* raw = argv[i];
        InternedKey arg = PSNameTable::INTERN(raw);

        if (arg == kArgHelp1 || arg == kArgHelp2)
        {
            opt.showHelp = true;
            return true;
        }

        if (arg == kArgInput || arg == kArgInputShort)
        {
            if (!requireArgValue(argc, argv, i))
                return false;

            opt.inputFile = argv[++i];
        }
        else if (arg == kArgOutput || arg == kArgOutputShort)
        {
            if (!requireArgValue(argc, argv, i))
                return false;

            opt.outputFile = argv[++i];
        }
        else if (arg == kArgFilter || arg == kArgFilterShort)
        {
            if (!requireArgValue(argc, argv, i))
                return false;

            opt.filterFile = argv[++i];
        }
        else if (arg == kArgWidth || arg == kArgWidthShort)
        {
            if (!requireArgValue(argc, argv, i))
                return false;

            if (!parseCliLength(argv[++i], opt.width))
            {
                printf("Invalid width: %s\n", argv[i]);
                return false;
            }
        }
        else if (arg == kArgHeight || arg == kArgHeightShort)
        {
            if (!requireArgValue(argc, argv, i))
                return false;

            if (!parseCliLength(argv[++i], opt.height))
            {
                printf("Invalid height: %s\n", argv[i]);
                return false;
            }
        }
        else if (arg == kArgDpi)
        {
            if (!requireArgValue(argc, argv, i))
                return false;

            if (!parseCliDouble(argv[++i], opt.dpi))
            {
                printf("Invalid dpi: %s\n", argv[i]);
                return false;
            }
        }
        else if (arg == kArgThreads)
        {
            if (!requireArgValue(argc, argv, i))
                return false;

            if (!parseCliInt(argv[++i], opt.threadCount))
            {
                printf("Invalid thread count: %s\n", argv[i]);
                return false;
            }
        }
        else if (arg == kArgResult)
        {
            if (!requireArgValue(argc, argv, i))
                return false;

            opt.resultName = argv[++i];
        }
        else if (arg == kArgBg)
        {
            if (!requireArgValue(argc, argv, i))
                return false;

            if (!parseCliHexU32(argv[++i], opt.background))
            {
                printf("Invalid background color: %s\n", argv[i]);
                return false;
            }
        }
        else if (arg == kArgNoFit)
        {
            opt.fitSvgToCanvas = false;
        }
        else if (arg == kArgVerbose)
        {
            opt.verbose = true;
        }
        else if (raw[0] == '-')
        {
            printf("Unknown option: %s\n", raw);
            return false;
        }
        else
        {
            if (positionalCount == 0)
                opt.inputFile = raw;
            else if (positionalCount == 1)
                opt.outputFile = raw;
            else
            {
                printf("Unexpected argument: %s\n", raw);
                return false;
            }

            ++positionalCount;
        }
    }

    if (!opt.inputFile && !opt.showHelp)
    {
        printf("Missing input file\n");
        return false;
    }

    if (opt.dpi <= 0.0)
    {
        printf("DPI must be greater than zero\n");
        return false;
    }

    if (opt.threadCount < 1)
    {
        printf("Thread count must be at least one\n");
        return false;
    }

    return true;
}






static INLINE WGSizeI resolveOutputSize(
    const WaavsFxJob& job,
    int fallbackW,
    int fallbackH) noexcept
{
    const int w = resolveOutputLength(
        job.options.width,
        (double)fallbackW,
        job.options.dpi,
        fallbackW);

    const int h = resolveOutputLength(
        job.options.height,
        (double)fallbackH,
        job.options.dpi,
        fallbackH);

    return WGSizeI{ w, h };
}


static bool decodeRasterInput(WaavsFxJob& job, const char* filename)
{
    BLResult r = job.sourceBitmap.read_from_file(filename);
    if (r != BL_SUCCESS)
    {
        printf("Failed to decode image: %s\n", filename);
        return false;
    }

    job.source = surfaceFromBLImage(job.sourceBitmap);

    if (job.source.empty())
    {
        printf("Decoded image has no usable surface: %s\n", filename);
        return false;
    }

    return true;
}

static SVGDocumentHandle createDocumentFromFile(
    const char* filename,
    int canWidth,
    int canHeight,
    double dpi) 
{
    auto mapped = MappedFile::create_shared(filename);

    // if the mapped file does not exist, return
    if (mapped == nullptr)
    {
        printf("File not found: %s\n", filename);
        return nullptr;
    }

    ByteSpan mappedSpan;
    mappedSpan.resetFromSize(mapped->data(), mapped->size());
    auto doc = SVGFactory::createFromChunk(mappedSpan, canWidth, canHeight, dpi);

    return doc;
}


static bool renderSvgInput(WaavsFxJob& job, const char* filename)
{
    auto parseSize = resolveOutputSize(job, 1920, 1080);

    auto doc = createDocumentFromFile(
        filename,
        parseSize.width,
        parseSize.height,
        job.options.dpi);

    if (!doc)
        return false;

    WGRectD sceneFrame = doc->objectBoundingBox();

    auto outputSize = resolveOutputSize(
        job,
        (int)std::ceil(sceneFrame.w),
        (int)std::ceil(sceneFrame.h));

    if (outputSize.width <= 0 || outputSize.height <= 0)
    {
        printf(
            "Invalid SVG output size: %d x %d\n",
            outputSize.width,
            outputSize.height);

        return false;
    }

    Surface img(outputSize.width, outputSize.height);

    SVGB2DDriver ctx;
    ctx.attach(img, job.options.threadCount);
    ctx.background(BLRgba32(job.options.background));
    ctx.renew();

    if (job.options.fitSvgToCanvas)
    {
        WGRectD surfaceFrame{
            0,
            0,
            (double)outputSize.width,
            (double)outputSize.height
        };

        WGMatrix3x3 tform{};
        PreserveAspectRatio par{};

        computeViewBoxToViewport(
            surfaceFrame,
            sceneFrame,
            par,
            tform);

        ctx.transform(tform);
    }

    doc->draw(&ctx, doc.get());
    ctx.detach();

    job.source = img;
    job.sourceSvg = doc;

    return true;
}

// ------------------------------
// looksLikeSvg
// 
// A simple heuristic to determine whether the input file 
// is an SVG based on the extension.
//
static INLINE bool looksLikeSvg(const char* filename) noexcept
{
    if (!filename)
        return false;

    return chunk_ends_with_cstr(filename, ".svg");
}

// ------------------------------
// loadInputSurface
static bool loadInputSurface(WaavsFxJob& job)
{
    const char* filename = job.options.inputFile;

    if (looksLikeSvg(filename))
        return renderSvgInput(job, filename);

    return decodeRasterInput(job, filename);
}

// ------------------------------
// loadFilterProgram
//
static bool loadFilterProgram(WaavsFxJob& job)
{
    if (!job.options.filterFile)
        return true;

    auto mapped = MappedFile::create_shared(job.options.filterFile);
    if (!mapped)
    {
        printf("Filter file not found: %s\n", job.options.filterFile);
        return false;
    }

    ByteSpan filterSpan;
    filterSpan.resetFromSize(mapped->data(), mapped->size());

    job.filterProgram = createPixelEffects(filterSpan);
    if (!job.filterProgram)
    {
        printf("Could not create filter program: %s\n", job.options.filterFile);
        return false;
    }

    return true;
}


// ------------------------------
// runFilterProgram
//
static bool runFilterProgram(WaavsFxJob& job)
{
    if (!job.filterProgram)
    {
        job.result = job.source;
        return true;
    }

    job.result = Surface(job.source.width(), job.source.height());

    WGResult r = applyEffectsProgram(
        *job.filterProgram,
        job.source,
        job.result);

    if (r != WG_SUCCESS)
    {
        printf("Filter execution failed\n");
        return false;
    }

    return true;
}

// ------------------------------
// writeOutputSurface
//
static bool writeOutputSurface(const Surface& surface, const char* filename)
{
    if (!filename)
        return false;

    BLImage img = blImageFromSurface(surface);

    BLResult r = img.write_to_file(filename);
    if (r != BL_SUCCESS)
    {
        printf("Failed to write output image: %s\n", filename);
        return false;
    }

    return true;
}

// ------------------------------
// runWaavsFx
//
static int runWaavsFx(const WaavsFxOptions& opt)
{
    WaavsFxJob job{};
    job.options = opt;

    // Compile filter graph once
    if (!loadFilterProgram(job))
        return 1;

    // Load input image/svg
    if (!loadInputSurface(job))
        return 1;

    // Execute compiled graph
    if (!runFilterProgram(job))
        return 1;

    // Save result
    if (!writeOutputSurface(job.result, job.options.outputFile))
        return 1;

    return 0;
}

// ------------------------------

int main(int argc, char** argv)
{
    auto factory = SVGFactory::getSingleton();

    WaavsFxOptions opt{};

    if (!parseCommandLine(argc, argv, opt))
    {
        printUsage();
        return 1;
    }

    if (opt.showHelp)
    {
        printUsage();
        return 0;
    }

    return runWaavsFx(opt);
}
