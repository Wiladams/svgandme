#pragma once


#include <cstdint>		// uint8_t, etc
#include <cstddef>		// nullptr_t, ptrdiff_t, size_t


#include "blend2d.h"
#include "bspan.h"
#include "base64.h"

//
// Parsing routines for the core SVG data types
// Higher level parsing routines will use these lower level
// routines to construct visual properties and structural components
// These routines are meant to be fairly low level, independent, and fast
//


namespace waavs {
    // parseNumber()
    //
    // Parse a number from the given chunk, advancing the chunk
    // to beyond where we found the number.
    // Assumption:  We're sitting at beginning of a number,
    // with no whitespace
    static bool parseNumber(ByteSpan& s, double &value) noexcept
    {
        double sign = 1.0;
        double res = 0.0;
        long long intPart = 0;
        uint64_t fracPart = 0;
        bool hasIntPart = false;
        bool hasFracPart = false;

        // Parse optional sign
        if (*s == '+') {
            s++;
        }
        else if (*s == '-') {
            sign = -1;
            s++;
        }

        // Parse integer part
        if (chrDecDigits[*s]) {

            intPart = chunk_to_u64(s);

            res = (double)intPart;
            hasIntPart = true;
        }

        // Parse fractional part.
        if (*s == '.') {
            s++; // Skip '.'
            auto sentinel = s.fStart;

            if (chrDecDigits(*s)) {
                fracPart = chunk_to_u64(s);
                auto ending = s.fStart;

                ptrdiff_t diff = ending - sentinel;
                res = res + ((double)fracPart) / (double)powd((double)10, double(diff));
                hasFracPart = true;
            }
        }

        // A valid number should have integer or fractional part.
        if (!hasIntPart && !hasFracPart)
            return false;


        // Parse optional exponent
        if (*s == 'e' || *s == 'E') {
            long long expPart = 0;
            s++; // skip 'E'

            double expSign = 1.0;
            if (*s == '+') {
                s++;
            }
            else if (*s == '-') {
                expSign = -1.0;
                s++;
            }

            if (chrDecDigits[*s]) {
                expPart = chunk_to_u64(s);
                res = res * powd(10, double(expSign * double(expPart)));
            }
        }

        value = res * sign;
        
        return true;
    }


    
    // parseNextNumber()
    // 
    // Consume the next number off the front of the chunk
    // modifying the input chunk to advance past the  removed number
    // Return true if we found a number, false otherwise
    //
    static inline bool parseNextNumber(ByteSpan& s, double& outNumber) noexcept
    {
        // typical whitespace found in lists of numbers, like on paths and polylines
        static charset whitespaceChars(",\t\n\f\r ");          

        // clear up leading whitespace, including ','
        s = chunk_ltrim(s, whitespaceChars);

        ByteSpan numChunk{};

		return parseNumber(s, outNumber);
    }
}


