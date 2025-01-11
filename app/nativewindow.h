#pragma once


/*
    This file, and the Window class represent the connection to the Win32 
    User32 interface library.  The idea is, if you include this single header
    in your application .cpp file, you have everything you need to create 
    a window of a given size, do drawing, keyboard and mouse handling.

    Notes:
    https://devblogs.microsoft.com/oldnewthing/20031013-00/?p=42193
*/


#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <stdio.h>
#include <cstring>
#include <cstdint>
#include <memory>

namespace waavs {
    // User32Window 
    // An instance of a User32WindowClass
    // Create one of these as a convenient way to manipulate a native window
    // 
    class User32Window {
    public:

        HWND fHandle{ nullptr };
        WNDCLASSEXA fClass{};
        bool fMouseInside = false;
        LONG fLastWindowStyle{};
        
        // Constructor taking an already allocated
        // window handle.  Use a WindowKind to allocate
        // an instance of a particular kind of window
        User32Window()
        {

        }
        //User32Window(HWND handle)
        //{
        //    fHandle = handle;
        //}

        // Virtual destructor so the window can be sub-classed
        virtual ~User32Window()
        {
            DestroyWindow(fHandle);
        }

        //HWND getHandle() const { return fHandle; }
        HWND windowHandle() const { return fHandle; }
		void setWindowHandle(HWND handle) { fHandle = handle; }
        
        // All the methods that are useful
        bool isValid() const { return fHandle != NULL; }

        // Hide the window
        void hide()
        {
            ::ShowWindow(fHandle, SW_HIDE);
        }

        // Show the window
        void show()
        {
            ::ShowWindow(fHandle, SW_SHOWNORMAL);
        }

        int getWidth()
        {
            RECT wRect;
            ::GetWindowRect(fHandle, &wRect);
            int cx = wRect.right - wRect.left;
            
            return cx;
        }

        int getHeight()
        {
            RECT wRect;
            ::GetWindowRect(fHandle, &wRect);
            int cy = wRect.bottom - wRect.top;

            return cy;
        }

        void moveTo(int x, int y)
        {
            RECT wRect;
            ::GetWindowRect(fHandle, &wRect);
            //int cx = wRect.right - wRect.left;
            //int cy = wRect.bottom - wRect.top;
            int flags = SWP_NOOWNERZORDER | SWP_NOSIZE;

            ::SetWindowPos(fHandle, (HWND)0, x, y, 0, 0, flags);
        }

        void setCanvasSize(long awidth, long aheight)
        {
            // Get current size of window
            RECT wRect;
            ::GetWindowRect(fHandle, &wRect);

            // Set the new size of the window based on the client area
            RECT cRect = { 0,0,awidth,aheight };
            ::AdjustWindowRect(&cRect, WS_OVERLAPPEDWINDOW, 1);

            //HWND hWndInsertAfter = (HWND)-1;
            HWND hWndInsertAfter = NULL;
            int X = wRect.left;
            int Y = wRect.top;
            int cx = cRect.right - cRect.left;
            int cy = cRect.bottom - cRect.top;
            UINT uFlags = 0;

            ::SetWindowPos(windowHandle(), hWndInsertAfter, X, Y, cx, cy, uFlags);

        }

        // Set the title of the window
        bool setTitle(const char* title)
        {
            if (title == nullptr)
                return false;

            BOOL res = ::SetWindowTextA(windowHandle(), title);
            
            return res != 0;
        }

        // Controlling a Window's style
        LONG addWindowStyle(int style)
        {
            return SetWindowLongA(fHandle, GWL_STYLE, GetWindowLongA(fHandle, GWL_EXSTYLE) | style);
        }

        // Set a style bit
        LONG setWindowStyle(int style)
        {
            auto res = ::SetWindowLongA(fHandle, GWL_STYLE, style);
            return res;
        }

        // Remove a specific style bit, or set of bits
        LONG removeWindowStyle(int style)
        {
            return SetWindowLongA(fHandle, GWL_STYLE, (~(LONG)style) & GetWindowLongA(fHandle, GWL_STYLE));
        }

        // Add a specific extended window style to
        // the ones that are already there
        LONG addExtendedStyle(LONG xstyle)
        {
            return SetWindowLongA(fHandle, GWL_EXSTYLE, GetWindowLongA(fHandle, GWL_EXSTYLE) | xstyle);
        }

        // Set a specific extended window style
        LONG setExtendedStyle(int xstyle)
        {
            return SetWindowLongA(fHandle, GWL_EXSTYLE, xstyle);
        }

        // clear a specific extended window style
        LONG removeExtendedStyle(int xstyle)
        {
            return SetWindowLongA(fHandle, GWL_EXSTYLE, (~(LONG)xstyle) & GetWindowLongA(fHandle, GWL_EXSTYLE));
        }

