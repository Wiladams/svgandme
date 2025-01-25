#pragma comment(lib, "blend2d.lib")


#include "apphost.h"


#include "svg.h"
#include "fonthandler.h"
#include "appwindow.h"


using namespace waavs;


FontHandler gFontHandler{};

// Create a routine to respond to frameevents
static void onFrameEvent(const FrameCountEvent& fe)
{
	// Create some objects
	
	refreshScreenNow();
}



// called once before main loop is running
void onLoad()
{
	printf("onLoad\n");

	// setup various subscriptions

	// resize app window
	createAppWindow(1024, 768, "SVG Win32");

	subscribe(onFrameEvent);
}

