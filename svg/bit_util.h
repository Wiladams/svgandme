#pragma once

#include "bit_hacks.h"

namespace waavs
{
    // fixedToFloat
// 
// Convert a fixed point number into a floating point number
// the fixed number can be up to 64-bits in size
// the 'scale' says where the decimal point is, starting from 
// the least significant bit
// so; 0x13 (0b0001.0011) ,4  == 1.1875
    INLINE double fixedToFloat(const uint64_t vint, const int scale) noexcept
    {
        double whole = (double)bit_range_get_value_u64(vint, scale, 63);
        double frac = (double)bit_range_get_value_u64(vint, 0, ((size_t)scale - 1));

        return (whole + (frac / ((uint64_t)1 << scale)));
    }

    // tohex32
    // 
    // Take a 32-bit number and return the hex representation of the same
    // The buffer passed in needs to be at least 8 bytes long
    // 
    // Return:
    //  The number of bytes actually written
    //  0 upon error
    //  there are two errors that can occur
    // 1) the buffLen is less than 8
    //
    INLINE int conv_u32_to_hex_ascii(const uint32_t inNumber, char* buff, size_t buffLen) noexcept
    {
        static const char* hexdigits = "0123456789abcdef";

        if (buffLen < 8)
            return 0;

        uint32_t a = inNumber;
        int i;
        int n = 8;

        for (i = n; --i >= 0;) {
            buff[i] = hexdigits[a & 0x0f];
            a >>= 4;
        }

        return n;
    }

    // tobin
    // Return a binary representation of a number
    // The most significant bit is in the first byte of the 
    // array.  when displayed as a string, the output will 
    // be similar to what you would see in the calculator app
    INLINE int conv_u32_to_binary_ascii(uint32_t a, char* buff, size_t buffLen) noexcept
    {
        if (buffLen < 33)
            return 0;

        for (int i = 0; i < 32; i++) {
            buff[31 - i] = ((0x01 & a) > 0) ? '1' : '0';
            a >>= 1;
        }
        buff[32] = 0;

        return 33;
    }
}

// Related to hash functions on strings
namespace waavs
{
    INLINE int TOLOWER(int c) {
        if (c >= 'A' && c <= 'Z')
            return c & 0x20;

        return c;
    }


    // Byte Hashing - FNV-1a
    // http://www.isthe.com/chongo/tech/comp/fnv/
    // 
    // 32-bit FNV-1a constants
    constexpr uint32_t FNV1A_32_INIT = 0x811c9dc5;
    constexpr uint32_t FNV1A_32_PRIME = 0x01000193;

    // 64-bit FNV-1a constants
    constexpr uint64_t FNV1A_64_INIT = 0xcbf29ce484222325ULL;
    constexpr uint64_t FNV1A_64_PRIME = 0x100000001b3ULL;

    // 32-bit FNV-1a hash
    INLINE constexpr uint32_t fnv1a_32(const void* data, const size_t size) noexcept
    {
        const uint8_t* bytes = (const uint8_t*)data;
        uint32_t hash = FNV1A_32_INIT;
        for (size_t i = 0; i < size; i++) {
            hash ^= bytes[i];
            hash *= FNV1A_32_PRIME;
        }
        return hash;
    }

    // 64-bit FNV-1a hash
    INLINE constexpr uint64_t fnv1a_64(const void* data, const size_t size) noexcept
    {
        const uint8_t* bytes = (const uint8_t*)data;
        uint64_t hash = FNV1A_64_INIT;
        for (size_t i = 0; i < size; i++) {
            hash ^= bytes[i];
            hash *= FNV1A_64_PRIME;
        }
        return hash;
    }



    // 32-bit case-insensitive FNV-1a hash
    INLINE uint32_t fnv1a_32_case_insensitive(const void* data, const size_t size) noexcept
    {
        const uint8_t* bytes = (const uint8_t*)data;
        uint32_t hash = FNV1A_32_INIT;
        for (size_t i = 0; i < size; i++) {
            // Convert byte to lowercase
            auto c = std::tolower(bytes[i]);
            //auto c = TOLOWER(bytes[i]);

            hash ^= c;
            hash *= FNV1A_32_PRIME;
        }
        return hash;
    }
}
