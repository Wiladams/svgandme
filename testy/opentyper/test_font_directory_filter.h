#include "font_directory_view.h"
#include "font_filter.h"
#include "font_faceset.h"
#include "font_predicates.h"




#include <cstdio>

using namespace waavs;


static bool testFontDirectoryFiltering( const char* fontDirectory)
{
    printf("========================================\n");
    printf("Font generator/filter test\n");
    printf("Directory: %s\n", fontDirectory);
    printf("========================================\n");


    // ================================================================
    // 1. Exercise FontDirectoryView directly as a generator.
    // ================================================================

    {
        FontDirectoryView source(fontDirectory);

        FontFace face;
        size_t count = 0;

        while (source(face))
        {
            if (!face.isValid())
            {
                printf("FAIL: generator returned invalid FontFace\n");
                return false;
            }

            ++count;

            printf(
                "%4zu  %-40s  weight=%u width=%u glyphs=%u\n",
                count,
                face.fullName()
                ? face.fullName()
                : "<unnamed>",
                unsigned(face.weight()),
                unsigned(face.width()),
                unsigned(face.glyphCount()));
        }

        if (count == 0)
        {
            printf("FAIL: directory produced no font faces\n");
            return false;
        }

        printf("\nGenerator produced %zu faces\n", count);
    }


    // ================================================================
    // 2. Empty FontFilter should match everything.
    //
    // This is the materialization test:
    //
    //     FontDirectoryView -> gatherFrom() -> FontFaceSet
    // ================================================================

    FontFaceSet allFaces;

    {
        FontDirectoryView source(fontDirectory);

        FontFilter all;

        allFaces =
            all.gatherFrom(
                std::move(source));

        if (allFaces.empty())
        {
            printf("FAIL: empty filter gathered no faces\n");
            return false;
        }

        printf(
            "Empty filter gathered %zu faces\n",
            allFaces.size());
    }


    // ================================================================
    // 3. Filter using a generic FontFace predicate.
    //
    // Choose something generic enough that normal font directories
    // should contain matches.
    // ================================================================

    FontFilter normalWeight;

    normalWeight.where(
        [](const FontFace& face) noexcept
        {
            return
                face.weight() >= 350 &&
                face.weight() <= 450;
        });


    FontFaceSet normalFaces;

    {
        FontDirectoryView source(fontDirectory);

        normalFaces =
            normalWeight.gatherFrom(
                std::move(source));

        printf(
            "Normal-weight filter gathered %zu faces\n",
            normalFaces.size());

        for (const FontFace& face : normalFaces)
        {
            if (face.weight() < 350 ||
                face.weight() > 450)
            {
                printf(
                    "FAIL: filter admitted weight %u\n",
                    unsigned(face.weight()));

                return false;
            }
        }
    }


    // ================================================================
    // 4. Exercise findFirstIn().
    //
    // This verifies early-consumption semantics.
    // ================================================================

    {
        FontDirectoryView source(fontDirectory);

        FontFace first =
            normalWeight.findFirstIn(
                std::move(source));

        if (!first.isValid())
        {
            printf(
                "FAIL: could not find a normal-weight font\n");

            return false;
        }

        if (first.weight() < 350 ||
            first.weight() > 450)
        {
            printf(
                "FAIL: findFirstIn() returned bad weight %u\n",
                unsigned(first.weight()));

            return false;
        }

        printf(
            "First normal-weight font: %s\n",
            first.fullName()
            ? first.fullName()
            : "<unnamed>");
    }


    // ================================================================
    // 5. Predicate composition.
    //
    // Multiple predicates within FontFilter have AND semantics.
    // ================================================================

    {
        FontFilter filter;

        filter
            .where(
                [](const FontFace& face) noexcept
                {
                    return face.glyphCount() > 100;
                })
            .where(
                [](const FontFace& face) noexcept
                {
                    return
                        face.weight() >= 300 &&
                        face.weight() <= 700;
                });


        FontDirectoryView source(fontDirectory);

        FontFaceSet result =
            filter.gatherFrom(
                std::move(source));


        for (const FontFace& face : result)
        {
            if (face.glyphCount() <= 100)
            {
                printf(
                    "FAIL: glyph-count predicate violated\n");

                return false;
            }

            if (face.weight() < 300 ||
                face.weight() > 700)
            {
                printf(
                    "FAIL: weight predicate violated\n");

                return false;
            }
        }

        printf(
            "Combined filter gathered %zu faces\n",
            result.size());
    }


    printf("\nPASS: generator/filter architecture\n");
    return true;
}






