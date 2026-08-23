// bit_bops.h

#pragma once

#include "bit_hacks.h"

/*
    Bitwise operators

    Provide bitwise operators as functions.  
    What's the use of these?  First, when performing bitwise
    operations, and you want them to be range checked.
    Most typically, when developing a language, and you want to 
    standardize the implementation of bitwise operators

    Routines provided for 8, 16, 32, 64-bit integer types
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
    // ----- Byte-swap functions -----
    //
    // All three functions implement the same pattern:
    // For each byte position i (0 = LSB), the byte at offset i is moved
    // to offset (N-1-i), where N is the number of bytes in the type.
    // The shift amount is ((N-1-i) - i) * 8; positive means left shift,
    // negative means right shift.  Using explicit multiplication by 8
    // makes the relationship between indices and bit positions clear.
    //
    // Example (32-bit): byte 3 (MSB) goes to byte 0 (>> 24),
    //                  byte 2 goes to byte 1 (>> 8),
    //                  byte 1 goes to byte 2 (<< 8),
    //                  byte 0 goes to byte 3 (<< 24).

    constexpr uint16_t bop_swap16(uint16_t v) noexcept {
        return ((v & ((uint16_t)0xff << (1 * 8))) >> (1 * 8)) |     // byte 1 -> byte 0
            ((v & ((uint16_t)0xff << (0 * 8))) << (1 * 8));         // byte 0 -> byte 1
    }

    constexpr uint32_t bop_swap32(uint32_t v) noexcept {
        return ((v & ((uint32_t)0xff << (3 * 8))) >> (3 * 8)) |     // byte 3 -> byte 0
            ((v & ((uint32_t)0xff << (2 * 8))) >> (1 * 8)) |        // byte 2 -> byte 1
            ((v & ((uint32_t)0xff << (1 * 8))) << (1 * 8)) |        // byte 1 -> byte 2
            ((v & ((uint32_t)0xff << (0 * 8))) << (3 * 8));         // byte 0 -> byte 3
    }

    constexpr  uint64_t bop_swap64(uint64_t v) noexcept {
        return ((v & ((uint64_t)0xff << (7 * 8))) >> (7 * 8)) |
            ((v & ((uint64_t)0xff << (6 * 8))) >> (5 * 8)) |
            ((v & ((uint64_t)0xff << (5 * 8))) >> (3 * 8)) |
            ((v & ((uint64_t)0xff << (4 * 8))) >> (1 * 8)) |
            ((v & ((uint64_t)0xff << (3 * 8))) << (1 * 8)) |
            ((v & ((uint64_t)0xff << (2 * 8))) << (3 * 8)) |
            ((v & ((uint64_t)0xff << (1 * 8))) << (5 * 8)) |
            ((v & ((uint64_t)0xff << (0 * 8))) << (7 * 8));

    }

    constexpr  uint32_t bop_swap32(uint32_t v) noexcept {
        return (
            (v & 0xff000000) >> 24) | 
            ((v & 0x00ff0000) >> 8) |
            ((v & 0x0000ff00) << 8) | 
            ((v & 0x000000ff) << 24);
    }

    constexpr  uint16_t bop_swap16(uint16_t a) noexcept { 
        return (
            a >> 8) | 
            (a << 8); 
    }
}

namespace waavs
{
    // 8-bit versions
    // 
    constexpr  uint8_t bop_not_u8(uint8_t a) noexcept { return ~a; }
    constexpr  uint8_t bop_and_u8(uint8_t a, uint8_t b) noexcept { return a & b; }
    constexpr  uint8_t bop_or_u8(uint8_t a, uint8_t b) noexcept { return a | b; }
    constexpr  uint8_t bop_xor_u8(uint8_t a, uint8_t b) noexcept { return a ^ b; }
    constexpr  uint8_t bop_lshift_u8(uint8_t a, unsigned int nbits) noexcept {
        return nbits < 8u ? uint8_t(uint16_t(a) << nbits) : uint8_t(0);
    }
    constexpr  uint8_t bop_rshift_u8(uint8_t a, unsigned int nbits) noexcept {
        return nbits < 8u ? uint8_t(uint16_t(a) >> nbits) : uint8_t(0);
    }
    constexpr  int8_t bop_arshift_8(int8_t a, unsigned int nbits) noexcept {
        return nbits < 8u ? int8_t(a >> nbits) : (a < 0 ? -1 : 0);
    }
    constexpr  uint8_t bop_rol_u8(uint8_t a, unsigned int n) noexcept {
        n &= 7u;
        return uint8_t((uint16_t(a) << n) | (uint16_t(a) >> ((8u - n) & 7u)));
    }
    constexpr  uint8_t bop_ror_u8(uint8_t a, unsigned int n) noexcept {
        n &= 7u;
        return uint8_t((uint16_t(a) >> n) | (uint16_t(a) << ((8u - n) & 7u)));
    }


    // 16-bit versions
    //
    constexpr  uint16_t bop_not_u16(uint16_t a) noexcept { return ~a; }
    constexpr  uint16_t bop_and_u16(uint16_t a, uint16_t b) noexcept { return a & b; }
    constexpr  uint16_t bop_or_u16(uint16_t a, uint16_t b) noexcept { return a | b; }
    constexpr  uint16_t bop_xor_u16(uint16_t a, uint16_t b) noexcept { return a ^ b; }
    constexpr  uint16_t bop_lshift_u16(uint16_t a, unsigned int nbits) noexcept { return nbits < 16u ? uint16_t(uint32_t(a) << nbits) : uint16_t(0); }
    constexpr  uint16_t bop_rshift_u16(uint16_t a, unsigned int nbits) noexcept { return nbits < 16u ? uint16_t(uint32_t(a) >> nbits) : uint16_t(0); }
    constexpr  int16_t bop_arshift_16(int16_t a, unsigned int nbits) noexcept { return nbits < 16u ? int16_t(a >> nbits) : int16_t(a < 0 ? -1 : 0); }
    constexpr  uint16_t bop_rol_u16(uint16_t a, unsigned int n) noexcept
    {
        n &= 15u;
        const uint32_t v = uint32_t(a);
        return uint16_t((v << n) | (v >> ((16u - n) & 15u)));
    }

    constexpr  uint16_t bop_ror_u16(uint16_t a, unsigned int n) noexcept
    {
        n &= 15u;
        const uint32_t v = uint32_t(a);
        return uint16_t((v >> n) | (v << ((16u - n) & 15u)));
    }

    // 32-bit versions
    //
    constexpr  uint32_t bop_not_u32(uint32_t a) noexcept { return ~a; }
    constexpr  uint32_t bop_and_u32(uint32_t a, uint32_t b) noexcept { return a & b; }
    constexpr  uint32_t bop_or_u32(uint32_t a, uint32_t b) noexcept { return a | b; }
    constexpr  uint32_t bop_xor_u32(uint32_t a, uint32_t b) noexcept { return a ^ b; }
    constexpr  uint32_t bop_lshift_u32(uint32_t a, unsigned int nbits) noexcept { return nbits < 32u ? (a << nbits) : 0u; }
    constexpr  uint32_t bop_rshift_u32(uint32_t a, unsigned int nbits) noexcept { return nbits < 32u ? (a >> nbits) : 0; }
    constexpr  int32_t  bop_arshift_32(int32_t a, unsigned int nbits) noexcept { return nbits <32u ? (a >> nbits) : (a<0 ? -1 : 0); }
    constexpr  uint32_t bop_rol_u32(uint32_t a, unsigned int n) noexcept {
        n &= 31u;
        return (a << n) | (a >> ((32u - n) & 31u));
    }
    constexpr  uint32_t bop_ror_u32(uint32_t a, unsigned int n) noexcept {
        n &= 31u;
        return (a >> n) | (a << ((32u - n) & 31u));
    }


    // 64-bit versions
    // Ensure n is within 0-63 to avoid undefined behavior
    constexpr  uint64_t bop_not_u64(uint64_t a) noexcept { return ~a; }
    constexpr  uint64_t bop_and_u64(uint64_t a, uint64_t b) noexcept { return a & b; }
    constexpr  uint64_t bop_or_u64(uint64_t a, uint64_t b) noexcept { return a | b; }
    constexpr  uint64_t bop_xor_u64(uint64_t a, uint64_t b) noexcept { return a ^ b; }
    constexpr  uint64_t bop_lshift_u64(uint64_t a, unsigned int nbits) noexcept { return nbits < 64u ? (a << nbits) : 0u; }
    constexpr  uint64_t bop_rshift_u64(uint64_t a, unsigned int nbits) noexcept { return nbits < 64u ? (a >> nbits) : 0u; }
    // Assumes two's-complement integers and arithmetic right shift for signed values.
    // This holds for the supported platforms: macOS, Windows, Linux, iOS, Android.
    constexpr  int64_t bop_arshift_64(int64_t a, unsigned int nbits) noexcept { return nbits < 64u ? (a >> nbits) : (a < 0 ? int64_t(-1) : int64_t(0)); }
    constexpr  uint64_t bop_rol_u64(uint64_t a, unsigned int n) noexcept { n &= 63u;  return (a << n) | (a >> ((64u - n) & 63u)); }
    constexpr  uint64_t bop_ror_u64(uint64_t a, unsigned int n) noexcept {
        n &= 63u; // Ensure n is within 0-63 to avoid undefined behavior
        return (a >> n) | (a << ((64u - n) & 63u));
    }
}