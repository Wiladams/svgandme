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
#include <cctype>
#include <type_traits>
#include <cmath>        // for fmaf

#define WAAVS_NOMINMAX 1




#ifdef _MSC_VER
#ifndef INLINE
#define INLINE __forceinline
#endif
#ifndef unlikely
#define unlikely(x) x
#endif // unlikely
#else
#ifndef INLINE
#define INLINE __attribute__((always_inline)) inline
#endif // really_inline

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif // unlikely
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



#ifndef WAAVS_ALIGN_UP_POW2
# define WAAVS_ALIGN_UP_POW2(x,a) ( ((x) + ((a)-1)) & ~((a)-1) )
#endif




// Definition of fmaf if not already defined
#ifndef WAAVS_FMAF
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#define WAAVS_FMAF(a,b,c) fmaf((a),(b),(c))
#else
#define WAAVS_FMAF(a,b,c) ((a)*(b)+(c))
#endif
#endif




// Useful assertions for type construction
// Asserts that a type is trivially copyable (memcpy-safe)
#define ASSERT_MEMCPY_SAFE(Type) \
    static_assert(std::is_trivially_copyable<Type>::value, #Type " must be trivially copyable"); \
    static_assert(std::is_standard_layout<Type>::value,    #Type " must have standard layout")

// Asserts that a type is a POD (Plain Old Data) — stricter than memcpy-safe
#define ASSERT_POD_TYPE(Type)                                                   \
    static_assert(std::is_trivially_copyable<Type>::value,                      \
        #Type " must be trivially copyable (POD requirement)");                 \
    static_assert(std::is_standard_layout<Type>::value,                         \
        #Type " must have standard layout (POD requirement)")

// Asserts that a type has a predictable C-compatible layout (but not necessarily trivial)
#define ASSERT_FLAT_STRUCT(Type) \
    static_assert(std::is_standard_layout<Type>::value, #Type " must have standard layout")

// Asserts that a type has an exact size (e.g., for packed structs or cache alignment)
#define ASSERT_STRUCT_SIZE(Type, ExpectedSize) \
    static_assert(sizeof(Type) == (ExpectedSize), #Type " must be " #ExpectedSize " bytes in size")


