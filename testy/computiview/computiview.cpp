// Put up a minimal UI drawing interaction environment
// allows you to test out drawing, with minimal framework overhead.
#include "computicle.h"




//
// onLoad
//
// Implement this if you want to do something when the app loads.
// You can setup event handlers, like to receive keyboard and mouse
// events, or frame events, which are sent at the frame rate specified
void onLoad()
{
	//printf("onLoad\n");
	//printf("Physical DPI: %d\n", physicalDpi);

	//setWindowTitle("QuickView");

	// use setFrameRate() to specify how often 
	// we want to receive frame events.  
	//subscribe(onFrameEvent);
}

//    Why on Earth are there two 'main' routines in here?
//    the first one 'main()' will be called when the code is compiled
//    as a CONSOLE subsystem.

//    the second one 'WinMain' will be called when the code is compiled
//    as a WINDOWS subsystem.  

//    By having both, we can control the startup in both cases.
//
static int ndtRun()
{
    return 0;
}



//    The 'main()' function is in here to ensure that compiling
//    this will result in an executable file.
//
//    The code for the user just needs to implement at least the 'onLoad()' function
//

// If we're compiled as a CONSOLE subsystem, then 'main()' will be called
int main(int argc, char** argv)
{
    return ndtRun();
}

// If we're compiled as a WINDOWS system app, then this will be called
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdLine, int nShowCmd)
{
    // BUGBUG, need to parse those command line arguments
    //gargc = argc;
    //gargv = argv;

    return ndtRun();
}
