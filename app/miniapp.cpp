
/*
    The primary function of this file is to act as a bridge between
    the Windows environment, and our desired, fairly platform independent
    application environment.

    All you need to setup a Windows application is this file, and the accompanying headers.
    It will operate either in console, or Windows mode.

    This file deals with user input (mouse, keyboard, pointer, joystick, touch)
    initiating a pub/sub system for applications to subscribe to.

    The design is meant to be the smallest tightest bare essentials of Windows code
    necessary to write fairly decent applications.
*/

#include "miniapp.h"

#include <cstdio>
#include <array>
#include <iostream>
#include <memory>
#include <future>

#include <shellapi.h>           // for drag-drop support
#include <ShellScalingApi.h>    // For GetDpiForMonitor

#include "layeredwindow.h"
#include "stopwatch.h"
#include "appwindow.h"




using namespace waavs;

static void processFrameTiming();

// Some function signatures
// WinMSGObserver - Function signature for Win32 message handler
typedef LRESULT(*WinMSGObserver)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


// Application routines
// appmain looks for this routine in the compiled application
static VOIDROUTINE gOnloadHandler = nullptr;
static VOIDROUTINE gOnUnloadHandler = nullptr;

static VOIDROUTINE gOnLoopHandler = nullptr;

// Painting
static WinMSGObserver gPaintHandler = nullptr;



// Miscellaneous globals
int gargc;
char** gargv;

unsigned int gSystemThreadCount = 0;  // how many compute threads the system reports

static StopWatch gAppClock;



bool gIsLayered = false;

// Some globals friendly to the p5 environment
// Display Globals
int canvasWidth = 0;
int canvasHeight = 0;
uint8_t* canvasPixelData = nullptr;
size_t canvasStride = 0;


int rawPixelHeight = 0;
int rawPixelWidth = 0;
unsigned int physicalDpi = 192;   // starting pixel density

// Stuff related to rate of displaying frames
float fFrameRate = 1;
double fInterval = 1000;
double fNextMillis = 0;
size_t fDroppedFrames = 0;
uint64_t fFrameCount = 0;         // how many frames drawn so far


// Keyboard globals
// BUGBUG - these should go into the apphost.h
uint8_t keyStates[256]{};
int keyCode = 0;
int keyChar = 0;




// Raw Mouse input
float rawMouseX = 0;
float rawMouseY = 0;

// Joystick
static Joystick gJoystick1(JOYSTICKID1);
static Joystick gJoystick2(JOYSTICKID2);


User32PixelMap* appFrameBuffer()
{
    //static User32PixelMap gAppFrameBuffer{};
    static std::unique_ptr<User32PixelMap> gAppFrameBuffer = std::make_unique<User32PixelMap>();

    return gAppFrameBuffer.get();
}


//
//    https://docs.microsoft.com/en-us/windows/desktop/inputdev/using-raw-input
//
//Raw input utility functions
static const USHORT HID_MOUSE = 2;
static const USHORT HID_JOYSTICK = 4;
static const USHORT HID_GAMEPAD = 5;
static const USHORT HID_KEYBOARD = 6;

// Register for mouseand keyboard
static void HID_RegisterDevice(HWND hTarget, USHORT usage, USHORT usagePage = 1)
{
    RAWINPUTDEVICE hid[1]{};

    hid[0].usUsagePage = usagePage;
    hid[0].usUsage = usage;
    hid[0].dwFlags = (RIDEV_DEVNOTIFY | RIDEV_INPUTSINK);
    hid[0].hwndTarget = hTarget;
    UINT uiNumDevices = 1;

    ::RegisterRawInputDevices(hid, uiNumDevices, sizeof(RAWINPUTDEVICE));
    //printf("HID_RegisterDevice: HWND: 0x%p,  %d  %d\n", hTarget, bResult, ::GetLastError());
}

static void HID_UnregisterDevice(USHORT usage)
{
    RAWINPUTDEVICE hid{ 0 };
    hid.usUsagePage = 1;
    hid.usUsage = usage;
    hid.dwFlags = RIDEV_REMOVE;
    hid.hwndTarget = nullptr;
    UINT uiNumDevices = 1;

    ::RegisterRawInputDevices(&hid, uiNumDevices, sizeof(RAWINPUTDEVICE));
}


