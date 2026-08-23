#pragma once

#include "core_nametable.h"

#include "font_face.h"
#include "font_faceset.h"
#include "unicode_coverage.h"

namespace waavs
{
    // ================================================================
    // Intern external name text once, during filter construction.
    // ================================================================
    WG_NODISCARD
        static FontName internName(const char* name) noexcept
    {
        if (!name || !*name)
            return FontName{};

        return WSNameSet::INTERN(name);
    }

    // ================================================================
    // Interned name matching
    //
    // OpenTypeFaceData interns names when parsing the name table.
    //
    // The const char* overload interns the query string once when the
    // filter is constructed. Matching then becomes an interned-name
    // identity comparison rather than repeated string comparison.
    // ================================================================

    // ------------------------------------------------
    inline FontFacePredFn familyInterned(FontName name)
    {
        return [name](const FontFace& face) noexcept
            {
                return face.familyName() == name;
            };
    }
    inline FontFacePredFn familyName(const char* name)
    {
        return familyInterned(internName(name));
    }

    // ------------------------------------------------
    inline FontFacePredFn subfamilyInterned(FontName name)
    {
        return [name](const FontFace& face) noexcept
            {
                return face.subfamilyName() == name;
            };
    }
    inline FontFacePredFn subfamily(const char* name)
    {
        return subfamilyInterned(internName(name));
    }

    // ------------------------------------------------
    inline FontFacePredFn fullNameInterned(FontName name)
    {
        return [name](const FontFace& face) noexcept
            {
                return face.fullName() == name;
            };
    }

    inline FontFacePredFn fullName(const char* name)
    {
        return fullNameInterned(internName(name));
    }

    // ------------------------------------------------
    inline FontFacePredFn postScriptNameInterned(FontName name)
    {
        return [name](const FontFace& face) noexcept
            {
                return face.postScriptName() == name;
            };
    }
    inline FontFacePredFn postScriptName(const char* name)
    {
        return postScriptNameInterned(internName(name));
    }

    // ------------------------------------------------
    inline FontFacePredFn glyphCount(uint32_t count)
    {
        return [count](const FontFace& face) noexcept
            {
                return face.glyphCount() == count;
            };
    }

    inline FontFacePredFn minGlyphCount(uint32_t count)
    {
        return [count](const FontFace& face) noexcept
            {
                return face.glyphCount() >= count;
            };
    }

    inline FontFacePredFn maxGlyphCount(uint32_t count)
    {
        return [count](const FontFace& face) noexcept
            {
                return face.glyphCount() <= count;
            };
    }

    inline FontFacePredFn unitsPerEm(uint16_t value)
    {
        return [value](const FontFace& face) noexcept
            {
                return face.unitsPerEm() == value;
            };
    }

    // ------------------------------------------------

    inline FontFacePredFn weightBetween(uint16_t minWeight, uint16_t maxWeight)
    {
        return [minWeight, maxWeight](
            const FontFace& face) noexcept
            {
                return
                    face.weight() >= minWeight &&
                    face.weight() <= maxWeight;
            };
    }



    // ================================================================
    // Unicode coverage
    // ================================================================

    inline FontFacePredFn covers(uint32_t codepoint)
    {
        return [codepoint](const FontFace& face) noexcept
            {
                return
                    face.unicodeCoverage()
                    .contains(codepoint);
            };
    }


    inline FontFacePredFn covers(const UnicodeCoverage& required)
    {
        // Capture the finalized immutable coverage by value.
        //
        // This makes the filter self-contained even if the caller's
        // original UnicodeCoverage was temporary.
        return [required](const FontFace& face) noexcept
            {
                return
                    face.unicodeCoverage()
                    .containsAll(required);
            };
    }


    // ------------------------------------------------
    inline FontFacePredFn intersects(UnicodeCoverage coverage)
    {
        return [coverage = std::move(coverage)](
            const FontFace& face) noexcept
            {
                return face.unicodeCoverage()
                    .intersects(coverage);
            };
    }

    // ================================================================
    // Terminals
    // ================================================================
    // ================================================================
    // Gather all matching faces into a concrete FontFaceSet.
    //
    // This is the eager/materializing filtering operation.
    //
    // The source may be:
    //
    //     FontDirectoryView
    //     FontFaceSet
    //     OpenType container
    //     any future FontFacePredFn implementation
    //
    // FontFilter does not need to know the concrete source type.
    // ================================================================

    template<typename Source>
    FontFaceSet gatherFrom(Source source)
    {
        FontFaceSet result;

        FontFace face;

        while (source(face))
            result.addFontFace(std::move(face));

        return result;
    }

    // Sugar for inclusion in filtering pipelines.
    struct GatherOp{};

    inline constexpr GatherOp gather;

    template<typename Source>
    FontFaceSet operator|( Source source, GatherOp)
    {
        return gatherFrom(
            std::move(source));
    }

    // ---------------------------
    template<typename Source>
    FontFace findFirstIn(Source source)
    {
        FontFace face;

        if (source(face))
            return face;

        return {};
    }

    struct FirstOp{};

    inline constexpr FirstOp first;

    template<typename Source>
    FontFace operator|( Source source, FirstOp)
    {
        return findFirstIn(
            std::move(source));
    }
}
