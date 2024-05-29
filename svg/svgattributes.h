#pragma once

#include <memory>
#include <vector>
#include <cstdint>		// uint8_t, etc
#include <cstddef>		// nullptr_t, ptrdiff_t, size_t


#include "xmlscan.h"
#include "svgcss.h"

#include "irendersvg.h"
#include "svgcolors.h"
#include "svgstructuretypes.h"




//================================================
// SVGTransform
// Transformation matrix
//================================================

namespace waavs {
    struct SVGTransform : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute("transform", [](const ByteSpan& value) {
                auto node = std::make_shared<SVGTransform>(nullptr);
			    node->loadFromChunk(value);
				return node;
				});
        }


        BLMatrix2D fTransform{};

        SVGTransform(IAmGroot* iMap) : SVGVisualProperty(iMap) { 
            autoDraw(false);
        }
        SVGTransform(const SVGTransform& other) = delete;


        const BLMatrix2D& getTransform() const { return fTransform; }

        const BLVar& getVariant() override
        {
            if (fVar.isNull())
            {
                blVarAssignWeak(&fVar, &fTransform);
            }
            return fVar;
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            ByteSpan s = inChunk;
            fTransform.reset();     // set to identity initially


            while (s)
            {
                s = chunk_skip_wsp(s);

                BLMatrix2D tm{};
                tm.reset();

                if (chunk_starts_with_cstr(s, "matrix"))
                {
                    s = parseMatrix(s, tm);
                    fTransform = tm;
                    set(true);
                }
                else if (chunk_starts_with_cstr(s, "translate"))
                {
                    s = parseTranslate(s, tm);
                    fTransform.transform(tm);
                    set(true);
                }
                else if (chunk_starts_with_cstr(s, "scale"))
                {
                    s = parseScale(s, tm);
                    fTransform.transform(tm);
                    set(true);
                }
                else if (chunk_starts_with_cstr(s, "rotate"))
                {
                    s = parseRotate(s, tm);
                    fTransform.transform(tm);
                    set(true);
                }
                else if (chunk_starts_with_cstr(s, "skewX"))
                {
                    s = parseSkewX(s, tm);
                    fTransform.transform(tm);
                    set(true);
                }
                else if (chunk_starts_with_cstr(s, "skewY"))
                {
                    s = parseSkewY(s, tm);
                    fTransform.transform(tm);
                    set(true);
                }
                else {
                    s++;
                }

            }

            return true;
        }

        void drawSelf(IRenderSVG* ctx) override
        {
            ctx->applyTransform(fTransform);
        }
    };

}

// Specific types of attributes
namespace waavs {

    
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

        SVGOpacity(IAmGroot* iMap) :SVGVisualProperty(iMap) {}


        void drawSelf(IRenderSVG* ctx) override
        {
			ctx->globalOpacity(fValue);
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            SVGDimension dim;
			dim.loadFromChunk(inChunk);
			fValue = dim.calculatePixels(1.0);
            fVar = fValue;
            
            set(true);
			needsBinding(false);
            
            return true;
        }
        

    };
    
    struct SVGFillOpacity : public SVGOpacity
    {
        static void registerFactory() {
            registerSVGAttribute("fill-opacity", [](const ByteSpan& value) {auto node = std::make_shared<SVGFillOpacity>(nullptr); node->loadFromChunk(value);  return node;});
        }
        

        SVGFillOpacity(IAmGroot* iMap) 
            :SVGOpacity(iMap) {}
        
        void drawSelf(IRenderSVG* ctx) override
        {
			ctx->fillOpacity(fValue);
        }

    };

    struct SVGStrokeOpacity : public SVGOpacity
    {
        static void registerFactory() {
            registerSVGAttribute("stroke-opacity", [](const ByteSpan& value) {auto node = std::make_shared<SVGStrokeOpacity>(nullptr); node->loadFromChunk(value);  return node; });

        }

        
        SVGStrokeOpacity(IAmGroot* iMap) 
            :SVGOpacity(iMap) {}


        void drawSelf(IRenderSVG* ctx) override
        {
            ctx->strokeOpacity(fValue);
        }

    };

}

