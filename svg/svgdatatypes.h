#pragma once


#include <cstdint>		// uint8_t, etc
#include <cstddef>		// nullptr_t, ptrdiff_t, size_t


#include "blend2d.h"

#include "bspan.h"
#include "membuff.h"
#include "base64.h"
#include "converters.h"
#include "svgenums.h"
#include "maths.h"
#include "viewport.h"

//
// Parsing routines for the core SVG data types
// Higher level parsing routines will use these lower level
// routines to construct visual properties and structural components
// These routines are meant to be fairly low level, independent, and fast
//


namespace waavs {
    

    
    // readNextNumber()
    // 
    // Consume the next number off the front of the chunk
    // modifying the input chunk to advance past the  removed number
    // Return true if we found a number, false otherwise
    //
    static inline bool readNextNumber(ByteSpan& s, double& outNumber) noexcept
    {
        // typical whitespace found in lists of numbers, like on paths and polylines
        static const charset nextNumWsp(",+\t\n\f\r ");          

        // clear up leading whitespace, including ','
        s = chunk_ltrim(s, nextNumWsp);
        if (!s)
            return false;

		return readNumber(s, outNumber);
    }

    static inline bool readNextFlag(ByteSpan& s, double& outNumber) noexcept
    {
        // typical whitespace found in lists of numbers, like on paths and polylines
        static charset whitespaceChars(",\t\n\f\r ");

        // clear up leading whitespace, including ','
        s = chunk_ltrim(s, whitespaceChars);

        ByteSpan numChunk{};

		if (*s == '0' || *s == '1') {
			outNumber = (double)(*s - '0');
			s++;
			return true;
		}
        
		return false;

    }

    // readNumericArguments()
    //
    // Read a list of numeric arguments as specified in the 'argTypes'
    // c, r - read a number
	// f - read a flag
    // 
	// The number of arguments read is determined by the length of the argTypes string
    //
    static bool readNumericArguments(ByteSpan& s, const char* argTypes, double* outArgs) noexcept
    {
        // typical whitespace found in lists of numbers, like on paths and polylines
        static charset segWspChars(",\t\n\f\r ");


        for (int i = 0; argTypes[i]; i++)
        {
            switch (argTypes[i])
            {
            case 'c':		// read a coordinate
            case 'r':		// read a radius
            {

                if (!readNextNumber(s, outArgs[i]))
                    return false;
            } break;

            case 'f':		// read a flag
            {
                if (!readNextFlag(s, outArgs[i]))
                    return false;
            } break;

            default:
            {
                return false;
            }
            }
        }

        return true;
    }
    
}



namespace waavs {
    static bool parseViewBox(const ByteSpan& inChunk, BLRect &r) noexcept
    {
        ByteSpan s = inChunk;
        if (!s)
            return false;

        double outArgs[4]{ 0 };
        
        if (!readNumericArguments(s, "cccc", outArgs))
            return false;

		r.reset(outArgs[0], outArgs[1], outArgs[2], outArgs[3]);

        return true;
    }
}

namespace waavs
{

    
    //==============================================================================
    // SVGLength
    // Representation of a unit based length.
    // This is the DOM specific replacement of SVGDimension
    //   Reference:  https://svgwg.org/svg2-draft/types.html#InterfaceSVGNumber
    //==============================================================================
    struct SVGLength {
        
		double fValue{ 0 };
        int fUnitType = SVG_LENGTHTYPE_UNKNOWN;
        
		SVGLength(double value, int kind) noexcept : fValue(value), fUnitType(kind) {}
		
        int unitType() const noexcept { return fUnitType; }
        
        double value(const double range) const noexcept {
            switch (fUnitType) {
                case SVG_LENGTHTYPE_NUMBER:
                    return fValue;
                case SVG_LENGTHTYPE_PERCENTAGE:
					return fValue * range / 100.0;
            }

            return 0;
        }
    };
    
    
    //==============================================================================
    // SVGAngle
    // Specification for an angle in SVG
    // 
    //==============================================================================
    enum SVGAngleUnits
	{
		SVG_ANGLETYPE_UNKNOWN = 0,
		SVG_ANGLETYPE_UNSPECIFIED = 1,
		SVG_ANGLETYPE_DEG = 2,
		SVG_ANGLETYPE_RAD = 3,
		SVG_ANGLETYPE_GRAD = 4,
        SVG_ANGLETYPE_TURN = 5,
	};
    
