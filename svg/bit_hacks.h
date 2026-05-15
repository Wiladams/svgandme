// bithacks.h

#pragma once

#include "definitions.h"
#include <bit>

//
// Some of the routines here are inspired by the bit twiddling hacks
// http://graphics.stanford.edu/~seander/bithacks.html
//
// 
//
// Some bitwise manipulation routines
// Probably mostly not used
// isLE, and isBE are probably the most useful

namespace waavs
{
    // Determine which endianness the CPU has at runtime
    // little-endian (intel standard)
#if defined(__cpp_lib_endian) && __cpp_lib_endian >= 201907L
    static constexpr bool isLE() noexcept { return std::endian::native == std::endian::little; }
    static constexpr bool isBE() noexcept { return std::endian::native == std::endian::big; }
#else
    static constexpr bool isLE() noexcept { return __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__; }
    static constexpr bool isBE() noexcept { return __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__; }
#endif
}

namespace waavs
{
    // repl_u8_u16 (32, 64) - replicate a single uint8_t across all the
    // bytes of a larger unsigned int, returning that value
    constexpr uint16_t repl_u8_u16(const uint8_t x) { return static_cast<uint16_t>(x) * 0x0101u; }
    constexpr uint32_t repl_u8_u32(const uint8_t x) { return static_cast<uint32_t>(x) * 0x01010101u; }
    constexpr uint64_t repl_u8_u64(const uint8_t x) { return static_cast<uint64_t>(x) * 0x0101010101010101ull; }
    
    // Convert between integer types
    // going from u64 to others
    constexpr uint8_t  conv_u64_to_u8(const uint64_t x) noexcept { return static_cast<uint8_t>(x); }
    constexpr uint16_t conv_u64_to_u16(const uint64_t x) noexcept { return static_cast<uint16_t>(x); }
    constexpr uint32_t conv_u64_to_u32(const uint64_t x) noexcept { return static_cast<uint32_t>(x); }
    static INLINE double conv_u64_to_f64(uint64_t v) noexcept
    {
        double d{};
        memcpy(&d, &v, 8);
        return d;
    }

    // Going from others to u64
    constexpr uint64_t conv_u8_to_u64(uint8_t v) noexcept { return (uint64_t)v; }
    constexpr uint64_t conv_u16_to_u64(uint16_t v) noexcept { return (uint64_t)v; }
    constexpr uint64_t conv_u32_to_u64(uint32_t v) noexcept { return (uint64_t)v; }
    static INLINE uint64_t conv_f32_to_u64(float f) noexcept
    {
        static_assert(sizeof(float) == 4, "Expected float to be 32 bits");
        uint32_t u{};
        memcpy(&u, &f, 4);
        return (uint64_t)u;
    }
    
    static INLINE uint64_t conv_f64_to_u64(double d) noexcept
    {
        uint64_t u{};
        static_assert(sizeof(double) == 8, "double must be 64-bit");
        memcpy(&u, &d, sizeof(double));

        return u;
    }

}   








namespace waavs
{
    // ---------------------------------
    // bit_range_mask_u64
    // 
    // create a bitmask that represents the given range of bits, inclusive
    // bit numbers are 0 through 63, inclusive.  This is the basis for 
    // other masks.
    // typical ranges might be:
    //   low=0, high=7  == 0x00000000000000FF
    //   low=8, high=15 == 0x000000000000FF00
    //   low=0, high=15 == 0x000000000000FFFF
    //   low =24, high=31 == 0x0000000000FF000000
    //   low=63, high=63 == 0x8000000000000000
    // etc.
    // Returns 0 if the low and high values are invalid, 
    //  such as low > high, or high >= 64
    // 
    //  Discussion: 
    //   https://stackoverflow.com/questions/28035794/masking-bits-within-a-range-given-in-parameter-in-c
    //
    //  uint64_t mask = (uint64_t)1 << (high-low);
    //  mask <<= 1;
    //  mask--;         // turn on all the bits below
    //  mask <<= low;   // shift up to proper position
    //  return mask;

    static constexpr uint64_t bit_range_mask_u64(size_t low, size_t high) noexcept
    {
        if (low > high || high >= 64)
            return 0;

        const size_t width = high - low + 1;

        if (width == 64)
            return UINT64_MAX;

        return ((static_cast<uint64_t>(1) << width) - static_cast<uint64_t>(1)) << low;
    }

    static constexpr uint32_t bit_range_mask_u32(size_t low, size_t high) noexcept
    {
        if (high >= 32)
            return 0;
        return static_cast<uint32_t>(bit_range_mask_u64(low, high));
    }

