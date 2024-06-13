#pragma once

//
// bstream
// This provides a 'stream' like interface to a chunk of memory.
// The idea is, if you have a piece of memory, and you need to 
// serialize and deserialize things into and out of it, you can 
// use this convenience.
// 
// This pattern is seen all the time when reading binary data from
// files, or network buffers.  It's used for image loading, protocol
// parsing, interop between interfaces, etc.
// 
// The functions here are done as straight C instead of C++ classes
// to provide simple and fast implementation without any dependencies.
//
// The functions here do minimal boundary checking.  They are meant
// to be fairly low level, so they will be safe and not run over
// the ends of buffers, but your values will be invalid if
// you don't first check for the amount of space available before 
// you make your calls.
// 
// This puts the burden of safety checks in the hands of the user,
// so they can determine which course of action to take.
// 
// The routines here are all inline, noexcept, so they should be 
// safe to use in a lot of places without incurring additional cost.
//



#include "definitions.h"
#include "bspan.h"


namespace waavs
{
    struct BStream
    {
        static constexpr int MSEEK_CUR = 1;
        static constexpr int MSEEK_END = 2;
        static constexpr int MSEEK_SET = 0;
        
        ByteSpan fSource{};
        ByteSpan fCursor{};
        bool fStreamIsLE{ true };
        
        BStream() = default;
        BStream(const ByteSpan& inChunk)
            :fSource(inChunk)
        {
            fCursor = fSource;
        }
        BStream(const void* inData, const size_t sz)
            : fSource((const uint8_t *)inData, (const uint8_t *)inData+sz)
        {
            fCursor = fSource;
        }
        
        BStream& operator=(const BStream& other)
        {
			fSource = other.fSource;
			fCursor = other.fCursor;
			fStreamIsLE = other.fStreamIsLE;
			return *this;
        }
        
        // operator *
        // operator[]
        // operator++
        
		const uint8_t* data() const
		{
			return fCursor.fStart;
		}
        
		size_t size() const
		{
			return chunk_size(fSource);
		}

		// Switch the stream to big endian or littleEndian
        void bigEndian(bool streamIsBig) { fStreamIsLE = !streamIsBig; }
		bool bigEndian() const { return !fStreamIsLE; }
        
        size_t seek(ptrdiff_t offset, ptrdiff_t relativeTo = MSEEK_SET)
        {
			switch (relativeTo)
			{
			case MSEEK_CUR:
				fCursor.fStart = fCursor.fStart+offset;
				break;
			case MSEEK_END:
				fCursor.fStart = fSource.fEnd - offset;
				break;
			case MSEEK_SET:
				fCursor.fStart = fSource.fStart + offset;
				break;
			}
			
            // clamp to boundaries
            if (fCursor.fStart < fSource.fStart)
				fCursor.fStart = fSource.fStart;
			if (fCursor.fStart > fSource.fEnd)
				fCursor.fStart = fSource.fEnd;
            
            return fCursor.fStart - fSource.fStart;
		}
        
        size_t tell()
        {
			return fCursor.fStart - fSource.fStart;
        }

		const uint8_t * tellPointer() const
		{
			return fCursor.fStart;
		}
        
		size_t remaining()
		{
			return fSource.fEnd - fCursor.fStart;
		}

        // returns whether we're currently sitting at End Of File
        bool isEOF() const
        {
			return fCursor.fStart >= fSource.fEnd;
        }
        
        bool skip(ptrdiff_t n)  noexcept { fCursor += n; return true; }
        

        // Return a span from the beginning to the end of the stream
        const ByteSpan & span() const noexcept
        {
            return fSource;
        }
        
        // return a subspan using the specified offset and length
		ByteSpan subSpan(size_t offset, size_t sz) const
		{
            // First check if we starting beyond our end
			// If so, return an empty span
            if (offset > size())
				return ByteSpan{};
            
            // Check whether the size asked for fits within the range
            // available from the offset
			// If not, return a span that is as long as possible
			if (offset + sz > size())
				sz = size() - offset;
            
            // It is up to the user to check the size of the span returned, as it might be 
            // smaller than the size asked for
			const unsigned char* start = fSource.fStart + offset;
			const unsigned char* end = start + sz;
			return ByteSpan{ start, end };
		}
        
        // Return a span from the current cursor position, to the size requested
        // do NOT advance the cursor
        // The span can be less than the size requested, 
        // it's up to the caller to check the size returned.
		ByteSpan getSpan(const size_t sz) noexcept
		{
            size_t maxBytes = sz > remaining() ? remaining() : sz;
            ByteSpan result((void*)fCursor.fStart, maxBytes);
            
			return result;
		}
        