namespace waavs {
    struct SVGPaintOrderAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute("paint-order", [](const ByteSpan& value) {auto node = std::make_shared<SVGPaintOrderAttribute>(nullptr); node->loadFromChunk(value);  return node; });
        }

        ByteSpan fValue{};
        
        
        SVGPaintOrderAttribute(IAmGroot* iMap) :SVGVisualProperty(iMap) {

        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            fValue = inChunk;
            
            set(true);

            return true;
        }
        
    };

    
    struct SVGRawAttribute : public SVGVisualProperty
    {
		static void registerFactory() {
            registerSVGAttribute("systemLanguage", [](const ByteSpan& value) {auto node = std::make_shared<SVGRawAttribute>(nullptr); node->loadFromChunk(value);  return node; });
		}

        SVGRawAttribute(IAmGroot* iMap) :SVGVisualProperty(iMap) { 
            //visible(false); 
        }        


		bool loadSelfFromChunk(const ByteSpan& inChunk) override
		{
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
            registerSVGAttribute("font-size", [](const ByteSpan& value) {auto node = std::make_shared<SVGFontSize>(nullptr); node->loadFromChunk(value);  return node; });
        }
        
        
        SVGDimension dimValue{};
        double fValue{ 16.0 };

        SVGFontSize(IAmGroot* inMap) : SVGVisualProperty(inMap) {}

        SVGFontSize& operator=(const SVGFontSize& rhs)
		{
			dimValue = rhs.dimValue;
			fValue = rhs.fValue;
			return *this;
		}

		double value() const { return fValue; }
        
        
        void drawSelf(IRenderSVG* ctx) override
        {
            ctx->textSize(fValue);
        }

        void bindToGroot(IAmGroot* groot) override
        {
            needsBinding(false);
            
            if (nullptr == groot)
                return;
            
			fValue = dimValue.calculatePixels(16, 0, groot->dpi());
        }
        
        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
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
            registerSVGAttribute("font-family", [](const ByteSpan& value) {auto node = std::make_shared<SVGFontFamily>(nullptr); node->loadFromChunk(value);  return node; });
        }
        


        std::string fValue{ "Arial" };

        SVGFontFamily(IAmGroot* inMap) : SVGVisualProperty(inMap) {}

        SVGFontFamily& operator=(const SVGFontFamily& rhs) = delete;


		const std::string& value() { return fValue; }
        
        void drawSelf(IRenderSVG* ctx) override
        {
            ctx->textFamily(fValue.c_str());
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            fValue = toString(chunk_trim(inChunk, xmlwsp));
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
    
    struct SVGFontSelection : public SVGVisualProperty
    {
        BLFont fFont;
        
        std::string fFamilyName{};
        SVGFontSize fFontSize;
        uint32_t fFontStyle = BL_FONT_STYLE_NORMAL;
		uint32_t fFontWeight = BL_FONT_WEIGHT_NORMAL;
		uint32_t fFontStretch = BL_FONT_STRETCH_NORMAL;
        
        
		SVGFontSelection(IAmGroot* inMap) 
            : SVGVisualProperty(inMap)
            , fFontSize(inMap) 
        {
            needsBinding(true);
            set(false);
        }
        
		SVGFontSelection& operator=(const SVGFontSelection& rhs) 
		{
            fFont.reset();
            
			fFamilyName = rhs.fFamilyName;
			fFontSize = rhs.fFontSize;
			fFontStyle = rhs.fFontStyle;
			fFontWeight = rhs.fFontWeight;
			fFontStretch = rhs.fFontStretch;

            set(false);
            needsBinding(true);
            
			return *this;
		}
        
        void bindToGroot(IAmGroot* groot) override
        {
            if (!isSet())
                return;

            FontHandler* fh = groot->fontHandler();
            
            // resolve the size
            // lookup the font face
            fFontSize.bindToGroot(groot);
            auto fsize = fFontSize.value();

			//bool success = fh->selectFont(fFontFamily.value().c_str(), fFont, fsize, fFontStyle, fFontWeight, fFontStretch);
            bool success = fh->selectFont(fFamilyName.c_str(), fFont, (float)fsize, fFontStyle, fFontWeight, fFontStretch);
            if (success)
                set(true);
		}
        
        void loadFromXmlAttributes(const XmlAttributeCollection& elem)
        {   
            // look for font-family
			auto familyChunk = elem.getAttribute("font-family");
            if (familyChunk) {
                fFamilyName = std::string(familyChunk.fStart, familyChunk.fEnd);
                set(true);
            }
            
            // look for font-size
            // This can get resolved at binding time
			fFontSize.loadFromChunk(elem.getAttribute("font-size"));
            if (fFontSize.isSet())
                set(true);
            
            // look for font-style
            SVGFontStyleAttribute styleAttribute;
            styleAttribute.loadFromChunk(elem.getAttribute("font-style"));
            if (styleAttribute.isSet()) {
                fFontStyle = styleAttribute.value();
                set(true);
            }
            
            // look for font-weight
			SVGFontWeightAttribute weightAttribute;
			weightAttribute.loadFromChunk(elem.getAttribute("font-weight"));
            if (weightAttribute.isSet()) {
                fFontWeight = weightAttribute.value();
                set(true);
            }

			// look for font-stretch
            SVGFontStretchAttribute stretchAttribute;
			stretchAttribute.loadFromChunk(elem.getAttribute("font-stretch"));
			if (stretchAttribute.isSet()) {
				fFontStretch = stretchAttribute.value();
				set(true);
			}
        }

		void draw(IRenderSVG* ctx) override
		{
            // BUGBUG - not quite sure if we need both checks
            //if (isSet() && visible())
            if (isSet())
			    ctx->font(fFont);
		}
    };
}

