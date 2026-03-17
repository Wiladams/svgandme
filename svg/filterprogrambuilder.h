
#pragma once


#include "filterprogram.h"
#include "nametable.h"
#include "svgdatatypes.h"
#include "surface.h"

namespace waavs::filter
{

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

    // feComposite
    inline InternedKey operator_() { static InternedKey k = PSNameTable::INTERN("operator"); return k; }
    inline InternedKey k1() { static InternedKey k = PSNameTable::INTERN("k1"); return k; }
    inline InternedKey k2() { static InternedKey k = PSNameTable::INTERN("k2"); return k; }
    inline InternedKey k3() { static InternedKey k = PSNameTable::INTERN("k3"); return k; }
    inline InternedKey k4() { static InternedKey k = PSNameTable::INTERN("k4"); return k; }

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
    // --------------------------------------------------------
    // Filter enum families for closed vocabularies
    // These values are emitted into mem[] as u32.
    // --------------------------------------------------------
    
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




// Converting from string keys to enum values
// for filter attributes that have a closed vocabulary
namespace waavs
{
    static INLINE FilterBlendMode parseFilterBlendMode(InternedKey k) noexcept
    {
        if (!k) return FILTER_BLEND_NORMAL;

        static InternedKey kNormal = PSNameTable::INTERN("normal");
        static InternedKey kMultiply = PSNameTable::INTERN("multiply");
        static InternedKey kScreen = PSNameTable::INTERN("screen");
        static InternedKey kDarken = PSNameTable::INTERN("darken");
        static InternedKey kLighten = PSNameTable::INTERN("lighten");
        static InternedKey kOverlay = PSNameTable::INTERN("overlay");
        static InternedKey kColorDodge = PSNameTable::INTERN("color-dodge");
        static InternedKey kColorBurn = PSNameTable::INTERN("color-burn");
        static InternedKey kHardLight = PSNameTable::INTERN("hard-light");
        static InternedKey kSoftLight = PSNameTable::INTERN("soft-light");
        static InternedKey kDifference = PSNameTable::INTERN("difference");
        static InternedKey kExclusion = PSNameTable::INTERN("exclusion");

        if (k == kNormal)     return FILTER_BLEND_NORMAL;
        if (k == kMultiply)   return FILTER_BLEND_MULTIPLY;
        if (k == kScreen)     return FILTER_BLEND_SCREEN;
        if (k == kDarken)     return FILTER_BLEND_DARKEN;
        if (k == kLighten)    return FILTER_BLEND_LIGHTEN;
        if (k == kOverlay)    return FILTER_BLEND_OVERLAY;
        if (k == kColorDodge) return FILTER_BLEND_COLOR_DODGE;
        if (k == kColorBurn)  return FILTER_BLEND_COLOR_BURN;
        if (k == kHardLight)  return FILTER_BLEND_HARD_LIGHT;
        if (k == kSoftLight)  return FILTER_BLEND_SOFT_LIGHT;
        if (k == kDifference) return FILTER_BLEND_DIFFERENCE;
        if (k == kExclusion)  return FILTER_BLEND_EXCLUSION;

        return FILTER_BLEND_NORMAL;
    }

