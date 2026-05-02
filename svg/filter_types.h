// filter_types.h

#pragma once

//
// This file provides access to types and enums
// which are required by both filter builders
// and executors.  It is a core piece that should 
// be included in pretty much every filter related
// file.
// 


#include "definitions.h"

#include "bit_hacks.h"
#include "wggeometry.h"
#include "nametable.h"
#include "coloring.h"
#include "svgdatatypes.h"


namespace waavs
{
    enum FilterColorInterpolation : uint32_t
    {
        FILTER_COLOR_INTERPOLATION_AUTO = 0,    // When there is not an authored value
        FILTER_COLOR_INTERPOLATION_LINEAR_RGB,
        FILTER_COLOR_INTERPOLATION_SRGB
    };

    // feBlend 'mode' attribute
    enum FilterBlendMode : uint32_t
    {
        FILTER_BLEND_NORMAL = 0,
        FILTER_BLEND_MULTIPLY,
        FILTER_BLEND_SCREEN,
        FILTER_BLEND_DARKEN,
        FILTER_BLEND_LIGHTEN,
        FILTER_BLEND_OVERLAY,
        FILTER_BLEND_COLOR_DODGE,
        FILTER_BLEND_COLOR_BURN,
        FILTER_BLEND_HARD_LIGHT,
        FILTER_BLEND_SOFT_LIGHT,
        FILTER_BLEND_DIFFERENCE,
        FILTER_BLEND_EXCLUSION
    };

    enum FilterCompositeOp : uint32_t
    {
        FILTER_COMPOSITE_OVER = 0,
        FILTER_COMPOSITE_IN,
        FILTER_COMPOSITE_OUT,
        FILTER_COMPOSITE_ATOP,
        FILTER_COMPOSITE_XOR,
        FILTER_COMPOSITE_ARITHMETIC
    };


    enum FilterColorMatrixType : uint32_t
    {
        FILTER_COLOR_MATRIX_MATRIX = 0,
        FILTER_COLOR_MATRIX_SATURATE,
        FILTER_COLOR_MATRIX_HUE_ROTATE,
        FILTER_COLOR_MATRIX_LUMINANCE_TO_ALPHA
    };

    enum FilterTransferFuncType : uint32_t
    {
        FILTER_TRANSFER_IDENTITY = 0,
        FILTER_TRANSFER_TABLE,
        FILTER_TRANSFER_DISCRETE,
        FILTER_TRANSFER_LINEAR,
        FILTER_TRANSFER_GAMMA
    };

    enum FilterMorphologyOp : uint32_t
    {
        FILTER_MORPHOLOGY_ERODE = 0,
        FILTER_MORPHOLOGY_DILATE
    };

    enum FilterEdgeMode : uint32_t
    {
        FILTER_EDGE_DUPLICATE = 0,
        FILTER_EDGE_WRAP,
        FILTER_EDGE_NONE
    };

    enum FilterChannelSelector : uint32_t
    {
        FILTER_CHANNEL_R = 0,
        FILTER_CHANNEL_G,
        FILTER_CHANNEL_B,
        FILTER_CHANNEL_A
    };

    enum FilterTurbulenceType : uint32_t
    {
        FILTER_TURBULENCE_TURBULENCE = 0,
        FILTER_TURBULENCE_FRACTAL_NOISE
    };

    enum FilterLightType : uint32_t
    {
        FILTER_LIGHT_DISTANT = 1,
        FILTER_LIGHT_POINT = 2,
        FILTER_LIGHT_SPOT = 3
    };
}


namespace waavs::filter
{

    // --------------------------------------------------------
    // Common filter keys
    // --------------------------------------------------------
    INLINE InternedKey color_interpolation_filters() noexcept { static InternedKey k = PSNameTable::INTERN("color-interpolation-filters"); return k; }
    INLINE InternedKey kColorInterp_linearRGB() noexcept { static InternedKey k = PSNameTable::INTERN("linearRGB"); return k; }
    INLINE InternedKey kColorInterp_sRGB()      noexcept { static InternedKey k = PSNameTable::INTERN("sRGB");      return k; }


