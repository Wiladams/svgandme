#pragma once

#include <memory>
#include <vector>
#include <cstdint>		// uint8_t, etc
#include <cstddef>		// nullptr_t, ptrdiff_t, size_t


#include "maths.h"
#include "xmlscan.h"

#include "svgstructuretypes.h"


namespace waavs {
    //
    // SVGPatternExtendMode
    // 
	// A structure that represents the extend mode of a pattern.
	struct SVGPatternExtendMode : public SVGVisualProperty 
    {
        static void registerFactory() {
            registerSVGAttribute("extendMode", [](const ByteSpan& value) {
                auto node = std::make_shared<SVGPatternExtendMode>(nullptr);
                node->loadFromChunk(value);
                return node;
                });
        }
        
        BLExtendMode fExtendMode{ BL_EXTEND_MODE_REPEAT };      // repeat by default

        SVGPatternExtendMode(IAmGroot* iMap) : SVGVisualProperty(iMap) 
        {
            autoDraw(false);
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!inChunk)
                return false;
            
            BLExtendMode outMode{ BL_EXTEND_MODE_PAD };
            if (parseExtendMode(inChunk, outMode))
			{
				fExtendMode = outMode;
                set(true);
                
				return true;
			}
            
            return false;
        }
        
		BLExtendMode value() const { return fExtendMode; }
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
    struct SVGOpacity : public SVGVisualProperty
    {
        static void registerFactory() {
			registerSVGAttribute("opacity", [](const ByteSpan& value) {
				auto node = std::make_shared<SVGOpacity>(nullptr);
			node->loadFromChunk(value);
			return node;
				});
            
        }
        
        double fValue{1};
        BLVar fOpacityVar;
        
        SVGOpacity(IAmGroot* groot) :SVGVisualProperty(groot) {}

        const BLVar getVariant() noexcept override { return fOpacityVar; }
        
        
        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
			ctx->globalOpacity(fValue);
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!inChunk)
                return false;
            
            SVGDimension dim;
			dim.loadFromChunk(inChunk);
			fValue = dim.calculatePixels(1.0);
            fOpacityVar = fValue;
            
            set(true);
			needsBinding(false);
            
            return true;
        }
        

    };
    
    struct SVGFillOpacity : public SVGOpacity
    {
        static void registerFactory() {
            registerSVGAttribute("fill-opacity", [](const ByteSpan& value) {
                auto node = std::make_shared<SVGFillOpacity>(nullptr); 
                node->loadFromChunk(value);  
                return node;
                });
            
        }
        
        SVGFillOpacity(IAmGroot* iMap) 
            :SVGOpacity(iMap) {}
        
        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
			ctx->fillOpacity(fValue);
        }

    };

    struct SVGStrokeOpacity : public SVGOpacity
    {
        static void registerFactory() {
            registerSVGAttribute("stroke-opacity", [](const ByteSpan& value) {
                auto node = std::make_shared<SVGStrokeOpacity>(nullptr); 
                node->loadFromChunk(value);  
                return node; 
                });

        }

        
        SVGStrokeOpacity(IAmGroot* iMap) 
            :SVGOpacity(iMap) {}


        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->strokeOpacity(fValue);
        }

    };

}

namespace waavs {
    struct SVGPaintOrderAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute("paint-order", [](const ByteSpan& value) {
                auto node = std::make_shared<SVGPaintOrderAttribute>(nullptr); 
                node->loadFromChunk(value);  
                return node; 
                });
        }

        ByteSpan fValue{};
        
        
        SVGPaintOrderAttribute(IAmGroot* iMap) :SVGVisualProperty(iMap) 
        {

        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!inChunk)
                return false;
            
            fValue = inChunk;
            
            set(true);

            return true;
        }
        
    };


}

