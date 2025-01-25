
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

#include "apphost.h"

#include <cstdio>
#include <array>
#include <iostream>
#include <memory>
#include <future>

#include <shellapi.h>           // for drag-drop support
#include <ShellScalingApi.h>    // For GetDpiForMonitor

#include "layeredwindow.h"
#include "stopwatch.h"





using namespace waavs;

static void processFrameTiming();

// Some function signatures
// WinMSGObserver - Function signature for Win32 message handler
typedef LRESULT (*WinMSGObserver)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define LODWORD(ull) ((DWORD)((ULONGLONG)(ull) & 0x00000000ffffffff))
#define HIDWORD(ull) ((DWORD)((((ULONGLONG)(ull))>>32) & 0x00000000ffffffff))

// Application routines
// appmain looks for this routine in the compiled application
static VOIDROUTINE gOnloadHandler = nullptr;
static VOIDROUTINE gOnUnloadHandler = nullptr;

static VOIDROUTINE gOnLoopHandler = nullptr;

// Painting
static WinMSGObserver gPaintHandler = nullptr;

// Topics applications can subscribe to
SignalEventTopic gSignalEventTopic;
KeyboardEventTopic gKeyboardEventTopic;
MouseEventTopic gMouseEventTopic;
JoystickEventTopic gJoystickEventTopic;
FileDropEventTopic gFileDropEventTopic;
TouchEventTopic gTouchEventTopic;
PointerEventTopic gPointerEventTopic;
GestureEventTopic gGestureEventTopic;
FrameCountEventTopic gFrameCountEventTopic;
ResizeEventTopic gResizeEventTopic;

// Miscellaneous globals
int gargc;
char **gargv;

unsigned int gSystemThreadCount=0;  // how many compute threads the system reports

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

// Mouse Globals
bool mouseIsPressed = false;
float mouseX = 0;
float mouseY = 0;
float mouseDelta = 0;
float mouseHDelta = 0;
float pmouseX = 0;
float pmouseY = 0;


// Raw Mouse input
float rawMouseX = 0;
float rawMouseY = 0;

// Joystick
static Joystick gJoystick1(JOYSTICKID1);
static Joystick gJoystick2(JOYSTICKID2);

