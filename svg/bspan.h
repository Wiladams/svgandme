#ifndef BSPAN_H_INCLUDED
#define BSPAN_H_INCLUDED



#include <cctype>
#include <cmath>
#include <cstdint>

#include <iterator>	// for std::data(), std::size()
#include <string.h>

#include "bit_hacks.h"
#include "bit_util.h"


namespace waavs 
{
    //
    // ByteSpan
    // 
    // A core type for representing a contiguous sequence of bytes.
    // As of C++ 20, there is std::span<>, and that would be a good 
    // choice, but it is not yet widely supported, and forces a jump
    // to C++20 besides.
    // 
    // The ByteSpan is used in everything from networking
    // to graphics bitmaps to audio buffers.
    // Having a universal representation of a chunk of data
    // allows for easy interoperability between different
    // subsystems.  
    // 
    // The ByteSpan, is just like 'span' and 'view' objects
    // it does not "own" the memory, it just points at it.
    // It is used as a stand-in for various data representations.
    // A key aspect of the ByteSpan is its ability to be used
    // as a 'cursor' to traverse the data it points to.


    struct ByteSpan final
    {
    private:
        const unsigned char* fStart{ nullptr };
        const unsigned char* fEnd{ nullptr };

    public:
        static const ByteSpan& null() noexcept 
        {
            static ByteSpan nullSpan{};
            return nullSpan;
        }

        // Constructors
        constexpr ByteSpan() noexcept = default;

        // Construct from start and end pointers
        constexpr ByteSpan(const unsigned char* start, const unsigned char* end) noexcept 
            : fStart(start)
            , fEnd(end) {}
        
        // Construct from a pointer and size
        constexpr ByteSpan(const unsigned char * start, size_t sz) noexcept
            : fStart(start)
            , fEnd(start ? start + sz : nullptr)
        {
        }

        // Construct from a null-terminated C string
        // Error:  If there is no null terminator, this will read past the end of the buffer
        ByteSpan(const char* cstr) noexcept
        {
            fStart = (reinterpret_cast<const unsigned char *>(cstr));
            fEnd = fStart;

            // Advance the end pointer until we see the null terminator
            while (*fEnd != '\0') {
                ++fEnd;
            }
        }

        //~ByteSpan() = default;


        constexpr void reset()
        { fStart = nullptr; fEnd = nullptr; }
        
        constexpr void resetPointers(const unsigned char* start, const unsigned char* end) noexcept
        {
            fStart = start;
            fEnd = end;
        }
        constexpr void resetStart(const unsigned char* start) noexcept { fStart = start; }
        constexpr void resetEnd(const unsigned char* end) noexcept {fEnd = end;}

        constexpr void resetFromSize(const void *data, size_t sz) noexcept
        {
            fStart = static_cast<const unsigned char *>(data);
            fEnd = fStart + sz;
        }
        
        // Static factory methods for convenience
        static  constexpr ByteSpan fromPointers(const unsigned char* startAt, const unsigned char* endAt) noexcept
        {
            ByteSpan bs(startAt, endAt);
            return bs;
        }
        
        static INLINE ByteSpan fromPointerAndSize(const unsigned char* start, size_t sz)
        {
            ByteSpan bs;
            bs.fStart = start;
            bs.fEnd = start + sz;
            return bs;
        }

        // setting up for a range-based for loop
        // not actually that useful, as it's just memory traversal
        // but, having data() and size() hides the internals
        constexpr const unsigned char* data() const noexcept { return fStart; }
        constexpr size_t size() const noexcept { return (fStart && fEnd >= fStart) ? size_t(fEnd - fStart) : 0; }

        constexpr const uint8_t* begin() const noexcept { return fStart; }
        constexpr const uint8_t* end() const noexcept { return fEnd; }
        constexpr bool empty() const noexcept { return size() == 0; }


        // Type conversions
        explicit constexpr operator bool() const noexcept { return !empty(); }