namespace waavs
{
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
		SVG_ANGLETYPE_GRAD = 4
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
		else
			return SVG_ANGLETYPE_UNKNOWN;
    }
    
    // returns in radians
    static bool parseAngle(ByteSpan &s, double& value, SVGAngleUnits& units)
    {
        s = chunk_ltrim(s, xmlwsp);
        
        if (!s)
            return false;

		if (!parseNumber(s, value))
			return false;
        
        units = parseAngleUnits(s);


        switch (units)
        {
            // If degrees or unspecified, convert degress to radians
            case SVG_ANGLETYPE_UNSPECIFIED:
            case SVG_ANGLETYPE_DEG:
                value = value * (3.14159265358979323846 / 180.0);
            break;
            
			// If radians, do nothing
            case SVG_ANGLETYPE_RAD:
            break;
            
			// If gradians specified, convert to radians
            case SVG_ANGLETYPE_GRAD:
                value = value * (3.14159265358979323846 / 200.0);
            break;
            
            default:
                return false;
        }

        return true;
    }
    
    //==============================================================================
    // SVGLength
    // Representation of a unit based length.
    // This is the DOM specific replacement of SVGDimension
    //==============================================================================
    enum SVGLengthRelativeUnits {
        SVG_LENGTH_RELATIVE_UNKNOWN = 0,
        SVG_LENGTH_RELATIVE_EM      = 1,
		SVG_LENGTH_RELATIVE_EX      = 2,
		SVG_LENGTH_RELATIVE_CH      = 3,
		SVG_LENGTH_RELATIVE_REM     = 4,
        SVG_LENGTH_RELATIVE_VW      = 5,
		SVG_LENGTH_RELATIVE_VH      = 6,
		SVG_LENGTH_RELATIVE_VMIN    = 7,
		SVG_LENGTH_RELATIVE_VMAX    = 8,
    };
    
    enum SVGLengthAbsoluteUnits {
		SVG_LENGTH_ABSOLUTE_UNKNOWN = 0,
		SVG_LENGTH_ABSOLUTE_CM = 1, // centimeters
		SVG_LENGTH_ABSOLUTE_MM,     // millimeters
		SVG_LENGTH_ABSOLUTE_IN,     // inches
		SVG_LENGTH_ABSOLUTE_PT,     // points
		SVG_LENGTH_ABSOLUTE_PC,     // Picas
		SVG_LENGTH_ABSOLUTE_PX,     // pixels
		SVG_LENGTH_ABSOLUTE_Q       // quarter-millimeters
    };
    
	enum SVGLengthUnits
	{
		SVG_LENGTHTYPE_UNKNOWN = 0,
		SVG_LENGTHTYPE_USER = 1,
		SVG_LENGTHTYPE_PERCENTAGE,
		SVG_LENGTHTYPE_CM,
        SVG_LENGTHTYPE_EMS,
        SVG_LENGTHTYPE_EXS,
        SVG_LENGTHTYPE_IN,
		SVG_LENGTHTYPE_MM,
        SVG_LENGTHTYPE_PC,
		SVG_LENGTHTYPE_PT,
        SVG_LENGTHTYPE_PX,
	};

    enum SVGDimensionUnits
    {
		SVG_UNITS_UNKNOWN = 0,
        SVG_UNITS_USER,
        SVG_UNITS_PERCENT,
        SVG_UNITS_CM,
        SVG_UNITS_EM,
        SVG_UNITS_EX,
        SVG_UNITS_IN,
        SVG_UNITS_MM,
        SVG_UNITS_PC,
        SVG_UNITS_PT,
        SVG_UNITS_PX,
    };
    
    //
    // parseLengthUnits
    // %
    // em
    // ex
	// px
    // pt
	// pc
	// cm
	// mm
	// in
    //		SVG_LENGTHTYPE_UNKNOWN = 0,

        
    static bool parseLengthUnits(ByteSpan& s, SVGLengthUnits &units) noexcept
    {
		// If no units specified, then it is a number
        // in user units
         if (!s)
         {
             units = SVG_LENGTHTYPE_USER;   return true;
         }

        if (s[0] == 'p' && s[1] == 'x')
        {
            s += 2; units = SVG_LENGTHTYPE_PX;
        }
        else if (s[0] == 'p' && s[1] == 't')
        {
            s += 2; units = SVG_LENGTHTYPE_PT;
        }
        else if (s[0] == 'p' && s[1] == 'c')
        {
            s += 2; units = SVG_LENGTHTYPE_PC;
        }
        else if (s[0] == 'm' && s[1] == 'm')
        {
            s += 2; units = SVG_LENGTHTYPE_MM;
        }
        else if (s[0] == 'c' && s[1] == 'm')
        {
            s += 2; units = SVG_LENGTHTYPE_CM;
        }
        else if (s[0] == 'i' && s[1] == 'n')
        {
            s += 2; units = SVG_LENGTHTYPE_IN;
        }
        else if (s[0] == '%')
        {
            s += 1; units = SVG_LENGTHTYPE_PERCENTAGE;
        }
        else if (s[0] == 'e' && s[1] == 'm')
        {
            s += 2; units= SVG_LENGTHTYPE_EMS;
        }
        else if (s[0] == 'e' && s[1] == 'x')
		{
            s += 2; units = SVG_LENGTHTYPE_EXS;

		}
		else
		{
			return false;
		}

        return true;

    }

    
    //==============================================================================
    // SVGDimension
    // used for length, time, frequency, resolution, location
    //==============================================================================


    // Turn a units indicator into an enum
    static SVGDimensionUnits parseDimensionUnits(const ByteSpan& units)
    {
        // if the chunk is blank, then return user units
        if (!units)
            return SVG_UNITS_USER;

        if (units[0] == 'p' && units[1] == 'x')
            return SVG_UNITS_PX;
        else if (units[0] == 'p' && units[1] == 't')
            return SVG_UNITS_PT;
        else if (units[0] == 'p' && units[1] == 'c')
            return SVG_UNITS_PC;
        else if (units[0] == 'm' && units[1] == 'm')
            return SVG_UNITS_MM;
        else if (units[0] == 'c' && units[1] == 'm')
            return SVG_UNITS_CM;
        else if (units[0] == 'i' && units[1] == 'n')
            return SVG_UNITS_IN;
        else if (units[0] == '%')
            return SVG_UNITS_PERCENT;
        else if (units[0] == 'e' && units[1] == 'm')
            return SVG_UNITS_EM;
        else if (units[0] == 'e' && units[1] == 'x')
            return SVG_UNITS_EX;

        return SVG_UNITS_USER;
    }

    
    struct SVGDimension 
    {
        double fValue;
        SVGDimensionUnits fUnits;
        bool fIsSet{ false };

        SVGDimension()
            : fValue(0.0f)
            , fUnits(SVGDimensionUnits::SVG_UNITS_USER)
        {
        }

        SVGDimension(const SVGDimension& other)
            :fValue(other.fValue)
            , fUnits(other.fUnits)
        {}

		SVGDimension(double value, SVGDimensionUnits units, bool setValue = true)
            : fValue(value)
            , fUnits(units)
        {
            fIsSet = setValue;
        }

        SVGDimension& operator=(const SVGDimension& rhs)
        {
            fValue = rhs.fValue;
            fUnits = rhs.fUnits;

            return *this;
        }

		bool isSet() const { return fIsSet; }
        
        double value() const { return fValue; }
        SVGDimensionUnits units() const { return fUnits; }
        
        // Using the units and other information, calculate the actual value
        double calculatePixels(double length = 1.0, double orig = 0, double dpi = 96) const
        {
            switch (fUnits) {
            case SVG_UNITS_USER:		return fValue;
            case SVG_UNITS_PX:			return fValue;
            case SVG_UNITS_PT:			return fValue / 72.0f * dpi;
            case SVG_UNITS_PC:			return fValue / 6.0f * dpi;
            case SVG_UNITS_MM:			return fValue / 25.4f * dpi;
            case SVG_UNITS_CM:			return fValue / 2.54f * dpi;
            case SVG_UNITS_IN:			return fValue * dpi;
            case SVG_UNITS_EM:			return fValue * length;                 // attr.fontSize;
                //case SVG_UNITS_EX:			return fValue * attr.fontSize * 0.52f; // x-height of Helvetica.
            case SVG_UNITS_PERCENT:	
                double clampedVal = waavs::clamp(fValue, 0.0, 100.0);
                return orig + ((clampedVal / 100.0f) * length);
            }

            return fValue;
        }

        bool loadFromChunk(const ByteSpan& inChunk)
        {
            ByteSpan s = chunk_ltrim(inChunk, xmlwsp);
            
            if (inChunk.size() == 0)
            {
                //printf("SVGDimension::loadSelfFromChunk; ERROR - inChunk is blank\n");
                fIsSet = false;
                return false;
            }
            
            if (!parseNumber(s, fValue))
                return false;
            
            //ByteSpan numChunk;
            //auto nextPart = scanNumber(s, numChunk);
            //fValue = toDouble(numChunk);
            fUnits = parseDimensionUnits(s);
            fIsSet = true;
            
            return true;
        }
    };

}