    // Common filter primitive keys
    INLINE InternedKey SourceGraphic() noexcept { static InternedKey k = PSNameTable::INTERN("SourceGraphic"); return k; }
    INLINE InternedKey SourceAlpha() noexcept { static InternedKey k = PSNameTable::INTERN("SourceAlpha"); return k; }
    INLINE InternedKey BackgroundImage() noexcept { static InternedKey k = PSNameTable::INTERN("BackgroundImage"); return k; }
    INLINE InternedKey Filter_Last() noexcept { static InternedKey k = PSNameTable::INTERN("__last__"); return k; }


    // all filter primitives have these common attributes
    inline InternedKey in() { static InternedKey k = PSNameTable::INTERN("in"); return k; }
    inline InternedKey in2() { static InternedKey k = PSNameTable::INTERN("in2"); return k; }
    inline InternedKey result() { static InternedKey k = PSNameTable::INTERN("result"); return k; }

    // feGaussianBlur
    inline InternedKey stdDeviation() { static InternedKey k = PSNameTable::INTERN("stdDeviation"); return k; }

    inline InternedKey flood_color() { static InternedKey k = PSNameTable::INTERN("flood-color"); return k; }
    inline InternedKey flood_opacity() { static InternedKey k = PSNameTable::INTERN("flood-opacity"); return k; }

    // feOffset
    inline InternedKey feOffset_dx() { static InternedKey k = PSNameTable::INTERN("dx"); return k; }
    inline InternedKey feOffset_dy() { static InternedKey k = PSNameTable::INTERN("dy"); return k; }

    // feColorMatrix
    inline InternedKey type_() { static InternedKey k = PSNameTable::INTERN("type"); return k; }
    inline InternedKey values() { static InternedKey k = PSNameTable::INTERN("values"); return k; }

    // feBlend
    inline InternedKey mode() { static InternedKey k = PSNameTable::INTERN("mode"); return k; }
    
    // feBlend modes 
    INLINE InternedKey kBlend_normal()   noexcept { static InternedKey k = PSNameTable::INTERN("normal");   return k; }
    INLINE InternedKey kBlend_multiply() noexcept { static InternedKey k = PSNameTable::INTERN("multiply"); return k; }
    INLINE InternedKey kBlend_screen()   noexcept { static InternedKey k = PSNameTable::INTERN("screen");   return k; }
    INLINE InternedKey kBlend_overlay()  noexcept { static InternedKey k = PSNameTable::INTERN("overlay");  return k; }
    INLINE InternedKey kBlend_darken()   noexcept { static InternedKey k = PSNameTable::INTERN("darken");   return k; }
    INLINE InternedKey kBlend_lighten()  noexcept { static InternedKey k = PSNameTable::INTERN("lighten");  return k; }
    INLINE InternedKey kBlend_color_dodge() noexcept { static InternedKey k = PSNameTable::INTERN("color-dodge"); return k; }
    INLINE InternedKey kBlend_color_burn()   noexcept { static InternedKey k = PSNameTable::INTERN("color-burn");   return k; }
    INLINE InternedKey kBlend_hard_light()  noexcept { static InternedKey k = PSNameTable::INTERN("hard-light");  return k; }
    INLINE InternedKey kBlend_soft_light()  noexcept { static InternedKey k = PSNameTable::INTERN("soft-light");  return k; }
    INLINE InternedKey kBlend_difference()  noexcept { static InternedKey k = PSNameTable::INTERN("difference");  return k; }
    INLINE InternedKey kBlend_exclusion()   noexcept { static InternedKey k = PSNameTable::INTERN("exclusion");   return k; }



