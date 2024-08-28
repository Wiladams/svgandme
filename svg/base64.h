#pragma once

#ifndef BASE64_H
#define BASE64_H

//
// Base 64 encoding and decoding
// The couple of routines in this file will encode and decode base64 strings.
// The encoding is done according to RFC 4648.
// The decoding is done according to RFC 4648 and RFC 2045.
//
// The decode routine is tolerant of whitespace and other non-base64 characters.
// it will just ignore them.
// The routines here may not be the fastest vectorized versions, but they are simple
// and easily portable.

//#define BASE64_ENCODE_OUT_SIZE(s) ((unsigned int)((((s) + 2) / 3) * 4 + 1))
//#define BASE64_DECODE_OUT_SIZE(s) ((unsigned int)(((s) / 4) * 3))



namespace waavs {
	constexpr auto BASE64_PAD = '=';
	constexpr auto BASE64DE_FIRST = '+';
	constexpr auto BASE64DE_LAST = 'z';
	
	
	// BASE 64 encode table
	// According to RFC 4648
	static const char base64en[] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
		'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
		'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
		'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
		'w', 'x', 'y', 'z', '0', '1', '2', '3',
		'4', '5', '6', '7', '8', '9', '+', '/',
	};
	
	// ASCII order for BASE 64 decode, 255 is unused and invalid character
	static const unsigned char base64de[] = {
	 // nul, soh, stx, etx, eot, enq, ack, bel,
		255, 255, 255, 255, 255, 255, 255, 255,
	 //  bs,  ht,  nl,  vt,  np,  cr,  so,  si,
		255, 255, 255, 255, 255, 255, 255, 255,
	 // dle, dc1, dc2, dc3, dc4, nak, syn, etb,
		255, 255, 255, 255, 255, 255, 255, 255,
	 // can,  em, sub, esc,  fs,  gs,  rs,  us,
	    255, 255, 255, 255, 255, 255, 255, 255,
	 //  sp, '!', '"', '#', '$', '%', '&', ''',
		255, 255, 255, 255, 255, 255, 255, 255,
	 // '(', ')', '*', '+', ',', '-', '.', '/',
		255, 255, 255,  62, 255, 255, 255,  63,
	 // '0', '1', '2', '3', '4', '5', '6', '7',
	     52,  53,  54,  55,  56,  57,  58,  59,
	 // '8', '9', ':', ';', '<', '=', '>', '?',
	     60,  61, 255, 255, 255, 255, 255, 255,
	 // '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
		255,   0,   1,  2,   3,   4,   5,    6,
	 // 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	      7,   8,   9,  10,  11,  12,  13,  14,
	 // 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	     15,  16,  17,  18,  19,  20,  21,  22,
	 // 'X', 'Y', 'Z', '[', '\', ']', '^', '_',
	     23,  24,  25, 255, 255, 255, 255, 255,
	 // '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	    255,  26,  27,  28,  29,  30,  31,  32,
	 // 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	     33,  34,  35,  36,  37,  38,  39,  40,
	 // 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	     41,  42,  43,  44,  45,  46,  47,  48,
	 // 'x', 'y', 'z', '{', '|', '}', '~', del,
	     49,  50,  51, 255, 255, 255, 255, 255
	};
	

	
	struct base64 {
		// Given an input buffer size, getDecodeOutputSize() returns the size
		// of buffer needed to contain the decoded data.
		static unsigned int getDecodeOutputSize(const size_t inputSize) noexcept
		{
			return ((unsigned int)(((inputSize) / 4) * 3));
		}
		
		static unsigned int getEncodeOutputSize(const size_t inputSize) noexcept
		{
			return ((unsigned int)((((inputSize)+2) / 3) * 4 + 1));
		}
		
		// base64_encode
		// out is null-terminated encode string.
		// return values is out length, excluding the terminating `\0'
		static unsigned int encode(const unsigned char* in, unsigned int inlen, char* out) noexcept
		{
			int s;
			unsigned int i;
			unsigned int j;
			unsigned char c;
			unsigned char l;

			s = 0;
			l = 0;
			for (i = j = 0; i < inlen; i++) {
				c = in[i];


				switch (s) {
				case 0:
					s = 1;
					out[j++] = base64en[(c >> 2) & 0x3F];
					break;
				case 1:
					s = 2;
					out[j++] = base64en[((l & 0x3) << 4) | ((c >> 4) & 0xF)];
					break;
				case 2:
					s = 0;
					out[j++] = base64en[((l & 0xF) << 2) | ((c >> 6) & 0x3)];
					out[j++] = base64en[c & 0x3F];
					break;
				}
				l = c;
			}

			switch (s) {
			case 1:
				out[j++] = base64en[(l & 0x3) << 4];
				out[j++] = BASE64_PAD;
				out[j++] = BASE64_PAD;
				break;
			case 2:
				out[j++] = base64en[(l & 0xF) << 2];
				out[j++] = BASE64_PAD;
				break;
			}

			out[j] = 0;

			return j;
		}
		
		// decode
		// Given an input that is base64 encoded, decode it to the output buffer.
		// the return value of the function is the number of bytes that
		// are in the 'out' buffer
		// skip over whitespace, ignore invalid characters
		static size_t decode(const char* in, size_t inlen, unsigned char* out) noexcept
		{
			size_t i{ 0 };	// i - tracks the input location
			unsigned int j;
			unsigned char c;


			i = 0;
			j = 0;
			while (i < inlen) {
				c = base64de[(unsigned char)in[i++]];
				if (c == 255) {
					continue;
				}
				out[j] = c << 2;
				c = base64de[(unsigned char)in[i++]];
				if (c == 255) {
					continue;
				}
				out[j] |= c >> 4;
				if (i < inlen) {
					out[j + 1] = c << 4;
					c = base64de[(unsigned char)in[i++]];
					if (c == 255) {
						continue;
					}
					out[j + 1] |= c >> 2;
				}
				if (i < inlen) {
					out[j + 2] = c << 6;
					c = base64de[(unsigned char)in[i++]];
					if (c == 255) {
						continue;
					}
					out[j + 2] |= c;
				}
				j += 3;
			}


			return j;
		}
	};
}


#endif // BASE64_H