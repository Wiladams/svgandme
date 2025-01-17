#pragma once

//
// SVG Enum handling
//
// There are numerous enums in SVG, and we want to quickly convert between their
// textual representation, and their numeric value.  
// The WSEnum represents the enumeration as a map between the static text and the numeric value.
// We use ByteSpan as the key, and ByteSpanHash as the hashing function.  ByteSpan is a good choice, 
// because the literal text is fixed at compile time, so does not change during the duration
// of the program.  Using the ByteSpanHash, will make for a quick lookup.  This may or may not be
// faster than the typical if-else chain, but it's more flexible, and unifies
// around a single implementation, which can be changed in the future if a 
// faster way is found.
//
// In the cases where the enum values map directly to a blend2d equivalent, we use
// that value.  In other cases, we create a standard enum, and use those values.
// 
// There is a reverse lookup 'getEnumKey', where we can convert the numeric value
// back into the string representation.  This is useful for debugging.

#include <unordered_map>

#include "bspan.h"
#include "blend2d.h"
#include "wsenum.h"


namespace waavs {
	enum SpaceUnitsKind : uint32_t {
		SVG_SPACE_USER = 0,
		SVG_SPACE_OBJECT = 1,
		SVG_SPACE_STROKEWIDTH = 2
	};

	static WSEnum SVGSpaceUnits = {
		{ "userSpaceOnUse", SpaceUnitsKind::SVG_SPACE_USER },
		{ "objectBoundingBox", SpaceUnitsKind::SVG_SPACE_OBJECT }
	};
}

namespace waavs {
	enum AspectRatioAlignKind : uint32_t {
		SVG_ASPECT_RATIO_NONE = 0,
		SVG_ASPECT_RATIO_XMINYMIN = 1,
		SVG_ASPECT_RATIO_XMIDYMIN = 2,
		SVG_ASPECT_RATIO_XMAXYMIN = 3,
		SVG_ASPECT_RATIO_XMINYMID = 4,
		SVG_ASPECT_RATIO_XMIDYMID = 5,		// default
		SVG_ASPECT_RATIO_XMAXYMID = 6,
		SVG_ASPECT_RATIO_XMINYMAX = 7,
		SVG_ASPECT_RATIO_XMIDYMAX = 8,
		SVG_ASPECT_RATIO_XMAXYMAX = 9,
	};
	
	enum AspectRatioMeetOrSliceKind : uint32_t {
		SVG_ASPECT_RATIO_MEET = 10,
		SVG_ASPECT_RATIO_SLICE = 11
	};

	static WSEnum SVGAspectRatioAlignEnum = {
		{"none", AspectRatioAlignKind::SVG_ASPECT_RATIO_NONE},
		{"xMinYMin", AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMIN},
		{"xMidYMin", AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMIN},
		{"xMaxYMin", AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMIN},
		{"xMinYMid", AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMID},
		{"xMidYMid", AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMID},
		{"xMaxYMid", AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMID},
		{"xMinYMax", AspectRatioAlignKind::SVG_ASPECT_RATIO_XMINYMAX},
		{"xMidYMax", AspectRatioAlignKind::SVG_ASPECT_RATIO_XMIDYMAX},
		{"xMaxYMax", AspectRatioAlignKind::SVG_ASPECT_RATIO_XMAXYMAX},

	};

	static WSEnum SVGAspectRatioMeetOrSliceEnum = {
		{"meet", AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_MEET},
		{"slice", AspectRatioMeetOrSliceKind::SVG_ASPECT_RATIO_SLICE}
	};
}

namespace waavs {
	enum PaintOrderKind : uint32_t {
		SVG_PAINT_ORDER_NONE = 0,		// 00
		SVG_PAINT_ORDER_FILL = 1,		// 01
		SVG_PAINT_ORDER_STROKE = 2,		// 10
		SVG_PAINT_ORDER_MARKERS = 3,	// 11
		SVG_PAINT_ORDER_NORMAL = 57,	// 111001
	};

	static WSEnum SVGPaintOrderEnum = {
		{"fill", PaintOrderKind::SVG_PAINT_ORDER_FILL},
		{"stroke", PaintOrderKind::SVG_PAINT_ORDER_STROKE},
		{"markers", PaintOrderKind::SVG_PAINT_ORDER_MARKERS},
	};
}




namespace waavs {
	// Could be used as bitfield
	enum MarkerPosition : uint32_t {

		MARKER_POSITION_START = 0,
		MARKER_POSITION_MIDDLE = 1,
		MARKER_POSITION_END = 2,
		//MARKER_POSITION_ALL = 3
	};

