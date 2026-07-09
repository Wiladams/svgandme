#pragma once

#ifndef BASE64_H_INCLUDED
#define BASE64_H_INCLUDED


#include <cstdint>

//
// Base 64 encoding and decoding
// The couple of routines in this file will encode and decode base64 strings.
// The encoding is done according to RFC 4648.
// The decoding is done according to RFC 4648 and RFC 2045.
//
// The decode routine is tolerant of whitespace, but will stop when
// it sees invalid characters.  It will also stop when it
// sees the first padding character ('=').
//
// The routines here may not be the fastest vectorized versions, but they are simple
// and easily portable.  There was some effort put into making the decode
// routine fast, by trying to decode 4 characters at a time when possible, and
// using a characterization table to quickly identify valid characters and their values.




namespace waavs {

    enum class DecodeStatus : uint32_t
    {
        DECODE_STATUS_Success = 0,
        DECODE_STATUS_InvalidByte,
        DECODE_STATUS_InvalidPadding,
        DECODE_STATUS_IncompleteQuartet,
        DECODE_STATUS_OutputTooSmall
    };


    static constexpr uint8_t BASE64_INVALID = 255;
    static constexpr uint8_t WSP = 254;
    static constexpr uint8_t PAD = 253;
    
    // BASE 64 encode table
    // According to RFC 4648
    alignas(256) inline static constexpr char base64en[64] = {
 //   0    1    2    3    4    5    6    7
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
 //   8    9   10   11   12   13   14   15
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
 //  16   17   18   19   20   21   22   23
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
 //  24   25   26   27   28   29   30   31
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
 //  32   33   34   35   36   37   38   39
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
 //  40   41   42   43   44   45   46   47
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
 //  48   49   50   51   52   53   54   55
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
 //  56   57   58   59   60   61   62   63
    '4', '5', '6', '7', '8', '9', '+', '/'
    };

    
    // ASCII order for BASE 64 decode,
    // 255 is unused and invalid character
    inline static constexpr unsigned char base64de[256] =
    {
    // nul, soh, stx, etx, eot, enq, ack, bel,
       255, 255, 255, 255, 255, 255, 255, 255,
    //  bs,  ht,  nl,  vt,  np,  cr,  so,  si,
       255, WSP, WSP, 255, 255, WSP, 255, 255,
    // dle, dc1, dc2, dc3, dc4, nak, syn, etb,
       255, 255, 255, 255, 255, 255, 255, 255,
    // can,  em, sub, esc,  fs,  gs,  rs,  us,
       255, 255, 255, 255, 255, 255, 255, 255,
    //  sp, '!', '"', '#', '$', '%', '&', ''',
       WSP, 255, 255, 255, 255, 255, 255, 255,
    // '(', ')', '*', '+', ',', '-', '.', '/',
       255, 255, 255,  62, 255, 255, 255,  63,
    // '0', '1', '2', '3', '4', '5', '6', '7',
        52,  53,  54,  55,  56,  57,  58,  59,
    // '8', '9', ':', ';', '<', '=', '>', '?',
        60,  61, 255, 255, 255, PAD, 255, 255,
    // '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
       255,   0,   1,   2,   3,   4,   5,   6,
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
        49,  50,  51, 255, 255, 255, 255, 255,
    // 0x80–0x87
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0x88–0x8F
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0x90–0x97
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0x98–0x9F
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0xA0–0xA7
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0xA8–0xAF
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0xB0–0xB7
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0xB8–0xBF
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0xC0–0xC7
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0xC8–0xCF
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0xD0–0xD7
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0xD8–0xDF
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0xE0–0xE7
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0xE8–0xEF
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0xF0–0xF7
       255, 255, 255, 255, 255, 255, 255, 255,
    // 0xF8–0xFF
       255, 255, 255, 255, 255, 255, 255, 255
    };



    struct base64 {
        //static constexpr unsigned char BASE64DE_FIRST = '+';
        //static constexpr unsigned char BASE64DE_LAST = 'z';
        static constexpr unsigned char BASE64_PAD_CHAR = '=';

        // Given an input buffer size, getDecodeOutputSize() returns the size
        // of buffer needed to contain the decoded data.
        static constexpr size_t getDecodeOutputSize(size_t inputSize) noexcept
        {
            return (inputSize / 4) * 3;
        }

        static constexpr size_t getEncodeOutputSize(size_t inputSize) noexcept
        {
            return ((inputSize+2) / 3) * 4 + 1;
        }