//==================================================================
//  SVG Text Properties
//==================================================================
namespace waavs {
    struct SVGFontSize : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute("font-size", [](const ByteSpan& value) {
                auto node = std::make_shared<SVGFontSize>(nullptr); 
                node->loadFromChunk(value);  
                return node; 
                });
        }
        
        
        SVGDimension dimValue{};
        double fValue{ 16.0 };

        SVGFontSize(IAmGroot* inMap) 
            : SVGVisualProperty(inMap) 
        {
        }

        SVGFontSize& operator=(const SVGFontSize& rhs)
		{
			dimValue = rhs.dimValue;
			fValue = rhs.fValue;
			
            return *this;
		}

		double value() const { return fValue; }
        
        
        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            //ctx->textSize(fValue);
        }

        void bindToGroot(IAmGroot* groot, SVGViewable* container) noexcept override
        {
            if (nullptr == groot)
                return;
            
			fValue = dimValue.calculatePixels(16, 0, groot->dpi());
            
            needsBinding(false);
        }
        
        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
			if (!inChunk)
				return false;
            
            dimValue.loadFromChunk(inChunk);

            if (!dimValue.isSet())
                return false;
            
            needsBinding(true);

            set(true);
                
            return true;
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
            registerSVGAttribute("font-family", [](const ByteSpan& value) {
                auto node = std::make_shared<SVGFontFamily>(nullptr); 
                node->loadFromChunk(value);  
                return node; 
                });
        }
        


        ByteSpan fValue{};

        SVGFontFamily(IAmGroot* inMap) : SVGVisualProperty(inMap) {}

        SVGFontFamily& operator=(const SVGFontFamily& rhs) = delete;


		const ByteSpan& value() const { return fValue; }
        
        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            //ctx->textFamily(fValue.c_str());
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!inChunk)
				return false;
            
            fValue = inChunk;
            //fValue = toString(chunk_trim(inChunk, xmlwsp));
            set(true);
            
			return true;
        }

    };

	//========================================================
	// SVGFontStyle
	// attribute name="font-style" type="string" default="normal"
    //========================================================
    struct SVGFontStyleAttribute : public SVGVisualProperty
    {
        uint32_t fStyle{ BL_FONT_STYLE_NORMAL };

        SVGFontStyleAttribute() :SVGVisualProperty(nullptr) { set(false); needsBinding(false); }
        
        
        uint32_t value() const {return fStyle;}
        
        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
			ByteSpan s = chunk_trim(inChunk, xmlwsp);
            
            set(false);
            
			if (!s)
				return false;

            set(true);
            
            if (s == "normal")
                fStyle = BL_FONT_STYLE_NORMAL;
            else if (s == "italic")
                fStyle = BL_FONT_STYLE_ITALIC;
            else if (s == "oblique")
                fStyle = BL_FONT_STYLE_OBLIQUE;
            else
                set(false);
            
            return true;
        }
    };
    
    struct SVGFontWeightAttribute : public SVGVisualProperty
    {
        uint32_t fWeight{ BL_FONT_WEIGHT_NORMAL };
        
        SVGFontWeightAttribute() :SVGVisualProperty(nullptr) {}

		uint32_t value() const { return fWeight; }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            ByteSpan s = chunk_trim(inChunk, xmlwsp);

            set(false);

            if (!s)
                return false;

            set(true);

            if (s == "100")
                fWeight = BL_FONT_WEIGHT_THIN;
            else if (s == "200")
                fWeight = BL_FONT_WEIGHT_EXTRA_LIGHT;
            else if (s == "300")
                fWeight = BL_FONT_WEIGHT_LIGHT;
            else if (s == "normal" || s == "400")
                fWeight = BL_FONT_WEIGHT_NORMAL;
            else if (s == "500")
                fWeight = BL_FONT_WEIGHT_MEDIUM; 
            else if (s == "600")
                fWeight = BL_FONT_WEIGHT_SEMI_LIGHT;
            else if (s == "bold" || s == "700")
                fWeight = BL_FONT_WEIGHT_BOLD;
            else if (s == "800")
                fWeight = BL_FONT_WEIGHT_SEMI_BOLD;
            else if (s == "900")
				fWeight = BL_FONT_WEIGHT_EXTRA_BOLD;
			else if (s == "1000")
				fWeight = BL_FONT_WEIGHT_BLACK;
			else
				set(false);

			return true;
		}
	};

    struct SVGFontStretchAttribute : public SVGVisualProperty
    {
        uint32_t fValue{ BL_FONT_STRETCH_NORMAL };

        SVGFontStretchAttribute() :SVGVisualProperty(nullptr) {}

        uint32_t value() const { return fValue; }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            ByteSpan s = chunk_trim(inChunk, xmlwsp);

            set(false);

            if (!s)
                return false;

            set(true);

            if (s == "condensed")
                fValue = BL_FONT_STRETCH_CONDENSED;
            else if (s == "extra-condensed")
                fValue = BL_FONT_STRETCH_EXTRA_CONDENSED;
            else if (s == "semi-condensed")
                fValue = BL_FONT_STRETCH_SEMI_CONDENSED;
            else if (s == "normal" || s == "400")
                fValue = BL_FONT_STRETCH_NORMAL;
            else if (s == "semi-expanded")
                fValue = BL_FONT_STRETCH_SEMI_EXPANDED;
            else if (s == "extra-expanded")
                fValue = BL_FONT_STRETCH_EXTRA_EXPANDED;
            else if (s == "expanded")
                fValue = BL_FONT_STRETCH_EXPANDED;
            else
            {
                set(false);
                return false;
            }
            return true;
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

    struct SVGPaintAttribute : public SVGVisualProperty
    {
        BLVar fPaintVar{};

        SVGPaintAttribute(IAmGroot* groot) :SVGVisualProperty(groot) {}

        const BLVar getVariant() noexcept override
        {
            return fPaintVar;
        }
    };

    //=====================================================
    // SVG Paint
    // General base class for paint.  Other kinds of paints
    // such as fill, stroke, stop-color, descend from this
    //=====================================================
    struct SVGPaint : public SVGPaintAttribute
    {
        ByteSpan fPaintReference{};


        SVGPaint(IAmGroot* iMap) : SVGPaintAttribute(iMap) {}
        SVGPaint(const SVGPaint& other) = delete;



        //
        // resolvePaint()
        // We have a reference to something this is supposed to be our 
        // paint.  Try to retrieve it, and get it's variant.
        // BUGBUG - Ideally, thie is where we could do ObjectBBox binding
        // 
        virtual void resolvePaint(IAmGroot* groot, SVGViewable* container)
        {
            if (!fPaintReference)
                return;


            if (chunk_starts_with_cstr(fPaintReference, "url("))
            {
                auto node = groot->findNodeByUrl(fPaintReference);

                if (nullptr == node)
                    return;


                // Tell the referant node to resolve itself
                node->bindToGroot(groot, container);

                const BLVar& aVar = node->getVariant();

                auto res = blVarAssignWeak(&fPaintVar, &aVar);
                if (res != BL_SUCCESS)
                    return;

                set(true);
            }
        }



        // called when we have a reference to something
        void bindToGroot(IAmGroot* groot, SVGViewable* container) noexcept override
        {
            if (!needsBinding()) {
                return;
            }

            resolvePaint(groot, nullptr);

            needsBinding(false);
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

        void setOpacity(double opacity)
        {
            uint32_t outValue;
            if (BL_SUCCESS == blVarToRgba32(&fPaintVar, &outValue))
            {
                BLRgba32 newColor(outValue);
                newColor.setA((uint32_t)(opacity * 255));
                blVarAssignRgba32(&fPaintVar, newColor.value);
            }
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            static const char* rgbStr = "rgb(";
            static const char* rgbaStr = "rgba(";
            static const char* rgbStrCaps = "RGB(";
            static const char* rgbaStrCaps = "RGBA(";
            static const char* hslStr = "hsl(";
            static const char* hslaStr = "hsla(";

            ByteSpan str = inChunk;


            size_t len = 0;

            // First check to see if it's a lookup by 'url'
            // if it is, then register our desire to do a lookup
            // and finish for now.
            if (chunk_starts_with_cstr(str, "url("))
            {
                fPaintReference = str;
                //fNeedsResolving = true;
                needsBinding(true);

                return true;
            }

            BLRgba32 c(128, 128, 128);
            len = str.size();
            if ((len >= 1) && (*str == '#'))
            {
                c = parseColorHex(str);
                fPaintVar = c;
                set(true);
            }
            else if (str.startsWith(rgbStr) ||
                str.startsWith(rgbaStr) ||
                str.startsWith(rgbaStrCaps) ||
                str.startsWith(rgbStrCaps))
            {
                parseColorRGB(str, c);
                fPaintVar = c;
                set(true);
            }
            else if (str.startsWith(hslStr) ||
                str.startsWith(hslaStr))
            {
                c = parseColorHsl(str);
                fPaintVar = c;

                set(true);
            }
            else {
                if (str == "none") {
                    fPaintVar = BLVar::null();
                    set(true);
                }
                else if ((str == "context-stroke") || (str == "context-fill"))
                {
                    set(false);
                }
                else if ((str == "inherit") || (str == "currentColor"))
                {
                    // Take on whatever color value was previously set
                    // somewhere in the tree
                    set(false);
                }
                else {
                    c = getSVGColorByName(str);
                    fPaintVar = c;

                    set(true);
                }
            }

            return true;
        }
    };
}

namespace waavs {
    //
    //
    struct SVGFillPaint : public SVGPaint
    {
        static void registerFactory() {
            registerSVGAttribute("fill", [](const ByteSpan& value) {
                auto node = std::make_shared<SVGFillPaint>(nullptr);
                node->loadFromChunk(value);
                return node;
                });
        }


        SVGFillPaint(IAmGroot* root) : SVGPaint(root) { }

        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->fill(getVariant());
        }

    };

    //=========================================================
    // SVGFillRule
    //=========================================================
    static bool parseFillRule(const ByteSpan& inChunk, BLFillRule& value)
    {
        if (inChunk == "nonzero")
            value = BL_FILL_RULE_NON_ZERO;
        else if (inChunk == "evenodd")
            value = BL_FILL_RULE_EVEN_ODD;
        else
            return false;

        return true;
    }
    
    struct SVGFillRule : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute("fill-rule", [](const ByteSpan& value) {
                auto node = std::make_shared<SVGFillRule>(nullptr);
                node->loadFromChunk(value);
                return node;
                });
        }


        BLFillRule fValue{ BL_FILL_RULE_EVEN_ODD };

        SVGFillRule(IAmGroot* iMap) : SVGVisualProperty(iMap) {}

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            ByteSpan s = chunk_trim(inChunk, xmlwsp);

			if (!parseFillRule(s, fValue))
				return false;
            
            set(true);

            return true;
        }

        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            if (isSet())
                ctx->fillRule(fValue);
        }
    };
}





