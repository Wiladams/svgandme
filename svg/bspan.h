#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>	// for std::data(), std::size()

#include "bithacks.h"
#include "charset.h"
#include "maths.h"

namespace waavs {


	//
	// A core type for representing a contiguous sequence of bytes
	// As of C++ 20, there is std::span, but it is not yet widely supported
	// 
	// The ByteSpan is used in everything from networking
	// to graphics bitmaps to audio buffers.
	// Having a universal representation of a chunk of data
	// allows for easy interoperability between different
	// subsystems.  
	// 
	// It also allows us to eliminate disparate implementations that
	// are used for the same purpose.

	struct ByteSpan
	{
		const unsigned char* fStart{ nullptr };
		const unsigned char* fEnd{ nullptr };

		// Constructors
		ByteSpan() : fStart(nullptr), fEnd(nullptr) {}
		ByteSpan(const unsigned char* start, const unsigned char* end) : fStart(start), fEnd(end) {}
		ByteSpan(const char* cstr) : fStart((const unsigned char*)cstr), fEnd((const unsigned char*)cstr + strlen(cstr)) {}
		explicit ByteSpan(const void* data, size_t sz) :fStart((const unsigned char*)data), fEnd((const unsigned char*)data + sz) {}



		// Type conversions
		explicit operator bool() const { return (fEnd - fStart) > 0; };


		// Array access
		unsigned char& operator[](size_t i) { return ((unsigned char*)fStart)[i]; }
		const unsigned char& operator[](size_t i) const { return ((unsigned char*)fStart)[i]; }

		// get current value from fStart, like a 'peek' operation
		unsigned char& operator*() { static unsigned char zero = 0;  if (fStart < fEnd) return *(unsigned char*)fStart; return  zero; }
		const uint8_t& operator*() const { static unsigned char zero = 0;  if (fStart < fEnd) return *(unsigned char*)fStart; return  zero; }

		ByteSpan& operator+= (size_t n) {
			if (n > size())
				n = size();
			fStart += n;

			return *this;
		}


		ByteSpan& operator++() { return operator+=(1); }			// prefix notation ++y
		ByteSpan& operator++(int i) { return operator+=(1); }       // postfix notation y++



		// setting up for a range-based for loop
		const unsigned char* data() const noexcept { return (unsigned char*)fStart; }
		const unsigned char* begin() const noexcept { return fStart; }
		const unsigned char* end() const noexcept { return fEnd; }
		size_t size()  const noexcept { return fEnd - fStart; }
		const bool empty() const noexcept { return fStart == fEnd; }

		void setAll(unsigned char c) noexcept { memset((uint8_t*)fStart, c, size()); }
		
		//
		// Note:  For the various 'as_xxx' routines, there is no size
		// error checking.  It is assumed that whatever is calling the
		// function will do appropriate error checking beforehand.
		// This is a little unsafe, but allows the caller to decide where
		// they want to do the error checking, and therefore control the
		// performance characteristics better.

		// Read a single byte
		uint8_t as_u8()  const noexcept
		{
			uint8_t result = *((uint8_t*)fStart);

			return result;
		}

		// Read a unsigned 16 bit value
		// assuming stream is in little-endian format
		// and machine is also little-endian
		uint16_t as_u16_le() const noexcept
		{
			uint16_t r = *((uint16_t*)fStart);

			return r;

			//return ((uint8_t *)fStart)[0] | (((uint8_t *)fStart)[1] << 8);
		}

		// Read a unsigned 32-bit value
		// assuming stream is in little endian format
		uint32_t as_u32_le() const noexcept
		{
			uint32_t r = *((uint32_t*)fStart);


			return r;

			//return ((uint8_t *)fStart)[0] | (((uint8_t *)fStart)[1] << 8) | (((uint8_t *)fStart)[2] << 16) |(((uint8_t *)fStart)[3] << 24);
		}

		// Read a unsigned 64-bit value
		// assuming stream is in little endian format
		uint64_t as_u64_le() const noexcept
		{
			uint64_t r = *((uint64_t*)fStart);

			return r;
			//return ((uint8_t *)fStart)[0] | (((uint8_t *)fStart)[1] << 8) | (((uint8_t *)fStart)[2] << 16) | (((uint8_t *)fStart)[3] << 24) |
			//    (((uint8_t *)fStart)[4] << 32) | (((uint8_t *)fStart)[5] << 40) | (((uint8_t *)fStart)[6] << 48) | (((uint8_t *)fStart)[7] << 56);
		}