// Controlling drawing
void frameRate(float newRate) noexcept
{
    fFrameRate = newRate;
    fInterval = 1000.0 / newRate;
    fNextMillis = gAppClock.millis() + fInterval;
}

float getFrameRate() noexcept
{
    return fFrameRate;
}

uint64_t frameCount() noexcept
{
    return fFrameCount;
}




// This function is called at any time to display whatever is in the app 
// window to the actual screen
void refreshScreenNow()
{

    if (!gIsLayered) {
        // if we're not layered, then do a regular
        // sort of WM_PAINT based drawing
        //InvalidateRect(getAppWindow()->windowHandle(), NULL, 1);
        //::RedrawWindow(getAppWindow()->windowHandle(), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
        ::RedrawWindow(getAppWindow()->windowHandle(), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    else {
        // This is the workhorse of displaying directly
        // to the screen.  Everything to be displayed
        // must be in the FrameBuffer, even window chrome
        LayeredWindowInfo lw(canvasWidth, canvasHeight);
        lw.display(getAppWindow()->windowHandle(), appFrameBuffer()->bitmapDC());
    }
}


//
//    Environment
//
void show()
{
    getAppWindow()->show();
}

void hide()
{
    getAppWindow()->hide();
}





/*
    Turn Windows keyboard messages into keyevents that can
    more easily be handled at the application level
*/
void refreshKeyStates()
{
    // Get the state of all keys on the keyboard
    ::GetKeyboardState(keyStates);
}












/*
static LRESULT HandlePaintMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //printf("HandlePaintMessage\n");

    LRESULT res = 0;
    PAINTSTRUCT ps;

    HDC hdc = BeginPaint(hWnd, &ps);

    int xDest = 0;
    int yDest = 0;
    int DestWidth = canvasWidth;
    int DestHeight = canvasHeight;
    int xSrc = 0;
    int ySrc = 0;
    int SrcWidth = canvasWidth;
    int SrcHeight = canvasHeight;

    BITMAPINFO info = appFrameBuffer()->bitmapInfo();

    // Make sure we sync all current drawing
    // BUGBUG - we don't have a way to guarantee this
    // so, we'll leave it up to the app

    int pResult = StretchDIBits(hdc,
        xDest, yDest,
        DestWidth, DestHeight,
        xSrc, ySrc,
        SrcWidth, SrcHeight,
        appFrameBuffer()->data(), &info,
        DIB_RGB_COLORS,
        SRCCOPY);

    EndPaint(hWnd, &ps);

    return res;
}
*/





/*
// Handle messages related to resizing the window
//
static LRESULT HandleSizeMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res = 0;

    // Only resize bigger, never get smaller
    WORD aWidth = waavs::max(canvasWidth, LOWORD(lParam));
    WORD aHeight = waavs::max(canvasHeight, HIWORD(lParam));

    setCanvasSize(aWidth, aHeight);

    // onCanvasResize();
    ResizeEvent re{ aWidth, aHeight };
    gResizeEventTopic.notify(re);

    refreshScreenNow();

    return res;
}
*/



// Controlling the runtime
// Halt the runtime
void halt() {
    ::PostQuitMessage(0);
}


// Turn raw input on
void rawInput()
{
    HWND localWindow = getAppWindow()->windowHandle();

    HID_RegisterDevice(getAppWindow()->windowHandle(), HID_MOUSE);
    HID_RegisterDevice(getAppWindow()->windowHandle(), HID_KEYBOARD);
}

// turn raw input off
void noRawInput()
{
    // unregister devices
    HID_UnregisterDevice(HID_MOUSE);
    HID_UnregisterDevice(HID_KEYBOARD);
}

// Turn old school joystick support on
void joystick()
{
    gJoystick1.attachToWindow(getAppWindow()->windowHandle());
    gJoystick2.attachToWindow(getAppWindow()->windowHandle());
}

// Turn old school joystick support off
void noJoystick()
{
    gJoystick1.detachFromWindow();
    gJoystick2.detachFromWindow();
}

// Turning Touch input on
bool touch()
{
    BOOL bResult = ::RegisterTouchWindow(getAppWindow()->windowHandle(), 0);
    return (bResult != 0);
}

// Turn touch input off
bool noTouch()
{
    BOOL bResult = ::UnregisterTouchWindow(getAppWindow()->windowHandle());
    return (bResult != 0);
}

// Report whether the application windows is a touch window
bool isTouch()
{
    ULONG flags = 0;
    BOOL bResult = ::IsTouchWindow(getAppWindow()->windowHandle(), &flags);
    return (bResult != 0);
}

// Turn on drop file support
bool dropFiles()
{
    ::DragAcceptFiles(getAppWindow()->windowHandle(), TRUE);
    return true;
}

// Turn off drop file support
bool noDropFiles()
{
    ::DragAcceptFiles(getAppWindow()->windowHandle(), FALSE);
    return true;
}

//
// Window management
//
void layered()
{
    if (nullptr == getAppWindow())
        return;

    getAppWindow()->setLayered(true);

    gIsLayered = true;
}

void noLayered()
{
    if (nullptr == getAppWindow())
        return;

    getAppWindow()->setLayered(false);

    gIsLayered = false;
}

bool isLayered()
{
    return gIsLayered;
}


/*
// Set an opacity value between 0.0 and 1.0
// 1.0 == fully opaque (not transparency)
// less than that makes the whole window more transparent
void windowOpacity(float o)
{
    getAppWindow()->setOpacity(o);
}
*/

void setCanvasPosition(int x, int y)
{
    getAppWindow()->moveTo(x, y);
}

bool setCanvasSize(long aWidth, long aHeight)
{
    appFrameBuffer()->init(aWidth, aHeight);

    canvasWidth = aWidth;
    canvasHeight = aHeight;

    canvasPixelData = appFrameBuffer()->data();
    canvasStride = appFrameBuffer()->stride();

    return true;
}

// Put the application canvas into a window
void createAppWindow(long aWidth, long aHeight, const char* title)
{
    setCanvasSize(aWidth, aHeight);
    getAppWindow()->setCanvasSize(aWidth, aHeight);
    getAppWindow()->setTitle(title);

    showAppWindow();
}

// A basic Windows event loop
void showAppWindow()
{
    getAppWindow()->show();
}

static void processFrameTiming()
{
    // Do frame timing event dispatch
    // 
    // 
    // We'll wait here until it's time to 
    // signal the frame
    //if (gAppClock.millis() > fNextMillis)
    //{
    fFrameCount++;

    FrameCountEvent fce{};
    fce.frameCount = fFrameCount;
    fce.seconds = gAppClock.seconds();

    //gFrameCountEventTopic.notify(fce);
}


//
//    Generic Windows message handler
//    This is used as the function to associate with a window class
//    when it is registered.
//
static LRESULT CALLBACK MsgHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res = 0;

    // Get pointer to window class from handle
    User32Window* win = reinterpret_cast<User32Window*> (::GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (msg)
    {
    case WM_CREATE: {
        CREATESTRUCTA* pCreate = reinterpret_cast<CREATESTRUCTA*>(lParam);
        return 0;
    }
                  break;

    case WM_ERASEBKGND: {
        //printf("WM_ERASEBKGND\n");
        // return non-zero indicating we dealt with erasing the background
        res = 1;

        //if (gPaintHandler != nullptr) {
        //    gPaintHandler(hWnd, msg, wParam, lParam);
        //}

        //RECT rect;
        //if (::GetUpdateRect(hWnd, &rect, FALSE))
        //{
        //    ::ValidateRect(hWnd, NULL);
        //}
    }
                      break;

    case WM_PAINT: {
        //printf("WM_PAINT\n");
        if (gPaintHandler != nullptr) {
            gPaintHandler(hWnd, msg, wParam, lParam);
        }

        //RECT rect;
        //if (::GetUpdateRect(hWnd, &rect, FALSE))
        //{
        //    ::ValidateRect(hWnd, NULL);
        //}
    }
                 break;

    case WM_MOVING:
    case WM_WINDOWPOSCHANGING:
        //printf("screen moving\n");
        //screenRefresh();
        processFrameTiming();
        //::InvalidateRect(hWnd, NULL, 1);
        break;

    case WM_SIZE:
        //HandleSizeMessage(hWnd, msg, wParam, lParam);
        res = ::DefWindowProcA(hWnd, msg, wParam, lParam);
        break;

    case WM_DESTROY:
        // By doing a PostQuitMessage(), a 
        // WM_QUIT message will eventually find its way into the
        // message queue.
        ::PostQuitMessage(0);
        return 0;
        break;

    case WM_INPUT:
        //res = HandleRawInputMessage(hWnd, msg, wParam, lParam);
        break;




    default:
    {
            res = ::DefWindowProcA(hWnd, msg, wParam, lParam);
    }
    break;
    }

    //else if ((msg >= WM_NCPOINTERUPDATE) && (msg <= WM_POINTERROUTEDRELEASED)) {
    //    HandlePointerMessage(hWnd, msg, wParam, lParam);
    //}

    return res;
}



//
// Look for the dynamic routines that will be used
// to setup client applications.
// Most notable is 'onLoad()' and 'onUnload'
//
static void registerHandlers()
{
    // we're going to look within our own module
    // to find handler functions.  This is because the user's application should
    // be compiled with the application, so the exported functions should
    // be attainable using 'GetProcAddress()'

    HMODULE hInst = ::GetModuleHandleA(NULL);

    // Start with our default paint message handler
    //gPaintHandler = HandlePaintMessage;


    // One of the primary handlers the user can specify is 'onPaint'.  
    // If implemented, this function will be called whenever a WM_PAINT message
    // is seen by the application.
    //WinMSGObserver handler = (WinMSGObserver)::GetProcAddress(hInst, "onPaint");
    //if (handler != nullptr) {
    //    gPaintHandler = handler;
    //}

    // Get the general app routines
    // onLoad()
    gOnloadHandler = (VOIDROUTINE)::GetProcAddress(hInst, "onLoad");
    gOnUnloadHandler = (VOIDROUTINE)::GetProcAddress(hInst, "onUnload");
    gOnLoopHandler = (VOIDROUTINE)::GetProcAddress(hInst, "onLoop");
}



static void run()
{
    static bool running = true;

    // Make sure we have all the event handlers connected
    registerHandlers();

    // call the application's 'onLoad()' if it exists
    if (gOnloadHandler != nullptr) {
        gOnloadHandler();
    }

    showAppWindow();

    gAppClock.reset();

    /*
    // Do a typical Windows message pump
    MSG msg{};

    while (running) {
        DWORD waitResult = MsgWaitForMultipleObjectsEx(
            0,              // no kernel objects to wait on
            nullptr,        // null because no kernel objects to wait on
            fInterval,      // 1000 / fps
            QS_ALLEVENTS,
            MWMO_INPUTAVAILABLE);

        switch (waitResult)
        {
        case WAIT_OBJECT_0:
            // Process all the messages that might be sitting
            // in the message queue
            while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    running = false;
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
            break;

        case WAIT_TIMEOUT:
            // Handle per frame logic
            processFrameTiming();
            break;

        case WAIT_FAILED:
            // If the wait failed for some reason other than
            // timeout,break out and stop
            running = false;
            break;

        default:
            // If there are any objects in the wait (like events)
            // handle them here
            break;
        }


        // Give the user application some control to do what
        // it wants
        // call onLoop() if it exists
        if (gOnLoopHandler != nullptr) {
            gOnLoopHandler();
        }
    }
    */
    
    getAppWindow()->waitToFinish();
}




// We want to capture the true physical screen pixel density
// and not just the adjusted one.  Also, we want the entire process
// to use high dpi, and not just logical dpi
// ::SetThreadDpiAwarenessContext(dpiContext);   // only for a single thread
// else
// ::SetProcessDPIAware();

static void setDPIAware()
{
    auto dpiContext = DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2;

    // If Windows 10.0 or higher
    ::SetProcessDpiAwarenessContext(dpiContext);

    rawPixelWidth = ::GetSystemMetrics(SM_CXSCREEN);
    rawPixelHeight = ::GetSystemMetrics(SM_CYSCREEN);


    // Create a DC to query EDID-basd physical size
    // This won't be accurate if using a virtual terminal, or if the 
    // monitor driver does not report it accurately
    auto dhdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

    // How big is the screen physically
    // DeviceCaps gives it in millimeters, so we convert to inches
    auto screenWidthInches = ::GetDeviceCaps(dhdc, HORZSIZE) / 25.4;
    auto screenHeightInches = ::GetDeviceCaps(dhdc, VERTSIZE) / 25.4;


    // Calculate physical DPI (using vertial dimensions)
    double screenPpi = (double)rawPixelHeight / screenHeightInches;
    physicalDpi = (unsigned int)std::round(screenPpi);

    ::DeleteDC(dhdc);
}


// Initialize Windows networking
static void setupNetworking()
{

    // BUGBUG - decide whether or not we care about WSAStartup failing
    uint16_t version = MAKEWORD(2, 2);
    WSADATA lpWSAData;
    int res = ::WSAStartup(version, &lpWSAData);
    if (res != 0)
        printf("Error setting up networking: 0x%x\n", res);
}

//! \brief Retrieves the main (singleton) application window.
//
//! This function returns a pointer to the main application window. On the first
//! call, it creates a new window using the "appwindow" window class, which is
//! registered with the given class styles and message handler. The newly
//! created window measures 320 x 240. On subsequent calls, it returns the
//! already-created window pointer, ensuring only one window instance is
//! maintained for the entire application.
//
//! \note The returned window is created once and persists for the lifetime of
//!       the application unless explicitly destroyed. If it is destroyed,
//!       callers must handle re-creation or avoid further calls to this
//!       function.
//
//! \return A pointer to the main application window (\c waavs::User32Window).
//! 
waavs::ApplicationWindow* getAppWindow()
{
     static std::unique_ptr<waavs::ApplicationWindow> gAppWindow = nullptr;

    if (!gAppWindow)
    {
        int style = WS_OVERLAPPEDWINDOW;
        int xstyle = 0;
        WNDPROC handler = nullptr;

         auto win = new ApplicationWindow(320, 240);
        gAppWindow = std::unique_ptr<waavs::ApplicationWindow>(win);

    }

    return gAppWindow.get();
}

static bool prolog()
{
    // initialize blend2d library
    blRuntimeInit();

    // get count of system threads to use later
    gSystemThreadCount = std::thread::hardware_concurrency();

    setupNetworking();

    setDPIAware();
    //setupDpi();

    // get the application window running
    auto win = getAppWindow();
    if (win != nullptr)
        win->run();
    
    // set the canvas a default size to start
    // but don't show it
    setCanvasSize(320, 240);
    //gAppWindow = gAppWindowKind.createWindow("Application Window", 320, 240);

    return true;
}

// Do whatever cleanup needs to be done before
// exiting application
static void epilog()
{
    if (gOnUnloadHandler != nullptr) {
        gOnUnloadHandler();
    }

    // shutdown the blend2d library
    blRuntimeShutdown();

    // shut down networking stack
    ::WSACleanup();
}



//    Why on Earth are there two 'main' routines in here?
//    the first one 'main()' will be called when the code is compiled
//    as a CONSOLE subsystem.

//    the second one 'WinMain' will be called when the code is compiled
//    as a WINDOWS subsystem.  

//    By having both, we can control the startup in both cases.
//
static int ndtRun()
{
    if (!prolog()) {
        printf("error in prolog\n");
        return -1;
    }

    run();

    // do any cleanup after all is done
    epilog();

    return 0;
}



//    The 'main()' function is in here to ensure that compiling
//    this will result in an executable file.
//
//    The code for the user just needs to implement at least the 'onLoad()' function
//

// If we're compiled as a CONSOLE subsystem, then 'main()' will be called
int main(int argc, char** argv)
{
    gargc = argc;
    gargv = argv;

    return ndtRun();
}

// If we're compiled as a WINDOWS system app, then this will be called
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdLine, int nShowCmd)
{
    // BUGBUG, need to parse those command line arguments
    //gargc = argc;
    //gargv = argv;

    return ndtRun();
}