namespace waavs {
    
    struct SVGStrokePaint : public SVGPaint
    {
        static void registerFactory() {
            registerSVGAttribute("stroke", [](const ByteSpan& value) {
                auto node = std::make_shared<SVGStrokePaint>(nullptr); 
                node->loadFromChunk(value);  
                return node; 
                });
        }
        

		SVGStrokePaint(IAmGroot* root) : SVGPaint(root) {}

		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
            ctx->stroke(getVariant());
		}

    };


    //=========================================================
    // SVGStrokeWidth
    //=========================================================

    struct SVGStrokeWidth : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute("stroke-width", [](const ByteSpan& value) {auto node = std::make_shared<SVGStrokeWidth>(nullptr); node->loadFromChunk(value);  return node; });
        }

        double fWidth{ 1.0 };

        SVGStrokeWidth(IAmGroot* iMap) : SVGVisualProperty(iMap) { }

        SVGStrokeWidth(const SVGStrokeWidth& other) = delete;
        SVGStrokeWidth& operator=(const SVGStrokeWidth& rhs) = delete;

        
        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->strokeWidth(fWidth);
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (!inChunk)
                return false;
            
            ByteSpan s = inChunk;
            if (!readNumber(s, fWidth))
                return false;
            
            set(true);
            
            return true;
        }


    };

    //=========================================================
    ///  SVGStrokeMiterLimit
    /// A visual property to set the miter limit for a stroke
    //=========================================================
    struct SVGStrokeMiterLimit : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute("stroke-miterlimit", [](const ByteSpan& value) {auto node = std::make_shared<SVGStrokeMiterLimit>(nullptr); node->loadFromChunk(value);  return node; });
        }


        double fMiterLimit{ 4.0 };

        SVGStrokeMiterLimit(IAmGroot* iMap) : SVGVisualProperty(iMap) {}

        SVGStrokeMiterLimit(const SVGStrokeMiterLimit& other) = delete;
        SVGStrokeMiterLimit& operator=(const SVGStrokeMiterLimit& rhs) = delete;

        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->strokeMiterLimit(fMiterLimit);
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            ByteSpan s = inChunk;
            
            if (!readNumber(s, fMiterLimit))
                return false;
            
            fMiterLimit = clamp(fMiterLimit, 1.0, 10.0);

            set(true);
            needsBinding(false);

            return true;
        }


    };
    
    //=========================================================
    // SVGStrokeLineCap
    //=========================================================
    static bool parseLineCaps(const ByteSpan& inChunk, BLStrokeCap &value)
    {
        ByteSpan s = inChunk;
        
        if ( s == "butt")
            value = BL_STROKE_CAP_BUTT;
        else if (s == "round")
            value = BL_STROKE_CAP_ROUND;
        else if (s == "round-reverse")
            value = BL_STROKE_CAP_ROUND_REV;
        else if (s == "square")
            value = BL_STROKE_CAP_SQUARE;
        else if (s == "triangle")
            value = BL_STROKE_CAP_TRIANGLE;
        else if (s == "triangle-reverse")
            value = BL_STROKE_CAP_TRIANGLE_REV;
        else
            return false;

        return true;
    }
    
    struct SVGStrokeLineCap : public SVGVisualProperty
    {
        static void registerFactory()
        {
            registerSVGAttribute("stroke-linecap", [](const ByteSpan& value) {auto node = std::make_shared<SVGStrokeLineCap>(nullptr, "stroke-linecap"); node->loadFromChunk(value);  return node; });
            registerSVGAttribute("stroke-linecap-start", [](const ByteSpan& value) {auto node = std::make_shared<SVGStrokeLineCap>(nullptr, "stroke-linecap-start"); node->loadFromChunk(value);  return node; });
            registerSVGAttribute("stroke-linecap-end", [](const ByteSpan& value) {auto node = std::make_shared<SVGStrokeLineCap>(nullptr, "stroke-linecap-end"); node->loadFromChunk(value);  return node; });
        }


        BLStrokeCap fLineCap{ BL_STROKE_CAP_BUTT };
        BLStrokeCapPosition fLineCapPosition{};
        bool fBothCaps{ true };

        SVGStrokeLineCap(IAmGroot* iMap, const std::string& name) : SVGVisualProperty(iMap)
        {
            if (name == "stroke-linecap")
                fBothCaps = true;
            else if (name == "stroke-linecap-start")
            {
                fBothCaps = false;
                fLineCapPosition = BL_STROKE_CAP_POSITION_START;
            }
            else if (name == "stroke-linecap-end")
            {
                fBothCaps = false;
                fLineCapPosition = BL_STROKE_CAP_POSITION_END;
            }
        }

        SVGStrokeLineCap(const SVGStrokeLineCap& other) = delete;
        SVGStrokeLineCap& operator=(const SVGStrokeLineCap& rhs) = delete;

        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            if (fBothCaps) {
                ctx->strokeCaps(fLineCap);
            }
            else {
                ctx->strokeCap(fLineCap, fLineCapPosition);
            }

        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            ByteSpan s = inChunk;

			if (parseLineCaps(s, fLineCap))
			{
				set(true);
				needsBinding(false);
				return true;
			}

            set(false);
      

            return false;
        }

    };

    //=========================================================
    // SVGStrokeLineJoin
    // A visual property to set the line join for a stroke
    //=========================================================
    struct SVGStrokeLineJoin : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute("stroke-linejoin", [](const ByteSpan& value) {auto node = std::make_shared<SVGStrokeLineJoin>(nullptr); node->loadFromChunk(value);  return node; });
        }

        BLStrokeJoin fLineJoin{ BL_STROKE_JOIN_MITER_BEVEL };

        SVGStrokeLineJoin(IAmGroot* iMap) : SVGVisualProperty(iMap) {}
        SVGStrokeLineJoin(const SVGStrokeLineJoin& other) = delete;
        SVGStrokeLineJoin& operator=(const SVGStrokeLineJoin& rhs) = delete;


        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
        {
            ctx->strokeJoin(fLineJoin);
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            ByteSpan s = inChunk;

            set(true);

            if (s == "miter")
                fLineJoin = BL_STROKE_JOIN_MITER_BEVEL;
            else if (s == "round")
                fLineJoin = BL_STROKE_JOIN_ROUND;
            else if (s == "bevel")
                fLineJoin = BL_STROKE_JOIN_BEVEL;
            //else if (s == "arcs")
            //	fLineJoin = SVG_JOIN_ARCS;
            else if (s == "miter-clip")
                fLineJoin = BL_STROKE_JOIN_MITER_CLIP;
            else
                set(false);

            return true;
        }

    };
}



