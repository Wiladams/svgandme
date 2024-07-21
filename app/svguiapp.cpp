#pragma comment(lib, "blend2d.lib")



#include <filesystem>

#include "svguiapp.h"




using namespace waavs;



waavs::FontHandler gFontHandler{};
APP_EXTERN waavs::Recorder gRecorder{ nullptr };

// Create one of these first, so factory constructor will run
SVGFactory gSVG;


// Typography
static bool loadFont(const char* filename, BLFontFace& ff) noexcept
{
	bool success = gFontHandler.loadFontFace(filename, ff);
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
			if (endsWith(dir_entry.path().generic_string(), ".ttf") ||
				endsWith(dir_entry.path().generic_string(), ".TTF") ||
				endsWith(dir_entry.path().generic_string(), ".otf"))
			{
				BLFontFace ff{};
				bool success = gFontHandler.loadFontFace(dir_entry.path().generic_string().c_str(), ff);
			}
		}
	}

	return true;
}

static bool loadDefaultFonts() noexcept
{
	gFontHandler.loadDefaultFonts();

	return true;
}

static bool loadFontFiles(std::vector<const char*> filenames)
{
	gFontHandler.loadFonts(filenames);

	return true;
}