    static SVGAngleUnits parseAngleUnits(const ByteSpan& units)
    {
        if (!units)
			return SVG_ANGLETYPE_UNSPECIFIED;
        
		if (units == "deg")
			return SVG_ANGLETYPE_DEG;
		else if (units == "rad")
			return SVG_ANGLETYPE_RAD;
		else if (units == "grad")
			return SVG_ANGLETYPE_GRAD;
		else if (units == "turn")
			return SVG_ANGLETYPE_TURN;
		else
			return SVG_ANGLETYPE_UNKNOWN;
    }
    
    // parseAngle()
    // 
    // returns in radians
    static bool parseAngle(ByteSpan &s, double& value, SVGAngleUnits& units)
    {
        s = chunk_ltrim(s, xmlwsp);
        
        if (!s)
            return false;

		if (!readNumber(s, value))
			return false;
        
        units = parseAngleUnits(s);


        switch (units)
        {
            // If degrees or unspecified, convert degress to radians
            case SVG_ANGLETYPE_UNSPECIFIED:
            case SVG_ANGLETYPE_DEG:
                value = value * (3.14159265358979323846 / 180.0);
            break;
            
			// If radians, do nothing, already in radians
            case SVG_ANGLETYPE_RAD:
            break;
            
			// If gradians specified, convert to radians
            case SVG_ANGLETYPE_GRAD:
                value = value * (3.14159265358979323846 / 200.0);
            break;
            
			case SVG_ANGLETYPE_TURN:
				value = value * 2.0 * 3.14159265358979323846;
			break;
                
            default:
                return false;
        }

        return true;
    }
    

    


    
    //==============================================================================
    // SVGDimension
    // used for length, time, frequency, resolution, location
    //==============================================================================
    /*
		static const unsigned short SVG_LENGTHTYPE_NUMBER = 1;
    */
    
    // Turn a units indicator into an enum
    static bool parseDimensionUnits(const ByteSpan& inChunk, uint32_t &units)
    {

        if (getEnumValue(SVGDimensionEnum, inChunk, units))
            return true;

        
			units = SVG_LENGTHTYPE_UNKNOWN;
        
        return false;
    }
    
    //
    // calculateDistance()
    // 
    // To calculate distances when using a percentage value on a radius of something
    //
    static INLINE double calculateDistance(const double percentage, const double width, const double height) noexcept
    {
		return percentage / 100.0 * std::sqrt((width * width) + (height * height));
    }
    
    struct SVGDimension 
    {
        double fValue{ 0.0 };
        uint32_t fUnits{ SVG_LENGTHTYPE_NUMBER };
        bool fHasValue{ false };

        SVGDimension(const SVGDimension& other) = delete;
        
        SVGDimension() = default;
		SVGDimension(double value, unsigned short units, bool setValue = true)
            : fValue(value)
            , fUnits(units)
			, fHasValue(setValue)
        {
        }

        SVGDimension& operator=(const SVGDimension& rhs)
        {
            fValue = rhs.fValue;
            fUnits = rhs.fUnits;
            fHasValue = rhs.fHasValue;
            
            return *this;
        }

		bool isSet() const { return fHasValue; }
        double value() const { return fValue; }
        unsigned short units() const { return fUnits; }
        
		bool isPercentage() const { return fUnits == SVG_LENGTHTYPE_PERCENTAGE; }
		bool isNumber() const { return fUnits == SVG_LENGTHTYPE_NUMBER; }
        
