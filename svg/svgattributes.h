// svgattributes.h
#pragma once

#include <memory>
#include <vector>
#include <cstdint>		// uint8_t, etc
#include <cstddef>		// nullptr_t, ptrdiff_t, size_t
#include <list>

#include "maths.h"
#include "xmlscan.h"

#include "svgstructuretypes.h"
#include "svgcolors.h"
#include "twobitpu.h"


namespace waavs
{

    template <typename StorageT>
    struct PaintOrderProgram
    {
    public:
        using TBPU = TwoBitProcessingUnit<StorageT>;

        enum class Strictness {
            StrictSVG,
            Permissive
        };

        PaintOrderProgram() = default;
        explicit PaintOrderProgram(StorageT code) noexcept : fTPU(code) {}

        const TBPU& unit() const noexcept { return fTPU; }
        StorageT code() const noexcept { return fTPU.code(); }

        // SVG Paint Order Instructions builder
        // https://www.w3.org/TR/SVG2/render.html#PaintOrderProperty
        // Build up a paint order program from an SVG paint-order attribute span
        static PaintOrderProgram buildSVG(const ByteSpan& bs) noexcept
        {
            PaintOrderProgram prog;
            TBPU& tpu = prog.fTPU;

            // Default program is fill, stroke, markers
            // so, create that code if the attribute is 'normal' or empty
            if (!bs || bs == "normal")
            {
                // fill, stroke, markers
                tpu.emit(TBPU::OP_1, 0);    // Fill
                tpu.emit(TBPU::OP_2, 1);    // Stroke
                tpu.emit(TBPU::OP_3, 2);    // Markers
                tpu.terminate(3);

                return prog;
            }

            uint32_t usedMask = 0;
            uint32_t slot = 0;

            // A helper to ensure we only emit unique ops
            auto emit_unique_op = [&](uint32_t op) noexcept {
                if (op < TBPU::OP_1 || op > TBPU::OP_3) return;
                if (slot >= 3) return;

                const uint32_t bit = 1u << (op - 1u); // op=1..3 -> bit 0..2
                if (usedMask & bit) return;

                usedMask |= bit;
                tpu.emit(op, slot++);
                };

            // 1) parse the user provided tokens
            // (strict: max 3 unique recognized tokens
            ByteSpan p = bs;
            while (p && slot < 3)
            {
                ByteSpan tok = chunk_token(p, chrWspChars);
                if (tok.empty())
                    break;

                uint32_t kind = 0;
                if (getEnumValue(SVGPaintOrderEnum, tok, kind))
                {
                    emit_unique_op(kind);
                }

                // unknown tokens ignored
            }

            // 2) append missing in canonical order
            emit_unique_op(TBPU::OP_1);     // SVG_PAINT_ORDER_FILL
            emit_unique_op(TBPU::OP_2);     // SVG_PAINT_ORDER_STROKE
            emit_unique_op(TBPU::OP_3);     // SVG_PAINT_ORDER_MARKERS

            tpu.terminate(slot);
            return prog;
        }

        // Execute the paint order program on the given context
        template <class Renderer>
        void run(Renderer& r) const noexcept
        {
            struct Executor
            {
                Renderer& r;

                void execute(uint32_t op) const noexcept
                {
                    switch (op) {
                    case TBPU::OP_1: r.onFill();  break;
                    case TBPU::OP_2: r.onStroke(); break;
                    case TBPU::OP_3: r.onMarkers(); break;
                    default: break;
                    }
                }
            };

            Executor exec{ r };
            fTPU.run(exec);
        }

    private:
        TBPU fTPU;
    };
}


namespace waavs {
    //
    // SVGPatternExtendMode
    // 
    // A structure that represents the extend mode of a pattern.
    struct SVGPatternExtendMode : public SVGVisualProperty 
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::extendMode(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGPatternExtendMode>(nullptr);
                node->loadFromAttributes(attrs);
                return node;
                });
        }
        
        uint32_t fExtendMode{ BL_EXTEND_MODE_REPEAT };      // repeat by default

        SVGPatternExtendMode(IAmGroot* groot) : SVGVisualProperty(groot) 
        {
            setName(svgattr::extendMode());
            //setAutoDraw(false);
        }

        BLExtendMode value() const { return static_cast<BLExtendMode>(fExtendMode); }
        
        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {   
            bool success = getEnumValue(SVGExtendMode, inChunk, fExtendMode);
            set(success);
            setNeedsBinding(false);
            
            return success;
        }

    };
}




