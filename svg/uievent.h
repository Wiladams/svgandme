#pragma once

/*
    Data types for ui events
*/

#include <vector>
#include <string>



#include "maths.h"


namespace waavs {
    // Basic type to encapsulate a mouse event
    enum MouseActivityKind {
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
    enum KeyboardActivityKind {
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
    enum touchActivityKind {
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

    enum GestureActivityKind {
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
    enum JoystickActivityKind {
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
        double seconds;         // Relative to application start time
        uint64_t frameCount;    // Since application start
    };

    struct ResizeEvent
    {
		size_t width;
		size_t height;
	};

}



