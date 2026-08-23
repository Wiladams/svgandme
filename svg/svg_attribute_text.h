#pragma once

#include "svgstructuretypes.h"


//==================================================================
//  SVG Text Properties
//==================================================================
// Typography Attributes
namespace waavs 
{
    struct SVGTextAnchorAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::text_anchor(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGTextAnchorAttribute>();
                node->loadFromAttributes(attrs, groot);
                return node;
                });
        }


        SVGAlignment fValue{ SVGAlignment::SVG_ALIGNMENT_START };

        SVGTextAnchorAttribute() :SVGVisualProperty(nullptr)
        {
            setName(svgattr::text_anchor());
        }

        int value() const { return fValue; }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            bool success = getEnumValue(SVGTextAnchor, inChunk, (uint32_t&)fValue);
            set(success);
            setNeedsBinding(false);

            return success;
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            ctx->textAnchor(fValue);
        }

    };
}
