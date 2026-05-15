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

    //
    // SVGOpacity
    // https://svgwg.org/svg2-draft/render.html#ObjectAndGroupOpacityProperties
    // Opacity, when applied to a group, should create a backing
    // store to be created.
    // We don't need to set the global opacity as part of drawing
    // this attribute.  Allow the specific geometry to do what it will 
    // with this attribute during it's own drawing.  That might include
    // simply inheriting the value.
    //
    struct SVGOpacity : public SVGVisualProperty // , public IServePaint
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::opacity(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGOpacity>(nullptr);
            node->loadFromAttributes(attrs);
            return node;
                });
        }
        
        double fValue{1};
        BLVar fOpacityVar;
        
        SVGOpacity(IAmGroot* groot) :SVGVisualProperty(groot) { setName(svgattr::opacity()); }        
        
        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->globalOpacity(fValue);
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!inChunk)
                return false;
            
            SVGNumberOrPercent op{};
            ByteSpan s = inChunk;

            if (!readSVGNumberOrPercent(s, op))
                return false;

            fValue = waavs::clamp(op.calculatedValue(), 0.0, 1.0);
            fOpacityVar = fValue;

            set(true);
            setNeedsBinding(false);
            
            return true;
        }
        

    };
    
    struct SVGFillOpacity : public SVGOpacity
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::fill_opacity(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGFillOpacity>(nullptr); 
                node->loadFromAttributes(attrs);
                return node;
                });
            
        }
        
        SVGFillOpacity(IAmGroot* iMap)  :SVGOpacity(iMap)  { setName(svgattr::fill_opacity());}
        
        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->fillOpacity(fValue);
        }

    };

    struct SVGStrokeOpacity : public SVGOpacity
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::stroke_opacity(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGStrokeOpacity>(nullptr); 
                node->loadFromAttributes(attrs);
                return node; 
                });

        }

        SVGStrokeOpacity(IAmGroot* iMap) :SVGOpacity(iMap) { setName(svgattr::stroke_opacity()); }

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




namespace waavs {
    //
    // htt://www.w3.org/TR/SVG11/feature#PaintAttribute
    // color, 
    // fill, fill-opacity,
    // fill-rule, 
    // stroke, stroke-opacity,
    // stroke-width
    // stroke-dasharray, stroke-dashoffset, 
    // stroke-linecap, stroke-linejoin, stroke-miterlimit,  
    
    // SVGColor
    // Representation of a color.  
    // Not a Paint Server, just a pure sRGBA color.  
    // The internal representation is ColorSRGB, which is what the SVG
    // spec says the authored colors are.  We use 4 float values as a matter
    // of convenience to convert to other representations.
    // 
    // The values are in the range [0..1].  
    // And the components are NOT premultiplied, so they are
    // 'straight' 
    // 
    // This can be converted to pixel values, or other color
    // representations when needed
    // When parsing a color, it might be an actual color returned
    // or it might be symbolic.  If it is an actual color, then 
    // isColor() returns true
    // Otherwise, one of the other symbolic tests will tell what it was
    // Multiple of the symbolic types might be set
    // so, you can have: isReference() == true AND isOpacity() == true
    //
	enum ColorSemantics : uint32_t
	{
        COLOR_SEMANTIC_UNKNOWN          = 0x00,     // neither opacity, nor color
                                                    // distinctly different than 'none'
        COLOR_SEMANTIC_NONE             = 0x01,     // literal 'none'
        COLOR_SEMANTIC_OPACITY          = 0x02,     // opacity specified
		COLOR_SEMANTIC_COLOR            = 0x04,     // regular color
		COLOR_SEMANTIC_INHERIT          = 0x08,     // literal 'inherit'
		COLOR_SEMANTIC_CURRENT          = 0x10,     // literal 'currentColor'
        COLOR_SEMANTIC_CONTEXT_STROKE   = 0x20,     // literal 'context-stroke'
        COLOR_SEMANTIC_CONTEXT_FILL     = 0x40,     // literal 'context-fill'
        COLOR_SEMANTIC_REFERENCE        = 0x80,     // starts-with 'url('
    };

