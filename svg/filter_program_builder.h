// filter_program_builder.h

#pragma once

#include "filter_program.h"
#include "nametable.h"
#include "svgdatatypes.h"
#include "surface.h"



// Converting from string keys to enum values
// for filter attributes that have a closed vocabulary
namespace waavs
{

    namespace waavs
    {
        static INLINE FilterColorInterpolation parseFilterColorInterpolation(InternedKey k) noexcept
        {
            // Filter effects default to linearRGB.
            if (!k) return FILTER_COLOR_INTERPOLATION_LINEAR_RGB;

            if (k == filter::kColorInterp_linearRGB()) return FILTER_COLOR_INTERPOLATION_LINEAR_RGB;
            if (k == filter::kColorInterp_sRGB())      return FILTER_COLOR_INTERPOLATION_SRGB;

            // If you later admit "auto" at parse time, collapse it here for now.
            // if (k == filter::kColorInterp_auto()) return FILTER_COLOR_INTERPOLATION_LINEAR_RGB;

            return FILTER_COLOR_INTERPOLATION_LINEAR_RGB;
        }
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
        return resultKey ? resultKey : filter::Filter_Last();
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

        return argb32_unpack_alpha_norm(px);
    }

    // computeHeightNormal()
    //
    // Computes the normal vector at pixel (x,y) by sampling the alpha channel of
    //
    static INLINE void computeHeightNormal(const Surface& s,
        int x, int y,
        float surfaceScale,
        float dux, float duy,
        float& nx, float& ny, float& nz) noexcept
    {
        const float hL = pixelHeightFromAlpha(s, x - 1, y);
        const float hR = pixelHeightFromAlpha(s, x + 1, y);
        const float hU = pixelHeightFromAlpha(s, x, y - 1);
        const float hD = pixelHeightFromAlpha(s, x, y + 1);

        float dHx = 0.0f;
        float dHy = 0.0f;

        if (dux > 0.0f)
            dHx = (hR - hL) / (2.0f * dux);

        if (duy > 0.0f)
            dHy = (hD - hU) / (2.0f * duy);

        nx = -surfaceScale * dHx;
        ny = -surfaceScale * dHy;
        nz = 1.0f;

        const float len2 = nx * nx + ny * ny + nz * nz;
        if (len2 > 1e-20f)
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