		//=============================================
		// BIG ENDIAN
		//=============================================
		// Read a unsigned 16 bit value
		// assuming stream is in big endian format
		uint16_t as_u16_be() noexcept
		{
			uint16_t r = *((uint16_t*)fStart);
			return bswap16(r);
		}

		// Read a unsigned 32-bit value
		// assuming stream is in big endian format
		uint32_t as_u32_be() noexcept
		{
			uint32_t r = *((uint32_t*)fStart);
			return bswap32(r);
		}

		// Read a unsigned 64-bit value
		// assuming stream is in big endian format
		uint64_t as_u64_be() noexcept
		{
			uint64_t r = *((uint64_t*)fStart);
			return bswap64(r);
		}

	};


	static inline size_t copy(ByteSpan& a, const ByteSpan& b) noexcept;
	static inline size_t copy_to_cstr(char* str, size_t len, const ByteSpan& a) noexcept;
	static inline int compare(const ByteSpan& a, const ByteSpan& b) noexcept;
	static inline int comparen(const ByteSpan& a, const ByteSpan& b, int n) noexcept;
	static inline int comparen_cstr(const ByteSpan& a, const char* b, int n) noexcept;
	static inline bool chunk_is_equal(const ByteSpan& a, const ByteSpan& b) noexcept;
	static inline bool chunk_is_equal_cstr(const ByteSpan& a, const char* s) noexcept;

	// Some utility functions for common operations

	static inline void chunk_truncate(ByteSpan& dc) noexcept;
	static inline ByteSpan& chunk_skip(ByteSpan& dc, size_t n) noexcept;
	static inline ByteSpan& chunk_skip_to_end(ByteSpan& dc) noexcept;

	

	//
	// operators for comparison
	// operator!=;
	// operator<=;
	// operator>=;
	static inline bool operator==(const ByteSpan& a, const ByteSpan& b) noexcept;
	static inline bool operator==(const ByteSpan& a, const char* b) noexcept;
	static inline bool operator< (const ByteSpan& a, const ByteSpan& b) noexcept;
	static inline bool operator> (const ByteSpan& a, const ByteSpan& b) noexcept;
	static inline bool operator!=(const ByteSpan& a, const ByteSpan& b) noexcept;
	static inline bool operator<=(const ByteSpan& a, const ByteSpan& b) noexcept;
	static inline bool operator>=(const ByteSpan& a, const ByteSpan& b) noexcept;


	// ByteSpan routines
	static inline ByteSpan chunk_from_cstr(const char* data) noexcept { return ByteSpan{ (uint8_t*)data, (uint8_t*)data + strlen(data) }; }


	static inline size_t chunk_size(const ByteSpan& a) noexcept { return a.size(); }
	static inline bool chunk_empty(const ByteSpan& dc)  noexcept { return dc.fEnd == dc.fStart; }
	static inline size_t copy(ByteSpan& a, const ByteSpan& b) noexcept
	{
		size_t maxBytes = a.size() < b.size() ? a.size() : b.size();
		memcpy((uint8_t*)a.fStart, b.fStart, maxBytes);
		return maxBytes;
	}

	static inline int compare(const ByteSpan& a, const ByteSpan& b) noexcept
	{
		size_t maxBytes = a.size() < b.size() ? a.size() : b.size();
		return memcmp(a.fStart, b.fStart, maxBytes);
	}

	static inline int comparen(const ByteSpan& a, const ByteSpan& b, int n) noexcept
	{
		size_t maxBytes = a.size() < b.size() ? a.size() : b.size();
		if (maxBytes > n)
			maxBytes = n;
		return memcmp(a.fStart, b.fStart, maxBytes);
	}

	static inline int comparen_cstr(const ByteSpan& a, const char* b, int n) noexcept
	{
		size_t maxBytes = a.size() < n ? a.size() : n;
		return memcmp(a.fStart, b, maxBytes);
	}

	static inline bool chunk_is_equal(const ByteSpan& a, const ByteSpan& b) noexcept
	{
		if (a.size() != b.size())
			return false;
		return memcmp(a.fStart, b.fStart, a.size()) == 0;
	}

