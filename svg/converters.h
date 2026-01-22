#ifndef CONVERTERS_H_INCLUDED
#define CONVERTERS_H_INCLUDED


#pragma once

//
// Converters
// Various routines to convert from one data type to another.
// Mainly parsing numeric types
//


#include "bspan.h"
#include "maths.h"

// Forward declarations
static INLINE bool read_required_digits(waavs::ByteSpan& s, uint64_t& v, size_t requiredDigits) noexcept;

static INLINE uint8_t  hexToDec(const uint8_t vIn) ;
static INLINE int toBoolInt(const waavs::ByteSpan& inChunk);

static inline bool readNextNumber(waavs::ByteSpan& s, double& outNumber) noexcept;
static inline bool readNextFlag(waavs::ByteSpan& s, int& outNumber) noexcept;
static int readFloatArguments(waavs::ByteSpan& s, const char* argTypes, float* outArgs) noexcept;
static int readNumericArguments(waavs::ByteSpan& s, const char* argTypes, double* outArgs) noexcept;



// given an input character representing a hex digit
// return the decimal value of that hex digit
// BUGBUG - This assumes valid input.  
// No error checking is done. The input is presumed to be valid
static INLINE uint8_t  hexToDec(const uint8_t vIn)
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


// return 1 if the chunk is "true" or "1" or "t" or "T" or "y" or "Y" or "yes" or "Yes" or "YES"
// return 0 if the chunk is "false" or "0" or "f" or "F" or "n" or "N" or "no" or "No" or "NO"
// return 0 otherwise
static INLINE int toBoolInt(const waavs::ByteSpan& inChunk)
{
    waavs::ByteSpan s = inChunk;

    if (s == "true" || s == "1" || s == "t" || s == "T" || s == "y" || s == "Y" || s == "yes" || s == "Yes" || s == "YES")
        return 1;
    else if (s == "false" || s == "0" || s == "f" || s == "F" || s == "n" || s == "N" || s == "no" || s == "No" || s == "NO")
        return 0;
    else
        return 0;
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
    // In these routines, the difference between parse and read
    // is that parse does not modify the input ByteSpan, whereas
    // read will advance the start of the ByteSpan to after the
    // last character that was parsed.
    
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


    // read_u64
    //
    // Read a 64-bit unsigned integer from the input span
    // advance the span 
    static INLINE bool read_u64(ByteSpan& s, uint64_t& v, size_t digitsRead) noexcept
    {
        const unsigned char* sStart = s.fStart;
        const unsigned char* sEnd = s.fEnd;

        digitsRead = 0;

        // if the input is empty, or the first byte is not a digit
        // then return false
        if ((sStart >= sEnd) || !is_digit(*sStart))
            return false;


        v = 0;

        while ((sStart < sEnd) && is_digit(*sStart))
        {
            v = (v * 10) + (uint64_t)(*sStart - '0');
            ++sStart;
            ++digitsRead;
        }

        s.fStart = sStart;

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
        
        if (s.empty())
            return false;

        const unsigned char* sStart = s.fStart;
        const unsigned char* sEnd = s.fEnd;

        // Check for a sign if it's there
        int sign = 1;
        if (sStart < sEnd && (*sStart == '-' || *sStart == '+'))
        {
            sign = (*sStart == '-') ? -1 : 1;
            ++sStart;
        }

        s.fStart = sStart;
        uint64_t uvalue{ 0 };
        size_t digitsRead{ 0 };
        if (!read_u64(s, uvalue, digitsRead))
            return false;

        v = (sign < 0) ? -(int64_t)uvalue : (int64_t)uvalue;

        return true;
    }

    // Read an unsigned integer value from the ByteSpan
    // use the dual constraints of the limits of the ByteSpan
    // as well as the maximum number of digits to read
    // There MUST be the required number of digits available
    // or return false.
    // read_required_digits("234", &value, 2);
    // Will return '23' as a value
    //
    static INLINE bool read_required_digits(ByteSpan &s, uint64_t &v, size_t requiredDigits) noexcept
    {
        if (s.size()<requiredDigits)
            return false;
        
        v = 0;
        int i = 0;
        while (i < requiredDigits)
        {
			// return false, because we've exhausted
            // the input, without reaching the required
            // number of digits
            // We shoud not need this test, as we already
            // checked the size above
            //if (!s)
			//	return false;

            // get the current byte, and try to convert
            // to a digit.  
            // If not digit, return false, because we still
            // haven't satisfied the constraint of having a
            // required number of digits
            unsigned char abyte = *s-'0';

			// if the byte is not a digit, return false
            // leave the cursor where we failed in case
            // the caller wants to check the value
            if (abyte < 0 || abyte>9)
                return false;

            ++s;

            // We've got a valid digit, so 
            // multiply the return value by 10
            // and add the current byte value
            v = (v * 10) + abyte;

            // increment count of digits processed
            ++i;
        }

        // if we've gotten to here, we have satisfied
        // the constraint of having enough required digits
        // but there is still more input to be consumed.
        // We'll return true, as the constraint has been 
        // satisfied.
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
    // Parse a number from the given ByteSpan, advancing the start
    // of the ByteSpan to beyond where we found the last character of the number.
    // 
	// Note: These numbers support a full mantissa, so the extended 'e' notation
    // is supported.  When reading numbers from a style sheet, readCSSNumber should
    // be used.
    // 
    // Construction:
    // number ::= integer ([Ee] integer)?
    //    | [+-] ? [0 - 9] * "."[0 - 9] + ([Ee] integer) ?

    // Assumption:  We're sitting at beginning of a number, all whitespace handling
    // has already occured.
    static bool INLINE readNumber(ByteSpan& s, double& value) noexcept
    {
        if (s.empty())
            return false;

        const unsigned char* startAt = s.fStart;
        const unsigned char* endAt = s.fEnd;

        bool isNegative = false;
        double res = 0.0;

        // integer part
        uint64_t intPart = 0;

        // fractional part
        uint64_t fracPart = 0;
        uint64_t fracBase = 1;


        bool hasIntPart = false;
        bool hasFracPart = false;



        // Parse optional sign
        if (*startAt == '+' || *startAt == '-')
        {
            isNegative = (*startAt == '-');
            startAt++;
            if (startAt >= endAt) return false;

        }

        // Parse integer part
        size_t digitsRead = 0;
		unsigned char c = *startAt;
        if (is_digit(c))
        {
            hasIntPart = true;
            s.fStart = startAt;
            read_u64(s, intPart, digitsRead);
            startAt = s.fStart;
            res = static_cast<double>(intPart);
        }

        // Parse fractional part.
        bool fracHasDigits = false;
        if ((startAt < endAt) && (*startAt == '.'))
        {
            startAt++; // Skip '.'

            fracBase = 1;

            // Add the fraction portion without calling out to powd
            while ((startAt < endAt) && is_digit(*startAt)) 
            {
                fracHasDigits = true;
                fracPart = fracPart * 10 + static_cast<uint64_t>(*startAt - '0');
                fracBase *= 10;
                startAt++;
            }

            if (fracHasDigits) 
            {
                hasFracPart = true;
                res += (static_cast<double>(fracPart) / static_cast<double>(fracBase));
            }

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
                read_u64(s, expPart, digitsRead);
                startAt = s.fStart;
                res = res * std::pow(10, double(expSign * double(expPart)));
            }
        }
        s.fStart = startAt;

        value = (!isNegative)? res : -res;

        return true;
    }

    static INLINE bool parseNumber(const ByteSpan& inChunk, double& value)
    {
        ByteSpan s = inChunk;
        return readNumber(s, value);
    }

    // readNextFloat()
    // 
    // Consume the next number off the front of the chunk
    // modifying the input chunk to advance past the  removed number
    // Return true if we found a number, false otherwise
    //
    static inline bool readNextFloat(ByteSpan& s, float& outNumber) noexcept
    {
        // typical whitespace found in lists of numbers, 
        // like on paths and polylines
        //static const charset nextNumWsp = chrWspChars + ",+"; // (",+\t\n\f\r ");
        static const charset nextNumWsp = chrWspChars + ","; // (",+\t\n\f\r ");

        s.skipWhile(nextNumWsp);

        if (s.empty())
            return false;
        double aNumber{ 0 };
        auto success = readNumber(s, aNumber);

		if (success)
		{
			outNumber = (float)aNumber;
			return true;
		}

        return false;
    }

    static inline bool readNextNumber(ByteSpan& s, double& outNumber) noexcept
    {
        // typical whitespace found in lists of numbers, 
        // like on paths and polylines
        //static const charset nextNumWsp = chrWspChars + ",+"; // (",+\t\n\r ");
        static const charset nextNumWsp = chrWspChars + ","; // (",+\t\n\r ");

		s.skipWhile(nextNumWsp);

        if (s.empty())
            return false;

		return readNumber(s, outNumber);
    }

    static inline bool readNextFlag(ByteSpan& s, int& outNumber) noexcept
    {
        // typical whitespace found in lists of numbers, like on paths and polylines
        static charset wspChars = chrWspChars + ",";

        // clear up leading whitespace, including ','
        s.skipWhile(wspChars);

        if (s.empty())
            return false;

        if (*s == '0' || *s == '1') {
            outNumber = (int)(*s - '0');
            s++;
            return true;
        }

        return false;

    }


    
    // readNumericArguments()
    //
    // Read a list of numeric arguments as specified in the 'argTypes'
    // c, r - read a number
	// f - read a flag
    // 
	// The number of arguments read is determined by the length of the argTypes string
    //
    static int readFloatArguments(ByteSpan& s, const char* argTypes, float* outArgs) noexcept
    {
        int i = 0;
        for (i = 0; argTypes[i]; i++)
        {
            switch (argTypes[i])
            {
            case 'c':		// read a coordinate
            case 'r':		// read a radius
            {
                if (!readNextFloat(s, outArgs[i]))
                    return i;
            } break;

            case 'f':		// read a flag
            {
                int aflag{ 0 };
                if (!readNextFlag(s, aflag))
                    return i;
                outArgs[i] = aflag;
            } break;

            default:
            {
                return 0;
            }
            }
        }

        return i;
    }

    static int readNumericArguments(ByteSpan& s, const char* argTypes, double* outArgs) noexcept
    {
        // typical whitespace found in lists of numbers, like on paths and polylines
        //static charset segWspChars(",\t\n\f\r ");

        int i = 0;
        for (i = 0; argTypes[i]; i++)
        {
            switch (argTypes[i])
            {
            case 'c':		// read a coordinate
            case 'r':		// read a radius
            {
                if (!readNextNumber(s, outArgs[i]))
                    return i;
            } break;

            case 'f':		// read a flag
            {
                int aflag{ 0 };
                if (!readNextFlag(s, aflag))
                    return i;
                outArgs[i] = aflag;

            } break;

            default:
            {
                return 0;
            }
            }
        }

        return i;
    }
}


#endif // _CONVERTERS_H_INCLUDED