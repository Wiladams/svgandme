#pragma once


// This is an implementation of a Window that is run in its own thread.
// You construct it, then call 'run' and it will run its own message
// loop.  Closing the window will cause the thread to exit.

#include <shellapi.h>           // for drag-drop support



#include <thread>
#include <objbase.h>
#include <vector>

#include "nativewindow.h"
#include "pubsub.h"
#include "stopwatch.h"
#include "joystick.h"


// Make Topic publishers available
// Doing C++ pub/sub
// These are only accessible through the C++ interface
using SignalEventTopic = waavs::Topic<intptr_t>;
using MouseEventTopic = waavs::Topic<waavs::MouseEvent>;				// Mouse events
using KeyboardEventTopic = waavs::Topic<waavs::KeyboardEvent>;		// Keyboard events
using JoystickEventTopic = waavs::Topic<waavs::JoystickEvent>;		// Joystick events
using FileDropEventTopic = waavs::Topic<waavs::FileDropEvent>;		// File drop events
using TouchEventTopic = waavs::Topic<waavs::TouchEvent>;				// Touch events
using PointerEventTopic = waavs::Topic<waavs::PointerEvent>;			// Pointer events
using GestureEventTopic = waavs::Topic<waavs::GestureEvent>;			// Gesture events
using FrameCountEventTopic = waavs::Topic<waavs::FrameCountEvent>;	// Timed Frame count events
using ResizeEventTopic = waavs::Topic<waavs::ResizeEvent>;			// Application Window Resize events	

#define LODWORD(ull) ((DWORD)((ULONGLONG)(ull) & 0x00000000ffffffff))
#define HIDWORD(ull) ((DWORD)((((ULONGLONG)(ull))>>32) & 0x00000000ffffffff))



namespace waavs {
    
    //
    // AFrameBuffer - A structure that combines a GDI DIBSection with a blend2d BLImage
    // You can get either a blend2d context out of it, 
    // or a GDI32 Drawing Context
    // The BLImage is used to represent the image data for ease of use
    //
    struct AFrameBuffer 
    {
        uint8_t* fFrameBufferData{ nullptr };
        
        BITMAPINFO fGDIBMInfo{ {0} };          // retain bitmap info for  future usage
        HBITMAP fGDIDIBHandle = nullptr;       // Handle to the dibsection to be created
        HDC     fGDIBitmapDC = nullptr;        // DeviceContext dib is selected into
        size_t fBytesPerRow{ 0 };
        
        // Blend2d image and context
        BLContext fB2dContext{};
        BLImage fB2dImage{};
        


        AFrameBuffer(int w, int h)
        {
            reset(w, h);
        }
        
        virtual ~AFrameBuffer()
        {
            fB2dContext.end();
            
            ::DeleteDC(fGDIBitmapDC);
            ::DeleteObject(fGDIDIBHandle);
        }
        
		BLImage& getBlend2dImage() { return fB2dImage; }
		BLContext& getBlend2dContext() { return fB2dContext; }
        
		HDC getGDIContext() { return fGDIBitmapDC; }
        

        
        void resetGDIDC()
        {
            if (fGDIBitmapDC)
                ::DeleteDC(fGDIBitmapDC);

            // Create a GDI Device Context
            fGDIBitmapDC = ::CreateCompatibleDC(nullptr);

            // Do some setup to the DC to make it suitable
            // for drawing with GDI if we choose to do that
            ::SetGraphicsMode(fGDIBitmapDC, GM_ADVANCED);
            ::SetBkMode(fGDIBitmapDC, TRANSPARENT);        // for GDI text rendering

        }
        
        void resetGDIDibSection(int w, int h)
        {
            // The framebuffer is based on a DIBSection
            if (fGDIDIBHandle)
            {
                // destroy current DIB
                DeleteObject(fGDIDIBHandle);
                fGDIDIBHandle = nullptr;
            }

            fBytesPerRow = 4 * w;
            fGDIBMInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            fGDIBMInfo.bmiHeader.biWidth = (LONG)w;
            fGDIBMInfo.bmiHeader.biHeight = -(LONG)h;	// top-down DIB Section
            fGDIBMInfo.bmiHeader.biPlanes = 1;
            fGDIBMInfo.bmiHeader.biClrImportant = 0;
            fGDIBMInfo.bmiHeader.biClrUsed = 0;
            fGDIBMInfo.bmiHeader.biCompression = BI_RGB;
            fGDIBMInfo.bmiHeader.biBitCount = 32;
            fGDIBMInfo.bmiHeader.biSizeImage = DWORD(fBytesPerRow * h);


            fGDIDIBHandle = ::CreateDIBSection(nullptr, &fGDIBMInfo, DIB_RGB_COLORS, (void**)&fFrameBufferData, nullptr, 0);

			if ((fGDIBitmapDC != nullptr) && (fGDIDIBHandle != nullptr))
                ::SelectObject(fGDIBitmapDC, fGDIDIBHandle);
        }
        