namespace waavs {

    //======================================================
    // SVGViewbox
    // A document may or may not have this property
    //======================================================

    struct SVGViewbox : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute("viewBox", [](const ByteSpan& value) {auto node = std::make_shared<SVGViewbox>(nullptr); node->loadFromChunk(value);  return node; });

        }

        
        BLRect fRect{};
        
        SVGViewbox() :SVGVisualProperty(nullptr) {}
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

    };


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
        MarkerOrientation fOrientation{ MarkerOrientation::MARKER_ORIENT_AUTO };


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


        // Given the specified orientation, and a path, calculate the angle
        // of rotation for the marker
        // return the value in radians
        double calculateRadians(MarkerPosition pos, const BLPoint& p1, const BLPoint& p2, const BLPoint& p3) const
        {
            if (fOrientation == MarkerOrientation::MARKER_ORIENT_ANGLE)
            {
                // fAngle is already in radians
                return fAngle;
            }

            // Calculate the angle based on tangent
			double diffx1 = p2.x - p1.x;
			double diffy1 = p2.y - p1.y;
			//double diffx2 = p3.x - p2.x;
			//double diffy2 = p3.y - p2.y;
            
            //double diffx1 = p1.x - p2.x;
            //double diffy1 = p1.y - p2.y;
            double diffx2 = p3.x - p2.x;
            double diffy2 = p3.y - p2.y;

            double ang1 = std::atan2(diffy1, diffx1);
            double ang2 = std::atan2(diffy2, diffx2);
            
            switch (pos)
            {
                case MARKER_POSITION_START: 
                {
                    if (fOrientation == MarkerOrientation::MARKER_ORIENT_AUTOSTARTREVERSE)
                        return ang1 + waavs::pi;
                }
                break;
                
                case MARKER_POSITION_MIDDLE:
                    return (ang1 + ang2) / 2.0;
                break;

                case MARKER_POSITION_END:
                    return ang1;
                break;
            }


            return ang1;
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
            registerSVGAttribute("marker", [](const ByteSpan& value) {auto node = std::make_shared<SVGMarkerAttribute>(nullptr); node->loadFromChunk(value);  return node; });
            registerSVGAttribute("marker-start", [](const ByteSpan& value) {auto node = std::make_shared<SVGMarkerAttribute>(nullptr); node->loadFromChunk(value);  return node; });
            registerSVGAttribute("marker-mid", [](const ByteSpan& value) {auto node = std::make_shared<SVGMarkerAttribute>(nullptr); node->loadFromChunk(value);  return node; });
            registerSVGAttribute("marker-end", [](const ByteSpan& value) {auto node = std::make_shared<SVGMarkerAttribute>(nullptr); node->loadFromChunk(value);  return node; });
        }
     

       
        std::shared_ptr<SVGViewable> fWrappedNode = nullptr;

        
        SVGMarkerAttribute(IAmGroot* groot) : SVGVisualProperty(groot) { }
        
		std::shared_ptr<SVGViewable> markerNode() const
		{
            return fWrappedNode;
		}
        
        void bindToGroot(IAmGroot* groot, SVGViewable* container) noexcept override
        {
            
            if (chunk_starts_with_cstr(rawValue(), "url("))
            {
                fWrappedNode = groot->findNodeByUrl(rawValue());

                if (fWrappedNode != nullptr)
                {
					fWrappedNode->bindToGroot(groot, container);
                    set(true);
                }
            }
            needsBinding(false);
        }
        
        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
			autoDraw(false); // we mark it as invisible, because we don't want it drawing when attributes are drawn
			// we only want it to draw when we're drawing during polyline/polygon drawing
            needsBinding(true);
            
            return true;
        }
        
        void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
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
            registerSVGAttribute("clip-path", [](const ByteSpan& value) {
                auto node = std::make_shared<SVGClipPathAttribute>(nullptr);
                node->loadFromChunk(value);

                return node;
                });

        }


        std::shared_ptr<SVGViewable> fClipNode{ nullptr };
        BLVar fClipVar;
        
        
        SVGClipPathAttribute(IAmGroot* groot) : SVGVisualProperty(groot) {}


        bool loadFromUrl(IAmGroot* groot, const ByteSpan& inChunk)
        {
            if (nullptr == groot)
                return false;

            ByteSpan str = inChunk;

            fClipNode = groot->findNodeByUrl(inChunk);

            if (fClipNode == nullptr) {
                set(false);
                return false;
            }

            // pull out the color value
            // BUGBUG - this will not always be the case
            // as what we point to might be a gradient or pattern
            if (fClipNode->needsBinding())
                fClipNode->bindToGroot(groot, nullptr);

            set(true);

            return true;
        }

        
		const BLVar getVariant() noexcept override
		{
            if (fClipNode == nullptr)
                return fClipVar;
            
            // BUGBUG
            //return fClipNode->getVariant();
            return BLVar::null();
		}

        // Let's get a connection to our referenced thing
        void bindToGroot(IAmGroot* groot, SVGViewable* container) noexcept override
        {
            ByteSpan str = rawValue();

            // Just load the referenced node at this point
            
            if (chunk_starts_with_cstr(str, "url("))
            {
                loadFromUrl(groot, str);
            }

            needsBinding(false);
        }
        
        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            needsBinding(true);
            set(true);
            
            return true;
        }
    };
}
    
