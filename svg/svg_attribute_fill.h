#pragma once

#include "svgstructuretypes.h"

namespace waavs
{
    // BUGBUG - I don't think we actually need this class,
    // because the fill-opacity is taken care of when 
    // we construct the fill paint object.
    struct SVGFillOpacity : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::fill_opacity(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGFillOpacity>();
                node->loadFromAttributes(attrs, groot);
                return node;
                });

        }

        double fValue{ 1 };

        SVGFillOpacity() 
            :SVGVisualProperty(nullptr) {
            setName(svgattr::fill_opacity());
        }  

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!inChunk)
                return false;

            SVGNumberOrPercent op{};
            ByteSpan s = inChunk;

            if (!numberOrPercent_read(s, op))
                return false;

            fValue = waavs::clamp(op.normalizedValue(), 0.0, 1.0);

            set(true);
            setNeedsBinding(false);

            return true;
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            ctx->fillOpacity(fValue);
        }

    };

    //=========================================================
    // SVGFillRule
    //=========================================================

    struct SVGFillRuleAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::fill_rule(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGFillRuleAttribute>(nullptr);
                node->loadFromAttributes(attrs, groot);
                return node;
                });
        }


        BLFillRule fValue{ BL_FILL_RULE_EVEN_ODD };

        SVGFillRuleAttribute(IAmGroot* iMap) : SVGVisualProperty(iMap) { setName(svgattr::fill_rule()); }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            bool success = getEnumValue(SVGFillRule, inChunk, (uint32_t&)fValue);
            set(success);
            setNeedsBinding(false);

            return success;
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            ctx->fillRule(fValue);
        }
    };
}

