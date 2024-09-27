#pragma once

//
// SVG Enumerator handling
//
// There are numerous enums in SVG, and we want to quickly convert between their
// textual representation, and their numeric value.  
// The SVGEnum represents the enumeration as a map between the static text and the numeric value.
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

namespace waavs {
	using SVGEnum = std::unordered_map<ByteSpan, uint32_t, ByteSpanHash, ByteSpanEquivalent>;

	static INLINE bool getEnumValue(SVGEnum enumMap, const ByteSpan& key, uint32_t & value) noexcept
	{
		auto it = enumMap.find(key);
		if (it == enumMap.end())
			return false;
		value = it->second;
		return true;
	}

	// Return the key that corresponds to the specified value
	static INLINE bool getEnumKey(SVGEnum enumMap, uint32_t value, ByteSpan& key) noexcept
	{
		for (auto it = enumMap.begin(); it != enumMap.end(); ++it)
		{
			if (it->second == value)
			{
				key = it->first;
				return true;
			}
		}
		return false;
	}
}

namespace waavs {
	enum SpaceUnitsKind : uint32_t {
		SVG_SPACE_USER = 0,
		SVG_SPACE_OBJECT = 1
	};
	
	static SVGEnum SVGSpaceUnits = {
		{ "userSpaceOnUse", SpaceUnitsKind::SVG_SPACE_USER },
		{ "objectBoundingBox", SpaceUnitsKind::SVG_SPACE_OBJECT }
	};
	
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
	
	static SVGEnum SVGDimensionEnum = {
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
		SVG_ALIGNMENT_START = 0x02,
		SVG_ALIGNMENT_MIDDLE = 0x01,
		SVG_ALIGNMENT_END = 0x04,
	};
	
	static SVGEnum SVGTextAnchor = {
		{ "start", SVG_ALIGNMENT_START },
		{ "middle", SVG_ALIGNMENT_MIDDLE },
		{ "end", SVG_ALIGNMENT_END }
	};

    static SVGEnum SVGTextAlign = {
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

	static SVGEnum SVGDominantBaseline = {
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
    
	static SVGEnum SVGFontWeight = {
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
    
    static SVGEnum SVGFontStretch = {
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
    
	static SVGEnum SVGFontStyle = {
		{"normal", BL_FONT_STYLE_NORMAL},
		{"italic", BL_FONT_STYLE_ITALIC},
		{"oblique", BL_FONT_STYLE_OBLIQUE}
	};
    
}

namespace waavs {
	static SVGEnum SVGLineCaps = {
		{ "butt", BL_STROKE_CAP_BUTT },
		{ "round", BL_STROKE_CAP_ROUND },
		{ "square", BL_STROKE_CAP_SQUARE },

        // blend2d specific extensions
        {"round-reverse", BL_STROKE_CAP_ROUND_REV},
        {"triangle", BL_STROKE_CAP_TRIANGLE},
        {"triangle-reverse", BL_STROKE_CAP_TRIANGLE_REV},
	};


	static SVGEnum SVGLineJoin = {
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

	static SVGEnum SVGVectorEffect = {
		{ "none", VECTOR_EFFECT_NONE },
		{ "non-scaling-stroke", VECTOR_EFFECT_NON_SCALING_STROKE },
		{ "non-scaling-size", VECTOR_EFFECT_NON_SCALING_SIZE },
		{ "non-rotation", VECTOR_EFFECT_NON_ROTATION },
		{ "fixed-position", VECTOR_EFFECT_FIXED_POSITION },
	};
    

}

namespace waavs {

	static SVGEnum SVGFillRule = {
		{ "nonzero", BL_FILL_RULE_NON_ZERO },
		{ "evenodd", BL_FILL_RULE_EVEN_ODD },
	};




}

// Parsing spreadMethod, which is applied to the 
// ExtendMode of the gradient
namespace waavs {
	static SVGEnum SVGSpreadMethod = {
		{ "pad", BL_EXTEND_MODE_PAD },
		{ "reflect", BL_EXTEND_MODE_REFLECT },
		{ "repeat", BL_EXTEND_MODE_REPEAT },
	};

	static SVGEnum SVGExtendMode = {
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

