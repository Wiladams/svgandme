#pragma once

#include "definitions.h"

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
	struct charset {
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

		charset& add(const char achar) noexcept
		{
			bits[static_cast<unsigned char>(achar)] = 1;

			return *this;
		}

		charset& add(const char* chars) noexcept
		{
			const char* s = chars;
			while (0 != *s) {
				bits[static_cast<unsigned char>(*s)] = 1;
				++s;
			}

			return *this;
		}

		charset& add(const charset& aset) noexcept
		{
			for (size_t i = 0; i < 256; ++i)
				bits[i] |= aset.bits[i];

			return *this;
		}

		charset& remove(const char achar) noexcept
		{
			bits[static_cast<unsigned char>(achar)] = 0;
			return *this;
		}
		charset& remove(const char* chars) noexcept
		{
			const char* s = chars;
			while (0 != *s) {
				bits[static_cast<unsigned char>(*s)] = 0;
				++s;
			}
			return *this;
		}
		charset& remove(const charset& aset) noexcept
		{
			for (size_t i = 0; i < 256; ++i)
				bits[i] &= ~aset.bits[i];
			return *this;
		}

		// Convenience for adding characters and strings
		charset& operator+=(const char achar) noexcept { this->add(achar); return *this; }
		charset& operator+=(const char* chars) noexcept { this->add(chars); return *this; }
		charset& operator+=(const charset& aset) noexcept { this->add(aset); return *this; }

		// Convenience for removing characters and ranges from a set
		charset& operator-=(const char achar) noexcept { bits[achar]=0; return *this; }
		charset& operator-=(const char* chars) noexcept 
		{ 
			return this->remove(chars);
		}
		charset& operator-=(const charset& aset) noexcept 
		{ 
			return this->remove(aset);
		}


		// get an inverse of the set
		charset& operator~() noexcept
		{
			return invert();
		}

		charset& invert() noexcept
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
		constexpr bool operator [](const size_t idx) const noexcept { return bits[idx] != 0; }

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

	};



}



namespace waavs {
	// The classic character ccategorizers (isdigit(), isalpha(), etc.) from ctype.h
	// are replicated here.  They are implemented as constexpr functions, so they can
	// be used in other constexpr functions.
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



	/*
	struct Whitespace
	{
		static unsigned char test(unsigned char ch) noexcept
		{
			static const unsigned char data[256] =
			{
				// 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
				0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  1,  0,  0,  // 0
				0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 1
				1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 2
				0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 3
				0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 4
				0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 5
				0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 6
				0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 7
				0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 8
				0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // 9
				0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // A
				0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // B
				0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // C
				0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // D
				0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // E
				0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0   // F
			};
			return data[static_cast<unsigned char>(ch)];
		}

		constexpr bool operator()(unsigned char c) const noexcept {
			return test(c);
		}
	};
	

	struct IsDigit {
		constexpr bool operator()(unsigned char c) const noexcept {
			return waavs::is_digit(c);
		}
	};

	struct IsXDigit {
		constexpr bool operator()(unsigned char c) const noexcept {
			return waavs::is_xdigit(c);
		}
	};
	*/
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
	static constexpr charset chrWspChars("\t\r\n\f\v ");          // whitespace characters
	static constexpr charset chrAlphaChars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
	static constexpr charset chrDecDigits("0123456789");
	static constexpr charset chrHexDigits("0123456789ABCDEFabcdef");
}

