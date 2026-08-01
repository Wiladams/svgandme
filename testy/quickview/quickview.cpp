// Put up a minimal UI drawing interaction environment
// allows you to test out drawing, with minimal framework overhead.
#include "apphost.h"
#include "surface_draw.h"

using namespace waavs;

static void drawStuff()
{
	Surface newSurf(100, 100);
	wg_surface_fill(newSurf, 0xFF00ffff);	// clear to teal
	wg_blit_copy(*getAppSurface(), newSurf, 10, 10);

    refreshScreenNow();
}

static void printMouseEvent(const MouseEvent& me)
{
	printf("onMouseEvent: %d, %f, %f\n", me.activity, me.x, me.y);
	printf("  control: %d, shift: %d, lbutton: %d, rbutton: %d, mbutton: %d, xbutton1: %d, xbutton2: %d\n",
        me.control, me.shift, me.lbutton, me.rbutton, me.mbutton, me.xbutton1, me.xbutton2);

}

static void onMouseEvent(const MouseEvent& me)
{
	printf("onMouseEvent: %d, %f, %f\n", me.activity, me.x, me.y);
	printf("  control: %d, shift: %d, lbutton: %d, rbutton: %d, mbutton: %d, xbutton1: %d, xbutton2: %d\n",
		me.control, me.shift, me.lbutton, me.rbutton, me.mbutton, me.xbutton1, me.xbutton2);
    printf("  delta: %f\n", me.delta);

}

// onFrameEvent
//
// Called according to the frame rate specified in onLoad()
// 
static void onFrameEvent(const FrameCountEvent& fe)
{
	drawStuff();
}

static void onPenEvent(const PenEvent& pe)
{
	printf("onPenEvent: %d, %f, %f\n", pe.activity, pe.x, pe.y);
}

static void onTouchEvent(const TouchEvent& te)
{
	printf("onTouchEvent: %d, %f, %f\n", te.activity, te.x, te.y);
}

//
// onLoad
//
// Implement this if you want to do something when the app loads.
// You can setup event handlers, like to receive keyboard and mouse
// events, or frame events, which are sent at the frame rate specified
void onLoad()
{
	touch();
	//printf("onLoad\n");
	//printf("Physical DPI: %d\n", physicalDpi);

	setWindowTitle("QuickView");

	// use setFrameRate() to specify how often 
	// we want to receive frame events.  
	subscribe(onFrameEvent);
    subscribe(onPenEvent);
    subscribe(onTouchEvent);
    subscribe(onMouseEvent);
}