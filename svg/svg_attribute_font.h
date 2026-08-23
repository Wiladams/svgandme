#pragma once

#include "svgstructuretypes.h"

namespace waavs
{
    //
    // SVGFontSize (font-size)
    //
    // This is fairly complex.  There are several categories of sizes
    // Absolute size values (FontSizeKeywordKind, SVGFontSizeKeywordEnum)
    //  xx-small
    //   x-small
    //     small
    //     medium
    //     large
    //   x-large
    //  xx-large
    // xxx-large
    // 
    // Relative size values
    //   smaller
    //   larger
    // 
    // Length values
    //   px, pt, pc, cm, mm, in, em, ex, ch, rem, vw, vh, vmin, vmax
    // 
    // Percentage values
    //   100%
    // math
    //   calc(100% - 10px)
    // 
    // Global values
    //   inherit, initial, revert, revert-layer, unset
    //
    // So, there are two steps to figuring out what the value should
    // be.  
    // 1) Figure out which of these categories of measures is being used
    // 2) Figure out what the actual value is
    // Other than the length values, we need to figure out at draw time
    // what the actual value is, because we need to know what the current value
    // is, and calculate relative to that.
    //
    struct SVGFontSize : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::font_size(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGFontSize>();
                node->loadFromAttributes(attrs, groot);
                return node;
                });
        }


        SVGVariableSize dimValue{};
        double fValue{ 16.0 };

        SVGFontSize()
            : SVGVisualProperty(nullptr)
        {
            setName(svgattr::font_size());
        }

        SVGFontSize& operator=(const SVGFontSize& rhs)
        {
            dimValue = rhs.dimValue;
            fValue = rhs.fValue;

            return *this;
        }

        double value() const { return fValue; }


        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!inChunk)
                return false;

            // BUGBUG
            // We need to check size keywords first, then 
            // decide what the value should be.
            // But, realistically, we can't do that here, only 
            // at binding time.  So, we should just preserve the raw
            // value here, and pick up later.
            dimValue.loadFromChunk(inChunk);

            if (!dimValue.isSet())
                return false;

            set(true);
            setNeedsBinding(true);

            return true;
        }

        virtual void bindToContext(IDrawGraphics* ctx, IAmGroot* groot) noexcept override
        {
            //if (nullptr == groot)
            //    return;
            double dpi = groot ? groot->dpi() : 96.0;

            if (dimValue.isSet() && ctx != nullptr)
            {
                double fsize = ctx->state().getFontSize();
                fValue = dimValue.calculatePixels(ctx->state().getFont(), fsize, 0, dpi);
            }

            setNeedsBinding(false);
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot*) override
        {
            ctx->fontSize(static_cast<float>(fValue));
        }
    };

    //========================================================
    // SVGFontFamily
    // This is a fairly complex attribute, as the family might be
    // a font family name, or it might be a class, such as 'sans-serif'
    // attribute name="font-style" type="string" default="normal"
    // BUGBUG
    struct SVGFontFamily : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::font_family(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGFontFamily>(nullptr);
                node->loadFromAttributes(attrs, groot);
                return node;
                });
        }



        ByteSpan fValue{};

        SVGFontFamily(IAmGroot* groot) 
            : SVGVisualProperty(groot) { setName(svgattr::font_family()); }

        SVGFontFamily& operator=(const SVGFontFamily& rhs) = delete;


        ByteSpan value() const { return fValue; }



        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!inChunk)
                return false;

            fValue = inChunk;
            set(true);
            setNeedsBinding(false);

            return true;
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            ctx->fontFamily(fValue);
        }
    };

    //========================================================
    // SVGFontStyle
    // attribute name="font-style" type="string" default="normal"
    //========================================================
    struct SVGFontStyleAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::font_style(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGFontStyleAttribute>();
                node->loadFromAttributes(attrs, groot);
                return node;
                });
        }


        BLFontStyle fValue{ BL_FONT_STYLE_NORMAL };

        SVGFontStyleAttribute() :SVGVisualProperty(nullptr)
        {
            setName(svgattr::font_style());

            set(false);
        }

        int value() const { return fValue; }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            bool success = getEnumValue(SVGFontStyle, inChunk, (uint32_t&)fValue);
            set(success);
            setNeedsBinding(false);

            return success;
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            ctx->fontStyle(fValue);
        }
    };

    struct SVGFontWeightAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttributeByName("font-weight", [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGFontWeightAttribute>();
                node->loadFromAttributes(attrs, groot);
                return node;
                });
        }

        BLFontWeight fWeight{ BL_FONT_WEIGHT_NORMAL };

        SVGFontWeightAttribute() :SVGVisualProperty(nullptr) { setName(svgattr::font_weight()); }

        BLFontWeight value() const { return fWeight; }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            bool success = getEnumValue(SVGFontWeight, inChunk, (uint32_t&)fWeight);
            set(success);
            setNeedsBinding(false);

            return success;
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            ctx->fontWeight(value());
        }
    };

    struct SVGFontStretchAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::font_stretch(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGFontStretchAttribute>();
                node->loadFromAttributes(attrs, groot);
                return node;
                });
        }

        BLFontStretch fValue{ BL_FONT_STRETCH_NORMAL };

        SVGFontStretchAttribute() :SVGVisualProperty(nullptr) { setName(svgattr::font_stretch()); }

        BLFontStretch value() const { return fValue; }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            bool success = getEnumValue(SVGFontStretch, inChunk, (uint32_t&)fValue);
            set(success);
            setNeedsBinding(false);

            return success;

        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            ctx->fontStretch(value());
        }

    };

}

