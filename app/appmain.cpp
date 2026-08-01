
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
#include "pixeling.h"




using namespace waavs;

static void processFrameTiming();

// Some function signatures
// WinMSGObserver - Function signature for Win32 message handler
typedef bool (*WinMSGObserver)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT &result);

#define LODWORD(ull) ((DWORD)((ULONGLONG)(ull) & 0x00000000ffffffff))
#define HIDWORD(ull) ((DWORD)((((ULONGLONG)(ull))>>32) & 0x00000000ffffffff))

// Application routines
// appmain looks for this routine in the compiled application
static VOIDROUTINE gOnloadHandler = nullptr;
static VOIDROUTINE gOnUnloadHandler = nullptr;

static VOIDROUTINE gOnLoopHandler = nullptr;

// Painting
//static WinMSGObserver gPaintHandler = nullptr;

// Topics applications can subscribe to
SignalEventTopic gSignalEventTopic;
KeyboardEventTopic gKeyboardEventTopic;
MouseEventTopic gMouseEventTopic;
JoystickEventTopic gJoystickEventTopic;
FileDropEventTopic gFileDropEventTopic;
TouchEventTopic gTouchEventTopic;
//PointerEventTopic gPointerEventTopic;
GestureEventTopic gGestureEventTopic;
FrameCountEventTopic gFrameCountEventTopic;
ResizeEventTopic gResizeEventTopic;
PenEventTopic gPenEventTopic;

// Miscellaneous globals
int gargc;
char **gargv;


static StopWatch gAppClock;

bool gIsLayered = false;
bool gInSizeMove = false;

// Some globals friendly to the p5 environment
// Display Globals
int appFrameWidth = 0;
int appFrameHeight = 0;
uint8_t* appFramePixelData = nullptr;
size_t appFrameStride = 0;


static int screenPixelHeight = 0;
static int screenPixelWidth = 0;
static double screenWidthInches;
static double screenHeightInches;

unsigned int physicalDpi = 96;   // starting pixel density

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



static constexpr int DEFAULT_APP_WINDOW_WIDTH = 640;
static constexpr int DEFAULT_APP_WINDOW_HEIGHT = 480;

APP_EXPORT bool getScreenPixelCount(int& pixelWidth, int& pixelHeight)
{
    pixelWidth = screenPixelWidth;
    pixelHeight = screenPixelHeight;

    return true;
}


APP_EXPORT bool getScreenInches(double& scrWidth, double& scrHeight)
{
    scrWidth = screenWidthInches;
    scrHeight = screenHeightInches;

    return true;
}

Surface * getAppSurface()
{
    static std::unique_ptr<Surface> gAppSurface = std::make_unique<Surface>();
    return gAppSurface.get();
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
void setFrameRate(float newRate) noexcept
{
    fFrameRate = newRate;
    fInterval = 1000.0 / newRate;
    fNextMillis = gAppClock.millis() + fInterval;
}

float getFrameRate() noexcept { return fFrameRate;}
uint64_t frameCount() noexcept { return fFrameCount;}


bool presentLayeredWindow(HWND hwnd)
{
    // This is the workhorse of displaying directly
    // to the screen.  Everything to be displayed
    // must be in the FrameBuffer, even window chrome
    LayeredWindowInfo lw(appFrameWidth, appFrameHeight);
    //lw.display(getAppWindow()->windowHandle(), getAppFrameBuffer()->getGDIContext());
}

// requestScreenRefresh()
void refreshScreenRefresh()
{

}

// This function is called at any time to display whatever is in the app 
// window to the actual screen
bool refreshScreenNow()
{
    User32Window* window = getAppWindow();
    if (!window)
        return false;

    HWND hwnd = window->windowHandle();
    if (!hwnd || !::IsWindow(hwnd))
        return false;

    if (gIsLayered)
        return presentLayeredWindow(hwnd);


    // if we're not layered, then do a regular
    // sort of WM_PAINT based drawing
    return ::RedrawWindow(hwnd,
        NULL, 
        NULL, 
        RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE) != FALSE;

}


//
//    Environment
//

// show()
// Show the application window
//
void show()
{
    getAppWindow()->show();
}

// hide()
// Hide the application window
//
void hide()
{
    getAppWindow()->hide();
}

// cursor()
// Show the cursor, if there is one
//
void cursor()
{
    ::ShowCursor(1);
}

// noCursor()
// 
// Hide the cursor, if there is one
// BUGBUG - we should be more robust here and
// check to see if there's even a mouse attached
// Then, decrement count enough times to make showing
// the cursor absolute rather than relative.
void noCursor()
{
    ::ShowCursor(0);
}



//  refreshKeyStates()
// 
//  Turn Windows keyboard messages into keyevents that can 
//  more easily be handled at the application level
//
void refreshKeyStates()
{
    // Get the state of all keys on the keyboard
    ::GetKeyboardState(keyStates);
}

//  HandleKeyboardMessage()
// 
//   Handle keyboard messages from Windows
//
static bool PublishKeyboardMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    KeyboardEvent e{};

    keyCode = 0;
    keyChar = 0;

    e.keyCode = (int)wParam;
    e.repeatCount =LOWORD(lParam);  // 0 - 15
    e.scanCode = ((lParam & 0xff0000) >> 16);        // 16 - 23
    e.isExtended = (lParam & 0x1000000) != 0;    // 24
    e.wasDown = (lParam & 0x40000000) != 0;    // 30

    // Get the state of all keys on the keyboard
    ::GetKeyboardState(keyStates);

    //printf("HandleKeyboardMessage: %04x\n", msg);
    switch (msg) {
    case WM_KEYDOWN:
        e.activity = KEYPRESSED;
        break;

    case WM_KEYUP:
        e.activity = KEYRELEASED;
        break;

    case WM_CHAR:
        e.activity = KEYTYPED;
        break;

    default:
        return false;
    }

    // publish keyboard event
    gKeyboardEventTopic.notify(e);

    return true;
}