        // Using the units and other information, calculate the actual value
        double calculatePixels(double length = 1.0, double orig = 0, double dpi = 96) const
        {
            switch (fUnits) {
            case SVG_LENGTHTYPE_UNKNOWN:     return fValue;
            case SVG_LENGTHTYPE_NUMBER:		return fValue;                  // User units and PX units are the same
            case SVG_LENGTHTYPE_PX:			return fValue;                  // User units and px units are the same
            case SVG_LENGTHTYPE_PT:			return fValue / 72.0f * dpi;
            case SVG_LENGTHTYPE_PC:			return fValue / 6.0f * dpi;
            case SVG_LENGTHTYPE_MM:			return fValue / 25.4f * dpi;
            case SVG_LENGTHTYPE_CM:			return fValue / 2.54f * dpi;
            case SVG_LENGTHTYPE_IN:			return fValue * dpi;
            case SVG_LENGTHTYPE_EMS:			return fValue * length;                 // length should represent 'em' height of font                 
			case SVG_LENGTHTYPE_EXS:		    return fValue * length * 0.52f;          // x-height, fontHeight * 0.52., assuming length is font height
            case SVG_LENGTHTYPE_PERCENTAGE:
                //double clampedVal = waavs::clamp(fValue, 0.0, 100.0);
                //return orig + ((clampedVal / 100.0f) * length);
                return orig + (fValue / 100.0f) * length;
            }

            return fValue;
        }

        bool loadFromChunk(const ByteSpan& inChunk)
        {
            ByteSpan s = chunk_ltrim(inChunk, xmlwsp);
            
            // don't change the state of 'hasValue'
            // if we previously parsed something, and now
            // we're being asked to parse again, just leave
            // the old state if there's nothing new
            if (!s)
                return false;

            
            if (!readNumber(s, fValue))
                return false;
            
            fHasValue = parseDimensionUnits(s, fUnits);
            
            return fHasValue;
        }
    };

    // This is meanto to represent the many different ways
    // a size can be specified
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
    struct SVGVariableSize
    {
        ByteSpan fSpanValue{};
        SVGSizeKind fKindOfSize{ SVG_SIZE_KIND_INVALID };
        
        uint32_t fUnits{ SVG_LENGTHTYPE_NUMBER };
        double fValue{ 0.0 };

        bool fHasValue{ false };

 
        SVGVariableSize() = default;
        SVGVariableSize(const SVGDimension& other) = delete;
/*
        SVGVariableSize(double value, unsigned short units, bool setValue = true)
            : fValue(value)
            , fUnits(units)
            , fHasValue(setValue)
        {
        }

        SVGVariableSize& operator=(const SVGDimension& rhs)
        {
            fValue = rhs.fValue;
            fUnits = rhs.fUnits;
            fHasValue = rhs.fHasValue;

            return *this;
        }
*/
        bool isSet() const { return fHasValue; }
        double value() const { return fValue; }
        unsigned short units() const { return fUnits; }

        bool parseValue(double& value, const BLFont& font, double length = 1.0, double orig = 0, double dpi = 96, SpaceUnitsKind units = SpaceUnitsKind::SVG_SPACE_USER) const
        {
            if (!isSet())
                return false;

            value = calculatePixels(font, length, orig, dpi, units);
            return true;
        }