    static INLINE FilterCompositeOp parseFilterCompositeOp(InternedKey k) noexcept
    {
        if (!k) return FILTER_COMPOSITE_OVER;

        static InternedKey kOver = PSNameTable::INTERN("over");
        static InternedKey kIn = PSNameTable::INTERN("in");
        static InternedKey kOut = PSNameTable::INTERN("out");
        static InternedKey kAtop = PSNameTable::INTERN("atop");
        static InternedKey kXor = PSNameTable::INTERN("xor");
        static InternedKey kArithmetic = PSNameTable::INTERN("arithmetic");

        if (k == kOver)       return FILTER_COMPOSITE_OVER;
        if (k == kIn)         return FILTER_COMPOSITE_IN;
        if (k == kOut)        return FILTER_COMPOSITE_OUT;
        if (k == kAtop)       return FILTER_COMPOSITE_ATOP;
        if (k == kXor)        return FILTER_COMPOSITE_XOR;
        if (k == kArithmetic) return FILTER_COMPOSITE_ARITHMETIC;

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

    //============================================================
    // Filter program builder helpers
    //============================================================




    static constexpr uint64_t kNOP_IsPercent = 0x0000000100000000ull;
    static constexpr uint64_t kNOP_IsSet = 0x8000000000000000ull;

    static INLINE uint64_t packNumberOrPercent(const SVGNumberOrPercent& v) noexcept
    {
        uint32_t lo = 0;
        float fv = static_cast<float>(v.fValue);
        memcpy(&lo, &fv, sizeof(lo));

        uint64_t packed = uint64_t(lo);

        if (v.fIsPercent)
            packed |= kNOP_IsPercent;

        if (v.fIsSet)
            packed |= kNOP_IsSet;

        return packed;
    }



    static INLINE bool packedNumberOrPercentIsSet(uint64_t packed) noexcept
    {
        return (packed & kNOP_IsSet) != 0;
    }

    static INLINE bool packedNumberOrPercentIsPercent(uint64_t packed) noexcept
    {
        return (packed & kNOP_IsPercent) != 0;
    }

    static INLINE double packedNumberOrPercentValue(uint64_t packed) noexcept
    {
        uint32_t lo = uint32_t(packed & 0xFFFFFFFFu);
        float fv = 0.0f;
        memcpy(&fv, &lo, sizeof(fv));
        return double(fv);
    }




    // ------------------------------------
    // Emitters for common operand types
    // ------------------------------------
    static INLINE void emit_u32(FilterProgramStream& out, uint32_t v) noexcept
    {
        out.mem.push_back((uint64_t)v);
    }

    static INLINE void emit_f32(FilterProgramStream& out, float v) noexcept
    {
        out.mem.push_back(u64_from_f32(v));
    }

    static INLINE void emit_key(FilterProgramStream& out, InternedKey k) noexcept
    {
        out.mem.push_back(u64_from_key(k));
    }

    static INLINE void emit_u32x2(FilterProgramStream& out, uint32_t a, uint32_t b) noexcept
    {
        out.mem.push_back(u64_pack_u32x2(a, b));
    }

    // For inline lists
    static INLINE void emit_f32_list(FilterProgramStream& out, const float* vals, uint32_t count) noexcept
    {
        for (uint32_t i = 0; i < count; ++i)
            out.mem.push_back(u64_from_f32(vals[i]));
    }

    static INLINE void emit_counted_f32_list(FilterProgramStream& out, const float* vals, uint32_t count) noexcept
    {
        emit_u32(out, count);
        emit_f32_list(out, vals, count);
    }

    static INLINE void emit_counted_key_list(FilterProgramStream& out, const InternedKey* vals, uint32_t count) noexcept
    {
        emit_u32(out, count);
        for (uint32_t i = 0; i < count; ++i)
            out.mem.push_back(u64_from_key(vals[i]));
    }


    static INLINE InternedKey finishLast(InternedKey resultKey) noexcept
    {
        return resultKey ? resultKey : kFilter_Last();
    }
}


// Some predicate helpers
namespace waavs
{
    static INLINE bool compositeOpNeedsArithmeticCoeffs(FilterCompositeOp op) noexcept
    {
        return op == FILTER_COMPOSITE_ARITHMETIC;
    }

    static INLINE bool colorMatrixTypeHasMatrix(FilterColorMatrixType t) noexcept
    {
        return t == FILTER_COLOR_MATRIX_MATRIX;
    }

    static INLINE bool colorMatrixTypeHasSingleValue(FilterColorMatrixType t) noexcept
    {
        return t == FILTER_COLOR_MATRIX_SATURATE || t == FILTER_COLOR_MATRIX_HUE_ROTATE;
    }

    static INLINE bool transferFuncTypeUsesTable(FilterTransferFuncType t) noexcept
    {
        return t == FILTER_TRANSFER_TABLE || t == FILTER_TRANSFER_DISCRETE;
    }
}

namespace waavs {
    static INLINE uint32_t samplePixelEdge(const Surface& s, int x, int y, FilterEdgeMode edgeMode) noexcept
    {
        const int W = (int)s.width();
        const int H = (int)s.height();

        if (W <= 0 || H <= 0)
            return 0u;

        switch (edgeMode)
        {
        case FILTER_EDGE_WRAP:
            // Wrap coordinates periodically into the image extent.
            x %= W;
            y %= H;

            if (x < 0) x += W;
            if (y < 0) y += H;
            break;

        case FILTER_EDGE_NONE:
            // Outside image contributes transparent black.
            if (x < 0 || y < 0 || x >= W || y >= H)
                return 0u;
            break;

        case FILTER_EDGE_DUPLICATE:
        default:
            // Clamp to nearest valid edge pixel.
            if (x < 0) x = 0;
            else if (x >= W) x = W - 1;

            if (y < 0) y = 0;
            else if (y >= H) y = H - 1;
            break;
        }

        const uint32_t* row = (const uint32_t*)s.rowPointer((size_t)y);
        return row[x];
    }