    //static InternedKey kNormal = PSNameTable::INTERN("normal");
    //static InternedKey kMultiply = PSNameTable::INTERN("multiply");
    //static InternedKey kScreen = PSNameTable::INTERN("screen");
    //static InternedKey kDarken = PSNameTable::INTERN("darken");
    //static InternedKey kLighten = PSNameTable::INTERN("lighten");
    //static InternedKey kOverlay = PSNameTable::INTERN("overlay");
    //static InternedKey kColorDodge = PSNameTable::INTERN("color-dodge");
    //static InternedKey kColorBurn = PSNameTable::INTERN("color-burn");
    //static InternedKey kHardLight = PSNameTable::INTERN("hard-light");
    //static InternedKey kSoftLight = PSNameTable::INTERN("soft-light");
    //static InternedKey kDifference = PSNameTable::INTERN("difference");
    //static InternedKey kExclusion = PSNameTable::INTERN("exclusion");


    // feComposite
    inline InternedKey operator_() { static InternedKey k = PSNameTable::INTERN("operator"); return k; }
    inline InternedKey k1() { static InternedKey k = PSNameTable::INTERN("k1"); return k; }
    inline InternedKey k2() { static InternedKey k = PSNameTable::INTERN("k2"); return k; }
    inline InternedKey k3() { static InternedKey k = PSNameTable::INTERN("k3"); return k; }
    inline InternedKey k4() { static InternedKey k = PSNameTable::INTERN("k4"); return k; }

    // feComposite operator values
    INLINE InternedKey kCompOp_over()       noexcept { static InternedKey k = PSNameTable::INTERN("over");       return k; }
    INLINE InternedKey kCompOp_in()         noexcept { static InternedKey k = PSNameTable::INTERN("in");         return k; }
    INLINE InternedKey kCompOp_out()        noexcept { static InternedKey k = PSNameTable::INTERN("out");        return k; }
    INLINE InternedKey kCompOp_atop()       noexcept { static InternedKey k = PSNameTable::INTERN("atop");       return k; }
    INLINE InternedKey kCompOp_xor()        noexcept { static InternedKey k = PSNameTable::INTERN("xor");        return k; }
    INLINE InternedKey kCompOp_arithmetic() noexcept { static InternedKey k = PSNameTable::INTERN("arithmetic"); return k; }

    // feComponentTransfer / feFunc*
    inline InternedKey tableValues() { static InternedKey k = PSNameTable::INTERN("tableValues"); return k; }
    inline InternedKey slope() { static InternedKey k = PSNameTable::INTERN("slope"); return k; }
    inline InternedKey intercept() { static InternedKey k = PSNameTable::INTERN("intercept"); return k; }
    inline InternedKey amplitude() { static InternedKey k = PSNameTable::INTERN("amplitude"); return k; }
    inline InternedKey exponent() { static InternedKey k = PSNameTable::INTERN("exponent"); return k; }
    inline InternedKey offset() { static InternedKey k = PSNameTable::INTERN("offset"); return k; }

    inline InternedKey feFuncR() { static InternedKey k = PSNameTable::INTERN("feFuncR"); return k; }
    inline InternedKey feFuncG() { static InternedKey k = PSNameTable::INTERN("feFuncG"); return k; }
    inline InternedKey feFuncB() { static InternedKey k = PSNameTable::INTERN("feFuncB"); return k; }
    inline InternedKey feFuncA() { static InternedKey k = PSNameTable::INTERN("feFuncA"); return k; }
    
    
    static InternedKey kIdentity = PSNameTable::INTERN("identity");
    static InternedKey kTable = PSNameTable::INTERN("table");
    static InternedKey kDiscrete = PSNameTable::INTERN("discrete");
    static InternedKey kLinear = PSNameTable::INTERN("linear");
    static InternedKey kGamma = PSNameTable::INTERN("gamma");

    // feConvolveMatrix
    inline InternedKey order() { static InternedKey k = PSNameTable::INTERN("order"); return k; }
    inline InternedKey kernelMatrix() { static InternedKey k = PSNameTable::INTERN("kernelMatrix"); return k; }
    inline InternedKey divisor() { static InternedKey k = PSNameTable::INTERN("divisor"); return k; }
    inline InternedKey bias() { static InternedKey k = PSNameTable::INTERN("bias"); return k; }
    inline InternedKey targetX() { static InternedKey k = PSNameTable::INTERN("targetX"); return k; }
    inline InternedKey targetY() { static InternedKey k = PSNameTable::INTERN("targetY"); return k; }
    inline InternedKey edgeMode() { static InternedKey k = PSNameTable::INTERN("edgeMode"); return k; }
    inline InternedKey kernelUnitLength() { static InternedKey k = PSNameTable::INTERN("kernelUnitLength"); return k; }
    inline InternedKey preserveAlpha() { static InternedKey k = PSNameTable::INTERN("preserveAlpha"); return k; }

