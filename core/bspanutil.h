#pragma once


//	Herein you will find various objects and functions which are
//	useful while scanning, tokenizing, parsing streams of text.

#include "bspan.h"
#include "charset.h"
#include "maths.h"
#include "bsfunc.h"

#include <cmath>
#include <charconv>

namespace waavs {
	static void writeChunkToFile(const ByteSpan& chunk, const char* filename)
	{
		FILE* f{};
		errno_t err = fopen_s(&f, filename, "wb");
		if ((err != 0) || (f == nullptr))
			return;
		
		fwrite(chunk.data(), 1, chunk.size(), f);
		fclose(f);
	}
	
	static void writeChunk(const ByteSpan& chunk)
	{
		ByteSpan s = chunk;

		while (s && *s) {
			printf("%c", *s);
			s++;
		}
	}

	static void writeChunkBordered(const ByteSpan& chunk)
	{
		ByteSpan s = chunk;

		printf("||");
		while (s && *s) {
			printf("%c", *s);
			s++;
		}
		printf("||");
	}

	static void printChunk(const ByteSpan& chunk)
	{
		if (chunk)
		{
			writeChunk(chunk);
			printf("\n");
		}
		else
			printf("BLANK==CHUNK\n");

	}
}

namespace waavs
{
	// simple type parsing
	static inline int64_t toInteger(const ByteSpan& inChunk) noexcept;
	static inline double toNumber(const ByteSpan& inChunk) noexcept;
	static inline double toDouble(const ByteSpan& inChunk) noexcept;
	static inline std::string toString(const ByteSpan& inChunk) noexcept;
	static inline int toBoolInt(const ByteSpan& inChunk) noexcept;
}

namespace waavs
{
	static charset wspChars(" \r\n\t\f\v");		// a set of typical whitespace chars
	


		static inline size_t copy_to_cstr(char* str, size_t len, const ByteSpan& a) noexcept;
		static inline ByteSpan chunk_ltrim(const ByteSpan& a, const charset& skippable) noexcept;
		static inline ByteSpan chunk_rtrim(const ByteSpan& a, const charset& skippable) noexcept;
		static inline ByteSpan chunk_trim(const ByteSpan& a, const charset& skippable) noexcept;
		static inline ByteSpan chunk_skip_wsp(const ByteSpan& a) noexcept;
		
		static inline bool chunk_starts_with(const ByteSpan& a, const ByteSpan& b) noexcept;
		static inline bool chunk_starts_with_char(const ByteSpan& a, const uint8_t b) noexcept;
		static inline bool chunk_starts_with_cstr(const ByteSpan& a, const char* b) noexcept;
		
		static inline bool chunk_ends_with(const ByteSpan& a, const ByteSpan& b) noexcept;
		static inline bool chunk_ends_with_char(const ByteSpan& a, const uint8_t b) noexcept;
		static inline bool chunk_ends_with_cstr(const ByteSpan& a, const char* b) noexcept;
		
		static inline ByteSpan chunk_token(ByteSpan& a, const charset& delims) noexcept;
		static inline ByteSpan chunk_find_char(const ByteSpan& a, char c) noexcept;

		// Number Conversions
		static inline double chunk_to_double(const ByteSpan& inChunk) noexcept;
		
}



namespace waavs
{
	static inline size_t copy_to_cstr(char* str, size_t len, const ByteSpan& a) noexcept
	{
		size_t maxBytes = chunk_size(a) < len ? chunk_size(a) : len;
		memcpy(str, a.fStart, maxBytes);
		str[maxBytes] = 0;

		return maxBytes;
	}

	// Trim the left side of skippable characters
	static inline ByteSpan chunk_ltrim(const ByteSpan& a, const charset& skippable) noexcept
	{
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		while (start < end && skippable(*start))
			++start;
		return { start, end };
	}