        // base64_encode
        // out is null-terminated encode string.
        // return values is out length, excluding the terminating `\0'
        static size_t encode(const unsigned char* in, size_t inlen, char* out) noexcept 
        {
            size_t i = 0, j = 0;
            while (i + 2 < inlen) {
                out[j++] = base64en[(in[i] >> 2) & 0x3F];
                out[j++] = base64en[((in[i] & 0x03) << 4) | (in[i + 1] >> 4)];
                out[j++] = base64en[((in[i + 1] & 0x0F) << 2) | (in[i + 2] >> 6)];
                out[j++] = base64en[in[i + 2] & 0x3F];
                i += 3;
            }

            // handle remaining 1 or 2 bytes
            if (i < inlen) {
                out[j++] = base64en[(in[i] >> 2) & 0x3F];
                if (i + 1 < inlen) {
                    out[j++] = base64en[((in[i] & 0x03) << 4) | (in[i + 1] >> 4)];
                    out[j++] = base64en[(in[i + 1] & 0x0F) << 2];
                }
                else {
                    out[j++] = base64en[(in[i] & 0x03) << 4];
                    out[j++] = BASE64_PAD_CHAR;
                }
                out[j++] = BASE64_PAD_CHAR;
            }

            out[j] = '\0';
            return j;
        }


        // Decode a Base64 buffer.
        // 'in'     - pointer to input Base64-encoded data
        // 'inlen'  - length of the input buffer
        // 'out'    - destination for decoded bytes
        // 'maxOut' - maximum bytes that can be written to 'out'
        // Returns the number of bytes actually written to 'out'.
        //
        // Output buffer must be at least ((inlen / 4) * 3) bytes.
 
        static DecodeStatus decode(
            const uint8_t* in,
            size_t inlen,
            uint8_t* out,
            size_t outCap,
            size_t& bytesRead,
            size_t& bytesWritten) noexcept
        {
            bytesRead = 0;
            bytesWritten = 0;

            if ((in == nullptr && inlen != 0) || (out == nullptr && outCap != 0))
                return DecodeStatus::DECODE_STATUS_InvalidByte;

            const uint8_t* src = in;
            const uint8_t* end = in + inlen;
            uint8_t* dst = out;
            uint8_t* dstEnd = out + outCap;

            uint8_t q[4];
            uint8_t qCount = 0;
            bool sawPadQuartet = false;

            while (src < end) {
                const uint8_t c = *src;
                const uint8_t cls = base64de[c];

                if (cls == WSP) {
                    ++src;
                    continue;
                }

                if (sawPadQuartet) {
                    bytesRead = (size_t)(src - in);
                    bytesWritten = (size_t)(dst - out);
                    return DecodeStatus::DECODE_STATUS_InvalidPadding;
                }

                if (cls == BASE64_INVALID) {
                    bytesRead = (size_t)(src - in);
                    bytesWritten = (size_t)(dst - out);
                    return DecodeStatus::DECODE_STATUS_InvalidByte;
                }

                q[qCount++] = cls;
                ++src;

                if (qCount != 4)
                    continue;

                if (q[0] <= 63 && q[1] <= 63 && q[2] <= 63 && q[3] <= 63) {
                    if ((size_t)(dstEnd - dst) < 3) {
                        bytesRead = (size_t)(src - in);
                        bytesWritten = (size_t)(dst - out);
                        return DecodeStatus::DECODE_STATUS_OutputTooSmall;
                    }

                    const uint32_t v =
                        (uint32_t(q[0]) << 18) |
                        (uint32_t(q[1]) << 12) |
                        (uint32_t(q[2]) << 6) |
                        uint32_t(q[3]);

                    dst[0] = uint8_t(v >> 16);
                    dst[1] = uint8_t(v >> 8);
                    dst[2] = uint8_t(v);

                    dst += 3;
                    qCount = 0;
                    continue;
                }

                if (q[0] > 63 || q[1] > 63) {
                    bytesRead = (size_t)(src - in);
                    bytesWritten = (size_t)(dst - out);
                    return DecodeStatus::DECODE_STATUS_InvalidPadding;
                }

                if (q[2] == PAD && q[3] == PAD) {
                    if ((size_t)(dstEnd - dst) < 1) {
                        bytesRead = (size_t)(src - in);
                        bytesWritten = (size_t)(dst - out);
                        return DecodeStatus::DECODE_STATUS_OutputTooSmall;
                    }

                    const uint32_t v =
                        (uint32_t(q[0]) << 18) |
                        (uint32_t(q[1]) << 12);

                    dst[0] = uint8_t(v >> 16);
                    dst += 1;
                    qCount = 0;
                    sawPadQuartet = true;
                    continue;
                }

                if (q[2] <= 63 && q[3] == PAD) {
                    if ((size_t)(dstEnd - dst) < 2) {
                        bytesRead = (size_t)(src - in);
                        bytesWritten = (size_t)(dst - out);
                        return DecodeStatus::DECODE_STATUS_OutputTooSmall;
                    }

                    const uint32_t v =
                        (uint32_t(q[0]) << 18) |
                        (uint32_t(q[1]) << 12) |
                        (uint32_t(q[2]) << 6);

                    dst[0] = uint8_t(v >> 16);
                    dst[1] = uint8_t(v >> 8);
                    dst += 2;
                    qCount = 0;
                    sawPadQuartet = true;
                    continue;
                }

                bytesRead = (size_t)(src - in);
                bytesWritten = (size_t)(dst - out);
                return DecodeStatus::DECODE_STATUS_InvalidPadding;
            }

            if (qCount == 0) {
                bytesRead = inlen;
                bytesWritten = (size_t)(dst - out);
                return DecodeStatus::DECODE_STATUS_Success;
            }

            if (qCount == 1) {
                bytesRead = inlen;
                bytesWritten = (size_t)(dst - out);
                return DecodeStatus::DECODE_STATUS_IncompleteQuartet;
            }

            if (q[0] > 63 || q[1] > 63) {
                bytesRead = inlen;
                bytesWritten = (size_t)(dst - out);
                return DecodeStatus::DECODE_STATUS_InvalidPadding;
            }

            if (qCount == 2) {
                if ((size_t)(dstEnd - dst) < 1) {
                    bytesRead = inlen;
                    bytesWritten = (size_t)(dst - out);
                    return DecodeStatus::DECODE_STATUS_OutputTooSmall;
                }

                const uint32_t v =
                    (uint32_t(q[0]) << 18) |
                    (uint32_t(q[1]) << 12);

                dst[0] = uint8_t(v >> 16);
                dst += 1;

                bytesRead = inlen;
                bytesWritten = (size_t)(dst - out);
                return DecodeStatus::DECODE_STATUS_Success;
            }

            if (q[2] > 63) {
                bytesRead = inlen;
                bytesWritten = (size_t)(dst - out);
                return DecodeStatus::DECODE_STATUS_InvalidPadding;
            }

            if ((size_t)(dstEnd - dst) < 2) {
                bytesRead = inlen;
                bytesWritten = (size_t)(dst - out);
                return DecodeStatus::DECODE_STATUS_OutputTooSmall;
            }

            {
                const uint32_t v =
                    (uint32_t(q[0]) << 18) |
                    (uint32_t(q[1]) << 12) |
                    (uint32_t(q[2]) << 6);

                dst[0] = uint8_t(v >> 16);
                dst[1] = uint8_t(v >> 8);
                dst += 2;
            }

            bytesRead = inlen;
            bytesWritten = (size_t)(dst - out);
            return DecodeStatus::DECODE_STATUS_Success;
        }