//  HandleMouseMessage()
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

    //e.xbutton1 = (wParam & MK_XBUTTON1) != 0;
    //e.xbutton2 = (wParam & MK_XBUTTON2) != 0;
    const WORD xButton = GET_XBUTTON_WPARAM(wParam);
    e.xbutton1 = xButton == XBUTTON1;
    e.xbutton2 = xButton == XBUTTON2;

    const bool isPressed = e.lbutton || e.rbutton || e.mbutton || e.xbutton1 || e.xbutton2;
    const bool isXButtonMessage = 
        (msg == WM_XBUTTONDOWN || 
            msg == WM_XBUTTONUP || 
            msg == WM_XBUTTONDBLCLK);

    switch(msg) {
        case WM_LBUTTONDBLCLK:
	    case WM_MBUTTONDBLCLK:
	    case WM_RBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK:
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
            //printf("UNKNOWN MOUSE\n");
            return ::DefWindowProcA(hwnd, msg, wParam, lParam);
        break;
    }

    mouseX = e.x;
    mouseY = e.y;
    mouseDelta = e.delta;
    mouseIsPressed = isPressed;

    //printf("MOUSE: %d %d\n", e.xbutton1, e.xbutton2);

    gMouseEventTopic.notify(e);

    return isXButtonMessage ? TRUE : 0;
}

// HandleRawInputMessage()
//
static LRESULT HandleRawInputMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //printf("HandleRawInputMessage (WM_INPUT)\n");
    
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

// HandleJoystickMessage()
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