        //  Sets the opacity of a window
        // 0.0 - fully transparent
        // 1.0 - fully opaque
        //
        int setOpacity(double opacity)
        {
            LONG style = ::GetWindowLong(fHandle, GWL_EXSTYLE);

            if (opacity == 1.0f)
            {
                // if full opacity is desired
                // just mark as unlayered
                if (style & WS_EX_LAYERED)
                    removeExtendedStyle(WS_EX_LAYERED);
            }
            else {
                const BYTE alpha = (BYTE)((int)(opacity * 255.0f));
                // mark window as layered if necessary
                addExtendedStyle(WS_EX_LAYERED);

                ::SetLayeredWindowAttributes(fHandle, 0, alpha, LWA_ALPHA);
            }

            return 0;
        }

        bool setLayered(bool layered)
        {
            if (layered) {
                addExtendedStyle(WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP);
                fLastWindowStyle = setWindowStyle(WS_POPUP);
            }
            else {
                removeExtendedStyle(WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP);
                setWindowStyle(fLastWindowStyle);
            }
            
            // Call this to ensure Windows actually updates the attributes
            // before moving on
            ::SetWindowPos(fHandle, 0, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            
            return true;
        }
        
        int setNonLayered()
        {
            // Call this to ensure Windows actually updates the attributes
            // before moving on
            ::SetWindowPos(fHandle, 0, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }


    };


    // In Win32, a window 'class' must be registered
    // before you can use the CreateWindow call.  This
    // User32WindowClass object makes it easier to handle these
    // Window Classes, and do that registration.  As well,
    // it makes it relatively easy to create instances
    // of classes.
    struct User32WindowClass 
    {
    protected:
        WNDCLASSEXA fWndClass{};      // data structure holding class information
        bool fIsRegistered{};
        int fLastError{};

        char* fClassName{};       // this is only here to guarantee string sticks around
        uint16_t    fClassAtom{};

    public:
        User32WindowClass(const char* classOrAtom)
            : fIsRegistered(false)
            , fLastError(0)
            , fClassAtom(0)
        {
            // In this case, we're essentially doing a lookup
            // The classOrAtom is either a pointer to a string
            // or it's an unsigned uint16
            memset(&fWndClass, 0, sizeof(fWndClass));

            BOOL bResult = GetClassInfoExA(GetModuleHandleA(NULL), classOrAtom, &fWndClass);
            if (bResult == 0) {
                fLastError = GetLastError();
                return;
            }

            // if we got to here, the lookup worked
            // We know the window class is registered
            // We won't bother trying to get the atom as
            // it is of no practical use
            fIsRegistered = true;
        }

        User32WindowClass(const char* className, unsigned int classStyle, WNDPROC wndProc = nullptr)
            :fLastError(0),
            fClassAtom(0),
            fClassName(nullptr),
            fIsRegistered(false),
            fWndClass{ 0 }
        {
            if (className == nullptr) {
                return;
            }
            fClassName = _strdup(className);

            memset(&fWndClass, 0, sizeof(fWndClass));
            fWndClass.cbSize = sizeof(fWndClass);
            fWndClass.hInstance = GetModuleHandleA(NULL);
            fWndClass.lpszClassName = fClassName;
            fWndClass.lpfnWndProc = wndProc;
            fWndClass.style = classStyle;

            // The user might want to change any of these as they are not passed
            // into the constructor.
            fWndClass.hbrBackground = nullptr;
            fWndClass.hCursor = nullptr;        // LoadCursorA(nullptr, IDC_ARROW);
            fWndClass.lpszMenuName = nullptr;
            fWndClass.hIcon = nullptr;          // LoadIcon(nullptr, IDI_APPLICATION);
            fWndClass.hIconSm = nullptr;

            // Try to register the window class
            fClassAtom = ::RegisterClassExA(&fWndClass);

            if (fClassAtom == 0) {
                fLastError = GetLastError();
                return;
            }

            fIsRegistered = true;
        }

        bool isValid() const { return fIsRegistered; }

        int getLastError() const { return fLastError; }
        const char* getName() const { return fWndClass.lpszClassName; }

        virtual User32Window* createWindow(const char* title, int width, int height, int style = WS_OVERLAPPEDWINDOW, int xstyle = 0)
        {
            if (!isValid())
                return {};

            // Create an instance of the window object
            // We want to pass that to the window creation routine
            auto pWin = new User32Window();

            HMODULE hInst = fWndClass.hInstance;

            // Create the window handle
            int winxstyle = xstyle;
            int winstyle = style;

            //HMODULE hInst = GetModuleHandleA(NULL);
            HWND winHandle = CreateWindowExA(
                winxstyle,
                fWndClass.lpszClassName,
                title,
                winstyle,
                CW_USEDEFAULT, CW_USEDEFAULT,
                width, height,
                NULL,
                NULL,
                fWndClass.hInstance,
                pWin);

            if (winHandle == nullptr)
            {
                delete pWin;
                return {};
            }
            
            // store the pointer to the class inside the window handle
			//::SetWindowLongPtr(winHandle, GWLP_USERDATA, (LONG_PTR)win);
            
			return pWin;
        }
    };
}


