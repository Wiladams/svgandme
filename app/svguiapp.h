#pragma once

#include "apphost.h"
#include "svg.h"
#include "recorder.h"


APP_EXTERN waavs::FontHandler gFontHandler;
APP_EXTERN waavs::Recorder gRecorder;

bool loadDefaultFonts() noexcept;
bool loadFont(const char* filename, BLFontFace& ff) noexcept;
bool loadFontDirectory(const char* dir) noexcept;

