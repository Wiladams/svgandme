#pragma once

#include "definitions.h"


/*
    Binary operators

    Provide binary operators as functions.  Useful when you need
    to use a function as a parameter.

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
    tobit
    tohex
*/
namespace waavs {
    static const char* hexdigits = "0123456789abcdef";

    // tohex32
    // Take a 32-bit number and return the hex representation of the same
    // The buffer passed in needs to be at least 8 bytes long
    //
    // Return:
    //  The number of bytes actually written
    //  0 upon error
    //  Really the only error that can occur is if the buffLen is less than 8
    static INLINE int tohex32(uint32_t a, char* buff, size_t buffLen)
    {
        if (buffLen < 8)
            return 0;

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
    static INLINE int tobin32(uint32_t a, char* buff, size_t buffLen)
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


    // 16-bit versions
    //
    static INLINE uint16_t bnot16(uint16_t a) { return ~a; }
    static INLINE uint16_t band16(uint16_t a, uint16_t b) { return a & b; }
    static INLINE uint16_t bor16(uint16_t a, uint16_t b) { return a | b; }
    static INLINE uint16_t bxor16(uint16_t a, uint16_t b) { return a ^ b; }
    static INLINE uint16_t lshift16(uint16_t a, unsigned int nbits) { return a << nbits; }
    static INLINE uint16_t rshift16(uint16_t a, unsigned int nbits) { return a >> nbits; }
    static INLINE int16_t arshift16(int16_t a, unsigned int nbits) { return a >> nbits; }
    static INLINE uint16_t rol16(uint16_t a, unsigned int n) { return ((a << n) | (a >> (16 - n))); }
    static INLINE uint16_t ror16(uint16_t a, unsigned int n) { return ((a << (16 - n)) | (a >> n)); }
    //static INLINE uint16_t bswap16(uint16_t a) {return _byteswap_ushort(a);}
    static INLINE uint16_t bswap16(uint16_t a) { return (a >> 8) | (a << 8); }

    static INLINE uint16_t tobit16(uint64_t a) { return (uint16_t)a; }


    // 32-bit versions
    //
    static INLINE uint32_t bnot32(uint32_t a) { return ~a; }
    static INLINE uint32_t band32(uint32_t a, uint32_t b) { return a & b; }
    static INLINE uint32_t bor32(uint32_t a, uint32_t b) { return a | b; }
    static INLINE uint32_t bxor32(uint32_t a, uint32_t b) { return a ^ b; }
    static INLINE uint32_t lshift32(uint32_t a, unsigned int nbits) { return a << nbits; }
    static INLINE uint32_t rshift32(uint32_t a, unsigned int nbits) { return a >> nbits; }
    static INLINE int32_t arshift16(int32_t a, unsigned int nbits) { return a >> nbits; }
    static INLINE uint32_t rol32(uint32_t a, unsigned int n) { return ((a << n) | (a >> (32 - n))); }
    static INLINE uint32_t ror32(uint32_t a, unsigned int n) { return ((a << (32 - n)) | (a >> n)); }
    //static INLINE uint32_t bswap32(uint32_t a) {return _byteswap_ulong(a);}
    static INLINE uint32_t bswap32(uint32_t v) {
        return ((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >> 8) |
            ((v & 0x0000ff00) << 8) | ((v & 0x000000ff) << 24);
    }

    static INLINE uint32_t tobit32(uint64_t a) { return (uint32_t)a; }


    // 64-bit versions
    //
    static INLINE uint64_t bnot64(uint64_t a) { return ~a; }
    static INLINE uint64_t band64(uint64_t a, uint64_t b) { return a & b; }
    static INLINE uint64_t bor64(uint64_t a, uint64_t b) { return a | b; }
    static INLINE uint64_t bxor64(uint64_t a, uint64_t b) { return a ^ b; }
    static INLINE uint64_t lshift64(uint64_t a, unsigned int nbits) { return a << nbits; }
    static INLINE uint64_t rshift64(uint64_t a, unsigned int nbits) { return a >> nbits; }
    static INLINE int64_t arshift16(int64_t a, unsigned int nbits) { return a >> nbits; }
    static INLINE uint64_t rol64(uint64_t a, unsigned int n) { return ((a << n) | (a >> (64 - n))); }
    static INLINE uint64_t ror64(uint64_t a, unsigned int n) { return ((a << (64 - n)) | (a >> n)); }
    //static INLINE uint64_t bswap64(uint64_t a) {return _byteswap_uint64(a);}
    static INLINE uint64_t bswap64(uint64_t v) {
        return ((v & ((uint64_t)0xff << (7 * 8))) >> (7 * 8)) |
            ((v & ((uint64_t)0xff << (6 * 8))) >> (5 * 8)) |
            ((v & ((uint64_t)0xff << (5 * 8))) >> (3 * 8)) |
            ((v & ((uint64_t)0xff << (4 * 8))) >> (1 * 8)) |
            ((v & ((uint64_t)0xff << (3 * 8))) << (1 * 8)) |
            ((v & ((uint64_t)0xff << (2 * 8))) << (3 * 8)) |
            ((v & ((uint64_t)0xff << (1 * 8))) << (5 * 8)) |
            ((v & ((uint64_t)0xff << (0 * 8))) << (7 * 8));

    }


}   // end of namespace