    // feDisplacementMap
    inline InternedKey scale() { static InternedKey k = PSNameTable::INTERN("scale"); return k; }
    inline InternedKey xChannelSelector() { static InternedKey k = PSNameTable::INTERN("xChannelSelector"); return k; }
    inline InternedKey yChannelSelector() { static InternedKey k = PSNameTable::INTERN("yChannelSelector"); return k; }

    // feTurbulence
    inline InternedKey baseFrequency() { static InternedKey k = PSNameTable::INTERN("baseFrequency"); return k; }
    inline InternedKey numOctaves() { static InternedKey k = PSNameTable::INTERN("numOctaves"); return k; }
    inline InternedKey seed() { static InternedKey k = PSNameTable::INTERN("seed"); return k; }
    inline InternedKey stitchTiles() { static InternedKey k = PSNameTable::INTERN("stitchTiles"); return k; }

    // lighting
    inline InternedKey surfaceScale() { static InternedKey k = PSNameTable::INTERN("surfaceScale"); return k; }
    inline InternedKey diffuseConstant() { static InternedKey k = PSNameTable::INTERN("diffuseConstant"); return k; }
    inline InternedKey specularConstant() { static InternedKey k = PSNameTable::INTERN("specularConstant"); return k; }
    inline InternedKey specularExponent() { static InternedKey k = PSNameTable::INTERN("specularExponent"); return k; }
    inline InternedKey lighting_color() { static InternedKey k = PSNameTable::INTERN("lighting-color"); return k; }

    inline InternedKey azimuth() { static InternedKey k = PSNameTable::INTERN("azimuth"); return k; }
    inline InternedKey elevation() { static InternedKey k = PSNameTable::INTERN("elevation"); return k; }
    inline InternedKey pointsAtX() { static InternedKey k = PSNameTable::INTERN("pointsAtX"); return k; }
    inline InternedKey pointsAtY() { static InternedKey k = PSNameTable::INTERN("pointsAtY"); return k; }
    inline InternedKey pointsAtZ() { static InternedKey k = PSNameTable::INTERN("pointsAtZ"); return k; }
    inline InternedKey limitingConeAngle() { static InternedKey k = PSNameTable::INTERN("limitingConeAngle"); return k; }

    // feDistantLight
    inline InternedKey feDistantLight() { static InternedKey k = PSNameTable::INTERN("feDistantLight"); return k; }
    inline InternedKey fePointLight() { static InternedKey k = PSNameTable::INTERN("fePointLight"); return k; }
    inline InternedKey feSpotLight() { static InternedKey k = PSNameTable::INTERN("feSpotLight"); return k; }

    // diffuse/specular lighting common
    inline InternedKey x() { static InternedKey k = PSNameTable::INTERN("x"); return k; }
    inline InternedKey y() { static InternedKey k = PSNameTable::INTERN("y"); return k; }
    inline InternedKey z() { static InternedKey k = PSNameTable::INTERN("z"); return k; }

    // feMorphology
    inline InternedKey radius() { static InternedKey k = PSNameTable::INTERN("radius"); return k; }

    // feMerge / feMergeNode
    inline InternedKey feMergeNode_in() { static InternedKey k = PSNameTable::INTERN("in"); return k; }

    // feImage
    inline InternedKey href() { static InternedKey k = PSNameTable::INTERN("href"); return k; }
    inline InternedKey xlink_href() { static InternedKey k = PSNameTable::INTERN("xlink:href"); return k; }

}

namespace waavs
{
    // A simple struct to hold a pair of numbers, 
// with the possibility of the second being optional/implied.
    struct SVGNumberPair
    {
        float a{ 0.0f };
        float b{ 0.0f };
        bool hasB{ false }; // if false, b is implied = a

