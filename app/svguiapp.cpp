#pragma comment(lib, "blend2d.lib")



#include <filesystem>

#include "svguiapp.h"
#include "stopwatch.h"



using namespace waavs;

static StopWatch gSVGAppClock;


double seconds() noexcept
{
	return gSVGAppClock.seconds();
}

double millis() noexcept
{
	// get millis from p5 stopwatch
	return gSVGAppClock.millis();
}


waavs::FontHandler& getFontHandler() {
	static waavs::FontHandler fh;
	return fh;
}


APP_EXTERN waavs::Recorder gRecorder{ nullptr };

static VOIDROUTINE gSetupHandler = nullptr;



// Typography
bool loadFont(const char* filename, BLFontFace& ff) noexcept
{
	bool success = getFontHandler().loadFontFace(filename, ff);
	return success;
}

bool loadFontDirectory(const char* dir) noexcept
{
	const std::filesystem::path fontPath(dir);

	// If the path does not exist
	// return immediately
	if (!std::filesystem::exists(fontPath))
	{
		return false;
	}

	if (fontPath.empty())
	{
		return false;
	}

	for (const auto& dir_entry : std::filesystem::directory_iterator(fontPath))
	{
		if (dir_entry.is_regular_file())
		{
			if ((dir_entry.path().extension() == ".ttf") ||
				(dir_entry.path().extension() == ".otf") ||
				(dir_entry.path().extension() == ".TTF"))
			{
				BLFontFace ff{};
				bool success = getFontHandler().loadFontFace(dir_entry.path().generic_string().c_str(), ff);
			}
		}
	}

	return true;
}

bool loadDefaultFonts() noexcept
{
	// Load in some fonts to start
	std::vector<const char*> fontNames{
		"c:\\Windows\\Fonts\\arial.ttf",
		"c:\\Windows\\Fonts\\calibri.ttf",
		"c:\\Windows\\Fonts\\cascadiacode.ttf",
		"c:\\Windows\\Fonts\\consola.ttf",
		"c:\\Windows\\Fonts\\cour.ttf",
		"c:\\Windows\\Fonts\\gothic.ttf",
		"c:\\Windows\\Fonts\\segoui.ttf",
		"c:\\Windows\\Fonts\\tahoma.ttf",
		"c:\\Windows\\Fonts\\terminal.ttf",
		"c:\\Windows\\Fonts\\times.ttf",
		"c:\\Windows\\Fonts\\verdana.ttf",
		"c:\\Windows\\Fonts\\wingding.ttf"
	};
	return getFontHandler().loadFonts(fontNames);
}



static void registerAppHandlers()
{
	// we're going to look within our own module
	// to find handler functions.  This is because the user's application should
	// be compiled with the application, so the exported functions should
	// be attainable using 'GetProcAddress()'

	HMODULE hInst = ::GetModuleHandleA(NULL);


	// Get the general app routines
	// setup()
	gSetupHandler = (VOIDROUTINE)::GetProcAddress(hInst, "setup");

}

void onLoad()
{
	//printf("onLoad\n");
	//printf("Physical DPI: %d\n", physicalDpi);
	
	registerAppHandlers();

	// if setup() exists, call that
	if (gSetupHandler != nullptr)
		gSetupHandler();

	// Refresh the screen at least once
	refreshScreenNow();
}