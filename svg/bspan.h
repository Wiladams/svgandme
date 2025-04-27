#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>  // for std::memchr
#include <iterator>	// for std::data(), std::size()
#include <string.h>

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

		static const ByteSpan& null() noexcept 
		{
			static ByteSpan nullSpan{};
			return nullSpan;
		}

		// Constructors
		constexpr ByteSpan() = default;
		constexpr ByteSpan(const unsigned char* start, const unsigned char* end) noexcept : fStart(start), fEnd(end) {}
		ByteSpan(const char* cstr) noexcept
			: fStart(reinterpret_cast<const uint8_t*>(cstr)),
			fEnd(reinterpret_cast<const uint8_t*>(cstr) + std::strlen(cstr)) {
		}
		explicit constexpr ByteSpan(const void* data, size_t sz) noexcept
			: fStart(static_cast<const uint8_t*>(data)), fEnd(fStart + sz) {
		}

		~ByteSpan() = default;

		constexpr void reset() { fStart = nullptr; fEnd = nullptr; }
		
		// setting up for a range-based for loop
		constexpr const uint8_t* data() const noexcept { return (unsigned char*)fStart; }
		constexpr const uint8_t* begin() const noexcept { return fStart; }
		constexpr const uint8_t* end() const noexcept { return fEnd; }

		constexpr size_t size() const noexcept { return (fEnd >= fStart) ? static_cast<size_t>(fEnd - fStart) : 0; }
		constexpr bool empty() const noexcept { return fStart == fEnd; }


		// Type conversions
		explicit constexpr operator bool() const noexcept { return (fEnd - fStart) > 0; };


		// Array access
		uint8_t& operator[](size_t i) noexcept { return const_cast<uint8_t&>(fStart[i]); }
		const uint8_t& operator[](size_t i) const noexcept { return fStart[i]; }


		// get current value from fStart, like a 'peek' operation
		// If the ByteSpan is currently empty, these will return 0, rather than 
		// throwing an exception
		uint8_t operator*() const noexcept {
			return (fStart < fEnd) ? *fStart : 0;
		}


		//
		// operators for comparison
		// operator==;
		// operator!=;
		// operator<=;
		// operator>=;
		
		// isEqual()
		// A pointer comparison
		bool isEqual(const ByteSpan& b) const noexcept
		{
			if (fStart == b.fStart && fEnd == b.fEnd)
				return true;

			return false;
		}

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
			return (size() == b.size()) && (std::memcmp(fStart, b.fStart, size()) == 0);
		}

		bool operator==(const char* b) const noexcept
		{
			return (b && std::strncmp(reinterpret_cast<const char*>(fStart), b, size()) == 0);
		}


		bool operator!=(const ByteSpan& other) const noexcept
		{
			return !(*this == other);
		}

		bool operator<(const ByteSpan& b) const noexcept
		{
			size_t minSize = size() < b.size() ? size() : b.size();
			int cmp = memcmp(fStart, b.fStart, minSize);
			return (cmp < 0) || (cmp == 0 && size() < b.size());
		}


		bool operator>(const ByteSpan& b) const noexcept
		{
			size_t minSize = size() < b.size() ? size() : b.size();
			int cmp = memcmp(fStart, b.fStart, minSize);
			return (cmp > 0) || (cmp == 0 && size() > b.size());
		}


		bool operator<=(const ByteSpan& b) const noexcept
		{
			size_t minSize = size() < b.size() ? size() : b.size();
			int cmp = memcmp(fStart, b.fStart, minSize);
			return (cmp < 0) || (cmp == 0 && size() <= b.size());
		}


		bool operator>=(const ByteSpan& b) const noexcept
		{
			size_t minSize = size() < b.size() ? size() : b.size();
			int cmp = memcmp(fStart, b.fStart, minSize);
			return (cmp > 0) || (cmp == 0 && size() >= b.size());
		}

		// advance the start pointer the specified number of entries
		// constrain to end 
		constexpr ByteSpan& remove_prefix(size_t n) noexcept
		{
			fStart = (fStart + n <= fEnd) ? fStart + n : fEnd; 
			return *this;
		}

		constexpr ByteSpan& operator+=(size_t n) noexcept {fStart = (fStart + n <= fEnd) ? fStart + n : fEnd; return *this;}

		constexpr ByteSpan& operator++() noexcept { return (*this += 1); }
		 ByteSpan operator++(int) noexcept { ByteSpan temp = *this; ++(*this); return temp; } 


		void setAll(uint8_t c) noexcept { std::memset(const_cast<uint8_t*>(fStart), c, size()); }
		void copyFrom(const void* src, size_t sz) noexcept {
			if (sz > 0) std::memcpy(const_cast<uint8_t*>(fStart), src, sz);
		}

		
		// subSpan()
		// Create a bytespan that is a subspan of the current span
		// If the requested position plus size is greater than the amount
		// of span remaining at that position, the size will be truncated 
		// to the amount remaining from the requested position.
		constexpr ByteSpan subSpan(size_t startAt, size_t sz) const noexcept
		{
			const uint8_t* newStart = (startAt < size()) ? fStart + startAt : fEnd;
			const uint8_t* newEnd = (newStart + sz <= fEnd) ? newStart + sz : fEnd;
			return { newStart, newEnd };
		}


		ByteSpan take(size_t n) const noexcept
		{
			return subSpan(0, n);
		}

		// Some convenient routines
		ByteSpan &prefix_trim(const charset& skippable) noexcept
		{
			while (fStart < fEnd && skippable(*fStart))
				++fStart;

			return *this;
		}

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

	// Some utility functions for common operations
	INLINE ByteSpan& chunk_skip(ByteSpan& dc, size_t n) noexcept;
	INLINE ByteSpan& chunk_skip_to_end(ByteSpan& dc) noexcept;

	





	// ByteSpan routines

	// inline size_t chunk_size(const ByteSpan& a) noexcept { return a.size(); }
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

	INLINE ByteSpan chunk_skip_until_cstr(const ByteSpan& inChunk, const char* str) noexcept
	{
		const uint8_t* start = inChunk.fStart;
		const uint8_t* end = inChunk.fEnd;
		size_t len = std::strlen(str);

		while (start < end - len + 1)  // Ensure we don't read past the buffer
		{
			if (std::memcmp(start, str, len) == 0)
				return { start, end };  // Return position at match

			++start;
		}

		return { end, end };  // No match found, return empty
	}

	INLINE ByteSpan chunk_skip_until_chunk(const ByteSpan& inChunk, const ByteSpan& match) noexcept
	{
		const uint8_t* start = inChunk.fStart;
		const uint8_t* end = inChunk.fEnd;
		size_t len = match.size();

		if (len == 0 || start == end)
			return { end, end };  // Empty input or match, return empty

		while (start < end - len + 1)
		{
			if (std::memcmp(start, match.fStart, len) == 0)
				return { start, end };  // Return position at match

			++start;
		}

		return { end, end };  // No match found, return empty
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
		return a.endsWith(ByteSpan(b));
	}

	// chuk_token_char()
	// 
	// Given a source chunk chunk, and a single character delimeter
	// split the source into two chunks, the token, and the rest
	// The 'token', represents the portion of the source before the delim
	// The 'rest', represents the portion of the source after the delim
	// 
	// Returns - false if the delimeter is not found

	INLINE bool chunk_token_char(const ByteSpan &src, const uint8_t delim, ByteSpan &tok, ByteSpan &rest)
	{
		const uint8_t* startAt = src.begin();
		const uint8_t* endAt = src.end();

		// Start with token being entire source input
		tok = src;

		// Try to find the delimeter
		const uint8_t* tokenEnd = static_cast<const uint8_t*>(std::memchr(startAt, delim, endAt - startAt));

		if (!tokenEnd) {
			// No delimiter found, return entire input
			//tok.fStart = src.begin();
			//tok.fEnd = src.end();

			// mark the 'rest' as being blank
			rest.fStart = endAt;
			rest.fEnd = endAt;
			return false;
		}
		else {
			// truncate the token to match where the 
			// delimeter was found
			tok.fEnd = tokenEnd;

			rest.fStart = tokenEnd + 1;  // Advance past the delimiter
			rest.fEnd = endAt;
		}

		return true;
	}

	// Given an input chunk
	// split it into two chunks, 
	// Returns - the first chunk before delimeter
	//	a - adjusted to reflect the rest of the input after delims
	// If delimeter NOT found
	//	returns the entire input chunk
	//	and 'a' is set to an empty chunk
	INLINE ByteSpan chunk_token_char(ByteSpan& a, const char delim) noexcept
	{
		if (!a) return {}; // Return empty ByteSpan if 'a' is empty

		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;

		// Use std::memchr() to find the delimiter efficiently
		const uint8_t* tokenEnd = static_cast<const uint8_t*>(std::memchr(start, delim, end - start));

		if (tokenEnd) {
			a.fStart = tokenEnd + 1;  // Advance past the delimiter
		}
		else {
			tokenEnd = end;  // No delimiter found, return entire input
			a.fStart = end;  // Move 'a' to empty state
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
	// Return 
	//	If character found, return chunk beginning where the character was found
	//  If character not found, return empty chunk
	INLINE ByteSpan chunk_find_char(const ByteSpan& a, char c) noexcept
	{
		if (!a) return {}; // Return empty chunk if input is empty

		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;

		// Use std::memchr() to locate the first occurrence of 'c'
		const uint8_t* found = static_cast<const uint8_t*>(std::memchr(start, c, end - start));

		if (!found) return {}; // Character not found, return empty chunk

		return { found, end };
	}


	// 
	// chunk_find()
	// 
	// Scan a ByteSpan 'src', looking for the search span 'str'
	// return true if it's found, and set the 'value' ByteSpan 
	// to the location.
	INLINE bool chunk_find(const ByteSpan& src, const ByteSpan& str, ByteSpan& value) noexcept
	{
		if (src.size() < str.size() || str.empty())
			return false;

		const uint8_t* srcStart = src.fStart;
		const uint8_t* srcEnd = src.fEnd - (str.size() - 1); // Avoid over-scanning
		const uint8_t* pattern = str.fStart;
		const size_t patternSize = str.size();

		while (srcStart < srcEnd)
		{
			// Fast forward to the first occurrence of the first character of `str`
			srcStart = static_cast<const uint8_t*>(std::memchr(srcStart, *pattern, srcEnd - srcStart));
			if (!srcStart) return false; // Not found

			// Check if the rest of the `str` matches
			if (std::memcmp(srcStart, pattern, patternSize) == 0)
			{
				value = { srcStart, srcStart + patternSize };
				return true;
			}

			++srcStart; // Move to the next position
		}

		return false;
	}


	INLINE ByteSpan chunk_find_cstr(const ByteSpan& a, const char* c) noexcept
	{
		if (!a || !c || *c == '\0')
			return {};  // Return empty ByteSpan if input is invalid

		const uint8_t* start = a.fStart;
		const uint8_t* end = a.fEnd;
		const size_t clen = std::strlen(c);

		if (clen > static_cast<size_t>(end - start))
			return {};  // If search string is larger than input, it can't be found

		const uint8_t firstChar = static_cast<uint8_t>(c[0]);

		while (start < end)
		{
			// Use memchr() to quickly find the first character
			start = static_cast<const uint8_t*>(std::memchr(start, firstChar, end - start));
			if (!start || start + clen > end)
				return {};  // If first char is not found or rest of string doesn't fit, return empty

			// Check the rest of the string using memcmp()
			if (std::memcmp(start, c, clen) == 0)
				return { start, end };

			++start;  // Move to next potential match
		}

		return {};
	}




	INLINE ByteSpan chunk_read_bracketed(ByteSpan& src, const uint8_t lbracket, const uint8_t rbracket) noexcept
	{
		// Skip leading whitespace
		src = chunk_ltrim(src, chrWspChars);

		if (!src || *src != lbracket)
			return {};  // No valid opening bracket found

		// Advance past the opening bracket
		src++;
		const uint8_t* beginAttrValue = src.fStart;

		// Use std::memchr() to find the closing rbracket
		const uint8_t* endAttrValue = static_cast<const uint8_t*>(std::memchr(beginAttrValue, rbracket, src.fEnd - beginAttrValue));

		if (!endAttrValue)
			return {};  // No closing bracket found

		// Advance `src` past the closing bracket
		src.fStart = endAttrValue + 1;

		return { beginAttrValue, endAttrValue };
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
		// Skip leading whitespace
		src = chunk_ltrim(src, chrWspChars);

		if (!src)
			return false;

		// Capture the opening quote character
		const uint8_t quote = *src;

		// Move past the opening quote
		src++;
		const uint8_t* beginAttrValue = src.fStart;

		// Use std::memchr() to find the matching closing quote
		const uint8_t* endAttrValue = static_cast<const uint8_t*>(std::memchr(beginAttrValue, quote, src.fEnd - beginAttrValue));

		if (!endAttrValue)
			return false;  // No closing quote found

		// Assign the extracted span
		dataChunk = { beginAttrValue, endAttrValue };

		// Advance `src` past the closing quote
		src.fStart = endAttrValue + 1;

		return true;
	}

}

namespace waavs {

	// Efficiently reads the next key-value attribute pair from `src`
	// Attributes are separated by '=' and values are enclosed in '"' or '\''
	static bool readNextKeyAttribute(ByteSpan& src, ByteSpan& key, ByteSpan& value) noexcept
	{
		key.reset();
		value.reset();

		static charset quoteChars("\"'");

		// Trim leading whitespace
		src = chunk_ltrim(src, chrWspChars);

		if (!src)
			return false;

		// Handle end tag scenario (e.g., `/>`)
		if (*src == '/')
			return false;

		// Extract attribute name (before '=')
		ByteSpan attrNameChunk = chunk_token_char(src, '=');
		key = chunk_trim(attrNameChunk, chrWspChars);

		// If no '=' found, return false
		if (!src)
			return false;

		// Skip past '=' and any whitespace
		src = chunk_ltrim(src, chrWspChars);

		if (!src)
			return false;

		// Ensure we have a quoted value
		uint8_t quote = *src;
		if (!quoteChars(quote))
			return false;

		// Move past the opening quote
		src++;

		// Locate the closing quote using `memchr`
		const uint8_t* endQuote = static_cast<const uint8_t*>(std::memchr(src.fStart, quote, src.size()));

		if (!endQuote)
			return false; // No closing quote found

		// Assign the attribute value (excluding quotes)
		value = { src.fStart, endQuote };

		// Move past the closing quote
		src.fStart = endQuote + 1;

		return true;
	}

	// Searches `inChunk` for an attribute `key` and returns its value if found
	static bool getKeyValue(const ByteSpan& inChunk, const ByteSpan& key, ByteSpan& value) noexcept
	{
		ByteSpan src = inChunk;
		ByteSpan name{};
		bool insideQuotes = false;
		uint8_t quoteChar = 0;

		while (src)
		{
			// Skip leading whitespace
			src = chunk_ltrim(src, chrWspChars);

			if (!src)
				return false;

			// If we hit a quote, skip the entire quoted section
			if (*src == '"' || *src == '\'')
			{
				quoteChar = *src;
				src++; // Move past opening quote

				// Use `chunk_find_char` to efficiently skip to closing quote
				src = chunk_find_char(src, quoteChar);
				if (!src)
					return false;

				src++; // Move past closing quote
				continue;
			}

			// Extract the next token as a potential key
			ByteSpan keyCandidate = chunk_token_char(src, '=');
			keyCandidate = chunk_trim(keyCandidate, chrWspChars);

			// If this matches the requested key, extract the value
			if (keyCandidate == key)
			{
				// Skip whitespace before value
				src = chunk_ltrim(src, chrWspChars);

				if (!src)
					return false;

				// **Only accept quoted values**
				if (*src == '"' || *src == '\'')
				{
					quoteChar = *src;
					src++;
					value.fStart = src.fStart;

					// Find the closing quote **quickly**
					src = chunk_find_char(src, quoteChar);
					if (!src)
						return false;

					value.fEnd = src.fStart;  // Exclude the closing quote
					src++;
					return true; // Successfully found key and value
				}

				// **Reject unquoted values in XML**
				return false;
			}

			// If there was no `=`, continue scanning
			src = chunk_ltrim(src, chrWspChars);
			if (src && *src == '=')
				src++; // Skip past '=' and continue parsing
		}

		return false; // Key not found
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