AFrameBuffer* appFrameBuffer()
{
    static std::unique_ptr<AFrameBuffer> gAppFrameBuffer = std::make_unique<AFrameBuffer>();
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
		//::RedrawWindow(getAppWindow()->windowHandle(), NULL, NULL, RDW_INVALIDATE );
        ::RedrawWindow(getAppWindow()->windowHandle(), NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    else {
        // This is the workhorse of displaying directly
        // to the screen.  Everything to be displayed
        // must be in the FrameBuffer, even window chrome
        LayeredWindowInfo lw(canvasWidth, canvasHeight);
        lw.display(getAppWindow()->windowHandle(), appFrameBuffer()->getGDIContext());
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

void cursor()
{
    ::ShowCursor(1);
}

// Show the cursor, if there is one
// BUGBUG - we should be more robust here and
// check to see if there's even a mouse attached
// Then, decrement count enough times to make showing
// the cursor absolute rather than relative.
void noCursor()
{
    ::ShowCursor(0);
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

static LRESULT HandleKeyboardMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res = 0;

    keyCode = 0;
    keyChar = 0;

    KeyboardEvent e{};
    e.keyCode = (int)wParam;
    e.repeatCount =LOWORD(lParam);  // 0 - 15
    e.scanCode = ((lParam & 0xff0000) >> 16);        // 16 - 23
    e.isExtended = (lParam & 0x1000000) != 0;    // 24
    e.wasDown = (lParam & 0x40000000) != 0;    // 30

    // Get the state of all keys on the keyboard
    ::GetKeyboardState(keyStates);

//printf("HandleKeyboardMessage: %04x\n", msg);

    switch(msg) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            e.activity = KEYPRESSED;
            keyCode = e.keyCode;
            break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            e.activity = KEYRELEASED;
            keyCode = e.keyCode;
            break;

        case WM_CHAR:
        case WM_SYSCHAR:
            e.activity = KEYTYPED;
            keyChar = e.keyCode;
            break;
    }
    
    // publish keyboard event
    gKeyboardEventTopic.notify(e);

    return res;
}


//
//    Turn Windows mouse messages into mouse events which can
//    be dispatched by the application.
//
static LRESULT HandleMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{   
    LRESULT res = 0;
    MouseEvent e{};

    // Coordinates relative to upper left corner of the screen?
    auto x = GET_X_LPARAM(lParam);
    auto y = GET_Y_LPARAM(lParam);

    e.x = (float)x;
    e.y = (float)y;
    e.control = (wParam & MK_CONTROL) != 0;
    e.shift = (wParam& MK_SHIFT) != 0;
    e.lbutton = (wParam & MK_LBUTTON) != 0;
    e.rbutton = (wParam & MK_RBUTTON) != 0;
    e.mbutton = (wParam & MK_MBUTTON) != 0;
    e.xbutton1 = (wParam & MK_XBUTTON1) != 0;
    e.xbutton2 = (wParam & MK_XBUTTON2) != 0;
    bool isPressed = e.lbutton || e.rbutton || e.mbutton || e.xbutton1 || e.xbutton2;

    switch(msg) {
        case WM_LBUTTONDBLCLK:
	    case WM_MBUTTONDBLCLK:
	    case WM_RBUTTONDBLCLK:
            e.activity = MOUSEDOUBLECLICKED;
            break;

        case WM_MOUSEMOVE:
            e.activity = MOUSEMOVED;
            break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
            e.activity = MOUSEPRESSED;

            // Capture the mouse
            SetCapture(hwnd);
            break;
        
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
            e.activity = MOUSERELEASED;

            // Release the mouse
			ReleaseCapture();
            break;
        
            // The mouse wheel messages report their location
            // in screen coordinates, rather than window client
            // area coordinates.  So, we have to translate, because
            // we want everything to be in window client coordinates
        case WM_MOUSEWHEEL: {
            e.activity = MOUSEWHEEL;
            e.delta = (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;

            POINT PT{};
            PT.x = x;
            PT.y = y;
            ::ScreenToClient(hwnd, &PT);

            e.x = (float)PT.x;
            e.y = (float)PT.y;
            }
            break;
        
        case WM_MOUSEHWHEEL: {
            e.activity = MOUSEHWHEEL;
            auto delta = GET_WHEEL_DELTA_WPARAM(wParam);
            e.delta =  (float)delta / (float)WHEEL_DELTA;

            POINT PT{};
            PT.x = x;
            PT.y = y;
            ::ScreenToClient(hwnd, &PT);

            e.x = (float)PT.x;
            e.y = (float)PT.y;
        }
            break;

        default:
            printf("UNKNOWN MOUSE\n");
            return res;
        break;
    }

    mouseX = e.x;
    mouseY = e.y;
    mouseDelta = e.delta;
    mouseIsPressed = isPressed;

    //printf("MOUSE: %d %d\n", e.xbutton1, e.xbutton2);

    gMouseEventTopic.notify(e);

    return res;
}


static LRESULT HandleRawInputMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    printf("HandleRawInputMessage (WM_INPUT)\n");
    
    LRESULT res = 0;
    

    bool inBackground = GET_RAWINPUT_CODE_WPARAM(wParam) == 1;
    HRAWINPUT inputHandle = (HRAWINPUT)lParam;
    UINT uiCommand = RID_INPUT;
    UINT cbSize = 0;

    // First, find out how much space will be needed
    UINT size = ::GetRawInputData((HRAWINPUT)lParam, uiCommand, nullptr, &cbSize, sizeof(RAWINPUTHEADER));


    // Allocate space, and try it again
    std::vector<uint8_t> buff(cbSize, 0);
    size = ::GetRawInputData((HRAWINPUT)lParam, uiCommand, buff.data(), &cbSize, sizeof(RAWINPUTHEADER));
    //printf("WM_INPUT: %d - %d\n", cbSize, size);
    if (size == cbSize) {
        RAWINPUT* raw = (RAWINPUT*)buff.data();

        // See what we got
        //printf("RAWINPUT: %d\n", raw->header.dwType);

        switch (raw->header.dwType) {
        case RIM_TYPEMOUSE: {
            rawMouseX = (float)raw->data.mouse.lLastX;
            rawMouseY = (float)raw->data.mouse.lLastY;
            //mouseEvent();
            //printf("RAWMOUSE: %d %d\n", raw->data.mouse.lLastX, raw->data.mouse.lLastY);
        }
                          break;

        case RIM_TYPEKEYBOARD: {
            //keyboardEvent
        }
                             break;
        }
    }

    return res;
}

// 
// Handling the joystick messages through the Windows
// Messaging method is very limited.  It will only 
// trigger for a limited set of buttons and axes movements
// This handler is here for a complete API.  If the user app
// wants to get more out of the joystick, it can access the
// joystick directly with 'gJoystick1' or 'gJoystick2' and
// call getPosition() at any time.  Typically during 'update()'
// or 'draw()'
static LRESULT HandleJoystickMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res = 0;

    JoystickEvent e;

    // We can handle up to two joysticks using
    // this mechanism
    switch (msg) {
    case MM_JOY1BUTTONDOWN:
        gJoystick1.getPosition(e);
        e.activity = JOYPRESSED;
        break;

    case MM_JOY2BUTTONDOWN:
        gJoystick2.getPosition(e);
        e.activity = JOYPRESSED;
        break;

    case MM_JOY1BUTTONUP:
        gJoystick1.getPosition(e);
        e.activity = JOYRELEASED;
        break;
    case MM_JOY2BUTTONUP:
        gJoystick2.getPosition(e);
        e.activity = JOYRELEASED;
        break;

    case MM_JOY1MOVE:
        gJoystick1.getPosition(e);
        e.activity = JOYMOVED;
        break;
    case MM_JOY2MOVE:
        gJoystick2.getPosition(e);
        e.activity = JOYMOVED;
        break;

    case MM_JOY1ZMOVE:
        gJoystick1.getPosition(e);
        e.activity = JOYZMOVED;
    break;
    case MM_JOY2ZMOVE:
        gJoystick2.getPosition(e);
        e.activity = JOYZMOVED;
        break;
    }

    gJoystickEventTopic.notify(e);

    return res;
}

static LRESULT HandleTouchMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res = 0;

    // cInputs could be set to a maximum value (10) and
    // we could reuse the same allocated array each time
    // rather than allocating a new one each time.
    //std::cout << "wm_touch_event: " << wParam << std::endl;

    int cInputs = LOWORD(wParam);
    int cbSize = sizeof(TOUCHINPUT);

    //printf("HandleTouchMessage 1.0: %d\n", cInputs);
    //PTOUCHINPUT pInputs = new TOUCHINPUT[cInputs];
    TOUCHINPUT pInputs[10];    // = new TOUCHINPUT[cInputs];

    // 0 == failure?
    BOOL bResult = ::GetTouchInputInfo((HTOUCHINPUT)lParam, cInputs, pInputs, cbSize);

    if (bResult == 0) {
        //delete[] pInputs;
        DWORD err = ::GetLastError();
        //printf("getTouchInputInfo, ERROR: %d %D\n", bResult, err);

        return 0;
    }

    // construct an event for each item
    // BUGBUG - really this should be a deque of events
    // like with file drops
    for (int i = 0; i < cInputs; i++) {
        TOUCHINPUT ti = pInputs[i];
        TouchEvent e = {};

        // convert values to local TouchEvent structure
        e.id = ti.dwID;
        e.rawX = (float)ti.x;
        e.rawY = (float)ti.y;

        POINT PT{};
        PT.x = TOUCH_COORD_TO_PIXEL(ti.x);
        PT.y = TOUCH_COORD_TO_PIXEL(ti.y);

        // Convert from screen coordinates to local
        // client coordinates
        ::ScreenToClient(hwnd, &PT);

        // Assign x,y the client area value of the touch point
        e.x = (float)PT.x;
        e.y = (float)PT.y;


        // Now, deal with masks
        //#define TOUCHINPUTMASKF_TIMEFROMSYSTEM  0x0001  // the dwTime field contains a system generated value
        //#define TOUCHINPUTMASKF_EXTRAINFO       0x0002  // the dwExtraInfo field is valid
        //#define TOUCHINPUTMASKF_CONTACTAREA     0x0004  // the cxContact and cyContact fields are valid

        if ((ti.dwMask & TOUCHINPUTMASKF_CONTACTAREA) != 0) {
            e.rawWidth = ti.cxContact;
            e.rawHeight = ti.cyContact;
            e.w = TOUCH_COORD_TO_PIXEL(e.rawWidth);
            e.h = TOUCH_COORD_TO_PIXEL(e.rawHeight);
        }

        // figure out kind of activity and attributes
        /*
        #define TOUCHEVENTF_MOVE            0x0001
#define TOUCHEVENTF_DOWN            0x0002
#define TOUCHEVENTF_UP              0x0004
#define TOUCHEVENTF_INRANGE         0x0008
#define TOUCHEVENTF_PRIMARY         0x0010
#define TOUCHEVENTF_NOCOALESCE      0x0020
#define TOUCHEVENTF_PEN             0x0040
#define TOUCHEVENTF_PALM            0x0080
        */
        if (ti.dwFlags & TOUCHEVENTF_DOWN) {
            e.activity = TOUCH_DOWN;
            e.isDown = true;
        }

        if (pInputs[i].dwFlags & TOUCHEVENTF_UP) {
            e.activity = TOUCH_UP;
            e.x = -1;
            e.y = -1;
            e.isUp = true;
        }

        if (pInputs[i].dwFlags & TOUCHEVENTF_MOVE) {
            e.activity = TOUCH_MOVE;
            e.isMoving = true;
        }

        // Attributes of the event
        if (pInputs[i].dwFlags & TOUCHEVENTF_INRANGE) {
            e.isHovering = true;
        }

        if (ti.dwFlags & TOUCHEVENTF_PRIMARY) {
            e.isPrimary = true;
        }

        if (pInputs[i].dwFlags & TOUCHEVENTF_PALM) {
            e.isPalm = true;
        }
        
        if (pInputs[i].dwFlags & TOUCHEVENTF_PEN) {
            e.isPen = true;
        }

        gTouchEventTopic.notify(e);
    }
    //delete[] pInputs;

    if (!CloseTouchInputHandle((HTOUCHINPUT)lParam))
    {
        // error handling
    }

    return res;
}