// Specific types of attributes
namespace waavs {

    // SVGOpacity
    // 
    // https://svgwg.org/svg2-draft/render.html#ObjectAndGroupOpacityProperties
    // Opacity is an attribute that causes a sub-tree to be rendered into
    // a buffer, then a global opacity is applied to that buffer as it's being
    // composited into the final output.
    // So, this property triggers the offscreen drawing.  We retain the actual
    // opacity value so that when the compositing occurs, the value can be 
    // applied to the context at that point.
    // 
    //
    struct SVGOpacity : public SVGVisualProperty 
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::opacity(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGOpacity>();
            node->loadFromAttributes(attrs);
            return node;
                });
        }
        
        double fValue{1};
        
        SVGOpacity() : SVGVisualProperty(nullptr) { setName(svgattr::opacity()); }        
        
        //void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        //{
        //    ctx->globalOpacity(fValue);
        //}

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!inChunk)
                return false;
            
            SVGNumberOrPercent op{};
            ByteSpan s = inChunk;

            if (!readSVGNumberOrPercent(s, op))
                return false;

            fValue = waavs::clamp(op.calculatedValue(), 0.0, 1.0);

            set(true);
            setNeedsBinding(false);
            
            return true;
        }
        

    };
    
    struct SVGFillOpacity : public SVGOpacity
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::fill_opacity(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGFillOpacity>(); 
                node->loadFromAttributes(attrs);
                return node;
                });
            
        }
        
        SVGFillOpacity()  :SVGOpacity()  { setName(svgattr::fill_opacity());}
        
        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->fillOpacity(fValue);
        }

    };

    struct SVGStrokeOpacity : public SVGOpacity
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::stroke_opacity(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGStrokeOpacity>(); 
                node->loadFromAttributes(attrs);
                return node; 
                });

        }

        SVGStrokeOpacity() :SVGOpacity() { setName(svgattr::stroke_opacity()); }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->strokeOpacity(fValue);
        }

    };

}



namespace waavs {
    struct SVGPaintOrderAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::paint_order(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGPaintOrderAttribute>(nullptr); 
                node->loadFromAttributes(attrs);
                return node; 
                });
        }

        ByteSpan fValue{};
        uint8_t fInstruct = PaintOrderKind::SVG_PAINT_ORDER_NORMAL;
        
        SVGPaintOrderAttribute(IAmGroot* iMap) :SVGVisualProperty(iMap) 
        {
            setName(svgattr::paint_order());
        }

        uint32_t instructions() const noexcept { return fInstruct; }



        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!inChunk)
                return false;
            
            fValue = inChunk;
            PaintOrderProgram<uint8_t> prog = PaintOrderProgram<uint8_t>::buildSVG(inChunk);
            fInstruct = prog.code();

            set(true);
            setNeedsBinding(false);
            
            return true;
        }
        
        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->setPaintOrder(fInstruct);
        }

    };
}



//==================================================================
//  SVG Text Properties
//==================================================================
// Typography Attributes
namespace waavs {
    struct SVGTextAnchorAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::text_anchor(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGTextAnchorAttribute>();
                node->loadFromAttributes(attrs);
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
            bool success = getEnumValue(SVGTextAnchor, inChunk, (uint32_t &)fValue);
            set(success);
            setNeedsBinding(false);
            
            return success;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->setTextAnchor(fValue);
        }

    };
}