namespace waavs {
    //====================================
    // Text Anchoring
    //====================================
    //enum class TEXTANCHOR : unsigned
    //{
    //    MIDDLE = 0x01,
    //    START = 0x02,
    //    END = 0x04,
    //};
    
    struct SVGTextAnchor : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute("text-anchor", [](const ByteSpan& value) {auto node = std::make_shared<SVGTextAnchor>(nullptr); node->loadFromChunk(value);  return node; });
        }

        
        ALIGNMENT fValue{ ALIGNMENT::LEFT };

        SVGTextAnchor(IAmGroot* iMap) : SVGVisualProperty(iMap) {}
        SVGTextAnchor(const SVGTextAnchor& other) = delete;


        SVGTextAnchor& operator=(const SVGTextAnchor& rhs) = delete;

        void drawSelf(IRenderSVG* ctx) override
        {
            ctx->textAlign(fValue, ALIGNMENT::BASELINE);
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            if (inChunk == "start")
                fValue = ALIGNMENT::LEFT;
            else if (inChunk == "middle")
                fValue = ALIGNMENT::CENTER;
            else if (inChunk == "end")
                fValue = ALIGNMENT::RIGHT;

            set(true);

            return true;
        }


    };
}




//======================================================
// Definition of SVG Paint
//======================================================
namespace waavs {
    // Representation of color according to CSS specification
    // https://www.w3.org/TR/css-color-4/#typedef-color
    // Over time, this structure could represent the full specification
    // but for practical purposes, we'll focus on rgb, rgba for now
    /*
    <color> = <absolute-color-base> | currentcolor | <system-color>

    <absolute-color-base> = <hex-color> | <absolute-color-function> | <named-color> | transparent
    <absolute-color-function> = <rgb()> | <rgba()> |
                            <hsl()> | <hsla()> | <hwb()> |
                            <lab()> | <lch()> | <oklab()> | <oklch()> |
                            <color()>
    */

}

namespace waavs {
    // Scan a bytespan for a sequence of hex digits
    // without using scanf. return 'true' upon success
    // false for any error.
    // The format is either
	// #RRGGBB
    // #RGB
    // Anything else is an error
    static inline uint8_t  hexCharToDecimal(uint8_t value)
    {
		if (value >= '0' && value <= '9')
			return value - '0';
		else if (value >= 'a' && value <= 'f')
			return value - 'a' + 10;
		else if (value >= 'A' && value <= 'F')
			return value - 'A' + 10;
		else
			return 0;
    }
    