// HandleTouchMessage()
//
static LRESULT HandleTouchMessage(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    const UINT inputCount = LOWORD(wParam);
    const HTOUCHINPUT inputHandle =
        reinterpret_cast<HTOUCHINPUT>(lParam);

    // Reuse the allocation across messages. This assumes touch
    // messages are dispatched on a single UI thread.
    static std::vector<TOUCHINPUT> inputs;

    if (inputCount == 0)
    {
        // We are not consuming the message, so DefWindowProc
        // remains responsible for the touch-input handle.
        return ::DefWindowProcA(
            hwnd,
            msg,
            wParam,
            lParam);
    }

    // May throw std::bad_alloc. The handler deliberately allows
    // allocation failures to propagate to the application's
    // exception boundary.
    inputs.resize(inputCount);

    if (!::GetTouchInputInfo(
        inputHandle,
        inputCount,
        inputs.data(),
        sizeof(TOUCHINPUT)))
    {
        // Do not close inputHandle here. DefWindowProc will close
        // it when processing the unhandled message.
        return ::DefWindowProcA(
            hwnd,
            msg,
            wParam,
            lParam);
    }

    for (UINT i = 0; i < inputCount; ++i)
    {
        const TOUCHINPUT& ti = inputs[i];

        TouchEvent e{};

        e.id = ti.dwID;

        // TOUCHINPUT coordinates are expressed in hundredths of
        // a physical pixel and are relative to the screen.
        e.rawX = static_cast<float>(ti.x);
        e.rawY = static_cast<float>(ti.y);

        POINT pt{
            TOUCH_COORD_TO_PIXEL(ti.x),
            TOUCH_COORD_TO_PIXEL(ti.y)
        };

        if (::ScreenToClient(hwnd, &pt))
        {
            e.x = static_cast<float>(pt.x);
            e.y = static_cast<float>(pt.y);
        }
        else
        {
            // Preserve useful screen-pixel coordinates if the
            // conversion unexpectedly fails.
            e.x = static_cast<float>(
                TOUCH_COORD_TO_PIXEL(ti.x));

            e.y = static_cast<float>(
                TOUCH_COORD_TO_PIXEL(ti.y));
        }

        if ((ti.dwMask &
            TOUCHINPUTMASKF_CONTACTAREA) != 0)
        {
            e.rawWidth =
                static_cast<float>(ti.cxContact);

            e.rawHeight =
                static_cast<float>(ti.cyContact);

            e.w = static_cast<float>(
                TOUCH_COORD_TO_PIXEL(ti.cxContact));

            e.h = static_cast<float>(
                TOUCH_COORD_TO_PIXEL(ti.cyContact));
        }

        // Treat DOWN, UP, and MOVE as mutually exclusive
        // activities in the application's event model.
        if ((ti.dwFlags & TOUCHEVENTF_DOWN) != 0)
        {
            e.activity = TOUCH_DOWN;
            e.isDown = true;
        }
        else if ((ti.dwFlags & TOUCHEVENTF_UP) != 0)
        {
            e.activity = TOUCH_UP;
            e.isUp = true;
        }
        else if ((ti.dwFlags & TOUCHEVENTF_MOVE) != 0)
        {
            e.activity = TOUCH_MOVE;
            e.isMoving = true;
        }
        else
        {
            // No activity represented by the current TouchEvent
            // model. Ignore this record but continue processing
            // the rest of the message.
            continue;
        }

        e.isHovering =
            (ti.dwFlags & TOUCHEVENTF_INRANGE) != 0;

        e.isPrimary =
            (ti.dwFlags & TOUCHEVENTF_PRIMARY) != 0;

        e.isPalm =
            (ti.dwFlags & TOUCHEVENTF_PALM) != 0;

        e.isPen =
            (ti.dwFlags & TOUCHEVENTF_PEN) != 0;

        gTouchEventTopic.notify(e);
    }

    // GetTouchInputInfo succeeded, so this handler consumed the
    // message and is responsible for releasing the handle.
    if (!::CloseTouchInputHandle(inputHandle))
    {
        const DWORD error = ::GetLastError();
        (void)error;

        // Optional diagnostic:
        // printf(
        //     "CloseTouchInputHandle failed: %lu\n",
        //     error);
    }

    return 0;
}