	// determines the orientation of a marker
	enum MarkerOrientation : uint32_t
	{
		MARKER_ORIENT_AUTO,
		MARKER_ORIENT_AUTOSTARTREVERSE,
		MARKER_ORIENT_ANGLE,
		MARKER_ORIENT_INVALID = 255,
	};


	static WSEnum MarkerOrientationEnum = {
		{"auto", MarkerOrientation::MARKER_ORIENT_AUTO},
		{"auto-start-reverse", MarkerOrientation::MARKER_ORIENT_AUTOSTARTREVERSE},
	};



	static WSEnum MarkerUnitEnum = {
		{"strokeWidth", SpaceUnitsKind::SVG_SPACE_STROKEWIDTH},
		{"userSpaceOnUse", SpaceUnitsKind::SVG_SPACE_USER},
	};
}

namespace waavs {
	// Represents which of the many different ways you can specify
	// the size of a font is being used.
	enum SVGSizeKind : uint32_t
	{
		SVG_SIZE_KIND_INVALID = 0,
		SVG_SIZE_KIND_ABSOLUTE = 1,
		SVG_SIZE_KIND_RELATIVE = 2,
		SVG_SIZE_KIND_LENGTH = 3,
		SVG_SIZE_KIND_PERCENTAGE = 4,
		SVG_SIZE_KIND_MATH = 5,
		SVG_SIZE_KIND_GLOBAL = 6,
	};

	// SVG_SIZE_KIND_ABSOLUTE
	enum SVGSizeAbsoluteKind : uint32_t
	{
		SVG_SIZE_ABSOLUTE_XX_SMALL = 1,
		SVG_SIZE_ABSOLUTE_X_SMALL = 2,
		SVG_SIZE_ABSOLUTE_SMALL = 3,
		SVG_SIZE_ABSOLUTE_MEDIUM = 4,
		SVG_SIZE_ABSOLUTE_LARGE = 5,
		SVG_SIZE_ABSOLUTE_X_LARGE = 6,
		SVG_SIZE_ABSOLUTE_XX_LARGE = 7,
		SVG_SIZE_ABSOLUTE_XXX_LARGE = 8,
	};

	static WSEnum SVGSizeAbsoluteEnum = {
	{"xx-small", SVGSizeAbsoluteKind::SVG_SIZE_ABSOLUTE_XX_SMALL},
	{"x-small", SVGSizeAbsoluteKind::SVG_SIZE_ABSOLUTE_X_SMALL},
	{"small", SVGSizeAbsoluteKind::SVG_SIZE_ABSOLUTE_SMALL},
	{"medium", SVGSizeAbsoluteKind::SVG_SIZE_ABSOLUTE_MEDIUM},
	{"large", SVGSizeAbsoluteKind::SVG_SIZE_ABSOLUTE_LARGE},
	{"x-large", SVGSizeAbsoluteKind::SVG_SIZE_ABSOLUTE_X_LARGE},
	{"xx-large", SVGSizeAbsoluteKind::SVG_SIZE_ABSOLUTE_XX_LARGE},
	{"xxx-large", SVGSizeAbsoluteKind::SVG_SIZE_ABSOLUTE_XXX_LARGE},
	};

	enum SVGSizeRelativeKind : uint32_t
	{
		SVG_SIZE_RELATIVE_LARGER = 1,
		SVG_SIZE_RELATIVE_SMALLER = 2,
	};

	static WSEnum SVGSizeRelativeEnum = {
		{"larger", SVGSizeRelativeKind::SVG_SIZE_RELATIVE_LARGER},
		{"smaller", SVGSizeRelativeKind::SVG_SIZE_RELATIVE_SMALLER},
	};

	// SVG_SIZE_KIND_LENGTH
	enum SVGLengthKind : uint32_t {
		SVG_LENGTHTYPE_UNKNOWN = 0,
		SVG_LENGTHTYPE_NUMBER = 1,
		SVG_LENGTHTYPE_PERCENTAGE = 2,
		SVG_LENGTHTYPE_EMS = 3,
		SVG_LENGTHTYPE_EXS = 4,
		SVG_LENGTHTYPE_PX = 5,
		SVG_LENGTHTYPE_CM = 6,
		SVG_LENGTHTYPE_MM = 7,
		SVG_LENGTHTYPE_IN = 8,
		SVG_LENGTHTYPE_PT = 9,
		SVG_LENGTHTYPE_PC = 10,
	};

