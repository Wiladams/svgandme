#pragma once

#include "apphost.h"
#include "svg.h"
#include "recorder.h"



APP_EXTERN waavs::Recorder gRecorder;

APP_EXPORT waavs::FontHandler & getFontHandler();

#ifdef __cplusplus
extern "C" {
#endif

APP_EXPORT bool loadDefaultFonts() noexcept;
APP_EXPORT bool loadFont(const char* filename, BLFontFace& ff) noexcept;
APP_EXPORT bool loadFontDirectory(const char* dir) noexcept;

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C" {
#endif
	// get fractions of seconds
	APP_EXPORT double millis() noexcept;
	APP_EXPORT double seconds() noexcept;


	
	
	APP_EXPORT void setup();		// called once before main loop is running

#ifdef __cplusplus
}
#endif