static bool HandlePenMessage(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT& result)
{
    const UINT32 pointerId =
        GET_POINTERID_WPARAM(wParam);

    POINTER_INPUT_TYPE pointerType = PT_POINTER;

    if (!::GetPointerType(pointerId, &pointerType))
        return false;

    if (pointerType != PT_PEN)
        return false;

    PenEvent e{};
    e.pointerId = pointerId;

    // WM_POINTERCAPTURECHANGED is primarily a cleanup notification.
    // It does not need fresh pen sample data.
    if (msg == WM_POINTERCAPTURECHANGED)
    {
        e.activity = PEN_CAPTURE_LOST;
        gPenEventTopic.notify(e);

        result = 0;
        return true;
    }

    POINTER_PEN_INFO penInfo{};

    if (!::GetPointerPenInfo(pointerId, &penInfo))
        return false;

    const POINTER_INFO& info =
        penInfo.pointerInfo;

    POINT pt = info.ptPixelLocation;

    // POINTER_INFO positions are screen coordinates.
    if (!::ScreenToClient(hwnd, &pt))
        return false;

    e.x = static_cast<float>(pt.x);
    e.y = static_cast<float>(pt.y);

    const UINT32 pointerFlags =
        info.pointerFlags;

    e.inRange =
        (pointerFlags & POINTER_FLAG_INRANGE) != 0;

    e.inContact =
        (pointerFlags & POINTER_FLAG_INCONTACT) != 0;

    e.primary =
        (pointerFlags & POINTER_FLAG_PRIMARY) != 0;

    e.canceled =
        (pointerFlags & POINTER_FLAG_CANCELED) != 0;

    e.barrelButton =
        (penInfo.penFlags & PEN_FLAG_BARREL) != 0;

    e.inverted =
        (penInfo.penFlags & PEN_FLAG_INVERTED) != 0;

    e.eraser =
        (penInfo.penFlags & PEN_FLAG_ERASER) != 0;

    e.hasPressure =
        (penInfo.penMask & PEN_MASK_PRESSURE) != 0;

    e.hasRotation =
        (penInfo.penMask & PEN_MASK_ROTATION) != 0;

    e.hasTiltX =
        (penInfo.penMask & PEN_MASK_TILT_X) != 0;

    e.hasTiltY =
        (penInfo.penMask & PEN_MASK_TILT_Y) != 0;

    if (e.hasPressure)
    {
        e.pressure =
            static_cast<float>(penInfo.pressure) /
            1024.0f;
    }

    if (e.hasRotation)
        e.rotation = static_cast<float>(penInfo.rotation);

    if (e.hasTiltX)
        e.tiltX = static_cast<float>(penInfo.tiltX);

    if (e.hasTiltY)
        e.tiltY = static_cast<float>(penInfo.tiltY);

    switch (msg)
    {
    case WM_POINTERENTER:
        e.activity = PEN_ENTERED;
        break;

    case WM_POINTERLEAVE:
        e.activity = PEN_LEFT;
        break;

    case WM_POINTERDOWN:
        e.activity = PEN_PRESSED;
        break;

    case WM_POINTERUP:
        e.activity = PEN_RELEASED;
        break;

    case WM_POINTERUPDATE:
        e.activity =
            e.inContact
            ? PEN_MOVED
            : PEN_HOVERED;
        break;

    default:
        return false;
    }

    gPenEventTopic.notify(e);

    // We handled this pen pointer message.
    result = 0;
    return true;
}



// HandlePaintMessage()
//
// Takes care of putting the appFrameBuffer onto the screen in response
// to a Windows paint message.
//
static LRESULT HandlePaintMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //printf("HandlePaintMessage\n");

    LRESULT res = 0;
    PAINTSTRUCT ps;

	HDC hdc = BeginPaint(hWnd, &ps);
        
    int Destx = 0;
    int Desty = 0;
    int DestWidth = appFrameWidth;
    int DestHeight = appFrameHeight;
    int xSrc = 0;
    int ySrc = 0;
    int SrcWidth = appFrameWidth;
    int SrcHeight = appFrameHeight;
    UINT StartScan = 0;
    UINT cLines = appFrameHeight;

    Surface* s = getAppSurface();

    BITMAPINFO bminfo{};
    bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bminfo.bmiHeader.biWidth = (LONG)s->width();
    bminfo.bmiHeader.biHeight = -(LONG)s->height();	// top-down DIB Section
    bminfo.bmiHeader.biPlanes = 1;
    bminfo.bmiHeader.biClrImportant = 0;
    bminfo.bmiHeader.biClrUsed = 0;
    bminfo.bmiHeader.biCompression = BI_RGB;
    bminfo.bmiHeader.biBitCount = 32;
    bminfo.bmiHeader.biSizeImage = DWORD(s->stride() * s->height());


    // We are assuming the affFrameBuffer is free to be
    // drawn at this moment

    int pResult = SetDIBitsToDevice(
        hdc,
        Destx,
        Desty,
        DestWidth,
        DestHeight,
        xSrc,
        ySrc,
        StartScan,
        cLines,
        s->data(),
        &bminfo,
        DIB_RGB_COLORS
    );

    
	EndPaint(hWnd, &ps);

    return res;
}

// HandleFileDropMessage()
//
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

// HandleGestureMessage()
// 
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


    // If getting gesture info fails, return result
    // of default message handling
    if (!bResult) {
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    // If we are here, we can process well known gestures
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

    case GID_BEGIN:
    case GID_END:
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }


    if (!bHandled)
    {
        // don't close the handle here
        // DefWindowProc assumes ownership
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    
    ::CloseGestureInfoHandle(reinterpret_cast<HGESTUREINFO>(lParam));

    gGestureEventTopic.notify(ge);

    return 0;
}

