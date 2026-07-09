// charset.h

#pragma once

#include "definitions.h"
#include "bit_hacks.h"


namespace waavs {

// Represent a set of characters as a bitset
//
// Typical usage:
//   charset whitespaceChars("\t\n\f\r ");
//
//	 // skipping over whitespace
//   while (whitespaceChars[c])
//		c = nextChar();
//
//
//  This is better than simply using the old classic isspace() and other functions
//  as you can define your own sets, depending on your needs:
//
//  charset delimeterChars("()<>[]{}/%");
//
//  Of course, there will surely be a built-in way of doing this in C/C++ 
//  and it will no doubt be tied to particular version of the compiler.  Use that
//  if it suits your needs.  Meanwhile, at least you can see how such a thing can
//  be implemented.



    // charset
    // Data structure to hold character set and provide
    // convenient C++ implementation for working with them
    // The character set is limited to values of 0 - 255, so not
    // suitable for Unicode or other multi-byte character sets
    //
    // The character set makes a tradeoff of size vs speed.  It 
    // could be implemented as an array of 32 bytes, but that would 
    // introduce bit manipulations to set and clear bits.
    // The current implementation favors the usage of a straight forward
    // array of 256 bytes, which is fast to set and clear bits, but takes
    // up more space.
    //
    // Alignment is 64 byte boundary, which should hopefully make it more cache
    // friendly, and possibly facilitate SIMD operations in the future.
    //
    // The constructors are constexpr, which makes it easy to statically
    // implement characters sets, either at compile time, or at runtime
    struct charset final
    {
        // Scoped fast-path toggle
        static constexpr bool kUseFastPath = true;

        alignas(64) uint8_t bits[256] = {};

        // Common Constructors
        constexpr charset() noexcept = default;
        constexpr explicit charset(const char achar) noexcept
        {
            bits[static_cast<unsigned char>(achar)] = 1;
        }

        constexpr charset(const char* chars) noexcept
        {
            for (size_t i = 0; chars[i] != 0; ++i) {
                bits[static_cast<unsigned char>(chars[i])] = 1;
            }
        }

        constexpr charset(const charset& aset) noexcept
        {
            for (size_t i = 0; i < 256; ++i)
                bits[i] = aset.bits[i];
        }

        constexpr charset& add(const char achar) noexcept
        {
            bits[static_cast<unsigned char>(achar)] = 1;

            return *this;
        }

        constexpr charset& add(const char* chars) noexcept
        {
            const char* s = chars;
            while (0 != *s) {
                bits[static_cast<unsigned char>(*s)] = 1;
                ++s;
            }

            return *this;
        }

        constexpr charset& add(const charset& aset) noexcept
        {
            for (size_t i = 0; i < 256; ++i)
                bits[i] |= aset.bits[i];

            return *this;
        }

        constexpr charset& remove(const char achar) noexcept
        {
            bits[static_cast<unsigned char>(achar)] = 0;
            return *this;
        }

        constexpr charset& remove(const char* chars) noexcept
        {
            const char* s = chars;
            while (0 != *s) {
                bits[static_cast<unsigned char>(*s)] = 0;
                ++s;
            }
            return *this;
        }

        constexpr charset& remove(const charset& aset) noexcept
        {
            for (size_t i = 0; i < 256; ++i)
                bits[i] &= ~aset.bits[i];
            return *this;
        }

        // Convenience for adding characters and strings
        constexpr charset& operator+=(const char achar) noexcept { this->add(achar); return *this; }
        constexpr charset& operator+=(const char* chars) noexcept { this->add(chars); return *this; }
        constexpr charset& operator+=(const charset& aset) noexcept { this->add(aset); return *this; }

        // Convenience for removing characters and ranges from a set
        constexpr charset& operator-=(const char achar) noexcept { bits[(uint8_t)achar]=0; return *this; }
        constexpr charset& operator-=(const char* chars) noexcept  {  return this->remove(chars); }
        constexpr charset& operator-=(const charset& aset) noexcept  {  return this->remove(aset); }


        // get an inverse of the set
        constexpr charset operator~() const noexcept
        {
            return inversion();
            //charset copy(*this);
            //return copy.invert();
        }

