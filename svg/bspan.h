#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>	// for std::data(), std::size()

#include "bithacks.h"
#include "charset.h"


namespace waavs {


	//
	// ByteSpan
	// 
	// A core type for representing a contiguous sequence of bytes.
	// As of C++ 20, there is std::span<>, and that would be a good 
	// choice, but it is not yet widely supported, and forces a jump
	// to C++20 besides.
	// 
	// The ByteSpan is used in everything from networking
	// to graphics bitmaps to audio buffers.
	// Having a universal representation of a chunk of data
	// allows for easy interoperability between different
	// subsystems.  
	// 
	// The ByteSpan, is just like 'span' and 'view' objects
	// it does not "own" the memory, it just points at it.
	// It is used as a stand-in for various data representations.
	// A key aspect of the ByteSpan is its ability to be used
	// as a 'cursor' to traverse the data it points to.


	struct ByteSpan
	{
		const unsigned char* fStart{ nullptr };
		const unsigned char* fEnd{ nullptr };

		// Constructors
		ByteSpan() noexcept : fStart(nullptr), fEnd(nullptr) {}
		ByteSpan(const unsigned char* start, const unsigned char* end) noexcept : fStart(start), fEnd(end) {}
		ByteSpan(const char* cstr) noexcept : fStart((const unsigned char*)cstr), fEnd((const unsigned char*)cstr + strlen(cstr)) {}
		explicit ByteSpan(const void* data, size_t sz) noexcept :fStart((const unsigned char*)data), fEnd((const unsigned char*)data + sz) {}

		// reset()
		void reset() { fStart = nullptr; fEnd = nullptr; }

		// Type conversions
		explicit operator bool() const noexcept { return (fEnd - fStart) > 0; };


		// Array access
		unsigned char& operator[](size_t i) noexcept { return ((unsigned char*)fStart)[i]; }
		const unsigned char& operator[](size_t i) const noexcept { return ((unsigned char*)fStart)[i]; }

		// get current value from fStart, like a 'peek' operation
		unsigned char& operator*() noexcept { static unsigned char zero = 0;  if (fStart < fEnd) return *(unsigned char*)fStart; return  zero; }
		const uint8_t& operator*() const noexcept { static unsigned char zero = 0;  if (fStart < fEnd) return *(unsigned char*)fStart; return  zero; }


		//
		// operators for comparison
		// operator==;
		// operator!=;
		// operator<=;
		// operator>=;
		
		bool equivalent(const ByteSpan& b) const noexcept
		{
			if (size() != b.size())
				return false;
			return memcmp(fStart, b.fStart, size()) == 0;
		}
		
		// operator==
		// Perform a full content comparison of the two spans
		bool operator==(const ByteSpan& b) const noexcept
		{
			if (size() != b.size())
				return false;
			return memcmp(fStart, b.fStart, size()) == 0;
		}

		bool operator==(const char* b) const noexcept
		{
			size_t len = strlen(b);
			if (size() != len)
				return false;
			return memcmp(fStart, b, len) == 0;
		}

		bool operator!=(const ByteSpan& b) const noexcept
		{
			if (size() != b.size())
				return true;
			return memcmp(fStart, b.fStart, size()) != 0;
		}

		bool operator<(const ByteSpan& b) const noexcept
		{
			size_t maxBytes = size() < b.size() ? size() : b.size();
			return memcmp(fStart, b.fStart, maxBytes) < 0;
		}

		bool operator>(const ByteSpan& b) const noexcept
		{
			size_t maxBytes = size() < b.size() ? size() : b.size();
			return memcmp(fStart, b.fStart, maxBytes) > 0;
		}

		bool operator<=(const ByteSpan& b) const noexcept
		{
			size_t maxBytes = size() < b.size() ? size() : b.size();
			return memcmp(fStart, b.fStart, maxBytes) <= 0;
		}

		bool operator>=(const ByteSpan& b) const noexcept
		{
			size_t maxBytes = size() < b.size() ? size() : b.size();
			return memcmp(fStart, b.fStart, maxBytes) >= 0;
		}


		ByteSpan& operator+= (size_t n) noexcept {
			if (n > size())
				n = size();
			fStart += n;

			return *this;
		}


		ByteSpan& operator++() noexcept { return operator+=(1); }			// prefix notation ++y
		ByteSpan& operator++(int ) noexcept { return operator+=(1); }       // postfix notation y++