        void resetB2d(int w, int h)
        {
            // Then we tie a BLImage to the same so
            // we can use both drawing APIs
            if (!fB2dImage.empty())
                fB2dImage.reset();

            BLResult br = fB2dImage.createFromData(static_cast<int>(w), static_cast<int>(h), BL_FORMAT_PRGB32, fFrameBufferData, fBytesPerRow);

            // Create a context for the image
            BLContextCreateInfo ctxInfo{};
            ctxInfo.threadCount = 4;

            fB2dContext.begin(fB2dImage, &ctxInfo);
        }
        
        void reset(int w, int h)
        {
            fB2dContext.end();
            
            resetGDIDC();
            resetGDIDibSection(w, h);
            resetB2d(w,h);
        }
    };
 
    // struct SwapChain
    // A structure consisting of a specified number of AFrameBuffer
    // rendering targets.  They can be swapped using the 'swap()' method.
    //
    struct ASwapChain {
        std::vector<std::unique_ptr<AFrameBuffer>> fBuffers{};

        size_t fNumBuffers{ 0 };
		size_t fFrontBufferIndex{ 0 };
        
        ASwapChain(size_t sz)
            : ASwapChain(10,10,sz)
        {}
        
        ASwapChain(int w, int h, size_t sz)
            : fNumBuffers(sz)
        {   
            reset(w, h);
        }

        void reset(int w, int h)
        {
            fBuffers.clear();
			for (size_t i = 0; i < fNumBuffers; i++)
			{
                auto fb = std::make_unique<AFrameBuffer>(w, h);
                
                fBuffers.push_back(std::move(fb));
			}
            
            fFrontBufferIndex = 0;
        }

        size_t swap()
        {
            fFrontBufferIndex = (fFrontBufferIndex + 1) % fNumBuffers;
            return fFrontBufferIndex;
        }
        
        AFrameBuffer* getNthBuffer(size_t n)
        {
			size_t realIndex = (fFrontBufferIndex + n) % fNumBuffers;
            return fBuffers[realIndex].get();
        }
        
		AFrameBuffer* getFrontBuffer()
		{
            return getNthBuffer(0);
		}
        
        AFrameBuffer* getNextBuffer()
        {
            return getNthBuffer(1);
        }
        
    };
    
    //
    // Note:  The ApplicationWindow should be double buffered
    // It should maintain a 'front buffer' and 'back buffer'
	// A call to 'paint', should simply blt the back buffer to the front buffer
    // and swap the two.
    // It should simply lock the back buffer then perform the blt
    //
    struct ApplicationWindow : public User32Window
    {
    private:
        // Thread management
        std::thread fThread;        // hold on to the thread that created us
        bool fIsRunning;
        StopWatch fAppClock;        // application clock
        
        ASwapChain fSwapChain{ 2 };
         
 
        
        int fWindowWidth{};
        int fWindowHeight{};
        
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
        Joystick gJoystick1{ JOYSTICKID1 };
        Joystick gJoystick2{ JOYSTICKID2 };
        
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

        
        
        static const char* getClassName()
        {
            static const char* CLASS_NAME = "window_threaded";
            return CLASS_NAME;
        }
        
        static void registerWindowClass()
        {
			static bool registered = false;

            if (registered)
                return;
            
            registered = true;
            
            
            WNDCLASSEXA wndClass{};      // data structure holding class information

            //unsigned int classStyle = CS_GLOBALCLASS | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
            unsigned int classStyle = CS_OWNDC | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
            const char* className = getClassName();

            memset(&wndClass, 0, sizeof(wndClass));
            wndClass.cbSize = sizeof(WNDCLASSEXA);
            wndClass.hInstance = GetModuleHandleA(NULL);
            wndClass.lpszClassName = getClassName();
            wndClass.lpfnWndProc = ApplicationWindow::ApplicationWindowProc;
            wndClass.style = classStyle;

            wndClass.hbrBackground = nullptr;
            wndClass.lpszMenuName = nullptr;

            wndClass.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
            wndClass.hIcon = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
            wndClass.hIconSm = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);


