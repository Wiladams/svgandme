#pragma once

// test_font_system.cpp
//
// Exercises the new font abstraction layer:
//
//   OpenTypeContainer
//          |
//          v
//   IProvideFontFaces
//          |
//          +----> FontFace
//          |
//          v
//      FontMonger
//
// Recommended inputs:
//
//   test_font_system somefont.ttf
//   test_font_system somefont.otf
//   test_font_system collection.ttc
//
// Or several at once:
//
//   test_font_system font1.ttf font2.otf collection.ttc

#include "font_interfaces.h"
#include "font_face.h"
#include "font_monger.h"
#include "opentype_container.h"

#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cstring>

using namespace waavs;
using namespace waavs::opentype;

// ============================================================================
// Test accounting
// ============================================================================

struct TestState
{
    size_t passed{ 0 };
    size_t failed{ 0 };

    void check(
        bool condition,
        const char* message)
    {
        if (condition)
        {
            ++passed;
            std::cout
                << "  [PASS] "
                << message
                << '\n';
        }
        else
        {
            ++failed;
            std::cout
                << "  [FAIL] "
                << message
                << '\n';
        }
    }

    bool success() const noexcept
    {
        return failed == 0;
    }
};


// ============================================================================
// Utility
// ============================================================================

static const char* safeName(const char* name) noexcept
{
    return name ? name : "(null)";
}


static void separator(char ch = '=', size_t count = 72)
{
    for (size_t i = 0; i < count; ++i)
        std::cout << ch;

    std::cout << '\n';
}


// ============================================================================
// FontFace display
// ============================================================================

static void displayFontFace(
    const FontFace& face,
    size_t index)
{
    std::cout << "\n";
    std::cout << "  Face " << index << '\n';

    if (!face)
    {
        std::cout << "    [INVALID]\n";
        return;
    }

    const auto props = face.properties();

    std::cout
        << "    Family:      "
        << safeName(face.familyName())
        << '\n';

    std::cout
        << "    Subfamily:   "
        << safeName(face.subfamilyName())
        << '\n';

    std::cout
        << "    Full name:   "
        << safeName(face.fullName())
        << '\n';

    std::cout
        << "    PostScript:  "
        << safeName(face.postScriptName())
        << '\n';

    std::cout
        << "    Glyphs:      "
        << face.glyphCount()
        << '\n';

    std::cout
        << "    Units/em:    "
        << face.unitsPerEm()
        << '\n';

    std::cout
        << "    Weight:      "
        << props.weight
        << '\n';

    std::cout
        << "    Width:       "
        << props.width
        << '\n';

    std::cout
        << "    cmap samples:\n";

    struct Sample
    {
        uint32_t codepoint;
        const char* label;
    };

    static constexpr Sample samples[] =
    {
        { 0x0041, "A"       },
        { 0x0061, "a"       },
        { 0x0030, "0"       },
        { 0x00A9, "copyright" },
        { 0x20AC, "euro"    },
        { 0x0627, "Arabic alef" },
        { 0x4E00, "CJK one" },
        { 0x1F600, "emoji"   },
    };

    for (const auto& sample : samples)
    {
        uint32_t glyph =
            face.glyphIndex(sample.codepoint);

        std::cout
            << "      U+"
            << std::hex
            << std::uppercase
            << std::setfill('0')
            << std::setw(
                sample.codepoint > 0xFFFF ? 6 : 4)
            << sample.codepoint
            << std::dec
            << std::nouppercase
            << std::setfill(' ')
            << " -> "
            << glyph
            << "  "
            << sample.label
            << '\n';
    }
}


// ============================================================================
// Generic IProvideFontFaces test
//
// Important: this knows nothing about OpenTypeContainer.
// ============================================================================

static void testProvider(
    const IProvideFontFaces& provider,
    TestState& state)
{
    std::cout
        << "\n=== IProvideFontFaces ===\n";

    const size_t count =
        provider.fontFaceCount();

    std::cout
        << "  Faces: "
        << count
        << '\n';

    state.check(
        count > 0,
        "provider contains at least one face");

    for (size_t i = 0; i < count; ++i)
    {
        FontFace face =
            provider.fontFace(i);

        state.check(
            face.isValid(),
            "enumerated FontFace is valid");

        if (face)
            displayFontFace(face, i);
    }

    // Boundary condition.
    FontFace invalid =
        provider.fontFace(count);

    state.check(
        !invalid,
        "fontFace(count) rejects out-of-range index");
}


// ============================================================================
// FontFace copy/value semantics
// ============================================================================