        void set(float v) { a = v; b = v; hasB = false; }
    };


    static INLINE bool lengthToFilterNumberOrPercent(const SVGLengthValue& inVal,
        SVGNumberOrPercent& outVal,
        double dpi = 96.0,
        const BLFont* font = nullptr) noexcept
    {
        return lengthValueToNumberOrPercent(inVal, outVal, dpi, font, true);
    }

    // Parse: <number> [<number>]
    // Note: negative numbers are treated as 0, per SVG spec.
    // 
    // Uses readNextNumber(), so it naturally accepts comma/space separators.
    static INLINE bool parseNumberPair(ByteSpan s, SVGNumberPair& out) noexcept
    {
        s.skipSpaces();

        if (!s) { out.set(0.0f); return false; }

        double x = 0.0;
        if (!readNextNumber(s, x)) { out.set(0.0f); return false; }

        double y = 0.0;
        if (readNextNumber(s, y)) {
            out.a = (float)x;
            out.b = (float)y;
            out.hasB = true;
        }
        else {
            out.set((float)x);
        }

        // SVG behavior: negative treated as 0
        if (out.a < 0.0f) out.a = 0.0f;
        if (out.b < 0.0f) out.b = 0.0f;

        return true;
    }

    static INLINE bool parseF32Attr(const ByteSpan& s, float& out) noexcept
    {
        double v = 0.0;
        if (!parseNumber(s, v))
            return false;

        out = static_cast<float>(v);

        return true;
    }

    // Note: negative numbers are treated as 0, per SVG spec.
    static INLINE bool parseU32Attr(const ByteSpan& s, uint32_t& out) noexcept
    {
        double v = 0.0;
        if (!parseNumber(s, v))
            return false;

        if (v < 0.0)
            v = 0.0;

        out = static_cast<uint32_t>(v);

        return true;
    }

    // Parse a pair of numbers
    // Different from parseNumberPair in that the second value
    // is NOT optional.
    // Note: negative numbers are NOT treated as 0 here
    static INLINE bool parseFloatPairAttr(const ByteSpan& s, float& a, float& b) noexcept
    {
        SVGTokenListView tv(s);
        double x = 0.0;
        double y = 0.0;

        if (!tv.readANumber(x))
            return false;

        if (!tv.readANumber(y))
            y = x;

        a = (float)x;
        b = (float)y;
        return true;
    }

    static INLINE bool parseU32PairAttr(const ByteSpan& s, uint32_t& a, uint32_t& b) noexcept
    {
        SVGTokenListView tv(s);
        double x = 0.0;
        double y = 0.0;

        if (!tv.readANumber(x))
            return false;

        if (!tv.readANumber(y))
            y = x;

        if (x < 1.0) x = 1.0;
        if (y < 1.0) y = 1.0;

        a = (uint32_t)x;
        b = (uint32_t)y;
        return true;
    }

    // Parse a list of numbers, e.g. for feColorMatrix values.
    static INLINE bool parseFloatListAttr(const ByteSpan& s, std::vector<float>& out) noexcept
    {
        out.clear();

        SVGTokenListView tv(s);
        double v = 0.0;
        while (tv.readANumber(v))
            out.push_back((float)v);

        return !out.empty();
    }

    // Parse a list of numbers, with an expected count. 
    // Return false if the count doesn't match.
    static INLINE bool parseFloatListExact(const ByteSpan& s, float* dst, size_t n) noexcept
    {
        SVGTokenListView tv(s);
        double v = 0.0;

        for (size_t i = 0; i < n; ++i)
        {
            if (!tv.readANumber(v))
                return false;
            dst[i] = (float)v;
        }

        return true;
    }

}