            ATOM classAtom = ::RegisterClassExA(&wndClass);

            if (classAtom == 0)
            {
                DWORD err = GetLastError();
                printf("Error registering window class: %d\n", err);
            }
        }
        



        
        //
        //    https://docs.microsoft.com/en-us/windows/desktop/inputdev/using-raw-input
        //
        // Raw input utility functions
        static constexpr USHORT HID_MOUSE = 2;
        static constexpr USHORT HID_JOYSTICK = 4;
        static constexpr USHORT HID_GAMEPAD = 5;
        static constexpr USHORT HID_KEYBOARD = 6;

        // Register for mouseand keyboard
        static bool HIDRegisterDevice(HWND hTarget, USHORT usage, USHORT usagePage = 1)
        {
            RAWINPUTDEVICE hid[1]{};

            hid[0].usUsagePage = usagePage;
            hid[0].usUsage = usage;
            hid[0].dwFlags = (RIDEV_DEVNOTIFY | RIDEV_INPUTSINK);
            hid[0].hwndTarget = hTarget;
            UINT uiNumDevices = 1;

            ::RegisterRawInputDevices(hid, uiNumDevices, sizeof(RAWINPUTDEVICE));

            return true;
        }
        
        static bool HID_UnregisterDevice(HWND hTarget, USHORT usage)
        {
            RAWINPUTDEVICE hid{ 0 };
            hid.usUsagePage = 1;
            hid.usUsage = usage;
            hid.dwFlags = RIDEV_REMOVE;
            hid.hwndTarget = hTarget;
            UINT uiNumDevices = 1;

            ::RegisterRawInputDevices(&hid, uiNumDevices, sizeof(RAWINPUTDEVICE));

            return true;
        }
        
    public:
        ApplicationWindow(int w, int h)
            :User32Window()
            ,fSwapChain(w,h,2)
            ,fWindowWidth(w)
            ,fWindowHeight(h)
            ,fIsRunning(false)
            
        {
            registerWindowClass();
        }
        
        virtual ~ApplicationWindow()
        {
            if (fThread.joinable()) {
                // Post a message telling the window to close
				// and wait for it to exit the message loop
                PostMessageA(fHandle, WM_CLOSE, 0, 0);
                fThread.join();
            }
            
        }
        
        AFrameBuffer* frontBuffer() {
            return fSwapChain.getFrontBuffer();
        }

        AFrameBuffer* backBuffer() {
            return fSwapChain.getNthBuffer(1);
        }

        
        // Common application API
        // Timing
        double seconds() const noexcept { return fAppClock.seconds();}
        double millis() const noexcept { return fAppClock.millis();}

        // Window management
        // Show the cursor, if there is one
        void showCursor()
        {
            ::ShowCursor(1);
        }


        // BUGBUG - we should be more robust here and
        // check to see if there's even a mouse attached
        // Then, decrement count enough times to make showing
        // the cursor absolute rather than relative.
        void hideCursor()
        {
            ::ShowCursor(0);
        }
        
        //===========================================================
        // Specific Message Handlers
        //===========================================================
        
