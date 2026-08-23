// font_face.h

#pragma once

#include "font_interfaces.h"

#include <memory>
#include <cstdint>
#include <functional>

namespace waavs
{
    struct Font;


    // ========================================================================
    // FontFace
    //
    // Represents one immutable font face/design.
    //
    // A FontFace is:
    //
    //   - format independent
    //   - size independent
    //   - cheap to copy
    //   - safe to retain independently of the source/container
    //
    // The underlying implementation is supplied through
    // IProvideFontFaceData.
    //
    // Examples:
    //
    //   "Noto Sans Regular"
    //   "Noto Sans Bold"
    //   "Noto Sans Arabic Regular"
    //
    // A FontFace can subsequently be realized at a particular size
    // as a Font.
    // ========================================================================

    struct FontFace
    {
    private:
        std::shared_ptr<const IProvideFontFaceData> fData;

    public:
        // ====================================================================
        // Construction
        // ====================================================================

        FontFace() noexcept = default;

        explicit FontFace(std::shared_ptr<const IProvideFontFaceData> data) noexcept
            : fData(std::move(data))
        {
        }


        // ====================================================================
        // Validity
        // ====================================================================

        bool isValid() const noexcept
        {
            return bool(fData);
        }

        explicit operator bool() const noexcept
        {
            return isValid();
        }

        const IProvideFontFaceData& operator*() const noexcept
        {
            return *fData;
        }

        const IProvideFontFaceData* operator->() const noexcept
        {
            return fData.get();
        }

        const UnicodeCoverage& unicodeCoverage() const noexcept
        {
            static const UnicodeCoverage empty{};

            return fData
                ? fData->unicodeCoverage()
                : empty;
        }

        // ====================================================================
        // Identity
        //
        // These are interned FontName values.
        // ====================================================================

        FontName sourceLocation() const noexcept
        {
            return fData ? fData->sourceLocation() : nullptr;
        }

        FontName familyName() const noexcept
        {
            return fData
                ? fData->familyName()
                : nullptr;
        }

        FontName subfamilyName() const noexcept
        {
            return fData
                ? fData->subfamilyName()
                : nullptr;
        }

        FontName fullName() const noexcept
        {
            return fData
                ? fData->fullName()
                : nullptr;
        }

        FontName postScriptName() const noexcept
        {
            return fData
                ? fData->postScriptName()
                : nullptr;
        }


        // ====================================================================
        // Intrinsic properties
        // ====================================================================

        FontFaceProperties properties() const noexcept
        {
            return fData
                ? fData->properties()
                : FontFaceProperties{};
        }


        // Convenience accessors.
        //
        // These prevent callers from having to extract the whole property
        // structure when only one characteristic is needed.

        uint16_t weight() const noexcept
        {
            return properties().weight;
        }

        uint16_t width() const noexcept
        {
            return properties().width;
        }

        FontSlant slant() const noexcept
        {
            return properties().slant;
        }


        // ====================================================================
        // Basic font geometry
        // ====================================================================

        uint32_t glyphCount() const noexcept
        {
            return fData
                ? fData->glyphCount()
                : 0;
        }

        uint16_t unitsPerEm() const noexcept
        {
            return fData
                ? fData->unitsPerEm()
                : 0;
        }


        // ====================================================================
        // Character mapping
        // ====================================================================

        uint32_t glyphIndex( uint32_t codepoint) const noexcept
        {
            return fData
                ? fData->glyphIndex(codepoint)
                : 0;
        }

        bool hasGlyph( uint32_t codepoint) const noexcept
        {
            return glyphIndex(codepoint) != 0;
        }


        // ====================================================================
        // Face identity
        //
        // Two FontFace values are the same face when they refer to the same
        // underlying face-data object.
        // ====================================================================

        bool operator==( const FontFace& other) const noexcept
        {
            return fData == other.fData;
        }

        bool operator!=( const FontFace& other) const noexcept
        {
            return fData != other.fData;
        }


        // ====================================================================
        // Font creation
        //
        // Declaration only.
        //
        // Define this after Font is complete, most naturally in font.h.
        // ====================================================================

        Font createFont(double size) const noexcept;
    };

    // For generating font faces
    using FontFaceGenFn = std::function<bool(FontFace&)>;

    // For making a decision based on a FontFace
    using FontFacePredFn = std::function<bool(const FontFace&)>;

} // namespace waavs

