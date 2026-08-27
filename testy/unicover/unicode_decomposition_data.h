// unicode_decomposition_data.h

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "unicode_coverage_data.h"


namespace waavs
{
    // ========================================================================
    // Canonical decomposition geometry
    //
    // Reuses the existing Unicode page geometry:
    //
    //      1024 code points per leaf page
    //        32 leaf pages per master page
    //     32768 code points per master region
    //        34 master regions
    //
    // ========================================================================

    using UnicodeDecompositionMasterPageRef = uint16_t;
    using UnicodeDecompositionPageRef = uint16_t;
    using UnicodeDecompositionRecordRef = uint16_t;


    // ========================================================================
    // Page references
    //
    // Physical page references:
    //
    //      0x0000 .. 0xFFFE
    //
    // Empty page/master:
    //
    //      0xFFFF
    //
    // Page and master references occupy independent address spaces.
    // ========================================================================

    static constexpr uint16_t kUnicodeDecompositionPageEmpty = 0xFFFFu;

    static constexpr uint32_t kUnicodeDecompositionMaxPoolPages = static_cast<uint32_t>(kUnicodeDecompositionPageEmpty);


    // ========================================================================
    // Record references
    //
    // Record references are deliberately different from page references.
    //
    //      0            no decomposition
    //
    //      1..0xFFFF    index + 1 into UnicodeDecompositionRecord[]
    //
    // This permits all-zero leaf pages to represent "no decomposition" and
    // allows them to collapse naturally to kUnicodeDecompositionPageEmpty.
    //
    // ========================================================================

    static constexpr UnicodeDecompositionRecordRef kUnicodeDecompositionRecordNone = 0;

    static constexpr uint32_t kUnicodeDecompositionMaxRecords = 0xFFFFu;


    [[nodiscard]]
    static constexpr bool unicodeDecompositionRecordRefValid(
        UnicodeDecompositionRecordRef ref) noexcept
    {
        return ref != kUnicodeDecompositionRecordNone;
    }


    [[nodiscard]]
    static constexpr uint32_t unicodeDecompositionRecordIndex(
        UnicodeDecompositionRecordRef ref) noexcept
    {
        return static_cast<uint32_t>(ref) - 1u;
    }


    [[nodiscard]]
    static constexpr UnicodeDecompositionRecordRef
        unicodeDecompositionRecordRef(uint32_t index) noexcept
    {
        return static_cast<UnicodeDecompositionRecordRef>(index + 1u);
    }


    // ========================================================================
    // UnicodeDecompositionRecord
    //
    // Unicode 17 canonical decomposition mappings stored in UnicodeData.txt
    // contain either:
    //
    //      one code point
    //
    //          or
    //
    //      two code points
    //
    // No explicit canonical mapping is longer than two.
    //
    // second == kUnicodeDecompositionSecondNone means singleton mapping.
    //
    // ========================================================================

    static constexpr uint32_t kUnicodeDecompositionSecondNone = 0xFFFFFFFFu;


    struct UnicodeDecompositionRecord
    {
        uint32_t first;
        uint32_t second;
    };


    // ========================================================================
    // UnicodeDecompositionPage
    //
    // One logical 1024-code-point leaf page.
    //
    // Each entry is either:
    //
    //      0
    //
    //          no canonical decomposition
    //
    // or:
    //
    //      UnicodeDecompositionRecordRef
    //
    //          1-based reference into UnicodeDecompositionRecord[]
    //
    // A completely empty page is represented by
    // kUnicodeDecompositionPageEmpty and is not stored physically.
    //
    // ========================================================================

    struct UnicodeDecompositionPage
    {
        UnicodeDecompositionRecordRef mapping[kUnicodeSubSize];
    };


    // ========================================================================
    // UnicodeDecompositionMasterPage
    //
    // One 32768-code-point master region.
    //
    // Each entry references one physical UnicodeDecompositionPage or contains
    // kUnicodeDecompositionPageEmpty.
    //
    // A completely empty master region can itself be represented by
    // kUnicodeDecompositionPageEmpty in UnicodeDecompositionData.
    //
    // ========================================================================

