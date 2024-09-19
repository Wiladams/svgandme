#pragma once

#include "bspan.h"

namespace waavs {
	//
	// MemBuff
	// 
	// This is a very simple data structure that allocates a chunk of memory
	// When the destructor is called, the memory is freed.
	// This could easily be handled by something like a unique_ptr, but I don't
	// want to force the usage of std library when it's not really needed.
	// besides, it's so easy and convenient and small.
	// Note:  This could be a sub-class of ByteSpan, but the semantics are different
	// With a ByteSpan, you can alter the start/end pointers, but with a memBuff, you can't.
	// so, it is much easier to return a ByteSpan, and let that be manipulated instead.
	// 

	struct MemBuff final
	{
	private:
		uint8_t* fData{ nullptr };
		size_t fSize{ 0 };

	public:
		// Use default constructor
		constexpr MemBuff() noexcept = default;

		// Copy constructor (make a deep copy)
		MemBuff(const MemBuff& other) noexcept
		{
			fSize = other.fSize;
			if (fSize > 0)
			{
				fData = new uint8_t[fSize];
				memcpy(fData, other.fData, fSize);
			}
		}
		
		// Move Constructor (take over ownership of data)
		MemBuff(MemBuff&& other) noexcept
		{
			fData = other.fData;
			fSize = other.fSize;
		
			// Leave 'other' in a valid, but empty state
			other.fData = nullptr;
			other.fSize = 0;
		}
		
		// Construct with a known size
		MemBuff(const size_t sz) noexcept
		{
			fSize = sz;
			fData = new uint8_t[sz];
		}

		// Construct from a ByteSpan
		MemBuff(const ByteSpan& chunk) noexcept
		{
			fSize = chunk.size();
			fData = new uint8_t[fSize];
			memcpy(fData, chunk.data(), fSize);
		}





		~MemBuff() noexcept
		{
			if (fData != nullptr)
				delete[] fData;
		}


		MemBuff& reset() {
			if (fData != nullptr)
				delete[] fData;
			fData = nullptr;
			fSize = 0;
			
			return *this;
		}
		
		// Operators
		
		// Copy Assignment operator (deep copy)
		MemBuff& operator=(const MemBuff& other) noexcept
		{
			// short circuit on self assignment
			if (this == &other)
				return *this;
			

			reset();

			fSize = other.fSize;
			if (fSize > 0)
			{
				fData = new uint8_t[fSize];
				memcpy(fData, other.fData, fSize);
			}
			
			return *this;
		}
		
		// Move Assignment operator (take over ownership of data)
		MemBuff& operator=(MemBuff&& other) noexcept
		{
			// short circuit on self assignment
			if (this == &other)
				return *this;

			reset();

			fData = other.fData;
			fSize = other.fSize;

			// Leave 'other' in a valid, but empty state
			other.fData = nullptr;
			other.fSize = 0;

			printf("MemBuff - END\n");
			
			return *this;
		}
		

		// Conveniences
		constexpr uint8_t* begin() const noexcept { return fData; }
		constexpr uint8_t* end() const noexcept { return fData + fSize; }
		
		constexpr uint8_t* data() const noexcept { return fData; }
		constexpr size_t size() const noexcept { return fSize; }


		// initFromSpan
		// copy the data from the input span into the memory buffer
		//

		bool initFromSpan(const ByteSpan& srcSpan) noexcept
		{
			reset();

			fSize = srcSpan.size();

			if (fSize > 0) {
				fData = new uint8_t[fSize];
				memcpy(fData, srcSpan.fStart, fSize);
			}

			return true;
		}


		// span()
		// 
		// Create a ByteSpan from the memory buffer.
		// This is pure convenience, as a ByteSpan can easily be created
		// from the data() and size() functions.
		// The lifetime of the ByteSpan that is returned it not governed
		// by the MemBuff object.  This is something the caller must manage.
		ByteSpan span() const noexcept { return ByteSpan(begin(), end()); }
	};
}
