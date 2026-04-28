// membuff.h

#pragma once

#include "bspan.h"
#include <atomic>

namespace waavs
{
    INLINE void memset_u32(void* dstvoid, uint32_t pixel, size_t count) noexcept
    {
        if (count == 0)
            return;

        uint32_t* dst32 = static_cast<uint32_t*>(dstvoid);

        // If all bytes of the uint32_t are the same, 
        // we can use memset for a faster fill
        const uint8_t b = uint8_t(pixel & 0xFF);
        if (pixel == (uint32_t(b) * 0x01010101u)) {
            std::memset(dst32, b, count * sizeof(uint32_t));
            return;
        }

#if WAAS_HAS_NEON
        size_t i = 0;
        uint32x4_t v = vdupq_n_u32(pixel);

        for (; i + 16 <= count; i += 16) {
            vst1q_u32(dst32 + i + 0, v);
            vst1q_u32(dst32 + i + 4, v);
            vst1q_u32(dst32 + i + 8, v);
            vst1q_u32(dst32 + i + 12, v);
        }
        for (; i + 4 <= count; i += 4) {
            vst1q_u32(dst32 + i, v);
        }
        for (; i < count; ++i) {
            dst32[i] = pixel;
        }
#else
        // Fallback to scalar fill if there's any leftover
        // or if SIMD is not available
        for (size_t i = 0; i < count; ++i)
            dst32[i] = pixel;
#endif
    }
}



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
            resetFromSize(sz);
        }

        // Construct from a ByteSpan
        MemBuff(const ByteSpan& chunk) noexcept
        {
            resetFromSpan(chunk);
        }





        ~MemBuff() noexcept
        {
            if (fData != nullptr)
                delete[] fData;
        }


        void reset() 
        {
            if (fData != nullptr)
                delete[] fData;
            fData = nullptr;
            fSize = 0;
        }
        
        bool resetFromSize(const size_t sz) noexcept
        {
            reset();
            fSize = sz;
            if (fSize > 0) {
                fData = new uint8_t[fSize];
            }
            return true;
        }

        // resetFromSpan
        // 
        // copy the data from the input span into the memory buffer
        //
        bool resetFromSpan(const ByteSpan& srcSpan) noexcept
        {
            reset();

            fSize = srcSpan.size();

            if (fSize > 0) {
                fData = new (std::nothrow) uint8_t[fSize];
                if (!fData) {
                    fSize = 0;
                    return false; // allocation failed
                }
                memcpy(fData, srcSpan.fStart, fSize);
            }

            return true;
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
            
            return *this;
        }
        

        // Conveniences
        constexpr uint8_t* begin() const noexcept { return fData; }
        constexpr uint8_t* end() const noexcept { return fData + fSize; }
        
        uint8_t* data() noexcept { return fData; }
        constexpr uint8_t* data() const noexcept { return fData; }
        constexpr size_t size() const noexcept { return fSize; }
        constexpr bool empty() const noexcept { return fSize == 0; }




        // span()
        // 
        // Create a ByteSpan from the memory buffer.
        // This is pure convenience, as a ByteSpan can easily be created
        // from the data() and size() functions.
        // The lifetime of the ByteSpan that is returned it not governed
        // by the MemBuff object.  This is something the caller must manage.
        ByteSpan span() const noexcept { return ByteSpan::fromPointers(begin(), end()); }
    };


    struct RefMemBuff final
    {
        std::atomic<uint32_t> refCount{ 1 };
        MemBuff buffer;

        static RefMemBuff* create(size_t size) noexcept
        {
            RefMemBuff* mem = new RefMemBuff();
            if (!mem)
                return nullptr;

            if (!mem->buffer.resetFromSize(size))
            {
                delete mem;
                return nullptr;
            }

            return mem;
        }

        void addRef() noexcept
        {
            refCount.fetch_add(1, std::memory_order_relaxed);
        }

        void release() noexcept
        {
            if (refCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
                delete this;
        }

        uint8_t* data() noexcept { return buffer.data(); }
        const uint8_t* data() const noexcept { return buffer.data(); }

        size_t size() const noexcept { return buffer.size(); }
        bool empty() const noexcept { return buffer.empty(); }
    };


}
