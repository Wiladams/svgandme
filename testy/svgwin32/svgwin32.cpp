#pragma comment(lib, "blend2d.lib")


#include "apphost.h"


#include "svg.h"
#include "fonthandler.h"
#include "appwindow.h"



using namespace waavs;

// Create one of these first, so factory constructor will run
SVGFactory gSVG;

FontHandler gFontHandler{};



// called once before main loop is running
void onLoad()
{
	printf("onLoad\n");

	// setup various subscriptions

	// resize app window
	createAppWindow(1024, 768, "SVG Win32");
}