        // Using the units and other information, calculate the actual value
        double calculatePixels(const BLFont& font, double length = 1.0, double orig = 0, double dpi = 96, SpaceUnitsKind units= SpaceUnitsKind::SVG_SPACE_USER) const
        {
            auto &fm = font.metrics();
            double fontSize = fm.size;
            float emHeight = (fm.ascent + fm.descent);
            

            
            switch (fKindOfSize)
            {

                
                case SVG_SIZE_KIND_ABSOLUTE: {
                    if (!isSet())
                        return length;

                    switch (fUnits) {
					    case SVG_SIZE_ABSOLUTE_XX_SMALL: return (3.0 / 5.0) * fontSize;
						case SVG_SIZE_ABSOLUTE_X_SMALL: return (3.0 / 4.0) * fontSize;
						case SVG_SIZE_ABSOLUTE_SMALL: return (8.0 / 9.0) * fontSize;
                        case SVG_SIZE_ABSOLUTE_MEDIUM:  return fontSize;
						case SVG_SIZE_ABSOLUTE_LARGE: return (6.0 / 5.0) * fontSize;
						case SVG_SIZE_ABSOLUTE_X_LARGE: return (3.0 / 2.0) * fontSize;
						case SVG_SIZE_ABSOLUTE_XX_LARGE: return 2.0 * fontSize;
						case SVG_SIZE_ABSOLUTE_XXX_LARGE: return 3.0 * fontSize;
                    }
                }break;

                case SVG_SIZE_KIND_LENGTH: {
                    if (!isSet())
                        return fValue;
                    
                    switch (fUnits) {
                        case SVG_LENGTHTYPE_UNKNOWN:     return fValue;
                        case SVG_LENGTHTYPE_NUMBER:
                            // User units and PX units are the same
                            if (units == SpaceUnitsKind::SVG_SPACE_OBJECT) {
								if (fValue <= 1.0) {
									return orig + (fValue * length);
								}
                                return orig + fValue;
							}

							return orig+fValue;

                        case SVG_LENGTHTYPE_PX:			return orig+fValue;                  // User units and px units are the same
                        case SVG_LENGTHTYPE_PT:			return orig+((fValue / 72.0f) * dpi);
                        case SVG_LENGTHTYPE_PC:			return ((fValue / 6.0f) * dpi);
                        case SVG_LENGTHTYPE_MM:			return ((fValue / 25.4f) * dpi);
                        case SVG_LENGTHTYPE_CM:			return ((fValue / 2.54f) * dpi);
                        case SVG_LENGTHTYPE_IN:			return (fValue * dpi);
                        case SVG_LENGTHTYPE_EMS:		return (fValue * emHeight);                 // length should represent 'em' height of font                 
                        case SVG_LENGTHTYPE_EXS:		return (fValue * fm.xHeight);          // x-height, fontHeight * 0.52., assuming length is font height
                        case SVG_LENGTHTYPE_PERCENTAGE:
                            return orig + ((fValue / 100.0f) * length);
                            break;
                    }
                }

                default:
                    return fValue;
            }

            return fValue;
        }

        bool loadFromChunk(const ByteSpan& inChunk)
        {
            fSpanValue = chunk_ltrim(inChunk, xmlwsp);

            // don't change the state of 'hasValue'
            // if we previously parsed something, and now
            // we're being asked to parse again, just leave
            // the old state if there's nothing new
            if (!fSpanValue)
                return false;
            
			// Figure out what kind of value we have based
            // on looking at the various enums
            uint32_t enumval{ 0 };
            if (fSpanValue == "math") {
                fKindOfSize = SVG_SIZE_KIND_MATH;
				fHasValue = true;

            }
            else if (getEnumValue(SVGSizeAbsoluteEnum, fSpanValue, enumval))
            {
                fKindOfSize = SVG_SIZE_KIND_ABSOLUTE;
                fUnits = enumval;
                fHasValue = true;

            }
            else if (getEnumValue(SVGSizeRelativeEnum, fSpanValue, enumval))
            {
				fKindOfSize = SVG_SIZE_KIND_RELATIVE;
                fUnits = enumval;
                fHasValue = true;

            }
            else {
                ByteSpan numValue = fSpanValue;
                if (!readNumber(numValue, fValue))
                    return false;
				fKindOfSize = SVG_SIZE_KIND_LENGTH;
                fHasValue = parseDimensionUnits(numValue, fUnits);
            }
            
            return true;
        }
    };
}







namespace waavs {
    
    static bool parseStyleAttribute(const ByteSpan & inChunk, XmlAttributeCollection &styleAttributes) noexcept
    {
        if (!inChunk)
            return false;
        
        // Turn the style element into attributes of an XmlElement, 
        // then, the caller can use that to more easily parse whatever they're
        // looking for.
        ByteSpan styleChunk = inChunk;

        if (styleChunk) {
            ByteSpan key{};
            ByteSpan value{};
            while (readNextCSSKeyValue(styleChunk, key, value))
            {
                styleAttributes.addAttribute(key, value);
            }
        }

        return true;
    }
}


