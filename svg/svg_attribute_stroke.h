#pragma once

#include "svgstructuretypes.h"

namespace waavs
{

    //=========================================================
    // SVGStrokeWidth
    //=========================================================

    struct SVGStrokeWidth : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::stroke_width(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGStrokeWidth>(nullptr);
                node->loadFromAttributes(attrs, groot);
                return node;
                });
        }

        double fWidth{ 1.0 };

        SVGStrokeWidth(IAmGroot* iMap)
            : SVGVisualProperty(iMap)
        {
            setName(svgattr::stroke_width());
        }

        SVGStrokeWidth(const SVGStrokeWidth& other) = delete;
        SVGStrokeWidth& operator=(const SVGStrokeWidth& rhs) = delete;


        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            ByteSpan s = inChunk;
            if (!number_read(s, fWidth))
                return false;

            set(true);

            return true;
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            (void)groot;

            ctx->strokeWidth(fWidth);
        }
    };

    //=========================================================
    ///  SVGStrokeMiterLimit
    /// A visual property to set the miter limit for a stroke
    //=========================================================
    struct SVGStrokeMiterLimit : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::stroke_miterlimit(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGStrokeMiterLimit>(nullptr); 
                node->loadFromAttributes(attrs, groot);  
                return node; });
        }


        double fMiterLimit{ 4.0 };

        SVGStrokeMiterLimit(IAmGroot* iMap) : SVGVisualProperty(iMap) { setName(svgattr::stroke_miterlimit()); }

        SVGStrokeMiterLimit(const SVGStrokeMiterLimit& other) = delete;
        SVGStrokeMiterLimit& operator=(const SVGStrokeMiterLimit& rhs) = delete;



        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            ByteSpan s = inChunk;

            if (!number_read(s, fMiterLimit))
                return false;

            fMiterLimit = clamp(fMiterLimit, 1.0, 10.0);

            set(true);
            setNeedsBinding(false);

            return true;
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            ctx->strokeMiterLimit(fMiterLimit);
        }
    };

    //=========================================================
    // SVGStrokeLineCap
    //=========================================================


    struct SVGStrokeLineCap : public SVGVisualProperty
    {
        static void registerFactory()
        {
            registerSVGAttributeByName(svgattr::stroke_linecap(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {auto node = std::make_shared<SVGStrokeLineCap>(nullptr, svgattr::stroke_linecap()); node->loadFromAttributes(attrs, groot);  return node; });
            registerSVGAttributeByName(svgattr::stroke_linecap_start(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {auto node = std::make_shared<SVGStrokeLineCap>(nullptr, svgattr::stroke_linecap_start()); node->loadFromAttributes(attrs, groot);  return node; });
            registerSVGAttributeByName(svgattr::stroke_linecap_end(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {auto node = std::make_shared<SVGStrokeLineCap>(nullptr, svgattr::stroke_linecap_end()); node->loadFromAttributes(attrs, groot);  return node; });
        }


        BLStrokeCap fLineCap{ BL_STROKE_CAP_BUTT };
        BLStrokeCapPosition fLineCapPosition{};
        bool fBothCaps{ true };

        SVGStrokeLineCap(IAmGroot* iMap, InternedKey key) : SVGVisualProperty(iMap)
        {
            setName(key);

            if (key == svgattr::stroke_linecap())
                fBothCaps = true;
            else if (key == svgattr::stroke_linecap_start())
            {
                fBothCaps = false;
                fLineCapPosition = BL_STROKE_CAP_POSITION_START;
            }
            else if (key == svgattr::stroke_linecap_end())
            {
                fBothCaps = false;
                fLineCapPosition = BL_STROKE_CAP_POSITION_END;
            }
        }

        SVGStrokeLineCap(const SVGStrokeLineCap& other) = delete;
        SVGStrokeLineCap& operator=(const SVGStrokeLineCap& rhs) = delete;



        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            bool success = getEnumValue(SVGLineCaps, inChunk, (uint32_t&)fLineCap);
            set(success);
            setNeedsBinding(false);

            return success;
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            if (fBothCaps) {
                ctx->strokeCaps(fLineCap);
            }
            else {
                ctx->strokeCap(fLineCap, fLineCapPosition);
            }

        }
    };

    //=========================================================
    // SVGStrokeLineJoin
    // A visual property to set the line join for a stroke
    //=========================================================
    struct SVGStrokeLineJoin : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::stroke_linejoin(), [](const XmlAttributeCollection& attrs, IAmGroot* groot) {
                auto node = std::make_shared<SVGStrokeLineJoin>(nullptr); 
                node->loadFromAttributes(attrs, groot);  
                return node; });
        }

        BLStrokeJoin fLineJoin{ BL_STROKE_JOIN_MITER_BEVEL };

        SVGStrokeLineJoin(IAmGroot* iMap) : SVGVisualProperty(iMap) { setName(svgattr::stroke_linejoin()); }
        SVGStrokeLineJoin(const SVGStrokeLineJoin& other) = delete;
        SVGStrokeLineJoin& operator=(const SVGStrokeLineJoin& rhs) = delete;

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            bool success = getEnumValue(SVGLineJoin, inChunk, (uint32_t&)fLineJoin);
            set(success);
            setNeedsBinding(false);

            return success;
        }

        void applySelfToContext(IDrawGraphics* ctx, IAmGroot* groot) override
        {
            ctx->lineJoin(fLineJoin);
        }
    };
}