static void testFontFaceCopies(
    const IProvideFontFaces& provider,
    TestState& state)
{
    std::cout
        << "\n=== FontFace value semantics ===\n";

    if (provider.fontFaceCount() == 0)
    {
        state.check(
            false,
            "provider has a face for copy test");
        return;
    }

    FontFace a =
        provider.fontFace(0);

    FontFace b = a;

    state.check(
        a.isValid() && b.isValid(),
        "copied FontFace remains valid");

    state.check(
        a.glyphCount() == b.glyphCount(),
        "copied FontFace preserves glyph count");

    state.check(
        a.unitsPerEm() == b.unitsPerEm(),
        "copied FontFace preserves units/em");

    // Since names are interned, copies of the same face should
    // expose exactly the same interned pointer.
    state.check(
        a.familyName() == b.familyName(),
        "family name retains interned pointer identity");

    state.check(
        a.fullName() == b.fullName(),
        "full name retains interned pointer identity");

    // Exercise cmap through both handles.
    static constexpr uint32_t codepoint = 'A';

    state.check(
        a.glyphIndex(codepoint) ==
        b.glyphIndex(codepoint),
        "copied FontFace has identical cmap behavior");
}


// ============================================================================
// Basic face sanity
// ============================================================================

static void testFontFaceSanity(
    const FontFace& face,
    TestState& state)
{
    std::cout
        << "\n=== FontFace sanity ===\n";

    state.check(
        face.isValid(),
        "FontFace is valid");

    if (!face)
        return;

    state.check(
        face.glyphCount() > 0,
        "glyph count is non-zero");

    state.check(
        face.unitsPerEm() > 0,
        "unitsPerEm is non-zero");

    state.check(
        face.familyName() != nullptr,
        "family name is available");

    state.check(
        face.fullName() != nullptr,
        "full name is available");

    // Glyph 0 is the missing-glyph convention, so this merely
    // checks that cmap can successfully resolve something common
    // for ordinary text fonts.
    //
    // Don't make this a hard universal requirement for every
    // possible specialized font.
    const uint32_t glyphA =
        face.glyphIndex('A');

    std::cout
        << "  glyphIndex('A') = "
        << glyphA
        << '\n';
}


// ============================================================================
// Container test
// ============================================================================

static void testOpenTypeContainer(
    const SharedMemBuff& data,
    TestState& state,
    FontMonger& monger)
{
    std::cout
        << "\n=== OpenTypeContainer ===\n";

    OpenTypeContainer container(data);

    state.check(
        container.isValid(),
        "OpenTypeContainer parsed successfully");

    if (!container.isValid())
        return;

    state.check(
        container.fontFaceCount() > 0,
        "OpenTypeContainer exposes FontFaces");

    // The important polymorphic test:
    //
    // Forget that this is an OpenTypeContainer and consume it only
    // through the generic interface.
    const IProvideFontFaces& provider =
        container;

    testProvider(provider, state);

    if (provider.fontFaceCount() > 0)
    {
        FontFace first =
            provider.fontFace(0);

        testFontFaceSanity(
            first,
            state);

        testFontFaceCopies(
            provider,
            state);
    }

    // FontMonger should consume the same generic interface.
    const size_t before =
        monger.fontFaceCount();

    monger.addFontFaces(provider);

    const size_t after =
        monger.fontFaceCount();

    state.check(
        after == before + provider.fontFaceCount(),
        "FontMonger accepted every face from provider");
}


// ============================================================================
// Lifetime test
//
// This is one of the most important tests for the new architecture.
//
// FontFace must retain the backing font data independently of the
// OpenTypeContainer object that originally produced it.
// ============================================================================

static FontFace acquireEscapingFace(
    const SharedMemBuff& data,
    TestState& state)
{
    OpenTypeContainer container(data);

    state.check(
        container.isValid(),
        "temporary container is valid");

    if (!container.isValid() ||
        container.fontFaceCount() == 0)
    {
        return {};
    }

    return container.fontFace(0);
}


static void testFaceLifetime( const SharedMemBuff& data, TestState& state)
{
    std::cout
        << "\n=== FontFace lifetime ===\n";

    const uint32_t initialRefs =
        data.refCount();

    std::cout
        << "  Initial source refs: "
        << initialRefs
        << '\n';

    FontFace face;

    {
        OpenTypeContainer container(data);

        state.check(
            container.isValid(),
            "temporary container is valid");

        if (!container.isValid() ||
            container.fontFaceCount() == 0)
        {
            return;
        }

        std::cout
            << "  Refs with container: "
            << data.refCount()
            << '\n';

        face =
            container.fontFace(0);

        state.check(
            face.isValid(),
            "acquired FontFace from temporary container");

        std::cout
            << "  Refs with face:      "
            << data.refCount()
            << '\n';
    }

    // The OpenTypeContainer is now gone.
    //
    // The FontFace must retain whatever OpenTypeFaceData/source
    // ownership it needs.

    std::cout
        << "  Refs after container: "
        << data.refCount()
        << '\n';

    state.check(
        face.isValid(),
        "FontFace survives destruction of source container");

    if (!face)
        return;

    state.check(
        face.familyName() != nullptr,
        "name remains available after container destruction");

    state.check(
        face.glyphCount() > 0,
        "glyph count remains available after container destruction");

    const uint32_t glyphA =
        face.glyphIndex('A');

    std::cout
        << "  glyphIndex('A'):      "
        << glyphA
        << '\n';

    // We shouldn't insist that every conceivable font has 'A',
    // but this verifies that cmap lookup can still execute after
    // the container has disappeared.
}