    struct UnicodeDecompositionMasterPage
    {
        UnicodeDecompositionPageRef sub[kUnicodeSubsPerMaster];
    };


    // ========================================================================
    // UnicodeDecompositionData
    //
    // Persistent root for canonical decomposition.
    //
    // Each master reference either:
    //
    //      indexes UnicodeDecompositionMasterPage[]
    //
    // or:
    //
    //      equals kUnicodeDecompositionPageEmpty
    //
    // ========================================================================

    struct UnicodeDecompositionData
    {
        UnicodeDecompositionMasterPageRef masters[kUnicodeMasterCount];
    };


    // ========================================================================
    // Helpers
    // ========================================================================

    [[nodiscard]]
    static constexpr bool unicodeDecompositionIsSingleton(
        const UnicodeDecompositionRecord& record) noexcept
    {
        return record.second == kUnicodeDecompositionSecondNone;
    }


    [[nodiscard]]
    static constexpr uint32_t unicodeDecompositionLength(
        const UnicodeDecompositionRecord& record) noexcept
    {
        return
            unicodeDecompositionIsSingleton(record)
            ? 1u
            : 2u;
    }


    // ========================================================================
    // Geometry / ABI
    // ========================================================================

    static_assert(
        kUnicodeSubsPerMaster == 32,
        "Unicode decomposition requires 32 sub-pages per master page");

    static_assert(
        kUnicodeMasterCount == 34,
        "Unicode decomposition requires 34 master regions");


    static_assert(
        sizeof(UnicodeDecompositionMasterPageRef) == 2,
        "UnicodeDecompositionMasterPageRef must be 16 bits");

    static_assert(
        sizeof(UnicodeDecompositionPageRef) == 2,
        "UnicodeDecompositionPageRef must be 16 bits");

    static_assert(
        sizeof(UnicodeDecompositionRecordRef) == 2,
        "UnicodeDecompositionRecordRef must be 16 bits");


    static_assert(
        sizeof(UnicodeDecompositionRecord) == 8,
        "UnicodeDecompositionRecord must be exactly 8 bytes");

    static_assert(
        sizeof(UnicodeDecompositionPage) == 2048,
        "UnicodeDecompositionPage must be exactly 2048 bytes");

    static_assert(
        sizeof(UnicodeDecompositionMasterPage) == 64,
        "UnicodeDecompositionMasterPage must be exactly 64 bytes");

    static_assert(
        sizeof(UnicodeDecompositionData) == 68,
        "UnicodeDecompositionData must be exactly 68 bytes");


    static_assert(
        std::is_trivially_copyable<
        UnicodeDecompositionRecord>::value,
        "UnicodeDecompositionRecord must be trivially copyable");

    static_assert(
        std::is_trivially_copyable<
        UnicodeDecompositionPage>::value,
        "UnicodeDecompositionPage must be trivially copyable");

    static_assert(
        std::is_trivially_copyable<
        UnicodeDecompositionMasterPage>::value,
        "UnicodeDecompositionMasterPage must be trivially copyable");

    static_assert(
        std::is_trivially_copyable<
        UnicodeDecompositionData>::value,
        "UnicodeDecompositionData must be trivially copyable");


    static_assert(
        std::is_standard_layout<
        UnicodeDecompositionRecord>::value,
        "UnicodeDecompositionRecord must have standard layout");

    static_assert(
        std::is_standard_layout<
        UnicodeDecompositionPage>::value,
        "UnicodeDecompositionPage must have standard layout");

    static_assert(
        std::is_standard_layout<
        UnicodeDecompositionMasterPage>::value,
        "UnicodeDecompositionMasterPage must have standard layout");

    static_assert(
        std::is_standard_layout<
        UnicodeDecompositionData>::value,
        "UnicodeDecompositionData must have standard layout");

} // namespace waavs