//======================================================
// Definition of SVG colors
//======================================================

    // Representation of color according to CSS specification
    // https://www.w3.org/TR/css-color-4/#typedef-color
    // Over time, this structure could represent the full specification
    // but for practical purposes, we'll focus on rgb, rgba for now
    //
    //<color> = <absolute-color-base> | currentcolor | <system-color>
    //
    //<absolute-color-base> = <hex-color> | <absolute-color-function> | <named-color> | transparent
    //<absolute-color-function> = <rgb()> | <rgba()> |
    //                        <hsl()> | <hsla()> | <hwb()> |
    //                        <lab()> | <lch()> | <oklab()> | <oklch()> |
    //                        <color()>
    


namespace waavs {
    // Scan a bytespan for a sequence of hex digits
    // without using scanf. return 'true' upon success
    // false for any error.
    // The format is either
    // #RRGGBB
    // #RGB
    // Anything else is an error
    static bool hexSpanToRgba32(const ByteSpan& inSpan, BLRgba32& outValue) noexcept
    {
        outValue.value = 0;

        if (inSpan.size() == 0)
            return false;

        if (inSpan[0] != '#')
            return false;

        uint8_t r{};
        uint8_t g{};
        uint8_t b{};
        uint8_t a{0xffu};
        
        if (inSpan.size() == 4) {
            // #RGB
            r = hexToDec(inSpan[1]);
            g = hexToDec(inSpan[2]);
            b = hexToDec(inSpan[3]);

            outValue = BLRgba32(r * 17, g * 17, b * 17);			// same effect as (r<<4|r), (g<<4|g), ..

            return true;
        } 
        else if (inSpan.size() == 5) {
            // #RGBA
            r = hexToDec(inSpan[1]);
            g = hexToDec(inSpan[2]);
            b = hexToDec(inSpan[3]);
            a = hexToDec(inSpan[4]);

            outValue = BLRgba32(r * 17, g * 17, b * 17, a*17);			// same effect as (r<<4|r), (g<<4|g), ..

            return true;
        } 
        else if (inSpan.size() == 7) {
            // #RRGGBB
            r = (hexToDec(inSpan[1]) << 4) | hexToDec(inSpan[2]);
            g = (hexToDec(inSpan[3]) << 4) | hexToDec(inSpan[4]);
            b = (hexToDec(inSpan[5]) << 4) | hexToDec(inSpan[6]);

            outValue = BLRgba32(r, g, b);
            return true;
        }
        else if (inSpan.size() == 9) {
            // #RRGGBBAA
            r = (hexToDec(inSpan[1]) << 4) | hexToDec(inSpan[2]);
            g = (hexToDec(inSpan[3]) << 4) | hexToDec(inSpan[4]);
            b = (hexToDec(inSpan[5]) << 4) | hexToDec(inSpan[6]);
			a = (hexToDec(inSpan[7]) << 4) | hexToDec(inSpan[8]);
            outValue = BLRgba32(r, g, b, a);
            
            return true;
        }
        
        return false;
    }

    // Turn a 3 or 6 digit hex string into a BLRgba32 value
    // if there's an error in the conversion, a transparent color is returned
    // BUGBUG - maybe a more obvious color should be returned
    static BLRgba32 parseColorHex(const ByteSpan& chunk) noexcept
    {
        BLRgba32 res{};
        if (hexSpanToRgba32(chunk, res))
            return res;

        return BLRgba32(0);
    }


    //
    // parse a color string
    // Return a BLRgba32 value
    static double hue_to_rgb(double p, double q, double t) noexcept 
    {
        if (t < 0) t += 1;
        if (t > 1) t -= 1;
        if (t < 1 / 6.0) return p + (q - p) * 6 * t;
        if (t < 1 / 2.0) return q;
        if (t < 2 / 3.0) return p + (q - p) * (2 / 3.0 - t) * 6;
        return p;
    }

    static BLRgba32 hsl_to_rgb(double h, double s, double l) noexcept
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