        //
        //    Turn Windows mouse messages into mouse events which can
        //    be dispatched by the application.
        //
        LRESULT HandleMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam)
        {
            LRESULT res = 0;
            MouseEvent e{};

            // Coordinates relative to upper left corner of the screen?
            auto x = GET_X_LPARAM(lParam);
            auto y = GET_Y_LPARAM(lParam);

            e.x = (float)x;
            e.y = (float)y;
            e.control = (wParam & MK_CONTROL) != 0;
            e.shift = (wParam & MK_SHIFT) != 0;
            e.lbutton = (wParam & MK_LBUTTON) != 0;
            e.rbutton = (wParam & MK_RBUTTON) != 0;
            e.mbutton = (wParam & MK_MBUTTON) != 0;
            e.xbutton1 = (wParam & MK_XBUTTON1) != 0;
            e.xbutton2 = (wParam & MK_XBUTTON2) != 0;
            bool isPressed = e.lbutton || e.rbutton || e.mbutton || e.xbutton1 || e.xbutton2;

            switch (msg) {
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
                SetCapture(windowHandle());
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
                ::ScreenToClient(windowHandle(), &PT);

                e.x = (float)PT.x;
                e.y = (float)PT.y;
            }
                              break;

            case WM_MOUSEHWHEEL: {
                e.activity = MOUSEHWHEEL;
                auto delta = GET_WHEEL_DELTA_WPARAM(wParam);
                e.delta = (float)delta / (float)WHEEL_DELTA;

                POINT PT{};
                PT.x = x;
                PT.y = y;
                ::ScreenToClient(windowHandle(), &PT);

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
        
        
        LRESULT HandlePointerMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            LRESULT res = 0;

            PointerEvent e{};

            gPointerEventTopic.notify(e);

            return res;
        }

        
        LRESULT HandleKeyboardMessage(UINT msg, WPARAM wParam, LPARAM lParam)
        {
            LRESULT res = 0;

            keyCode = 0;
            keyChar = 0;

            KeyboardEvent e{};
            e.keyCode = (int)wParam;
            e.repeatCount = LOWORD(lParam);             // 0 - 15
            e.scanCode = ((lParam & 0xff0000) >> 16);   // 16 - 23
            e.isExtended = (lParam & 0x1000000UL) != 0; // 24
            e.wasDown = (lParam & 0x40000000UL) != 0;   // 30

            // Get the state of all keys on the keyboard
            ::GetKeyboardState(keyStates);

 
            switch (msg) {
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
// Handling the joystick messages through the Windows
// Messaging method is very limited.  It will only 
// trigger for a limited set of buttons and axes movements
// This handler is here for a complete API.  If the user app
// wants to get more out of the joystick, it can access the
// joystick directly with 'gJoystick1' or 'gJoystick2' and
// call getPosition() at any time.  Typically during 'update()'
// or 'draw()'
        LRESULT HandleJoystickMessage(UINT msg, WPARAM wParam, LPARAM lParam)
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
        

        
// Handling gestures
// Gesture messages WM_GESTURE, will only arrive if we're NOT
// registered for touch messages.  In an app, it might be useful
// to call noTouch() to ensure this is the case.
//
// https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/Win7Samples/Touch/MTGestures/cpp/GestureEngine.h
//
        LRESULT HandleGestureMessage(UINT msg, WPARAM wParam, LPARAM lParam)
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
                return DefWindowProc(windowHandle(), msg, wParam, lParam);
            }

            // If we are here, we can process well known gestures
            LRESULT res = 0;
            BOOL bHandled = FALSE;

            // Turn the physical screen location into local
            // window location
            POINT ptPoint{};
            ptPoint.x = gi.ptsLocation.x;
            ptPoint.y = gi.ptsLocation.y;
            ::ScreenToClient(windowHandle(), &ptPoint);

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

        
        LRESULT HandleRawInputMessage(UINT msg, WPARAM wParam, LPARAM lParam)
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


        
        LRESULT HandleTouchMessage(UINT msg, WPARAM wParam, LPARAM lParam)
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
                ::ScreenToClient(windowHandle(), &PT);

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


        LRESULT HandleFileDropMessage(UINT msg, WPARAM wParam, LPARAM lParam)
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

        


        virtual LRESULT handleWin32Message(UINT message, WPARAM wParam, LPARAM lParam)
        {
            LRESULT res = 0;
            
            switch (message)
            {
                case WM_PAINT: {
                    PAINTSTRUCT ps;

                    HDC hdc = BeginPaint(windowHandle(), &ps);

                    AFrameBuffer* frontBuffer = fSwapChain.getFrontBuffer();

                    if (frontBuffer) {
                        RECT rc;
                        GetClientRect(windowHandle(), &rc);

                        int w = rc.right - rc.left;
                        int h = rc.bottom - rc.top;

                        HDC memDC = frontBuffer->getGDIContext();
                        BitBlt(hdc, 0, 0, w, h,memDC, 0, 0, SRCCOPY);
                    }
                    EndPaint(windowHandle(),  &ps);
                }
                break;
                
                case WM_SIZE: {
                    // Resize framebuffer to new size
                    int w = LOWORD(lParam);
                    int h = HIWORD(lParam);
                    fSwapChain.reset(w, h);

					InvalidateRect(windowHandle(), nullptr, FALSE);
                    }
                    return 0;
                break;

                case WM_CREATE: {
                    // This is the earliest opportunity to do some window
                    // setup stuff, like framebuffer creation
                    RECT rc;
                    ::GetClientRect(windowHandle(), &rc);

                    int w = rc.right - rc.left;
					int h = rc.bottom - rc.top;
                    fSwapChain.reset(w, h);
                }
                break;
                    
                case WM_DESTROY:
                    // By doing a PostQuitMessage(), a 
                    // WM_QUIT message will eventually find its way into the
                    // message queue.
                    ::PostQuitMessage(0);
                    
                    return 0;
                break;
                
                case WM_DROPFILES:
                    return HandleFileDropMessage(message, wParam, lParam);
                break;
                    
                case WM_TOUCH:
                    return HandleTouchMessage(message, wParam, lParam);
                    break;

                case WM_GESTURE:
                    // we will only receive WM_GESTURE if not receiving WM_TOUCH
                    return HandleGestureMessage(message, wParam, lParam);
                    break;

                    
                default:
                {
                    if ((message >= WM_MOUSEFIRST) && (message <= WM_MOUSELAST)) {
                        // Handle all mouse messages
                        HandleMouseMessage(message, wParam, lParam);
                    }
                    else if ((message >= WM_KEYFIRST) && (message <= WM_KEYLAST)) {
                        // Handle all keyboard messages
                        HandleKeyboardMessage(message, wParam, lParam);
                    }
                    else if ((message >= MM_JOY1MOVE) && (message <= MM_JOY2BUTTONUP)) {
                        //printf("MM_JOYxxx: %p\n", gJoystickHandler);
                        //HandleJoystickMessage(message, wParam, lParam);
                    }
                    else {
                        return ::DefWindowProcA(windowHandle(), message, wParam, lParam);
                    }
                }
            }

            return res;
        }

        //
        // Sequence of messages upon startup
        //  WM_GETMINMAXINFO    - 0x0024
        //  WM_NCCREATE         - 0x0081
        //  WM_NCCALSSIZE       - 0x0083
        //  WM_CREATE           -- 0x0001
        //
        static LRESULT CALLBACK ApplicationWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            ApplicationWindow* pThis = nullptr;

            
            // WM_NCCREATE is pretty early in the window's creation process, so it's
            // here that we associate the pointer to the window to the window handle
            // through the GWLP_USERDATA property.
            if (msg == WM_NCCREATE) {
                CREATESTRUCTA* pCreate = reinterpret_cast<CREATESTRUCTA*>(lParam);
                pThis = reinterpret_cast<ApplicationWindow*>(pCreate->lpCreateParams);
                if (pThis) {
                    pThis->setWindowHandle(hwnd);

                    // Assign 'this' to the userdata of the window
                    // So we can access it during further message handling
                    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
                }
            }
            else {
                // For any other message, including WM_CREATE, we can reliably retrieve the
                // pointer to the window from the GWLP_USERDATA property.
                pThis = reinterpret_cast<ApplicationWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            }

            if (pThis)
            {
                // If we already have a pointer to the window object
                // let that handle the message
                return pThis->handleWin32Message(msg, wParam, lParam);
            }

			// If we don't have a pointer to the window object, let the default
			// window procedure handle the message
            return DefWindowProcA(hwnd, msg, wParam, lParam);

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
        
        void initWindow()
        {
            // Initialize COM
            HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            if (FAILED(hr)) {
                return;
            }

            // Create Direct2D Factory
            //hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pFactory);
            //if (FAILED(hr)) {
            //    CoUninitialize();
            //    return;
            //}



            DWORD winxstyle = WS_EX_APPWINDOW;
            DWORD winstyle = WS_OVERLAPPEDWINDOW;
            HMODULE hInst = ::GetModuleHandle(NULL);

            fHandle = CreateWindowExA(
                winxstyle,
                getClassName(),
                "window title",
                winstyle,
                CW_USEDEFAULT, CW_USEDEFAULT,
                fWindowWidth, fWindowHeight,
                NULL,
                NULL,
                hInst,
                this);


            // If the window handle was not created, then return
            // and end the thread
			if (fHandle == NULL) {
                int err = ::GetLastError();
                printf("FAIL CreateWindowExA: %d\n", err);
                CoUninitialize();
                
				return;
			}

            show();
            
            fIsRunning = true;
            
            MSG msg = {};
            while (GetMessage(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            // Out of message queue, end the thread
            CoUninitialize();
        }

        void run()
        {
            fThread = std::thread(&ApplicationWindow::initWindow, this);
        }

        void stop()
        {
            if (fThread.joinable()) {
                // Post a message telling the window to close
                // and wait for it to exit the message loop
                PostMessageA(windowHandle(), WM_CLOSE, 0, 0);
                fThread.join();
            }
        }

        void waitToFinish()
        {
            if (fThread.joinable())
                fThread.join();
        }
    };
    

}



