#pragma once

#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstdint>		// uint8_t, etc
#include <cstddef>		// nullptr_t, ptrdiff_t, size_t

#include "xmlscan.h"

#include "svgcss.h"
#include "svgcolors.h"
#include "irendersvg.h"

#include "fastavxbase64.h"
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
                size_t outBuffSize = BASE64_DECODE_OUT_SIZE(value.size());
                char * outBuff{ new char[outBuffSize]{} };
                
                auto decodedSize = fast_avx2_base64_decode((char *)outBuff, (const char *)value.data(), value.size());
                //auto decodedSize = base64_decode((const char*)value.data(), value.size(), outBuff);

				if (decodedSize > 0)
				{
                    BLDataView dataView{};
					dataView.reset((uint8_t *)outBuff, decodedSize);
                    
                    // create a BLFontData
                    BLFontData fontData;
                    fontData.createFromData(outBuff, decodedSize);

                    
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
    // href of an <image> tage, or as a lookup for a 
    // fill, or stroke, paint attribute.
    // What we're passed are the contents of the 'url()'.  So, only
    // what's in between the parenthesis
    //
    static bool parseImage(const ByteSpan& inChunk, BLImage& img)
    {
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
            // BUGBUG - This is not needed if base64 decoder
            // can deal with whitespace
            // allocate some memory to cleanup the buffer
            uint8_t* inBuff{ new uint8_t[value.size()]{} };
            size_t inCharCount = 0;

            for (int i=0; i< value.size(); i++)

            {
                if (!xmlwsp.contains(value[i]))
                {
                    inBuff[inCharCount] = value[i];
                    inCharCount++;
                }
            }
            ByteSpan inBuffChunk(inBuff, inCharCount);

            //size_t outBuffSize = BASE64_DECODE_OUT_SIZE(chunk_size(inBuffChunk));
            size_t outBuffSize = chunk_size(value);
            
            // allocate some memory to decode into
            uint8_t* outBuff{ new uint8_t[outBuffSize]{} };
            ByteSpan outChunk(outBuff, outBuffSize);

            if (mime == "image/png")
            {
                //auto decodedSize = base64_decode((const char*)inBuffChunk.fStart, chunk_size(inBuffChunk), outBuff);
                auto decodedSize = fast_avx2_base64_decode((char*)outBuff, (const char*)inBuffChunk.fStart, inBuffChunk.size());
                
                if (decodedSize>0)
                {
                    BLResult res = img.readFromData(outBuff, decodedSize);
                    success = (res == BL_SUCCESS);
                }
            }
            else if ((mime == "image/jpeg") || (mime == "image/jpg"))
            {
                //auto decodedSize = base64_decode((const char*)inBuffChunk.fStart, chunk_size(inBuffChunk), outBuff);
                auto decodedSize = fast_avx2_base64_decode((char*)outBuff, (const char*)inBuffChunk.fStart, inBuffChunk.size());

				outChunk = ByteSpan(outBuff, decodedSize);
                //writeChunkToFile(outChunk, "base64.jpg");
                
                if (decodedSize > 0)
                {
                    BLResult res = img.readFromData(outBuff, decodedSize);
                    success = (res == BL_SUCCESS);
                }
            }
            
            delete[] outBuff;
            delete[] inBuff;
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