// Converting from string keys to enum values
// for filter attributes that have a closed vocabulary
namespace waavs
{
    // Parse an exact boolean value, which can be "true"/"false" 
    // or a number (0=false, nonzero=true).
    static INLINE bool parseBoolAttr(const ByteSpan& s, bool& out) noexcept
    {
        if (!s)
            return false;

        static InternedKey kTrue = PSNameTable::INTERN("true");
        static InternedKey kFalse = PSNameTable::INTERN("false");
        InternedKey k = PSNameTable::INTERN(s);

        if (k == kTrue) {
            out = true;
            return true;
        }

        if (k == kFalse) {
            out = false;
            return true;
        }

        uint32_t v = 0;
        if (!parseU32Attr(s, v))
            return false;

        out = (v != 0);
        return true;
    }

    static INLINE bool parseStitchTilesAttr(const ByteSpan& s, bool& out) noexcept
    {
        if (!s)
            return false;

        static InternedKey kStitch = PSNameTable::INTERN("stitch");
        static InternedKey kNoStitch = PSNameTable::INTERN("noStitch");
        InternedKey k = PSNameTable::INTERN(s);

        if (k == kStitch) {
            out = true;
            return true;
        }

        if (k == kNoStitch) {
            out = false;
            return true;
        }

        return false;
    }

    static INLINE FilterColorInterpolation parseFilterColorInterpolation(InternedKey k, FilterColorInterpolation def) noexcept
    {
        // Filter effects default to linearRGB.
        if (!k) return def;

        if (k == filter::kColorInterp_linearRGB()) return FILTER_COLOR_INTERPOLATION_LINEAR_RGB;
        if (k == filter::kColorInterp_sRGB())      return FILTER_COLOR_INTERPOLATION_SRGB;

        // If we admit "auto" later at parse time, collapse it here for now.
        // if (k == filter::kColorInterp_auto()) return FILTER_COLOR_INTERPOLATION_LINEAR_RGB;

        return def;
    }


    static INLINE FilterBlendMode parseFilterBlendMode(InternedKey k) noexcept
    {
        if (!k) return FILTER_BLEND_NORMAL;


        if (k == filter::kBlend_normal())     return FILTER_BLEND_NORMAL;
        if (k == filter::kBlend_multiply())   return FILTER_BLEND_MULTIPLY;
        if (k == filter::kBlend_screen())     return FILTER_BLEND_SCREEN;
        if (k == filter::kBlend_darken())     return FILTER_BLEND_DARKEN;
        if (k == filter::kBlend_lighten())    return FILTER_BLEND_LIGHTEN;
        if (k == filter::kBlend_overlay())    return FILTER_BLEND_OVERLAY;
        if (k == filter::kBlend_color_dodge()) return FILTER_BLEND_COLOR_DODGE;
        if (k == filter::kBlend_color_burn())  return FILTER_BLEND_COLOR_BURN;
        if (k == filter::kBlend_hard_light())  return FILTER_BLEND_HARD_LIGHT;
        if (k == filter::kBlend_soft_light())  return FILTER_BLEND_SOFT_LIGHT;
        if (k == filter::kBlend_difference()) return FILTER_BLEND_DIFFERENCE;
        if (k == filter::kBlend_exclusion())  return FILTER_BLEND_EXCLUSION;

        return FILTER_BLEND_NORMAL;
    }

    static INLINE FilterCompositeOp parseFilterCompositeOp(InternedKey k) noexcept
    {
        if (!k) return FILTER_COMPOSITE_OVER;


        if (k == filter::kCompOp_over())       return FILTER_COMPOSITE_OVER;
        if (k == filter::kCompOp_in())         return FILTER_COMPOSITE_IN;
        if (k == filter::kCompOp_out())        return FILTER_COMPOSITE_OUT;
        if (k == filter::kCompOp_atop())       return FILTER_COMPOSITE_ATOP;
        if (k == filter::kCompOp_xor())        return FILTER_COMPOSITE_XOR;
        if (k == filter::kCompOp_arithmetic()) return FILTER_COMPOSITE_ARITHMETIC;

        return FILTER_COMPOSITE_OVER;
    }