// HandleSizeMessage()
// 
// Handle messages related to resizing the window
//
static LRESULT HandleSizeMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
    
    WORD newWidth = LOWORD(lParam);
    WORD newHeight = HIWORD(lParam);
    
    // Only resize bigger, never get smaller
   	WORD canWidth = waavs::max(appFrameWidth, newWidth);
	WORD canHeight = waavs::max(appFrameHeight, newHeight);
    
	//setCanvasSize(aWidth, aHeight);
    setAppFrameSize(newWidth, newHeight);

    // onCanvasResize();
    ResizeEvent re{ newWidth, newHeight };
	gResizeEventTopic.notify(re);
    
    refreshScreenNow();
    
    return 0;
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

void subscribe(PenEventTopic::Subscriber s)
{
    gPenEventTopic.subscribe(s);
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




bool setAppFrameSize(long aWidth, long aHeight)
{
    Surface* s = getAppSurface();
    if (!s) {
        return false;
    }

    *s = Surface::createOwned(aWidth, aHeight);
    //s->reset(aWidth, aHeight);

    appFrameWidth = aWidth;
    appFrameHeight = aHeight;

    appFramePixelData = (uint8_t *)s->data();
    appFrameStride = s->stride();

    return true;
}

// Put the application canvas into a window
void createAppWindow(long aWidth, long aHeight, const char* title)
{
    auto win = getAppWindow();

    setAppFrameSize(aWidth, aHeight);
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
    const double nowMillis = gAppClock.millis();

    // This routine may be called freely from multiple points in
    // the message-processing path. Only dispatch when due.
    if (nowMillis < fNextMillis)
        return;

    // Advance the deadline before notification. This prevents a
    // re-entrant call from publishing the same frame again.
    //
    // Emit only one event even when several intervals were missed.
    // Advancing from the previous deadline preserves the original
    // cadence rather than introducing timing drift.
    const double intervalsElapsed =
        std::floor((nowMillis - fNextMillis) / fInterval) + 1.0;

    fNextMillis += intervalsElapsed * fInterval;

    ++fFrameCount;

    FrameCountEvent fce{};
    fce.frameCount = fFrameCount;
    fce.seconds = nowMillis / 1000.0;

    gFrameCountEventTopic.notify(fce);
}


/*
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
*/

//
//    Generic Windows message handler
//    This is used as the function to associate with a window class
//    when it is registered.
//
static bool isMouseEvent(UINT msg) {return (msg >= WM_MOUSEFIRST) && (msg <= WM_MOUSELAST);}
static bool isKeyboardEvent(UINT msg) { return (msg >= WM_KEYFIRST) && (msg <= WM_KEYLAST); }
static bool isJoystickEvent(UINT msg) { return (msg >= MM_JOY1MOVE) && (msg <= MM_JOY2BUTTONUP); }
static bool isPenEvent(UINT msg) { 
    switch (msg) {
        case WM_POINTERENTER:
        case WM_POINTERLEAVE:
        case WM_POINTERDOWN:
        case WM_POINTERUP:
        case WM_POINTERUPDATE:
        case WM_POINTERCAPTURECHANGED:
            return true;
    }

    return false;
}

static LRESULT CALLBACK MsgHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    User32Window* window = nullptr;
    
    if (msg == WM_NCCREATE)
    {
        auto* create = reinterpret_cast<CREATESTRUCTA*>(lParam);

        window = static_cast<User32Window*>(create->lpCreateParams);

        if (!window) {
            return FALSE;
        }

        window->setWindowHandle(hWnd);

        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));

        return ::DefWindowProcA(hWnd, msg, wParam, lParam);
	}

    // Get pointer to window class from handle
    window = reinterpret_cast<User32Window*>(::GetWindowLongPtrA(hWnd, GWLP_USERDATA));

    if (msg == WM_NCDESTROY)
    {
        LRESULT result = ::DefWindowProcA(hWnd, msg, wParam, lParam);
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
        if (window)
            window->setWindowHandle(nullptr);

        return result;
    }
    
    // Mouse and keyboard are typically the most frequent
    // so let's handle them up front.
    if (isMouseEvent(msg)) {
        // Handle all mouse messages
        return HandleMouseMessage(hWnd, msg, wParam, lParam);
    }

    if (isKeyboardEvent(msg)) {

        PublishKeyboardMessage(hWnd, msg, wParam, lParam);

        return ::DefWindowProcA(hWnd, msg, wParam, lParam);
    }

    switch (msg)
    {
    case WM_ERASEBKGND:
        //printf("WM_ERASEBKGND\n");
        // return non-zero to tell the system we handled
        // the erasing of the background, and no further
        // processing needs to occur
        return 1;

    case WM_PAINT: 
        return HandlePaintMessage(hWnd, msg, wParam, lParam);


    case WM_ENTERSIZEMOVE:
        gInSizeMove = true;
        return 0;

    case WM_EXITSIZEMOVE:
        gInSizeMove = false;
        refreshScreenNow();
        return 0;


    case WM_MOVING:
    case WM_SIZING:
        processFrameTiming();
        break;  // Continue to DefWindowProcA() for default processing

    case WM_WINDOWPOSCHANGING:
        if (gInSizeMove)
            processFrameTiming();
        break;  // Continue to DefWindowProcA() for default processing

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;

        return HandleSizeMessage(hWnd, msg, wParam, lParam);

    case WM_DESTROY:
        // By doing a PostQuitMessage(), a 
        // WM_QUIT message will eventually find its way into the
        // message queue.
        ::PostQuitMessage(0);
        return 0;

    case WM_INPUT:
        return  HandleRawInputMessage(hWnd, msg, wParam, lParam);

    case WM_TOUCH:
        return HandleTouchMessage(hWnd, msg, wParam, lParam);

    // we will only receive WM_GESTURE if not receiving WM_TOUCH
    case WM_GESTURE:
        return HandleGestureMessage(hWnd, msg, wParam, lParam);

    case WM_DROPFILES:
        return HandleFileDropMessage(hWnd, msg, wParam, lParam);
        break;

    }
    


    if (isJoystickEvent(msg)) {
        //printf("MM_JOYxxx: %p\n", gJoystickHandler);
        return HandleJoystickMessage(hWnd, msg, wParam, lParam);
    }

    if (isPenEvent(msg)) {
        LRESULT result = 0;
        if (HandlePenMessage(hWnd, msg, wParam, lParam, result))
            return result;
    }

    return ::DefWindowProcA(hWnd, msg, wParam, lParam);
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


    // Get the general app routines
    // onLoad()
    gOnloadHandler = (VOIDROUTINE)::GetProcAddress(hInst, "onLoad");
    gOnUnloadHandler = (VOIDROUTINE)::GetProcAddress(hInst, "onUnload");
    gOnLoopHandler = (VOIDROUTINE)::GetProcAddress(hInst, "onLoop");
}


