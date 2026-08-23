#pragma once


#include "lang_charset.h"

// Related to hash functions on strings
namespace waavs
{

    // Byte Hashing - FNV-1a
    // http://www.isthe.com/chongo/tech/comp/fnv/
    // 
    // 32-bit FNV-1a constants
    constexpr uint32_t FNV1A_32_INIT = 0x811c9dc5;
    constexpr uint32_t FNV1A_32_PRIME = 0x01000193;

    // 64-bit FNV-1a constants
    constexpr uint64_t FNV1A_64_INIT = 0xcbf29ce484222325ULL;
    constexpr uint64_t FNV1A_64_PRIME = 0x100000001b3ULL;

    // 32-bit FNV-1a hash
    INLINE constexpr uint32_t fnv1a_32(const void* data, const size_t size) noexcept
    {
        const uint8_t* bytes = (const uint8_t*)data;
        uint32_t hash = FNV1A_32_INIT;
        for (size_t i = 0; i < size; i++) {
            hash ^= bytes[i];
            hash *= FNV1A_32_PRIME;
        }
        return hash;
    }

    // 64-bit FNV-1a hash
    INLINE constexpr uint64_t fnv1a_64(const void* data, const size_t size) noexcept
    {
        const uint8_t* bytes = (const uint8_t*)data;
        uint64_t hash = FNV1A_64_INIT;
        for (size_t i = 0; i < size; i++) {
            hash ^= bytes[i];
            hash *= FNV1A_64_PRIME;
        }
        return hash;
    }



    // 32-bit case-insensitive FNV-1a hash
    INLINE uint32_t fnv1a_32_case_insensitive(const void* data, const size_t size) noexcept
    {
        const uint8_t* bytes = (const uint8_t*)data;
        uint32_t hash = FNV1A_32_INIT;
        for (size_t i = 0; i < size; i++) {
            // Convert byte to lowercase
            auto c = to_lower(bytes[i]);

            hash ^= c;
            hash *= FNV1A_32_PRIME;
        }
        return hash;
    }


}


namespace waavs
{
    // report the next highest power of 2 for a given number.
    static inline size_t ws_table_next_pow2(size_t n) noexcept
    {
        if (n < 2) return 2;

        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;

#if SIZE_MAX > 0xffffffffu
        n |= n >> 32;
#endif

        return n + 1;
    }

    // ws_table_ptr_hash
    // 
    // We want to use memory pointers as keys in our open 
    // hash table.  The problem with pointers is they tend 
    // to cluster together, so just using them directly will
    // not result in a good distribution of hash values.  
    // We run the pointers through this hash function to obtain
    // better distribution before storing a value in the hash table
    // 
    // Note:
    // Derived from MurmurHash3 finalizer, 
    // which is a good hash function for 64-bit integers.
    // BUGBUG: could create explicity 32-bit and 64-bit versions
    // fmix64
    // fmix32
    //
        // MurmurHash3 32-bit finalizer
    static inline uint32_t fmix32(uint32_t x) noexcept
    {
        x ^= x >> 16;
        x *= 0x85ebca6bU;
        x ^= x >> 13;
        x *= 0xc2b2ae35U;
        x ^= x >> 16;

        return x;
    }

    // MurmurHash3 64-bit finalizer
    static inline uint64_t fmix64(uint64_t x) noexcept
    {
        x ^= x >> 33;
        x *= 0xff51afd7ed558ccdULL;
        x ^= x >> 33;
        x *= 0xc4ceb9fe1a85ec53ULL;
        x ^= x >> 33;

        return x;
    }

    static inline size_t ws_table_ptr_hash(const void* p) noexcept
    {
        uintptr_t x = reinterpret_cast<uintptr_t>(p);

#if INTPTR_MAX > 0xffffffff
        return static_cast<size_t>(
            fmix64(static_cast<uint64_t>(x)));
#else
        return static_cast<size_t>(
            fmix32(static_cast<uint32_t>(x)));
#endif
    }


}

