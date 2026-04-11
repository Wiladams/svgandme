#pragma once

#include "definitions.h"

#include <vector>

#include "filter_types.h"



namespace waavs
{
    typedef uint16_t FilterOpType;
    typedef uint64_t FilterMemWord;

    // These bits help describe various policies 
    // and behaviors of the filter program, such 
    // as how to handle missing inputs, how to 
    // initialize output tiles, etc.
    enum FilterPolicyBits : uint32_t
    {
        FPP_NONE = 0,

        // arity / source model
        FPP_GENERATOR = 1u << 0,
        FPP_MULTI_INPUT = 1u << 1,

        // output initialization
        FPP_INIT_CLEAR = 1u << 2,
        FPP_INIT_COPY_PRIMARY = 1u << 3,
        FPP_INIT_NOINIT = 1u << 4,

        // input handling
        FPP_IN1_IMPLICIT_LAST = 1u << 5,
        FPP_IN2_IMPLICIT_SOURCE = 1u << 6,
        FPP_MISSING_INPUT_FAIL = 1u << 7,
        FPP_MISSING_INPUT_FALLBACK = 1u << 8,
        FPP_MISSING_INPUT_IGNORE = 1u << 9,

        // write behavior
        FPP_PRESERVE_UNTOUCHED = 1u << 10,
        FPP_FULL_TILE_WRITE = 1u << 11,
        FPP_BINARY_INTERSECTION = 1u << 12
    };


    // An operation code is a combination of an opcode id
    // any operation modifiers, which are attached as flags 
    // in the upper bits of the opcode.
    // Current representation is 8-bits.  If we find the need for
    // more opcodes, or more bits, then this will need to expand to 
    // 16-bit, or 32-bit as required.
    // lower 5 bits (bit positions 0..4): opcode id, value range [0..31]
    // upper 3 bits (bit positions 5..7): flags 
    enum : uint8_t {
        FOP_ID_MASK = 0x1F,
        FOP_FLAG_MASK = 0xE0,
        FOP_FLAG_SHIFT = 5,

        // Common flags used by multiple ops
        FOPF_HAS_OUT = 1u << 5,  // out key is present
        FOPF_HAS_IN2 = 1u << 6,  // in2 key is present (for 2-input ops)
        FOPF_HAS_SUBR = 1u << 7,  // primitive subregion payload present (future)
    };

    // FilterOpId, these are the actual opcodes for the filter program
    // without any modifiers
    enum FilterOpId : FilterOpType {
        FOP_END = 0,                // indicates end of program; not an actual filter primitive

        // Inputs / sources
        // (These are usually “virtual” sources in SVG: SourceGraphic, SourceAlpha, etc.
        //  You may not need explicit ops for them; keep as comments unless you want them.)
        // FOP_SOURCE_GRAPHIC,
        // FOP_SOURCE_ALPHA,
        // FOP_BACKGROUND_IMAGE,
        // FOP_BACKGROUND_ALPHA,
        // FOP_FILL_PAINT,
        // FOP_STROKE_PAINT,

        // Core primitives (common)
        FOP_BLEND,                 // feBlend
        FOP_COLOR_MATRIX,          // feColorMatrix
        FOP_COMPONENT_TRANSFER,    // feComponentTransfer (compound: contains feFuncR/G/B/A)
        FOP_COMPOSITE,             // feComposite
        FOP_CONVOLVE_MATRIX,       // feConvolveMatrix
        FOP_DIFFUSE_LIGHTING,      // feDiffuseLighting (compound: contains light element)
        FOP_DISPLACEMENT_MAP,      // feDisplacementMap
        FOP_FLOOD,                 // feFlood
        FOP_GAUSSIAN_BLUR,         // feGaussianBlur
        FOP_IMAGE,                 // feImage
        FOP_MERGE,                 // feMerge (compound: contains feMergeNode list)
        FOP_MORPHOLOGY,            // feMorphology
        FOP_OFFSET,                // feOffset
        FOP_SPECULAR_LIGHTING,     // feSpecularLighting (compound: contains light element)
        FOP_TILE,                  // feTile
        FOP_TURBULENCE,            // feTurbulence

        // Less common / SVG 2 additions that still appear in the wild
        FOP_DROP_SHADOW,           // feDropShadow (SVG 2; common in authoring tools)
    };


    // Helpers to pack and unpack opcodes and flags into the ops vector of the filter program.
    // Pack an opcode id and optional flags into a single byte for the ops vector.
    static INLINE FilterOpType packOp(FilterOpId id, uint8_t flags = 0) noexcept
    {
        return (FilterOpType)((id & FOP_ID_MASK) | (flags & FOP_FLAG_MASK));
    }

