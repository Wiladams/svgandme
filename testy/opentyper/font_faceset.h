// font_faceset.h
//
// Lightweight passive collection of FontFace objects.
//
// FontFaceSet:
//   - owns/references a collection of FontFace handles
//   - implements IProvideFontFaces
//   - knows nothing about files, directories, containers, or filtering
//
// Higher-level operations such as FontFilter::gatherFrom() can return a
// FontFaceSet.

#pragma once

#include "font_interfaces.h"
#include "font_face.h"

#include <cstddef>
#include <utility>
#include <vector>


namespace waavs
{
    struct FontFaceSet
    {
    private:
        std::vector<FontFace> fFaces;

    public:
        FontFaceSet() = default;


        // ================================================================
        // Add a single face
        // ================================================================

        void addFontFace(const FontFace& face)
        {
            if (!face.isValid())
                return;

            fFaces.push_back(face);
        }


        void addFontFace(FontFace&& face)
        {
            if (!face.isValid())
                return;

            fFaces.push_back(std::move(face));
        }


        // ================================================================
        // Add all faces exposed by another provider.
        //
        // This preserves the composability of IProvideFontFaces.
        //
        // Sources can include:
        //
        //     FontMonger
        //     another FontFaceSet
        //     OpenTypeContainer
        //     future font providers
        // ================================================================
        /*
        template<typename Source>
        void addFontFaces(Source& source)
        {
            FontFace face;

            while (source(face))
            {
                if (face.isValid())
                    addFontFace(std::move(face));
            }
        }

        */

        // ================================================================
        // Convenience collection access
        // ================================================================

        WG_NODISCARD
        bool empty() const noexcept
        {
            return fFaces.empty();
        }


        WG_NODISCARD
        size_t size() const noexcept
        {
            return fFaces.size();
        }


        void clear() noexcept
        {
            fFaces.clear();
        }


        void reserve(size_t count)
        {
            fFaces.reserve(count);
        }


        WG_NODISCARD
        FontFace front() const noexcept
        {
            if (fFaces.empty())
                return FontFace{};

            return fFaces.front();
        }


        WG_NODISCARD
        FontFace back() const noexcept
        {
            if (fFaces.empty())
                return FontFace{};

            return fFaces.back();
        }


        // ================================================================
        // Optional direct indexed access.
        //
        // fontFace(index) remains the interface-level operation.
        // operator[] is useful when the caller already has a FontFaceSet.
        // ================================================================

        WG_NODISCARD
        FontFace operator[](size_t index) const noexcept
        {
            return fFaces[index];
        }


        // ================================================================
        // Iteration
        //
        // These make FontFaceSet pleasant to use directly:
        //
        //     for (const auto& face : faces)
        //     {
        //         ...
        //     }
        //
        // They do not become part of IProvideFontFaces.
        // ================================================================

        auto begin() noexcept
        {
            return fFaces.begin();
        }

        auto end() noexcept
        {
            return fFaces.end();
        }

        auto begin() const noexcept
        {
            return fFaces.begin();
        }

        auto end() const noexcept
        {
            return fFaces.end();
        }

        auto cbegin() const noexcept
        {
            return fFaces.cbegin();
        }

        auto cend() const noexcept
        {
            return fFaces.cend();
        }
    };

} // namespace waavs