namespace waavs {
    enum VectorEffectKind {
		VECTOR_EFFECT_NONE,
		VECTOR_EFFECT_NON_SCALING_STROKE,
	    VECTOR_EFFECT_NON_SCALING_SIZE,
        VECTOR_EFFECT_NON_ROTATION,
        VECTOR_EFFECT_FIXED_POSITION,
    };
    
    static bool parseVectorEffect(const ByteSpan& inChunk, VectorEffectKind& value)
    {
        if (inChunk == "none")
        {
            value = VECTOR_EFFECT_NONE;
        }
        else if (inChunk == "non-scaling-stroke")
        {
            value = VECTOR_EFFECT_NON_SCALING_STROKE;
        }
        else if (inChunk == "non-scaling-size")
        {
            value = VECTOR_EFFECT_NON_SCALING_SIZE;
        }
        else if (inChunk == "non-rotation")
        {
            value = VECTOR_EFFECT_NON_ROTATION;
        }
        else if (inChunk == "fixed-position")
        {
            value = VECTOR_EFFECT_FIXED_POSITION;
        }
        else
            return false;
        
        return true;
    }
    
    struct SVGVectorEffectAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute("vector-effect", [](const ByteSpan& value) {auto node = std::make_shared<SVGVectorEffectAttribute>(nullptr); node->loadFromChunk(value);  return node; });
        }


        VectorEffectKind fEffectKind{ VECTOR_EFFECT_NONE };

        
		SVGVectorEffectAttribute(IAmGroot* groot) : SVGVisualProperty(groot) {}
        
        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            needsBinding(false);
            
            if (!parseVectorEffect(inChunk, fEffectKind))
                return false;
            
            set(true);
            return true;
        }

		void drawSelf(IRenderSVG* ctx, IAmGroot* groot) override
		{
			if (VECTOR_EFFECT_NON_SCALING_STROKE == fEffectKind)
			{
                ctx->strokeBeforeTransform(true);
			}
		}
    };
}