        return BLRgba32(uint32_t(r * 255), uint32_t(g * 255), uint32_t(b * 255));
    }

    static BLRgba32 parseColorHsl(const ByteSpan& inChunk) noexcept
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
        SVGDimension od{255,SVG_LENGTHTYPE_NUMBER};

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
        h = hd.calculatePixels(360) / 360.0;
        s = sd.calculatePixels(100) / 100.0;
        l = ld.calculatePixels(100) / 100.0;
        //o = od.calculatePixels(255);

        auto res = hsl_to_rgb(h, s, l);
        res.setA(uint32_t(o * 255));

        return res;
    }

    // Parse rgb color. The pointer 'str' must point at "rgb(" (4+ characters).
    // This function returns gray (rgb(128, 128, 128) == '#808080') on parse errors
    // for backwards compatibility. Note: other image viewers return black instead.

    static bool parseColorRGB(const ByteSpan& inChunk, BLRgba32 &aColor)
    {
        // skip past the leading "rgb("
        ByteSpan s = inChunk;
        auto leading = chunk_token_char(s, '(');

        // s should now point to the first number
        // and 'leading' should contain 'rgb'
        // BUGBUG - we can check 'leading' to see if it's actually 'rgb'
        // but we'll just assume it is for now

        // get the numbers by separating at the ')'
        auto nums = chunk_token_char(s, ')');

        // So, now nums contains the individual numeric values, separated by ','
        // The individual numeric values are either
        // 50%
        // 50

        int i = 0;
        uint8_t rgbi[4]{};

        // Get the first token, which is red
        // if it's not there, then return gray
        auto num = chunk_token_char(nums, ',');
        if (num.size() < 1)
        {
            //aColor.reset(128, 128, 128, 0);
            aColor.reset(255, 255, 0, 255);
            return false;
        }
        
        while (num)
        {
            SVGDimension cv{};
            cv.loadFromChunk(num);


            if (cv.units() == SVG_LENGTHTYPE_PERCENTAGE)
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
                    rgbi[i] = (uint8_t)waavs::clamp(cv.value(),0.0,255.0);
            }
            i++;
            num = chunk_token_char(nums, ',');
        }

        if (i == 4)
            aColor.reset(rgbi[0], rgbi[1], rgbi[2], rgbi[3]);
        else
            aColor.reset(rgbi[0], rgbi[1], rgbi[2]);

        return true;
    }
}

namespace waavs {
    //
    // parseFont()
    // Turn a base64 encoded inlined font into a BLFontFace
    // This will typically be created in a style sheet with 
	// a @font-face rule
    //
    static bool parseFont(const ByteSpan& inChunk, BLFontFace& face)
    {
        bool success{ false };
        ByteSpan value = inChunk;;

        // figure out what kind of encoding we're dealing with
        // value starts with: 'data:image/png;base64,<base64 encoded image>
        //
        ByteSpan data = chunk_token(value, ":");
        auto mime = chunk_token(value, ";");
        auto encoding = chunk_token(value, ",");


		if (value) {
            if (encoding == "base64" && mime == "base64")
            {
                unsigned int outBuffSize = base64::getDecodeOutputSize(value.size());
                MemBuff outBuff(outBuffSize);
                
                auto decodedSize = base64::decode((const char*)value.data(), value.size(), outBuff.data());

				if (decodedSize > 0)
				{
                    BLDataView dataView{};
					dataView.reset(outBuff.data(), decodedSize);
                    
                    // create a BLFontData
                    BLFontData fontData;
                    fontData.createFromData(outBuff.data(), decodedSize);

                    
					BLResult result = face.createFromData(fontData, 0);
					if (result == BL_SUCCESS)
					{
						success = true;
					}
				}
			}
		}

        return success;
    }
}

namespace waavs {


    static bool parseMarkerOrientation(const ByteSpan& inChunk, MarkerOrientation& orient)
    {
        uint32_t value;
        
        if (getEnumValue(MarkerOrientationEnum, inChunk, value))
        {
            orient = static_cast<MarkerOrientation>(value);
            return true;
        }
        
        // Assume the orientation is an angle
        orient = MarkerOrientation::MARKER_ORIENT_ANGLE;
        
        return true;
    }
}






   


namespace waavs
{