namespace waavs {
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
            registerSVGAttribute(svgattr::font_size(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGFontSize>(nullptr); 
                node->loadFromAttributes(attrs);
                return node; 
                });
        }
        
        
        SVGVariableSize dimValue{};
        double fValue{ 16.0 };

        SVGFontSize(IAmGroot* inMap) 
            : SVGVisualProperty(inMap) 
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

        virtual void bindToContext(IRenderSVG* ctx, IAmGroot* groot) noexcept override
        {
            if (nullptr == groot)
                return;
            double dpi = groot ? groot->dpi() : 96.0;

            if (dimValue.isSet() && ctx != nullptr)
            {
                double fsize = ctx->getFontSize();
                fValue = dimValue.calculatePixels(ctx->getFont(), fsize, 0, dpi);
            }

            setNeedsBinding(false);
        }
        
        void applySelfToContext(IRenderSVG* ctx, IAmGroot*) override
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
            registerSVGAttribute(svgattr::font_family(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGFontFamily>(nullptr); 
                node->loadFromAttributes(attrs);
                return node; 
                });
        }
        


        ByteSpan fValue{};

        SVGFontFamily(IAmGroot* inMap) : SVGVisualProperty(inMap) { setName(svgattr::font_family()); }

        SVGFontFamily& operator=(const SVGFontFamily& rhs) = delete;


        const ByteSpan& value() const { return fValue; }
        


        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!inChunk)
                return false;
            
            fValue = inChunk;
            set(true);
            setNeedsBinding(false);
            
            return true;
        }
        
        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
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
            registerSVGAttribute(svgattr::font_style(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGFontStyleAttribute>();
                node->loadFromAttributes(attrs);
                return node;
                });
        }
        
        
        BLFontStyle fValue{ BL_FONT_STYLE_NORMAL };

        SVGFontStyleAttribute() :SVGVisualProperty(nullptr) 
        { 
            setName(svgattr::font_style());  
            
            set(false); 
        }
        
        int value() const {return fValue;}
        
        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            bool success = getEnumValue(SVGFontStyle, inChunk, (uint32_t &)fValue);
            set(success);
            setNeedsBinding(false);
            
            return success;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->fontStyle(fValue);
        }
    };
    
    struct SVGFontWeightAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttributeByName("font-weight", [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGFontWeightAttribute>();
                node->loadFromAttributes(attrs);
                return node;
                });
        }
        
        BLFontWeight fWeight{ BL_FONT_WEIGHT_NORMAL };
        
        SVGFontWeightAttribute() :SVGVisualProperty(nullptr) { setName(svgattr::font_weight()); }

        BLFontWeight value() const { return fWeight; }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            bool success = getEnumValue(SVGFontWeight, inChunk, (uint32_t &)fWeight);
            set(success);
            setNeedsBinding(false);
            
            return success;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->fontWeight(value());
        }
    };

    struct SVGFontStretchAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::font_stretch(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGFontStretchAttribute>();
                node->loadFromAttributes(attrs);
                return node;
                });
        }
        
        BLFontStretch fValue{ BL_FONT_STRETCH_NORMAL };

        SVGFontStretchAttribute() :SVGVisualProperty(nullptr) { setName(svgattr::font_stretch()); }

        BLFontStretch value() const { return fValue; }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            bool success = getEnumValue(SVGFontStretch, inChunk, (uint32_t &)fValue);
            set(success);
            setNeedsBinding(false);
            
            return success;

        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->fontStretch(value());
        }
        
    };
    
}



namespace waavs 
{

    //=========================================================
    // SVGFillRule
    //=========================================================

    struct SVGFillRuleAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::fill_rule(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGFillRuleAttribute>(nullptr);
                node->loadFromAttributes(attrs);
                return node;
                });
        }


        BLFillRule fValue{ BL_FILL_RULE_EVEN_ODD };

        SVGFillRuleAttribute(IAmGroot* iMap) : SVGVisualProperty(iMap) { setName(svgattr::fill_rule()); }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            bool success = getEnumValue(SVGFillRule, inChunk, (uint32_t &)fValue);
            set(success);
            setNeedsBinding(false);
            
            return success;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->fillRule(fValue);
        }
    };
}





namespace waavs {
    



    //=========================================================
    // SVGStrokeWidth
    //=========================================================

    struct SVGStrokeWidth : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::stroke_width(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGStrokeWidth>(nullptr); 
                node->loadFromAttributes(attrs);  
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
            if (!readNumber(s, fWidth))
                return false;
            
            set(true);
            
            return true;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
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
            registerSVGAttribute(svgattr::stroke_miterlimit(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGStrokeMiterLimit>(nullptr); node->loadFromAttributes(attrs);  return node; });
        }


        double fMiterLimit{ 4.0 };

        SVGStrokeMiterLimit(IAmGroot* iMap) : SVGVisualProperty(iMap) { setName(svgattr::stroke_miterlimit()); }

        SVGStrokeMiterLimit(const SVGStrokeMiterLimit& other) = delete;
        SVGStrokeMiterLimit& operator=(const SVGStrokeMiterLimit& rhs) = delete;



        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            ByteSpan s = inChunk;
            
