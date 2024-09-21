#pragma once

//
// SVG Enumerator handling
//
// There are numerous enums in SVG, and we want to quickly convert between their
// textual representation, and their numeric value.  The SVGEnum represents
// the enumeration as a map between the string and the numeric value.
// We use ByteSpan as the key, and ByteSpanHash as the hashing function.  This will
// turn the lookup into a hash conversion, and quick lookup.  This may or may not be
// faster than the typical if-else chain, but it's a bit more flexible, and unifies
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
	using SVGEnum = std::unordered_map<ByteSpan, int, ByteSpanHash>;

	static INLINE bool getEnumValue(SVGEnum enumMap, const ByteSpan& key, int& value) noexcept
	{
		auto it = enumMap.find(key);
		if (it == enumMap.end())
			return false;
		value = it->second;
		return true;
	}

	// Return the key that corresponds to the specified value
	static INLINE bool getEnumKey(SVGEnum enumMap, int value, ByteSpan& key) noexcept
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
	
    // Text Alignment
    enum ALIGNMENT : int
    {
        CENTER = 0x01,

        LEFT = 0x02,
        RIGHT = 0x04,

        TOP = 0x10,
        BASELINE = 0x20,
        BOTTOM = 0x40,
        MIDLINE = 0x80,

    };

	static SVGEnum SVGTextAnchor = {
		{ "start", (int)ALIGNMENT::LEFT },
		{ "middle", (int)ALIGNMENT::CENTER },
		{ "end", (int)ALIGNMENT::RIGHT }
	};

    static SVGEnum SVGTextAlign = {
        { "start", (int)ALIGNMENT::LEFT },
        { "middle", (int)ALIGNMENT::CENTER },
        { "end", (int)ALIGNMENT::RIGHT }
    };
    

	// Dominant Baseline
    enum class DOMINANTBASELINE : int
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
		{ "auto", (int)DOMINANTBASELINE::AUTO },
		{ "alphabetic", (int)DOMINANTBASELINE::ALPHABETIC },
		{ "central", (int)DOMINANTBASELINE::CENTRAL },
		{ "hanging", (int)DOMINANTBASELINE::HANGING },
		{ "ideographic", (int)DOMINANTBASELINE::IDEOGRAPHIC },
		{ "mathematical", (int)DOMINANTBASELINE::MATHEMATICAL },
		{ "middle", (int)DOMINANTBASELINE::MIDDLE },
		{ "no-change", (int)DOMINANTBASELINE::NO_CHANGE },
		{ "reset-size", (int)DOMINANTBASELINE::RESET_SIZE },
		{ "text-after-edge", (int)DOMINANTBASELINE::TEXT_AFTER_EDGE },
		{ "text-before-edge", (int)DOMINANTBASELINE::TEXT_BEFORE_EDGE },
		{ "text-bottom", (int)DOMINANTBASELINE::TEXT_BOTTOM },
		{ "text-top", (int)DOMINANTBASELINE::TEXT_TOP },
		{ "use-script", (int)DOMINANTBASELINE::USE_SCRIPT }
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
    enum VectorEffectKind {
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