// runLoop()
//
// This is the central scheduler for the WAAVS runtime.
//
// Although Windows applications are traditionally organized around a
// message pump, this loop has a broader responsibility.  It arbitrates
// between three independent sources of work:
//
//   1. Windows messages (input, painting, window management, etc.)
//   2. Time-based work (frame timing and animation scheduling)
//   3. Application work (onLoop() and future runtime services)
//
// The scheduler attempts to give each category a fair share of execution
// time.  It intentionally does not drain the Windows message queue before
// returning to other work, since a continuous stream of input messages
// could otherwise starve animation and application processing.
//
// Likewise, frame timing is no longer driven by the wait timeout.
// processFrameTiming() maintains its own schedule and simply returns
// when no frame is due.  This allows the scheduler to poll for frame
// deadlines whenever convenient without introducing timing drift.
//
// MsgWaitForMultipleObjectsEx() is used only to avoid busy-waiting while
// the application is idle.  A short timeout allows the thread to sleep,
// yet still wake frequently enough to remain responsive even when no
// Windows messages are arriving.
//
// This design intentionally resembles a small cooperative kernel
// scheduler rather than a traditional message pump.  Future runtime
// services (signals, timers, task queues, fibers, etc.) can participate
// in this scheduling loop without changing its overall structure.

