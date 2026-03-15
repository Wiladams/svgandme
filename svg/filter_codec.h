#pragma once

// Data structures and routines to support filter meta information
// in a filter program.  These structures and routines facilitate
// going between in-memory representation of filter information
// and the encoded filter program serialized form.
// This helps keep the ABI in one place with respect to those filters

#include "filterprogram.h"

namespace waavs
{
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
 
struct FilterGaussianBlurParams
{
    float stdDevX;
    float stdDevY;
};

bool encodeGaussianBlur(FilterProgramBuilder& builder,const FilterGaussianBlurParams& params) noexcept;
{
}

bool decodeGaussianBlur(const FilterProgramStream& stream, size_t& ip, FilterIO& io, WGRectD* subr, FilterGaussianBlurParams& params) noexcept;
{
}

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
}