static LRESULT HandlePointerMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res = 0;

    PointerEvent e{};

    gPointerEventTopic.notify(e);

    return res;
}

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
    UINT StartScan = 0;
    UINT cLines = canvasHeight;
    
    BITMAPINFO info = appFrameBuffer()->bitmapInfo();
    
    // Make sure we sync all current drawing
    // BUGBUG - we don't have a way to guarantee this
    // so, we'll leave it up to the app

    //int pResult = StretchDIBits(hdc,
    //    xDest,yDest,
    //    DestWidth,DestHeight,
    //    xSrc,ySrc,
    //    SrcWidth, SrcHeight,
    //    appFrameBuffer()->data(), &info,
    //    DIB_RGB_COLORS,
    //    SRCCOPY);
    int pResult = SetDIBitsToDevice(
        hdc,
        xDest,
        yDest,
        DestWidth,
        DestHeight,
        xSrc,
        ySrc,
        StartScan,
        cLines,
        appFrameBuffer()->data(),
        &info,
        DIB_RGB_COLORS
    );

    
	EndPaint(hWnd, &ps);

    return res;
}

static LRESULT HandleFileDropMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT res = 0;
    HDROP dropHandle = (HDROP)wParam;
    

    // if we have a drop handler, then marshall all the file names
    // and call the event handler. 
    // If the handler does not exist, don't bother with all the work

    FileDropEvent e;
    // Find out where the drop occured
    POINT pt;
    ::DragQueryPoint(dropHandle, &pt);
    e.x = (float)pt.x;
    e.y = (float)pt.y;

    // First, find out how many files were dropped
    auto n = ::DragQueryFileA(dropHandle, 0xffffffff, nullptr, 0);

    // Now that we know how many, query individual files
    // FileDropEvent
    char namebuff[512];

    if (n > 0) {
        for (size_t i = 0; i < n; i++) {
            ::DragQueryFileA(dropHandle, (UINT)i, namebuff, 512);
            e.filenames.push_back(std::string(namebuff));
        }

        gFileDropEventTopic.notify(e);
    }

    ::DragFinish(dropHandle);

    return res;
}