	static inline bool chunk_is_equal_cstr(const ByteSpan& a, const char* cstr) noexcept
	{
		size_t len = strlen(cstr);
		if (a.size() != len)
			return false;
		return memcmp(a.fStart, cstr, len) == 0;
	}


	static inline void chunk_truncate(ByteSpan& dc) noexcept
	{
		dc.fEnd = dc.fStart;
	}

	static inline ByteSpan& chunk_skip(ByteSpan& dc, size_t n) noexcept
	{
		if (n > dc.size())
			n = dc.size();
		dc.fStart += n;

		return dc;
	}

	static inline ByteSpan& chunk_skip_to_end(ByteSpan& dc) noexcept { dc.fStart = dc.fEnd; }


	

	
	static inline bool operator==(const ByteSpan& a, const ByteSpan& b) noexcept
	{
		if (a.size() != b.size())
			return false;
		return memcmp(a.fStart, b.fStart, a.size()) == 0;
	}

	static inline bool operator==(const ByteSpan& a, const char* b) noexcept
	{
		size_t len = strlen(b);
		if (a.size() != len)
			return false;
		return memcmp(a.fStart, b, len) == 0;
	}

	static inline bool operator!=(const ByteSpan& a, const ByteSpan& b) noexcept
	{
		if (a.size() != b.size())
			return true;
		return memcmp(a.fStart, b.fStart, a.size()) != 0;
	}

	static inline bool operator<(const ByteSpan& a, const ByteSpan& b) noexcept
	{
		size_t maxBytes = a.size() < b.size() ? a.size() : b.size();
		return memcmp(a.fStart, b.fStart, maxBytes) < 0;
	}

	static inline bool operator>(const ByteSpan& a, const ByteSpan& b) noexcept
	{
		size_t maxBytes = a.size() < b.size() ? a.size() : b.size();
		return memcmp(a.fStart, b.fStart, maxBytes) > 0;
	}

	static inline bool operator<=(const ByteSpan& a, const ByteSpan& b) noexcept
	{
		size_t maxBytes = a.size() < b.size() ? a.size() : b.size();
		return memcmp(a.fStart, b.fStart, maxBytes) <= 0;
	}

	static inline bool operator>=(const ByteSpan& a, const ByteSpan& b) noexcept
	{
		size_t maxBytes = a.size() < b.size() ? a.size() : b.size();
		return memcmp(a.fStart, b.fStart, maxBytes) >= 0;
	}


}


// Implementation of hash function for ByteSpan
// so it can be used in 'map' collections
namespace std {
	template<>
	struct hash<waavs::ByteSpan> {
		size_t operator()(const waavs::ByteSpan& span) const {
			uint32_t hash = 0;
			
			for (const unsigned char* p = span.fStart; p != span.fEnd; ++p) {
				hash = hash * 31 + *p;
			}
			return hash;
		}
	};
}


// Functions that are implemented here
namespace waavs {

	static inline ByteSpan chunk_subchunk(const ByteSpan& a, const size_t start, const size_t sz) noexcept;
	static inline ByteSpan chunk_take(const ByteSpan& dc, size_t n) noexcept;

	//static void writeChunkToFile(const ByteSpan& chunk, const char* filename) noexcept;
	static void writeChunk(const ByteSpan& chunk) noexcept;
	static void writeChunkBordered(const ByteSpan& chunk) noexcept;
	static void printChunk(const ByteSpan& chunk) noexcept;
}

namespace waavs
{
	static charset digitChars("0123456789");

	static inline uint64_t chunk_to_u64(ByteSpan& s) noexcept;
	static inline int64_t chunk_to_i64(ByteSpan& s) noexcept;

	// simple type parsing
	static inline int64_t toInteger(const ByteSpan& inChunk) noexcept;
	static inline double toNumber(const ByteSpan& inChunk) noexcept;
	static inline double toDouble(const ByteSpan& inChunk) noexcept;
	static inline std::string toString(const ByteSpan& inChunk) noexcept;
	static inline int toBoolInt(const ByteSpan& inChunk) noexcept;

	// Number Conversions
	static inline double chunk_to_double(const ByteSpan& inChunk) noexcept;
}

namespace waavs
{
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



}


namespace waavs {

