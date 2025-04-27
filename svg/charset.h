#pragma once

#include <bitset>
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
	struct charset;

	static INLINE bool charset_contains(const charset& aset, unsigned char c) noexcept;
	static INLINE charset& charset_add_char(charset& aset, unsigned char c) noexcept;
	static INLINE charset& charset_add_chars(charset& aset, const char* chars) noexcept;
	static INLINE charset& charset_add_charset(charset& aset, const charset& bset) noexcept;

	static INLINE charset& charset_remove_char(charset& aset, unsigned char c) noexcept;
	static INLINE charset& charset_remove_chars(charset& aset, const char* chars) noexcept;
	static INLINE charset& charset_remove_charset(charset& aset, const charset& bset) noexcept;

	static INLINE charset charset_inverse(const charset& aset) noexcept;


	// charset
	// Data structure to hold character set and provide
	// convenient C++ implementation for working with them
	struct charset {
		std::bitset<256> bits{};

		// Common Constructors
		constexpr charset() noexcept = default;
		explicit charset(const char achar) noexcept { charset_add_char(*this, achar); }
		charset(const char* chars) noexcept { charset_add_chars(*this, chars); }
		charset(const charset& aset) noexcept { charset_add_charset(*this,aset); }


		/*
		// Methods for removing characters from the set
		charset& removeChar(const char achar) noexcept
		{
			bits.reset(achar);
			return *this;
		}
		
		charset& removeChars(const char* chars) noexcept
		{
			const char* s = chars;
			while (0 != *s)
				bits.reset(*s++);
			return *this;
		}
		
		charset& removeCharset(const charset& aset) noexcept
		{
			bits &= ~aset.bits;
			return *this;
		}
		*/

		// Convenience for adding characters and strings
		charset& operator+=(const char achar) noexcept { charset_add_char(*this, achar); return *this; }
		charset& operator+=(const char* chars) noexcept { charset_add_chars(*this, chars); return *this; }
		charset& operator+=(const charset& aset) noexcept { charset_add_charset(*this, aset); return *this; }
		
		// Convenience for removing characters and ranges from a set
		charset& operator-=(const char achar) noexcept { charset_remove_char(*this, achar); return *this; }
		charset& operator-=(const char* chars) noexcept { charset_remove_chars(*this, chars); return *this; }
		charset& operator-=(const charset& aset) noexcept { charset_remove_charset(*this, aset); return *this; }

		// Creating a new set
		charset operator+(const char achar) const noexcept { charset result(*this); return charset_add_char(result, achar); }
		charset operator+(const char* chars) const noexcept { charset result(*this); return charset_add_chars(result, chars); }
		charset operator+(const charset& aset) const noexcept { charset result(*this); return charset_add_charset(result, aset); }
		
		charset operator-(const char achar) const noexcept { charset result(*this); charset_remove_char(result, achar); return result; }
		charset operator-(const char* chars) const noexcept { charset result(*this); charset_remove_chars(result, chars); return result; }
		charset operator-(const charset& aset) const noexcept { charset result(*this); charset_remove_charset(result, aset);  return result; }



		// get an inverse of the set
		charset operator~() const noexcept
		{
			charset result;
			result.bits = ~bits;
			return result;
		}
		
		charset inverse() const noexcept
		{
			return ~(*this);
		}
		
		// Checking for set membership
		bool contains(const uint8_t idx) const noexcept { return bits[idx];}

		// This one makes it look like an array
		constexpr bool operator [](const size_t idx) const noexcept { return bits[idx]; }

		// This one makes it look like a function
		constexpr bool operator ()(const size_t idx) const noexcept {return bits[idx];}
		
		// operator^ is the union of two sets
		charset operator^(const charset& other) const noexcept { charset result(*this); result.bits |= other.bits; return result; }
		charset& operator^=(const charset& other) noexcept { bits |= other.bits; return *this; }
		
	};
	
	// Some utility routines for working with character sets
	static INLINE bool charset_contains(const charset& aset, unsigned char c) noexcept { return aset.bits[c]; }
	static INLINE charset& charset_add_char(charset& aset, unsigned char c) noexcept { aset.bits.set(c);return aset; }
	static INLINE charset& charset_add_chars(charset& aset, const char* chars) noexcept
	{
		const char* s = chars;
		while (0 != *s)
			aset.bits.set(*s++);

		return aset;
	}
	static INLINE charset& charset_add_charset(charset& aset, const charset& bset) noexcept
	{
		aset.bits |= bset.bits;
		return aset;
	}

	static INLINE charset& charset_remove_char(charset& aset, unsigned char c) noexcept
	{
		aset.bits.reset(c);
		return aset;
	}

	static INLINE charset& charset_remove_chars(charset& aset, const char* chars) noexcept
	{
		const char* s = chars;
		while (0 != *s)
			aset.bits.reset(*s++);
		return aset;
	}

	static INLINE charset& charset_remove_charset(charset& aset, const charset& bset) noexcept
	{
		aset.bits &= ~bset.bits;
		return aset;
	}


	static INLINE charset charset_inverse(charset& aset) noexcept
	{
		charset result;
		result.bits = ~aset.bits;
		return result;
	}

	// Some common character sets
	static charset chrWspChars("\t\r\n\f\v ");          // whitespace characters
	static charset chrAlphaChars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
	static charset chrDecDigits("0123456789");
	static charset chrHexDigits("0123456789ABCDEFabcdef");

	// The classic character ccategorizers (isdigit(), isalpha(), etc.) from ctype.h
	// are replicated here.  They are implemented as constexpr functions, so they can
	// be used in other constexpr functions.
	static constexpr bool is_digit(const unsigned char c) noexcept { return ((c >= '0') && (c <= '9')); }
	static constexpr bool is_xdigit(const unsigned char c) noexcept { return (is_digit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')); }
	static constexpr bool is_alpha(const unsigned char c) noexcept { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
	static constexpr bool is_alnum(const unsigned char c) noexcept { return is_alpha(c) || is_digit(c); }
	static constexpr bool is_space(const unsigned char c) noexcept { return c == 0x20 || c == 0x09 || c == 0x0A || c == 0x0D; }
	static constexpr bool is_upper(const unsigned char c) noexcept { return (c >= 'A' && c <= 'Z'); }
	static constexpr bool is_lower(const unsigned char c) noexcept { return (c >= 'a' && c <= 'z'); }
	static constexpr bool is_print(const unsigned char c) noexcept { return (c >= 0x20 && c <= 0x7E); }
	static constexpr bool is_punct(const unsigned char c) noexcept { return (c >= 0x21 && c <= 0x2F) || (c >= 0x3A && c <= 0x40) || (c >= 0x5B && c <= 0x60) || (c >= 0x7B && c <= 0x7E); }
	static constexpr bool is_cntrl(const unsigned char c) noexcept { return (c < 0x20) || (c == 0x7F); }
	static constexpr bool is_graph(const unsigned char c) noexcept { return (c >= 0x21 && c <= 0x7E); }

	static constexpr unsigned char to_lower(const unsigned char c) noexcept {
		return (c >= 'A' && c <= 'Z') ? (c | 32) : c;
	}

	static constexpr unsigned char to_upper(const unsigned char c) noexcept {
		return (c >= 'a' && c <= 'z') ? (c & ~32) : c;
	}

}