static bool testEmojiFonts( const char* fontDirectory)
{
    // ------------------------------------------------------------
    // Build an emoji coverage requirement.
    //
    // This is deliberately a broad pictographic range:
    //
    //     U+1F300 - U+1FAFF
    //
    // Using covers(required) means the font must cover the ENTIRE
    // requested range.
    //
    // That may be stricter than desired for many emoji fonts.
    // See the alternative test below using intersects().
    // ------------------------------------------------------------

    // Setup a coverage object for the range of
    // condepoints we're looking for
    UnicodeCoverageBuilder builder;
    builder.addRange(0x1F300, 0x1FAFF);
    UnicodeCoverage emojiRange = builder.finalize();


    // ------------------------------------------------------------
    // Find fonts that contain at least SOME emoji glyphs.
    //
    // intersects() is usually the more useful test here because few
    // fonts cover every codepoint in such a broad range.
    // ------------------------------------------------------------

    FontFilter emojiFilter;

    emojiFilter.where(
        [](const FontFace& face) noexcept
        {
            static constexpr uint32_t samples[] =
            {
                0x1F600, // 😀
                0x1F602, // 😂
                0x1F44D, // 👍
                0x1F680, // 🚀
                0x1F4A1, // 💡
                0x1F525, // 🔥
                0x1F389, // 🎉
                0x1F984  // 🦄
            };

            size_t matches = 0;

            for (uint32_t cp : samples)
            {
                if (face.unicodeCoverage().contains(cp))
                    ++matches;
            }

            return matches >= 4;
        });

    //FontFilter emojiFilter;
    //emojiFilter.intersects(emojiRange);

    // Start with a view of all the files
    //FontDirectoryView source(fontDirectory);
    // Gather the ones that satisfy the emoji filter
    FontFaceSet emojiFonts = emojiFilter.gatherFrom(FontDirectoryView(fontDirectory));


    printf( "Emoji-capable fonts: %zu\n",
        emojiFonts.size());

    // Print the fullName for the fonts that matched
    for (const FontFace& face : emojiFonts)
    {
        printf( "%-45s glyphs=%u\n",
            face.fullName()
            ? face.fullName()
            : "<unnamed>",
            unsigned(face.glyphCount()));
    }


    if (emojiFonts.empty())
    {
        printf(
            "FAIL: no fonts intersect emoji range\n");

        return false;
    }


    // ------------------------------------------------------------
    // Verify every gathered font actually intersects the range.
    // ------------------------------------------------------------

    for (const FontFace& face : emojiFonts)
    {
        if (!face.unicodeCoverage()
            .intersects(emojiRange))
        {
            printf(
                "FAIL: gathered font does not intersect emoji range: %s\n",
                face.fullName()
                ? face.fullName()
                : "<unnamed>");

            return false;
        }
    }


    printf("PASS: emoji filtering\n");

    return true;
}

void testEmojiFilter(const char* fontDirectory)
{
    FontDirectoryView source(fontDirectory);
    FontFilter emojiFilter;

    emojiFilter.where(
        [](const FontFace& face) noexcept
        {
            static constexpr uint32_t samples[] =
            {
                0x1F600, // 😀
                0x1F602, // 😂
                0x1F44D, // 👍
                0x1F680, // 🚀
                0x1F4A1, // 💡
                0x1F525, // 🔥
                0x1F389, // 🎉
                0x1F984  // 🦄
            };

            size_t matches = 0;

            for (uint32_t cp : samples)
            {
                if (face.unicodeCoverage().contains(cp))
                    ++matches;
            }

            return matches >= 4;
        });


    FontFace face;
    size_t matchCount = 0;

    while (source(face))
    {
        if (!face.isValid())
            continue;

        if (!emojiFilter(face))
            continue;

        ++matchCount;

        printf(
            "%-45s glyphs=%-6u source=%s\n",
            face.fullName()
            ? face.fullName()
            : "<unnamed>",
            unsigned(face.glyphCount()),
            face.sourceLocation()
            ? face.sourceLocation()
            : "<unknown>");
    }

    printf(
        "Emoji-capable fonts: %zu\n",
        matchCount);
}


static bool testEmojiFiltering(const char* fontDirectory)
{
    // ================================================================
    // Emoji coverage
    // ================================================================

    UnicodeCoverageBuilder builder;

    builder.addRange(
        0x1F300,
        0x1FAFF);

    UnicodeCoverage emojiRange =
        builder.finalize();


    // ================================================================
    // Build the pipeline from independent predicates.
    //
    // FontDirectoryView
    //     |
    //     v
    // intersects emoji range
    //     |
    //     v
    // at least 500 glyphs
    //     |
    //     v
    // weight 300-700
    //
    // Every intermediate stage remains:
    //
    //     bool(FontFace&)
    // ================================================================

    auto fonts =
        FontDirectoryView(fontDirectory)
        | intersects(emojiRange)
        | minGlyphCount(500)
        | weightBetween(300, 700);


    // ================================================================
    // Consume the resulting generator.
    // ================================================================

    FontFace face;
    size_t matchCount = 0;


    while (fonts(face))
    {
        if (!face.isValid())
        {
            printf(
                "FAIL: pipeline returned invalid FontFace\n");

            return false;
        }


        ++matchCount;


        printf(
            "%-45s weight=%-4u glyphs=%-6u source=%s\n",
            face.fullName()
            ? face.fullName()
            : "<unnamed>",
            unsigned(face.weight()),
            unsigned(face.glyphCount()),
            face.sourceLocation()
            ? face.sourceLocation()
            : "<unknown>");
    }


    printf(
        "Pipeline matches: %zu\n",
        matchCount);


    if (matchCount == 0)
    {
        printf(
            "FAIL: pipeline produced no matching fonts\n");

        return false;
    }


    // ================================================================
    // Once exhausted, the pipeline remains exhausted.
    // ================================================================

    if (fonts(face))
    {
        printf(
            "FAIL: exhausted pipeline produced another face\n");

        return false;
    }


    printf(
        "PASS: font filtering pipeline\n");

    return true;
}


int main()
{
    //testFontDirectoryFiltering("c:\\windows\\fonts\\");

    //testEmojiFonts("c:\\windows\\fonts\\");
    //testEmojiFonts("w:\\fonts\\");
    //testEmojiFilter("w:\\fonts\\");
    //testEmojiFiltering("c:\\windows\\fonts\\");
    testEmojiFiltering("w:\\fonts\\commonfonts\\");


    return 0;
}