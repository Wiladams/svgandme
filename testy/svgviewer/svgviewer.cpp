#pragma comment(lib, "blend2d.lib")

#include "app/apphost.h"

#include <filesystem>


#include "mappedfile.h"
#include "svguiapp.h"
#include "svgwaavs.h"
#include "svgicons.h"
#include "viewnavigator.h"
#include "svgb2ddriver.h"


using namespace waavs;

// Assuming Mac Studio display,  which is 5120 x 2880
//static constexpr int canvasWidth = 1024, canvasHeight = 768;
// HD size?
//static constexpr int canvasWidth = 1920, canvasHeight = 1024;
// One quarter of a apple studio display is 2560 x 1440
static constexpr int canvasWidth = 2560, canvasHeight = 1440;
// 3380x1900 - two thirds
//static constexpr int canvasWidth = 2560, canvasHeight = 2300;
static constexpr float kFrameRate = 30.0f;

static std::unique_ptr<SVGB2DDriver> gDrawingContext{ nullptr };

// Reference to currently active document
std::shared_ptr<SVGDocument> gDoc{ nullptr };
ViewNavigator gNavigator{};

// Animation management
bool gAnimate{ false };
bool gPerformTransform{ true };
bool gAutoGrow{ true };

double gRecordingStart{ 0 };

// retrieve a pointer to a unique drawing context
static IRenderSVG *getDrawingContext()
{
    if (!gDrawingContext)
    {
        gDrawingContext = std::make_unique<SVGB2DDriver>();
        Surface* s = getAppSurface();
        if (s)
            getDrawingContext()->attach(*s, 4);
    }

    return gDrawingContext.get();
}

// docFromFilename
//
// Given a filename, parse the svg in the file, and return 
// a shared pointer to it.
//
static std::shared_ptr<SVGDocument> docFromFilename(const char* filename)
{
    auto mapped = MappedFile::create_shared(filename);
    
    // if the mapped file does not exist, return
    if (mapped == nullptr)
    {
        printf("File not found: %s\n", filename);
        return nullptr;
    }

    ByteSpan aspan;
    aspan.resetFromSize(mapped->data(), mapped->size());
    std::shared_ptr<SVGDocument> aDoc = SVGFactory::createFromChunk(aspan, appFrameWidth, appFrameHeight, physicalDpi);
    
    // release the mapped file, since we no longer need it.
    mapped->close();

    // If we successfully created a document, 
    // release the old drawing context, so that 
    // it will be re-created anew.
    gDrawingContext.release();
    gDrawingContext = nullptr;

    return aDoc;
}



static void drawBackground()
{
    // We need to clear the context first
    getAppSurface()->clearAll();
    getDrawingContext()->renew();
    getDrawingContext()->background(BLRgba32(0xffffffff));
    getDrawingContext()->clearToBackground();
}

static void drawForeground()
{
    // Tracking the conversion of a rectangle through the current transform, 
    // to see how it maps to the surface
    // Works with a .svg where there is a rectangle of the same size being
    // drawn in red.  If we see any of that red, instead of green, then there
    // is something wrong with the transform.
    //WGRectD inFrame{ 100, 100, 200, 200 };
    //WGMatrix3x3 m = getDrawingContext()->getTransform();
    //WGRectD greenFrame = mapRectAABB(m, inFrame);

    //getAppSurface()->fillRect(greenFrame, 0xff00ff00);
}

static void drawDocument()
{
    // First apply the transform as tracked by the navigator
    if (gPerformTransform)
    {
        const WGMatrix3x3 & m = gNavigator.sceneToSurfaceTransform();
        getDrawingContext()->transform(m);
    }


    // draw the document into the ctx
    if (gDoc != nullptr)
    {
        gDoc->draw(getDrawingContext(), gDoc.get());
    }

    getDrawingContext()->flush();
}



static void draw()
{	
    //double startTime = seconds();

    drawBackground();
    drawDocument();
    drawForeground();
}

static void resetView(const WGRectD &bd, const WGRectD &fr)
{
    gNavigator.resetNavigator();
    gNavigator.setFrame(fr);
    gNavigator.setBounds(bd);
}

static void handleViewChange(const bool& changeOccured)
{
    if (gDoc == nullptr)
        return;

    //printf("svgviewer::handleChange\n");
    draw();

    // we do this screenRefresh here because mouse dragging
    // runs the window in a modal way, starving us of regular
    // redraw messages, based on timing, so we force a redraw
    // message through the message queue
    refreshScreenNow();
}

static void handleChange(const bool& changeOccured)
{
    if (gDoc == nullptr)
        return;

    if (gAnimate)
    {
        gDoc->update(gDoc.get());
    }
    
    //printf("svgviewer::handleChange\n");
    draw();
    
    // we do this screenRefresh here because mouse dragging
    // runs the window in a modal way, starving us of regular
    // redraw messages, based on timing, so we force a redraw
    // message through the message queue
    refreshScreenNow();

}


