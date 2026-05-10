// Put up a minimal UI drawing interaction environment
// allows you to test out drawing, with minimal framework overhead.
#include "apphost.h"

using namespace waavs;

static void drawStuff()
{
	Surface newSurf(100, 100);
	newSurf.fillAll(0xFF00ffff);	// clear to blue
	getAppSurface()->blit(newSurf, 10, 10);

    refreshScreenNow();
}

// onFrameEvent
//
// Called according to the frame rate specified in onLoad()
// 
static void onFrameEvent(const FrameCountEvent& fe)
{
	drawStuff();
}

//
// onLoad
//
// Implement this if you want to do something when the app loads.
// You can setup event handlers, like to receive keyboard and mouse
// events, or frame events, which are sent at the frame rate specified
void onLoad()
{

	// use setFrameRate() to specify how often we want to receive frame events.  
	//printf("onLoad\n");
	//printf("Physical DPI: %d\n", physicalDpi);
	subscribe(onFrameEvent);
}