    //
    // parsing transforms
    //
    static ByteSpan parseTransformArgs(const ByteSpan& inChunk, double* args, int maxNa, int& na)
    {
        // Now we're ready to parse the individual numbers
        static const charset numDelims = xmlwsp + ',';
        
        na = 0;

        ByteSpan s = inChunk;

        // Skip past everything until we see a '('
        s = chunk_find_char(s, '(');

        // If we got to the end of the chunk, and did not see the '('
        // then just return
        if (!s)
            return s;

        // by the time we're here, we're sitting right on top of the 
        //'(', so skip past that to get to what should be the numbers
        s++;

        // We want to create a  chunk that contains all the numbers
        // without the closing ')', so create a chunk that just
        // represents that range.
        // Start the chunk at the current position
        // and expand it after we find the ')'
        ByteSpan item = s;
        item.fEnd = item.fStart;

        // scan until the closing ')'
        s = chunk_find_char(s, ')');


        // At this point, we're either sitting at the ')' or at the end
        // of the chunk.  If we're at the end of the chunk, then we
        // didn't find the closing ')', so just return
        if (!s)
            return s;

        // We found the closing ')', so if we use the current position
        // as the end (sitting on top of the ')', the item chunk will
        // perfectly represent the numbers we want to parse.
        item.fEnd = s.fStart;

        // Create a chunk that will represent a specific number to be parsed.
        ByteSpan numChunk{};

        // Move the source chunk cursor past the ')', so that whatever
        // needs to use it next is ready to go.
        s++;



        while (item) {
            if (na >= maxNa)
                break;

            if (!readNextNumber(item, args[na]))
                break;
            na++;
        }

        return s;
    }

    static ByteSpan parseMatrix(ByteSpan& inMatrix, BLMatrix2D& m)
    {
        ByteSpan s = inMatrix;
        m.reset();  // start with identity


        double t[6];    // storage for our 6 numbers
        int na = 0;     // Number of arguments parsed

        s = parseTransformArgs(s, t, 6, na);

        if (na != 6)
            return s;

        m.reset(t[0], t[1], t[2], t[3], t[4], t[5]);

        return s;
    }


    static ByteSpan parseTranslate(const ByteSpan& inChunk, BLMatrix2D& xform)
    {
        double args[2];
        int na = 0;
        ByteSpan s = inChunk;
        s = parseTransformArgs(s, args, 2, na);
        if (na == 1)
            args[1] = 0.0;

        xform.translate(args[0], args[1]);

        return s;
    }

    static ByteSpan parseScale(const ByteSpan& inChunk, BLMatrix2D& xform)
    {
        double args[2]{ 0 };
        int na = 0;
        ByteSpan s = inChunk;

        s = parseTransformArgs(s, args, 2, na);

        if (na == 1)
            args[1] = args[0];

        xform.scale(args[0], args[1]);

        return s;
    }


    static ByteSpan parseSkewX(const ByteSpan& inChunk, BLMatrix2D& xform)
    {
        double args[1];
        int na = 0;
        ByteSpan s = inChunk;
        s = parseTransformArgs(s, args, 1, na);

        xform.skew(radians(args[0]), 0.0);

        return s;
    }

    static ByteSpan parseSkewY(const ByteSpan& inChunk, BLMatrix2D& xform)
    {
        double args[1]{ 0 };
        int na = 0;
        ByteSpan s = inChunk;
        s = parseTransformArgs(s, args, 1, na);

        xform.skew(0.0, radians(args[0]));

        return s;
    }


    static ByteSpan parseRotate(const ByteSpan& inChunk, BLMatrix2D& xform) noexcept
    {
        double args[3]{ 0 };
        int na = 0;
        ByteSpan s = inChunk;

        s = parseTransformArgs(s, args, 3, na);

        // If there was only one parameter, then the center
        // of rotation is the point 0,0
        if (na == 1)
            args[1] = args[2] = 0.0;

		double ang = radians(args[0]);
        double x = args[1];
		double y = args[2];
        
        xform.rotate(ang, x, y);

        return  s;
    }

