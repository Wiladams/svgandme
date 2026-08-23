#pragma once

#include "svg_attribute_reference.h"

namespace waavs
{
    struct SVGClipPathElement;

    struct SVGClipPathAttribute : public SVGReferenceAttribute
    {
        static InternedKey attributeKey() noexcept { return svgattr::clip_path(); }

        static void registerFactory() { registerReferenceAttribute<SVGClipPathAttribute>(); }

        SVGClipPathAttribute()
            : SVGReferenceAttribute(
                svgattr::clip_path(),
                SVGReferenceSyntax::Url)
        {
        }



    protected:
        void onReferencedNodeResolved(const std::shared_ptr<IViewable>& node) noexcept override
        {
        }
    };



}