        //static DecodeStatus decode(
        //    const uint8_t* in,
        //    size_t inlen,
        //    uint8_t* out,
        //    size_t outCap,
        //    size_t& bytesWritten) noexcept
        //{
        //    size_t bytesRead = 0;
        //    return decode(in, inlen, out, outCap, bytesRead, bytesWritten);
        //}

        static size_t decode(const unsigned char* in, size_t inlen, unsigned char* out, size_t maxOut) noexcept
        {
            size_t bytesRead = 0;
            size_t bytesWritten = 0;
            DecodeStatus status = decode(in, inlen, out, maxOut, bytesRead, bytesWritten);
            return (status == DecodeStatus::DECODE_STATUS_Success) ? bytesWritten : 0;
        }

        /*
        static size_t decode(const unsigned char* in, size_t inlen, unsigned char* out, size_t maxOut) noexcept {
            size_t i = 0, j = 0;
            uint32_t acc = 0;
            int count = 0;

            while (i < inlen) {
                unsigned char c = in[i++];
                unsigned char val = base64de[c];  // Safe because base64de[256] is fully populated

                if (val != 255) {
                    acc = (acc << 6) | val;
                    ++count;

                    if (count == 4) {
                        if (j + 3 > maxOut) break;  // Prevent overrun
                        out[j++] = (acc >> 16) & 0xFF;
                        out[j++] = (acc >> 8) & 0xFF;
                        out[j++] = acc & 0xFF;
                        acc = 0;
                        count = 0;
                    }
                }
            }

            // Handle padding cases
            if (count == 3 && j + 2 <= maxOut) {
                out[j++] = (acc >> 10) & 0xFF;
                out[j++] = (acc >> 2) & 0xFF;
            }
            else if (count == 2 && j + 1 <= maxOut) {
                out[j++] = (acc >> 4) & 0xFF;
            }

            return j;
        }
        */
    };

}


#endif // BASE64_H