    // A few little helpers for color semantics
    inline ColorSemantics operator|(ColorSemantics a, ColorSemantics b) noexcept
    {
        return static_cast<ColorSemantics>(
            static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline ColorSemantics operator&(ColorSemantics a, ColorSemantics b) noexcept
    {
        return static_cast<ColorSemantics>(
            static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    inline ColorSemantics& operator|=(ColorSemantics& a, ColorSemantics b) noexcept
    {
        a = a | b;
        return a;
    }


    // Parse only the color portion of a color specification
    // not concerned with a separate opacity value
    static WGResult parseColor(const ByteSpan& colorSpan, ColorSRGB& cSRGB) noexcept
    {
        static const char* rgbStr = "rgb(";
        static const char* rgbaStr = "rgba(";
        static const char* rgbStrCaps = "RGB(";
        static const char* rgbaStrCaps = "RGBA(";
        static const char* hslStr = "hsl(";
        static const char* hslaStr = "hsla(";

        float opacity = 1.0f;

        colorsrgb_reset(cSRGB, 0, 0, 0, opacity);

        ByteSpan cSpan = colorSpan;
        size_t len = 0;
        len = cSpan.size();
        if ((len >= 1) && (*cSpan == '#'))
        {
            return parse_colorsrgb_from_hex(cSpan, cSRGB);
        }
        else if (cSpan.startsWith(rgbStr) ||
            cSpan.startsWith(rgbaStr) ||
            cSpan.startsWith(rgbaStrCaps) ||
            cSpan.startsWith(rgbStrCaps))
        {
            return parse_colorsrgb_from_func(cSpan, cSRGB);
        }
        else if (cSpan.startsWith(hslStr) ||
            cSpan.startsWith(hslaStr))
        {
            // BUGBUG - need to modernize Hsl to fill
            // in ColorSRGB
            //c = parseColorHsl(str);
            return WGErrorCode::WG_ERROR_NotImplemented;
        }
        else {
            // so we can detect failure and return false from this function
            return get_color_by_name_as_srgb(cSpan, cSRGB);
        }
    }

    struct SVGColor : public SVGVisualProperty
    {
        ColorSRGB fValue{};
		ColorSemantics fSemantics{ COLOR_SEMANTIC_UNKNOWN };
        InternedKey fColorField;
        InternedKey fOpacityField;

		SVGColor(InternedKey colorField, InternedKey opacityField)
			: SVGVisualProperty(nullptr) 
            ,fColorField(colorField)
            ,fOpacityField(opacityField)
		{
			setName(colorField);
		}

        void setValue(float r, float g, float b, float a = 1.0f) noexcept
        {
            colorsrgb_reset(fValue, r, g, b, a);
            fSemantics = COLOR_SEMANTIC_COLOR;
            set(true);
            setNeedsBinding(false);
        }

		ColorSRGB value() const { return fValue; }
        
        void addSemantic(ColorSemantics sem) noexcept { fSemantics |= sem; }

        bool hasSemantic(ColorSemantics sem) const noexcept 
        { 
            return (static_cast<uint32_t>(fSemantics & sem)) != 0; 
        }
		bool isNone() const { return hasSemantic(COLOR_SEMANTIC_NONE); }
		bool isInherit() const { return hasSemantic(COLOR_SEMANTIC_INHERIT); }
		bool isCurrent() const { return hasSemantic(COLOR_SEMANTIC_CURRENT); }
		bool isContextStroke() const { return hasSemantic(COLOR_SEMANTIC_CONTEXT_STROKE); }
        bool isContextFill() const { return hasSemantic(COLOR_SEMANTIC_CONTEXT_FILL); }
        bool isColor() const { 
            // Something is a color if it is explicityly a color,
            // or if it is exclusively opacity, and nothing else.
            return hasSemantic(COLOR_SEMANTIC_COLOR) ||
                (fSemantics == COLOR_SEMANTIC_OPACITY); 
        }
        bool isOpacity() const { return hasSemantic(COLOR_SEMANTIC_OPACITY); }
        bool isReference() const { return hasSemantic(COLOR_SEMANTIC_REFERENCE); }


        // load a color from a set of attributes
        // This function will try to figure out what the color is
        // Function forms:
        //   rgb(255, 0, 0)
        //   rgba(255, 0, 0, 0.5)
        //   rgb(255 0 0 / 50%)

        //   hsl(120, 100%, 50%)
        //   hsla(120, 100%, 50%, 0.5)

        // Literals:
        //   #ffRRGGBB  - full hex for RGBA
        //   #ff0000    - full hex for RGB
        //   #FFFF      - single hex duplicated for RGBA
        //   #FFF       - single hex duplicated for RGB
        //   color name

        // Note:  When the colorField is a 'url()' reference,
        // we don't resolve the actual reference here, we just
        // retain it as the rawValue() of the attribute, and 
        // fill in the semantics to be: COLOR_SEMANTIC_REFERENCE
        // so isReference() will return true.
        // 
        // Note: 'currentColor' is a special value.
        // 1) If the colorSpan == 'currentColor' and the set of attributes 
        //  has a 'color' attribute, then substitute that value of that
        //  attribute in as the colorSpan before trying to parse the color
        // 2) If the current attribute set does not contain a 'color'
        //  attribute, then mark the semantics as: COLOR_SEMANTIC_CURRENT
        //  so isCurrent() will return true.  
        //  In that case, the paint server dealing with it will resolve 
        //  the current color at time of render, probably taking it from 
        //  the drawing context at the site of call.
        //
        bool loadFromAttributes(const XmlAttributeCollection& attrs) override
        {
            static const char* rgbStr = "rgb(";
            static const char* rgbaStr = "rgba(";
            static const char* rgbStrCaps = "RGB(";
            static const char* rgbaStrCaps = "RGBA(";
            static const char* hslStr = "hsl(";
            static const char* hslaStr = "hsla(";

            ByteSpan colorSpan{};
            ByteSpan opacitySpan{};

            // You can still have a valid color if only 
            // opacity is specified.
            attrs.getValue(fColorField, colorSpan);
            attrs.getValue(fOpacityField, opacitySpan);
            setRawValue(colorSpan);

            // only return early if neither color nor opacity is specified
            if (!colorSpan)
            {
                if (!opacitySpan)
                    return false;
            }

            // temporary values
            ColorSRGB cSRGB{};
            double opacity = 1.0;
            colorsrgb_reset(cSRGB, 0, 0, 0, opacity);


            // Capture opacity early, because even if it ends up being
            // a reference, we still might want to apply the opacity
            
            // Check the opacity first, as it is valid to just
            // specify an opacity, without a color value.  The default
            // color value might be context specific, but we'll go
            // with black for the moment.
            // 
            // If there is an opacity field, the value should be
            // between [0..1], or a percentage.
            // Include the opacity with the color, but since the 
            // color is sRGB, DO NOT PREMULTIPLY 
            // 
            if (opacitySpan)
            {
                SVGNumberOrPercent op{};
                ByteSpan s = opacitySpan;
                if (readSVGNumberOrPercent(s, op))
                {
                    opacity = clamp01f((float)op.calculatedValue());
                    fSemantics = COLOR_SEMANTIC_OPACITY;
                    set(true);
                }
            }

            // Quick success if we have a URL reference
            if (chunk_starts_with_cstr(colorSpan, "url("))
            {
                set(true);
                setNeedsBinding(true);
                fSemantics = COLOR_SEMANTIC_REFERENCE;

                return true;
            }




            // Finally, if we have a color value, we need to 
            // parse it.  
            if (colorSpan)
            {
                // Before we try to parse the color, check 
                // to see if the value is 'currentColor'.
                // If it is, then we need to check if there 
                // is a 'color' attribute in the current set of attributes.
                if (colorSpan == "currentColor")
                {
                    ByteSpan currentColorValue{};
                    if (attrs.getValue(svgattr::color(), currentColorValue))
                    {
                        colorSpan = currentColorValue;
                    }
                    else {
                        set(true);
                        fSemantics = COLOR_SEMANTIC_CURRENT;
                        return true;
                    }
                }


                ByteSpan cSpan = colorSpan;
                size_t len = 0;
                len = cSpan.size();
                if ((len >= 1) && (*cSpan == '#'))
                {
                    if (parse_colorsrgb_from_hex(cSpan, cSRGB) == WG_SUCCESS)
                    {
                        set(true);
                        fSemantics = COLOR_SEMANTIC_COLOR;
                    }
                    else {
                        return false;
                    }
                }
                else if (cSpan.startsWith(rgbStr) ||
                    cSpan.startsWith(rgbaStr) ||
                    cSpan.startsWith(rgbaStrCaps) ||
                    cSpan.startsWith(rgbStrCaps))
                {
                    if (parse_colorsrgb_from_func(cSpan, cSRGB) == WG_SUCCESS)
                    {
                        set(true);
                        fSemantics = COLOR_SEMANTIC_COLOR;
                    }
                    else {
                        return false;
                    }
                }
                else if (cSpan.startsWith(hslStr) ||
                    cSpan.startsWith(hslaStr))
                {
                    // BUGBUG - need to modernize Hsl to fill
                    // in ColorSRGB
                    //c = parseColorHsl(str);

                    set(true);
                    fSemantics = COLOR_SEMANTIC_COLOR;
                }
                else {
                    if (cSpan == svgval::none()) {
                        set(true);
                        fSemantics = COLOR_SEMANTIC_NONE;
                    }
                    else if (cSpan == "context-stroke")
                    {
                        set(true);
                        fSemantics = COLOR_SEMANTIC_CONTEXT_STROKE;
                    }
                    else if (cSpan == "context-fill")
                    {
                        set(true);
                        fSemantics = COLOR_SEMANTIC_CONTEXT_FILL;
                    }
                    else if ((cSpan == "inherit"))
                    {
                        set(true);
                        fSemantics = COLOR_SEMANTIC_INHERIT;
                    }
                    else if (cSpan == "currentColor")
                    {
                        // Take on whatever color was set in the 'color'
                        // attribute of this node, or whatever is the 
                        // currently Active color at the time of render
                        set(true);
                        fSemantics = COLOR_SEMANTIC_CURRENT;
                    }
                    else {
                        // BUGBUG - should change the call to take 
                        // cSRGB as parameter and return WGResult, 
                        // so we can detect failure and return false from this function
                        if (get_color_by_name_as_srgb(cSpan, cSRGB) == WG_SUCCESS)
                        {
                            set(true);
                            fSemantics = COLOR_SEMANTIC_COLOR;
                        }
                    }
                }
            }

            // Finally, assign the color value.  
            // Since we are not premultiplying, 
            // we apply (multiply) the opacity against the alpha
            // channel, but we don't modify the r,g,b values.
            fValue = cSRGB;
            fValue.a = clamp01f(fValue.a * opacity);

            return true;
        }
    };


    //=====================================================
    // SVG Paint
    // 
    // General base class for paint.  
    // This is essentially the "paint server".  We parse
    // what the 'color' is, and if it's a reference to something
    // that reference's 'getVariant' is called at the right time
    // otherwise, we return the appropriate color value.
    // The primary usage here is 'fill' and 'stroke' attributes.
    // 'stop-color' and filter primitives handle their own colors
    // as they just use SVGColor instead, as there is no allowance
    // for 'paint server' style
    
            // There are a couple of cases when using 'currentColor' as the
            // color value.
            // 1) It is being used on an element, where there is also a 'color' attribute
            //   In this case, whatever is in the 'color' attribute needs to become our
            //   BLVar value, and set on the context at drawing time.
            // 2) It is being used on an element, where there is no 'color' attribute
            //   In this case, it is inheritance.  The color value is determined by what is
            //   currently on the 'currentColor()' property of the context, so we should use
            //   that when it comes time to draw.

    //=====================================================
    struct SVGPaint : public SVGVisualProperty, public IServePaint
    {
        ByteSpan fPaintReference{};
        SVGColor fColor;
        InternedKey fColorKey;
        InternedKey fOpacityKey;


        SVGPaint(InternedKey colorKey, InternedKey opacityKey)
            : SVGVisualProperty(nullptr)
            , fColor(colorKey, opacityKey)
        {}

        SVGPaint(const SVGPaint& other) = delete;

        const BLVar getVariant(IRenderSVG* ctx, IAmGroot* groot) noexcept override
        {
            // if it's simply a color, then do a quick convert and return
            if (fColor.isColor())
            {
				BLRgba32 bColor{};
                bColor.value = Pixel_ARGB32_from_ColorSRGB(fColor.value());
                BLVar tmpVar{};
                tmpVar = bColor;

                return tmpVar;
            }
			else if (fColor.isNone())
			{
				return BLVar::null();
			}
            else if (fColor.isCurrent())
            {
				// if it's 'currentColor', then we need to look up 
                // the current color from the context.
                // we still need to apply our specific opacity if
                // it exists
                return ctx->getDefaultColor();
            }
            else if (fColor.isInherit())
            {
                // if it's 'inherit' treat it the same
                // as 'currentColor'
                // we'll keep it separate for now until we
                // confirm the semantic differences
                // this is a little more tricky, we want to retrieve
                // the inherited color, depends on whether we are
                // stroke or fill
                return ctx->getDefaultColor();
            }
            else if (fColor.isReference())
            {
                // we must have groot to do a lookup
				if (!groot)
					return BLVar::null();

                auto node = groot->findNodeByUrl(fColor.rawValue());

                if (nullptr == node)
                    return BLVar::null();

                // dynamic cast to IServePaint, if it fails, then we can't use it as a paint server
                auto paintServer = dynamic_cast<IServePaint*>(node.get());
                if (paintServer == nullptr)
                    return BLVar::null();
                
                // assume calling getVariant on the referant node will
                // cause itself to do binding
                BLVar tmpVar = paintServer->getVariant(ctx, groot);
                return tmpVar;
            }

            // otherwise, use the variant we have calculated
            return BLVar::null();
        }

        // paint usually comes in color/opacity pairs
        // so use SVGColor to read it, if it's not a URL 
        // reference
        bool loadFromAttributes(const XmlAttributeCollection& attrs) override
        {
			if (!fColor.loadFromAttributes(attrs))
                return false;

            set(true);

            return true;
        }



        void update(IAmGroot* groot) override
        {
            ByteSpan ref = rawValue();

            if (chunk_starts_with_cstr(ref, "url("))
            {
                if (groot != nullptr) {
                    auto node = groot->findNodeByUrl(ref);
                    if (nullptr != node)
                    {
                        node->update(groot);
                    }
                }
            }
        }
        
        
    };
}

namespace waavs {
    struct SVGColorPaint : public SVGPaint
    {
        static void registerFactory() {
            registerSVGAttributeByName("color", [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGColorPaint>();
                node->loadFromAttributes(attrs);
                return node;
                });
        }


        SVGColorPaint() 
            : SVGPaint(svgattr::color(), nullptr) { setName(svgattr::color()); }


        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->setDefaultColor(getVariant(ctx, groot));
        }

    };
}

namespace waavs {
    //
    //
    struct SVGFillPaint : public SVGPaint
    {
        static void registerFactory() {
            registerSVGAttribute(svgattr::fill(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGFillPaint>();
                node->loadFromAttributes(attrs);
                return node;
                });
        }

        SVGFillPaint() 
            : SVGPaint(svgattr::fill(), svgattr::fill_opacity()) 
        { 
            setName(svgattr::fill()); 
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            BLVar aVar = getVariant(ctx, groot);
            ctx->fill(aVar);
        }

    };

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
    
    struct SVGStrokePaint : public SVGPaint
    {
        static void registerFactory() {
            registerSVGAttributeByName(svgattr::stroke(), [](const XmlAttributeCollection& attrs) {
                auto node = std::make_shared<SVGStrokePaint>(); 
                node->loadFromAttributes(attrs);
                return node; 
                });
        }

        SVGStrokePaint() 
            : SVGPaint(svgattr::stroke(), svgattr::stroke_opacity()) 
        { setName(svgattr::stroke()); }

        // BUGBUG - getVariant is where we should be resolving
        // things like 'currentColor' as well as references
        /*
        bool loadFromAttributes(const XmlAttributeCollection& attrs) override
        {
            // get the value of the 'stroke' attribute.
            // if it == 'color', then we need to get the color from 
            // other attributes
            ByteSpan strokeAttr{};

            if (!attrs.getValue(svgattr::stroke(), strokeAttr))
                return false;

            // There are a couple of cases when using 'currentColor' as the
            // stroke value.
            // 1) It is being used on an element, where there is also a 'color' attribute
            //   In this case, whatever is in the 'color' attribute needs to become our
            //   BLVar value, and set on the context at drawing time.
            // 2) It is being used on an element, where there is no 'color' attribute
            //   In this case, it is inheritance.  The color value is determined by what is
            //   currently on the 'currentColor()' property of the context, so we should use
            //   that when it comes time to draw.
            if (strokeAttr == "currentColor")
            {
                // get the value of the 'color' attribute, if it exists
                // and use that as the color
                ByteSpan colorAttr{};
                if (!attrs.getValue(svgattr::color(),colorAttr))
                {
                    return loadFromChunk(colorAttr);
                }
                else {
                    // if the 'color' attribute does not exist
                    // set our rawValue to 'currentColor', and pick up 
                    // the default color from the drawing context at drawing time
                    setRawValue(strokeAttr);
                    set(true);
                    return true;
                }

            }

            // It's not 'currentColor', so process an immediate color value
            return loadFromChunk(strokeAttr);
        }
        */

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            BLVar aVar = getVariant(ctx, groot);
            if (aVar.is_null())
                ctx->noStroke();
            else
                ctx->stroke(aVar);
        }

    };


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


    //
    // SVGOrient
    // Determines how the marker should be oriented, and ultimately
    // what angle of rotation should be applied before drawing
    // 
    struct SVGOrient
    {
        double fAngle{ 0 };
        MarkerOrientation fOrientation{ MarkerOrientation::MARKER_ORIENT_ANGLE };


        SVGOrient(IAmGroot* groot) {}

        // In order to calculate the angle, we need the path
        // so we can determine the tangent at the start or end
        bool loadFromChunk(const ByteSpan& inChunk)
        {
            ByteSpan s = inChunk;
            s = chunk_skip_wsp(s);

            if (!s)
                return false;

            if (!parseMarkerOrientation(s, fOrientation))
                return false;

            switch (fOrientation)
            {
            case MarkerOrientation::MARKER_ORIENT_AUTO:
                return true;
                break;


            case MarkerOrientation::MARKER_ORIENT_AUTOSTARTREVERSE:
            {
                return true;

            } break;

            case MarkerOrientation::MARKER_ORIENT_ANGLE:
            default:
            {
                SVGAngleUnits units{ SVGAngleUnits::SVG_ANGLETYPE_UNKNOWN };
                return parseAngle(s, fAngle, units);
            } break;

            }

            return true;
        }


        // calculate the angle between line segments
        // return the value in radians
        //
        double calcRadians(MarkerPosition pos, const BLPoint& p1, const BLPoint& p2, const BLPoint& p3) const
        {

            
            if (fOrientation == MarkerOrientation::MARKER_ORIENT_ANGLE)
            {
                // fAngle is already in radians
                return fAngle;
            }
            
            // get the angle between the vectors
            auto angleOfVec = [](double x, double y) noexcept -> double {
                return std::atan2(y, x);
                };
            
            auto norm = [](double x, double y, double& ox, double& oy) noexcept -> bool {
                const double len2 = x * x + y * y;
                if (len2 == 0.0) { ox = 0.0; oy = 0.0; return false; }
                const double inv = 1.0 / std::sqrt(len2);
                ox = x * inv;
                oy = y * inv;
                return true;
                };


            // v1 = p2 - p1 (incoming tangent direction in your triplet)
            const double v1x = p2.x - p1.x;
            const double v1y = p2.y - p1.y;

            // v2 = p3 - p2 (outgoing tangent direction)
            const double v2x = p3.x - p2.x;
            const double v2y = p3.y - p2.y;

            double angle = 0.0;


            switch (pos)
            {
            case MARKER_POSITION_START:
            {
                // angle is between the first point, and the second point
                angle = angleOfVec(v1x, v1y);


                // If the marker is at the start, and orient='auto-start-reverse',
                // then we want to flip the angle 180 degrees, which we can do 
                // by adding 'pi' to it, as that's half a circle.
                if (fOrientation == MarkerOrientation::MARKER_ORIENT_AUTOSTARTREVERSE)
                    angle = angle + waavs::kPi;
            }
            break;

            case MARKER_POSITION_END:
                angle = angleOfVec(v1x, v1y);
                break;


            case MARKER_POSITION_MIDDLE:
            {
                // Normalize both, then bisect by vector sum
                double u1x, u1y, u2x, u2y;
                const bool ok1 = norm(v1x, v1y, u1x, u1y);
                const bool ok2 = norm(v2x, v2y, u2x, u2y);

                if (!ok1 && !ok2) {
                    angle = 0.0;
                    break;
                }
                if (!ok1) { angle = angleOfVec(u2x, u2y); break; }
                if (!ok2) { angle = angleOfVec(u1x, u1y); break; }

                // Bisector candidate
                double bx = u1x + u2x;
                double by = u1y + u2y;

                // If cusp (180°), sum is near zero; choose a stable fallback
                const double bLen2 = bx * bx + by * by;
                if (bLen2 == 0.0) {
                    // choose outgoing (common pragmatic choice)
                    angle = angleOfVec(u2x, u2y);
                }
                else {
                    angle = angleOfVec(bx, by);
                }
            }
            break;


            }
            
            //printf("calcRadians::MARKER_POSITION: %d - %3.2f\n", pos, waavs::degrees(angle));
            
            return radians_normalize(angle);
        }

    };
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
            
            if (chunk_starts_with_cstr(rawValue(), "url("))
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

        //const BLVar getVariant(IRenderSVG*, IAmGroot*) noexcept override
        //{
        //    if (fClipNode == nullptr)
        //        return fClipVar;

            // BUGBUG
            //return fClipNode->getVariant();
        //    return BLVar::null();
        //}

        
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
            
            if (chunk_starts_with_cstr(str, "url("))
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
        bool fIsNone{ true };

        SVGStrokeDashArray() : SVGVisualProperty(nullptr)
        {
            setName(svgattr::stroke_dasharray());
        }



        bool loadFromAttributes(const XmlAttributeCollection& attrs) override
        {
            ByteSpan dashArrayAttr{};
            if (!attrs.getValue(svgattr::stroke_dasharray(), dashArrayAttr))
                return false;

            // parse the dash array value
            if (!parseStrokeDashArray(dashArrayAttr, fArray, fIsNone))
                return set(false);

            set(true);
            setNeedsBinding(false);

            // Get the dash-offset if it exists
            ByteSpan dashOffsetAttr{};
            attrs.getValue(svgattr::stroke_dashoffset(), dashOffsetAttr);

            return true;
        }

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* groot) override
        {
            (void)groot;

            if (!ctx)
                return;

            if (fIsNone)
            {
                // No dash pattern.
                ctx->clearStrokeDashArray();
                ctx->clearStrokeDashOffset();

                return;
            }

            // Store raw dash segments in state.
            ctx->dashArray(fArray);
        }
    };


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

        void applySelfToContext(IRenderSVG* ctx, IAmGroot* /*groot*/) override
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

} // namespace waavs