namespace waavs {
    // SVGExtendMode
    // Supports the various ExtendMode kinds that BLPattern supports
    static bool parseExtendMode(const ByteSpan& inChunk, BLExtendMode& outMode)
    {
        if (!inChunk)
            return false;

        // These are the simple ones
        if (inChunk == "pad")
			outMode = BL_EXTEND_MODE_PAD;
		else if (inChunk == "repeat")
			outMode = BL_EXTEND_MODE_REPEAT;
		else if (inChunk == "reflect")
			outMode = BL_EXTEND_MODE_REFLECT;

        // These are more complex
		else if (inChunk == "pad-x-pad-y")
			outMode = BL_EXTEND_MODE_PAD_X_PAD_Y;
		else if (inChunk == "pad-x-repeat-y")
			outMode = BL_EXTEND_MODE_PAD_X_REPEAT_Y;
		else if (inChunk == "pad-x-reflect-y")
			outMode = BL_EXTEND_MODE_PAD_X_REFLECT_Y;
		else if (inChunk == "repeat-x-repeat-y")
			outMode = BL_EXTEND_MODE_REPEAT_X_REPEAT_Y;
		else if (inChunk == "repeat-x-pad-y")
			outMode = BL_EXTEND_MODE_REPEAT_X_PAD_Y;
		else if (inChunk == "repeat-x-reflect-y")
			outMode = BL_EXTEND_MODE_REPEAT_X_REFLECT_Y;
		else if (inChunk == "reflect-x-reflect-y")
			outMode = BL_EXTEND_MODE_REFLECT_X_REFLECT_Y;
		else if (inChunk == "reflect-x-pad-y")
			outMode = BL_EXTEND_MODE_REFLECT_X_PAD_Y;
		else if (inChunk == "reflect-x-repeat-y")
			outMode = BL_EXTEND_MODE_REFLECT_X_REPEAT_Y;
		else
			return false;

        
        return true;
    }
}