    static constexpr uint16_t bit_range_mask_u16(size_t low, size_t high) noexcept
    {
        if (high >= 16)
            return 0;
        return static_cast<uint16_t>(bit_range_mask_u64(low, high));
    }

    static constexpr uint8_t bit_range_mask_u8(size_t low, size_t high) noexcept
    {
        if (high >= 8)
            return 0;
        return static_cast<uint8_t>(bit_range_mask_u64(low, high));
    }

    // ---------------------------------
    // bit range value.  Returns the value of the masked
    // bits of an integer, with the value shifted towards
    // the lowest bit, so they are 'plain' values
    // 
    // bit_range_value_u64
    //
    // core routine for the rest of the bit range value routines
    // is essentially the reverse of what the masking does
    static constexpr uint64_t bit_range_get_value_u64(uint64_t src, size_t lowbit, size_t highbit) noexcept
    {
        return (src & bit_range_mask_u64(lowbit, highbit)) >> lowbit;
    }

    static constexpr WGResult bit_range_set_value_u64(uint64_t src, size_t lowbit, size_t highbit, uint64_t value) noexcept
    {
        const uint64_t mask = bit_range_mask_u64(lowbit, highbit);
        return (src & ~mask) | ((value << lowbit) & mask);
    }

    static constexpr uint32_t bit_range_set_value_u32(
        uint32_t src,
        size_t lowbit,
        size_t highbit,
        uint32_t value) noexcept
    {
        if (highbit >= 32)
            return src;

        return static_cast<uint32_t>(
            bit_range_set_value_u64(src, lowbit, highbit, value));
    }

    static constexpr uint16_t bit_range_set_value_u16(
        uint16_t src,
        size_t lowbit,
        size_t highbit,
        uint16_t value) noexcept
    {
        if (highbit >= 16)
            return src;

        return static_cast<uint16_t>(
            bit_range_set_value_u64(src, lowbit, highbit, value));
    }

    static constexpr uint8_t bit_range_set_value_u8(
        uint8_t src,
        size_t lowbit,
        size_t highbit,
        uint8_t value) noexcept
    {
        if (highbit >= 8)
            return src;

        return static_cast<uint8_t>(
            bit_range_set_value_u64(src, lowbit, highbit, value));
    }

    // bit_is_set
    // bit_set
    // bit_clear
    // 
    // 

    static constexpr bool bit_is_set_u64(uint64_t v, size_t bit) noexcept
    {
        return (bit < 64) && ((v >> bit) & UINT64_C(1));
    }

    static constexpr uint64_t bit_set_u64(uint64_t v, size_t bit) noexcept
    {
        return (bit < 64) ? (v | (UINT64_C(1) << bit)) : v;
    }

    static constexpr uint64_t bit_clear_u64(uint64_t v, size_t bit) noexcept
    {
        return (bit < 64) ? (v & ~(UINT64_C(1) << bit)) : v;
    }

}


// The following is of questionable usage, as binary literals
// already exist.
// 
// turn a numeric literal into a hex constant
// (avoids problems with leading zeroes)
// 8-bit constants max value 0x11111111, always fits in unsigned long
//
#define HEX_(n) 0x##n##LU

 // 8-bit conversion function
#define B8_(x) ((x & 0x0000000FLU) ?   1:0) \
             | ((x & 0x000000F0LU) ?   2:0) \
             | ((x & 0x00000F00LU) ?   4:0) \
             | ((x & 0x0000F000LU) ?   8:0) \
             | ((x & 0x000F0000LU) ?  16:0) \
             | ((x & 0x00F00000LU) ?  32:0) \
             | ((x & 0x0F000000LU) ?  64:0) \
             | ((x & 0xF0000000LU) ? 128:0)


//
// Examples
// B8(01010101)     == 85
//
// for upto 8-bit binary constants
#define B8(d) ((unsigned char) B8_(HEX_(d)))

// for upto 16-bit binary constants, MSB first
#define B16(dmsb,dlsb) ((unsigned short) (B8(dmsb) << 8) \
        | B8(dlsb))

// for upto 32-bit binary constants, MSB first
#define B32(dmsb,db2,db3,dlsb) ((unsigned long) (B8(dmsb) << 24) \
        | (B8(db2) << 16) \
        | (B8(db3) << 8) \
        | B8(dlsb))

// for upto 64-bit binary constants, MSB first
#define B64(dmsb,db2,db3,db4,db5,db6,db7,dlsb) ((unsigned long long) (B8(dmsb) << 56) \
        | (B8(db2) << 48) \
        | (B8(db3) << 40) \
        | (B8(db4) << 32) \
        | (B8(db5) << 24) \
        | (B8(db6) << 16) \
        | (B8(db7) << 8) \
        | B8(dlsb))