            if (!readNumber(s, fMiterLimit))
                return false;
            
            fMiterLimit = clamp(fMiterLimit, 1.0, 10.0);

            set(true);
            setNeedsBinding(false);

            return true;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
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
            registerSVGAttributeByName(svgattr::stroke_linecap(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGStrokeLineCap>(nullptr, svgattr::stroke_linecap()); node->loadFromAttributes(attrs);  return node; });
            registerSVGAttributeByName(svgattr::stroke_linecap_start(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGStrokeLineCap>(nullptr, svgattr::stroke_linecap_start()); node->loadFromAttributes(attrs);  return node; });
            registerSVGAttributeByName(svgattr::stroke_linecap_end(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGStrokeLineCap>(nullptr, svgattr::stroke_linecap_end()); node->loadFromAttributes(attrs);  return node; });
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
            bool success = getEnumValue(SVGLineCaps, inChunk, (uint32_t &)fLineCap);
            set(success);
            setNeedsBinding(false);
            
            return success;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
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
            registerSVGAttribute(svgattr::stroke_linejoin(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGStrokeLineJoin>(nullptr); node->loadFromAttributes(attrs);  return node; });
        }

        BLStrokeJoin fLineJoin{ BL_STROKE_JOIN_MITER_BEVEL };

        SVGStrokeLineJoin(IAmGroot* iMap) : SVGVisualProperty(iMap) { setName(svgattr::stroke_linejoin()); }
        SVGStrokeLineJoin(const SVGStrokeLineJoin& other) = delete;
        SVGStrokeLineJoin& operator=(const SVGStrokeLineJoin& rhs) = delete;

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            bool success = getEnumValue(SVGLineJoin, inChunk, (uint32_t &)fLineJoin);
            set(success);
            setNeedsBinding(false);
            
            return success;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->lineJoin(fLineJoin);
        }
    };
}



namespace waavs {

    //================================================
    // SVGTransform
    // Transformation matrix
    //================================================
    struct SVGTransform : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::transform(), [](const XmlAttributeCollection& attrs)
                {auto node = std::make_shared<SVGTransform>(); node->loadFromAttributes(attrs);  return node; });
        }

        WGMatrix3x3 fMatrix = WGMatrix3x3::makeIdentity() ;

        SVGTransform() : SVGVisualProperty(nullptr)
        {
            setName(svgattr::transform());
            setAutoDraw(false);
        }
        SVGTransform(const SVGTransform& other) = delete;
        SVGTransform& operator=(const SVGTransform& rhs) = delete;



        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!parseTransform(inChunk, fMatrix)) {
                printf("Failed to parse transform: '%.*s'\n", (int)inChunk.size(), inChunk.data());
                return false;
            }

            set(true);
            setNeedsBinding(false);

            return true;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            //if ((fMatrix.type() != BLTransformType::BL_TRANSFORM_TYPE_INVALID) &&
            //    (fMatrix.type() != BLTransformType::BL_TRANSFORM_TYPE_IDENTITY))
                ctx->applyTransform(fMatrix);
        }
    };
}

namespace waavs {
    //======================================================
    // SVGViewbox
    // A document may or may not have this property
    //======================================================
    /*
    struct SVGViewbox : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttributeByName("viewBox", [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGViewbox>(nullptr); node->loadFromAttributes(attrs);  return node; });

        }

        
        WGRect fRect{};
        
        SVGViewbox() :SVGVisualProperty(nullptr) { id("viewBox"); }
        SVGViewbox(IAmGroot* iMap) :SVGVisualProperty(iMap) {}
        
        SVGViewbox(const SVGViewbox& other) = delete;
        SVGViewbox& operator=(const SVGViewbox& rhs) = delete;

        // This will translate relative to the current x, y position
        void translateBy(double dx, double dy)
        {
            fRect.x += dx;
            fRect.y += dy;
        }

        // This does a scale relative to a given point
        // It will also do the translation at the same time
        void scaleBy(double sx, double sy, double centerx, double centery)
        {            
            fRect.x = centerx + (fRect.x - centerx) * sx;
            fRect.y = centery + (fRect.y - centery) * sy;
            fRect.w *= sx;
            fRect.h *= sy;
        }

        double x() const { return fRect.x; }
        double y() const { return fRect.y; }
        double width() const { return fRect.w; }
        double height() const { return fRect.h; }


        
        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!parseViewBox(inChunk, fRect))
                return false;

            set(true);
            
            return true;
        }

        // BUGBUG - need a bindSelfToContext()
    };
    */
}