static void onFileDrop(const FileDropEvent& fde)
{
    // assuming there's at least one file that 
    // has been dropped.
    for (int i = 0; i < fde.filenames.size(); i++)
    {
        // Create a new SVGDocument for each file
        // And create a window to display each document
        double startTime = seconds();
        
        // BUGBUG - may need to explicityly unload previous document
        gDoc = docFromFilename(fde.filenames[i].c_str());


        double endTime = seconds();
        //printf("== fileDrop: SVGDocument::createFromFilename took %f seconds\n", endTime - startTime);

        if (gDoc != nullptr)
        {
            WGRectD viewFr = { 0,0,canvasWidth, canvasHeight };
            WGRectD docFr = viewFr;

            // If there is a documentElement, we can use its viewport
            // for the object frame
            docFr = gDoc->viewPort();

            // Set the initial viewport
            resetView(docFr, viewFr);
            
            handleChange(true);

            break;
        }

    }
}

// Create a routine to respond to frameevents
// which are sent at the frame rate specified
static void onFrameEvent(const FrameCountEvent& fe)
{
    //printf("frameEvent: %d\n", (int)fe.frameCount);
    //printf("Actual Frame Rate: %d\n", (int)(fe.frameCount / seconds()));
    
    //handleChange(true);
    if (gAnimate && gDoc)
    {
        gDoc->update(gDoc.get());
        draw();
        refreshScreenNow();
    }


    getRecorder()->saveFrame();
}



static void onResizeEvent(const ResizeEvent& re)
{
    //gNavigator.setFrame(BLRect(0, 0, appFrameWidth, appFrameHeight));
    
    //printf("onResizeEvent: %d x %d\n", re.width, re.height);

    //ctxInfo.threadCount = 0;
    Surface* s = getAppSurface();
    if (!s)
        return;

    getDrawingContext()->attach(*s, 4);
    handleChange(true);
}

static void onMouseEvent(const MouseEvent& e)
{
    gNavigator.onMouseEvent(e);
}




static void onKeyboardEvent(const KeyboardEvent& ke)
{
    //printf("SVGViewer::onKeyboardEvent: %d\n", ke.key);
    if (ke.activity == KEYRELEASED)
    {
        switch (ke.keyCode)
        {
            case 'G':
                gAutoGrow = !gAutoGrow;
            break;
            
            case VK_PLAY:
            case VK_PAUSE:
            case 'R':
                getRecorder()->toggleRecording();
                if (getRecorder()->isRecording())
                {
                    gRecordingStart = seconds();
                }
                else
                {
                    double duration = seconds() - gRecordingStart;
                    printf("Recording Frames: %d  Duration: %3.2f  FPS: %f\n", getRecorder()->frameCount(), duration, getRecorder()->frameCount() / duration);
                }
            break;			

            case 'A':
                gAnimate = !gAnimate;
            break;

            case 'T':
                gPerformTransform = !gPerformTransform;
                handleChange(true);

            break;
        }
    }
}

static void setupFonts()
{
    //loadDefaultFonts();
    
    loadFontDirectory("c:\\windows\\fonts");
    loadFontDirectory("x:\\Fonts\\commonfonts");
    //loadFontDirectory("x:\\Fonts\\Fonts");



    //loadFontDirectory("..\\resources");

    /*
    // stragglers
    // Load in some fonts to start
    std::vector<const char*> fontNames{
        "x:\\Fonts\\NotoColorEmoji-Regular.ttf",
        "x:\\Fonts\\NotoEmoji-Regular.ttf",
        "x:\\Fonts\\NotoMono-Regular.ttf",
        "x:\\Fonts\\NotoSansBengaliUI-Regular.ttf",
        "x:\\Fonts\\NotoSans-Black.ttf",
        "x:\\Fonts\\NotoSans-Bold.ttf",
        "x:\\Fonts\\NotoSans-Regular.ttf",
        "x:\\Fonts\\NotoSansDevanagari-Regular.ttf",
        //"x:\\Fonts\\Fonts\\Walkway Bold.ttf",
        //"x:\\Fonts\\Fonts\\Walkway.ttf",
    };

    auto fh = FontHandler::getFontHandler();

    if (nullptr != fh)
        fh->loadFonts(fontNames);
    */

}



// called once before main loop is running
void setup()
{
    //printf("setup()\n");

    // register to receive various events
    subscribe(onFileDrop);
    subscribe(onResizeEvent);
    subscribe(onFrameEvent);
    subscribe(onMouseEvent);
    subscribe(onKeyboardEvent);


    setupFonts();
    
    setFrameRate(kFrameRate);
    
    dropFiles();
    
    // set app window size and title
    createAppWindow(canvasWidth, canvasHeight, "SVGViewer");
    
    Surface* s = getAppSurface();
    getRecorder()->reset(s, "frame", 15, 0);
    
    
    // Set the initial viewport
    //gViewPort.surfaceFrame({0, 0, (double)canvasWidth, (double)canvasHeight});
    gNavigator.setFrame({ 0, 0, (double)appFrameWidth, (double) appFrameHeight });
    gNavigator.subscribe(handleViewChange);
    
    // Load extension elements
    //SVGScriptElement::registerFactory();
   // DisplayCaptureElement::registerFactory();
    SVGFramesElement::registerFactory();                // <frames>

    getDrawingContext()->background(BLRgba32(0xffffffff));
    getDrawingContext()->clearToBackground();
}
