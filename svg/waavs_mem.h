#ifndef WAAVS_MEM_H_INCLUDED
#define WAAVS_MEM_H_INCLUDED


#include <stdint.h>
#include <stdlib.h>     // aligned_alloc

/* ---- alignment helpers ---- */
/* ----- portability: 16-byte alignment for SIMD friendliness ----- */
#if defined(_MSC_VER)
#define WAAVS_ALIGN16 __declspec(align(16))
#elif defined(__GNUC__) || defined(__clang__)
#define WAAVS_ALIGN16 __attribute__((aligned(16)))
#else
#define WAAVS_ALIGN16 /* unknown compiler, no alignment */
#endif

#ifndef WAAVS_ALIGN32
# if defined(_MSC_VER)
#   define WAAVS_ALIGN32 __declspec(align(32))
# elif defined(__GNUC__) || defined(__clang__)
#   define WAAVS_ALIGN32 __attribute__((aligned(32)))
# else
#   define WAAVS_ALIGN32
# endif
#endif


#ifndef WAAVS_ALIGN64
# if defined(_MSC_VER)
#   define WAAVS_ALIGN64 __declspec(align(64))
# elif defined(__GNUC__) || defined(__clang__)
#   define WAAVS_ALIGN64 __attribute__((aligned(64)))
# else
#   define WAAVS_ALIGN64
# endif
#endif

// Forward function declarations
static inline void* waavs_aligned_malloc(size_t size, size_t alignment);
static inline void waavs_aligned_free(void* p);



// aligned malloc/free
static inline void* waavs_aligned_malloc(size_t size, size_t alignment)
{
#if defined(_MSC_VER)
    return _aligned_malloc(size, alignment);
#elif (__STDC_VERSION__ >= 201112L) && !defined(__APPLE__)
    size_t padded = WAAVS_ALIGN_UP_POW2(size, alignment);
    return aligned_alloc(alignment, padded);
#else
    void* p = NULL;
    if (alignment < sizeof(void*)) alignment = sizeof(void*);
    if (posix_memalign(&p, alignment, size) != 0) p = NULL;
    return p;
#endif
}

static inline void waavs_aligned_free(void* p)
{
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    free(p);
#endif
}

#endif