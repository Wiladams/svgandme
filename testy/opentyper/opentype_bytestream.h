// opentype_bytestream.h - Fixed with seek()
#pragma once

#include "opentype_types.h"
#include "lang_span.h"
#include "lang_memory.h"
#include "core_bytestream.h"

namespace waavs {
    namespace opentype {

        struct OpenTypeByteStream : public BigEndianByteStream
        {

        public:
            OpenTypeByteStream() = default;

            explicit OpenTypeByteStream(const ByteSpan& data) noexcept
                : BigEndianByteStream(data) { }

            explicit OpenTypeByteStream(const MemBuff& buffer) noexcept
                : BigEndianByteStream(ByteSpan(buffer.data(), buffer.size()))
            {
            }

            explicit OpenTypeByteStream(const SharedMemBuff& buffer) noexcept
                : BigEndianByteStream(ByteSpan(buffer.data(), buffer.size()))
            {
            }


            // ========================================================================
            // OpenType Specific Types
            // ========================================================================

            inline bool readFixed(double& out) noexcept 
            {
                int32_t fixed;
                if (!readInt32(fixed)) 
                    return false;
                
                out = static_cast<double>(fixed) / 65536.0;
                
                return true;
            }

            inline bool readF2Dot14(double& out) noexcept 
            {
                int16_t fixed;
                if (!readInt16(fixed)) 
                    return false;
                out = static_cast<double>(fixed) / 16384.0;
                
                return true;
            }

            inline bool readOffset16(uint16_t& out) noexcept 
            {
                return readUInt16(out);
            }

            inline bool readOffset32(uint32_t& out) noexcept 
            {
                return readUInt32(out);
            }

            // ========================================================================
            // Convenience Methods
            // ========================================================================

            // Create a substream beginning at 'offset' and extending
            // to the end of this stream.
            //
            // 'offset' is relative to this stream's base.
            inline OpenTypeByteStream subStream(size_t offset) const noexcept
            {
                return OpenTypeByteStream(slice(offset));
            }

            inline OpenTypeByteStream subStream( size_t offset, size_t length) const noexcept
            {
                return OpenTypeByteStream(slice(offset, length));
            }



            // ========================================================================
            // Validation
            // ========================================================================
            /*
            static inline bool isSupportedFontContainer(const ByteSpan& data) noexcept 
            {
                if (data.size() < 4) 
                    return false;  // Only need 4 bytes for signature check

                uint32_t signature = OSIG(data.begin());

                // Check for all valid OpenType signatures
                //signature == 0x00020000 || // TrueType with TrueType outlines (variant)
                return signature == 0x00010000 || // TrueType with TrueType outlines (MOST COMMON)
                    signature == OTAG("true") || // "true" (Apple TrueType)
                    signature == OTAG("typ1") || // "typ1" (Apple Type 1, rare)
                    signature == OTAG("OTTO") || // "OTTO" (OpenType with CFF)
                    signature == OTAG("ttcf");   // "ttcf" (TrueType Collection)
            }
            */

            static inline uint32_t calcChecksum(const ByteSpan& data) noexcept {
                uint32_t sum = 0;
                const uint8_t* p = data.begin();
                size_t i = 0;

                for (; i + 3 < data.size(); i += 4) {
                    uint32_t word = (static_cast<uint32_t>(p[i]) << 24) |
                        (static_cast<uint32_t>(p[i + 1]) << 16) |
                        (static_cast<uint32_t>(p[i + 2]) << 8) |
                        static_cast<uint32_t>(p[i + 3]);
                    sum += word;
                }

                uint32_t remaining = 0;
                switch (data.size() - i) {
                case 3: remaining |= static_cast<uint32_t>(p[i + 2]) << 8; [[fallthrough]];
                case 2: remaining |= static_cast<uint32_t>(p[i + 1]) << 16; [[fallthrough]];
                case 1: remaining |= static_cast<uint32_t>(p[i]) << 24; break;
                default: break;
                }
                sum += remaining;

                return sum;
            }

        };

    } // namespace opentype
} // namespace waavs