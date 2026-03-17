#include "apphost.h"

using namespace waavs;

static void drawStuff()
{
	Surface newSurf(100, 100);
	newSurf.fillAll(0xFF00ffff);	// clear to blue
	getAppSurface()->blit(newSurf, 10, 10);

    refreshScreenNow();
}

static void onFrameEvent(const FrameCountEvent& fe)
{
	drawStuff();
}

void onLoad()
{
	//printf("onLoad\n");
	//printf("Physical DPI: %d\n", physicalDpi);
	subscribe(onFrameEvent);
}