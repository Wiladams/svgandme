#pragma once

#include "bit_hacks.h"

/*
    Bitwise operators

    Provide bitwise operators as functions.  
    For those places where you need a function pointer 
    representing bitwise operators.
    Also, in places where you want to check the parameters
    and offer a consistent behavior for out-of-range shift counts, etc.


    bnot
    band
    bor
    bxor
    lshift
    rshift
    *arshift
    rol
    ror
    bswap

*/

namespace waavs
{
    constexpr INLINE uint64_t bop_swap64(uint64_t v) noexcept {
        return ((v & ((uint64_t)0xff << (7 * 8))) >> (7 * 8)) |
            ((v & ((uint64_t)0xff << (6 * 8))) >> (5 * 8)) |
            ((v & ((uint64_t)0xff << (5 * 8))) >> (3 * 8)) |
            ((v & ((uint64_t)0xff << (4 * 8))) >> (1 * 8)) |
            ((v & ((uint64_t)0xff << (3 * 8))) << (1 * 8)) |
            ((v & ((uint64_t)0xff << (2 * 8))) << (3 * 8)) |
            ((v & ((uint64_t)0xff << (1 * 8))) << (5 * 8)) |
            ((v & ((uint64_t)0xff << (0 * 8))) << (7 * 8));

    }

    constexpr INLINE uint32_t bop_swap32(uint32_t v) noexcept {
        return (
            (v & 0xff000000) >> 24) | 
            ((v & 0x00ff0000) >> 8) |
            ((v & 0x0000ff00) << 8) | 
            ((v & 0x000000ff) << 24);
    }

    constexpr INLINE uint16_t bop_swap16(uint16_t a) noexcept { 
        return (
            a >> 8) | 
            (a << 8); 
    }
}

namespace waavs
{

    // 16-bit versions
    //
    constexpr INLINE uint16_t bop_not_u16(uint16_t a) noexcept { return ~a; }
    constexpr INLINE uint16_t bop_and_u16(uint16_t a, uint16_t b) noexcept { return a & b; }
    constexpr INLINE uint16_t bop_or_u16(uint16_t a, uint16_t b) noexcept { return a | b; }
    constexpr INLINE uint16_t bop_xor_u16(uint16_t a, uint16_t b) noexcept { return a ^ b; }
    constexpr INLINE uint16_t bop_lshift_u16(uint16_t a, unsigned int nbits) noexcept { return nbits < 16u ? uint16_t(uint32_t(a) << nbits) : uint16_t(0); }
    constexpr INLINE uint16_t bop_rshift_u16(uint16_t a, unsigned int nbits) noexcept { return nbits < 16u ? uint16_t(uint32_t(a) >> nbits) : uint16_t(0); }
    constexpr INLINE int16_t bop_arshift_16(int16_t a, unsigned int nbits) noexcept { return nbits < 16u ? int16_t(a >> nbits) : int16_t(a < 0 ? -1 : 0); }
    constexpr INLINE uint16_t bop_rol_u16(uint16_t a, unsigned int n) noexcept
    {
        n &= 15u;
        const uint32_t v = uint32_t(a);
        return uint16_t((v << n) | (v >> ((16u - n) & 15u)));
    }

    constexpr INLINE uint16_t bop_ror_u16(uint16_t a, unsigned int n) noexcept
    {
        n &= 15u;
        const uint32_t v = uint32_t(a);
        return uint16_t((v >> n) | (v << ((16u - n) & 15u)));
    }

    // 32-bit versions
    //
    constexpr INLINE uint32_t bop_not_u32(uint32_t a) noexcept { return ~a; }
    constexpr INLINE uint32_t bop_and_u32(uint32_t a, uint32_t b) noexcept { return a & b; }
    constexpr INLINE uint32_t bop_or_u32(uint32_t a, uint32_t b) noexcept { return a | b; }
    constexpr INLINE uint32_t bop_xor_u32(uint32_t a, uint32_t b) noexcept { return a ^ b; }
    constexpr INLINE uint32_t bop_lshift_u32(uint32_t a, unsigned int nbits) noexcept { return a << nbits; }
    constexpr INLINE uint32_t bop_rshift_u32(uint32_t a, unsigned int nbits) noexcept { return a >> nbits; }
    constexpr INLINE int32_t  bop_arshift_32(int32_t a, unsigned int nbits) noexcept { return a >> nbits; }
    constexpr INLINE uint32_t bop_rol_u32(uint32_t a, unsigned int n) noexcept { return ((a << n) | (a >> (32 - n))); }
    constexpr INLINE uint32_t bop_ror_u32(uint32_t a, unsigned int n) noexcept { return ((a << (32 - n)) | (a >> n)); }



    // 64-bit versions
    // Ensure n is within 0-63 to avoid undefined behavior
    constexpr INLINE uint64_t bop_not_u64(uint64_t a) noexcept { return ~a; }
    constexpr INLINE uint64_t bop_and_u64(uint64_t a, uint64_t b) noexcept { return a & b; }
    constexpr INLINE uint64_t bop_or_u64(uint64_t a, uint64_t b) noexcept { return a | b; }
    constexpr INLINE uint64_t bop_xor_u64(uint64_t a, uint64_t b) noexcept { return a ^ b; }
    constexpr INLINE uint64_t bop_lshift_u64(uint64_t a, unsigned int nbits) noexcept { return nbits < 64u ? (a << nbits) : 0u; }
    constexpr INLINE uint64_t bop_rshift_u64(uint64_t a, unsigned int nbits) noexcept { return nbits < 64u ? (a >> nbits) : 0u; }
    // Assumes two's-complement integers and arithmetic right shift for signed values.
    // This holds for the supported platforms: macOS, Windows, Linux, iOS, Android.
    constexpr INLINE int64_t bop_arshift_64(int64_t a, unsigned int nbits) noexcept { return nbits < 64u ? (a >> nbits) : (a < 0 ? int64_t(-1) : int64_t(0)); }
    constexpr INLINE uint64_t bop_rol_u64(uint64_t a, unsigned int n) noexcept { n &= 63u;  return (a << n) | (a >> ((64u - n) & 63u)); }
    constexpr INLINE uint64_t bop_ror_u64(uint64_t a, unsigned int n) noexcept {
        n &= 63u; // Ensure n is within 0-63 to avoid undefined behavior
        return (a >> n) | (a << ((64u - n) & 63u));
    }
}