	// trim the right side of skippable characters
	static inline ByteSpan chunk_rtrim(const ByteSpan& a, const charset& skippable) noexcept
	{
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		while (start < end && skippable(*(end - 1)))
			--end;

		return { start, end };
	}

	// trim the left and right side of skippable characters
	static inline ByteSpan chunk_trim(const ByteSpan& a, const charset& skippable) noexcept
	{
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		while (start < end && skippable(*start))
			++start;
		while (start < end && skippable(*(end - 1)))
			--end;
		return { start, end };
	}

	static inline ByteSpan chunk_skip_wsp(const ByteSpan& a) noexcept
	{
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		while (start < end && wspChars(*start))
			++start;
		return { start, end };
	}



	static inline bool chunk_starts_with(const ByteSpan& a, const ByteSpan& b) noexcept
	{
		return chunk_is_equal(chunk_subchunk(a, 0, chunk_size(b)), b);
	}

	static inline bool chunk_starts_with_char(const ByteSpan& a, const uint8_t b) noexcept
	{
		return chunk_size(a) > 0 && a.fStart[0] == b;
	}

	static inline bool chunk_starts_with_cstr(const ByteSpan& a, const char* b) noexcept
	{
		return chunk_starts_with(a, chunk_from_cstr(b));
	}

	static inline bool chunk_ends_with(const ByteSpan& a, const ByteSpan& b) noexcept
	{
		return chunk_is_equal(chunk_subchunk(a, chunk_size(a) - chunk_size(b), chunk_size(b)), b);
	}

	static inline bool chunk_ends_with_char(const ByteSpan& a, const uint8_t b) noexcept
	{
		return chunk_size(a) > 0 && a.fEnd[-1] == b;
	}

	static inline bool chunk_ends_with_cstr(const ByteSpan& a, const char* b) noexcept
	{
		return chunk_ends_with(a, chunk_from_cstr(b));
	}

	// Given an input chunk
	// spit it into two chunks, 
	// Returns - the first chunk before delimeters
	// a - adjusted to reflect the rest of the input after delims
	// If delimeter NOT found
	// returns the entire input chunk
	// and 'a' is set to an empty chunk
	static inline ByteSpan chunk_token(ByteSpan& a, const charset& delims) noexcept
	{
		if (!a) {
			a = {};
			return {};
		}
		
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		const uint8_t* tokenEnd = start;
		while (tokenEnd < end && !delims(*tokenEnd))
			++tokenEnd;

		if (delims(*tokenEnd))
		{
			a.fStart = tokenEnd + 1;
		}
		else {
			a.fStart = tokenEnd;
		}

		return { start, tokenEnd };
	}

	// name alias
	static inline ByteSpan nextToken(ByteSpan& a, const charset&& delims) noexcept
	{
		return chunk_token(a, delims);
	}

	// Given an input chunk
	// find the first instance of a specified character
	// return the chunk preceding the found character
	// or or the whole chunk of the character is not found
	static inline ByteSpan chunk_find_char(const ByteSpan& a, char c) noexcept
	{
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		while (start < end && *start != c)
			++start;

		return { start, end };
	}

	static inline ByteSpan chunk_find_cstr(const ByteSpan& a, const char* c) noexcept
	{
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		const uint8_t* cstart = (const uint8_t*)c;
		const uint8_t* cend = cstart + strlen(c);
		while (start < end && !chunk_starts_with({ start, end }, { cstart, cend }))
			++start;

		return { start, end };
	}

	static inline ByteSpan chunk_read_bracketed(ByteSpan& src, const uint8_t lbracket, const uint8_t rbracket) noexcept
	{
		uint8_t* beginattrValue = nullptr;
		uint8_t* endattrValue = nullptr;
		uint8_t quote{};

		// Skip white space before the quoted bytes
		src = chunk_ltrim(src, wspChars);

		if (!src || *src != lbracket)
			return {};


		// advance past the lbracket, then look for the matching close quote
		src++;
		beginattrValue = (uint8_t*)src.fStart;

		// Skip until end of the value.
		while (src && *src != rbracket)
			src++;

		if (src)
		{
			endattrValue = (uint8_t*)src.fStart;
			src++;
		}

		// Store only well formed quotes
		return { beginattrValue, endattrValue };
	}
	
