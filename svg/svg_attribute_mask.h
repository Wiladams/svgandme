#pragma once

#include "svg_attribute_reference.h"

namespace waavs
{
    struct SVGMaskAttribute : public SVGReferenceAttribute
    {
        static InternedKey attributeKey() noexcept { return svgattr::mask(); }

        static void registerFactory() { registerReferenceAttribute<SVGMaskAttribute>(); }

        SVGMaskAttribute()
            : SVGReferenceAttribute( attributeKey(), SVGReferenceSyntax::Url)
        {
        }


    };
}