static void runLoop()
{
    // How long the thread may sleep when there is no immediate work.
    //
    // This prevents an idle busy-spin while still keeping the runtime
    // responsive to frame deadlines and application work.
    constexpr DWORD IDLE_WAIT_MILLIS = 1;

    // Maximum amount of time devoted to Windows messages during
    // one scheduler pass.
    constexpr double MESSAGE_TIME_SLICE_MILLIS = 2.0;

    MSG msg{};
    bool running = true;

    while (running)
    {
        const DWORD waitResult = ::MsgWaitForMultipleObjectsEx(
            0,                      // no kernel objects
            nullptr,
            IDLE_WAIT_MILLIS,
            QS_ALLEVENTS,
            MWMO_INPUTAVAILABLE);

        if (waitResult == WAIT_FAILED)
            break;

        // Frame timing is self-regulating, so it is safe to check
        // on every scheduler pass regardless of why the wait ended.
        processFrameTiming();

        if (waitResult == WAIT_OBJECT_0)
        {
            const double sliceStartMillis = gAppClock.millis();

            do
            {
                if (!::PeekMessageA(
                    &msg,
                    nullptr,
                    0,
                    0,
                    PM_REMOVE))
                {
                    break;
                }

                if (msg.message == WM_QUIT)
                {
                    running = false;
                    break;
                }

                ::TranslateMessage(&msg);
                ::DispatchMessageA(&msg);

                // Yield the scheduler once this message batch has
                // consumed its allotted time.
                const double elapsedMillis =
                    gAppClock.millis() - sliceStartMillis;

                if (elapsedMillis >= MESSAGE_TIME_SLICE_MILLIS)
                    break;

            } while (true);
        }

        if (!running)
            break;

        // Message handlers themselves may have consumed enough time
        // to cross another frame boundary.
        processFrameTiming();

        // Give the application its portion of the scheduler pass.
        if (gOnLoopHandler)
            gOnLoopHandler();
    }
}

/*
static void runLoop()
{
    static bool running = true;

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
}
*/

static void run()
{
    // Initiate frame timing variables
    fFrameCount = 0;
    fNextMillis = gAppClock.millis();

    // Make sure we have all the event handlers connected
    registerHandlers();

    // call the application's 'onLoad()' if it exists
    if (gOnloadHandler != nullptr) {
        gOnloadHandler();
    }




    showAppWindow();

    gAppClock.start();

    runLoop();
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

	screenPixelWidth = ::GetSystemMetrics(SM_CXSCREEN);
	screenPixelHeight = ::GetSystemMetrics(SM_CYSCREEN);
    

    // Create a DC to query EDID-based physical size
    // This won't be accurate if using a virtual terminal, or if the 
    // monitor driver does not report it accurately
    auto dhdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

    // How big is the screen physically
    // DeviceCaps gives it in millimeters, so we convert to inches
    screenWidthInches = ::GetDeviceCaps(dhdc, HORZSIZE) / 25.4;
    screenHeightInches = ::GetDeviceCaps(dhdc, VERTSIZE) / 25.4;


    // Calculate physical DPI (using vertial dimensions)
    double screenPpi = (double)screenPixelHeight / screenHeightInches;
    //physicalDpi = (unsigned int)std::round(screenPpi);

    ::DeleteDC(dhdc);
}


// Initialize Windows networking
static bool setupNetworking()
{
    // BUGBUG - decide whether or not we care about WSAStartup failing
    uint16_t version = MAKEWORD(2, 2);
    WSADATA lpWSAData;
    int res = ::WSAStartup(version, &lpWSAData);
    if (res != 0)
    {
        printf("Error setting up networking: 0x%x\n", res);
        return false;
    }

    return true;
}

//! \brief Retrieves the main (singleton) application window.
//
//! This function returns a pointer to the main application window. On the first
//! call, it creates a new window using the "appwindow" window class, which is
//! registered with the given class styles and message handler. The newly
//! created window measures 640 x 480. On subsequent calls, it returns the
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

        auto win = appWindowKind.createWindow("Application Window", 
            DEFAULT_APP_WINDOW_WIDTH, DEFAULT_APP_WINDOW_HEIGHT, 
            style, xstyle);
		gAppWindow = std::unique_ptr<waavs::User32Window>(win);
        
    }
    
    return gAppWindow.get();
}

static int getSystemThreadCount()
{
    return std::thread::hardware_concurrency();
}

static bool prolog()
{
    setupNetworking();

    setDPIAware();

    // set the canvas a default size to start
    // but don't show it
    auto win = getAppWindow();
    setAppFrameSize(DEFAULT_APP_WINDOW_WIDTH, DEFAULT_APP_WINDOW_HEIGHT);

    return true;
}

// Do whatever cleanup needs to be done before
// exiting application
static void epilog()
{   
    if (gOnUnloadHandler != nullptr) {
        gOnUnloadHandler();
    }
    
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