        // Copy what was previous in the stream, from our current position
        // moving the cursor forward as we go
        bool copyBack(size_t back_len, size_t back_dist)
        {
            uint8_t* output_next = (uint8_t *)fCursor.fStart;
            while (back_len-- > 0) {
				*fCursor = *(fCursor.fStart - back_dist);
                fCursor++;
                //*output_next = *(output_next - back_dist);
                //output_next++;
            }

            return true;
        }
        
        // Return a span that represents the bytes requested, 
        // up to the amount remaining
        ByteSpan read(size_t sz)
        {
			size_t maxBytes = sz > remaining() ? remaining() : sz;
            ByteSpan result((void*)fCursor.fStart, maxBytes);
            skip(maxBytes);

			return result;
        }
        
        // Copy the data out from the chunk
        // copy up to remaining() bytes
        // return the number of bytes copied
        size_t read_copy(void* outData, size_t sz)
        {
			size_t maxBytes = sz > remaining() ? remaining() : sz;
			memcpy(outData, fCursor.fStart, maxBytes);
            skip(maxBytes);
            
            return maxBytes;
        }
        
        // Reading integer sized values
		// Do range checking here to make sure we don't read past the end
        // The signed integer and float versions are type conversions from
        // the core integer types.
        
		// Read a single byte
        bool read_u8(uint8_t &value)  noexcept 
        {
			if (remaining() < 1)
				return false;
            
            value = fCursor.as_u8();
			skip(1);
			return true;
		}   
        
		bool read_u16(uint16_t& value)  noexcept
		{
			if (remaining() < 2)
				return false;

			if (fStreamIsLE)
				value = fCursor.as_u16_le();
			else
				value = fCursor.as_u16_be();
            
            skip(2);
            
            return true;
		}

        
		bool read_u32(uint32_t& value)  noexcept
		{
			if (remaining() < 4)
				return false;

			if (fStreamIsLE)
				value = fCursor.as_u32_le();
			else
				value = fCursor.as_u32_be();

            skip(4);
            
			return true;
		}

		bool read_u64(uint64_t& value)   noexcept
		{
			if (remaining() < 8)
				return false;

			if (fStreamIsLE)
				value = fCursor.as_u64_le();
			else
				value = fCursor.as_u64_be();
            
            skip(8);
			
            return true;
		}

		// Read le unsigned 16 bit value
         bool read_u16_le(uint16_t &value) noexcept
        {
            if (remaining() < 2)
                return false;
            
            value = fCursor.as_u16_le();
            skip(2);
            
            return true;
        }

        // Read le unsigned 32-bit value
        bool read_u32_le(uint32_t &value) noexcept
        {
            if (remaining() < 4)
                return false;
            
			value = fCursor.as_u32_le();
            skip(4);
            
            return true;
        }

        bool read_i32_le(int32_t& value) noexcept
        {
			return read_u32_le((uint32_t&)value);
        }
        
		// Read le unsigned 64-bit value
        bool read_u64_le(uint64_t &value) noexcept
        {
			if (remaining() < 8)
				return false;
            
			value = fCursor.as_u64_le();
			skip(8);
            return true;
        }

        bool read_f64_le(double& value) noexcept
        {
			return read_u64_le((uint64_t&)value);
        }
        
        //======================================
        // BIG ENDIAN FORMATS
		//======================================

        // Read be unsigned 16 bit value
        bool read_u16_be(uint16_t& value) noexcept
        {
            if (remaining() < 2)
                return false;

            value = fCursor.as_u16_be();
            skip(2);

            return true;
        }

        // Read be unsigned 32-bit value
        bool read_u32_be(uint32_t& value) noexcept
        {
            if (remaining() < 4)
                return false;

            value = fCursor.as_u32_be();
            skip(4);

            return true;
        }

        bool read_i32_be(int32_t& value) noexcept
        {
            
            bool success = read_u32_be((uint32_t&)value);
            return success;
        }
        
        // Read be unsigned 64-bit value
        bool read_u64_be(uint64_t& value) noexcept
        {
            if (remaining() < 8)
                return false;

            value = fCursor.as_u64_be();
            skip(8);
            return true;
        }
        
        bool read_f64_be(double& value) noexcept
        {
            return read_u64_be((uint64_t&)value);
        }