// ============================================================================
// FontMonger provider test
//
// FontMonger itself is an IProvideFontFaces.
// ============================================================================

static void testFontMonger(
    const FontMonger& monger,
    TestState& state)
{
    std::cout
        << "\n=== FontMonger ===\n";

    state.check(
        monger.fontFaceCount() > 0,
        "FontMonger contains faces");

    const IProvideFontFaces& provider = monger;

    state.check(
        provider.fontFaceCount() ==
        monger.fontFaceCount(),
        "FontMonger satisfies IProvideFontFaces");

    for (size_t i = 0;
        i < provider.fontFaceCount();
        ++i)
    {
        FontFace face =
            provider.fontFace(i);

        state.check(
            face.isValid(),
            "FontMonger returns valid FontFace");

        if (!face)
            continue;

        std::cout
            << "  ["
            << std::setw(3)
            << i
            << "] "
            << safeName(face.fullName())
            << "  ("
            << safeName(face.familyName())
            << ")\n";
    }

    FontFace invalid =
        provider.fontFace(
            provider.fontFaceCount());

    state.check(
        !invalid,
        "FontMonger rejects out-of-range face index");
}

static void testSharedSourceOwnership(
    const SharedMemBuff& data,
    TestState& state)
{
    std::cout
        << "\n=== Shared source ownership ===\n";

    const uint32_t before =
        data.refCount();

    {
        OpenTypeContainer container(data);

        state.check(
            container.isValid(),
            "container valid for ownership test");

        if (!container.isValid())
            return;

        const size_t count =
            container.fontFaceCount();

        std::cout
            << "  Faces:       "
            << count
            << '\n';

        std::cout
            << "  Refs before: "
            << before
            << '\n';

        std::cout
            << "  Refs loaded: "
            << data.refCount()
            << '\n';

        std::vector<FontFace> faces;
        faces.reserve(count);

        for (size_t i = 0; i < count; ++i)
        {
            FontFace face =
                container.fontFace(i);

            if (face)
                faces.push_back(face);
        }

        state.check(
            faces.size() == count,
            "all faces retained successfully");

        std::cout
            << "  Refs retained: "
            << data.refCount()
            << '\n';

        // There may be one source reference per OpenTypeFaceData,
        // depending upon your implementation. That's perfectly
        // legitimate.
        //
        // The important thing is that SharedMemBuff copies are
        // increasing references to ONE RefMemBuff, not copying
        // the font bytes.
    }

    std::cout
        << "  Refs final:  "
        << data.refCount()
        << '\n';
}

// ============================================================================
// File loading
//
// Adapt this one function to whatever SharedMemBuff file-loader you
// already use. Everything above is independent of file I/O.
// ============================================================================

#include <fstream>

static SharedMemBuff loadFontFile(const char* filename) noexcept
{
    if (!filename)
        return {};

    std::ifstream file(
        filename,
        std::ios::binary | std::ios::ate);

    if (!file)
        return {};

    const std::streamsize fileSize =
        file.tellg();

    if (fileSize <= 0)
        return {};

    file.seekg(0, std::ios::beg);

    SharedMemBuff buffer(
        static_cast<size_t>(fileSize));

    if (!buffer)
        return {};

    if (!file.read(
        reinterpret_cast<char*>(buffer.data()),
        fileSize))
    {
        return {};
    }

    return buffer;
}


// ============================================================================
// Main
// ============================================================================

int main(
    int argc,
    char** argv)
{
    if (argc < 2)
    {
        std::cerr
            << "Usage: "
            << argv[0]
            << " <font.ttf|font.otf|font.ttc> [...]\n";

        return 1;
    }

    TestState state;
    FontMonger monger;

    for (int i = 1; i < argc; ++i)
    {
        separator();

        std::cout
            << "Input: "
            << argv[i]
            << '\n';

        separator();

        SharedMemBuff data = loadFontFile(argv[i]);

        state.check( !data.empty(), "font file loaded");

        if (data.empty())
            continue;

        testOpenTypeContainer(
            data,
            state,
            monger);

        testFaceLifetime( data, state);
    }

    // FontMonger now contains the flattened faces from every
    // input resource, regardless of whether each resource was
    // a single font or a TTC.
    testFontMonger( monger, state);

    separator('=');

    std::cout
        << "RESULT\n"
        << "  Passed: "
        << state.passed
        << '\n'
        << "  Failed: "
        << state.failed
        << '\n';

    separator('=');

    return state.success()
        ? 0
        : 1;
}
