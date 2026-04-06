#pragma once

#include "definitions.h"

//
// This file contains helpers for scanning pieces of XML
// using either scalar code or NEON SIMD, depending on 
// what's available.
//
// The helpers are meant to be used by the XML tokenizer
// and other parts of the system, to make it simpler
// faster, more efficient, and less error-prone.
//
// Returns end if no matching byte is found.
static INLINE const uint8_t* find_first_of2_scalar(
    const uint8_t* p,
    const uint8_t* end,
    uint8_t c0,
    uint8_t c1) noexcept
{
    while (p < end)
    {
        const uint8_t v = *p;
        if (v == c0 || v == c1)
            return p;
        ++p;
    }

    return end;
}

// Returns end if no matching byte is found.
static INLINE const uint8_t* find_first_of3_scalar(
    const uint8_t* p,
    const uint8_t* end,
    uint8_t c0,
    uint8_t c1,
    uint8_t c2) noexcept
{
    while (p < end)
    {
        const uint8_t v = *p;
        if (v == c0 || v == c1 || v == c2)
            return p;
        ++p;
    }

    return end;
}

// Returns end if no matching byte is found.
static INLINE const uint8_t* find_first_of4_scalar(
    const uint8_t* p,
    const uint8_t* end,
    uint8_t c0,
    uint8_t c1,
    uint8_t c2,
    uint8_t c3) noexcept
{
    while (p < end)
    {
        const uint8_t v = *p;
        if (v == c0 || v == c1 || v == c2 || v == c3)
            return p;
        ++p;
    }

    return end;
}

// Returns end if all bytes are one of {c0,c1,c2,c3}.
static INLINE const uint8_t* find_first_not_of4_scalar(
    const uint8_t* p,
    const uint8_t* end,
    uint8_t c0,
    uint8_t c1,
    uint8_t c2,
    uint8_t c3) noexcept
{
    while (p < end)
    {
        const uint8_t v = *p;
        if (!(v == c0 || v == c1 || v == c2 || v == c3))
            return p;
        ++p;
    }

    return end;
}

#if defined(__ARM_NEON) || defined(__aarch64__)

// Helper: scan a 16-byte "match" mask that is 0xFF at matching lanes, 0x00 otherwise.
// Returns the first matching lane index, or 16 if none.
static INLINE int neon_first_true_lane_u8(uint8x16_t mask) noexcept
{
    alignas(16) uint8_t lanes[16];
    vst1q_u8(lanes, mask);

    for (int i = 0; i < 16; ++i)
    {
        if (lanes[i])
            return i;
    }

    return 16;
}

// Returns end if no matching byte is found.
static INLINE const uint8_t* find_first_of2_neon(
    const uint8_t* p,
    const uint8_t* end,
    uint8_t c0,
    uint8_t c1) noexcept
{
    const size_t n = (size_t)(end - p);
    size_t i = 0;

    const uint8x16_t vc0 = vdupq_n_u8(c0);
    const uint8x16_t vc1 = vdupq_n_u8(c1);

    for (; i + 16 <= n; i += 16)
    {
        const uint8x16_t v = vld1q_u8(p + i);

        const uint8x16_t m0 = vceqq_u8(v, vc0);
        const uint8x16_t m1 = vceqq_u8(v, vc1);
        const uint8x16_t mm = vorrq_u8(m0, m1);

        const int lane = neon_first_true_lane_u8(mm);
        if (lane < 16)
            return p + i + (size_t)lane;
    }

    return find_first_of2_scalar(p + i, end, c0, c1);
}

// Returns end if no matching byte is found.
static INLINE const uint8_t* find_first_of3_neon(
    const uint8_t* p,
    const uint8_t* end,
    uint8_t c0,
    uint8_t c1,
    uint8_t c2) noexcept
{
    const size_t n = (size_t)(end - p);
    size_t i = 0;

    const uint8x16_t vc0 = vdupq_n_u8(c0);
    const uint8x16_t vc1 = vdupq_n_u8(c1);
    const uint8x16_t vc2 = vdupq_n_u8(c2);

    for (; i + 16 <= n; i += 16)
    {
        const uint8x16_t v = vld1q_u8(p + i);

        const uint8x16_t m0 = vceqq_u8(v, vc0);
        const uint8x16_t m1 = vceqq_u8(v, vc1);
        const uint8x16_t m2 = vceqq_u8(v, vc2);
        const uint8x16_t mm = vorrq_u8(vorrq_u8(m0, m1), m2);

        const int lane = neon_first_true_lane_u8(mm);
        if (lane < 16)
            return p + i + (size_t)lane;
    }

    return find_first_of3_scalar(p + i, end, c0, c1, c2);
}

