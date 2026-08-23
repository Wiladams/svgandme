#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace waavs
{
    struct CoverageStats
    {
        size_t masterPlanesUsed = 0;
        size_t fullMasterPlanes = 0;
        size_t masterPages = 0;
        size_t fullSubPlanes = 0;
        size_t bitmapPages = 0;
        size_t allocatedPages = 0;

        size_t packedPageBytes = 0;
        size_t masterTableBytes = 0;
        size_t totalBytes = 0;

        size_t coveredCodePoints = 0;
    };

    // ============================================================================
    // UnicodeCoverage
    //
    // Immutable, finalized Unicode coverage set.
    //
    // Unicode is divided into:
    //
    //   master-plane : 32768 code points
    //   sub-plane    :  1024 code points
    //
    // Each master-plane contains 32 sub-plane references.
    // Each concrete sub-plane is represented by a 1024-bit bitmap = 128 bytes.
    //
    // All allocated objects use one fixed 128-byte page size.
    //
    // Page references are uint16_t offsets into the page array.
    // Two reference values are reserved:
    //
    //   kEmpty : entire referenced region is empty
    //   kFull  : entire referenced region is full
    //
    // The object is immutable after construction.
    //
    // ============================================================================

    class UnicodeCoverage
    {
    public:
        using PageRef = uint16_t;

        static constexpr uint32_t kUnicodeLimit = 0x110000u;

        static constexpr uint32_t kSubShift = 10u;
        static constexpr uint32_t kSubSize = 1u << kSubShift;       // 1024

        static constexpr uint32_t kMasterShift = 15u;
        static constexpr uint32_t kMasterSize = 1u << kMasterShift; // 32768

        static constexpr uint32_t kSubsPerMaster =
            kMasterSize / kSubSize;                                 // 32

        static constexpr uint32_t kMasterCount =
            (kUnicodeLimit + kMasterSize - 1u) / kMasterSize;        // 34

        static constexpr uint32_t kPageSize = 128u;

        static constexpr PageRef kFull = 0xFFFEu;
        static constexpr PageRef kEmpty = 0xFFFFu;

    private:
        [[nodiscard]]
        static uint32_t popCount64(uint64_t value) noexcept
        {
#if defined(_MSC_VER) && defined(_M_X64)
            return static_cast<uint32_t>(
                __popcnt64(value));
#elif defined(__GNUC__) || defined(__clang__)
            return static_cast<uint32_t>(
                __builtin_popcountll(value));
#else
            uint32_t count = 0;

            while (value) {
                value &= value - 1;
                ++count;
            }

            return count;
#endif
        }

        struct alignas(16) Page
        {
            std::array<uint8_t, kPageSize> data{};
        };

        struct MasterPage
        {
            PageRef sub[kSubsPerMaster];

            // Useful summary masks.
            //
            // bit N in nonEmptyMask:
            //   sub-plane N contains at least one code point.
            //
            // bit N in fullMask:
            //   sub-plane N is completely full.
            //
            uint32_t nonEmptyMask;
            uint32_t fullMask;

            uint8_t reserved[56];
        };

        struct BitPage
        {
            uint64_t bits[16];
        };

        static_assert(sizeof(Page) == kPageSize);
        static_assert(sizeof(MasterPage) == kPageSize);
        static_assert(sizeof(BitPage) == kPageSize);

    public:
        UnicodeCoverage() noexcept
        {
            mMasters.fill(kEmpty);
        }

        UnicodeCoverage(const UnicodeCoverage&) = default;
        UnicodeCoverage(UnicodeCoverage&&) noexcept = default;

        UnicodeCoverage& operator=(const UnicodeCoverage&) = default;
        UnicodeCoverage& operator=(UnicodeCoverage&&) noexcept = default;

        [[nodiscard]]
        CoverageStats stats() const noexcept
        {
            CoverageStats s;

            s.allocatedPages = mPages.size();
            s.packedPageBytes = mPages.size() * kPageSize;
            s.masterTableBytes = sizeof(mMasters);
            s.totalBytes = s.packedPageBytes +
                s.masterTableBytes;

            for (uint32_t mi = 0; mi < kMasterCount; ++mi) {
                const PageRef masterRef = mMasters[mi];

                if (masterRef == kEmpty)
                    continue;

                ++s.masterPlanesUsed;

                if (masterRef == kFull) {
                    ++s.fullMasterPlanes;

                    // Normally a full master is 32768 codepoints.
                    // Account for the final partial Unicode master just
                    // in case the representation ever allows it.
                    const uint32_t masterStart =
                        mi << kMasterShift;

                    const uint32_t remaining =
                        kUnicodeLimit - masterStart;

                    s.coveredCodePoints +=
                        remaining < kMasterSize
                        ? remaining
                        : kMasterSize;

                    s.fullSubPlanes += kSubsPerMaster;
                    continue;
                }

                ++s.masterPages;

                const MasterPage& master =
                    getMasterPage(masterRef);

                for (uint32_t si = 0;
                    si < kSubsPerMaster;
                    ++si) {

                    const PageRef subRef =
                        master.sub[si];

                    if (subRef == kEmpty)
                        continue;

                    if (subRef == kFull) {
                        ++s.fullSubPlanes;

                        const uint32_t subStart =
                            (mi << kMasterShift) +
                            (si << kSubShift);

                        const uint32_t remaining =
                            kUnicodeLimit - subStart;

                        s.coveredCodePoints +=
                            remaining < kSubSize
                            ? remaining
                            : kSubSize;

                        continue;
                    }

                    ++s.bitmapPages;

                    const BitPage& page =
                        getBitPage(subRef);

                    for (uint32_t wi = 0; wi < 16; ++wi)
                        s.coveredCodePoints +=
                        popCount64(page.bits[wi]);
                }
            }

            return s;
        }
        [[nodiscard]]
        bool empty() const noexcept
        {
            for (PageRef ref : mMasters) {
                if (ref != kEmpty)
                    return false;
            }

            return true;
        }

        [[nodiscard]]
        bool contains(uint32_t cp) const noexcept
        {
            if (cp >= kUnicodeLimit)
                return false;

            const uint32_t masterIndex = cp >> kMasterShift;
            const PageRef masterRef = mMasters[masterIndex];

            if (masterRef == kEmpty)
                return false;

            if (masterRef == kFull)
                return true;

            const MasterPage& master = getMasterPage(masterRef);

            const uint32_t subIndex =
                (cp >> kSubShift) & (kSubsPerMaster - 1u);

            const PageRef subRef = master.sub[subIndex];

            if (subRef == kEmpty)
                return false;

            if (subRef == kFull)
                return true;

            const BitPage& bits = getBitPage(subRef);

            const uint32_t bitIndex = cp & (kSubSize - 1u);

            return
                (bits.bits[bitIndex >> 6] &
                    (uint64_t(1) << (bitIndex & 63u))) != 0;
        }

        [[nodiscard]]
        bool containsAll(const UnicodeCoverage& other) const noexcept
        {
            for (uint32_t mi = 0; mi < kMasterCount; ++mi) {
                const PageRef mineRef = mMasters[mi];
                const PageRef otherRef = other.mMasters[mi];

                if (otherRef == kEmpty)
                    continue;

                if (mineRef == kFull)
                    continue;

                if (mineRef == kEmpty)
                    return false;

                if (otherRef == kFull)
                    return false;

                const MasterPage& mine =
                    getMasterPage(mineRef);

                const MasterPage& theirs =
                    other.getMasterPage(otherRef);

                // Fast rejection:
                //
                // If "theirs" contains anything in a sub-plane in which
                // "mine" contains nothing, containment is impossible.
                if ((theirs.nonEmptyMask & ~mine.nonEmptyMask) != 0)
                    return false;

                // If "theirs" has a completely full sub-plane, mine must
                // also have that sub-plane completely full.
                if ((theirs.fullMask & ~mine.fullMask) != 0)
                    return false;

                uint32_t active =
                    theirs.nonEmptyMask & ~theirs.fullMask;

                while (active != 0) {
                    const uint32_t si = firstSetBit(active);
                    active &= active - 1u;

                    const PageRef theirSub = theirs.sub[si];
                    const PageRef mySub = mine.sub[si];

                    assert(theirSub != kEmpty);
                    assert(theirSub != kFull);

                    if (mySub == kFull)
                        continue;

                    if (mySub == kEmpty)
                        return false;

                    const BitPage& a = getBitPage(mySub);
                    const BitPage& b = other.getBitPage(theirSub);

                    for (uint32_t wi = 0; wi < 16; ++wi) {
                        if ((b.bits[wi] & ~a.bits[wi]) != 0)
                            return false;
                    }
                }
            }

            return true;
        }

        [[nodiscard]]
        bool intersects(const UnicodeCoverage& other) const noexcept
        {
            for (uint32_t mi = 0; mi < kMasterCount; ++mi) {
                const PageRef mineRef = mMasters[mi];
                const PageRef otherRef = other.mMasters[mi];

                if (mineRef == kEmpty || otherRef == kEmpty)
                    continue;

                if (mineRef == kFull || otherRef == kFull)
                    return true;

                const MasterPage& mine =
                    getMasterPage(mineRef);

                const MasterPage& theirs =
                    other.getMasterPage(otherRef);

                uint32_t active =
                    mine.nonEmptyMask & theirs.nonEmptyMask;

                if (active == 0)
                    continue;

                // Any sub-plane that is full on either side and non-empty
                // on the other side intersects immediately.
                if (((mine.fullMask & theirs.nonEmptyMask) |
                    (theirs.fullMask & mine.nonEmptyMask)) != 0)
                    return true;

                active &= ~(mine.fullMask | theirs.fullMask);

                while (active != 0) {
                    const uint32_t si = firstSetBit(active);
                    active &= active - 1u;

                    const PageRef mySub = mine.sub[si];
                    const PageRef theirSub = theirs.sub[si];

                    const BitPage& a = getBitPage(mySub);
                    const BitPage& b = other.getBitPage(theirSub);

                    for (uint32_t wi = 0; wi < 16; ++wi) {
                        if ((a.bits[wi] & b.bits[wi]) != 0)
                            return true;
                    }
                }
            }

            return false;
        }

        [[nodiscard]]
        size_t pageCount() const noexcept
        {
            return mPages.size();
        }

        [[nodiscard]]
        size_t dataSize() const noexcept
        {
            return mPages.size() * sizeof(Page);
        }

        [[nodiscard]]
        size_t totalMemorySize() const noexcept
        {
            return sizeof(mMasters) + dataSize();
        }

        [[nodiscard]]
        const PageRef* masterRefs() const noexcept
        {
            return mMasters.data();
        }

        [[nodiscard]]
        const void* pageData() const noexcept
        {
            return mPages.data();
        }

    private:
        friend class UnicodeCoverageBuilder;

        std::array<PageRef, kMasterCount> mMasters{};
        std::vector<Page> mPages;

        [[nodiscard]]
        const MasterPage& getMasterPage(PageRef ref) const noexcept
        {
            assert(ref < mPages.size());

            return *reinterpret_cast<const MasterPage*>(
                mPages[ref].data.data());
        }

        [[nodiscard]]
        const BitPage& getBitPage(PageRef ref) const noexcept
        {
            assert(ref < mPages.size());

            return *reinterpret_cast<const BitPage*>(
                mPages[ref].data.data());
        }

        [[nodiscard]]
        static uint32_t firstSetBit(uint32_t value) noexcept
        {
#if defined(_MSC_VER)
            unsigned long index;
            _BitScanForward(&index, value);
            return static_cast<uint32_t>(index);
#elif defined(__GNUC__) || defined(__clang__)
            return static_cast<uint32_t>(__builtin_ctz(value));
#else
            uint32_t index = 0;
            while ((value & 1u) == 0) {
                value >>= 1;
                ++index;
            }
            return index;
#endif
        }
    };


    // ============================================================================
    // UnicodeCoverageBuilder
    //
    // Mutable representation optimized for building coverage during cmap parsing.
    //
    // This representation intentionally does NOT use page references.
    // It uses directly addressable temporary master/sub-plane structures.
    //
    // finalize() creates a compact immutable UnicodeCoverage.
    //
    // ============================================================================

    class UnicodeCoverageBuilder
    {
    private:
        struct SubPlane
        {
            std::array<uint64_t, 16> bits{};

            [[nodiscard]]
            bool empty() const noexcept
            {
                for (uint64_t word : bits) {
                    if (word != 0)
                        return false;
                }

                return true;
            }

            [[nodiscard]]
            bool full() const noexcept
            {
                for (uint64_t word : bits) {
                    if (word != ~uint64_t(0))
                        return false;
                }

                return true;
            }
        };

        struct MasterPlane
        {
            std::array<std::unique_ptr<SubPlane>,
                UnicodeCoverage::kSubsPerMaster> sub{};
        };

    public:
        UnicodeCoverageBuilder() = default;

        UnicodeCoverageBuilder(const UnicodeCoverageBuilder&) = delete;
        UnicodeCoverageBuilder&
            operator=(const UnicodeCoverageBuilder&) = delete;

        UnicodeCoverageBuilder(UnicodeCoverageBuilder&&) noexcept = default;
        UnicodeCoverageBuilder&
            operator=(UnicodeCoverageBuilder&&) noexcept = default;

        void clear() noexcept
        {
            for (auto& master : mMasters)
                master.reset();
        }

        [[nodiscard]]
        bool empty() const noexcept
        {
            for (const auto& master : mMasters) {
                if (master)
                    return false;
            }

            return true;
        }

        void add(uint32_t cp)
        {
            if (cp >= UnicodeCoverage::kUnicodeLimit)
                return;

            const uint32_t masterIndex =
                cp >> UnicodeCoverage::kMasterShift;

            const uint32_t subIndex =
                (cp >> UnicodeCoverage::kSubShift) &
                (UnicodeCoverage::kSubsPerMaster - 1u);

            const uint32_t bitIndex =
                cp & (UnicodeCoverage::kSubSize - 1u);

            SubPlane& sub = ensureSubPlane(masterIndex, subIndex);

            sub.bits[bitIndex >> 6] |=
                uint64_t(1) << (bitIndex & 63u);
        }

        void addRange(uint32_t first, uint32_t last)
        {
            if (first > last)
                return;

            if (first >= UnicodeCoverage::kUnicodeLimit)
                return;

            if (last >= UnicodeCoverage::kUnicodeLimit)
                last = UnicodeCoverage::kUnicodeLimit - 1u;

            while (first <= last) {
                const uint32_t masterIndex =
                    first >> UnicodeCoverage::kMasterShift;

                const uint32_t subIndex =
                    (first >> UnicodeCoverage::kSubShift) &
                    (UnicodeCoverage::kSubsPerMaster - 1u);

                const uint32_t subStart =
                    first & ~(UnicodeCoverage::kSubSize - 1u);

                const uint32_t subEnd =
                    subStart + UnicodeCoverage::kSubSize - 1u;

                const uint32_t final =
                    last < subEnd ? last : subEnd;

                SubPlane& sub =
                    ensureSubPlane(masterIndex, subIndex);

                setBits(
                    sub,
                    first - subStart,
                    final - subStart);

                if (final == last)
                    break;

                first = final + 1u;
            }
        }

        [[nodiscard]]
        bool contains(uint32_t cp) const noexcept
        {
            if (cp >= UnicodeCoverage::kUnicodeLimit)
                return false;

            const uint32_t masterIndex =
                cp >> UnicodeCoverage::kMasterShift;

            const MasterPlane* master =
                mMasters[masterIndex].get();

            if (!master)
                return false;

            const uint32_t subIndex =
                (cp >> UnicodeCoverage::kSubShift) &
                (UnicodeCoverage::kSubsPerMaster - 1u);

            const SubPlane* sub =
                master->sub[subIndex].get();

            if (!sub)
                return false;

            const uint32_t bitIndex =
                cp & (UnicodeCoverage::kSubSize - 1u);

            return
                (sub->bits[bitIndex >> 6] &
                    (uint64_t(1) << (bitIndex & 63u))) != 0;
        }

        [[nodiscard]]
        UnicodeCoverage finalize() const
        {
            UnicodeCoverage result;

            // Reserve conservatively.
            //
            // At most:
            //
            //   one master page per master plane
            //   one bitmap page per sub-plane
            //
            // Full and empty planes consume no bitmap pages.
            size_t estimatedPages = 0;

            for (const auto& master : mMasters) {
                if (!master)
                    continue;

                ++estimatedPages;

                for (const auto& sub : master->sub) {
                    if (sub && !sub->empty() && !sub->full())
                        ++estimatedPages;
                }
            }

            result.mPages.reserve(estimatedPages);

            for (uint32_t mi = 0;
                mi < UnicodeCoverage::kMasterCount;
                ++mi) {

                const MasterPlane* sourceMaster =
                    mMasters[mi].get();

                if (!sourceMaster) {
                    result.mMasters[mi] =
                        UnicodeCoverage::kEmpty;
                    continue;
                }

                bool masterEmpty = true;
                bool masterFull = true;

                for (uint32_t si = 0;
                    si < UnicodeCoverage::kSubsPerMaster;
                    ++si) {

                    const SubPlane* sub =
                        sourceMaster->sub[si].get();

                    if (sub && !sub->empty())
                        masterEmpty = false;

                    if (!sub || !sub->full())
                        masterFull = false;
                }

                if (masterEmpty) {
                    result.mMasters[mi] =
                        UnicodeCoverage::kEmpty;
                    continue;
                }

                if (masterFull &&
                    masterCanBeCompletelyFull(mi)) {

                    result.mMasters[mi] =
                        UnicodeCoverage::kFull;
                    continue;
                }

                const UnicodeCoverage::PageRef masterRef =
                    appendPage(result);

                result.mMasters[mi] = masterRef;

                auto& targetMaster =
                    mutableMasterPage(result, masterRef);

                targetMaster.nonEmptyMask = 0;
                targetMaster.fullMask = 0;

                std::memset(
                    targetMaster.reserved,
                    0,
                    sizeof(targetMaster.reserved));

                for (uint32_t si = 0;
                    si < UnicodeCoverage::kSubsPerMaster;
                    ++si) {

                    const SubPlane* sourceSub =
                        sourceMaster->sub[si].get();

                    if (!sourceSub || sourceSub->empty()) {
                        targetMaster.sub[si] =
                            UnicodeCoverage::kEmpty;
                        continue;
                    }

                    targetMaster.nonEmptyMask |=
                        uint32_t(1) << si;

                    if (sourceSub->full() &&
                        subPlaneCanBeCompletelyFull(mi, si)) {

                        targetMaster.sub[si] =
                            UnicodeCoverage::kFull;

                        targetMaster.fullMask |=
                            uint32_t(1) << si;

                        continue;
                    }

                    const UnicodeCoverage::PageRef bitRef =
                        appendPage(result);

                    targetMaster.sub[si] = bitRef;

                    auto& targetBits =
                        mutableBitPage(result, bitRef);

                    std::memcpy(
                        targetBits.bits,
                        sourceSub->bits.data(),
                        sizeof(targetBits.bits));

                    // The last Unicode sub-plane contains values beyond
                    // U+10FFFF. Ensure those bits never become observable.
                    maskNonUnicodeBits(mi, si, targetBits);
                }
            }

            return result;
        }

    private:
        std::array<std::unique_ptr<MasterPlane>,
            UnicodeCoverage::kMasterCount> mMasters{};

        [[nodiscard]]
        MasterPlane& ensureMaster(uint32_t masterIndex)
        {
            auto& ptr = mMasters[masterIndex];

            if (!ptr)
                ptr = std::make_unique<MasterPlane>();

            return *ptr;
        }

        [[nodiscard]]
        SubPlane& ensureSubPlane(
            uint32_t masterIndex,
            uint32_t subIndex)
        {
            MasterPlane& master =
                ensureMaster(masterIndex);

            auto& ptr = master.sub[subIndex];

            if (!ptr)
                ptr = std::make_unique<SubPlane>();

            return *ptr;
        }

        static void setBits(
            SubPlane& sub,
            uint32_t first,
            uint32_t last) noexcept
        {
            assert(first <= last);
            assert(last < UnicodeCoverage::kSubSize);

            const uint32_t firstWord = first >> 6;
            const uint32_t lastWord = last >> 6;

            const uint32_t firstBit = first & 63u;
            const uint32_t lastBit = last & 63u;

            if (firstWord == lastWord) {
                const uint64_t lowerMask =
                    ~uint64_t(0) << firstBit;

                const uint64_t upperMask =
                    lastBit == 63u
                    ? ~uint64_t(0)
                    : ((uint64_t(1) << (lastBit + 1u)) - 1u);

                sub.bits[firstWord] |=
                    lowerMask & upperMask;

                return;
            }

            sub.bits[firstWord] |=
                ~uint64_t(0) << firstBit;

            for (uint32_t wi = firstWord + 1u;
                wi < lastWord;
                ++wi) {

                sub.bits[wi] =
                    ~uint64_t(0);
            }

            sub.bits[lastWord] |=
                lastBit == 63u
                ? ~uint64_t(0)
                : ((uint64_t(1) << (lastBit + 1u)) - 1u);
        }

        [[nodiscard]]
        static UnicodeCoverage::PageRef
            appendPage(UnicodeCoverage& result)
        {
            assert(
                result.mPages.size() <
                static_cast<size_t>(UnicodeCoverage::kFull));

            const auto ref =
                static_cast<UnicodeCoverage::PageRef>(
                    result.mPages.size());

            result.mPages.emplace_back();

            return ref;
        }

        [[nodiscard]]
        static UnicodeCoverage::MasterPage&
            mutableMasterPage(
                UnicodeCoverage& result,
                UnicodeCoverage::PageRef ref)
        {
            return *reinterpret_cast<UnicodeCoverage::MasterPage*>(
                result.mPages[ref].data.data());
        }

        [[nodiscard]]
        static UnicodeCoverage::BitPage&
            mutableBitPage(
                UnicodeCoverage& result,
                UnicodeCoverage::PageRef ref)
        {
            return *reinterpret_cast<UnicodeCoverage::BitPage*>(
                result.mPages[ref].data.data());
        }

        [[nodiscard]]
        static bool masterCanBeCompletelyFull(
            uint32_t masterIndex) noexcept
        {
            const uint32_t first =
                masterIndex << UnicodeCoverage::kMasterShift;

            const uint32_t last =
                first + UnicodeCoverage::kMasterSize - 1u;

            return last < UnicodeCoverage::kUnicodeLimit;
        }

        [[nodiscard]]
        static bool subPlaneCanBeCompletelyFull(
            uint32_t masterIndex,
            uint32_t subIndex) noexcept
        {
            const uint32_t first =
                (masterIndex << UnicodeCoverage::kMasterShift) +
                (subIndex << UnicodeCoverage::kSubShift);

            const uint32_t last =
                first + UnicodeCoverage::kSubSize - 1u;

            return last < UnicodeCoverage::kUnicodeLimit;
        }

        static void maskNonUnicodeBits(
            uint32_t masterIndex,
            uint32_t subIndex,
            UnicodeCoverage::BitPage& page) noexcept
        {
            const uint32_t subStart =
                (masterIndex << UnicodeCoverage::kMasterShift) +
                (subIndex << UnicodeCoverage::kSubShift);

            if (subStart >= UnicodeCoverage::kUnicodeLimit) {
                std::memset(&page, 0, sizeof(page));
                return;
            }

            const uint32_t validCount =
                UnicodeCoverage::kUnicodeLimit - subStart;

            if (validCount >= UnicodeCoverage::kSubSize)
                return;

            const uint32_t fullWords = validCount >> 6;
            const uint32_t remainder = validCount & 63u;

            if (remainder != 0) {
                page.bits[fullWords] &=
                    (uint64_t(1) << remainder) - 1u;

                for (uint32_t wi = fullWords + 1u;
                    wi < 16;
                    ++wi) {

                    page.bits[wi] = 0;
                }
            }
            else {
                for (uint32_t wi = fullWords;
                    wi < 16;
                    ++wi) {

                    page.bits[wi] = 0;
                }
            }
        }
    };

} // namespace waavs
