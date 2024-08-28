#pragma once

#include "apphost.h"
#include "svg.h"
#include "recorder.h"


APP_EXTERN waavs::FontHandler gFontHandler;
APP_EXTERN waavs::Recorder gRecorder;

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
	APP_EXPORT void setup();		// called once before main loop is running

#ifdef __cplusplus
}
#endif