// Returns end if no matching byte is found.
static INLINE const uint8_t* find_first_of4_neon(
    const uint8_t* p,
    const uint8_t* end,
    uint8_t c0,
    uint8_t c1,
    uint8_t c2,
    uint8_t c3) noexcept
{
    const size_t n = (size_t)(end - p);
    size_t i = 0;

    const uint8x16_t vc0 = vdupq_n_u8(c0);
    const uint8x16_t vc1 = vdupq_n_u8(c1);
    const uint8x16_t vc2 = vdupq_n_u8(c2);
    const uint8x16_t vc3 = vdupq_n_u8(c3);

    for (; i + 16 <= n; i += 16)
    {
        const uint8x16_t v = vld1q_u8(p + i);

        const uint8x16_t m0 = vceqq_u8(v, vc0);
        const uint8x16_t m1 = vceqq_u8(v, vc1);
        const uint8x16_t m2 = vceqq_u8(v, vc2);
        const uint8x16_t m3 = vceqq_u8(v, vc3);
        const uint8x16_t mm = vorrq_u8(vorrq_u8(m0, m1), vorrq_u8(m2, m3));

        const int lane = neon_first_true_lane_u8(mm);
        if (lane < 16)
            return p + i + (size_t)lane;
    }

    return find_first_of4_scalar(p + i, end, c0, c1, c2, c3);
}

// Returns end if all bytes are one of {c0,c1,c2,c3}.
static INLINE const uint8_t* find_first_not_of4_neon(
    const uint8_t* p,
    const uint8_t* end,
    uint8_t c0,
    uint8_t c1,
    uint8_t c2,
    uint8_t c3) noexcept
{
    const size_t n = (size_t)(end - p);
    size_t i = 0;

    const uint8x16_t vc0 = vdupq_n_u8(c0);
    const uint8x16_t vc1 = vdupq_n_u8(c1);
    const uint8x16_t vc2 = vdupq_n_u8(c2);
    const uint8x16_t vc3 = vdupq_n_u8(c3);

    for (; i + 16 <= n; i += 16)
    {
        const uint8x16_t v = vld1q_u8(p + i);

        const uint8x16_t m0 = vceqq_u8(v, vc0);
        const uint8x16_t m1 = vceqq_u8(v, vc1);
        const uint8x16_t m2 = vceqq_u8(v, vc2);
        const uint8x16_t m3 = vceqq_u8(v, vc3);

        const uint8x16_t allowed = vorrq_u8(vorrq_u8(m0, m1), vorrq_u8(m2, m3));
        const uint8x16_t not_allowed = vmvnq_u8(allowed);

        const int lane = neon_first_true_lane_u8(not_allowed);
        if (lane < 16)
            return p + i + (size_t)lane;
    }

    return find_first_not_of4_scalar(p + i, end, c0, c1, c2, c3);
}

#endif

static INLINE const uint8_t* find_first_of2(
    const uint8_t* p,
    const uint8_t* end,
    uint8_t c0,
    uint8_t c1) noexcept
{
#if defined(__ARM_NEON) || defined(__aarch64__)
    return find_first_of2_neon(p, end, c0, c1);
#else
    return find_first_of2_scalar(p, end, c0, c1);
#endif
}

static INLINE const uint8_t* find_first_of3(
    const uint8_t* p,
    const uint8_t* end,
    uint8_t c0,
    uint8_t c1,
    uint8_t c2) noexcept
{
#if defined(__ARM_NEON) || defined(__aarch64__)
    return find_first_of3_neon(p, end, c0, c1, c2);
#else
    return find_first_of3_scalar(p, end, c0, c1, c2);
#endif
}

static INLINE const uint8_t* find_first_of4(
    const uint8_t* p,
    const uint8_t* end,
    uint8_t c0,
    uint8_t c1,
    uint8_t c2,
    uint8_t c3) noexcept
{
#if defined(__ARM_NEON) || defined(__aarch64__)
    return find_first_of4_neon(p, end, c0, c1, c2, c3);
#else
    return find_first_of4_scalar(p, end, c0, c1, c2, c3);
#endif
}

static INLINE const uint8_t* find_first_not_of4(
    const uint8_t* p,
    const uint8_t* end,
    uint8_t c0,
    uint8_t c1,
    uint8_t c2,
    uint8_t c3) noexcept
{
#if defined(__ARM_NEON) || defined(__aarch64__)
    return find_first_not_of4_neon(p, end, c0, c1, c2, c3);
#else
    return find_first_not_of4_scalar(p, end, c0, c1, c2, c3);
#endif
}



static INLINE const uint8_t* find_first_not_xml_wsp(
    const uint8_t* p,
    const uint8_t* end) noexcept
{
    return find_first_not_of4(p, end, ' ', '\t', '\r', '\n');
}

// ------------------------------------------------------------
// Scalar helpers
// ------------------------------------------------------------

static INLINE bool chr_is_namechar_ascii(uint8_t c) noexcept
{
    return
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') ||
        c == '_' ||
        c == '-' ||
        c == '.' ||
        c == ':';
}

static INLINE bool chr_is_namestart_ascii(uint8_t c) noexcept
{
    return
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        c == '_' ||
        c == ':';
}

static INLINE const uint8_t* find_first_not_namechar_ascii_scalar(
    const uint8_t* p,
    const uint8_t* end) noexcept
{
    while (p < end && chr_is_namechar_ascii(*p))
        ++p;

    return p;
}

static INLINE const uint8_t* find_first_not_namestart_ascii_scalar(
    const uint8_t* p,
    const uint8_t* end) noexcept
{
    while (p < end && chr_is_namestart_ascii(*p))
        ++p;

    return p;
}

