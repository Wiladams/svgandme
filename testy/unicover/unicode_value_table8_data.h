// unicode_value_table8_data.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "unicode_coverage_data.h"


namespace waavs
{
    // ========================================================================
    // UnicodeValueTable8 reference encoding
    //
    // Value tables reuse the existing Unicode coverage geometry:
    //
    //      sub-page       1024 code points
    //      master page   32768 code points
    //      32 sub-pages per master
    //      34 master regions
    //
    //
    // Both master-page and value-page references occupy independent 16-bit
    // address spaces.
    //
    // References are divided into:
    //
    //      0x0000 .. 0xFEFF     actual pool index
    //      0xFF00 .. 0xFFFF     uniform value 0 .. 255
    //
    //
    // A uniform reference directly represents a region in which every code
    // point has the same uint8_t value.
    //
    // For example:
    //
    //      0xFF00     uniform value 0
    //      0xFF01     uniform value 1
    //      ...
    //      0xFFFF     uniform value 255
    //
    //
    // Master-page references and value-page references use the same encoding,
    // but refer to independent pools.
    //
    // ========================================================================

    using UnicodeValueMasterPage8Ref = uint16_t;
    using UnicodeValuePage8Ref = uint16_t;


    static constexpr uint16_t kUnicodeValue8UniformBase = 0xFF00u;

    static constexpr uint32_t kUnicodeValue8MaxPoolPages =
        static_cast<uint32_t>(kUnicodeValue8UniformBase);


    // ========================================================================
    // Unicode Value8 reference helpers
    // ========================================================================

    [[nodiscard]]
    static constexpr bool unicodeValue8RefIsUniform(uint16_t ref) noexcept
    {
        return ref >= kUnicodeValue8UniformBase;
    }


    [[nodiscard]]
    static constexpr uint8_t unicodeValue8RefUniformValue(uint16_t ref) noexcept
    {
        return static_cast<uint8_t>(
            ref -
            kUnicodeValue8UniformBase);
    }


    [[nodiscard]]
    static constexpr uint16_t unicodeValue8UniformRef(uint8_t value) noexcept
    {
        return static_cast<uint16_t>(
            kUnicodeValue8UniformBase +
            static_cast<uint16_t>(value));
    }


    // ========================================================================
    // UnicodeValuePage8
    //
    // Represents one 1024-code-point VALUE page.
    //
    // Unlike UnicodeBitPage, every code point contains a complete uint8_t
    // value:
    //
    //      1024 x uint8_t
    //      = 1024 bytes
    //
    // Uniform pages are never required to be stored physically.  They can be
    // represented directly by UnicodeValuePage8Ref.
    //
    // ========================================================================

    struct UnicodeValuePage8
    {
        uint8_t values[kUnicodeSubSize];
    };


    // ========================================================================
    // UnicodeValueMasterPage8
    //
    // Represents 32768 Unicode code points.
    //
    // Each entry references:
    //
    //      - a UnicodeValuePage8 in the shared page pool
    //
    //          or
    //
    //      - an inline uniform uint8_t value
    //
    //
    // A completely uniform master region does not require this structure at
    // all.  UnicodeValueTable8Data can represent that master region directly
    // using a uniform UnicodeValueMasterPage8Ref.
    //
    // No summary masks are required.  Value tables perform direct point
    // lookup rather than set intersection / containment operations.
    //
    // ========================================================================

    struct UnicodeValueMasterPage8
    {
        UnicodeValuePage8Ref sub[kUnicodeSubsPerMaster];
    };


    // ========================================================================
    // UnicodeValueTable8Data
    //
    // Canonical persistent representation of one:
    //
    //      Unicode code point -> uint8_t
    //
    // mapping.
    //
    // Each master reference either:
    //
    //      - indexes the shared UnicodeValueMasterPage8 pool
    //
    //          or
    //
    //      - directly represents a uniform uint8_t value
    //
    //
    // This structure contains no pointers or ownership state and is suitable
    // for direct interpretation from validated database memory.
    //
    // ========================================================================

    struct UnicodeValueTable8Data
    {
        UnicodeValueMasterPage8Ref masters[kUnicodeMasterCount];
    };


    // ========================================================================
    // Geometry / reference checks
    // ========================================================================

    static_assert(
        kUnicodeSubsPerMaster == 32,
        "Unicode value tables require exactly 32 sub-pages per master page");

    static_assert(
        kUnicodeMasterCount == 34,
        "Unicode value tables require exactly 34 master regions");


    static_assert(
        sizeof(UnicodeValueMasterPage8Ref) == 2,
        "UnicodeValueMasterPage8Ref must be exactly 16 bits");

    static_assert(
        sizeof(UnicodeValuePage8Ref) == 2,
        "UnicodeValuePage8Ref must be exactly 16 bits");


    static_assert(
        kUnicodeValue8MaxPoolPages == 0xFF00u,
        "Unicode VALUE8 pool geometry requires 0xFF00 physical references");

    static_assert(
        unicodeValue8RefUniformValue(
            unicodeValue8UniformRef(0)) == 0,
        "Unicode VALUE8 uniform reference encoding is invalid");

    static_assert(
        unicodeValue8RefUniformValue(
            unicodeValue8UniformRef(255)) == 255,
        "Unicode VALUE8 uniform reference encoding is invalid");


    // ========================================================================
    // ABI checks
    // ========================================================================

    static_assert(
        sizeof(UnicodeValuePage8) == 1024,
        "UnicodeValuePage8 must be exactly 1024 bytes");


    static_assert(
        sizeof(UnicodeValueMasterPage8) == 64,
        "UnicodeValueMasterPage8 must be exactly 64 bytes");


    static_assert(
        sizeof(UnicodeValueTable8Data) == 68,
        "UnicodeValueTable8Data must be exactly 68 bytes");


    static_assert(
        std::is_trivially_copyable<UnicodeValuePage8>::value,
        "UnicodeValuePage8 must be trivially copyable");

    static_assert(
        std::is_trivially_copyable<UnicodeValueMasterPage8>::value,
        "UnicodeValueMasterPage8 must be trivially copyable");

    static_assert(
        std::is_trivially_copyable<UnicodeValueTable8Data>::value,
        "UnicodeValueTable8Data must be trivially copyable");


    static_assert(
        std::is_standard_layout<UnicodeValuePage8>::value,
        "UnicodeValuePage8 must have standard layout");

    static_assert(
        std::is_standard_layout<UnicodeValueMasterPage8>::value,
        "UnicodeValueMasterPage8 must have standard layout");

    static_assert(
        std::is_standard_layout<UnicodeValueTable8Data>::value,
        "UnicodeValueTable8Data must have standard layout");

} // namespace waavs