		// setting up for a range-based for loop
		const unsigned char* data() const noexcept { return (unsigned char*)fStart; }
		const unsigned char* begin() const noexcept { return fStart; }
		const unsigned char* end() const noexcept { return fEnd; }
		size_t size()  const noexcept { ptrdiff_t sz = fEnd - fStart; if (sz < 0) return 0; return sz; }
		const bool empty() const noexcept { return fStart == fEnd; }

		void setAll(unsigned char c) noexcept { memset((uint8_t*)fStart, c, size()); }

		void copyFrom(const void* src, size_t sz) noexcept { memcpy((uint8_t*)fStart, src, sz); }
		
		// subSpan()
		// Create a bytespan that is a subspan of the current span
		// If the requested position plus size is greater than the amount
		// of span remaining at that position, the size will be truncated 
		// to the amount remaining from the requested position.
		ByteSpan subSpan(const size_t startAt, const size_t sz) const noexcept
		{
			const uint8_t* start = fStart;
			const uint8_t* end = fEnd;
			if (startAt < size())
			{
				start += startAt;
				if (start + sz < end)
					end = start + sz;
				else
					end = fEnd;
			}
			else
			{
				start = end;
			}
			return { start, end };
		}

		ByteSpan take(size_t n) const noexcept
		{
			return subSpan(0, n);
		}

		// Some convenient routines

		bool startsWith(const ByteSpan& b) const noexcept
		{
			return (subSpan((size_t)0, b.size()) == b);
		}

		bool endsWith(const ByteSpan& b) const noexcept
		{
			return (subSpan(size() - b.size(), b.size()) == b);
		}
	};

}

namespace waavs {
	INLINE size_t copy(ByteSpan& a, const ByteSpan& b) noexcept;
	INLINE size_t copy_to_cstr(char* str, size_t len, const ByteSpan& a) noexcept;
	INLINE int compare(const ByteSpan& a, const ByteSpan& b) noexcept;
	INLINE int comparen(const ByteSpan& a, const ByteSpan& b, int n) noexcept;
	INLINE int comparen_cstr(const ByteSpan& a, const char* b, int n) noexcept;
	//INLINE bool chunk_is_equal_cstr(const ByteSpan& a, const char* s) noexcept;

	// Some utility functions for common operations

	INLINE void chunk_truncate(ByteSpan& dc) noexcept;
	INLINE ByteSpan& chunk_skip(ByteSpan& dc, size_t n) noexcept;
	INLINE ByteSpan& chunk_skip_to_end(ByteSpan& dc) noexcept;

	





	// ByteSpan routines
	INLINE ByteSpan chunk_from_cstr(const char* data) noexcept 
	{ 
		return ByteSpan{ (uint8_t*)data, (uint8_t*)data + strlen(data) }; 
	}


	// inline size_t chunk_size(const ByteSpan& a) noexcept { return a.size(); }
	INLINE bool chunk_empty(const ByteSpan& dc)  noexcept { return dc.fEnd == dc.fStart; }
	INLINE size_t copy(ByteSpan& a, const ByteSpan& b) noexcept
	{
		size_t maxBytes = a.size() < b.size() ? a.size() : b.size();
		memcpy((uint8_t*)a.fStart, b.fStart, maxBytes);
		return maxBytes;
	}

	INLINE int compare(const ByteSpan& a, const ByteSpan& b) noexcept
	{
		size_t maxBytes = a.size() < b.size() ? a.size() : b.size();
		return memcmp(a.fStart, b.fStart, maxBytes);
	}

	INLINE int comparen(const ByteSpan& a, const ByteSpan& b, int n) noexcept
	{
		size_t maxBytes = a.size() < b.size() ? a.size() : b.size();
		if (maxBytes > n)
			maxBytes = n;
		return memcmp(a.fStart, b.fStart, maxBytes);
	}

	INLINE int comparen_cstr(const ByteSpan& a, const char* b, int n) noexcept
	{
		size_t maxBytes = a.size() < n ? a.size() : n;
		
		return memcmp(a.fStart, b, maxBytes);
	}




	INLINE void chunk_truncate(ByteSpan& dc) noexcept
	{
		dc.fEnd = dc.fStart;
	}

	INLINE ByteSpan& chunk_skip(ByteSpan& dc, size_t n) noexcept
	{
		if (n > dc.size())
			n = dc.size();
		dc.fStart += n;

		return dc;
	}

	INLINE ByteSpan& chunk_skip_to_end(ByteSpan& dc) noexcept { dc.fStart = dc.fEnd; return dc; }


}