        constexpr charset inversion() const noexcept
        {
            charset copy(*this);
            for (size_t i = 0; i < 256; ++i)
            {
                copy.bits[i] = copy.bits[i] ? 0 : 1;
            }

            return copy;
        }

        constexpr charset& invert() noexcept
        {
            for (size_t i = 0; i < 256; ++i)
            {
                bits[i] = bits[i] ? 0 : 1;
            }

            return *this;
        }

        // Checking for set membership
        constexpr bool test(unsigned char c) const noexcept { return bits[c] != 0; }

        // This one makes it look like an array
        constexpr bool operator [](const unsigned char idx) const noexcept 
        { 
            return bits[idx] != 0; 
        }

        // This one makes it look like a function
        constexpr bool operator ()(const unsigned char c) const noexcept { return bits[c] != 0; }


        // Creating a new set
        constexpr charset operator+(const char achar) const noexcept
        {
            charset result(*this);
            result.bits[static_cast<unsigned char>(achar)] = 1;
            return result;
        }

        constexpr charset operator+(const char* chars) const noexcept 
        {
            charset result(*this);
            for (const char* p = chars; *p; ++p)
                result.bits[static_cast<unsigned char>(*p)] = 1;
            return result;
        }

        constexpr charset operator+(const charset& aset) const noexcept 
        {
            charset result;
            for (size_t i = 0; i < 256; ++i)
                result.bits[i] = this->bits[i] | aset.bits[i];
            return result;
        }

        constexpr charset operator-(const char achar) const noexcept 
        { 
            charset result(*this); 
            result.bits[static_cast<unsigned char>(achar)] = 0;

            return result; 
        }
        constexpr charset operator-(const char* chars) const noexcept
        {
            charset result(*this);
            for (const char* p = chars; *p; ++p)
                result.bits[static_cast<unsigned char>(*p)] = 0;
            return result;
        }
        constexpr charset operator-(const charset& aset) const noexcept
        {
            charset result;
            for (size_t i = 0; i < 256; ++i)
                result.bits[i] = this->bits[i] & ~aset.bits[i];
            return result;
        }

        // advanceWhile()
        // 
        // Given starting and ending pointers to a span, advance the 
        // pointer as long as the value at that position is within
        // the current set.
        //
        // Return
        //   A pointer that is positioned right after the last character
        //   that was found to be within the set
        //   If the whole span is within the set, then 'end' is returned
        // Note:  You can use this to easily find if all characters are
        // within a set, by just checking if the returned pointer == end
        inline const unsigned char* advanceWhile(const unsigned char* p, const unsigned char* end) const noexcept 
        {
            while (p < end && bits[static_cast<unsigned char>(*p)])
                ++p;
            return p;
/*
            if constexpr (kUseFastPath) {
                // 4-at-a-time fast scan
                while (p + 3 < end) {
                    const unsigned char c0 = p[0];
                    const unsigned char c1 = p[1];
                    const unsigned char c2 = p[2];
                    const unsigned char c3 = p[3];

                    if ((bits[c0] & bits[c1] & bits[c2] & bits[c3]) == 0)
                        break;

                    p += 4;
                }
            }
            else {
                // 2-at-a-time fallback
                while (p + 1 < end) {
                    if (!bits[static_cast<unsigned char>(p[0])]) return p;
                    if (!bits[static_cast<unsigned char>(p[1])]) return p + 1;
                    p += 2;
                }
            }

            // Cleanup remaining characters
            while (p < end && bits[static_cast<unsigned char>(*p)])
                ++p;

            return p;
*/
        }