namespace waavs {

}

//
// Markers
//
namespace waavs {
    struct SVGMarkerAttribute : public SVGVisualProperty
    {
        
        static void registerMarkerFactory() {
            registerSVGAttribute(svgattr::marker(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGMarkerAttribute>(svgattr::marker()); node->loadFromAttributes(attrs);  return node; });
            registerSVGAttribute(svgattr::marker_start(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGMarkerAttribute>(svgattr::marker_start()); node->loadFromAttributes(attrs);  return node; });
            registerSVGAttribute(svgattr::marker_mid(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGMarkerAttribute>(svgattr::marker_mid()); node->loadFromAttributes(attrs);  return node; });
            registerSVGAttribute(svgattr::marker_end(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGMarkerAttribute>(svgattr::marker_end()); node->loadFromAttributes(attrs);  return node; });
        }

        std::shared_ptr<IViewable> fWrappedNode = nullptr;

        
        SVGMarkerAttribute(const InternedKey key) : SVGVisualProperty(nullptr) 
        { 
            setName(key); 
        }
        
        std::shared_ptr<IViewable> markerNode(IRenderSVG* ctx, IAmGroot* groot)
        {
            if (fWrappedNode == nullptr)
            {
                bindToContext(ctx, groot);
            }
            return fWrappedNode;
        }
        
        void bindToContext(IRenderSVG *ctx, IAmGroot* groot) noexcept override
        {
            
            if (bspan_starts_with(rawValue(), "url("))
            {
                fWrappedNode = groot->findNodeByUrl(rawValue());

                if (fWrappedNode != nullptr)
                {
                    fWrappedNode->bindToContext(ctx, groot);
                    set(true);
                }
            }
            setNeedsBinding(false);
        }
        
        bool loadSelfFromChunk(const ByteSpan& ) override
        {
            setAutoDraw(false); // we mark it as invisible, because we don't want it drawing when attributes are drawn
            // we only want it to draw when we're drawing during polyline/polygon drawing
            setNeedsBinding(true);
            
            return true;
        }
        
        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            if (fWrappedNode)
                fWrappedNode->draw(ctx, groot);
        }
    };
}

namespace waavs {
    //======================================================
    // SVGClipAttribute
    // This is the attribute that can be connected to some
    // shape that's being drawn.  Whatever is doing the drawing
    // should call getVariant() to retrieve the BLImage which
    // represents the clipPath 
    //======================================================
    struct SVGClipPathAttribute : public SVGVisualProperty
    {
        static void registerFactory()
        {
            registerSVGAttribute(svgattr::clip_path(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGClipPathAttribute>(nullptr);
                node->loadFromAttributes(attrs);

                return node;
                });

        }


        std::shared_ptr<IViewable> fClipNode{ nullptr };
        BLVar fClipVar;
        
        
        SVGClipPathAttribute(IAmGroot* groot) : SVGVisualProperty(groot) { setName(svgattr::clip_path()); }

        
        bool loadFromUrl(IRenderSVG *ctx, IAmGroot* groot, const ByteSpan& inChunk)
        {
            if (nullptr == groot)
                return false;

            fClipNode = groot->findNodeByUrl(inChunk);

            if (fClipNode == nullptr) {
                set(false);
                return false;
            }

            // pull out the color value
            // BUGBUG - this will not always be the case
            // as what we point to might be a gradient or pattern
            if (fClipNode->needsBinding())
                fClipNode->bindToContext(ctx, groot);

            set(true);

            return true;
        }

        
        // Let's get a connection to our referenced thing
        void bindToContext(IRenderSVG *ctx, IAmGroot* groot) noexcept override
        {
            ByteSpan str = rawValue();

            // Just load the referenced node at this point
            
            if (bspan_starts_with(str, "url("))
            {
                loadFromUrl(ctx, groot, str);
            }

            setNeedsBinding(false);
        }
        
        bool loadSelfFromChunk(const ByteSpan& ) override
        {
            setNeedsBinding(true);
            set(true);
            
            return true;
        }
    };
}
    
namespace waavs {
    
