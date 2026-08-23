#pragma once

#include "svg_attribute_reference.h"

namespace waavs
{
    struct SVGFilterAttribute : public SVGReferenceAttribute
    {
        static InternedKey attributeKey() noexcept { return svgattr::filter(); }
        static void registerFactory() { registerReferenceAttribute<SVGFilterAttribute>(); }

        SVGFilterAttribute()
            : SVGReferenceAttribute(attributeKey(), SVGReferenceSyntax::Url)
        {
        }
    };
}