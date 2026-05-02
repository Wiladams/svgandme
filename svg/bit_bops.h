#pragma once

#include "bit_hacks.h"

/*
    Binary operators

    Provide binary operators as functions.  For those places where
    you need a function pointer representing bitwise operators


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
    //static INLINE uint64_t bswap64(uint64_t a) {return _byteswap_uint64(a);}
    constexpr uint64_t bswap64(uint64_t v) {
        return ((v & ((uint64_t)0xff << (7 * 8))) >> (7 * 8)) |
            ((v & ((uint64_t)0xff << (6 * 8))) >> (5 * 8)) |
            ((v & ((uint64_t)0xff << (5 * 8))) >> (3 * 8)) |
            ((v & ((uint64_t)0xff << (4 * 8))) >> (1 * 8)) |
            ((v & ((uint64_t)0xff << (3 * 8))) << (1 * 8)) |
            ((v & ((uint64_t)0xff << (2 * 8))) << (3 * 8)) |
            ((v & ((uint64_t)0xff << (1 * 8))) << (5 * 8)) |
            ((v & ((uint64_t)0xff << (0 * 8))) << (7 * 8));

    }

    //static INLINE uint32_t bswap32(uint32_t a) {return _byteswap_ulong(a);}
    constexpr uint32_t bswap32(uint32_t v) noexcept {
        return ((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >> 8) |
            ((v & 0x0000ff00) << 8) | ((v & 0x000000ff) << 24);
    }

    //INLINE uint16_t bswap16(uint16_t a) {return _byteswap_ushort(a);}
    constexpr uint16_t bswap16(uint16_t a) noexcept { return (a >> 8) | (a << 8); }
}

namespace waavs
{

    // 16-bit versions
    //
    constexpr uint16_t bnot16(uint16_t a) noexcept { return ~a; }
    constexpr uint16_t band16(uint16_t a, uint16_t b) noexcept { return a & b; }
    constexpr uint16_t bor16(uint16_t a, uint16_t b) noexcept { return a | b; }
    constexpr uint16_t bxor16(uint16_t a, uint16_t b) noexcept { return a ^ b; }
    constexpr uint16_t lshift16(uint16_t a, unsigned int nbits) noexcept { return a << nbits; }
    constexpr uint16_t rshift16(uint16_t a, unsigned int nbits) noexcept { return a >> nbits; }
    constexpr int16_t arshift16(int16_t a, unsigned int nbits) noexcept { return a >> nbits; }
    constexpr uint16_t rol16(uint16_t a, unsigned int n) noexcept { return ((a << n) | (a >> (16 - n))); }
    constexpr uint16_t ror16(uint16_t a, unsigned int n) noexcept { return ((a << (16 - n)) | (a >> n)); }


    // 32-bit versions
    //
    constexpr uint32_t bop_not_u32(uint32_t a) noexcept { return ~a; }
    constexpr uint32_t bop_and_u32(uint32_t a, uint32_t b) noexcept { return a & b; }
    constexpr uint32_t bop_or_u32(uint32_t a, uint32_t b) noexcept { return a | b; }
    constexpr uint32_t bop_xor_u32(uint32_t a, uint32_t b) noexcept { return a ^ b; }
    constexpr uint32_t bop_lshift_u32(uint32_t a, unsigned int nbits) noexcept { return a << nbits; }
    constexpr uint32_t bop_rshift_u32(uint32_t a, unsigned int nbits) noexcept { return a >> nbits; }
    constexpr int32_t  bop_arshift_16(int32_t a, unsigned int nbits) noexcept { return a >> nbits; }
    constexpr uint32_t bop_rol_u32(uint32_t a, unsigned int n) noexcept { return ((a << n) | (a >> (32 - n))); }
    constexpr uint32_t bop_ror_u32(uint32_t a, unsigned int n) noexcept { return ((a << (32 - n)) | (a >> n)); }



    // 64-bit versions
    //
    constexpr INLINE uint64_t bnot64(uint64_t a) { return ~a; }
    constexpr uint64_t band64(uint64_t a, uint64_t b) { return a & b; }
    constexpr uint64_t bor64(uint64_t a, uint64_t b) { return a | b; }
    constexpr uint64_t bxor64(uint64_t a, uint64_t b) { return a ^ b; }
    constexpr uint64_t lshift64(uint64_t a, unsigned int nbits) { return a << nbits; }
    constexpr uint64_t rshift64(uint64_t a, unsigned int nbits) { return a >> nbits; }
    constexpr int64_t arshift16(int64_t a, unsigned int nbits) { return a >> nbits; }
    constexpr uint64_t rol64(uint64_t a, unsigned int n) { return ((a << n) | (a >> (64 - n))); }
    constexpr uint64_t ror64(uint64_t a, unsigned int n) { return ((a << (64 - n)) | (a >> n)); }
}