	// Create a bytespan that is a subspan of another bytespan
	static inline ByteSpan chunk_subchunk(const ByteSpan& a, const size_t startAt, const size_t sz) noexcept
	{
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		if (startAt < chunk_size(a))
		{
			start += startAt;
			if (start + sz < end)
				end = start + sz;
			else
				end = a.fEnd;
		}
		else
		{
			start = end;
		}
		return { start, end };
	}


	static inline ByteSpan chunk_take(const ByteSpan& dc, size_t n) noexcept
	{
		return chunk_subchunk(dc, 0, n);
	}
}

namespace waavs {
	/*
	static void writeChunkToFile(const ByteSpan& chunk, const char* filename) noexcept
	{
		FILE* f{};
		errno_t err = fopen_s(&f, filename, "wb");
		if ((err != 0) || (f == nullptr))
			return;

		fwrite(chunk.data(), 1, chunk.size(), f);
		fclose(f);
	}
	*/
	
	static void writeChunk(const ByteSpan& chunk) noexcept
	{
		ByteSpan s = chunk;

		while (s && *s) {
			printf("%c", *s);
			s++;
		}
	}

	static void writeChunkBordered(const ByteSpan& chunk) noexcept
	{
		ByteSpan s = chunk;

		printf("||");
		while (s && *s) {
			printf("%c", *s);
			s++;
		}
		printf("||");
	}

	static void printChunk(const ByteSpan& chunk) noexcept
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
	static charset wspChars(" \r\n\t\f\v");		// a set of typical whitespace chars


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
		while (start < end&& wspChars(*start))
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
	// This routine alters the input chunk to reflect the remaining
	// characters after the number
	static inline uint64_t chunk_to_u64(ByteSpan& s) noexcept
	{
		uint64_t v = 0;

		while (s && digitChars(*s))
		{
			v = v * 10 + (uint64_t)(*s - '0');
			s++;
		}

		return v;
	}

	static inline int64_t chunk_to_i64(ByteSpan& s) noexcept
	{
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



	//
	// chunk_to_double()
	// 
	// parse floating point number
	// includes sign, exponent, and decimal point
	// The input chunk is altered, with the fStart pointer moved to the end of the number
	// Note:  If we want to include "charconv", the we can use the std::from_chars
	// This should be a fast implementation, far batter than something like atof, or sscanf
	// But, the routine here should be universally fast, when charconv is not available on the 
	// target platform.
	//
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
	
	static inline double chunk_to_double(const ByteSpan& inChunk) noexcept
	{
		ByteSpan s = inChunk;

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

namespace waavs {
	//
	// MemBuff
	// 
	// This is a very simple data structure that allocates a chunk of memory
	// When the destructor is called, the memory is freed.
	// This could easily be handled by something like a unique_ptr, but I don't
	// want to force the usage of std library when it's not really needed.
	// besides, it's so easy and convenient and small.
	// Note:  This could be a sub-class of ByteSpan, but the semantics are different
	// With a ByteSpan, you can alter the start/end pointers, but with a memBuff, you can't.
	// so, it is much easier to return a ByteSpan, and let that be manipulated instead.
	// 
	
	struct MemBuff {
		uint8_t* fData{};
		ptrdiff_t fSize{};

		MemBuff() {}
		
		MemBuff(size_t sz)
		{
			initSize(sz);
		}

		~MemBuff()
		{
			if (fData != nullptr)
				delete[] fData;
		}

		uint8_t* data() const { return fData; }
		size_t size() const { return fSize; }

		// initSize
		// Initialize the memory buffer with a given size
		bool initSize(const size_t sz)
		{
			fData = new uint8_t[sz];
			fSize = sz;

			return true;
		}
		
		// initFromSpan
		// copy the data from the input span into the memory buffer
		//
		bool initFromSpan(const ByteSpan& srcSpan)
		{
			if (fData != nullptr)
				delete[] fData;
			
			fSize = srcSpan.size();
			fData = new uint8_t[fSize];

			memcpy(fData, srcSpan.fStart, fSize);
			
			return true;
		}
		

		// create a ByteSpan from the memory buffer
		// The lifetime of the ByteSpan that is returned it not governed
		// by the MemBuff object.  This is something the caller must manage.
		ByteSpan span() const { return ByteSpan(fData, fData + fSize); }

	};
}


