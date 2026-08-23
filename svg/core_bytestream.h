#pragma once

#include "lang_span.h"

namespace waavs {
 
    struct ByteStream
    {
    protected:
        ByteSpan mData;
        size_t mOffset{ 0 };    // The current offset within the stream
    
    public:
        ByteStream() = default;

        explicit ByteStream(const ByteSpan &data) noexcept
            : mData(data)
        {
        }

        //virtual ~ByteStream() = default;

        void reset(const ByteSpan& data) noexcept
        {
            mData = data;
            mOffset = 0;
        }

        inline bool isEOF() const noexcept 
        {
            return mOffset >= mData.size();
        }
        inline bool isValid() const noexcept { return mData.begin()!=nullptr; }
        inline size_t size() const noexcept { return mData.size(); }

        inline size_t tell() const noexcept { return mOffset; }
        inline bool empty() const noexcept { return remaining() == 0; } 

        inline bool seek(size_t offset) noexcept
        {
            if (offset > mData.size()) 
                return false;
            
            mOffset = offset;
            return true;
        }

        inline void rewind() noexcept { mOffset = 0; }

        // Core byte handling methods
        inline size_t remaining() const noexcept { return mData.size() - mOffset; }

        inline ByteSpan remainingData() const noexcept
        {
            return mData.subSpan(mOffset);
        }

        inline bool validRange(size_t offset, size_t length) const noexcept
        {
            return offset <= mData.size() &&
                length <= mData.size() - offset;
        }

        inline bool skip(size_t count) noexcept
        {
            if (count > remaining())
                return false;

            mOffset += count;
            return true;
        }

        inline bool readBytes(size_t count, ByteSpan& out) noexcept
        {
            if (count > remaining())
                return false;

            out = mData.subSpan(mOffset, count);
            mOffset += count;

            return true;
        }

        inline bool peekBytes(size_t count, ByteSpan& out) const noexcept
        {
            if (count > remaining())
                return false;

            out = mData.subSpan(mOffset, count);
            return true;
        }

        inline bool peek(uint8_t& out) const noexcept
        {
            if (empty())
                return false;

            out = mData[mOffset];
            return true;
        }

        inline bool peek(size_t relativeOffset, uint8_t& out) const noexcept
        {
            if (relativeOffset >= remaining())
                return false;

            out = mData[mOffset + relativeOffset];
            return true;
        }


        // align
        // Align the current offset to the specified alignment 
        // (must be a power of 2)
        inline bool align(size_t alignment) noexcept
        {
            if (alignment == 0)
                return false;

            const size_t aligned =
                (mOffset + alignment - 1) / alignment * alignment;

            return seek(aligned);
        }


        inline ByteSpan slice(size_t offset) const noexcept
        {
            if (offset > mData.size())
                return {};

            return mData.subSpan(offset);
        }

        inline ByteSpan slice( size_t offset, size_t length) const noexcept
        {
            if (!validRange(offset, length))
                return {};

            return mData.subSpan(offset, length);
        }

    };


    struct BigEndianByteStream : public ByteStream
    {
        using ByteStream::ByteStream;

        inline bool readUInt8(uint8_t& out) noexcept
        {
            if (remaining() < 1)
                return false;

            out = mData[mOffset];
            mOffset += 1;

            return true;
        }

        inline bool readUInt16(uint16_t& out) noexcept
        {
            if (remaining() < 2)
                return false;

            const uint8_t* p = mData.begin() + mOffset;

            out =
                (static_cast<uint16_t>(p[0]) << 8) |
                static_cast<uint16_t>(p[1]);

            mOffset += 2;

            return true;
        }

        inline bool readInt16(int16_t& out) noexcept
        {
            uint16_t v;

            if (!readUInt16(v))
                return false;

            out = static_cast<int16_t>(v);

            return true;
        }

        inline bool readUInt32(uint32_t& out) noexcept
        {
            if (remaining() < 4)
                return false;

            const uint8_t* p = mData.begin() + mOffset;

            out =
                (static_cast<uint32_t>(p[0]) << 24) |
                (static_cast<uint32_t>(p[1]) << 16) |
                (static_cast<uint32_t>(p[2]) << 8) |
                static_cast<uint32_t>(p[3]);

            mOffset += 4;

            return true;
        }

        inline bool readInt32(int32_t& out) noexcept
        {
            uint32_t v;

            if (!readUInt32(v))
                return false;

            out = static_cast<int32_t>(v);

            return true;
        }

        inline bool readUInt64(uint64_t& out) noexcept
        {
            if (remaining() < 8)
                return false;

            const uint8_t* p = mData.begin() + mOffset;

            out =
                (static_cast<uint64_t>(p[0]) << 56) |
                (static_cast<uint64_t>(p[1]) << 48) |
                (static_cast<uint64_t>(p[2]) << 40) |
                (static_cast<uint64_t>(p[3]) << 32) |
                (static_cast<uint64_t>(p[4]) << 24) |
                (static_cast<uint64_t>(p[5]) << 16) |
                (static_cast<uint64_t>(p[6]) << 8) |
                static_cast<uint64_t>(p[7]);

            mOffset += 8;

            return true;
        }
    };
}