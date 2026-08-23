#pragma once


#include "definitions.h"
#include "lang_span.h"
#include "lang_memory.h"
#include "unicode_coverage.h"


#include <memory>

namespace waavs 
{
    struct Font;
    struct FontFace;
    struct FontMonger;

    using Tag = uint32_t;
    using Offset = uint32_t;

    using FontName = const char*;

    enum class FontSlant : uint8_t
    {
        Upright = 0,
        Italic = 1,
        Oblique = 2
    };


    struct FontFaceProperties
    {
        uint16_t weight{ 400 }; // normal = 400
        uint16_t width{ 100 };  // normal = 100
    
        FontSlant slant{ FontSlant::Upright };
    };


    struct TableRecord {
        Tag tag{ 0 };              // numeric tag for fast comparison
        uint32_t checksum{ 0 };
        Offset offset{ 0 };
        uint32_t length{ 0 };
        ByteSpan data;                // cached view of table data
    };




    struct IProvideFontFaceData
    {
        virtual ~IProvideFontFaceData() = default;

        // Where the font data came from.  Typically a file location
        virtual FontName sourceLocation() const noexcept = 0;

        // All names returned through IProvideFontFaceData are interned
        // through WSNameSet and therefore have stable lifetime and canonical
        // pointer identity.
        virtual FontName familyName() const noexcept = 0;
        virtual FontName subfamilyName() const noexcept = 0;
        virtual FontName fullName() const noexcept = 0;
        virtual FontName postScriptName() const noexcept = 0;

        virtual FontFaceProperties properties() const noexcept = 0;

        virtual uint32_t glyphCount() const noexcept = 0;
        virtual uint16_t unitsPerEm() const noexcept = 0;

        virtual uint32_t glyphIndex(uint32_t codepoint) const noexcept = 0;

        virtual const UnicodeCoverage& unicodeCoverage() const noexcept = 0;
        virtual bool supportsCodepoint( uint32_t codepoint) const noexcept
        {
            return unicodeCoverage().contains(codepoint);
        }

    };



    
    struct IProvideFontFaces
    {
        virtual ~IProvideFontFaces() = default;

        virtual size_t fontFaceCount() const noexcept = 0;
        virtual FontFace fontFace(size_t index) const noexcept = 0;
    };
    
}
