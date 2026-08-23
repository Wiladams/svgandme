#pragma once
#include <cassert>

#include "unicode_coverage.h"

using namespace waavs;

inline void dumpCoverageStats(
    const UnicodeCoverage& coverage)
{
    const auto s = coverage.stats();

    const double bytesPerCodePoint =
        s.coveredCodePoints != 0
        ? double(s.totalBytes) /
        double(s.coveredCodePoints)
        : 0.0;

    const size_t expectedPages =
        s.masterPages + s.bitmapPages;

    std::printf(
        "UnicodeCoverage Stats\n"
        "---------------------\n"
        "Covered codepoints  : %zu\n"
        "Master planes used  : %zu / %u\n"
        "Full master planes  : %zu\n"
        "Master pages        : %zu\n"
        "Full sub-planes     : %zu\n"
        "Bitmap pages        : %zu\n"
        "Allocated pages     : %zu\n"
        "Expected pages      : %zu\n"
        "Packed page bytes   : %zu\n"
        "Master table bytes  : %zu\n"
        "Total bytes         : %zu\n"
        "Bytes/codepoint     : %.3f\n",
        s.coveredCodePoints,
        s.masterPlanesUsed,
        UnicodeCoverage::kMasterCount,
        s.fullMasterPlanes,
        s.masterPages,
        s.fullSubPlanes,
        s.bitmapPages,
        s.allocatedPages,
        expectedPages,
        s.packedPageBytes,
        s.masterTableBytes,
        s.totalBytes,
        bytesPerCodePoint);
}

inline void testUnicodeCoverage()
{
    {
        UnicodeCoverageBuilder b;

        b.add('A');
        b.add('Z');
        b.add(0x0627);
        b.add(0x1F600);

        UnicodeCoverage c = b.finalize();
        dumpCoverageStats(c);

        assert(c.contains('A'));
        assert(c.contains('Z'));
        assert(c.contains(0x0627));
        assert(c.contains(0x1F600));

        assert(!c.contains('B'));
        assert(!c.contains(0x110000));
    }

    {
        UnicodeCoverageBuilder b;

        b.addRange(0x20, 0x7E);

        UnicodeCoverage c = b.finalize();

        for (uint32_t cp = 0x20; cp <= 0x7E; ++cp)
            assert(c.contains(cp));

        assert(!c.contains(0x1F));
        assert(!c.contains(0x7F));
    }

    {
        // Exactly one complete sub-plane.
        UnicodeCoverageBuilder b;

        b.addRange(0x400, 0x7FF);

        UnicodeCoverage c = b.finalize();

        assert(c.contains(0x400));
        assert(c.contains(0x7FF));
        assert(!c.contains(0x3FF));
        assert(!c.contains(0x800));
    }

    {
        // Exactly one complete master-plane.
        UnicodeCoverageBuilder b;

        b.addRange(0x0000, 0x7FFF);

        UnicodeCoverage c = b.finalize();

        assert(c.contains(0x0000));
        assert(c.contains(0x7FFF));
        assert(!c.contains(0x8000));

        // A full master needs no actual page.
        assert(c.pageCount() == 0);
    }

    {
        UnicodeCoverageBuilder fontBuilder;
        fontBuilder.addRange(0x20, 0xFF);
        fontBuilder.addRange(0x600, 0x6FF);

        UnicodeCoverage font =
            fontBuilder.finalize();

        UnicodeCoverageBuilder queryBuilder;
        queryBuilder.add('A');
        queryBuilder.add(0x0627);

        UnicodeCoverage query =
            queryBuilder.finalize();

        assert(font.containsAll(query));
        assert(font.intersects(query));
    }

    {
        UnicodeCoverageBuilder latinBuilder;
        latinBuilder.addRange(0x20, 0xFF);

        UnicodeCoverageBuilder arabicBuilder;
        arabicBuilder.addRange(0x600, 0x6FF);

        UnicodeCoverage latin =
            latinBuilder.finalize();

        UnicodeCoverage arabic =
            arabicBuilder.finalize();

        assert(!latin.intersects(arabic));
        assert(!latin.containsAll(arabic));
    }

    {
        // End of Unicode space.
        UnicodeCoverageBuilder b;

        b.addRange(0x10FF00, 0x10FFFF);

        UnicodeCoverage c =
            b.finalize();

        assert(c.contains(0x10FF00));
        assert(c.contains(0x10FFFF));
        assert(!c.contains(0x110000));
    }
}

int main()
{
    testUnicodeCoverage();
    return 0;
}