	static WSEnum SVGDimensionEnum = {
		{"",SVG_LENGTHTYPE_NUMBER },
		{"px", SVG_LENGTHTYPE_PX},
		{"pt", SVG_LENGTHTYPE_PT},
		{"pc", SVG_LENGTHTYPE_PC},
		{"mm", SVG_LENGTHTYPE_MM},
		{"cm", SVG_LENGTHTYPE_CM},
		{"in", SVG_LENGTHTYPE_IN},
		{"%",  SVG_LENGTHTYPE_PERCENTAGE},
		{"em", SVG_LENGTHTYPE_EMS},
		{"ex", SVG_LENGTHTYPE_EXS}
	};


}

namespace waavs {
    // Text Alignment
    enum TXTALIGNMENT : uint32_t
    {
        CENTER = 0x01,

        LEFT = 0x02,
        RIGHT = 0x04,

        TOP = 0x10,
        BASELINE = 0x20,
        BOTTOM = 0x40,
        MIDLINE = 0x80,

    };

	enum SVGAlignment : uint32_t {
		SVG_ALIGNMENT_NONE		= 0x00,
		SVG_ALIGNMENT_START		= 0x02,
		SVG_ALIGNMENT_MIDDLE	= 0x01,
		SVG_ALIGNMENT_END		= 0x04,
	};
	
	static WSEnum SVGTextAnchor = {
		{ "start", SVG_ALIGNMENT_START },
		{ "middle", SVG_ALIGNMENT_MIDDLE },
		{ "end", SVG_ALIGNMENT_END }
	};

    static WSEnum SVGTextAlign = {
        { "start", (int)SVG_ALIGNMENT_START },
        { "middle", (int)SVG_ALIGNMENT_MIDDLE },
        { "end", (int)SVG_ALIGNMENT_END }
    };
    

	// Dominant Baseline
    enum DOMINANTBASELINE : uint32_t
    {
        AUTO,
        ALPHABETIC,
        CENTRAL,
        HANGING,
        IDEOGRAPHIC,
        MATHEMATICAL,
        MIDDLE,
        NO_CHANGE,
        RESET_SIZE,
        TEXT_AFTER_EDGE,
        TEXT_BEFORE_EDGE,
        TEXT_BOTTOM,
        TEXT_TOP,
        USE_SCRIPT,
    };

	static WSEnum SVGDominantBaseline = {
		{ "auto", DOMINANTBASELINE::AUTO },
		{ "alphabetic", DOMINANTBASELINE::ALPHABETIC },
		{ "central", DOMINANTBASELINE::CENTRAL },
		{ "hanging", DOMINANTBASELINE::HANGING },
		{ "ideographic", DOMINANTBASELINE::IDEOGRAPHIC },
		{ "mathematical", DOMINANTBASELINE::MATHEMATICAL },
		{ "middle", DOMINANTBASELINE::MIDDLE },
		{ "no-change", DOMINANTBASELINE::NO_CHANGE },
		{ "reset-size", DOMINANTBASELINE::RESET_SIZE },
		{ "text-after-edge", DOMINANTBASELINE::TEXT_AFTER_EDGE },
		{ "text-before-edge", DOMINANTBASELINE::TEXT_BEFORE_EDGE },
		{ "text-bottom", DOMINANTBASELINE::TEXT_BOTTOM },
		{ "text-top", DOMINANTBASELINE::TEXT_TOP },
		{ "use-script", DOMINANTBASELINE::USE_SCRIPT }
	};
    
	static WSEnum SVGFontWeight = {
		{ "100", BL_FONT_WEIGHT_THIN },
		{ "200", BL_FONT_WEIGHT_EXTRA_LIGHT },
		{ "300", BL_FONT_WEIGHT_LIGHT },
		{ "400", BL_FONT_WEIGHT_NORMAL },
        {"normal", BL_FONT_WEIGHT_NORMAL},
		{ "500", BL_FONT_WEIGHT_MEDIUM },
		{ "600", BL_FONT_WEIGHT_SEMI_BOLD },
        {"bold", BL_FONT_WEIGHT_BOLD},
		{ "700", BL_FONT_WEIGHT_BOLD },
		{ "800", BL_FONT_WEIGHT_SEMI_BOLD },
		{ "900", BL_FONT_WEIGHT_EXTRA_BOLD },
		{ "1000", BL_FONT_WEIGHT_BLACK }
	};
    
