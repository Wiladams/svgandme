#pragma once

#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstdint>		// uint8_t, etc
#include <cstddef>		// nullptr_t, ptrdiff_t, size_t


#include "bspan.h"
#include "base64.h"



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
    
    // The angle struct holds the angle in radians
	// It can parse and convert to other units
    struct SVGAngle
    {
        double fValue{ 0 };
        bool fIsSet{ false };
		SVGAngleUnits fUnits{ SVG_ANGLETYPE_UNSPECIFIED };
        
        SVGAngle() :fIsSet(false), fValue(0) {}
		SVGAngle(double value, SVGAngleUnits units = SVG_ANGLETYPE_DEG) :fIsSet(true), fValue(value), fUnits(units) {}
        SVGAngle(const ByteSpan& inChunk) {
            loadFromChunk(inChunk);
        }
        
        bool isSet() const { return fIsSet; }

        bool valueInSpecifiedUnits(double& v) { v = fValue; return true; }
        double value() const { return fValue; }
        SVGAngleUnits unitType() const { return fUnits; }
        
        double radians() const { return fValue; }

        
        bool loadFromChunk(const ByteSpan& inChunk)
        {
            ByteSpan s = chunk_ltrim(inChunk, xmlwsp);

            if (s.size() == 0)
            {
                //printf("SVGAngle::loadSelfFromChunk; ERROR - inChunk is blank\n");
                fIsSet = false;
                return false;
            }

            ByteSpan numChunk;
            auto nextPart = scanNumber(s, numChunk);
            fValue = toDouble(numChunk);
            fUnits = parseAngleUnits(nextPart);

			// Convert from the units to radians
			switch (fUnits)
			{
            case SVG_ANGLETYPE_UNSPECIFIED:
			case SVG_ANGLETYPE_DEG:
				fValue = fValue * (3.14159265358979323846 / 180.0);
				break;
			case SVG_ANGLETYPE_RAD:
				break;
			case SVG_ANGLETYPE_GRAD:
				fValue = fValue * (3.14159265358979323846 / 200.0);
				break;
			default:
				//printf("SVGAngle::loadSelfFromChunk; ERROR - unknown units\n");
				fIsSet = false;
				return false;
			}
            
            fIsSet = true;
            return true;
        }
    };
    
    //==============================================================================
    // SVGLength
    // Representation of a unit based length.
    // This is the DOM specific replacement of SVGDimension
    //==============================================================================
        
	enum SVGLengthUnits
	{
		SVG_LENGTHTYPE_UNKNOWN = 0,
		SVG_LENGTHTYPE_NUMBER = 1,
		SVG_LENGTHTYPE_PERCENTAGE = 2,
		SVG_LENGTHTYPE_EMS = 3,
		SVG_LENGTHTYPE_EXS = 4,
		SVG_LENGTHTYPE_PX = 5,
		SVG_LENGTHTYPE_CM = 6,
		SVG_LENGTHTYPE_MM = 7,
		SVG_LENGTHTYPE_IN = 8,
		SVG_LENGTHTYPE_PT = 9,
		SVG_LENGTHTYPE_PC = 10
	};

    static SVGLengthUnits parseLengthUnits(const ByteSpan& units)
    {
		// If no units specified, then it is a number
        // in user units
        if (!units)
            return SVG_LENGTHTYPE_NUMBER;

        if (units[0] == '%')
            return SVG_LENGTHTYPE_PERCENTAGE;
        else if (units[0] == 'e')
        {
            if (units[1] == 'm')
                return SVG_LENGTHTYPE_EMS;
            else if (units[1] == 'x')
                return SVG_LENGTHTYPE_EXS;
        } else if (units[0] == 'p')
        {
            if (units[1] == 'x')
                return SVG_LENGTHTYPE_PX;
            else if (units[1] == 't')
                return SVG_LENGTHTYPE_PT;
            else if (units[1] == 'c')
                return SVG_LENGTHTYPE_PC;
        } else if (units == "cm")
            return SVG_LENGTHTYPE_CM;
        else if (units == "mm")
            return SVG_LENGTHTYPE_MM;
        else if (units == "in")
            return SVG_LENGTHTYPE_IN;

        return SVG_LENGTHTYPE_UNKNOWN;

    }
    //==============================================================================
    // SVGDimension
    // used for length, time, frequency, resolution, location
    //==============================================================================
    enum SVGDimensionUnits
    {
        SVG_UNITS_USER,
        SVG_UNITS_PX,
        SVG_UNITS_PT,
        SVG_UNITS_PC,
        SVG_UNITS_MM,
        SVG_UNITS_CM,
        SVG_UNITS_IN,
        SVG_UNITS_PERCENT,
        SVG_UNITS_EM,
        SVG_UNITS_EX
    };

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
            
            ByteSpan numChunk;
            auto nextPart = scanNumber(s, numChunk);
            fValue = toDouble(numChunk);
            fUnits = parseDimensionUnits(nextPart);
            fIsSet = true;
            
            return true;
        }
    };

}