	static bool hexSpanToDecimal(const ByteSpan& inSpan, BLRgba32& outValue)
	{
        outValue.value = 0;
        
		if (inSpan.size() == 0)
			return false;

		if (inSpan[0] != '#')
			return false;

		if (inSpan.size() == 4) {
			// #RGB
			// 0xRRGGBB
			uint8_t r = hexCharToDecimal(inSpan[1]);
			uint8_t g = hexCharToDecimal(inSpan[2]);
			uint8_t b = hexCharToDecimal(inSpan[3]);

            outValue = BLRgba32(r * 17, g * 17, b * 17);			// same effect as (r<<4|r), (g<<4|g), ..

			return true;
		}

		if (inSpan.size() == 7) {
			// #RRGGBB
			// 0xRRGGBB
            uint8_t r = (hexCharToDecimal(inSpan[1])<<4)| hexCharToDecimal(inSpan[2]);
            uint8_t g = (hexCharToDecimal(inSpan[3])<<4)| hexCharToDecimal(inSpan[4]);
            uint8_t b = (hexCharToDecimal(inSpan[5])<<4)| hexCharToDecimal(inSpan[6]);

            outValue = BLRgba32(r, g, b);
			return true;
		}

		return false;
	}
    
	// Turn a 3 or 6 digit hex string into a BLRgba32 value
    // if there's an error in the conversion, a transparent color is returned
    // BUGBUG - maybe a more obvious color should be returned
    static BLRgba32 parseColorHex(const ByteSpan& chunk)
    {
        BLRgba32 res{};
        if (hexSpanToDecimal(chunk, res))
            return res;
            
		return BLRgba32(0);
    }
    

    //
	// parse a color string
	// Return a BLRgba32 value
    static double hue_to_rgb(double p, double q, double t) {
        if (t < 0) t += 1;
        if (t > 1) t -= 1;
        if (t < 1 / 6.0) return p + (q - p) * 6 * t;
        if (t < 1 / 2.0) return q;
        if (t < 2 / 3.0) return p + (q - p) * (2 / 3.0 - t) * 6;
        return p;
    }
    
    BLRgba32 hsl_to_rgb(double h, double s, double l)
    {
        double r, g, b;

        if (s == 0) {
            r = g = b = l; // achromatic
        }
        else {
            double q = l < 0.5 ? l * (1 + s) : l + s - l * s;
            double p = 2 * l - q;
            r = hue_to_rgb(p, q, h + 1 / 3.0);
            g = hue_to_rgb(p, q, h);
            b = hue_to_rgb(p, q, h - 1 / 3.0);
        }

        return BLRgba32(r*255, g*255, b*255);
    }
    
    static BLRgba32 parseColorHsl(const ByteSpan& inChunk)
    {
        BLRgba32 defaultColor(0, 0, 0);
        ByteSpan str = inChunk;
        auto leading = chunk_token(str, "(");
        
        // h, s, l attributes can be numeric, or percent
        // get the numbers by separating at the ')'
        auto nums = chunk_token(str, ")");
        SVGDimension hd{};
        SVGDimension sd{};
		SVGDimension ld{};
        SVGDimension od{255,SVGDimensionUnits::SVG_UNITS_USER};
        
		double h = 0, s = 0, l = 0;
		double o = 1.0;
        
        auto num = chunk_token(nums, ",");
        if (!hd.loadFromChunk(num))
            return defaultColor;
        
        num = chunk_token(nums, ",");
        if (!sd.loadFromChunk(num))
            return defaultColor;

        num = chunk_token(nums, ",");
        if (!ld.loadFromChunk(num))
            return defaultColor;
        
        // check for opacity
        num = chunk_token(nums, ",");
        if (od.loadFromChunk(num))
        {
            o = od.calculatePixels(1.0);
        }

        
        // get normalized values to start with
		h = hd.calculatePixels(360)/360.0;
		s = sd.calculatePixels(100)/100.0;
		l = ld.calculatePixels(100)/100.0;
		//o = od.calculatePixels(255);
        
		auto res = hsl_to_rgb(h, s, l);
		res.setA(o*255);

		return res;
    }
    
    // Parse rgb color. The pointer 'str' must point at "rgb(" (4+ characters).
    // This function returns gray (rgb(128, 128, 128) == '#808080') on parse errors
    // for backwards compatibility. Note: other image viewers return black instead.

