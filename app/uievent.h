#pragma once

/*
    Data types for ui events
*/

#include <vector>
#include <string>
#include <functional>

#include <joystickapi.h>

#include "maths.h"

// to include joystickapi 
#pragma comment(lib, "winmm.lib")

namespace waavs {
    // Basic type to encapsulate a mouse event
    enum {
        // These are based on regular events
        MOUSEMOVED,
        MOUSEPRESSED,
        MOUSERELEASED,
        MOUSEWHEEL,
        MOUSEHWHEEL,

        // These are based on application semantics
        MOUSECLICKED,
        MOUSEDOUBLECLICKED,
        MOUSEDRAGGED,

        MOUSEENTERED,
        MOUSEHOVER,         // like move, when we don't have focus
        MOUSEEXITED           // exited boundary
    };

    struct MouseEvent {
        int id;
        int activity;
        int button;     // Which button(s) action
        float x;
        float y;
        float delta;

        // This could easily be a bitset, and the original
        // Windows version is in fact that
        // So, what to create that will be easy, but not obscure
        // derived attributed
        bool control;
        bool shift;
        bool lbutton;
        bool rbutton;
        bool mbutton;
        bool xbutton1;
        bool xbutton2;
    };

    // Keyboard interactions
    enum {
        KEYPRESSED,
        KEYRELEASED,
        KEYTYPED
    };

    struct KeyboardEvent {
        int id;
        int activity;
        int keyCode;        // wparam
        int repeatCount;    // 0 - 15
        int scanCode;       // 16 - 23
        bool isExtended;    // 24
        bool wasDown;       // 30
    };

    // Touch interactions
    enum {
        TOUCH_DOWN,
        TOUCH_UP,
        TOUCH_MOVE,
        //    TOUCH_HOVER,
    };

    struct TouchEvent {
        int id;
        int activity;

        float x;
        float y;
        size_t w;
        size_t h;

        float rawX;
        float rawY;
        unsigned long rawWidth;
        unsigned long rawHeight;


        bool isMoving;
        bool isUp;
        bool isDown;
        bool isPrimary;
        bool isHovering;
        bool isPalm;
        bool isPen;
    };

    enum {
        GESTURE_BEGIN = 1,
        GESTURE_END = 2,
        GESTURE_ZOOM = 3,
        GESTURE_PAN = 4,
        GESTURE_ROTATE = 5,
        GESTURE_TWOFINGERTAP = 6,
        GESTURE_PRESSANDTAP = 7
    };

    struct GestureEvent {
        int activity;   // What kind of gesture
        long x;         // Primary location of gesture
        long y;

        bool isBegin;   // Attributes of vector
        bool isEnd;
        bool isInertia;

        // pan with inertia
        long inertiaX;
        long inertiaY;

        //
        long deltaX;    // movement in each direction
        long deltaY;

        long distance;  // distance between points
    };

    // Pointer interactions
    struct PointerEvent {
        int id;
        int activity;
        int x;
        int y;
    };

    // File drop interactions
    struct FileDropEvent {
        float x;
        float y;
        std::vector<std::string> filenames;
    };

    // For joystick activity
    enum {
        JOYPRESSED,
        JOYRELEASED,
        JOYMOVED,
        JOYZMOVED
    };

    struct JoystickEvent {
        unsigned int ID;
        int activity;			// What kind of joystick action is it
        int buttons;			// Bitfield of buttons being pressed
        int numButtonsPressed;	// total number of buttons currently pressed
        int flags;
        int POV;				// Value of hat switch, degrees

        // Axes values, return [-1..1]
        float x;
        float y;
        float z;
        float r;
        float u;
        float v;
    };

    struct FrameCountEvent
    {
        double seconds;
        uint64_t frameCount;
    };


    // A kind of mini pub-sub for specific events

    struct KeyboardEventDispatch
    {
        std::function<void(const KeyboardEvent& e)> keyPressed = nullptr;
        std::function<void(const KeyboardEvent& e)> keyReleased = nullptr;
        std::function<void(const KeyboardEvent& e)> keyTyped = nullptr;

        void operator()(const KeyboardEvent& e)
        {

            switch (e.activity)
            {
            case KEYPRESSED:
                if (keyPressed != nullptr)
                    keyPressed(e);
                break;

            case KEYRELEASED:
                if (keyReleased != nullptr)
                    keyReleased(e);
                break;

            case KEYTYPED:
                if (keyTyped != nullptr)
                    keyTyped(e);
                break;
            }

        }
    };

    // A functor for dispatching mouse events
    struct MouseEventDispatch
    {
        std::function<void(const MouseEvent& e)> mousePressed = nullptr;
        std::function<void(const MouseEvent& e)> mouseReleased = nullptr;
        std::function<void(const MouseEvent& e)> mouseClicked = nullptr;
        std::function<void(const MouseEvent& e)> mouseDragged = nullptr;
        std::function<void(const MouseEvent& e)> mouseMoved = nullptr;
        std::function<void(const MouseEvent& e)> mouseWheel = nullptr;
        std::function<void(const MouseEvent& e)> mouseHWheel = nullptr;

