#pragma once

//
// Converters
// Various routines to convert from one data type to another.
// Mainly parsing numeric types
//


#include "bspan.h"
#include "maths.h"

namespace waavs {
    static INLINE uint8_t  hexToDec(const uint8_t vIn) noexcept
    {
        if (vIn >= '0' && vIn <= '9')
            return vIn - '0';
        else if (vIn >= 'a' && vIn <= 'f')
            return vIn - 'a' + 10;
        else if (vIn >= 'A' && vIn <= 'F')
            return vIn - 'A' + 10;
        else
            return 0;
    }
}

namespace waavs {

    // return 1 if the chunk is "true" or "1" or "t" or "T" or "y" or "Y" or "yes" or "Yes" or "YES"
    // return 0 if the chunk is "false" or "0" or "f" or "F" or "n" or "N" or "no" or "No" or "NO"
    // return 0 otherwise
    static INLINE int toBoolInt(const ByteSpan& inChunk) noexcept
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

namespace waavs {

    // parseHex64u
    //
	// Parse a hex string into a 64-bit unsigned integer.
	// The string must be a valid hex string, and must not contain any spaces.
    static INLINE bool parseHex64u(const ByteSpan& inSpan, uint64_t & outValue) noexcept
    {
		if (inSpan.size() == 0 || inSpan.size() > 8)
            return false;


        // building up outValue as we go
        outValue = 0;
        const uint8_t* c = inSpan.begin();
		while (c != inSpan.end())
		{
            // We shift by 4 bits, because we're processing a nibble at a time
            // and a nibble is 4 bits
			outValue <<= 4;
			outValue |= hexToDec(*c);

            c++;
		}
        
        return true;
    }
}

namespace waavs {
    // parse64u
    // Parse a 64 bit unsigned integer from a string
    // return true if successful, false if not
    // If successful, the value is stored in the out parameter
    //
    static INLINE bool parse64u(const ByteSpan& inChunk, uint64_t& v) noexcept
    {
        ByteSpan s = inChunk;

        if (!s)
            return false;

        const unsigned char* sStart = s.fStart;
        const unsigned char* sEnd = s.fEnd;

        // Return early if the next thing is not a digit
        if (!is_digit(*sStart))
            return false;

        // Initialize the value
        v = 0;

        // While we still have input to consume
        while ((sStart < sEnd) && is_digit(*sStart))
        {
            v = (v * 10) + (uint64_t)(*sStart - '0');
            sStart++;
        }

        return true;
    }

    // parse64i
    // Parse a 64 bit signed integer from the input span
    // Return true if successful, false otherwise
    // If successful, the value is stored in the out parameter
    //
    static INLINE bool parse64i(const ByteSpan& inChunk, int64_t& v) noexcept
    {
        ByteSpan s = inChunk;

        if (!s)
            return false;

        const unsigned char* sStart = s.fStart;
        const unsigned char* sEnd = s.fEnd;

        // Check for a sign if it's there
        int sign = 1;
        if (*sStart == '-') {
            sign = -1;
            sStart++;
        }
        else if (*sStart == '+') {
            sStart++;
        }

        uint64_t uvalue{ 0 };
        if (!parse64u(s, uvalue))
            return false;

        if (sign < 0)
            v = -(int64_t)uvalue;
        else
            v = (int64_t)uvalue;

        return true;
    }

    static INLINE bool read_u64(ByteSpan& s, uint64_t& v) noexcept
    {
        if (!s)
            return false;

        v = 0;
        const unsigned char* sStart = s.fStart;
        const unsigned char* sEnd = s.fEnd;

        while ((sStart < sEnd) && is_digit(*sStart))
        {
            v = (v * 10) + (uint64_t)(*sStart - '0');
            sStart++;
        }

        s.fStart = sStart;

        return true;
    }

    // Just put this from_chars implementation here in case
    // we ever want to use it instead
    //double outNumber = 0;
    //auto res = std::from_chars((const char*)s.fStart, (const char*)s.fEnd, outNumber);
    //if (res.ec == std::errc::invalid_argument)
    //{
    //	printf("chunk_to_double: INVALID ARGUMENT: ");
    //	printChunk(s);
    //}
    //return outNumber;
    // 
    // readNumber()
    //
    // Parse a double number from the given ByteSpan, advancing the start
    // of the ByteSpan to beyond where we found the last character of the number.
    // Assumption:  We're sitting at beginning of a number, all whitespace handling
    // has already occured.
    static bool INLINE readNumber(ByteSpan& s, double& value) noexcept
    {
        const unsigned char* startAt = s.fStart;
        const unsigned char* endAt = s.fEnd;

        double sign = 1.0;
        double res = 0.0;

        // integer part
        uint64_t intPart = 0;

        // fractional part
        uint64_t fracPart = 0;
        uint64_t fracBase = 1;


        bool hasIntPart = false;
        bool hasFracPart = false;

        // Parse optional sign
        if (*startAt == '+') {
            startAt++;
        }
        else if (*startAt == '-') {
            sign = -1;
            startAt++;
        }

        // Parse integer part
        if (is_digit(*startAt))
        {
            hasIntPart = true;
            s.fStart = startAt;
            read_u64(s, intPart);
            startAt = s.fStart;
            res = static_cast<double>(intPart);
        }

        // Parse fractional part.
        if ((startAt < endAt) && (*startAt == '.'))
        {
            hasFracPart = true;
            startAt++; // Skip '.'

            fracBase = 1;

            // Add the fraction portion without calling out to powd
            while ((startAt < endAt) && is_digit(*startAt)) {
                fracPart = fracPart * 10 + static_cast<uint64_t>(*startAt - '0');
                fracBase *= 10;
                startAt++;
            }
            res += (static_cast<double>(fracPart) / static_cast<double>(fracBase));

        }

        // If we don't have an integer or fractional
        // part, then just return false
        if (!hasIntPart && !hasFracPart)
            return false;

        // Parse optional exponent
        // mostly we don't see this, so we won't bother trying
        // to optimize it beyond using powd
        if ((startAt < endAt) && 
            (((*startAt == 'e') || (*startAt == 'E')) && 
            ((startAt[1] != 'm') && (startAt[1] != 'x'))))
        {
            // exponent parts
            uint64_t expPart = 0;
            double expSign = 1.0;

            startAt++; // skip 'E'
            if (*startAt == '+') {
                startAt++;
            }
            else if (*startAt == '-') {
                expSign = -1.0;
                startAt++;
            }


            if (is_digit(*startAt)) {
                s.fStart = startAt;
                read_u64(s, expPart);
                startAt = s.fStart;
                res = res * std::pow(10, double(expSign * double(expPart)));
            }
        }
        s.fStart = startAt;

        value = res * sign;

        return true;
    }

    static INLINE bool parseNumber(const ByteSpan& inChunk, double& value)
    {
        ByteSpan s = inChunk;
        return readNumber(s, value);
    }
}