    static BLRgba32 parseColorRGB(const ByteSpan& inChunk)
    {
        // skip past the leading "rgb("
        ByteSpan s = inChunk;
        auto leading = chunk_token(s, "(");

        // s should now point to the first number
        // and 'leading' should contain 'rgb'
        // BUGBUG - we can check 'leading' to see if it's actually 'rgb'
        // but we'll just assume it is for now

        // get the numbers by separating at the ')'
        auto nums = chunk_token(s, ")");

        // So, now nums contains the individual numeric values, separated by ','
        // The individual numeric values are either
        // 50%
        // 50

        int i = 0;
        uint8_t rgbi[4]{};

        // Get the first token, which is red
        // if it's not there, then return gray
        auto num = chunk_token(nums, ",");
        if (chunk_size(num) < 1)
            return BLRgba32(128, 128, 128, 0);

        while (num)
        {
            SVGDimension cv{};
			cv.loadFromChunk(num);
            //auto cv = parseDimension(num);

			if (cv.units() == SVGDimensionUnits::SVG_UNITS_PERCENT)
            {
                // it's a percentage
                // BUGBUG - we're assuming it's a range of [0..255]
                rgbi[i] = (uint8_t)cv.calculatePixels(255);
            }
            else
            {
                // it's a regular number
                if (i == 3)
                    rgbi[i] = (uint8_t)(cv.value() * 255.0f);
                else
                    rgbi[i] = (uint8_t)cv.value();
            }
            i++;
            num = chunk_token(nums, ",");
        }

        if (i == 4)
            return BLRgba32(rgbi[0], rgbi[1], rgbi[2], rgbi[3]);

        return BLRgba32(rgbi[0], rgbi[1], rgbi[2]);
    }


    //=====================================================
	// SVG Paint
    // General base class for paint.  Other kinds of paints
    // such as fill, stroke, stop-color, descend from this
	//=====================================================
    struct SVGPaint : public SVGVisualProperty
    {
        bool fExplicitNone{ false };

        
        SVGPaint(IAmGroot* iMap) : SVGVisualProperty(iMap) {}
        SVGPaint(const SVGPaint& other) = delete;

        
        bool loadFromUrl(IAmGroot* groot, const ByteSpan& inChunk)
        {
            if (nullptr == groot)
                return false;
            
            ByteSpan str = inChunk;

            auto node = groot->findNodeByUrl(inChunk);

            if (nullptr == node)
                return false;
            
            // pull out the variant, assuming there is one

            if (node->needsBinding())
                node->bindToGroot(groot);

            const BLVar& aVar = node->getVariant();

            
            auto res = blVarAssignWeak(&fVar, &aVar);
            if (res != BL_SUCCESS)
                return false;
            
            set(true);

            return true;
        }

        
        // called when we have a reference to something
        void bindToGroot(IAmGroot* groot) override
        {   
            fRoot = groot;
            
            BLRgba32 c{};

			ByteSpan str = rawValue();

            if (chunk_starts_with_cstr(str, "url("))
            {
                loadFromUrl(groot, str);
            }
            
            needsBinding(false);
        }

        void update() override
        {
            ByteSpan ref = rawValue();

            if (chunk_starts_with_cstr(ref, "url("))
            {
                if (root() != nullptr) {
                    auto node = root()->findNodeByUrl(ref);
                    if (nullptr != node)
                    {
                        node->update();

                    }
                }
            }
        }
        
        void setOpacity(double opacity)
        {
            uint32_t outValue;
            if (BL_SUCCESS == blVarToRgba32(&fVar, &outValue))
            {
                BLRgba32 newColor(outValue);
                newColor.setA((uint32_t)(opacity * 255));
                blVarAssignRgba32(&fVar, newColor.value);
            }
        }
        
        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            const char *rgbStr = "rgb(";
            const char* rgbaStr = "rgba(";
			const char* rgbStrCaps = "RGB(";
			const char* rgbaStrCaps = "RGBA(";
			const char* hslStr = "hsl(";
			const char* hslaStr = "hsla(";
            
            ByteSpan str = inChunk;


            size_t len = 0;

            // First check to see if it's a lookup by 'url'
            // if it is, then register our desire to do a lookup
            // and finish for now.
            if (chunk_starts_with_cstr(str, "url("))
            {
                //fReference = str;
                needsBinding(true);

                return true;
            }
            