    // Unpack just the opcode id from a packed op byte.
    static INLINE FilterOpId opId(FilterOpType op) noexcept
    {
        return (FilterOpId)(op & FOP_ID_MASK);
    }

    // Unpack just the flags from a packed op byte.
    static INLINE FilterOpType opFlags(FilterOpType op) noexcept
    {
        return (FilterOpType)(op & FOP_FLAG_MASK);
    }

    static INLINE bool opHasOut(FilterOpType op) noexcept
    {
        return (op & FOPF_HAS_OUT) != 0;
    }

    static INLINE bool opHasIn2(FilterOpType op) noexcept
    {
        return (op & FOPF_HAS_IN2) != 0;
    }

    static INLINE bool opHasSubregion(FilterOpType op) noexcept
    {
        return (op & FOPF_HAS_SUBR) != 0;
    }

    // Representation of a filter program 
    // This is essentially the machine "architecture"
    // Op codes are uint8_t, and have a couple of flag bits available for use by the op.
    // The mem vector is for the operands, which are 64-bit words that can be used to 
    // store numbers, pointers, or packed pairs of smaller values.
    struct FilterProgramStream
    {
        std::vector<FilterOpType>  ops;   // packed opcode+flags
        std::vector<FilterMemWord> mem;   // 64-bit words (numbers, pointers, packed pairs)

        // Filter-level coordinate system controls:
        SpaceUnitsKind filterUnits{ SpaceUnitsKind::SVG_SPACE_OBJECT }; // default objectBoundingBox
        SpaceUnitsKind primitiveUnits{ SpaceUnitsKind::SVG_SPACE_USER }; // default userSpaceOnUse


        void clear() { 
            ops.clear(); 
            mem.clear(); 
            filterUnits = SpaceUnitsKind::SVG_SPACE_OBJECT;
            primitiveUnits = SpaceUnitsKind::SVG_SPACE_USER;
        }
    };

    struct FilterProgramCursor
    {
        const FilterProgramStream* prog{};
        size_t opi{ 0 };
        size_t mi{ 0 };

        explicit FilterProgramCursor(const FilterProgramStream& p) : prog(&p) {}

        // Retrieve the next operation byte
        // advance the program counter
        // Return:
        //  false if we've reached the end of the ops vector
        // If we've encountered program end (FOP_END), we'll still
        // return true, and expect the caller to handle that condition.
        // 
        bool next(FilterOpType& op) noexcept
        {
            if (!prog || opi >= prog->ops.size()) 
                return false;
            
            op = prog->ops[opi++];
            
            return true;
        }

        // Retrieve the next operand word from mem, advance the mem counter.
        // Caller is responsible for ensuring that the operand type matches 
        // the expected type for the current op.
        uint64_t take() noexcept
        {
            // do a hard halt if we run past end of memory
            WAAVS_ASSERT(prog && mi < prog->mem.size());
            if (!prog || mi >= prog->mem.size()) 
                return 0;

            return prog->mem[mi++];
        }
    };


    // Helpers to pack and unpack various types into the 64-bit mem words of the filter program.
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


    static INLINE uint64_t u64_from_f32(float f) noexcept
    {
        static_assert(sizeof(float) == 4, "Expected float to be 32 bits");
        uint32_t u{};
        memcpy(&u, &f, 4);
        return (uint64_t)u;
    }

    static INLINE float f32_from_u64(uint64_t v) noexcept
    {
        uint32_t u = (uint32_t)(v & 0xFFFFFFFFu);
        float f{};
        memcpy(&f, &u, 4);
        return f;
    }

    static INLINE uint32_t u32_from_u64(uint64_t v) noexcept
    {
        return (uint32_t)(v & 0xFFFFFFFFu);
    }

    static INLINE uint64_t u64_from_f64(double d) noexcept
    {
        uint64_t u{};
        static_assert(sizeof(double) == 8, "double must be 64-bit");
        memcpy(&u, &d, 8);

        return u;
    }

    static INLINE double f64_from_u64(uint64_t v) noexcept
    {
        double d{};
        memcpy(&d, &v, 8);
        return d;
    }


    static INLINE uint64_t u64_pack_u32x2(uint32_t a, uint32_t b) noexcept
    {
        return (uint64_t)a | ((uint64_t)b << 32);
    }

    static INLINE void u64_unpack_u32x2(uint64_t v, uint32_t& a, uint32_t& b) noexcept
    {
        a = (uint32_t)(v & 0xFFFFFFFFu);
        b = (uint32_t)(v >> 32);
    }
 
}