// Implementation of hash function for ByteSpan
// so it can be used in 'map' collections
/*
namespace std {
	template<>
	struct hash<waavs::ByteSpan> {
		size_t operator()(const waavs::ByteSpan& span) const 
		{
			return waavs::fnv1a_32(span.data(), span.size());
		}
	};
}
*/

namespace waavs {
	struct ByteSpanHash {
		size_t operator()(const ByteSpan& span) const noexcept {
			return waavs::fnv1a_32(span.data(), span.size());
		}
	};

	struct ByteSpanEquivalent {
		bool operator()(const ByteSpan& a, const ByteSpan& b) const noexcept {
			if (a.size() != b.size())
				return false;
			return memcmp(a.fStart, b.fStart, a.size()) == 0;
		}
	};

	// Case insensitive 'string' comparison
	struct ByteSpanInsensitiveHash {
		size_t operator()(const ByteSpan& span) const noexcept {
			return waavs::fnv1a_32_case_insensitive(span.data(), span.size());
		}
	};
	
	struct ByteSpanCaseInsensitive {
		bool operator()(const ByteSpan& a, const ByteSpan& b) const noexcept {
			if (a.size() != b.size())
				return false;

			for (size_t i = 0; i < a.size(); ++i) {
				//if (TOLOWER(a[i]) != TOLOWER(b[i]))  // Case-insensitive comparison
				if (std::tolower(a[i]) != std::tolower(b[i]))  // Case-insensitive comparison

				return false;
			}
			
			return true;
		}
	};
}

// Functions that are implemented here

namespace waavs
{
	INLINE size_t copy_to_cstr(char* str, size_t len, const ByteSpan& a) noexcept;
	INLINE ByteSpan chunk_ltrim(const ByteSpan& a, const charset& skippable) noexcept;
	INLINE ByteSpan chunk_rtrim(const ByteSpan& a, const charset& skippable) noexcept;
	INLINE ByteSpan chunk_trim(const ByteSpan& a, const charset& skippable) noexcept;
	INLINE ByteSpan chunk_skip_wsp(const ByteSpan& a) noexcept;

	INLINE bool chunk_starts_with_char(const ByteSpan& a, const uint8_t b) noexcept;
	INLINE bool chunk_starts_with_cstr(const ByteSpan& a, const char* b) noexcept;

	INLINE bool chunk_ends_with_char(const ByteSpan& a, const uint8_t b) noexcept;
	INLINE bool chunk_ends_with_cstr(const ByteSpan& a, const char* b) noexcept;

	INLINE ByteSpan chunk_token(ByteSpan& a, const charset& delims) noexcept;
	INLINE ByteSpan chunk_find_char(const ByteSpan& a, char c) noexcept;
	INLINE bool chunk_find(const ByteSpan& src, const ByteSpan& str, ByteSpan& value) noexcept;
	INLINE ByteSpan chunk_find_cstr(const ByteSpan& a, const char* c) noexcept;

}






namespace waavs
{
	INLINE size_t copy_to_cstr(char* str, size_t len, const ByteSpan& a) noexcept
	{
		size_t maxBytes = a.size() < len ? a.size() : len;
		memcpy(str, a.fStart, maxBytes);
		str[maxBytes] = 0;

		return maxBytes;
	}

	// Trim the left side of skippable characters
	INLINE ByteSpan chunk_ltrim(const ByteSpan& a, const charset& skippable) noexcept
	{
		const uint8_t* startAt = a.fStart;
		const uint8_t* endAt = a.fEnd;
		while (startAt < endAt && skippable(*startAt))
			++startAt;

		return { startAt, endAt };
	}

	// trim the right side of skippable characters
	INLINE ByteSpan chunk_rtrim(const ByteSpan& a, const charset& skippable) noexcept
	{
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		while (start < end && skippable(*(end - 1)))
			--end;

		return { start, end };
	}

	// trim the left and right side of skippable characters
	INLINE ByteSpan chunk_trim(const ByteSpan& a, const charset& skippable) noexcept
	{
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;

		// trim from the beginning
		while (start < end && skippable(*start))
			++start;

		// trim from the end
		while (start < end && skippable(*(end - 1)))
			--end;

		return { start, end };
	}

	INLINE ByteSpan chunk_skip_wsp(const ByteSpan& a) noexcept
	{
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;

		while (start < end && chrWspChars(*start))
			++start;

		return { start, end };
	}

	INLINE ByteSpan chunk_skip_until_char(const ByteSpan& inChunk, const uint8_t achar) noexcept
	{
		const uint8_t* start = inChunk.fStart;
		const uint8_t* end = inChunk.fEnd;
		while (start < end && *start != achar)
			++start;

		return { start, end };
	}



