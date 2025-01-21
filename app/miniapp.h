#ifndef miniapp_h_included
#define miniapp_h_included

/*
	The primary benefit of the apphost.h/appmain.cpp combination is to turn
	windows specifics into simpler concepts.

	Typical Windows messages, such as mouse and keyboard, are turned into
	topics, that other modules can subscribe to.

	A 'main()' is provided, so a compiled application has a minimal runtime environment
	when it starts.  The user's app can implement a 'onLoad()' function,
	and not have to worry about whether it is a console or Window target.

	All other aspects of the application are up to the application environment,
	but at least all the Win32 specific stuff is wrapped up.
*/
#pragma comment(lib, "user32.lib")

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#include <windows.h>

#include <windowsx.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string>
#include <memory>


#include "definitions.h"


#if BUILD_AS_DLL
#define APP_API		__declspec(dllexport)
#else
//  #define APP_API		__declspec(dllimport)
#define APP_API
#endif

// APP_EXPORT 
// This is how we want to implement dynamically linked functions.
// Any functions the runtime is going to try and dynamically load MUST
// be decorated with 'APP_EXPORT'.
// That way, the compiled .dll will ensure to put the function pointer into
// the export section, such that subsequent 'GetProcAddress()' calls
// will find it, and the runtime can call it.

#define APP_EXPORT		__declspec(dllexport)


#define APP_EXTERN  extern



#include "blend2d.h"

#include "maths.h"
#include "pubsub.h"
#include "appwindow.h"
#include "uievent.h"
#include "joystick.h"
#include "network.h"
#include "user32pixelmap.h"






#ifdef __cplusplus
extern "C" {
#endif

	// These few function signatures are referred to by the runtime
	typedef void (*VOIDROUTINE)();
	typedef void (*PFNDOUBLE1)(const double param);
	typedef void (*PFNFLOAT1)(const float param);

	// Miscellaneous globals
	APP_EXPORT extern int gargc;
	APP_EXPORT extern char** gargv;
	APP_EXPORT extern unsigned int gSystemThreadCount;



	// Globals we expect the user to consume
	APP_EXPORT extern int rawPixelWidth;
	APP_EXPORT extern int rawPixelHeight;
	APP_EXPORT extern unsigned int physicalDpi;
	//APP_EXPORT extern unsigned int systemDpi;

	APP_EXPORT extern int canvasWidth;
	APP_EXPORT extern int canvasHeight;
	APP_EXPORT extern uint8_t* canvasPixelData;
	APP_EXPORT extern size_t canvasStride;




	// The following routines are part of the ndt runtime, and are called
	// by the implementing application.
	// 
	// The control the lifetime of the environment, creation of primary window
	// and whether various parts of the IO system are present
	APP_EXPORT waavs::User32PixelMap* appFrameBuffer();
	APP_EXPORT waavs::ApplicationWindow* getAppWindow();


	APP_EXPORT void createAppWindow(long aWidth, long aHeight, const char* title);
	APP_EXPORT void windowOpacity(float o);	// set overall opacity of window
	APP_EXPORT void showAppWindow();
	APP_EXPORT void halt();

	APP_EXPORT void frameRate(float newRate) noexcept;
	APP_EXPORT float getFrameRate() noexcept;
	APP_EXPORT size_t frameCount() noexcept;


	APP_EXPORT void refreshScreenNow();

	APP_EXPORT void layered();
	APP_EXPORT void noLayered();
	APP_EXPORT bool isLayered();

	APP_EXPORT void refreshKeyStates();

	// turn raw input on and off
	APP_EXPORT void rawInput();
	APP_EXPORT void noRawInput();

	// turn joystick processing on and off
	APP_EXPORT void joystick();
	APP_EXPORT void noJoystick();

	// Routines to manage ability to use
	// touch input
	APP_EXPORT bool touch();
	APP_EXPORT bool noTouch();
	APP_EXPORT bool isTouch();

	// gesture routines
	// nothing needed to turn on and off yet

	// Turn on/off file drop handling
	APP_EXPORT bool dropFiles();
	APP_EXPORT bool noDropFiles();




	APP_EXPORT void setCanvasPosition(int x, int y);
	APP_EXPORT bool setCanvasSize(long aWidth, long aHeight);



#ifdef __cplusplus
}
#endif

// The various 'onxxx' routines are meant to be implemented by
// user application code.  The general pattern is, 
// if a function is implemented then it will be called at the appropriate time.
// If the function is not implemented, then it simply won't be called.
//
// This makes for a nice 'fill in the blanks' kind of programming, where
// you just implement as much as you need, without being forced to 
// implement a bunch of boilerplate for no reason.
//

#ifdef __cplusplus
extern "C" {
#endif
	APP_EXPORT void onLoad();		// called once before main loop is running
	APP_EXPORT void onUnload();		// called once halt() occurs, and before app exits
	APP_EXPORT void onLoop();		// called each iteration of main application loop
#ifdef __cplusplus
}
#endif





APP_EXPORT void subscribe(SignalEventTopic::Subscriber s);
APP_EXPORT void subscribe(MouseEventTopic::Subscriber s);
APP_EXPORT void subscribe(KeyboardEventTopic::Subscriber s);
APP_EXPORT void subscribe(JoystickEventTopic::Subscriber s);
APP_EXPORT void subscribe(FileDropEventTopic::Subscriber s);
APP_EXPORT void subscribe(TouchEventTopic::Subscriber s);
APP_EXPORT void subscribe(GestureEventTopic::Subscriber s);
APP_EXPORT void subscribe(PointerEventTopic::Subscriber s);
APP_EXPORT void subscribe(FrameCountEventTopic::Subscriber s);
APP_EXPORT void subscribe(ResizeEventTopic::Subscriber s);

#endif	// miniapp_h_included