// Handling gestures
// Gesture messages WM_GESTURE, will only arrive if we're NOT
// registered for touch messages.  In an app, it might be useful
// to call noTouch() to ensure this is the case.
//
// https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/Win7Samples/Touch/MTGestures/cpp/GestureEngine.h
//
static LRESULT HandleGestureMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //std::cout << "HandleGestureMessage" << std::endl;
    GESTUREINFO gi;
    ZeroMemory(&gi, sizeof(GESTUREINFO));
    gi.cbSize = sizeof(GESTUREINFO);

    // get the gesture information
    BOOL bResult = GetGestureInfo((HGESTUREINFO)lParam, &gi);


    // If getting gesture info fails, return immediately
    // no further processing.
    // This will look the same as us handling the gesture, and
    // it might be better to throw some form of error
    if (!bResult) {
        //DWORD dwErr = ::GetLastError();
        return FALSE;
    }

    // if the gesture ID is BEGIN, or END, we can allow the default
    // behavior handle it
    switch (gi.dwID) {
    case GID_BEGIN:
    case GID_END:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    // If we are here, we can process well known gestures
    LRESULT res = 0;
    BOOL bHandled = FALSE;

    // Turn the physical screen location into local
    // window location
    POINT ptPoint{};
    ptPoint.x = gi.ptsLocation.x;
    ptPoint.y = gi.ptsLocation.y;
    ::ScreenToClient(hWnd, &ptPoint);
    
    GestureEvent ge{};
    ge.activity = gi.dwID;
    ge.x = ptPoint.x;
    ge.y = ptPoint.y;
    ge.distance = LODWORD(gi.ullArguments);

    // Turn flags into easy bool values
    ge.isBegin = ((gi.dwFlags & GF_BEGIN) == GF_BEGIN);
    ge.isEnd = ((gi.dwFlags & GF_END) == GF_END);
    ge.isInertia = ((gi.dwFlags & GF_INERTIA) == GF_INERTIA);

    //printf("ID: %d\tdwFlags: %d\n", gi.dwID, gi.dwFlags);

    // now interpret the gesture
    switch (gi.dwID) 
    {
        case GID_ZOOM:
            // zoom in and out, from a pinch or spread action
            // ullarguments indicates distance between the two points   
            bHandled = TRUE;
            break;
        case GID_PAN:
            // Code for panning goes here
            // If isInertia, then 
            // HIDWORD(ull) is an inertia vector (two 16-bit values).
            bHandled = TRUE;
            break;

        case GID_ROTATE:
            // Code for rotation goes here
            //printf("ROTATE\n");
            bHandled = TRUE;
            break;
        case GID_TWOFINGERTAP:
            // Code for two-finger tap goes here
            // printf("TWO FINGER TAP\n");
            bHandled = TRUE;
            break;
        case GID_PRESSANDTAP:
            // Code for roll over goes here
            //printf("PRESS AND TAP\n");
            // HIDWORD(ull), is the distance between the two points
            bHandled = TRUE;
            break;
        default:
            // A gesture was not recognized
            printf("GESTURE NOT RECOGNIZED\n");
            break;
        }



    
    ::CloseGestureInfoHandle((HGESTUREINFO)lParam);
    gGestureEventTopic.notify(ge);

    return res;
}