	INLINE bool chunk_starts_with_char(const ByteSpan& a, const uint8_t b) noexcept
	{
		return (a.size() > 0) && (a.fStart[0] == b);
	}

	INLINE bool chunk_starts_with(const ByteSpan& src, const ByteSpan& str) noexcept
	{
		return src.startsWith(str);
	}

	INLINE bool chunk_starts_with_cstr(const ByteSpan& a, const char* b) noexcept
	{
		// see if the null terminated string is at the beginning of the bytespan
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		while (*b && start < end && *start == *b)
		{
			++start;
			++b;
		}

		return *b == 0;
	}

	INLINE bool chunk_ends_with(const ByteSpan& a, const ByteSpan& b) noexcept
	{
		return a.endsWith(b);
	}

	INLINE bool chunk_ends_with_char(const ByteSpan& a, const uint8_t b) noexcept
	{
		return ((a.size() > 0) && (a.fEnd[-1] == b));
	}

	INLINE bool chunk_ends_with_cstr(const ByteSpan& a, const char* b) noexcept
	{
		return a.endsWith(chunk_from_cstr(b));
	}

	// Given an input chunk
	// spit it into two chunks, 
	// Returns - the first chunk before delimeters
	// a - adjusted to reflect the rest of the input after delims
	// If delimeter NOT found
	// returns the entire input chunk
	// and 'a' is set to an empty chunk
	INLINE ByteSpan chunk_token_char(ByteSpan& a, const char delim) noexcept
	{
		if (!a) {
			a = {};
			return {};
		}

		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		const uint8_t* tokenEnd = start;
		while (tokenEnd < end && *tokenEnd != delim)
			++tokenEnd;

		if (*tokenEnd == delim)
		{
			a.fStart = tokenEnd + 1;
		}
		else {
			a.fStart = tokenEnd;
		}

		return { start, tokenEnd };
	}

	INLINE ByteSpan chunk_token(ByteSpan& a, const charset& delims) noexcept
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
	INLINE ByteSpan nextToken(ByteSpan& a, const charset&& delims) noexcept
	{
		return chunk_token(a, delims);
	}



	// Given an input chunk
	// find the first instance of a specified character
	// return the chunk preceding the found character
	// or or the whole chunk of the character is not found
	INLINE ByteSpan chunk_find_char(const ByteSpan& a, char c) noexcept
	{
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		while (start < end && *start != c)
			++start;

		return { start, end };
	}

	// 
	// chunk_find()
	// 
	// Scan a ByteSpan 'src', looking for the search span 'str'
	// return true if it's found, and set the 'value' ByteSpan 
	// to the location.
	INLINE bool chunk_find(const ByteSpan& src, const ByteSpan& str, ByteSpan& value) noexcept
	{
		const uint8_t* start = src.fStart;
		const uint8_t* end = src.fEnd;
		const uint8_t* b = str.fStart;
		const uint8_t* bEnd = str.fEnd;

		while (start < end)
		{
			// find the first character of the search string in the source string
			while (start < end && *start != *b)
				++start;

			// if we've run out of source string, we didn't find it
			if (start == end)
				return false;

			// now that we've found a potential first character
			// of the search string, we need to see if the rest
			// of the search string is present
			while (start < end && b < bEnd && *start == *b)
			{
				++start;
				++b;
			}

			// if we've run out of search string, we found it
			if (b == bEnd)
			{
				value = { start - str.size(), start };
				return true;
			}
		}

		return false;
	}

	INLINE ByteSpan chunk_find_cstr(const ByteSpan& a, const char* c) noexcept
	{
		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		const uint8_t* cstart = (const uint8_t*)c;
		const uint8_t* cend = cstart + strlen(c);
		ByteSpan cChunk(cstart, cend);

		while (start < end)
		{
			if (*start == *cstart)
			{

				if (chunk_starts_with({ start, end }, cChunk))
				{
					break;
					//return { start, end };
				}
			}

			++start;
		}

		return { start, end };
	}