    // pixelHeightFromAlpha()
    //
    // Interprets the alpha channel of a pixel as a height value in [0,1].
    //
    static INLINE float pixelHeightFromAlpha(const Surface& s, int x, int y) noexcept
    {
        const int W = (int)s.width();
        const int H = (int)s.height();

        if (W <= 0 || H <= 0)
            return 0.0f;

        if (x < 0) x = 0;
        else if (x >= W) x = W - 1;

        if (y < 0) y = 0;
        else if (y >= H) y = H - 1;

        const uint32_t* row = (const uint32_t*)s.rowPointer((size_t)y);
        const uint32_t px = row[x];

        return float((px >> 24) & 0xFF) / 255.0f;
    }

    // computeHeightNormal()
    //
    // Computes the normal vector at pixel (x,y) by sampling the alpha channel of
    //
    static INLINE void computeHeightNormal(const Surface& s,
        int x, int y,
        float surfaceScale,
        float& nx, float& ny, float& nz) noexcept
    {
        // Sample neighboring heights from alpha.
        const float hL = pixelHeightFromAlpha(s, x - 1, y);
        const float hR = pixelHeightFromAlpha(s, x + 1, y);
        const float hU = pixelHeightFromAlpha(s, x, y - 1);
        const float hD = pixelHeightFromAlpha(s, x, y + 1);

        // Central-difference gradient.
        const float dHx = 0.5f * (hR - hL);
        const float dHy = 0.5f * (hD - hU);

        // SurfaceScale turns alpha-height into Z displacement.
        // Normal points "up" from the height field.
        nx = -dHx * surfaceScale;
        ny = -dHy * surfaceScale;
        nz = 1.0f;

        // Normalize.
        const float len2 = nx * nx + ny * ny + nz * nz;
        if (len2 > 0.0f)
        {
            const float invLen = 1.0f / std::sqrt(len2);
            nx *= invLen;
            ny *= invLen;
            nz *= invLen;
        }
        else
        {
            nx = 0.0f;
            ny = 0.0f;
            nz = 1.0f;
        }
    }


}



// ============================================================================
// FILTER PROGRAM ABI CONTRACT (HYBRID FORM: INTERNED GRAPH IDS + NUMERIC ENUMS)
// ============================================================================
//
// This document defines the binary contract between the filter program
// builder and the filter program executor.
//
// The program consists of:
//
//   ops[] : std::vector<uint8_t>
//   mem[] : std::vector<uint64_t>
//
// Each op byte packs:
//
//   bits [0..4] : opcode id (numeric range [0..31])
//   bits [5..7] : flags (FOPF_*)
//
// All operands for all opcodes are stored in mem[] as 64-bit words.
// A word may contain:
//
//   - an InternedKey (stored via u64_from_key)
//   - a u32 enum or integer in the low 32 bits
//   - an f32 value stored in the low 32 bits via u64_from_f32
//   - an f64 value in the full 64 bits
//   - a packed pair of u32 values via u64_pack_u32x2
//
// Programs are decoded strictly sequentially.
// No operand references another location inside mem[].
// All lists and arrays appear inline.
//
// ----------------------------------------------------------------------------
// IDENTIFIER STORAGE RULE
// ----------------------------------------------------------------------------
//
// The ABI uses two kinds of symbolic values:
//
// 1) OPEN-ENDED GRAPH IDENTIFIERS
//
//    These are stored as InternedKey values:
//
//      - in
//      - in2
//      - out/result
//      - reserved named inputs such as SourceGraphic, SourceAlpha,
//        BackgroundImage, BackgroundAlpha, FillPaint, StrokePaint
//
//    These identifiers represent filter graph connectivity and may contain
//    arbitrary user-defined names.
//
//    InternedKey is appropriate here because these names are open-ended and
//    author-defined. The name table itself acts as the symbol allocator.
//
// 2) CLOSED VOCABULARY PARAMETERS
//
//    These are stored as u32 enums:
//
//      - blend mode
//      - composite operator
//      - color matrix type
//      - transfer function type
//      - morphology operator
//      - edge mode
//      - channel selector
//      - turbulence type
//      - light type
//      - other fixed option sets
//
//    Closed vocabularies must not be stored as InternedKey in the ABI.
//
// ----------------------------------------------------------------------------
// COMMON IO PREFIX (applies to ALL real filter primitives)
// ----------------------------------------------------------------------------
//
// Every filter primitive begins with a common input/output prefix in mem[].
//
// Order is ALWAYS:
//
//   1) in1Key (InternedKey)            -- always present
//   2) in2Key (InternedKey)            -- present only if FOPF_HAS_IN2
//   3) outKey (InternedKey)            -- present only if FOPF_HAS_OUT
//
// If FOPF_HAS_OUT is not set, the result is written to the implicit
// unnamed "last" buffer managed by the executor.
//
// ----------------------------------------------------------------------------
// OPTIONAL PRIMITIVE SUBREGION
// ----------------------------------------------------------------------------
//
// If FOPF_HAS_SUBR is set, the following 4 float32 values appear
// immediately after the IO prefix:
//
//   subr_x (f32)
//   subr_y (f32)
//   subr_w (f32)
//   subr_h (f32)
//
// These values represent the primitive subregion.
//
// ----------------------------------------------------------------------------
// LIST STORAGE RULE
// ----------------------------------------------------------------------------
//
// Lists are stored INLINE immediately after the opcode parameters.
//
// Two patterns exist:
//
// 1) FIXED SIZE LIST
//
//    The opcode contract defines the exact number of elements.
//
//    Example:
//
//        matrix[20]
//
//    The next 20 mem words are the matrix values.
//
// 2) VARIABLE SIZE LIST
//
//    A u32 count (stored in the low 32 bits of a mem word) precedes the items:
//
//        count (u32)
//        item0
//        item1
//        item2
//        ...
//
// No list references or offsets are used.
//
// ----------------------------------------------------------------------------
// ENUM STORAGE RULE
// ----------------------------------------------------------------------------
//
// All closed-vocabulary options are encoded as u32 enum values in the low
// 32 bits of a mem word.
//
// Examples:
//
//   blend mode
//   composite operator
//   color matrix type
//   transfer function type
//   morphology operator
//   edge mode
//   channel selector
//   turbulence type
//   light type
//
// ----------------------------------------------------------------------------
// OPCODE-SPECIFIC CONTRACTS
// ----------------------------------------------------------------------------
//
// FOP_END:
//   No operands. Marks end of program.
//
// ----------------------------------------------------------------------------
//
// FOP_GAUSSIAN_BLUR (feGaussianBlur):
//
//   IO prefix (in1, [out])
//   [optional subregion]
//   stdDevX (f32)
//   stdDevY (f32)
//
// stdDevY equals stdDevX if author supplied only one value.
//
// ----------------------------------------------------------------------------
//
// FOP_OFFSET (feOffset):
//
//   IO prefix (in1, [out])
//   [optional subregion]
//   dx (f32)
//   dy (f32)
//
// ----------------------------------------------------------------------------
//
// FOP_BLEND (feBlend):
//
//   IO prefix (in1, in2, [out])
//   [optional subregion]
//   mode (u32 enum)
//
// ----------------------------------------------------------------------------
//
// FOP_COMPOSITE (feComposite):
//
//   IO prefix (in1, in2, [out])
//   [optional subregion]
//   operator (u32 enum)
//
// If operator == FILTER_COMPOSITE_ARITHMETIC:
//
//   k1 (f32)
//   k2 (f32)
//   k3 (f32)
//   k4 (f32)
//
// ----------------------------------------------------------------------------
//
// FOP_COLOR_MATRIX (feColorMatrix):
//
//   IO prefix (in1, [out])
//   [optional subregion]
//   type (u32 enum)
//
// If type == FILTER_COLOR_MATRIX_MATRIX:
//
//   matrix[20] (20 x f32, row-major 4x5)
//
// If type == FILTER_COLOR_MATRIX_SATURATE:
//
//   value (f32)
//
// If type == FILTER_COLOR_MATRIX_HUE_ROTATE:
//
//   angleDegrees (f32)
//
// If type == FILTER_COLOR_MATRIX_LUMINANCE_TO_ALPHA:
//
//   no additional parameters
//
// ----------------------------------------------------------------------------
//
// FOP_COMPONENT_TRANSFER (feComponentTransfer):
//
//   IO prefix (in1, [out])
//   [optional subregion]
//
// Then 4 consecutive channel blocks in order: R, G, B, A.
//
// Each channel block contains:
//
//   funcType (u32 enum)
//   p0 (f32)
//   p1 (f32)
//   p2 (f32)
//   tableCount (u32)
//   table[tableCount] (f32)
//
// Interpretation:
//
//   IDENTITY:
//       tableCount == 0
//
//   TABLE / DISCRETE:
//       tableCount > 0
//
//   LINEAR:
//       p0 = slope
//       p1 = intercept
//
//   GAMMA:
//       p0 = amplitude
//       p1 = exponent
//       p2 = offset
//
// ----------------------------------------------------------------------------
//
// FOP_CONVOLVE_MATRIX (feConvolveMatrix):
//
//   IO prefix (in1, [out])
//   [optional subregion]
//
//   order (u32x2: orderX, orderY)
//   kernel[orderX * orderY] (f32)
//   divisor (f32)
//   bias (f32)
//   target (u32x2: targetX, targetY)
//   edgeMode (u32 enum)
//   kernelUnitLengthX (f32)
//   kernelUnitLengthY (f32)
//   preserveAlpha (u32 low bit)
//
// ----------------------------------------------------------------------------
//
// FOP_DISPLACEMENT_MAP (feDisplacementMap):
//
//   IO prefix (in1, in2, [out])
//   [optional subregion]
//   scale (f32)
//   xChannel (u32 enum)
//   yChannel (u32 enum)
//
// ----------------------------------------------------------------------------
//
// FOP_FLOOD (feFlood):
//
//   IO prefix (in1 unused, [out])
//   [optional subregion]
//   rgba32Premul (u32)
//
// ----------------------------------------------------------------------------
//
// FOP_IMAGE (feImage):
//
//   IO prefix (in1 unused, [out])
//   [optional subregion]
//   imageKey (InternedKey)
//
// imageKey is a resolved engine-level image handle or image name.
// This remains an InternedKey because it is not a closed vocabulary.
//
// ----------------------------------------------------------------------------
//
// FOP_MERGE (feMerge):
//
//   IO prefix (in1 unused, [out])
//   [optional subregion]
//   inputCount (u32)
//   inputKey[inputCount] (InternedKey)
//
// ----------------------------------------------------------------------------
//
// FOP_MORPHOLOGY (feMorphology):
//
//   IO prefix (in1, [out])
//   [optional subregion]
//   operator (u32 enum)
//   radiusX (f32)
//   radiusY (f32)
//
// ----------------------------------------------------------------------------
//
// FOP_TILE (feTile):
//
//   IO prefix (in1, [out])
//   [optional subregion]
//
//   No additional parameters.
//
// ----------------------------------------------------------------------------
//
// FOP_TURBULENCE (feTurbulence):
//
//   IO prefix (in1 unused, [out])
//   [optional subregion]
//   type (u32 enum)
//   baseFreqX (f32)
//   baseFreqY (f32)
//   numOctaves (u32)
//   seed (f32)
//   stitchTiles (u32 low bit)
//
// ----------------------------------------------------------------------------
//
// FOP_DIFFUSE_LIGHTING (feDiffuseLighting):
//
//   IO prefix (in1, [out])
//   [optional subregion]
//   lightingColorRGBA32 (u32)
//   surfaceScale (f32)
//   diffuseConstant (f32)
//   kernelUnitLengthX (f32)
//   kernelUnitLengthY (f32)
//   lightType (u32 enum)
//   L0..L7 (8 x f32)
//
// ----------------------------------------------------------------------------
//
// FOP_SPECULAR_LIGHTING (feSpecularLighting):
//
//   IO prefix (in1, [out])
//   [optional subregion]
//   lightingColorRGBA32 (u32)
//   surfaceScale (f32)
//   specularConstant (f32)
//   specularExponent (f32)
//   kernelUnitLengthX (f32)
//   kernelUnitLengthY (f32)
//   lightType (u32 enum)
//   L0..L7 (8 x f32)
//
// ----------------------------------------------------------------------------
// LIGHT PAYLOAD INTERPRETATION
// ----------------------------------------------------------------------------
//
// lightType == FILTER_LIGHT_DISTANT:
//   L0 = azimuth
//   L1 = elevation
//   L2..L7 unused
//
// lightType == FILTER_LIGHT_POINT:
//   L0 = x
//   L1 = y
//   L2 = z
//   L3..L7 unused
//
// lightType == FILTER_LIGHT_SPOT:
//   L0 = x
//   L1 = y
//   L2 = z
//   L3 = pointsAtX
//   L4 = pointsAtY
//   L5 = pointsAtZ
//   L6 = specularExponent
//   L7 = limitingConeAngle
//
// ----------------------------------------------------------------------------
//
// FOP_DROP_SHADOW (feDropShadow):
//
//   IO prefix (in1, [out])
//   [optional subregion]
//   dx (f32)
//   dy (f32)
//   stdDevX (f32)
//   stdDevY (f32)
//   rgba32Premul (u32)
//
// ============================================================================