            BLRgba32 c{};
            len = chunk_size(str);
            if (len >= 1 && *str == '#')
            {
                c = parseColorHex(str);
                blVarAssignRgba32(&fVar, c.value);
                set(true);
            }
            else if (chunk_starts_with(str, rgbStr) || 
                chunk_starts_with(str, rgbaStr) ||
                chunk_starts_with(str, rgbaStrCaps) ||
                chunk_starts_with(str, rgbStrCaps))
            {
                c = parseColorRGB(str);
                blVarAssignRgba32(&fVar, c.value);
                set(true);
            }
            else if (chunk_starts_with(str, hslStr) ||
                chunk_starts_with(str, hslaStr))
            {
                c = parseColorHsl(str);
                blVarAssignRgba32(&fVar, c.value);
                set(true);
            }
            else {
                std::string cName = std::string(str.fStart, str.fEnd);
                if (cName == "none") {
                    fExplicitNone = true;
                    set(true);
                }
                else if (cName == "inherit")
                {
                    // Take on whatever color value was previously set
                    // somewhere in the tree
                    set(false);
                }
                else if (svgcolors.contains(cName))
                {
                    c = svgcolors[cName];
                    blVarAssignRgba32(&fVar, c.value);
                    set(true);
                }
                else {
                    // user wants some sort of color, which is either invalid name
                    // or a color function we don't support yet
                    // so set a default gray color
                    c = BLRgba32(128, 128, 128);
                    blVarAssignRgba32(&fVar, c.value);
                    set(true);
                }
            }
            
            return true;
        }
    };

	struct SVGFillPaint : public SVGPaint
	{
        static void registerFactory() {
            registerSVGAttribute("fill", [](const ByteSpan& value) {auto node = std::make_shared<SVGFillPaint>(nullptr); node->loadFromChunk(value);  return node; });
        }
        
        
		SVGFillPaint(IAmGroot* root) : SVGPaint(root) { }

        void drawSelf(IRenderSVG* ctx) override
        {
            if (fExplicitNone) {
                ctx->noFill();
            }
            else
            {
                //ctx->fill(getVariant());
                
                // Patterns can be recursive
                // use: Cannon-diagram2.svg to torture test this
				// BUGBUG - check the type of the variant
                const BLVar& aVar = getVariant();
                if (aVar.isGradient() || aVar.isRgba32() ) {
                    ctx->fill(aVar);
                }
                else if (aVar.isPattern()) {
                    // get the pattern out of the variant
                    //const BLPattern& aPatt = (const BLPattern&)aVar;

                    //ctx->setFillStyle(aPatt);
                    //ctx->fill(BLRgba32(0xff00ffff));
                    ctx->fill(aVar);
                }
                else {
                    BLObjectType aType = aVar.type();
                    printf("A Type: %d\n", (int)aType);
                    ctx->fill(BLRgba32(0xffff0000));
                }
                
            }
                
        }

	};

    struct SVGStrokePaint : public SVGPaint
    {
        static void registerFactory() {
            registerSVGAttribute("stroke", [](const ByteSpan& value) {auto node = std::make_shared<SVGStrokePaint>(nullptr); node->loadFromChunk(value);  return node; });
        }
        

		SVGStrokePaint(IAmGroot* root) : SVGPaint(root) {}

		void drawSelf(IRenderSVG* ctx) override
		{
            if (fExplicitNone) {
                ctx->noStroke();
            }
            else {
                // BUGBUG - check the type of the variant
                ctx->stroke(getVariant());
                //ctx->stroke(BLRgba32(0xff0d0d0d));
            }
		}

    };




}


namespace waavs {

