// unicode_composition_data.h

#pragma once

#include <cstdint>
#include <type_traits>


namespace waavs
{
    // ========================================================================
    // UnicodeCompositionRecord
    //
    // One canonical composition mapping:
    //
    //      first + second -> composite
    //
    // Examples:
    //
    //      U+0041 + U+0300 -> U+00C0
    //      U+0041 + U+030A -> U+00C5
    //      U+00C5 + U+0301 -> U+01FA
    //
    // Persistent composition records are ordered lexicographically by:
    //
    //      first
    //      second
    //
    // allowing the runtime representation to use binary search directly over
    // the mapped database memory.
    //
    // Hangul composition is not represented here. It is handled
    // algorithmically at runtime.
    // ========================================================================

    struct UnicodeCompositionRecord
    {
        uint32_t first;
        uint32_t second;
        uint32_t composite;
    };


    static constexpr uint32_t kUnicodeCompositionNone =
        0xFFFFFFFFu;


    // ========================================================================
    // ABI checks
    // ========================================================================

    static_assert(
        sizeof(UnicodeCompositionRecord) == 12,
        "UnicodeCompositionRecord must be exactly 12 bytes");

    static_assert(
        std::is_trivially_copyable<UnicodeCompositionRecord>::value,
        "UnicodeCompositionRecord must be trivially copyable");

    static_assert(
        std::is_standard_layout<UnicodeCompositionRecord>::value,
        "UnicodeCompositionRecord must have standard layout");

} // namespace waavs
