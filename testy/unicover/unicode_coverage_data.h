#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace waavs
{
    // ========================================================================
    // Coverage geometry
    // ========================================================================

    static constexpr uint32_t kUnicodeLimit = 0x110000u;

    static constexpr uint32_t kUnicodeSubShift = 10u;
    static constexpr uint32_t kUnicodeSubSize =
        1u << kUnicodeSubShift;

    static constexpr uint32_t kUnicodeMasterShift = 15u;
    static constexpr uint32_t kUnicodeMasterSize =
        1u << kUnicodeMasterShift;

    static constexpr uint32_t kUnicodeSubsPerMaster =
        kUnicodeMasterSize / kUnicodeSubSize;

    static constexpr uint32_t kUnicodeMasterCount =
        kUnicodeLimit / kUnicodeMasterSize;


    // ========================================================================
    // References
    //
    // Master-page references and bit-page references occupy independent
    // address spaces.
    //
    //      0x0000 .. 0xFFFD     actual pool index
    //      0xFFFE               completely full
    //      0xFFFF               completely empty
    //
    // ========================================================================

    using UnicodeMasterPageRef = uint16_t;
    using UnicodeBitPageRef = uint16_t;

    static constexpr uint16_t kUnicodePageFull = 0xFFFEu;
    static constexpr uint16_t kUnicodePageEmpty = 0xFFFFu;

    static constexpr uint32_t kUnicodeMaxPoolPages = static_cast<uint32_t>(kUnicodePageFull);


    // ========================================================================
    // UnicodeBitPage
    //
    // Represents coverage of 1024 Unicode code points.
    //
    //      16 x uint64_t
    //      = 1024 bits
    //      = 128 bytes
    //
    // ========================================================================

    struct UnicodeBitPage
    {
        uint64_t bits[16];
    };


    // ========================================================================
    // UnicodeMasterPage
    //
    // Represents 32768 Unicode code points.
    //
    // Each master page contains 32 references into the shared bit-page pool.
    //
    // nonEmptyMask:
    //      bit N indicates that sub-plane N contains at least one code point.
    //
    // fullMask:
    //      bit N indicates that sub-plane N is completely covered.
    //
    // reserved[] ensures a deterministic 128-byte representation suitable
    // for hashing, deduplication, and persistent storage.
    //
    // ========================================================================

    struct UnicodeMasterPage
    {
        UnicodeBitPageRef sub[kUnicodeSubsPerMaster];

        uint32_t nonEmptyMask;
        uint32_t fullMask;

        uint8_t reserved[56];
    };


    // ========================================================================
    // UnicodeCoverageData
    //
    // Canonical persistent representation of a Unicode coverage set.
    //
    // Each entry references the shared master-page pool or contains one of
    // the kUnicodePageFull / kUnicodePageEmpty sentinel values.
    //
    // This structure contains no pointers or ownership state.
    //
    // ========================================================================

    struct UnicodeCoverageData
    {
        UnicodeMasterPageRef masters[kUnicodeMasterCount];
    };


    // ========================================================================
    // Geometry / ABI checks
    // ========================================================================

    static_assert(
        kUnicodeSubsPerMaster == 32,
        "Unicode coverage requires exactly 32 sub-planes per master plane");

    static_assert(
        kUnicodeMasterCount == 34,
        "Unicode coverage requires exactly 34 master planes");

    static_assert(
        kUnicodeMasterCount* kUnicodeMasterSize == kUnicodeLimit,
        "Unicode master-plane geometry must exactly cover the Unicode code-point space");


    static_assert(
        sizeof(UnicodeMasterPageRef) == 2,
        "UnicodeMasterPageRef must be exactly 16 bits");

    static_assert(
        sizeof(UnicodeBitPageRef) == 2,
        "UnicodeBitPageRef must be exactly 16 bits");


    static_assert(
        sizeof(UnicodeBitPage) == 128,
        "UnicodeBitPage must be exactly 128 bytes");

    //static_assert(
    //    alignof(UnicodeBitPage) == 64,
    //    "UnicodeBitPage must be aligned to 64 bytes");


    static_assert(
        sizeof(UnicodeMasterPage) == 128,
        "UnicodeMasterPage must be exactly 128 bytes");

    //static_assert(
    //    alignof(UnicodeMasterPage) == 64,
    //    "UnicodeMasterPage must be aligned to 64 bytes");

    static_assert(
        offsetof(UnicodeMasterPage, nonEmptyMask) == 64,
        "UnicodeMasterPage nonEmptyMask must begin at byte offset 64");

    static_assert(
        offsetof(UnicodeMasterPage, fullMask) == 68,
        "UnicodeMasterPage fullMask must begin at byte offset 68");

    static_assert(
        offsetof(UnicodeMasterPage, reserved) == 72,
        "UnicodeMasterPage reserved area must begin at byte offset 72");


    static_assert(
        sizeof(UnicodeCoverageData) == 68,
        "UnicodeCoverageData must be exactly 68 bytes");


    static_assert(
        std::is_trivially_copyable<UnicodeBitPage>::value,
        "UnicodeBitPage must be trivially copyable");

    static_assert(
        std::is_trivially_copyable<UnicodeMasterPage>::value,
        "UnicodeMasterPage must be trivially copyable");

    static_assert(
        std::is_trivially_copyable<UnicodeCoverageData>::value,
        "UnicodeCoverageData must be trivially copyable");


    static_assert(
        std::is_standard_layout<UnicodeBitPage>::value,
        "UnicodeBitPage must have standard layout");

    static_assert(
        std::is_standard_layout<UnicodeMasterPage>::value,
        "UnicodeMasterPage must have standard layout");

    static_assert(
        std::is_standard_layout<UnicodeCoverageData>::value,
        "UnicodeCoverageData must have standard layout");

} // namespace waavs