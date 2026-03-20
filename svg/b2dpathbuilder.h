#pragma once

//
// svgpath.h 
// The SVGPath element contains a fairly complex set of commands in the 'd' attribute
// These commands are used to define the shape of the path, and can be fairly intricate.
// This file contains the code to parse the 'd' attribute and turn it into a BLPath object
//
// Usage:
// BLPath aPath{};
// parsePath("M 10 10 L 90 90", aPath);
//
// Noted:
// A convenient visualtion tool can be found here: https://svg-path-visualizer.netlify.app/
// References:
// https://svgwg.org/svg2-draft/paths.html#PathDataBNF



#include "pathsegment.h"
#include "pathprogramexec.h"
#include "pathprogramexec_b2d.h"

namespace waavs 
{


}