// Handle messages related to resizing the window
//
static LRESULT HandleSizeMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT res = 0;
    
    WORD newWidth = LOWORD(lParam);
    WORD newHeight = HIWORD(lParam);
    
    // Only resize bigger, never get smaller
   	WORD canWidth = waavs::max(canvasWidth, newWidth);
	WORD canHeight = waavs::max(canvasHeight, newHeight);
    
	//setCanvasSize(aWidth, aHeight);
    setCanvasSize(newWidth, newHeight);

    // onCanvasResize();
    ResizeEvent re{ newWidth, newHeight };
	gResizeEventTopic.notify(re);
    
    refreshScreenNow();
    
    return res;
}


//
//    Subscription routines
//
// General signal subscription
void subscribe(SignalEventTopic::Subscriber s)
{
    gSignalEventTopic.subscribe(s);
}

// Allow subscription to keyboard events
void subscribe(KeyboardEventTopic::Subscriber s)
{
    gKeyboardEventTopic.subscribe(s);
}

// Allow subscription to mouse events
void subscribe(MouseEventTopic::Subscriber s)
{
    gMouseEventTopic.subscribe(s);
}

// Allow subscription to mouse events
void subscribe(JoystickEventTopic::Subscriber s)
{
    gJoystickEventTopic.subscribe(s);
}

void subscribe(FileDropEventTopic::Subscriber s)
{
    gFileDropEventTopic.subscribe(s);
}

void subscribe(TouchEventTopic::Subscriber s)
{
    gTouchEventTopic.subscribe(s);
}

void subscribe(PointerEventTopic::Subscriber s)
{
    gPointerEventTopic.subscribe(s);
}

void subscribe(GestureEventTopic::Subscriber s)
{
    gGestureEventTopic.subscribe(s);
}

void subscribe(FrameCountEventTopic::Subscriber s)
{
    gFrameCountEventTopic.subscribe(s);
}

void subscribe(ResizeEventTopic::Subscriber s)
{
    gResizeEventTopic.subscribe(s);
}

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
    ::DragAcceptFiles(getAppWindow()->windowHandle(),TRUE);
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

// Change the window title
void setWindowTitle(const char* title)
{
    if (nullptr == getAppWindow())
        return;
    
	getAppWindow()->setTitle(title);
}