    struct SVGVectorEffectAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::vector_effect(), [](const XmlAttributeCollection& attrs) {auto node = std::make_shared<SVGVectorEffectAttribute>(nullptr); node->loadFromAttributes(attrs);  return node; });
        }

        VectorEffectKind fEffectKind{ VECTOR_EFFECT_NONE };

        
        SVGVectorEffectAttribute(IAmGroot* groot) : SVGVisualProperty(groot) { setName(svgattr::vector_effect()); }
        
        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            bool success = getEnumValue(SVGVectorEffect, inChunk, (uint32_t &)fEffectKind);
            set(success);
            setNeedsBinding(false);
            
            return success;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            if (VECTOR_EFFECT_NON_SCALING_STROKE == fEffectKind)
            {
                ctx->strokeBeforeTransform(true);
            }
        }
    };
}


// Dash array/offset attributes
namespace waavs 
{
    //=========================================================
     // SVGStrokeDashArray
     // stroke-dasharray: none | <length-percentage>#
     //=========================================================
    struct SVGStrokeDashArray : public SVGVisualProperty
    {
        static void registerFactory()
        {
            registerSVGAttribute(svgattr::stroke_dasharray(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGStrokeDashArray>();
                node->loadFromAttributes(attrs);
                return node;
                });
        }

        std::vector<float> fArray{};
        double fOffset{ 0.0 };
        bool fIsNone{ true };

        SVGStrokeDashArray() : SVGVisualProperty(nullptr)
        {
            setName(svgattr::stroke_dasharray());
        }

        bool loadFromAttributes(const XmlAttributeCollection& attrs) override
        {
            // Get the dash array and offset if they exist
            ByteSpan dashArrayAttr{};
            attrs.getValue(svgattr::stroke_dasharray(), dashArrayAttr);
            
            // Get the dash-offset if it exists
            ByteSpan dashOffsetAttr{};
            attrs.getValue(svgattr::stroke_dashoffset(), dashOffsetAttr);
            
            // if the two attributes are empty, then no dash pattern
            // was specified.  So we can just keep the attribute
            // not set and return.
            // It's not exactly the same as 'none', because it won't alter
            // the state of the drawing context, so whatever state dashing was
            // in previously will remain
            if (!dashArrayAttr && !dashOffsetAttr)
                return set(false);
            

            // parse the dash array value
            if (!parseStrokeDashArray(dashArrayAttr, fArray, fIsNone))
                return set(false);

            // get the offset value, if it exists, otherwise default to 0
            if (!parseNumber(dashOffsetAttr, fOffset))
                fOffset = 0.0;

            set(true);
            setNeedsBinding(false);

            return true;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            (void)groot;

            if (!ctx)
                return;

            // If we're not set, don't do anything
            if (!isSet())
                return;

            // If it was explicitly set to 'none', then we need
            // to clear any dash pattern on the context, 
            // which is different than just not setting it at all
            if (fIsNone)
            {
                // No dash pattern.
                ctx->clearStrokeDashArray();
                ctx->clearStrokeDashOffset();

                return;
            }

            // Store raw dash segments in state.
            ctx->dashArray(fArray);
            ctx->dashOffset(fOffset);
        }
    };

/*
//=========================================================
// SVGStrokeDashOffset
// stroke-dashoffset: <length-percentage>
//=========================================================
    struct SVGStrokeDashOffset : public SVGVisualProperty
    {
        static void registerFactory()
        {
            registerSVGAttribute(svgattr::stroke_dashoffset(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGStrokeDashOffset>(nullptr);
                node->loadFromAttributes(attrs);
                return node;
                });
        }

        float fOffset;
        bool fHasOffset{ false };

        SVGStrokeDashOffset(IAmGroot* groot) : SVGVisualProperty(groot)
        {
            setName(svgattr::stroke_dashoffset());
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            double offset = 0.0;
            if (parseNumber(inChunk, offset))
            {
                fHasOffset = true;
                fOffset = static_cast<float>(offset);
            }

            // Even if empty/not set, we still consider the property "set"
            // if the attribute existed; drawSelf() will clear the state.
            set(true);
            setNeedsBinding(false);

            return true;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            if (ctx == nullptr)
                return;

            if (!fHasOffset)
            {
                ctx->clearStrokeDashOffset();
                return;
            }

            ctx->dashOffset(fOffset);
        }
    };
    */
} // namespace waavs