        void operator()(const MouseEvent& e)
        {
            switch (e.activity)
            {
            case MOUSEDRAGGED:
                if (mouseDragged != nullptr)
                    mouseDragged(e);
                break;

            case MOUSEPRESSED:
                if (mousePressed != nullptr)
                    mousePressed(e);
                break;

            case MOUSEMOVED:
                if (mouseMoved != nullptr)
                    mouseMoved(e);
                break;

            case MOUSERELEASED:
                if (mouseReleased != nullptr)
                    mouseReleased(e);
                break;

            case MOUSEWHEEL:
                if (mouseWheel != nullptr)
                    mouseWheel(e);
                break;

            case MOUSEHWHEEL:
                if (mouseHWheel != nullptr)
                    mouseHWheel(e);
                break;
            }

        }
    };

}


namespace waavs {
	struct Joystick {
		unsigned int fID;		// The joystick id
		JOYINFOEX fInfo{};
		JOYCAPSA fCaps{};
		bool fIsValid;
		HWND fAttachedWindow;

		static bool isValid(UINT id)
		{
			JOYCAPSA caps;
			MMRESULT res = joyGetDevCapsA(id, &caps, sizeof(caps));

			return res == JOYERR_NOERROR;
		}

		Joystick(unsigned int id)
			:fID(id),
			fInfo(),
			fIsValid(false),
			fAttachedWindow(nullptr)
		{
			fInfo.dwSize = sizeof(JOYINFOEX);
			fIsValid = getCaps();
		}

		Joystick()
			:Joystick(JOYSTICKID1)
		{}

		bool isValid() const { return fIsValid; }
		bool getCaps()
		{
			return  joyGetDevCapsA(fID, &fCaps, sizeof(fCaps)) == JOYERR_NOERROR;
		}
		const char* name() const { return fCaps.szPname; }
		const char* regKey() const { return fCaps.szRegKey; }
		const char* oemVxD() const { return fCaps.szOEMVxD; }
		size_t numButtons() const { return fCaps.wNumButtons; }
		size_t numAxes() const { return fCaps.wNumAxes; }

		bool hasZ() const { return 0 != (fCaps.wCaps & JOYCAPS_HASZ); }
		bool hasR() const { return 0 != (fCaps.wCaps & JOYCAPS_HASR); }
		bool hasU() const { return 0 != (fCaps.wCaps & JOYCAPS_HASU); }
		bool hasV() const { return 0 != (fCaps.wCaps & JOYCAPS_HASV); }

		bool hasPOV() const { return 0 != (fCaps.wCaps & JOYCAPS_HASPOV); }
		bool hasPOV4DIR() const { return 0 != (fCaps.wCaps & JOYCAPS_POV4DIR); }
		bool hasPOVCTS() const { return 0 != (fCaps.wCaps & JOYCAPS_POVCTS); }


		bool getPosition(JoystickEvent& res)
		{
			JOYINFOEX info{ 0 };
			info.dwSize = sizeof(info);
			info.dwFlags = JOY_RETURNALL;

			auto result = joyGetPosEx(fID, &info);
			if (result != JOYERR_NOERROR) {
				return false;
			}

			res.ID = fID;
			res.flags = info.dwFlags;

			// Map the relatively raw axes value 
			// to be within a range (-1.0 .. 1.0 typically)
			res.x = map(float(info.dwXpos), float(fCaps.wXmin), float(fCaps.wXmax), -1, 1);
			res.y = map(float(info.dwYpos), float(fCaps.wYmin), float(fCaps.wYmax), 1, -1);
			res.z = map(float(info.dwZpos), float(fCaps.wZmin), float(fCaps.wZmax), 1, 0); //throttle reverse
			res.r = map(float(info.dwRpos), (float)fCaps.wRmin, (float)fCaps.wRmax, -1, 1);
			res.u = map(float(info.dwUpos), (float)fCaps.wUmin, (float)fCaps.wUmax, -1, 1);
			res.v = map(float(info.dwVpos), (float)fCaps.wVmin, (float)fCaps.wVmax, -1, 1);

			res.buttons = info.dwButtons;
			res.numButtonsPressed = info.dwButtonNumber;

			if (info.dwPOV == 0xffff) {
				res.POV = -1;	// centered
			}
			else {
				res.POV = info.dwPOV / 100;
			}

			return true;
		}

		bool attachToWindow(HWND hWnd)
		{
			return (JOYERR_NOERROR == joySetCapture(hWnd, fID, 0, 1));
		}

		bool detachFromWindow()
		{
			return (JOYERR_NOERROR == joyReleaseCapture(fID));
		}
	};

}