    //=========================================================
    // SVGFillRule
    //=========================================================
    struct SVGFillRule : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute("fill-rule", [](const ByteSpan& value) {auto node = std::make_shared<SVGFillRule>(nullptr); node->loadFromChunk(value);  return node; });
        }

        
        //BLFillRule fValue{ BL_FILL_RULE_NON_ZERO };
        BLFillRule fValue{ BL_FILL_RULE_EVEN_ODD };

        SVGFillRule(IAmGroot* iMap) : SVGVisualProperty(iMap) {}
        SVGFillRule(const SVGFillRule& other) = delete;

        SVGFillRule& operator=(const SVGFillRule& rhs) = delete;

        void drawSelf(IRenderSVG* ctx) override
        {
            ctx->fillRule(fValue);
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            ByteSpan s = chunk_trim(inChunk, xmlwsp);

            set(true);

            if (s == "nonzero")
                fValue = BL_FILL_RULE_NON_ZERO;
            else if (s == "evenodd")
                fValue = BL_FILL_RULE_EVEN_ODD;
            else
                set(false);

            return true;
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
            registerSVGAttribute("stroke-width", [](const ByteSpan& value) {auto node = std::make_shared<SVGStrokeWidth>(nullptr); node->loadFromChunk(value);  return node; });
        }


        
        double fWidth{ 1.0 };

		SVGStrokeWidth(IAmGroot* iMap) : SVGVisualProperty(iMap) { }
        
        SVGStrokeWidth(const SVGStrokeWidth& other) = delete;
        SVGStrokeWidth& operator=(const SVGStrokeWidth& rhs) = delete;

        void drawSelf(IRenderSVG* ctx) override
        {
            ctx->strokeWidth(fWidth);
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            fWidth = toNumber(inChunk);
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

        void drawSelf(IRenderSVG* ctx) override
        {
            ctx->strokeMiterLimit(fMiterLimit);
        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            fMiterLimit = toNumber(inChunk);
            fMiterLimit = clamp(fMiterLimit, 1.0, 10.0);

            set(true);
            needsBinding(false);
            
            return true;
        }


    };

    //=========================================================
    // SVGStrokeLineCap
    //=========================================================
    struct SVGStrokeLineCap : public SVGVisualProperty
    {
        static void registerFactory()
        {
            registerSVGAttribute("stroke-linecap", [](const ByteSpan& value) {auto node = std::make_shared<SVGStrokeLineCap>(nullptr,"stroke-linecap"); node->loadFromChunk(value);  return node; });
            registerSVGAttribute("stroke-linecap-start", [](const ByteSpan& value) {auto node = std::make_shared<SVGStrokeLineCap>(nullptr,"stroke-linecap-start"); node->loadFromChunk(value);  return node; });
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

        void drawSelf(IRenderSVG* ctx) override
        {
            if (fBothCaps) {
                ctx->strokeCaps(fLineCap);
            } 
            else{
				ctx->strokeCap(fLineCap, fLineCapPosition);
            }

        }

        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            ByteSpan s = inChunk;

            set(true);

            if (s == "butt")
                fLineCap = BL_STROKE_CAP_BUTT;
            else if (s == "round")
                fLineCap = BL_STROKE_CAP_ROUND;
            else if (s == "round-reverse")
                fLineCap = BL_STROKE_CAP_ROUND_REV;
            else if (s == "square")
                fLineCap = BL_STROKE_CAP_SQUARE;
            else if (s == "triangle")
                fLineCap = BL_STROKE_CAP_TRIANGLE;
            else if (s == "triangle-reverse")
                fLineCap = BL_STROKE_CAP_TRIANGLE_REV;
            else
                set(false);

            return true;
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


        void drawSelf(IRenderSVG* ctx) override
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

            //gSVGPropertyCreation["viewBox"] = [](IAmGroot* root, const XmlAttributeCollection& elem) {
            //    auto node = std::make_shared<SVGViewbox>(root);
            //    node->loadFromChunk(elem.getAttribute("viewBox"));
            //    return node;
            //};
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
            ByteSpan s = inChunk;

            if (!parseNextNumber(s, fRect.x))
                return false;
            if (!parseNextNumber(s, fRect.y))
                return false;
            if (!parseNextNumber(s, fRect.w))
                return false;
            if (!parseNextNumber(s, fRect.h))
                return false;

			set(true);

            return true;
        }

    };


}





namespace waavs {
    // Could be used as bitfield
    enum MarkerPosition {

        MARKER_POSITION_START = 0,
        MARKER_POSITION_MIDDLE = 1,
        MARKER_POSITION_END = 2,
        MARKER_POSITION_ALL = 3
    };
    
    // determines the orientation of a marker
	enum class MarkerOrientation
	{
		MARKER_ORIENT_AUTO,
        MARKER_ORIENT_AUTOSTARTREVERSE,
        MARKER_ORIENT_ANGLE
	};
    
    //
    // SVGOrient
    // Determines how the marker should be oriented, and ultimately
    // what angle of rotation should be applied before drawing
    // 
    struct SVGOrient
    {
        SVGAngle fAngle;
		MarkerOrientation fOrientation{ MarkerOrientation::MARKER_ORIENT_AUTO };
        
        
        SVGOrient(IAmGroot* groot) {}
        