    static INLINE FilterColorMatrixType parseFilterColorMatrixType(InternedKey k) noexcept
    {
        if (!k) return FILTER_COLOR_MATRIX_MATRIX;

        static InternedKey kMatrix = PSNameTable::INTERN("matrix");
        static InternedKey kSaturate = PSNameTable::INTERN("saturate");
        static InternedKey kHueRotate = PSNameTable::INTERN("hueRotate");
        static InternedKey kLuminanceToAlpha = PSNameTable::INTERN("luminanceToAlpha");

        if (k == kMatrix)           return FILTER_COLOR_MATRIX_MATRIX;
        if (k == kSaturate)         return FILTER_COLOR_MATRIX_SATURATE;
        if (k == kHueRotate)        return FILTER_COLOR_MATRIX_HUE_ROTATE;
        if (k == kLuminanceToAlpha) return FILTER_COLOR_MATRIX_LUMINANCE_TO_ALPHA;

        return FILTER_COLOR_MATRIX_MATRIX;
    }

    static INLINE FilterTransferFuncType parseFilterTransferFuncType(InternedKey k) noexcept
    {
        if (!k) return FILTER_TRANSFER_IDENTITY;

        static InternedKey kIdentity = PSNameTable::INTERN("identity");
        static InternedKey kTable = PSNameTable::INTERN("table");
        static InternedKey kDiscrete = PSNameTable::INTERN("discrete");
        static InternedKey kLinear = PSNameTable::INTERN("linear");
        static InternedKey kGamma = PSNameTable::INTERN("gamma");

        if (k == kIdentity) return FILTER_TRANSFER_IDENTITY;
        if (k == kTable)    return FILTER_TRANSFER_TABLE;
        if (k == kDiscrete) return FILTER_TRANSFER_DISCRETE;
        if (k == kLinear)   return FILTER_TRANSFER_LINEAR;
        if (k == kGamma)    return FILTER_TRANSFER_GAMMA;

        return FILTER_TRANSFER_IDENTITY;
    }

    static INLINE FilterMorphologyOp parseFilterMorphologyOp(InternedKey k) noexcept
    {
        if (!k) return FILTER_MORPHOLOGY_ERODE;

        static InternedKey kErode = PSNameTable::INTERN("erode");
        static InternedKey kDilate = PSNameTable::INTERN("dilate");

        if (k == kErode)  return FILTER_MORPHOLOGY_ERODE;
        if (k == kDilate) return FILTER_MORPHOLOGY_DILATE;

        return FILTER_MORPHOLOGY_ERODE;
    }

    static INLINE FilterEdgeMode parseFilterEdgeMode(InternedKey k) noexcept
    {
        if (!k) return FILTER_EDGE_DUPLICATE;

        static InternedKey kDuplicate = PSNameTable::INTERN("duplicate");
        static InternedKey kWrap = PSNameTable::INTERN("wrap");
        static InternedKey kNone = PSNameTable::INTERN("none");

        if (k == kDuplicate) return FILTER_EDGE_DUPLICATE;
        if (k == kWrap)      return FILTER_EDGE_WRAP;
        if (k == kNone)      return FILTER_EDGE_NONE;

        return FILTER_EDGE_DUPLICATE;
    }

    static INLINE FilterChannelSelector parseFilterChannelSelector(InternedKey k) noexcept
    {
        if (!k) return FILTER_CHANNEL_A;

        static InternedKey kR = PSNameTable::INTERN("R");
        static InternedKey kG = PSNameTable::INTERN("G");
        static InternedKey kB = PSNameTable::INTERN("B");
        static InternedKey kA = PSNameTable::INTERN("A");

        if (k == kR) return FILTER_CHANNEL_R;
        if (k == kG) return FILTER_CHANNEL_G;
        if (k == kB) return FILTER_CHANNEL_B;
        if (k == kA) return FILTER_CHANNEL_A;

        return FILTER_CHANNEL_A;
    }

    static INLINE FilterTurbulenceType parseFilterTurbulenceType(InternedKey k) noexcept
    {
        if (!k) return FILTER_TURBULENCE_TURBULENCE;

        static InternedKey kTurbulence = PSNameTable::INTERN("turbulence");
        static InternedKey kFractalNoise = PSNameTable::INTERN("fractalNoise");

        if (k == kTurbulence)   return FILTER_TURBULENCE_TURBULENCE;
        if (k == kFractalNoise) return FILTER_TURBULENCE_FRACTAL_NOISE;

        return FILTER_TURBULENCE_TURBULENCE;
    }
}



