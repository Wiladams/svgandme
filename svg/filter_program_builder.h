// filter_program_builder.h

#pragma once

#include "filter_program.h"
#include "nametable.h"
#include "svgdatatypes.h"


// --------------------------------------
// Encoder Helpers
// 
// Helpers to pack and unpack various types into the 64-bit mem words of the filter program.
// --------------------------------------
namespace waavs
{
    // Encode/decode u32 <-> u64 by storing the u32 in the low 32 bits of the u64.
    static INLINE uint64_t u64_from_u32(uint32_t v) noexcept
    {
        return (uint64_t)v;
    }
    
    static INLINE uint32_t u32_from_u64(uint64_t v) noexcept
    {
        return (uint32_t)(v & 0xFFFFFFFFu);
    }

    // Encode two u32 values into a single u64, with 'a' in the low 32 bits and 'b' in the high 32 bits.
    static INLINE uint64_t u64_pack_u32x2(uint32_t a, uint32_t b) noexcept
    {
        return (uint64_t)a | ((uint64_t)b << 32);
    }
    
    static INLINE void u32x2_from_u64(uint64_t v, uint32_t& a, uint32_t& b) noexcept
    {
        a = (uint32_t)(v & 0xFFFFFFFFu);
        b = (uint32_t)(v >> 32);
    }

    // Encode a float as a uint64_t with the float bits in the low 32 bits.
    static INLINE uint64_t u64_from_f32(float f) noexcept
    {
        static_assert(sizeof(float) == 4, "Expected float to be 32 bits");
        uint32_t u{};
        memcpy(&u, &f, 4);
        return (uint64_t)u;
    }

    // Encode/decode a float stored in the low 32 bits of a uint64_t.
    static INLINE float f32_from_u64(uint64_t v) noexcept
    {
        uint32_t u = (uint32_t)(v & 0xFFFFFFFFu);
        float f{};
        memcpy(&f, &u, 4);
        return f;
    }
    
    static INLINE double f64_from_u64(uint64_t v) noexcept
    {
        double d{};
        memcpy(&d, &v, 8);
        return d;
    }

    // Encode a double as a uint64_t with the double bits in the full 64 bits.
    static INLINE uint64_t u64_from_f64(double d) noexcept
    {
        uint64_t u{};
        static_assert(sizeof(double) == 8, "double must be 64-bit");
        memcpy(&u, &d, 8);

        return u;
    }

    // pack/unpack InternedKey as a uint64_t by storing the pointer bits.
    static INLINE uint64_t u64_from_key(InternedKey k) noexcept
    {
        // InternedKey is a pointer type; store its bits.
        static_assert(sizeof(uintptr_t) <= sizeof(uint64_t), "pointer must fit in 64 bits");

        uintptr_t p = (uintptr_t)k;
        return (uint64_t)p;
    }

    static INLINE InternedKey key_from_u64(uint64_t v) noexcept
    {
        return (InternedKey)(uintptr_t)v;
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


    // Unpacking a packed number or percentage
    static INLINE SVGNumberOrPercent unpackNumberOrPercent(uint64_t packed) noexcept
    {
        SVGNumberOrPercent out{};
        out.fIsSet = packedNumberOrPercentIsSet(packed);
        out.fIsPercent = packedNumberOrPercentIsPercent(packed);

        uint32_t lo = uint32_t(packed & 0xFFFFFFFFu);
        float fv = 0.0f;
        memcpy(&fv, &lo, sizeof(fv));
        out.fValue = double(fv);

        return out;
    }

    // ------------------------------------
    // Emitters for common operand types
    // ------------------------------------
    static INLINE void emit_u64(FilterProgramStream& out, uint64_t v) noexcept
    {
        out.mem.push_back(v);
    }

    static INLINE void emit_u32(FilterProgramStream& out, uint32_t v) noexcept
    {
        emit_u64(out, u64_from_u32(v));
    }

    static INLINE void emit_f32(FilterProgramStream& out, float v) noexcept
    {
        emit_u64(out, u64_from_f32(v));
    }

    static INLINE void emit_key(FilterProgramStream& out, InternedKey k) noexcept
    {
        emit_u64(out, u64_from_key(k));
    }

    static INLINE void emit_u32x2(FilterProgramStream& out, uint32_t a, uint32_t b) noexcept
    {
        emit_u64(out, u64_pack_u32x2(a, b));
    }

    // For inline lists
    static INLINE void emit_f32_list(FilterProgramStream& out, const float* vals, uint32_t count) noexcept
    {
        for (uint32_t i = 0; i < count; ++i)
            emit_u64(out, u64_from_f32(vals[i]));
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
            emit_u64(out, u64_from_key(vals[i]));
    }

    static INLINE void emit_ColorSRGB(FilterProgramStream& out, const ColorSRGB& c) noexcept
    {
        // We carry color as a straight sRGB color, 
        // so we just emit that directly
        emit_f32(out, c.r);
        emit_f32(out, c.g);
        emit_f32(out, c.b);
        emit_f32(out, c.a);
    }

    static INLINE InternedKey finishLast(InternedKey resultKey) noexcept
    {
        return resultKey ? resultKey : filter::Filter_Last();
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
//   srgba (ColorSRGB)
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
