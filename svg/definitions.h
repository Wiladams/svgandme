#pragma once



// Herein lie definitions of some things that are low level enough 
// to be universal
// Don't put Windows specifics in here. Put that stuff in the w32_***
// files.
//
// Put things like universal typedefs and the like in here
//
// If this file hasn't already been included, you should include
// it as the first item to be included.
//

#include <cstdint>		// uint8_t, etc
#include <cstddef>		// nullptr_t, ptrdiff_t, size_t

#define WAAVS_NOMINMAX 1

#ifdef _MSC_VER
#define INLINE __forceinline
#else
#define INLINE static inline
#endif

// MSVC++ has #defines for min/max that break various things
// so undef them, as we won't want to use those
#if WAAVS_NOMINMAX
    #ifdef min
        #undef min
    #endif
    #ifdef max
        #undef max
    #endif
#endif