        // In order to calculate the angle, we need the path
		// so we can determine the tangent at the start or end
		bool loadFromChunk(const ByteSpan& inChunk)
		{
			ByteSpan s = inChunk;
			s = chunk_skip_wsp(s);

			if (s == "auto")
			{
				fOrientation = MarkerOrientation::MARKER_ORIENT_AUTO;
				return true;
			}
			else if (s == "auto-start-reverse")
			{
				fOrientation = MarkerOrientation::MARKER_ORIENT_AUTOSTARTREVERSE;
				return true;
			}
			else
			{
				fOrientation = MarkerOrientation::MARKER_ORIENT_ANGLE;
				return fAngle.loadFromChunk(s);
			}

            return true;
		}


		// Given the specified orientation, and a path, calculate the angle
		// of rotation for the marker
        double calculateRadians(MarkerPosition pos, const BLPoint &p1, const BLPoint &p2) const
        {
            if (fOrientation == MarkerOrientation::MARKER_ORIENT_ANGLE)
            {
                return fAngle.radians();
            }
            
            
            double ang = std::atan2(p2.y - p1.y, p2.x - p1.x);

            
            switch (fOrientation)
            {
			case MarkerOrientation::MARKER_ORIENT_AUTO:
			{
                // BUGBUG - need to rework this.  'auto' is supposed to give
                // you an angle between a center point and the current location
				return ang;
			}
			case MarkerOrientation::MARKER_ORIENT_AUTOSTARTREVERSE:
			{
				// use p1 and p2 as a vector to calculate an angle in radians
				// where p1 is the origin, and p2 is the vector
				// then add pi to the angle

                
				return -ang;
			}

            }
            
            return ang;
        }
    };
    
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
        
		std::shared_ptr<SVGViewable> markerNode()
		{
            return fWrappedNode;
		}
        
        void bindToGroot(IAmGroot* groot) override
        {
            
            if (chunk_starts_with_cstr(rawValue(), "url("))
            {
                fWrappedNode = groot->findNodeByUrl(rawValue());

                if (fWrappedNode != nullptr)
                {
					fWrappedNode->bindToGroot(groot);
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
        
        void drawSelf(IRenderSVG* ctx) override
        {
			if (fWrappedNode)
				fWrappedNode->draw(ctx);
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
            gSVGPropertyCreation["clip-path"] = [](IAmGroot* groot, const XmlAttributeCollection& elem) {
                auto node = std::make_shared<SVGClipPathAttribute>(groot);
                node->loadFromChunk(elem.getAttribute("clip-path"));
                return node;
            };

        }


        std::shared_ptr<SVGViewable> fClipNode{ nullptr };
        
        
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
                fClipNode->bindToGroot(groot);

            set(true);

            return true;
        }

        
		const BLVar& getVariant() override
		{
            if (fClipNode == nullptr)
                return fVar;
            
            return fClipNode->getVariant();

		}

        // Let's get a connection to our referenced thing
        void bindToGroot(IAmGroot* groot) override
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
    
    struct SVGVectorEffectAttribute : public SVGVisualProperty
    {
        static void registerFactory() {
            registerSVGAttribute("vector-effect", [](const ByteSpan& value) {auto node = std::make_shared<SVGVectorEffectAttribute>(nullptr); node->loadFromChunk(value);  return node; });
        }



        bool fExplicitNone{ false };
        bool fRenderBeforeScale = false;
        VectorEffectKind fEffectKind{ VECTOR_EFFECT_NONE };

        
		SVGVectorEffectAttribute(IAmGroot* groot) : SVGVisualProperty(groot) {}
        
        bool loadSelfFromChunk(const ByteSpan& inChunk) override
        {
            needsBinding(false);
            set(true);

            if (inChunk == "none")
            {
                fExplicitNone = true;
            }
            else if (inChunk == "non-scaling-stroke")
            {
                fEffectKind = VECTOR_EFFECT_NON_SCALING_STROKE;
            }
            else if (inChunk == "non-scaling-size")
            {
                fEffectKind = VECTOR_EFFECT_NON_SCALING_SIZE;
            }
            else if (inChunk == "non-rotation")
            {
                fEffectKind = VECTOR_EFFECT_NON_ROTATION;
            }
            else if (inChunk == "fixed-position")
            {
                fEffectKind = VECTOR_EFFECT_FIXED_POSITION;
            }
            else {
                set(false);
            }

            return true;
        }

		void drawSelf(IRenderSVG* ctx) override
		{
			if (VECTOR_EFFECT_NON_SCALING_STROKE == fEffectKind)
			{
                ctx->strokeBeforeTransform(true);
			}
		}
    };
}