    // parseTransform()
    // 
    // Parse a transform attribute, stuffing the results
    // into a single BLMatrix2D structure
    // This will repeatedly apply the portions that are parsed
    //
    static bool parseTransform(const ByteSpan& inChunk, BLMatrix2D& xform)
    {        
        ByteSpan s = inChunk;
        s = chunk_skip_wsp(s);
        if (!s)
            return false;
        
        xform = BLMatrix2D::makeIdentity();
        
        bool isSet = false;

        while (s)
        {
            s = chunk_skip_wsp(s);

            BLMatrix2D tm{};
            tm.reset();

            if (chunk_starts_with_cstr(s, "matrix"))
            {
                s = parseMatrix(s, tm);
                xform = tm;
                isSet = true;
            }
            else if (chunk_starts_with_cstr(s, "translate"))
            {
                s = parseTranslate(s, tm);
                xform.transform(tm);
                isSet = true;
            }
            else if (chunk_starts_with_cstr(s, "scale"))
            {
                s = parseScale(s, tm);
                xform.transform(tm);
                isSet = true;
            }
            else if (chunk_starts_with_cstr(s, "rotate"))
            {
                s = parseRotate(s, tm);
                xform.transform(tm);
                isSet = true;
            }
            else if (chunk_starts_with_cstr(s, "skewX"))
            {
                s = parseSkewX(s, tm);
                xform.transform(tm);
                isSet = true;
            }
            else if (chunk_starts_with_cstr(s, "skewY"))
            {
                s = parseSkewY(s, tm);
                xform.transform(tm);
                isSet = true;
            }
            else {
                s++;
            }

        }

        return isSet;
    }
}



//
// These are various routines that help manipulate the BLRect
// structure.  Finding corners, moving, query containment
// scaling, merging, expanding, and the like

namespace waavs {
    static inline double right(const BLRect& r) { return r.x + r.w; }
    static inline double left(const BLRect& r) { return r.x; }
    static inline double top(const BLRect& r) { return r.y; }
    static inline double bottom(const BLRect& r) { return r.y + r.h; }
    static inline BLPoint center(const BLRect& r) { return { r.x + (r.w / 2),r.y + (r.h / 2) }; }

    static inline void moveBy(BLRect& r, double dx, double dy) { r.x += dx; r.y += dy; }
    static inline void moveBy(BLRect& r, const BLPoint& dxy) { r.x += dxy.x; r.y += dxy.y; }


    static inline bool containsRect(const BLRect& a, double x, double y)
    {
        return (x >= a.x && x < a.x + a.w && y >= a.y && y < a.y + a.h);
    }

    static inline bool containsRect(const BLRect& a, const BLPoint& pt)
    {
        return containsRect(a, pt.x, pt.y);
    }

    // mergeRect()
    // 
    // Perform a union operation between a BLRect and a BLPoint
    // Return a new BLRect that represents the union.
    static inline BLRect rectMerge(const BLRect& a, const BLPoint& b)
    {
        // return a BLRect that is the union of BLRect a 
        // and BLPoint b using local temporary variables
        double x1 = std::min(a.x, b.x);
        double y1 = std::min(a.y, b.y);
        double x2 = std::max(a.x + a.w, b.x);
        double y2 = std::max(a.y + a.h, b.y);

        return { x1, y1, x2 - x1, y2 - y1 };
    }

    // mergeRect()
    // 
    // Expand the size of the rectangle such that the new rectangle
    // firts the original (a) as well as the new one (b)
    // this is a union operation.
    static inline BLRect rectMerge(const BLRect& a, const BLRect& b)
    {
        // return a BLRect that is the union of BLRect a
        // and BLRect b, using local temporary variables
        double x1 = std::min(a.x, b.x);
        double y1 = std::min(a.y, b.y);
        double x2 = std::max(a.x + a.w, b.x + b.w);
        double y2 = std::max(a.y + a.h, b.y + b.h);

        return { x1, y1, x2 - x1, y2 - y1 };
    }

    static inline void expandRect(BLRect& a, const BLPoint& b) { a = rectMerge(a, b); }
    static inline void expandRect(BLRect& a, const BLRect& b) { a = rectMerge(a, b); }
}



