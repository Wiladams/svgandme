#pragma once

#pragma once

#include "font_face.h"

namespace waavs
{
    class Font
    {
    private:
        FontFace fFace;
        double fSize{ 0 };

    public:
        Font() = default;

        Font(const FontFace& face, double size) noexcept
            : fFace(face)
            , fSize(size)
        {
        }
    };


    inline Font FontFace::createFont(
        double size) const noexcept
    {
        if (!isValid() || size <= 0)
            return {};

        return Font(*this, size);
    }
}