        // get value of character at fStart, like a 'peek' operation
        // If the ByteSpan is currently empty, these will return 0, rather than 
        // throwing an exception, so check size before calling if that's necessary
        constexpr uint8_t operator*() const noexcept
        {
            return (fStart && fStart < fEnd) ? *fStart : 0;
        }

        // subSpan()
        // 
        // Create a ByteSpan that is a view on the current span
        // If the requested position plus size is greater than the amount
        // of span remaining at that position, the size will be truncated 
        // to the amount remaining from the requested position.
        // So, it's more like an intersection of the desired subspan
        // and the current span.
        constexpr ByteSpan subSpan(size_t startAt, size_t sz) const noexcept
        {
            const size_t n = size();
            const size_t off = startAt < n ? startAt : n;
            const size_t len = sz < (n - off) ? sz : (n - off);
            return ByteSpan(fStart + off, len);
        }

        constexpr ByteSpan take(size_t n) const noexcept
        {
            return subSpan(0, n);
        }

        // advance the start pointer the specified number of entries
        // constrain to end 
        constexpr ByteSpan& advance(size_t n) noexcept
        {
            fStart = (fStart + n <= fEnd) ? fStart + n : fEnd;
            return *this;
        }

        constexpr ByteSpan& advanceToEnd() noexcept
        {
            fStart = fEnd;
            return *this;
        }

        constexpr ByteSpan& operator+=(size_t n) noexcept  {  return advance(n); }

        constexpr ByteSpan& operator++() noexcept { return advance(1); }
        constexpr ByteSpan operator++(int) noexcept
        {
            ByteSpan tmp = *this;
            advance(1);
            return tmp;
        }




        // Array access
        uint8_t& operator[](size_t i) noexcept { return const_cast<uint8_t&>(fStart[i]); }
        const uint8_t& operator[](size_t i) const noexcept { return fStart[i]; }


        // BUGBUG - not sure these should be used any more
        // favoring interned strings is probably a better approach
        // operators for comparison
        // 
        // operator==;
        // operator!=;
        // operator<=;
        // operator>=;
        
        // isEqual()
        // A pointer comparison
        bool isEqual(const ByteSpan& b) const noexcept
        {
            return fStart == b.fStart && size() == b.size();
        }

        bool equivalent(const ByteSpan& b) const noexcept
        {
            const size_t n = size();
            if (n != b.size())
                return false;

            if (n == 0)
                return true;

            if (!fStart || !b.fStart)
                return false;

            return std::memcmp(fStart, b.fStart, n) == 0;
        }
        
        // operator==
        // Perform a full content comparison of the two spans
        bool operator==(const ByteSpan& b) const noexcept
        {
            return equivalent(b);
        }

        bool operator==(const char* b) const noexcept
        {
            if (!b)
                return false;

            return equivalent(ByteSpan(b));
        }


        bool operator!=(const ByteSpan& other) const noexcept
        {
            return !(*this == other);
        }

        bool operator<(const ByteSpan& b) const noexcept
        {
            size_t minSize = size() < b.size() ? size() : b.size();
            int cmp = memcmp(fStart, b.fStart, minSize);
            return (cmp < 0) || (cmp == 0 && size() < b.size());
        }


        bool operator>(const ByteSpan& b) const noexcept
        {
            size_t minSize = size() < b.size() ? size() : b.size();
            int cmp = memcmp(fStart, b.fStart, minSize);
            return (cmp > 0) || (cmp == 0 && size() > b.size());
        }


        bool operator<=(const ByteSpan& b) const noexcept
        {
            size_t minSize = size() < b.size() ? size() : b.size();
            int cmp = memcmp(fStart, b.fStart, minSize);
            return (cmp < 0) || (cmp == 0 && size() <= b.size());
        }

        bool operator>=(const ByteSpan& b) const noexcept
        {
            size_t minSize = size() < b.size() ? size() : b.size();
            int cmp = memcmp(fStart, b.fStart, minSize);
            return (cmp > 0) || (cmp == 0 && size() >= b.size());
        }

    };

    ASSERT_MEMCPY_SAFE(ByteSpan);
}



#endif // BSPAN_H_INCLUDED