        // advanceUntil
        // 
        // Most generic form of advanceUntil, which takes a pointer to 
        // the beginning and end of the span, and returns a pointer to the 
        // first character that is in the current set, or 'end' if no 
        // characters in the set are found.
        inline const unsigned char* advanceUntil(const unsigned char* p, const unsigned char* end) const noexcept 
        {
            while (p < end && !bits[static_cast<unsigned char>(*p)])
                ++p;
            return p;

/*
            if constexpr (kUseFastPath) {
                // 4-at-a-time fast scan
                while (p + 3 < end) {
                    const unsigned char c0 = p[0];
                    const unsigned char c1 = p[1];
                    const unsigned char c2 = p[2];
                    const unsigned char c3 = p[3];

                    if ((bits[c0] | bits[c1] | bits[c2] | bits[c3]) != 0)
                        break;

                    p += 4;
                }
            }
            else {
                // 2-at-a-time fallback
                while (p + 1 < end) {
                    if (bits[static_cast<unsigned char>(p[0])]) return p;
                    if (bits[static_cast<unsigned char>(p[1])]) return p + 1;
                    p += 2;
                }
            }

            // Cleanup remaining characters
            while (p < end && !bits[static_cast<unsigned char>(*p)])
                ++p;

            return p;
*/
        }
    };

}

