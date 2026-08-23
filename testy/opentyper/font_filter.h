// font_filter.h
//
// Composable font-face filtering.
//
// FontFilter is independent of FontMonger and FontFaceSet.
// It operates against the abstract font-face provider interfaces:
//
//     IProvideFontFaceData
//     IProvideFontFaces
//
// Core operations:
//
//     matches(face)       - test one face
//     findFirstIn(source) - stop at first matching face
//     gatherFrom(source)  - materialize all matching faces
//
// A lazy view can be added later without changing these semantics.

#pragma once

#include "font_face.h"
#include "font_faceset.h"
#include "core_nametable.h"

#include <cstdint>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>


namespace waavs
{

    class FontFilter
    {
    private:
        std::vector<FontFacePredFn> fPredicates;

    public:
        FontFilter() = default;

        // The font filter can also act as a predicate
        bool operator()(const FontFace& face) const
        {
            return matches(face);
        }


        // ================================================================
        // Generic predicate
        //
        // This is the escape hatch that keeps FontFilter from needing
        // built-in methods for every imaginable selection criterion.
        // ================================================================

        FontFilter& where(FontFacePredFn predicate)
        {
            fPredicates.push_back(std::move(predicate));
            return *this;
        }


        // ================================================================
        // FontFaceProperties
        //
        // Until the exact property vocabulary settles, this provides access
        // without coupling FontFilter to the details of FontFaceProperties.
        //
        // Later this can grow convenient methods such as:
        //
        //     weight()
        //     minWeight()
        //     width()
        //     italic()
        //     oblique()
        //
        // while whereProperties() remains the general mechanism.
        // ================================================================

        FontFilter& whereProperties(
            std::function<bool(const FontFaceProperties&)> predicate)
        {
            return where(
                [predicate = std::move(predicate)]
                (const FontFace& face)
                {
                    return predicate(
                        face.properties());
                });
        }


        // ================================================================
        // Filter composition
        //
        // Predicates within a FontFilter have AND semantics.
        // ================================================================

        FontFilter& andFilter(const FontFilter& other)
        {
            fPredicates.insert(
                fPredicates.end(),
                other.fPredicates.begin(),
                other.fPredicates.end());

            return *this;
        }


        static FontFilter combine(const FontFilter& a, const FontFilter& b)
        {
            FontFilter result;

            result.andFilter(a);
            result.andFilter(b);

            return result;
        }


        // ================================================================
        // Test one face
        //
        // An empty filter matches everything.
        // ================================================================

        WG_NODISCARD
        bool matches(const FontFace& face) const
        {
            for (const auto& predicate : fPredicates)
            {
                if (!predicate(face))
                    return false;
            }

            return true;
        }


        // ================================================================
        // Find the first matching face.
        //
        // This is useful when the caller does not need the entire result
        // collection. Evaluation stops immediately at the first match.
        // ================================================================

        //WG_NODISCARD
        template <typename Source>
        FontFace findFirstIn(Source src) const
        {
            FontFace face;
            while (src(face))
            {
                if (!face.isValid())
                    continue;

                if (matches(face))
                    return face;
            }

            return {};
        }




        // ================================================================
        // Gather all matching faces into a concrete FontFaceSet.
        //
        // This is the eager/materializing filtering operation.
        //
        // The source may be:
        //
        //     FontMonger
        //     FontFaceSet
        //     OpenType container
        //     any future IProvideFontFaces implementation
        //
        // FontFilter does not need to know the concrete source type.
        // ================================================================

        template<typename Source>
        FontFaceSet gatherFrom(Source source) const
        {
            FontFaceSet result;
            FontFace face;

            while (source(face))
            {
                if (!face.isValid())
                    continue;

                if (matches(face))
                    result.addFontFace(std::move(face));
            }

            return result;
        }


        // ================================================================
        // Meta Information
        // ================================================================

        WG_NODISCARD
        bool empty() const noexcept
        {
            return fPredicates.empty();
        }


        //WG_NODISCARD
        //size_t predicateCount() const noexcept
        //{
        //    return fPredicates.size();
        //}


        WG_NODISCARD
        size_t size() const noexcept
        {
            return fPredicates.size();
        }


        void clear() noexcept
        {
            fPredicates.clear();
        }


    private:


    };

    
    
    // ======================================
    // For filter pipeline support
    // ======================================

    template<typename Source, typename Predicate>
    class FontFaceFilterGen
    {
    private:
        Source fSource;
        Predicate fPredicate;

    public:
        FontFaceFilterGen(
            Source source,
            Predicate predicate)
            : fSource(std::move(source))
            , fPredicate(std::move(predicate))
        {
        }

        bool operator()(FontFace& face)
        {
            while (fSource(face))
            {
                if (fPredicate(face))
                    return true;
            }

            return false;
        }
    };


    template<typename Source>
    auto operator|(
        Source source,
        FontFacePredFn predicate)
    {
        return FontFaceFilterGen<
            Source,
            FontFacePredFn>(
                std::move(source),
                std::move(predicate));
    }


} // namespace waavs