// Specific types of attributes

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

            while (iter)
            {
                std::string name = std::string((*iter).first.fStart, (*iter).first.fEnd);
                if (!name.empty() && (*iter).second)
                {
                    auto value = (*iter).second;
					styleAttributes.addAttribute(name, value);
                }

                iter++;
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
    /*
    <color> = <absolute-color-base> | currentcolor | <system-color>

    <absolute-color-base> = <hex-color> | <absolute-color-function> | <named-color> | transparent
    <absolute-color-function> = <rgb()> | <rgba()> |
                            <hsl()> | <hsla()> | <hwb()> |
                            <lab()> | <lch()> | <oklab()> | <oklch()> |
                            <color()>
    */


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

        return BLRgba32(r * 255, g * 255, b * 255);
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
        h = hd.calculatePixels(360) / 360.0;
        s = sd.calculatePixels(100) / 100.0;
        l = ld.calculatePixels(100) / 100.0;
        //o = od.calculatePixels(255);

        auto res = hsl_to_rgb(h, s, l);
        res.setA(o * 255);

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
                    rgbi[i] = (uint8_t)waavs::clamp(cv.value(),0.0,255.0);
            }
            i++;
            num = chunk_token(nums, ",");
        }

        if (i == 4)
            return BLRgba32(rgbi[0], rgbi[1], rgbi[2], rgbi[3]);

        return BLRgba32(rgbi[0], rgbi[1], rgbi[2]);
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
                unsigned int outBuffSize = base64::getOutputSize(value.size());
                memBuff outBuff(outBuffSize);
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
    // Turn a base64 encoded inlined image into a BLImage
    // We are handed the attribute, typically coming from a 
    // href of an <image> tag, or as a lookup for a 
    // fill, or stroke, paint attribute.
    // What we're passed are the contents of the 'url()'.  So, only
    // what's in between the parenthesis
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
            unsigned int outBuffSize = base64::getOutputSize(value.size());
            memBuff outBuff(outBuffSize);

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
            
            // BUGBUG - Add gif as well
			// First try to decode it to see if we got a valid image
            BLResult res = img.readFromData(outBuff.data(), outBuff.size());
            success = (res == BL_SUCCESS);
            
            // If we didn't get it decoded, then try any specilized methods of decoding
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


    static ByteSpan parseRotate(const ByteSpan& inChunk, BLMatrix2D& xform)
    {
        double args[3]{ 0 };
        int na = 0;
        ByteSpan s = inChunk;

        s = parseTransformArgs(s, args, 3, na);

        if (na == 1)
            args[1] = args[2] = 0.0;

        xform.rotate(radians(args[0]), args[1], args[2]);

        return  s;
    }

    // Parse a transform attribute, stuffing the results
    // into a single BLMatrix2D structure
    // This will repeatedly apply the portions that are parsed
    static bool parseTransform(const ByteSpan& inChunk, BLMatrix2D& xform)
    {
        
        ByteSpan s = inChunk;
        xform = BLMatrix2D::makeIdentity();
        //xform.reset();
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