	INLINE ByteSpan chunk_read_bracketed(ByteSpan& src, const uint8_t lbracket, const uint8_t rbracket) noexcept
	{
		uint8_t* beginattrValue = nullptr;
		uint8_t* endattrValue = nullptr;
		//uint8_t quote{};

		// Skip white space before the quoted bytes
		src = chunk_ltrim(src, chrWspChars);

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

	//
	// chunk_read_quoted()
	// 
	// Read a quoted string from the input stream
	// Read a first quote, then use that as the delimiter
	// to read to the end of the string
	//
	static bool chunk_read_quoted(ByteSpan& src, ByteSpan& dataChunk) noexcept
	{
		uint8_t* beginattrValue = nullptr;
		uint8_t* endattrValue = nullptr;
		uint8_t quote{};

		// Skip white space before the quoted bytes
		src = chunk_ltrim(src, chrWspChars);

		if (!src)
			return false;

		// capture the quote character
		quote = *src;

		// advance past the quote, then look for the matching close quote
		src++;
		beginattrValue = (uint8_t*)src.fStart;

		// Skip until end of the value.
		while (src && *src != quote)
			src++;

		if (src)
		{
			endattrValue = (uint8_t*)src.fStart;
			src++;
		}

		// Store only well formed attributes
		dataChunk = { beginattrValue, endattrValue };

		return true;
	}
}

namespace waavs {

	// readNextKeyValue()
	// 
	// Attributes are separated by '=' and values are quoted with 
	// either "'" or '"'
	// Ex: <tagname attr1='first value'  attr2="second value" />
	// The src so it points past the end of the retrieved value
	static bool readNextKeyAttribute(ByteSpan& src, ByteSpan& key, ByteSpan& value) noexcept
	{
		// Zero these out in case there is failure
		//key = {};
		//value = {};

		static charset equalChars("=");
		static charset quoteChars("\"'");

		bool start = false;
		bool end = false;
		uint8_t quote{};

		uint8_t* beginattrValue = nullptr;
		uint8_t* endattrValue = nullptr;


		// Skip leading white space before the key name
		src = chunk_ltrim(src, chrWspChars);

		if (!src)
			return false;

		// Special case of running into an end tag
		if (*src == '/') {
			end = true;
			return false;
		}

		// Find end of the attrib name.
		//auto attrNameChunk = chunk_token(src, equalChars);
		auto attrNameChunk = chunk_token_char(src, '=');
		key = chunk_trim(attrNameChunk, chrWspChars);

		// Skip stuff past '=' until we see one of our quoteChars
		while (src && !quoteChars[*src])
			src++;

		// If we've run out of input, return false
		if (!src)
			return false;

		// capture the quote character
		quote = *src;

		// move past the beginning of the quote
		// and mark the beginning of the value portion
		src++;
		beginattrValue = (uint8_t*)src.fStart;

		// Skip anything that is not the quote character
		// to mark the end of the value
		// don't use quoteChars here, because it's valid to 
		// embed the other quote within the value
		src = chunk_skip_until_char(src, quote);

		// If we still have input, it means we found
		// the quote character, so mark the end of the
		// value
		if (src)
		{
			endattrValue = (uint8_t*)src.fStart;
			src++;
		}
		else {
			// We did not find the closing quote
			// so we don't have a valid value
			return false;
		}


		// Store only well formed attributes
		value = { beginattrValue, endattrValue };

		return true;
	}

	
	// readNextCSSKeyValue()
	// 
	// Properties are separated by ';'
	// values are separated from the key with ':'
	// Ex: <tagname style="stroke:black;fill:white" />
	// Return
	//   true - if a valid key/value pair was found
	//      in this case, key, and value will be populated
	//   false - if no key/value pair was found, or end of string
	//      in this case, key, and value will be undefined
	//
	static bool readNextCSSKeyValue(ByteSpan& src, ByteSpan& key, ByteSpan& value, const unsigned char fieldDelimeter = ';', const unsigned char keyValueSeparator = ':') noexcept
	{
		// Trim leading whitespace to begin
		src = chunk_ltrim(src, chrWspChars);

		// If the string is now blank, return immediately
		if (!src)
			return false;

		// peel off a key/value pair by taking a token up to the fieldDelimeter
		value = chunk_token_char(src, fieldDelimeter);

		// Now, separate the key from the value using the keyValueSeparator
		key = chunk_token_char(value, keyValueSeparator);

		// trim the key and value fields of whitespace
		key = chunk_trim(key, chrWspChars);
		value = chunk_trim(value, chrWspChars);

		return true;
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

	INLINE void writeChunk(const ByteSpan& chunk) noexcept
	{
		ByteSpan s = chunk;

		while (s && *s) {
			printf("%c", *s);
			s++;
		}
	}

	INLINE void writeChunkBordered(const ByteSpan& chunk) noexcept
	{
		ByteSpan s = chunk;

		printf("||");
		while (s && *s) {
			printf("%c", *s);
			s++;
		}
		printf("||");
	}

	INLINE void printChunk(const ByteSpan& chunk) noexcept
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