namespace waavs {
    // The classic character ccategorizers (isdigit(), isalpha(), etc.) from ctype.h
    // are replicated here.  They are implemented as constexpr functions, so they can
    // be used in other constexpr functions.
    // Probably not much of a performance difference between
    // these and the standard library functions, but at least 
    // you can be sure of how they work, and that they are constexpr.
    static constexpr bool is_digit(const unsigned char c) noexcept { return ((c >= '0') && (c <= '9')); }
    static constexpr bool is_xdigit(const unsigned char c) noexcept { return (((c >= '0') && (c <= '9')) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')); }
    static constexpr bool is_alpha(const unsigned char c) noexcept { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
    static constexpr bool is_alnum(const unsigned char c) noexcept { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || ((c >= '0') && (c <= '9')); }
    static constexpr bool is_space(const unsigned char c) noexcept { return c == 0x20 || c == 0x09 || c == 0x0A || c == 0x0D; }
    static constexpr bool is_upper(const unsigned char c) noexcept { return (c >= 'A' && c <= 'Z'); }
    static constexpr bool is_lower(const unsigned char c) noexcept { return (c >= 'a' && c <= 'z'); }
    static constexpr bool is_print(const unsigned char c) noexcept { return (c >= 0x20 && c <= 0x7E); }
    static constexpr bool is_punct(const unsigned char c) noexcept { return (c >= 0x21 && c <= 0x2F) || (c >= 0x3A && c <= 0x40) || (c >= 0x5B && c <= 0x60) || (c >= 0x7B && c <= 0x7E); }
    static constexpr bool is_cntrl(const unsigned char c) noexcept { return (c < 0x20) || (c == 0x7F); }
    static constexpr bool is_graph(const unsigned char c) noexcept { return (c >= 0x21 && c <= 0x7E); }

    // Returns true if all 16 bytes in v are XML whitespace:
    // ' ', '\t', '\n', '\r'
#if WAAVS_HAS_NEON
    static INLINE bool neon_is_all_wsp_16(uint8x16_t v) noexcept
    {
        const uint8x16_t vsp = vdupq_n_u8(' ');
        const uint8x16_t vtb = vdupq_n_u8('\t');
        const uint8x16_t vnl = vdupq_n_u8('\n');
        const uint8x16_t vcr = vdupq_n_u8('\r');

        const uint8x16_t msp = vceqq_u8(v, vsp);
        const uint8x16_t mtb = vceqq_u8(v, vtb);
        const uint8x16_t mnl = vceqq_u8(v, vnl);
        const uint8x16_t mcr = vceqq_u8(v, vcr);

        const uint8x16_t mw1 = vorrq_u8(msp, mtb);
        const uint8x16_t mw2 = vorrq_u8(mnl, mcr);
        const uint8x16_t mw = vorrq_u8(mw1, mw2);

#if defined(__aarch64__)
        return vminvq_u8(mw) == 0xFF;
#else
        alignas(16) uint8_t tmp[16];
        vst1q_u8(tmp, mw);

        for (int i = 0; i < 16; ++i)
        {
            if (tmp[i] != 0xFF)
                return false;
        }

        return true;
#endif
    }

    // Returns true if the 16-byte chunk beginning at p is all XML whitespace.
    static INLINE bool neon_span_is_all_xml_wsp_16(const uint8_t* p) noexcept
    {
        return neon_is_all_wsp_16(vld1q_u8(p));
    }
#endif


}

namespace waavs {
    static constexpr unsigned char to_lower(const unsigned char c) noexcept {
        return (c >= 'A' && c <= 'Z') ? (c | 32) : c;
    }

    static constexpr unsigned char to_upper(const unsigned char c) noexcept {
        return (c >= 'a' && c <= 'z') ? (c & ~32) : c;
    }
}

namespace waavs {
    // Some common character sets
    static constexpr charset chrWspChars("\t\r\n\f\v ");            // whitespace characters
    static constexpr charset chrWspComma = chrWspChars + ",";       // whitespace + comma	
    static constexpr charset chrAlphaChars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    static constexpr charset chrDecDigits("0123456789");
    static constexpr charset chrHexDigits("0123456789ABCDEFabcdef");
    static constexpr charset chrSignChars("+-");
}

namespace waavs
{
    // hex_nibble
// 
// given an input character representing a hex digit
// put the decimal value of that hex digit in outValue
    static INLINE WGResult hex_nibble(const uint8_t vIn, uint8_t& outValue) noexcept
    {
        if (vIn >= '0' && vIn <= '9')
        {
            outValue = vIn - '0';
            return WG_SUCCESS;
        }

        if (vIn >= 'a' && vIn <= 'f')
        {
            outValue = vIn - 'a' + 10;
            return WG_SUCCESS;
        }

        if (vIn >= 'A' && vIn <= 'F')
        {
            outValue = vIn - 'A' + 10;
            return WG_SUCCESS;
        }

        return WG_ERROR_Invalid_Argument;
    }

    // Read two bytes representing a hex byte (e.g. '4F') and 
    // convert them to a single byte value (0x4F in this example).
    // Return: 
    //  WG_SUCCESS if successful, or 
    //  WG_ERROR_Invalid_Argument if either of 
    //      the input characters is not a valid hex digit.
    static INLINE WGResult hex_byte(const uint8_t highNibble, const uint8_t lowNibble, uint8_t& outValue) noexcept
    {
        uint8_t highValue = 0;
        uint8_t lowValue = 0;
        if (hex_nibble(highNibble, highValue) != WG_SUCCESS)
            return WG_ERROR_Invalid_Argument;

        if (hex_nibble(lowNibble, lowValue) != WG_SUCCESS)
            return WG_ERROR_Invalid_Argument;

        outValue = (highValue << 4) | lowValue;

        return WG_SUCCESS;
    }

    // parseHex64u
//
// Parse a hex string into a 64-bit unsigned integer.
// The string must be a valid hex string, and must not contain any spaces.
    static INLINE bool parseHex64u(const ByteSpan& inSpan, uint64_t& outValue) noexcept
    {
        if (inSpan.size() == 0 || inSpan.size() > 16)
            return false;


        // building up outValue as we go
        outValue = 0;
        const uint8_t* c = inSpan.begin();
        while (c != inSpan.end())
        {
            uint8_t nibble = 0;
            if (hex_nibble(*c, nibble) == WG_SUCCESS)
            {
                // We shift by 4 bits, because we're processing a nibble
                // at a time. so shift and add the latest nibble
                outValue <<= 4;
                outValue |= nibble;
            }
            else {
                // if we get here, it means we had an invalid hex digit
                // in the input string.  We return false to indicate a parse failure.
                return false;
            }

            c++;
        }

        return true;
    }

    // return 1 if the chunk is "true" or "1" or "t" or "T" or "y" or "Y" or "yes" or "Yes" or "YES"
// return 0 if the chunk is "false" or "0" or "f" or "F" or "n" or "N" or "no" or "No" or "NO"
// return 0 otherwise
    static INLINE int toBoolInt(const ByteSpan& inChunk)
    {
        ByteSpan s = inChunk;

        if (s == "true" || s == "1" || s == "t" || s == "T" || s == "y" || s == "Y" || s == "yes" || s == "Yes" || s == "YES")
            return 1;
        else if (s == "false" || s == "0" || s == "f" || s == "F" || s == "n" || s == "N" || s == "no" || s == "No" || s == "NO")
            return 0;
        else
            return 0;
    }
}