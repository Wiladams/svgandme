#pragma once

#include "definitions.h"

//
// Some bitwise manipulation routines
//

namespace waavs
{
    // Determine which endianness the CPU has at runtime
    // little-endian (intel standard)
    INLINE bool isLE() noexcept { int i = 1; return (int)*((unsigned char*)&i) == 1; }
    
    // big-endian
    INLINE bool isBE() noexcept { return !isLE(); }
}

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


    // tohex32
    // 
    // Take a 32-bit number and return the hex representation of the same
    // The buffer passed in needs to be at least 8 bytes long
    // 
    // Return:
    //  The number of bytes actually written
    //  0 upon error
    //  there arw two errors that can occur
    // 1) the buffLen is less than 8
    //
    INLINE int tohex32(const uint32_t inNumber, char* buff, size_t buffLen) noexcept
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
    INLINE int tobin32(uint32_t a, char* buff, size_t buffLen) noexcept
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
    INLINE uint16_t bnot16(uint16_t a) noexcept { return ~a; }
    INLINE uint16_t band16(uint16_t a, uint16_t b) noexcept { return a & b; }
    INLINE uint16_t bor16(uint16_t a, uint16_t b) noexcept { return a | b; }
    INLINE uint16_t bxor16(uint16_t a, uint16_t b) noexcept { return a ^ b; }
    INLINE uint16_t lshift16(uint16_t a, unsigned int nbits) noexcept { return a << nbits; }
    INLINE uint16_t rshift16(uint16_t a, unsigned int nbits) noexcept { return a >> nbits; }
    INLINE int16_t arshift16(int16_t a, unsigned int nbits) noexcept { return a >> nbits; }
    INLINE uint16_t rol16(uint16_t a, unsigned int n) noexcept { return ((a << n) | (a >> (16 - n))); }
    INLINE uint16_t ror16(uint16_t a, unsigned int n) noexcept { return ((a << (16 - n)) | (a >> n)); }
    //INLINE uint16_t bswap16(uint16_t a) {return _byteswap_ushort(a);}
    INLINE uint16_t bswap16(uint16_t a) noexcept { return (a >> 8) | (a << 8); }

    INLINE uint16_t tobit16(uint64_t a) noexcept { return (uint16_t)a; }


    // 32-bit versions
    //
    INLINE uint32_t bnot32(uint32_t a) noexcept { return ~a; }
    INLINE uint32_t band32(uint32_t a, uint32_t b) noexcept { return a & b; }
    INLINE uint32_t bor32(uint32_t a, uint32_t b) noexcept { return a | b; }
    INLINE uint32_t bxor32(uint32_t a, uint32_t b) noexcept { return a ^ b; }
    INLINE uint32_t lshift32(uint32_t a, unsigned int nbits) noexcept { return a << nbits; }
    INLINE uint32_t rshift32(uint32_t a, unsigned int nbits) noexcept { return a >> nbits; }
    INLINE int32_t arshift16(int32_t a, unsigned int nbits) noexcept { return a >> nbits; }
    INLINE uint32_t rol32(uint32_t a, unsigned int n) noexcept { return ((a << n) | (a >> (32 - n))); }
    INLINE uint32_t ror32(uint32_t a, unsigned int n) noexcept { return ((a << (32 - n)) | (a >> n)); }
    //static INLINE uint32_t bswap32(uint32_t a) {return _byteswap_ulong(a);}
    INLINE uint32_t bswap32(uint32_t v) noexcept {
        return ((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >> 8) |
            ((v & 0x0000ff00) << 8) | ((v & 0x000000ff) << 24);
    }

    INLINE uint32_t tobit32(uint64_t a) noexcept { return (uint32_t)a; }


    // 64-bit versions
    //
    INLINE uint64_t bnot64(uint64_t a) { return ~a; }
    INLINE uint64_t band64(uint64_t a, uint64_t b) { return a & b; }
    INLINE uint64_t bor64(uint64_t a, uint64_t b) { return a | b; }
    INLINE uint64_t bxor64(uint64_t a, uint64_t b) { return a ^ b; }
    INLINE uint64_t lshift64(uint64_t a, unsigned int nbits) { return a << nbits; }
    INLINE uint64_t rshift64(uint64_t a, unsigned int nbits) { return a >> nbits; }
    INLINE int64_t arshift16(int64_t a, unsigned int nbits) { return a >> nbits; }
    INLINE uint64_t rol64(uint64_t a, unsigned int n) { return ((a << n) | (a >> (64 - n))); }
    INLINE uint64_t ror64(uint64_t a, unsigned int n) { return ((a << (64 - n)) | (a >> n)); }
    //static INLINE uint64_t bswap64(uint64_t a) {return _byteswap_uint64(a);}
    INLINE uint64_t bswap64(uint64_t v) {
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


//
// Some of the routines here are inspired by the bit twiddling hacks
// http://graphics.stanford.edu/~seander/bithacks.html
//

/* turn a numeric literal into a hex constant
 * (avoids problems with leading zeroes)
 * 8-bit constants max value 0x11111111, always fits in unsigned long
 */
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



namespace waavs {

// Return various forms of pow(2,bitnum)
// There are different ones, which allow the user to specify how
// many bits they want
INLINE constexpr uint8_t BIT8(size_t bitnum) noexcept {return (uint8_t)1 << bitnum; }
INLINE constexpr uint16_t BIT16(size_t bitnum) noexcept {return (uint16_t)1 << bitnum; }
INLINE constexpr uint32_t BIT32(size_t bitnum) noexcept {return (uint32_t)1 << bitnum; }
INLINE constexpr uint64_t BIT64(size_t bitnum) noexcept {return (uint64_t)1 << bitnum; }

// One general purpose which will default to BIT64
//static inline uint64_t BIT(unsigned int bitnum) {return BIT64(bitnum);}

// return true if the specified bit is set in the 64-bit value
INLINE constexpr bool isset(const uint64_t value, const size_t bitnum) noexcept {return (value & BIT64(bitnum)) > 0; }

// set a specific bit within a 64-bit value
INLINE constexpr uint64_t setbit(const uint64_t value, const size_t bitnum) noexcept {return (value | BIT64(bitnum));}

// BITMASK64
// A bitmask is an integer where all the bits from the 
// specified low value to the specified high value
// are set to 1.
// The trick used in BITMASK64 is to set a single bit
// above the required range, then subtracting 1 from that
// value.  By subtracting 1, we get all 1 bits below the
// single bit value.
// This set of 1 bits are then shifted upward by the number
// of the low bit, and we have our mask.
// Discussion: 
//   https://stackoverflow.com/questions/28035794/masking-bits-within-a-range-given-in-parameter-in-c
//
//  uint64_t mask = (uint64_t)1 << (high-low);
//  mask <<= 1;
//  mask--;         // turn on all the bits below
//  mask <<= low;   // shift up to proper position
//  return mask;

INLINE constexpr uint64_t BITMASK64(const size_t low, const size_t high) noexcept {return ((((uint64_t)1ULL << (high-low)) << 1) - 1) << low;}

INLINE constexpr uint8_t BITMASK8(const size_t low, const size_t high) noexcept {return (uint8_t)BITMASK64(low, high);}
INLINE constexpr uint16_t BITMASK16(const size_t low, const size_t high) noexcept {return (uint16_t)BITMASK64(low,high);}
INLINE constexpr uint32_t BITMASK32(const size_t low, const size_t high) noexcept {return (uint32_t)BITMASK64(low, high);}



// BITSVALUE
// Retrieve a value from a lowbit highbit pair
INLINE  constexpr uint64_t BITSVALUE(uint64_t src, size_t lowbit, size_t highbit) noexcept
{
    return ((src & BITMASK64(lowbit, highbit)) >> lowbit);
}

//
// getbitbyteoffset()
// 
// Given a bit number, calculate which byte
// it would be in, and which bit within that
// byte.
INLINE constexpr void getbitbyteoffset(size_t bitnumber, size_t &byteoffset, size_t &bitoffset) noexcept
{
    byteoffset = (int)(bitnumber / 8);
    bitoffset = bitnumber % 8;
}


INLINE constexpr uint64_t bitsValueFromBytes(const uint8_t *bytes, const size_t startbit, const size_t bitcount, bool bigendian = false) noexcept
{
    // Sanity check
    if (nullptr == bytes)
        return 0;

    uint64_t value = 0;

    if (bigendian) {
        for (int i=(int)bitcount; i>= 0; i--) {
            size_t byteoffset=0;
            size_t bitoffset=0;
            getbitbyteoffset(startbit+i, byteoffset, bitoffset);
            bool bitval = isset(bytes[byteoffset], bitoffset);

		    if (bitval) {
			    value = setbit(value, i);
            }
        }
    } else {
        for (size_t i=0; i<bitcount; i++) {
            size_t byteoffset=0;
            size_t bitoffset=0;
            getbitbyteoffset(startbit+i, byteoffset, bitoffset);
            bool bitval = isset(bytes[byteoffset], bitoffset);

            if (bitval) {
                value = setbit(value, i);
            }
        }
    }

    return value;
}

// Using the octal representation of the bit numbers, makes
// it more obvious what is going on.
// 
// swap 2 bytes (16-bit) around
//#define SWAP16(x) \
//    (((0x00000000000000ffull & (x)) << 010) |  \
//     ((0x000000000000ff00ull & (x)) >> 010))

//INLINE uint16_t swapUInt16(const uint16_t num) noexcept
//{
//    return (((num & 0x00ff) << 8) | ((num & 0xff00) >> 8));
//}

// swap 4 bytes (32-bit) around
//#define SWAP32(x)                           \
//  (((0x00000000000000ffull & (x)) << 030) | \
//   ((0x000000000000ff00ull & (x)) << 010) | \
//   ((0x0000000000ff0000ull & (x)) >> 010) | \
//   ((0x00000000ff000000ull & (x)) >> 030))

//INLINE uint32_t swapUInt32(const uint32_t num) noexcept
//{
//    uint32_t x = (num & 0x0000FFFF) << 16 | (num & 0xFFFF0000) >> 16;
//    x = (x & 0x00FF00FF) << 8 | (x & 0xFF00FF00) >> 8;

//    return x;
//}

// swap 8 bytes (64-bit) around
//#define SWAP64(x)                           \
//  (((0x00000000000000ffull & (x)) << 070) | \
//   ((0x000000000000ff00ull & (x)) << 050) | \
//   ((0x0000000000ff0000ull & (x)) << 030) | \
//   ((0x00000000ff000000ull & (x)) << 010) | \
//   ((0x000000ff00000000ull & (x)) >> 010) | \
//   ((0x0000ff0000000000ull & (x)) >> 030) | \
//   ((0x00ff000000000000ull & (x)) >> 050) | \
//   ((0xff00000000000000ull & (x)) >> 070))

//INLINE uint64_t swapUInt64(const uint64_t num) noexcept
//{
//    return  (num >> 56) |
//          ((num<<40) & 0x00FF000000000000) |
//          ((num<<24) & 0x0000FF0000000000) |
//          ((num<<8) & 0x000000FF00000000) |
//          ((num>>8) & 0x00000000FF000000) |
//          ((num>>24) & 0x0000000000FF0000) |
//          ((num>>40) & 0x000000000000FF00) |
//          (num << 56);
//}

INLINE int GetAlignedByteCount(const int width, const int bitsperpixel, const int alignment) noexcept
{
    return (((width * (bitsperpixel / 8)) + (alignment - 1)) & ~(alignment - 1));
}

// fixedToFloat
// 
// Convert a fixed point number into a floating point number
// the fixed number can be up to 64-bits in size
// the 'scale' says where the decimal point is, starting from 
// the least significant bit
// so; 0x13 (0b0001.0011) ,4  == 1.1875
INLINE double fixedToFloat(const uint64_t vint, const int scale) noexcept
{
    double whole = (double)waavs::BITSVALUE(vint, scale, 63);
    double frac = (double)waavs::BITSVALUE(vint, 0, ((size_t)scale - 1));

    return (whole + (frac / ((uint64_t)1 << scale)));
}

} // namespace


namespace waavs {
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
	INLINE uint32_t fnv1a_32(const void* data, const size_t size) noexcept
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
	INLINE uint64_t fnv1a_64(const void* data, const size_t size) noexcept
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