		//======================================
        // Writing to the stream
		//======================================
        bool write_u8(uint8_t a)  noexcept 
        { 
            if (remaining() < 1)
                return false;
            
            *fCursor = a; 
            fCursor++;  
            return true; 
        }

    };
}

/*
namespace ndt
{
    static INLINE uint8_t * tell_pointer(const DataCursor& dc) noexcept;


    static INLINE bool skip_to_alignment(DataCursor& dc, size_t num) noexcept;
    static INLINE bool skip_to_even(DataCursor& dc) noexcept;
    static INLINE const uint8_t* cursorPointer(DataCursor& dc) noexcept;

    static INLINE uint8_t peek_u8(const DataCursor& dc) noexcept;

    // Write in integer values
    static INLINE bool put_u8(DataCursor& dc, uint8_t a) noexcept;
    static INLINE bool put_u16_le(DataCursor& dc, uint16_t a) noexcept;
    static INLINE bool put_u32_le(DataCursor& dc, uint32_t a) noexcept;
    static INLINE bool put_u64_le(DataCursor& dc, uint64_t a) noexcept;
}

namespace ndt
{





    static INLINE ByteSpan make_chunk_toend(const DataCursor& dc) noexcept
    {
		return make_chunk(dc.fCurrent, dc.fEnd);
    }
    
    // some operator overloading
    
    // Retrieve attributes of the cursor

    static INLINE ptrdiff_t remaining(const DataCursor& dc) noexcept { return dc.fEnd - dc.fCurrent; }

    static INLINE uint8_t* tell_pointer(const DataCursor& dc) noexcept { return dc.fCurrent; }


    
    // Seek forwad to a boundary of the specified
    // number of bytes.
    static INLINE bool skip_to_alignment(DataCursor& dc, size_t num)  noexcept { return skip(dc, tell(dc) % num); }

    // Skip to the next even numbered offset
    static INLINE bool skipToEven(DataCursor& dc)  noexcept { return skip_to_alignment(dc, 2); }





    static INLINE bool put_u8(DataCursor& dc, uint8_t a)  noexcept { if (isEOF(dc))return false; *dc.fCurrent = a; dc.fCurrent++;  return true; }
    static INLINE bool put_u16_le(DataCursor& dc, uint16_t a)  noexcept
    {

        if (remaining(dc) < sizeof(a))
            return false;

        if (isLE())
        {
            *(uint16_t*)dc.fCurrent = a;
            skip(dc, 2);
        } else {

            uint8_t* bytes = (uint8_t*)&a;

            for (size_t i = 0; i < sizeof(a); i++)
                *dc.fCurrent++ = *bytes++;
        }

        //put_u8(dc, a & 0xff);
        //put_u8(dc, (a & 0xff00) >> 8);

        return true;
    }
    
    static INLINE bool put_u32_le(DataCursor& dc, uint32_t a)  noexcept
    {
        if (remaining(dc) < sizeof(a))
            return false;

        if (isLE())
        {
            *(uint32_t*)dc.fCurrent = a;
            skip(dc, 4);
        } else {
            uint8_t* bytes = (uint8_t*)&a;

            for (size_t i = 0; i < sizeof(a); i++)
                *dc.fCurrent++ = *bytes++;
        }

        //put_u8(dc, a & 0xff);
        //put_u8(dc, (a & 0xff00) >> 8);
        //put_u8(dc, (a & 0xff0000) >> 16);
        //put_u8(dc, (a & 0xff000000) >> 24);

        return true;
    }
    
    static INLINE bool put_u64_le(DataCursor& dc, uint64_t a)  noexcept
    {
        if (remaining(dc) < sizeof(a))
            return false;

        if (isLE())
        {
            *(uint64_t*)dc.fCurrent = a;
            skip(dc, 8);
        } else {
            uint8_t* bytes = (uint8_t*)&a;

            for (size_t i = 0; i < sizeof(a); i++)
                *dc.fCurrent++ = *bytes++;
        }

        //put_u8(dc, a & 0xff);
        //put_u8(dc, (a & 0xff00) >> 8);
        //put_u8(dc, (a & 0xff0000) >> 16);
       // put_u8(dc, (a & 0xff000000) >> 24);
        //put_u8(dc, (a & 0xff00000000) >> 32);
        //put_u8(dc, (a & 0xff0000000000) >> 40);
        //put_u8(dc, (a & 0xff000000000000) >> 48);
        //put_u8(dc, (a & 0xff00000000000000) >> 56);

        return true;
    }
}

*/