namespace waavs
{
    // Optional spans for list payloads (reference impl uses heap scratch).
    struct F32Span { const float* p{}; uint32_t n{}; };
    struct KeySpan { const InternedKey* p{}; uint32_t n{}; };

    // feComponentTransfer function specification for a single channel.
    struct ComponentFunc {
        FilterTransferFuncType type{ FILTER_TRANSFER_IDENTITY };
        float p0{}, p1{}, p2{};
        F32Span table{};
    };

    struct SVGComponentTransferFunc
    {
        FilterTransferFuncType fType{ FILTER_TRANSFER_IDENTITY };
        float fP0{ 1.0f };
        float fP1{ 0.0f };
        float fP2{ 0.0f };
        std::vector<float> fTable{};
    };

    // For feComposite, arithmetic operator parameters k1..k4 
// often have special cases when they are zero.
    enum ArithmeticCompositeKind : uint32_t
    {
        ARITH_ZERO = 0,
        ARITH_K1_ONLY,
        ARITH_K2_ONLY,
        ARITH_K3_ONLY,
        ARITH_K4_ONLY,
        ARITH_K2_K3,
        ARITH_K1_K2_K3,
        ARITH_GENERAL
    };

    // Classify the arithmetic composite operator based on which of k1..k4 are zero.
// Then use that later to specialize the inner loop for common cases.
    static INLINE ArithmeticCompositeKind classifyArithmetic(
        float k1, float k2, float k3, float k4) noexcept
    {
        const bool z1 = k1 == 0.0f;
        const bool z2 = k2 == 0.0f;
        const bool z3 = k3 == 0.0f;
        const bool z4 = k4 == 0.0f;

        if (z1 && z2 && z3 && z4) return ARITH_ZERO;
        if (!z1 && z2 && z3 && z4) return ARITH_K1_ONLY;
        if (z1 && !z2 && z3 && z4) return ARITH_K2_ONLY;
        if (z1 && z2 && !z3 && z4) return ARITH_K3_ONLY;
        if (z1 && z2 && z3 && !z4) return ARITH_K4_ONLY;
        if (z1 && !z2 && !z3 && z4) return ARITH_K2_K3;
        if (!z1 && !z2 && !z3 && z4) return ARITH_K1_K2_K3;

        return ARITH_GENERAL;
    }
}

namespace waavs
{
    struct LightPayload { float L[8]{}; };

    struct PixelToFilterUserMap
    {
        // Tile-local surface dimensions only.
        int surfaceW{ 0 };
        int surfaceH{ 0 };

        // Local semantic filter-space extent.
        WGRectD filterExtentUS{};

        float uxPerPixel{ 1.0f };
        float uyPerPixel{ 1.0f };
    };


    static INLINE void pixelCenterToFilterUserStandalone(
        const PixelToFilterUserMap& map,
        int px, int py,
        float& ux, float& uy) noexcept
    {
        //(void)map.surfaceW;
        //(void)map.surfaceH;

        // px/py are already tile-local pixel coordinates.
        const float fx = float(px) + 0.5f;
        const float fy = float(py) + 0.5f;

        ux = float(map.filterExtentUS.x) + fx * map.uxPerPixel;
        uy = float(map.filterExtentUS.y) + fy * map.uyPerPixel;
    }

}

namespace waavs
{
#if defined(__ARM_NEON) || defined(__aarch64__)
    static INLINE float32x4_t neon_clamp01_f32(float32x4_t v) noexcept
    {
        return vminq_f32(vmaxq_f32(v, vdupq_n_f32(0.0f)), vdupq_n_f32(1.0f));
    }

    static INLINE float32x4_t neon_recip_nr_f32(float32x4_t x) noexcept
    {
        float32x4_t y = vrecpeq_f32(x);
        y = vmulq_f32(y, vrecpsq_f32(x, y));
        y = vmulq_f32(y, vrecpsq_f32(x, y));
        return y;
    }
#endif
}
