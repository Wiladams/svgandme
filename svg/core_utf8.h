#pragma once

#include <cstdint>
#include <string>

#include "lang_span.h"


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
					(fSource.advanceToEnd());

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

namespace waavs
{
	// This is a little gem AI generated
	// At some point it might be useful to compare
    // this routine to the table based one in decode() above, 
	// to see if it's worth using this for fast skipping of codepoints.
	static INLINE const uint8_t* utf8_next(const uint8_t* p, const uint8_t* e) noexcept
	{
		if (p >= e) 
			return e;

		uint8_t c = *p++;
		if (c < 0x80) return p;

		// assumes valid UTF-8; fast skip only
		if ((c >> 5) == 0x6)  return (p + 1 <= e) ? (p + 1) : e; // 2 bytes total
		if ((c >> 4) == 0xE)  return (p + 2 <= e) ? (p + 2) : e; // 3 bytes total
		if ((c >> 3) == 0x1E) return (p + 3 <= e) ? (p + 3) : e; // 4 bytes total
		return e;
	}
}

// core_utf8.h or core_utf16.h – add after includes



namespace waavs {

	// Convert a UTF-16BE encoded byte span to a UTF-8 string.
	// Returns an empty string if the input is empty.
	// If the input length is odd, the last byte is ignored.
	// Invalid sequences (lone surrogates, truncated pairs) are replaced with U+FFFD.
	// This function may throw std::bad_alloc if memory allocation fails.
	inline std::string convertUtf16BeToUtf8(const ByteSpan& span) {
		if (span.empty()) return {};

		const uint8_t* p = span.begin();
		const uint8_t* end = span.end();

		// Ignore trailing odd byte
		if ((span.size() & 1) != 0) {
			--end;
		}

		std::string result;
		result.reserve(span.size() / 2); // rough lower bound, may grow

		auto readU16 = [&](const uint8_t*& ptr) -> uint32_t {
			uint32_t high = ptr[0];
			uint32_t low = ptr[1];
			ptr += 2;
			return (high << 8) | low;
			};

		while (p < end) {
			uint32_t u16 = readU16(p);
			uint32_t codepoint = 0;

			if (u16 >= 0xD800 && u16 <= 0xDBFF) {
				// High surrogate: need a low surrogate
				if (p + 2 <= end) {
					uint32_t low = readU16(p);
					if (low >= 0xDC00 && low <= 0xDFFF) {
						codepoint = 0x10000 + ((u16 - 0xD800) << 10) + (low - 0xDC00);
					}
					else {
						// Invalid low surrogate -> replacement char
						codepoint = 0xFFFD;
					}
				}
				else {
					// Truncated surrogate pair -> replacement char
					codepoint = 0xFFFD;
					break; // no more data
				}
			}
			else if (u16 >= 0xDC00 && u16 <= 0xDFFF) {
				// Lone low surrogate -> replacement char
				codepoint = 0xFFFD;
			}
			else {
				// Basic Multilingual Plane (BMP) codepoint
				codepoint = u16;
			}

			// Encode codepoint to UTF-8
			char buffer[4];
			size_t len = 0;

			if (codepoint < 0x80) {
				buffer[0] = static_cast<char>(codepoint);
				len = 1;
			}
			else if (codepoint < 0x800) {
				buffer[0] = static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
				buffer[1] = static_cast<char>(0x80 | (codepoint & 0x3F));
				len = 2;
			}
			else if (codepoint < 0x10000) {
				buffer[0] = static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
				buffer[1] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
				buffer[2] = static_cast<char>(0x80 | (codepoint & 0x3F));
				len = 3;
			}
			else if (codepoint <= 0x10FFFF) {
				buffer[0] = static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
				buffer[1] = static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
				buffer[2] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
				buffer[3] = static_cast<char>(0x80 | (codepoint & 0x3F));
				len = 4;
			}
			else {
				// Out of Unicode range -> replacement char
				buffer[0] = static_cast<char>(0xEF);
				buffer[1] = static_cast<char>(0xBF);
				buffer[2] = static_cast<char>(0xBD);
				len = 3;
			}

			result.append(buffer, len);
		}

		return result;
	}

	/*
	            // ========================================================================
            // UTF-8 Support
            // ========================================================================
						static inline bool isValidUTF8(const ByteSpan& data) noexcept {
				uint32_t state = UTF8_ACCEPT;
				uint32_t cp = 0;

				for (const uint8_t* p = data.begin(); p != data.end(); ++p) {
					uint32_t newState = decode(&state, &cp, *p);
					if (newState == UTF8_REJECT) {
						return false;
					}
				}

				return state == UTF8_ACCEPT;
			}

            inline bool readUTF8String(uint8_t delim, ByteSpan& out) noexcept {
                const uint8_t* start = mData.begin();
                const uint8_t* end = mData.end();

                const uint8_t* found = static_cast<const uint8_t*>(
                    std::memchr(start, delim, end - start));

                if (!found) {
                    out = mData;
                    mData.advanceToEnd();
                }
                else {
                    out = ByteSpan::fromPointers(start, found);
                    mData.resetStart(found + 1);
                }

                return isValidUTF8(out);
            }

            inline bool readUTF8String(size_t length, ByteSpan& out) noexcept {
                if (mData.size() < length) return false;

                out = mData.subSpan(0, length);
                mData.advance(length);

                return isValidUTF8(out);
            }
	*/
} // namespace waavs