    static WSEnum SVGFontStretch = {
		{"condensed", BL_FONT_STRETCH_CONDENSED},
		{"expanded", BL_FONT_STRETCH_EXPANDED},
		{"extra-condensed", BL_FONT_STRETCH_EXTRA_CONDENSED},
		{"extra-expanded", BL_FONT_STRETCH_EXTRA_EXPANDED},
		{"normal", BL_FONT_STRETCH_NORMAL},
		{"semi-condensed", BL_FONT_STRETCH_SEMI_CONDENSED},
		{"semi-expanded", BL_FONT_STRETCH_SEMI_EXPANDED},
		{"ultra-condensed", BL_FONT_STRETCH_ULTRA_CONDENSED},
		{"ultra-expanded", BL_FONT_STRETCH_ULTRA_EXPANDED}
    };
    
	static WSEnum SVGFontStyle = {
		{"normal", BL_FONT_STYLE_NORMAL},
		{"italic", BL_FONT_STYLE_ITALIC},
		{"oblique", BL_FONT_STYLE_OBLIQUE}
	};
    
}

namespace waavs {
	static WSEnum SVGLineCaps = {
		{ "butt", BL_STROKE_CAP_BUTT },
		{ "round", BL_STROKE_CAP_ROUND },
		{ "square", BL_STROKE_CAP_SQUARE },

        // blend2d specific extensions
        {"round-reverse", BL_STROKE_CAP_ROUND_REV},
        {"triangle", BL_STROKE_CAP_TRIANGLE},
        {"triangle-reverse", BL_STROKE_CAP_TRIANGLE_REV},
	};


	static WSEnum SVGLineJoin = {
		{ "miter", BL_STROKE_JOIN_MITER_BEVEL },
		{ "round", BL_STROKE_JOIN_ROUND },
		{ "bevel", BL_STROKE_JOIN_BEVEL },

		// blend2d specific extensions
        {"miter-clip",BL_STROKE_JOIN_MITER_CLIP },

	};
    

}

namespace waavs {
    enum VectorEffectKind : uint32_t {
        VECTOR_EFFECT_NONE,
        VECTOR_EFFECT_NON_SCALING_STROKE,
        VECTOR_EFFECT_NON_SCALING_SIZE,
        VECTOR_EFFECT_NON_ROTATION,
        VECTOR_EFFECT_FIXED_POSITION,
    };

	static WSEnum SVGVectorEffect = {
		{ "none", VECTOR_EFFECT_NONE },
		{ "non-scaling-stroke", VECTOR_EFFECT_NON_SCALING_STROKE },
		{ "non-scaling-size", VECTOR_EFFECT_NON_SCALING_SIZE },
		{ "non-rotation", VECTOR_EFFECT_NON_ROTATION },
		{ "fixed-position", VECTOR_EFFECT_FIXED_POSITION },
	};
    

}

namespace waavs {

	static WSEnum SVGFillRule = {
		{ "nonzero", BL_FILL_RULE_NON_ZERO },
		{ "evenodd", BL_FILL_RULE_EVEN_ODD },
	};




}

// Parsing spreadMethod, which is applied to the 
// ExtendMode of the gradient
namespace waavs {
	static WSEnum SVGSpreadMethod = {
		{ "pad", BL_EXTEND_MODE_PAD },
		{ "reflect", BL_EXTEND_MODE_REFLECT },
		{ "repeat", BL_EXTEND_MODE_REPEAT },
	};

	static WSEnum SVGExtendMode = {
		{"pad", BL_EXTEND_MODE_PAD},
		{"reflect", BL_EXTEND_MODE_REFLECT},
		{"repeat", BL_EXTEND_MODE_REPEAT},

		// blend2d specific
		{"pad-x-pad-y", BL_EXTEND_MODE_PAD_X_PAD_Y},
		{"pad-x-repeat-y", BL_EXTEND_MODE_PAD_X_REPEAT_Y},
		{"pad-x-reflect-y", BL_EXTEND_MODE_PAD_X_REFLECT_Y},
		
		{"repeat-x-pad-y", BL_EXTEND_MODE_REPEAT_X_PAD_Y},
		{"repeat-x-repeat-y", BL_EXTEND_MODE_REPEAT_X_REPEAT_Y},
		{"repeat-x-reflect-y", BL_EXTEND_MODE_REPEAT_X_REFLECT_Y},
		
		{"reflect-x-repeat-y", BL_EXTEND_MODE_REFLECT_X_REPEAT_Y},
		{"reflect-x-reflect-y", BL_EXTEND_MODE_REFLECT_X_REFLECT_Y},
		{"reflect-x-pad-y", BL_EXTEND_MODE_REFLECT_X_PAD_Y},
	};
	

}

