// lang_span.h

#pragma once

#include "bit_hacks.h"
#include "bit_util.h"

#include <cctype>
#include <cmath>
#include <cstdint>


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
        const uint8_t* fStart{ nullptr };
        const uint8_t* fEnd{ nullptr };

    public:
        static const ByteSpan& null() noexcept 
        {
            static ByteSpan nullSpan{};
            return nullSpan;
        }

        // Constructors
        constexpr ByteSpan() noexcept = default;

        // Construct from start and end pointers
        constexpr ByteSpan(const uint8_t* start, const uint8_t* end) noexcept 
            : fStart(start)
            , fEnd(end) {}
        
        // Construct from a pointer and size
        constexpr ByteSpan(const uint8_t * start, size_t sz) noexcept
            : fStart(start)
            , fEnd(start ? start + sz : nullptr)
        {
        }

        // Construct from a null-terminated C string
        // Error:  If there is no null terminator, this will read past the end of the buffer
        ByteSpan(const char* cstr) noexcept
        {
            if (!cstr)
            {
                fStart = nullptr;
                fEnd = nullptr;
                return;
            }

            fStart = reinterpret_cast<const uint8_t*>(cstr);
            fEnd = fStart + strlen(cstr);
        }

        //~ByteSpan() = default;


        constexpr void reset() { fStart = nullptr; fEnd = nullptr; }
        
        constexpr void resetPointers(const uint8_t * start, const uint8_t * end) noexcept
        {
            fStart = start;
            fEnd = end;
        }
        constexpr void resetStart(const uint8_t * start) noexcept { fStart = start; }
        constexpr void resetEnd(const uint8_t* end) noexcept {fEnd = end;}

        constexpr void resetFromSize(const void *data, size_t sz) noexcept
        {
            fStart = static_cast<const uint8_t*>(data);
            fEnd = fStart + sz;
        }
        


        // setting up for a range-based for loop
        // not actually that useful, as it's just memory traversal
        // but, having data() and size() hides the internals
        constexpr const uint8_t* data() const noexcept { return fStart; }
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


        // Create a ByteSpan beginning at startAt and extending
        // through the end of the current span.
        // If startAt is beyond the end, returns an empty span
        // positioned at the end.
        constexpr ByteSpan subSpan(size_t startAt) const noexcept
        {
            if (!fStart)
                return {};

            const size_t n = size();
            const size_t off = startAt < n ? startAt : n;

            return ByteSpan(fStart + off, n - off);
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
            if (!fStart)
                return {};

            const size_t n = size();
            const size_t off = startAt < n ? startAt : n;
            const size_t len = sz < (n - off) ? sz : (n - off);
            
            return ByteSpan(fStart + off, len);
        }

        constexpr ByteSpan take(size_t n) const noexcept
        {
            return subSpan(0, n);
        }

        // advance()
        // 
        // advance the start pointer the specified number of entries
        // constrain to end 
        // Protect against null pointers
        constexpr ByteSpan& advance(size_t n) noexcept
        {
            if (!fStart || !fEnd)
                return *this;

            const size_t remaining = size();
            fStart += (n < remaining) ? n : remaining;
            return *this;
        }

        constexpr ByteSpan& advanceToEnd() noexcept
        {
            fStart = fEnd;
            return *this;
        }

        constexpr ByteSpan& operator+=(size_t n) noexcept  {  return advance(n); }

        constexpr ByteSpan& operator++() noexcept { return advance(1); }
        //constexpr ByteSpan operator++(int) noexcept
        //{
        //    ByteSpan tmp = *this;
        //    advance(1);
        //    return tmp;
        //}




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

        // -----------------------------------------
        // Static factory methods for convenience
        // -----------------------------------------
        static  constexpr ByteSpan fromPointers(const uint8_t* startAt, const uint8_t* endAt) noexcept
        {
            ByteSpan bs(startAt, endAt);
            return bs;
        }

        static INLINE ByteSpan fromPointerAndSize(const uint8_t* start, size_t sz)
        {
            ByteSpan bs;
            bs.fStart = start;
            bs.fEnd = start + sz;
            return bs;
        }
    };

    ASSERT_MEMCPY_SAFE(ByteSpan);
}

namespace waavs
{
    // hex_nibble
// 
// given an input character representing a hex digit
// put the decimal value of that hex digit in outValue
    static INLINE WGResult hex_nibble(const uint8_t vIn, uint8_t& outValue) noexcept
    {
        if (vIn >= '0' && vIn <= '9')
        {
            outValue = vIn - '0';
            return WG_SUCCESS;
        }

        if (vIn >= 'a' && vIn <= 'f')
        {
            outValue = vIn - 'a' + 10;
            return WG_SUCCESS;
        }

        if (vIn >= 'A' && vIn <= 'F')
        {
            outValue = vIn - 'A' + 10;
            return WG_SUCCESS;
        }

        return WG_ERROR_Invalid_Argument;
    }

    // Read two bytes representing a hex byte (e.g. '4F') and 
    // convert them to a single byte value (0x4F in this example).
    // Return: 
    //  WG_SUCCESS if successful, or 
    //  WG_ERROR_Invalid_Argument if either of 
    //      the input characters is not a valid hex digit.
    static INLINE WGResult hex_byte(const uint8_t highNibble, const uint8_t lowNibble, uint8_t& outValue) noexcept
    {
        uint8_t highValue = 0;
        uint8_t lowValue = 0;
        if (hex_nibble(highNibble, highValue) != WG_SUCCESS)
            return WG_ERROR_Invalid_Argument;

        if (hex_nibble(lowNibble, lowValue) != WG_SUCCESS)
            return WG_ERROR_Invalid_Argument;

        outValue = (highValue << 4) | lowValue;

        return WG_SUCCESS;
    }

    // parseHex64u
//
// Parse a hex string into a 64-bit unsigned integer.
// The string must be a valid hex string, and must not contain any spaces.
    static INLINE bool parseHex64u(const ByteSpan& inSpan, uint64_t& outValue) noexcept
    {
        if (inSpan.size() == 0 || inSpan.size() > 16)
            return false;


        // building up outValue as we go
        outValue = 0;
        const uint8_t* c = inSpan.begin();
        while (c != inSpan.end())
        {
            uint8_t nibble = 0;
            if (hex_nibble(*c, nibble) == WG_SUCCESS)
            {
                // We shift by 4 bits, because we're processing a nibble
                // at a time. so shift and add the latest nibble
                outValue <<= 4;
                outValue |= nibble;
            }
            else {
                // if we get here, it means we had an invalid hex digit
                // in the input string.  We return false to indicate a parse failure.
                return false;
            }

            c++;
        }

        return true;
    }

    // return 1 if the chunk is "true" or "1" or "t" or "T" or "y" or "Y" or "yes" or "Yes" or "YES"
// return 0 if the chunk is "false" or "0" or "f" or "F" or "n" or "N" or "no" or "No" or "NO"
// return 0 otherwise
    static INLINE int toBoolInt(const ByteSpan& inChunk)
    {
        ByteSpan s = inChunk;

        if (s == "true" || s == "1" || s == "t" || s == "T" || s == "y" || s == "Y" || s == "yes" || s == "Yes" || s == "YES")
            return 1;
        else if (s == "false" || s == "0" || s == "f" || s == "F" || s == "n" || s == "N" || s == "no" || s == "No" || s == "NO")
            return 0;
        else
            return 0;
    }
}