// Set an opacity value between 0.0 and 1.0
// 1.0 == fully opaque (not transparency)
// less than that makes the whole window more transparent
void windowOpacity(float o)
{
    getAppWindow()->setOpacity(o);
}

void setCanvasPosition(int x, int y)
{
    getAppWindow()->moveTo(x, y);
}



bool setCanvasSize(long aWidth, long aHeight)
{
    appFrameBuffer()->reset(aWidth, aHeight);

    canvasWidth = aWidth;
    canvasHeight = aHeight;

    canvasPixelData = (uint8_t *)appFrameBuffer()->data();
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

    gFrameCountEventTopic.notify(fce);



    // catch up to next frame interval
    // this will possibly result in dropped
    // frames, but it will ensure we keep up
    // to speed with the wall clock
    //while (fNextMillis <= gAppClock.millis())
    //{
    //    fNextMillis += fInterval;
    //}
//}


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
    User32Window* pThis = nullptr;
    //reinterpret_cast<User32Window*> (::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTA* pCreate = reinterpret_cast<CREATESTRUCTA*>(lParam);
        pThis = reinterpret_cast<User32Window*>(pCreate->lpCreateParams);
        if (pThis) {
            pThis->setWindowHandle(hWnd);

            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        }
	}
	else {
		pThis = reinterpret_cast<User32Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}

    
    
    switch (msg)
    {



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
        HandleSizeMessage(hWnd, msg, wParam, lParam);
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
        res = HandleRawInputMessage(hWnd, msg, wParam, lParam);
        break;

    case WM_TOUCH:
        res = HandleTouchMessage(hWnd, msg, wParam, lParam);
        break;

    case WM_GESTURE:
        // we will only receive WM_GESTURE if not receiving WM_TOUCH
        HandleGestureMessage(hWnd, msg, wParam, lParam);
        break;

    case WM_DROPFILES:
        HandleFileDropMessage(hWnd, msg, wParam, lParam);
        break;

    default:
    {
        if ((msg >= WM_MOUSEFIRST) && (msg <= WM_MOUSELAST)) {
            // Handle all mouse messages
            HandleMouseMessage(hWnd, msg, wParam, lParam);
        }
        else if ((msg >= WM_KEYFIRST) && (msg <= WM_KEYLAST)) {
            // Handle all keyboard messages
            HandleKeyboardMessage(hWnd, msg, wParam, lParam);
        }
        else if ((msg >= MM_JOY1MOVE) && (msg <= MM_JOY2BUTTONUP)) {
            //printf("MM_JOYxxx: %p\n", gJoystickHandler);
            HandleJoystickMessage(hWnd, msg, wParam, lParam);
        }
        else {
            res = ::DefWindowProcA(hWnd, msg, wParam, lParam);
        }
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
    gPaintHandler = HandlePaintMessage;


    // One of the primary handlers the user can specify is 'onPaint'.  
    // If implemented, this function will be called whenever a WM_PAINT message
    // is seen by the application.
    WinMSGObserver handler = (WinMSGObserver)::GetProcAddress(hInst, "onPaint");
    if (handler != nullptr) {
        gPaintHandler = handler;
    }

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

        /*
        // we use peekmessage, so we don't stall on a GetMessage
        // should probably throw a wait here
        // WaitForSingleObject
        BOOL bResult = ::PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE);
        
        if (bResult > 0) {
            // If we are here, there is some message to be handled
            // If we see a quit message, it's time to stop the program
            if (msg.message == WM_QUIT) {
                break;
            }

            // Do regular Windows message dispatch
            ::TranslateMessage(&msg);
            ::DispatchMessageA(&msg);
        }
        else {
            processFrameTiming();
        }
        */

        // Give the user application some control to do what
        // it wants
        // call onLoop() if it exists
        if (gOnLoopHandler != nullptr) {
            gOnLoopHandler();
        }
    }
    

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
waavs::User32Window * getAppWindow()
{
    // Declare some standard Window Kinds we'll be using
    static User32WindowClass appWindowKind("appwindow",  CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC, MsgHandler);
    static std::unique_ptr<waavs::User32Window> gAppWindow = nullptr;
    
    if (!gAppWindow)
    {
        int style = WS_OVERLAPPEDWINDOW;
        int xstyle = 0;

        auto win = appWindowKind.createWindow("Application Window", 320, 240, style, xstyle);
		gAppWindow = std::unique_ptr<waavs::User32Window>(win);
        
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
int main(int argc, char **argv)
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