#if defined(__ARM_NEON) || defined(__aarch64__)

// ------------------------------------------------------------
// NEON helpers
// ------------------------------------------------------------
/*
// Returns first lane index whose byte is non-zero, or 16 if none.
static INLINE int neon_first_true_lane_u8(uint8x16_t mask) noexcept
{
    alignas(16) uint8_t lanes[16];
    vst1q_u8(lanes, mask);

    for (int i = 0; i < 16; ++i)
    {
        if (lanes[i])
            return i;
    }

    return 16;
}
*/

static INLINE uint8x16_t neon_range_inclusive_u8(
    uint8x16_t v,
    uint8_t lo,
    uint8_t hi) noexcept
{
    const uint8x16_t vlo = vdupq_n_u8(lo);
    const uint8x16_t vhi = vdupq_n_u8(hi);

    const uint8x16_t ge = vcgeq_u8(v, vlo);
    const uint8x16_t le = vcleq_u8(v, vhi);
    return vandq_u8(ge, le);
}

static INLINE uint8x16_t neon_eq_u8(uint8x16_t v, uint8_t c) noexcept
{
    return vceqq_u8(v, vdupq_n_u8(c));
}

// Valid name chars:
//   A-Z a-z 0-9 _ - . :
static INLINE uint8x16_t neon_is_namechar_ascii_mask(uint8x16_t v) noexcept
{
    const uint8x16_t upper = neon_range_inclusive_u8(v, 'A', 'Z');
    const uint8x16_t lower = neon_range_inclusive_u8(v, 'a', 'z');
    const uint8x16_t digit = neon_range_inclusive_u8(v, '0', '9');

    const uint8x16_t uscore = neon_eq_u8(v, '_');
    const uint8x16_t dash = neon_eq_u8(v, '-');
    const uint8x16_t dot = neon_eq_u8(v, '.');
    const uint8x16_t colon = neon_eq_u8(v, ':');

    uint8x16_t m = vorrq_u8(upper, lower);
    m = vorrq_u8(m, digit);
    m = vorrq_u8(m, uscore);
    m = vorrq_u8(m, dash);
    m = vorrq_u8(m, dot);
    m = vorrq_u8(m, colon);

    return m;
}

// Valid name-start chars:
//   A-Z a-z _ :
static INLINE uint8x16_t neon_is_namestart_ascii_mask(uint8x16_t v) noexcept
{
    const uint8x16_t upper = neon_range_inclusive_u8(v, 'A', 'Z');
    const uint8x16_t lower = neon_range_inclusive_u8(v, 'a', 'z');
    const uint8x16_t uscore = neon_eq_u8(v, '_');
    const uint8x16_t colon = neon_eq_u8(v, ':');

    uint8x16_t m = vorrq_u8(upper, lower);
    m = vorrq_u8(m, uscore);
    m = vorrq_u8(m, colon);

    return m;
}

static INLINE const uint8_t* find_first_not_namechar_ascii_neon(
    const uint8_t* p,
    const uint8_t* end) noexcept
{
    const size_t n = (size_t)(end - p);
    size_t i = 0;

    for (; i + 16 <= n; i += 16)
    {
        const uint8x16_t v = vld1q_u8(p + i);
        const uint8x16_t allowed = neon_is_namechar_ascii_mask(v);
        const uint8x16_t not_allowed = vmvnq_u8(allowed);

        const int lane = neon_first_true_lane_u8(not_allowed);
        if (lane < 16)
            return p + i + (size_t)lane;
    }

    return find_first_not_namechar_ascii_scalar(p + i, end);
}

static INLINE const uint8_t* find_first_not_namestart_ascii_neon(
    const uint8_t* p,
    const uint8_t* end) noexcept
{
    const size_t n = (size_t)(end - p);
    size_t i = 0;

    for (; i + 16 <= n; i += 16)
    {
        const uint8x16_t v = vld1q_u8(p + i);
        const uint8x16_t allowed = neon_is_namestart_ascii_mask(v);
        const uint8x16_t not_allowed = vmvnq_u8(allowed);

        const int lane = neon_first_true_lane_u8(not_allowed);
        if (lane < 16)
            return p + i + (size_t)lane;
    }

    return find_first_not_namestart_ascii_scalar(p + i, end);
}

#endif

// ------------------------------------------------------------
// Dispatch wrappers
// ------------------------------------------------------------

static INLINE const uint8_t* find_first_not_namechar_ascii(
    const uint8_t* p,
    const uint8_t* end) noexcept
{
#if defined(__ARM_NEON) || defined(__aarch64__)
    return find_first_not_namechar_ascii_neon(p, end);
#else
    return find_first_not_namechar_ascii_scalar(p, end);
#endif
}

static INLINE const uint8_t* find_first_not_namestart_ascii(
    const uint8_t* p,
    const uint8_t* end) noexcept
{
#if defined(__ARM_NEON) || defined(__aarch64__)
    return find_first_not_namestart_ascii_neon(p, end);
#else
    return find_first_not_namestart_ascii_scalar(p, end);
#endif
}