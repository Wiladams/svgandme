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
    constexpr double fixedToFloat(const uint64_t vint, const int scale) noexcept
    {
        if (scale == 0) {
            return (double)vint;
        }
        if (scale >= 64) {
            return (double)vint / (double)((uint64_t)1 << (scale - 64));
        }

        uint64_t whole = bit_range_get_value_u64(vint, (size_t)scale, 63);
        uint64_t frac = bit_range_get_value_u64(vint, 0, (size_t)scale - 1);

        return (double)whole + ((double)frac / (double)((uint64_t)1 << scale));
    }

    
    // conv_u32_to_hex_ascii
    // Convert a 32-bit number to a fixed-width hex string with leading zeros.
    // width must be between 1 and 8 inclusive.
    // Returns: number of characters written (excluding null terminator), or 0 on error.
    INLINE int conv_u32_to_hex_ascii(uint32_t inNumber, char* buff, size_t buffLen, int width) noexcept
    {
        static const char hexdigits[] = "0123456789abcdef";

        if (width < 1 || width > 8)
            return 0;

        if (buffLen < (size_t)(width + 1))
            return 0;

        for (int i = width - 1; i >= 0; --i) {
            buff[i] = hexdigits[inNumber & 0x0F];
            inNumber >>= 4;
        }
        buff[width] = '\0';

        return width;
    }

    // conv_u32_to_hex_ascii_var
    // 
    // Convert a uint32-number to a hex string without leading zeroes.
    // Figure out the number of bytes that will be required, 
    // then call conv_u32_to_hex_ascii using that width.
    INLINE int conv_u32_to_hex_ascii_var(uint32_t inNumber, char* buff, size_t buffLen) noexcept
    {
        if (inNumber == 0) {
            if (buffLen < 2) return 0;
            buff[0] = '0';
            buff[1] = '\0';
            return 1;
        }

        int width = 0;
        uint32_t temp = inNumber;
        while (temp != 0) {
            ++width;
            temp >>= 4;
        }
        return conv_u32_to_hex_ascii(inNumber, buff, buffLen, width);
    }

    // conv_u32_to_binary_ascii
    // 
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