	// Take a chunk containing a series of digits and turn
	// it into a 64-bit unsigned integer
	// Stop processing when the first non-digit is seen, 
	// or the end of the chunk
	static inline uint64_t chunk_to_u64(ByteSpan& s)
	{
		static charset digitChars("0123456789");

		uint64_t v = 0;

		while (s && digitChars(*s))
		{
			v = v * 10 + (uint64_t)(*s - '0');
			s++;
		}

		return v;
	}

	static inline int64_t chunk_to_i64(ByteSpan& s)
	{
		static charset digitChars("0123456789");

		int64_t v = 0;

		bool negative = false;
		if (s && *s == '-')
		{
			negative = true;
			s++;
		}

		while (s && digitChars(*s))
		{
			v = v * 10 + (int64_t)(*s - '0');
			s++;
		}

		if (negative)
			v = -v;

		return v;
	}

	// Parse a number which may have units after it
//   1.2em
// -1.0E2em
// 2.34ex
// -2.34e3M10,20
// 
// By the end of this routine, the numchunk represents the range of the 
// captured number.
// 
// The returned chunk represents what comes next, and can be used
// to continue scanning the original inChunk
//
// Note:  We assume here that the inChunk is already positioned at the start
// of a number (including +/- sign), with no leading whitespace

	static ByteSpan scanNumber(const ByteSpan& inChunk, ByteSpan& numchunk)
	{
		static charset digitChars("0123456789");                   // only digits

		ByteSpan s = inChunk;
		numchunk = inChunk;
		numchunk.fEnd = inChunk.fStart;


		// sign
		if (*s == '-' || *s == '+') {
			s++;
			numchunk.fEnd = s.fStart;
		}

		// integer part
		while (s && digitChars[*s]) {
			s++;
			numchunk.fEnd = s.fStart;
		}

		if (*s == '.') {
			// decimal point
			s++;
			numchunk.fEnd = s.fStart;

			// fraction part
			while (s && digitChars[*s]) {
				s++;
				numchunk.fEnd = s.fStart;
			}
		}

		// exponent
		// but it could be units (em, ex)
		if ((*s == 'e' || *s == 'E') && (s[1] != 'm' && s[1] != 'x'))
		{
			s++;
			numchunk.fEnd = s.fStart;

			// Might be a sign
			if (*s == '-' || *s == '+') {
				s++;
				numchunk.fEnd = s.fStart;
			}

			// Get any remaining digits
			while (s && digitChars[*s]) {
				s++;
				numchunk.fEnd = s.fStart;
			}
		}

		return s;
	}

	// parse floating point number
	// includes sign, exponent, and decimal point
	// The input chunk is altered, with the fStart pointer moved to the end of the number

