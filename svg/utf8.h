#pragma once




#include <cstdint>

#include "bspan.h"


namespace waavs
{
	//
	// Inspiration of this code: http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
	// With original copyright: 
	// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
	// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
	// 
	// Has been heavily modified to fit the needs of this project.
	//
 
	// *** DO NOT CHANGE THESE VALUES ***
	// They are used in the DFA table
	static constexpr int UTF8_ACCEPT = 0;
	static constexpr int UTF8_REJECT = 1;

	static const uint8_t utf8d[] = {
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
	  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
	  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
	  8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
	  0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
	  0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
	  0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
	  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
	  1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
	  1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
	  1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
	};

	// take a single byte and determine the next state of the decoder
	// returns the next state, or UTF8_REJECT if the byte is invalid
	// RETURN values:
	// 0: accept
	// 1: reject
	// 2..9: next state
	static uint32_t INLINE decode(uint32_t* state, uint32_t* codep, uint32_t byte) noexcept
	{
		uint32_t type = utf8d[byte];

		*codep = (*state != UTF8_ACCEPT) ?
			(byte & 0x3fu) | (*codep << 6) :
			(0xff >> type) & (byte);

		*state = utf8d[256 + *state * 16 + type];

		return *state;
	}


	// Utf8Iterator
	// 
	// Given a source span, emit codepoint values while decoding UTF-8.
	// Use like an iterator, ++ to advance * to get the current value
	//
	struct Utf8Iterator
	{
		ByteSpan fSource;
		uint32_t fState = UTF8_ACCEPT;
		uint32_t fCodepoint = 0;

		Utf8Iterator(const ByteSpan &source) noexcept : fSource(source) { next(); }

		explicit operator bool() const noexcept
		{
			return (bool)fSource;
		}

		// Decode the next codepoint from the source chunk.
		Utf8Iterator& next() noexcept
		{
			while (fSource)
			{
				uint32_t newState = decode(&fState, &fCodepoint, *fSource);
				if (newState == UTF8_ACCEPT)
					return *this;

				// Stop decoding by truncating the source.
				if (newState == UTF8_REJECT)
					chunk_truncate(fSource);

			}

			return *this;
		}

		Utf8Iterator& operator++() noexcept { return next(); }
		Utf8Iterator& operator++(int) noexcept { return next(); }

		uint32_t operator *() const noexcept { return fCodepoint; }
	};



	//----------------------------------------------------------
	// convertUTF32ToUTF8
	// 
	// Converting from Unicode codepoint to utf-8 octet sequence
	// The output buffer should be at least 4 bytes long
	//==========================================================
	static bool convertUTF32ToUTF8(uint64_t input, char* output, size_t & length) noexcept
	{
		const unsigned long BYTE_MASK = 0xBF;
		const unsigned long BYTE_MARK = 0x80;
		const unsigned long FIRST_BYTE_MARK[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

		if (input < 0x80) {
			length = 1;
		}
		else if (input < 0x800) {
			length = 2;
		}
		else if (input < 0x10000) {
			length = 3;
		}
		else if (input < 0x200000) {
			length = 4;
		}
		else {
			length = 0;    // This code won't convert this correctly anyway.
			return false;
		}

		output += length;

		// Scary scary fall throughs are annotated with carefully designed comments
		// to suppress compiler warnings such as -Wimplicit-fallthrough in gcc
		switch (length) {
		case 4:
			--output;
			*output = static_cast<char>((input | BYTE_MARK) & BYTE_MASK);
			input >>= 6;
			//fall through
		case 3:
			--output;
			*output = static_cast<char>((input | BYTE_MARK) & BYTE_MASK);
			input >>= 6;
			//fall through
		case 2:
			--output;
			*output = static_cast<char>((input | BYTE_MARK) & BYTE_MASK);
			input >>= 6;
			//fall through
		case 1:
			--output;
			*output = static_cast<char>(input | FIRST_BYTE_MARK[length]);
			break;
		default:
			return false;
		}

		return true;
	}
	
}
