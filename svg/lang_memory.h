// lang_memory.h

#pragma once

#include "definitions.h"



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
            resetFromData(other.fData, other.fSize);
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

        // Construct from a span
        MemBuff(const uint8_t *d, size_t s) noexcept
        {
            resetFromData(d, s);
        }

        ~MemBuff() noexcept
        {
            if (fData != nullptr)
                delete[] fData;
        }

        // Conveniences
        uint8_t* begin() noexcept { return fData; }
        const uint8_t* begin() const noexcept { return fData; }

        uint8_t* end() noexcept { return fData ? fData + fSize : nullptr; }
        const uint8_t* end() const noexcept { return fData ? fData + fSize : nullptr; }

        uint8_t* data() noexcept { return fData; }
        const uint8_t* data() const noexcept { return fData; }

        constexpr size_t size() const noexcept { return fSize; }
        constexpr bool empty() const noexcept { return fSize == 0; }

        // Setting up
        void reset() noexcept
        {
            if (fData != nullptr)
                delete[] fData;
            fData = nullptr;
            fSize = 0;
        }
        
        bool resetFromSize(const size_t sz) noexcept
        {
            if (sz == 0)
            {
                reset();
                return true;
            }

            uint8_t* newData =
                new (std::nothrow) uint8_t[sz];

            if (!newData)
                return false;

            delete[] fData;

            fData = newData;
            fSize = sz;

            return true;
        }


        // resetFromData
        // 
        // copy the data from the input span into the memory buffer

        bool resetFromData(
            const uint8_t* data,
            size_t size) noexcept
        {
            if (size == 0)
            {
                reset();
                return true;
            }

            if (!data)
                return false;

            uint8_t* newData =
                new (std::nothrow) uint8_t[size];

            if (!newData)
                return false;

            memcpy(newData, data, size);

            delete[] fData;

            fData = newData;
            fSize = size;

            return true;
        }


        // Operators
        
        // Copy Assignment operator (deep copy)
        MemBuff& operator=(const MemBuff& other) noexcept
        {
            if (this != &other)
            {
                resetFromData(other.fData, other.fSize);
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
        


    };


    struct RefMemBuff final
    {
        std::atomic<uint32_t> refCount{ 1 };
        MemBuff buffer;

        static RefMemBuff* create(size_t size) noexcept
        {
            RefMemBuff* mem = new (std::nothrow) RefMemBuff();
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

    struct SharedMemBuff final
    {
    private:
        RefMemBuff* fMem{ nullptr };

    public:
        constexpr SharedMemBuff() noexcept = default;

        explicit SharedMemBuff(size_t size) noexcept
        {
            fMem = RefMemBuff::create(size);
        }

        SharedMemBuff(const SharedMemBuff& other) noexcept
            : fMem(other.fMem)
        {
            if (fMem)
                fMem->addRef();
        }

        SharedMemBuff(SharedMemBuff&& other) noexcept
            : fMem(other.fMem)
        {
            other.fMem = nullptr;
        }

        ~SharedMemBuff() noexcept
        {
            reset();
        }

        SharedMemBuff& operator=(const SharedMemBuff& other) noexcept
        {
            if (this == &other)
                return *this;

            if (other.fMem)
                other.fMem->addRef();

            reset();

            fMem = other.fMem;
            return *this;
        }

        SharedMemBuff& operator=(SharedMemBuff&& other) noexcept
        {
            if (this == &other)
                return *this;

            reset();

            fMem = other.fMem;
            other.fMem = nullptr;

            return *this;
        }

        bool resetFromSize(size_t size) noexcept
        {
            RefMemBuff* mem = RefMemBuff::create(size);
            if (!mem && size > 0)
                return false;

            reset();

            fMem = mem;
            return true;
        }

        void reset() noexcept
        {
            if (fMem)
            {
                fMem->release();
                fMem = nullptr;
            }
        }

        uint8_t* data() noexcept
        {
            return fMem ? fMem->data() : nullptr;
        }

        const uint8_t* data() const noexcept
        {
            return fMem ? fMem->data() : nullptr;
        }

        uint8_t* begin() noexcept
        {
            return data();
        }

        const uint8_t* begin() const noexcept
        {
            return data();
        }

        uint8_t* end() noexcept
        {
            return fMem ? fMem->data() + fMem->size() : nullptr;
        }

        const uint8_t* end() const noexcept
        {
            return fMem ? fMem->data() + fMem->size() : nullptr;
        }

        size_t size() const noexcept
        {
            return fMem ? fMem->size() : 0;
        }

        bool empty() const noexcept
        {
            return size() == 0;
        }

        explicit operator bool() const noexcept
        {
            return fMem && !fMem->empty();
        }

        uint32_t refCount() const noexcept
        {
            return fMem ? fMem->refCount.load(std::memory_order_relaxed) : 0;
        }
    };

}