	static inline double chunk_to_double(const ByteSpan& inChunk) noexcept
	{
		ByteSpan s = inChunk;
		/*
		double outNumber = 0;

		
		auto res = std::from_chars((const char*)s.fStart, (const char*)s.fEnd, outNumber);
		if (res.ec == std::errc::invalid_argument)
		{
			printf("chunk_to_double: INVALID ARGUMENT: ");
			printChunk(s);
		}

		return outNumber;
		*/
		
		///*
		static charset digitChars("0123456789");

		double sign = 1.0;
		double res = 0.0;
		long long intPart = 0;
		uint64_t fracPart = 0;
		bool hasIntPart = false;
		bool hasFracPart = false;

		// Parse optional sign
		if (*s == '+') {
			s++;
		}
		else if (*s == '-') {
			sign = -1;
			s++;
		}

		// Parse integer part
		if (digitChars[*s]) {

			intPart = chunk_to_u64(s);

			res = (double)intPart;
			hasIntPart = true;
		}

		// Parse fractional part.
		if (*s == '.') {
			s++; // Skip '.'
			auto sentinel = s.fStart;

			if (digitChars(*s)) {
				fracPart = chunk_to_u64(s);
				auto ending = s.fStart;

				ptrdiff_t diff = ending - sentinel;
				res = res + ((double)fracPart) / (double)powd((double)10, double(diff));
				hasFracPart = true;
			}
		}

		// A valid number should have integer or fractional part.
		if (!hasIntPart && !hasFracPart)
			return 0.0;


		// Parse optional exponent
		if (*s == 'e' || *s == 'E') {
			long long expPart = 0;
			s++; // skip 'E'

			double expSign = 1.0;
			if (*s == '+') {
				s++;
			}
			else if (*s == '-') {
				expSign = -1.0;
				s++;
			}

			if (digitChars[*s]) {
				expPart = chunk_to_u64(s);
				res = res * powd(10, double(expSign * double(expPart)));
			}
		}

		return res * sign;
		//*/
	}

	// Consume the next number off the front of the chunk
	// modifying the input chunk to advance past the  number
	// we removed.
	// Return true if we found a number, false otherwise
	static inline bool parseNextNumber(ByteSpan& s, double& outNumber)
	{
		static charset whitespaceChars(",\t\n\f\r ");          // whitespace found in paths

		// clear up leading whitespace, including ','
		s = chunk_ltrim(s, whitespaceChars);

		ByteSpan numChunk{};
		s = scanNumber(s, numChunk);

		if (!numChunk)
			return false;

		outNumber = toDouble(numChunk);

		return true;
	}
}





namespace waavs {

	static inline int64_t toInteger(const ByteSpan& inChunk) noexcept
	{
		ByteSpan s = inChunk;
		return chunk_to_i64(s);
	}

	// toNumber
	// a floating point number
	static inline double toNumber(const ByteSpan& inChunk) noexcept
	{
		ByteSpan s = inChunk;
		return chunk_to_double(s);
	}

	static inline double toDouble(const ByteSpan& s) noexcept
	{
		return chunk_to_double(s);

	}

	// return 1 if the chunk is "true" or "1" or "t" or "T" or "y" or "Y" or "yes" or "Yes" or "YES"
	// return 0 if the chunk is "false" or "0" or "f" or "F" or "n" or "N" or "no" or "No" or "NO"
	// return 0 otherwise
	static inline int toBoolInt(const ByteSpan& inChunk) noexcept
	{
		ByteSpan s = inChunk;

		if (s == "true" || s == "1" || s == "t" || s == "T" || s == "y" || s == "Y" || s == "yes" || s == "Yes" || s == "YES")
			return 1;
		else if (s == "false" || s == "0" || s == "f" || s == "F" || s == "n" || s == "N" || s == "no" || s == "No" || s == "NO")
			return 0;
		else
			return 0;
	}

	static inline std::string toString(const ByteSpan& inChunk) noexcept
	{
		return std::string(inChunk.fStart, inChunk.fEnd);
	}

}


/*
	
	static inline void skipOverCharset(ByteSpan& dc, const charset& cs)
	{
		while (dc && cs.contains(*dc))
			dc++;
	}

	static inline void skipUntilCharset(ByteSpan& dc, const charset& cs)
	{
		while (dc && !cs.contains(*dc))
			++dc;
	}
	
	    // Turn a chunk into a vector of chunks, splitting on the delimiters
    // return number of tokens found
    static inline  int chunk_split(const ByteSpan& inChunk, const charset& delims, std::vector<svg2b2d::ByteSpan> spans, bool wantEmpties = false) noexcept
    {
        ByteSpan s = inChunk;

        while (s)
        {
            ByteSpan token = chunk_token(s, delims);
            //if (size(token) > 0)
            spans.push_back(token);

        }

        return spans.size();
    }
	
*/