namespace waavs {
    
    static bool parseStyleAttribute(const ByteSpan & inChunk, XmlAttributeCollection &styleAttributes)
    {
        if (!inChunk)
            return false;
        
        // Turn the style element into attributes of an XmlElement, 
        // then, the caller can use that to more easily parse whatever they're
        // looking for.
        ByteSpan styleChunk = inChunk;

        
        if (styleChunk) {
            // use CSSInlineIterator to iterate through the key value pairs
            // creating a visual attribute, using the gSVGPropertyCreation map
            CSSInlineStyleIterator iter(styleChunk);

            while (iter.next())
            {
                std::string name = std::string((*iter).first.fStart, (*iter).first.fEnd);
                if (!name.empty() && (*iter).second)
                {
                    auto value = (*iter).second;
					styleAttributes.addAttribute(name, value);
                }

                //iter++;
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
    static inline uint8_t  hexCharToDecimal(const uint8_t value) noexcept
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

    static bool hexSpanToDecimal(const ByteSpan& inSpan, BLRgba32& outValue) noexcept
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
            uint8_t r = (hexCharToDecimal(inSpan[1]) << 4) | hexCharToDecimal(inSpan[2]);
            uint8_t g = (hexCharToDecimal(inSpan[3]) << 4) | hexCharToDecimal(inSpan[4]);
            uint8_t b = (hexCharToDecimal(inSpan[5]) << 4) | hexCharToDecimal(inSpan[6]);

            outValue = BLRgba32(r, g, b);
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
        if (hexSpanToDecimal(chunk, res))
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
        {
            //aColor.reset(128, 128, 128, 0);
            aColor.reset(255, 255, 0, 255);
            return false;
        }
        
        while (num)
        {
            SVGDimension cv{};
            cv.loadFromChunk(num);


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
                    rgbi[i] = (uint8_t)waavs::clamp(cv.value(),0.0,255.0);
            }
            i++;
            num = chunk_token(nums, ",");
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
                //char * outBuff{ new char[outBuffSize]{} };
                
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
    //
    // parseImage()
    // 
    // Turn a base64 encoded inlined image into a BLImage
    // We are handed the attribute, typically coming from a 
    // href of an <image> tag, or as a lookup for a fill, or stroke, 
    // paint attribute.
    // What we're passed are the contents of the 'url()'.  
    // 
    // Example: <image id="image_textures" x="0" y="0" width="1024" height="768" xlink:href="data:image/jpeg;base64,/9j/...
    //
    static bool parseImage(const ByteSpan& inChunk, BLImage& img)
    {
        static int n = 1;
        bool success{ false };
        ByteSpan value = inChunk;

        // figure out what kind of encoding we're dealing with
        // value starts with: 'data:image/png;base64,<base64 encoded image>
        //
        ByteSpan data = chunk_token(value, ":");
        auto mime = chunk_token(value, ";");
        auto encoding = chunk_token(value, ",");


        if (encoding == "base64")
        {
            // allocate some memory to decode into
            unsigned int outBuffSize = base64::getDecodeOutputSize(value.size());
            MemBuff outBuff(outBuffSize);

            auto decodedSize = base64::decode((const char*)value.data(), value.size(), outBuff.data());

            // BUGBUG - write chunk to file for debugging
            //ByteSpan outChunk = ByteSpan(outBuff.data(), outBuff.size());
            //char filename[256]{ 0 };
			//sprintf_s(filename, "base64_%02d.dat", n++);
            //writeChunkToFile(outChunk, filename);
            
            if (decodedSize < 1) {
                printf("parseImage: Error in base54::decode, decoded \n");
                return false;
            }
            
			// See if it's a format that blend2d can deal with using its
            // own codecs
            BLResult res = img.readFromData(outBuff.data(), outBuff.size());
            success = (res == BL_SUCCESS);
            
            // If we didn't succeed in decoding, then try any specilized methods of decoding
            // we might have.
            if (!success) {
                if (mime == "image/gif")
                {
                    printf("parseImage:: trying to decode GIF\n");
					// try to decode it as a gif
                    //BLResult res = img.readFromData(outBuff.data(), outBuff.size());
                    //success = (res == BL_SUCCESS);
                }
            }
            
        }

        return success;
    }
}


   
//================================================
// SVGTransform
// Transformation matrix
//================================================

namespace waavs
{

    //
    // parsing transforms
    //
    static ByteSpan parseTransformArgs(const ByteSpan& inChunk, double* args, int maxNa, int& na)
    {
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

        // Now we're ready to parse the individual numbers
        auto numDelims = xmlwsp + ',';

        while (item) {
            if (na >= maxNa)
                break;

            if (!parseNextNumber(item, args[na]))
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
    inline double right(const BLRect& r) { return r.x + r.w; }
    inline double left(const BLRect& r) { return r.x; }
    inline double top(const BLRect& r) { return r.y; }
    inline double bottom(const BLRect& r) { return r.y + r.h; }
    inline BLPoint center(const BLRect& r) { return { r.x + (r.w / 2),r.y + (r.h / 2) }; }

    inline void moveBy(BLRect& r, double dx, double dy) { r.x += dx; r.y += dy; }
    inline void moveBy(BLRect& r, const BLPoint& dxy) { r.x += dxy.x; r.y += dxy.y; }


    inline bool containsRect(const BLRect& a, double x, double y)
    {
        return (x >= a.x && x < a.x + a.w && y >= a.y && y < a.y + a.h);
    }

    inline bool containsRect(const BLRect& a, const BLPoint& pt)
    {
        return containsRect(a, pt.x, pt.y);
    }

    // mergeRect()
    // 
    // Perform a union operation between a BLRect and a BLPoint
    // Return a new BLRect that represents the union.
    inline BLRect rectMerge(const BLRect& a, const BLPoint& b)
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
    inline BLRect rectMerge(const BLRect& a, const BLRect& b)
    {
        // return a BLRect that is the union of BLRect a
        // and BLRect b, using local temporary variables
        double x1 = std::min(a.x, b.x);
        double y1 = std::min(a.y, b.y);
        double x2 = std::max(a.x + a.w, b.x + b.w);
        double y2 = std::max(a.y + a.h, b.y + b.h);

        return { x1, y1, x2 - x1, y2 - y1 };
    }

    inline void expandRect(BLRect& a, const BLPoint& b) { a = rectMerge(a, b); }
    inline void expandRect(BLRect& a, const BLRect& b) { a = rectMerge(a, b); }
}



