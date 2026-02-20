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
#include "svgatoms.h"
#include "svgunits.h"
#include "xmlscan.h"

//
// Parsing routines for the core SVG data types
// Higher level parsing routines will use these lower level
// routines to construct visual properties and structural components
// These routines are meant to be fairly low level, independent, and fast
//
// Data types:
//   SVGLength
//   SVGDimension
//   SVGVariableSize
// 
// Parsing routines:
// parseViewBox
// parseAngleUnits
// parseAngle
// parseDimensionUnits
// calculateDistance
// parseStyleAttribute
//
// hexSpanToRgba32
// parseColorHex
// parseColorHsl
// parseColorRGB
//
// parseTransform

namespace waavs
{
    static INLINE BLRect mapRectAABB(const BLMatrix2D& m, const BLRect& r) noexcept
    {
        // Treat empty/degenerate as-is (or return empty).
        if (!(r.w > 0.0) || !(r.h > 0.0))
            return BLRect(r.x, r.y, r.w, r.h);

        const double x0 = r.x;
        const double y0 = r.y;
        const double x1 = r.x + r.w;
        const double y1 = r.y + r.h;

        // Transform 4 corners.
        BLPoint p0 = m.mapPoint(x0, y0);
        BLPoint p1 = m.mapPoint(x1, y0);
        BLPoint p2 = m.mapPoint(x1, y1);
        BLPoint p3 = m.mapPoint(x0, y1);

        double minX = p0.x, maxX = p0.x;
        double minY = p0.y, maxY = p0.y;

        auto expand = [&](const BLPoint& p) noexcept {
            if (p.x < minX) minX = p.x;
            if (p.x > maxX) maxX = p.x;
            if (p.y < minY) minY = p.y;
            if (p.y > maxY) maxY = p.y;
            };

        expand(p1);
        expand(p2);
        expand(p3);

        return BLRect(minX, minY, maxX - minX, maxY - minY);
    }

}


namespace waavs
{
    // Turn a units indicator into an enum
    // Instead of using WSEnum for this, we just do a direct comparison
    static INLINE uint32_t lengthUnitToEnum(InternedKey u) noexcept
    {
        if (!u || u == svgunits::none()) return SVG_LENGTHTYPE_NUMBER;

        if (u == svgunits::px())  return SVG_LENGTHTYPE_PX;
        if (u == svgunits::pt())  return SVG_LENGTHTYPE_PT;
        if (u == svgunits::pc())  return SVG_LENGTHTYPE_PC;
        if (u == svgunits::mm())  return SVG_LENGTHTYPE_MM;
        if (u == svgunits::cm())  return SVG_LENGTHTYPE_CM;
        if (u == svgunits::in_()) return SVG_LENGTHTYPE_IN;
        if (u == svgunits::pct()) return SVG_LENGTHTYPE_PERCENTAGE;
        if (u == svgunits::em())  return SVG_LENGTHTYPE_EMS;
        if (u == svgunits::ex())  return SVG_LENGTHTYPE_EXS;

        return SVG_LENGTHTYPE_UNKNOWN;
    }

    static bool parseDimensionUnits(const ByteSpan& inChunk, uint32_t& units)
    {
        if (inChunk.empty())
        {
            units = SVG_LENGTHTYPE_NUMBER;
            return true;
        }

        InternedKey ukey = waavs::svgunits::internUnit(inChunk);
        units = lengthUnitToEnum(ukey);

        return units != SVG_LENGTHTYPE_UNKNOWN;
    }
}



namespace waavs
{
    //==============================================================================
    // SVGLengthValue
    // Representation of a unit based length.
    // This is the DOM specific replacement of SVGDimension
    //   Reference:  https://svgwg.org/svg2-draft/types.html#InterfaceSVGNumber
    //==============================================================================
    struct SVGLengthValue
    {
        double fValue{ 0.0 };
        uint32_t fUnitType{ SVG_LENGTHTYPE_NUMBER };    // include percentage
        bool fIsSet{ false };

        SVGLengthValue() noexcept = default;
        SVGLengthValue(double value, uint32_t unitType) noexcept : fValue(value), fUnitType(unitType) {}
        SVGLengthValue(double value, uint32_t unitType, bool setIt) noexcept 
            : fValue(value), fUnitType(unitType), fIsSet(setIt) {}

        double value() const noexcept { return fValue; }
        uint32_t unitType() const noexcept { return fUnitType; }

        bool isSet() const noexcept { return fIsSet; }
        bool isPercentage() const noexcept { return fUnitType == SVG_LENGTHTYPE_PERCENTAGE; }
    };

    // This context is used when resolving length values
    struct LengthResolveCtx
    {
        double dpi{ 96.0 };             // Dots per inch for in, cm, mm, pt, pc conversions
        const BLFont* font{ nullptr };  // For em, ex calculations
        double ref{ 1.0 };              // Reference length for percentage calculations
        double origin{ 0.0 };           // Origin offset to add
        SpaceUnitsKind space{ SpaceUnitsKind::SVG_SPACE_USER };     // Which coordinate space to use
    };

    static INLINE LengthResolveCtx makeLengthCtxUser(double ref,
        double origin,
        double dpi,
        const BLFont* font,
        SpaceUnitsKind spc = SpaceUnitsKind::SVG_SPACE_USER) noexcept
    {
        LengthResolveCtx c{};
        c.ref = ref;
        c.origin = origin;
        c.dpi = dpi;
        c.font = font;
        c.space = spc;
        return c;
    }

    // Resolve an SVGLengthValue into a used length in "user units" (px in your engine).
//
// Spec notes:
// - Absolute units use 96px per inch (SVG2 / CSS pixels). :contentReference[oaicite:1]{index=1}
// - Percentages resolve against a "reference length" chosen by the property. :contentReference[oaicite:2]{index=2}
// - em/ex depend on font metrics; if ctx.font is null, you must choose a fallback
//   (I recommend treating em/ex as unresolved -> return raw number, or use a default em).
//
    static double resolveLengthUserUnits(const SVGLengthValue& L, const LengthResolveCtx& ctx) noexcept
    {
        if (!L.isSet())
            return ctx.origin;

        const double v = L.fValue;

        // If a property is operating in objectBoundingBox space, then "number" values
        // are fractions of ctx.ref (typically bbox width/height/diag depending on property).
        // Keep this rule *only* where the spec calls for objectBoundingBox units.
        if (ctx.space == SpaceUnitsKind::SVG_SPACE_OBJECT)
        {
            switch (L.fUnitType)
            {
            case SVG_LENGTHTYPE_NUMBER:     return ctx.origin + (v * ctx.ref);
            case SVG_LENGTHTYPE_PERCENTAGE: return ctx.origin + ((v / 100.0) * ctx.ref);
            default:                        break; // fall through for absolute units if you allow them here
            }
        }

        // User space (normal painting / geometry)
        switch (L.fUnitType)
        {
        default:
        case SVG_LENGTHTYPE_UNKNOWN:
        case SVG_LENGTHTYPE_NUMBER:
        case SVG_LENGTHTYPE_PX:
            return ctx.origin + v;

            // Absolute units (SVG2: 1in = 96px; 1pt=1/72in; 1pc=12pt; etc.). :contentReference[oaicite:3]{index=3}
        case SVG_LENGTHTYPE_IN: return ctx.origin + (v * ctx.dpi);
        case SVG_LENGTHTYPE_CM: return ctx.origin + (v * (ctx.dpi / 2.54));
        case SVG_LENGTHTYPE_MM: return ctx.origin + (v * (ctx.dpi / 25.4));
        case SVG_LENGTHTYPE_PT: return ctx.origin + (v * (ctx.dpi / 72.0));
        case SVG_LENGTHTYPE_PC: return ctx.origin + (v * (ctx.dpi / 6.0)); // 1pc = 12pt

            // Percentages resolve against a per-property reference length. :contentReference[oaicite:4]{index=4}
        case SVG_LENGTHTYPE_PERCENTAGE:
            return ctx.origin + ((v / 100.0) * ctx.ref);

            // Font-relative units:
            // em = computed font-size; ex ? x-height (font metric) in CSS/SVG model. :contentReference[oaicite:5]{index=5}
        case SVG_LENGTHTYPE_EMS:
        {
            if (!ctx.font) return ctx.origin + v; // fallback policy
            const auto& fm = ctx.font->metrics();
            const double em = fm.size;            // "font-size" in your BLFont
            return ctx.origin + (v * em);
        }
        case SVG_LENGTHTYPE_EXS:
        {
            if (!ctx.font) return ctx.origin + v; // fallback policy
            const auto& fm = ctx.font->metrics();
            const double ex = fm.xHeight;         // best available approximation
            return ctx.origin + (v * ex);
        }
        }
    }

    // resolveLengthOr
    // 
    // Resolve length if set; otherwise return fallback value.
    static INLINE double resolveLengthOr(const SVGLengthValue& L, const LengthResolveCtx& ctx, double fallback) noexcept
    {
        return L.isSet() ? resolveLengthUserUnits(L, ctx) : fallback;
    }



    // ==============================================================================
    // SVGNumberOrPercent
    // Representation of a number or percentage value.
    //   Reference:  https://svgwg.org/svg2-draft/types.html#InterfaceSVGNumber
    // ==============================================================================
    struct SVGNumberOrPercent
    {
        double fValue;
        bool fIsPercent;
        bool fIsSet;

        bool isSet() const noexcept { return fIsSet; }
        bool isPercent() const noexcept { return fIsPercent; }
        double value() const noexcept { return fValue; }

        double calculatedValue() const noexcept
        {
            if (fIsPercent)
                return fValue / 100.0;
            else
                return fValue;
        }
    };
}

namespace waavs
{

    // parseLengthValue()
 //
 // Parses a single <length> or <percentage> token:
 //   - optional leading whitespace
 //   - number (WAAVS readNumber grammar)
 //   - optional unit suffix:
 //       '%' OR [A-Za-z]+ OR nothing
 //   - optional trailing whitespace
 //
 // On success:
 //   - out.v, out.unit, out.set=true
 //   - 's' is advanced to the first byte after the token (including suffix),
 //     but not beyond trailing whitespace (we do trim the whitespace at end).
 //
 // On failure:
 //   - out is left unchanged
 //   - 's' is left in an unspecified advanced position (typical parser behavior).
 //
    static constexpr charset chrNotAlpha = ~chrAlphaChars;

    static INLINE bool parseLengthValue(const ByteSpan& inChunk, SVGLengthValue& out) noexcept
    {
        // Preserve 'out' on failure
        SVGLengthValue tmp = out;
        ByteSpan s = inChunk;

        // 1) Trim leading whitespace
        s = chunk_ltrim(s, chrWspChars);
        if (!s)
            return false;

        // 2) Parse number
        double v = 0.0;
        ByteSpan cur = s;
        if (!readNumber(cur, v))
            return false;

        // 3) Optional unit suffix
        uint32_t units = SVG_LENGTHTYPE_NUMBER;

        if (cur)
        {
            const uint8_t c = *cur;

            if (c == '%')
            {
                units = SVG_LENGTHTYPE_PERCENTAGE;
                ++cur;
            }
            else if (chrAlphaChars(c))
            {
                // consume alpha run without consuming the delimiter after it
                const uint8_t* uStart = cur.fStart;
                cur.skipWhile(chrAlphaChars);
                ByteSpan unitTok = ByteSpan::fromPointers( uStart, cur.fStart );

                if (!parseDimensionUnits(unitTok, units))
                    return false; // unknown unit => reject
            }
            // else: no suffix => NUMBER
        }

        // 4) Commit cursor to end-of-token and trim trailing whitespace
        s = cur;
        s = chunk_ltrim(s, chrWspChars);

        // 5) Commit output
        tmp.fValue = v;
        tmp.fUnitType = units;
        tmp.fIsSet = true;

        out = tmp;
        return true;
    }

    static INLINE bool readSVGNumberOrPercent(ByteSpan& s, SVGNumberOrPercent& out) noexcept
    {
        // leading wsp
        s = chunk_ltrim(s, chrWspChars);
        if (!s)
            return false;

        // number
        double v = 0.0;

        if (!readNumber(s, v))
            return false;

        // optional '%'
        bool isPct = false;
        if (!s.empty() && *s == '%') {
            isPct = true;
            ++s;
        }

        out.fValue = v;
        out.fIsPercent = isPct;
        out.fIsSet = true;

        return true;
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
        SVG_ANGLETYPE_GRAD = 4,
        SVG_ANGLETYPE_TURN = 5,
    };


    static SVGAngleUnits parseAngleUnits(InternedKey u)
    {
        if (!u)
            return SVG_ANGLETYPE_UNSPECIFIED;


        if (u == waavs::svgunits::deg()) return SVG_ANGLETYPE_DEG;
        if (u == waavs::svgunits::rad()) return SVG_ANGLETYPE_RAD;
        if (u == waavs::svgunits::grad()) return SVG_ANGLETYPE_GRAD;
        if (u == waavs::svgunits::turn()) return SVG_ANGLETYPE_TURN;

        return SVG_ANGLETYPE_UNKNOWN;
    }
}

namespace waavs 
{
//
// SVGTokenListView
//
// A zero-allocation forward iterator over SVG "list" attributes.
// Typical separators: whitespace and/or ','.
//
// This view can produce:
//  - number tokens (numeric lexeme only)
//  - length tokens (numeric lexeme + optional unit suffix or '%')
//
// Design goals:
//  - No allocation
//  - No copying of token text
//  - Compatible numeric grammar with WAAVS readNumber()
//  - Cursor is a ByteSpan (WAAVS idiom)
//
    struct SVGTokenListView final
    {
        ByteSpan fSrc{};    // The original source span (optional, for debugging)
        ByteSpan fCur{};    // The cursor span that advances as tokens are consumed

        // separators in SVG lists: whitespace and comma
        static const charset& sepChars() noexcept
        {
            static charset sSep = chrWspChars + ",";
            return sSep;
        }

        SVGTokenListView() noexcept = default;

        explicit SVGTokenListView(const ByteSpan& src) noexcept
        {
            reset(src);
        }

        void reset(const ByteSpan& src) noexcept
        {
            fSrc = src;
            fCur = src;
        }

        const ByteSpan& source() const noexcept { return fSrc; }
        const ByteSpan& cursor() const noexcept { return fCur; }
        ByteSpan remaining() const noexcept { return fCur; }

        bool empty() const noexcept { return fCur.empty(); }
        explicit operator bool() const noexcept { return (bool)fCur; }

        //
        // skipSeparators()
        // Skip list separators (whitespace and ',')
        //
        INLINE void skipSeparators() noexcept
        {
            fCur.skipWhile(sepChars());
        }



        //
        // nextNumberToken()
        // Return the next numeric lexeme as a ByteSpan [start..end),
        // with NO units included.
        //
        // Advances cursor to the end of the number token.
        //
        bool nextNumberToken(ByteSpan& outTok) noexcept
        {
            outTok.reset();

            skipSeparators();
            if (!fCur) return false;

            ByteSpan start = fCur;   // keep original start pointer
            double dummy = 0.0;

            if (!readNumber(fCur, dummy))
                return false;

            // fCur advanced to first byte after the number lexeme
            outTok = ByteSpan::fromPointers( start.fStart, fCur.fStart );
            return true;
        }

        //
        // nextLengthToken()
        // Return the next "length" token as a ByteSpan [start..end),
        // including optional unit suffix or '%'.
        //
        // Examples:
        //  "10"     -> "10"
        //  "10px"   -> "10px"
        //  "2.5em"  -> "2.5em"
        //  "30%"    -> "30%"
        //
        // Advances cursor to the end of token (number + suffix).
        //
        bool nextLengthToken(ByteSpan& outTok) noexcept
        {
            outTok.reset();

            skipSeparators();
            if (!fCur) return false;

            ByteSpan start = fCur;
            double dummy = 0.0;

            // Parse number portion (your readNumber leaves 'e' for em/ex)
            if (!readNumber(fCur, dummy))
                return false;

            // Parse optional unit suffix
            if (fCur)
            {
                if (*fCur == '%')
                {
                    ++fCur; // include '%'
                }
                else if (chrAlphaChars(*fCur))
                {
                    // consume [A-Za-z]+
                    const uint8_t* p = fCur.fStart;
                    const uint8_t* e = fCur.fEnd;
                    while (p < e && chrAlphaChars(*p))
                        ++p;
                    fCur.fStart = p;
                }
            }

            outTok = ByteSpan::fromPointers( start.fStart, fCur.fStart );
            return true;
        }


        //
        // nextIdentToken()
        // Sometimes SVG lists contain identifiers (ex: "none").
        // This parses an identifier token:
        //   ident := [A-Za-z_][A-Za-z0-9_-]*
        //
        // This does NOT attempt to parse CSS escapes; it's for SVG-ish keywords.
        //
        bool nextIdentToken(ByteSpan& outTok) noexcept
        {
            outTok.reset();

            skipSeparators();
            if (!fCur) return false;

            const uint8_t c0 = *fCur;
            if (!(chrAlphaChars(c0) || c0 == '_'))
                return false;

            const uint8_t* start = fCur.fStart;
            const uint8_t* end = fCur.fEnd;
            const uint8_t* p = start;

            // first char already validated
            ++p;

            while (p < end)
            {
                const uint8_t c = *p;
                if (chrAlphaChars(c) || is_digit(c) || c == '_' || c == '-')
                {
                    ++p;
                    continue;
                }
                break;
            }

            outTok = ByteSpan::fromPointers( start, p );
            fCur.fStart = p;
            return true;
        }

        //
        // skipOneTokenOrChar()
        // Best-effort forward progress on malformed inputs.
        // Tries length token, then ident token; if neither matches,
        // skips separators then one char.
        //
        bool skipOneTokenOrChar() noexcept
        {
            const uint8_t* before = fCur.fStart;

            ByteSpan tok{};
            if (nextLengthToken(tok)) return true;
            if (nextIdentToken(tok))  return true;

            skipSeparators();
            if (fCur) ++fCur;

            return fCur.fStart != before;
        }

        // peekHasMoreNumber()
        // 
        // Returns true if a number token exists ahead (after separators).
        // Does not advance the cursor.
        //
        bool peekHasMoreNumber() const noexcept
        {
            ByteSpan tmp = fCur;
            tmp.skipWhile(sepChars());
            if (!tmp) return false;

            double dummy = 0.0;
            ByteSpan t2 = tmp;
            return waavs::readNumber(t2, dummy);
        }


        //
        // isListOfNumbers()
        // 
        // Cheap detection: returns true if there is more than one numeric token.
        // (Does not allocate; scans using readNumber() twice.)
        //
        bool isListOfNumbers() const noexcept
        {
            ByteSpan tmp = fCur;
            tmp.skipWhile(sepChars());
            if (!tmp) return false;

            double dummy = 0.0;
            if (!readNumber(tmp, dummy)) return false;

            tmp.skipWhile(sepChars());
            if (!tmp) return false;

            // second number?
            ByteSpan t2 = tmp;
            return readNumber(t2, dummy);
        }



        // Convenience operators to read specific data types
        bool readANumber(double &out) noexcept
        {
            ByteSpan tok;
            if (!nextNumberToken(tok))
                return false;
            return readNumber(tok, out);
        }

        bool readAFlag(int& out) noexcept
        {
            skipSeparators();
            if (fCur.empty())
                return false;
            return readNextFlag(fCur, out);
        }
    };

    static int readNumericArguments(ByteSpan& s, const char* argTypes, double* outArgs) noexcept
    {
        SVGTokenListView listView(s);

        int i = 0;
        for (; argTypes[i]; ++i)
        {
            switch (argTypes[i])
            {
            case 'c':		// read a coordinate
            case 'r':		// read a radius
            {
                if (!listView.readANumber(outArgs[i])) {
                    s = listView.remaining();
                }
            } break;

            case 'f':		// read a flag
            {
                int aflag = 0;
                if (!listView.readAFlag(aflag)) {
                    s = listView.remaining();
                    return i;
                }

                outArgs[i] = double(aflag);

            } break;

            default:
            {
                s = listView.remaining();
                return 0;
            }
            }
        }

        return i;
    }
}


namespace waavs
{
    // parseAngle()
    // 
    // returns in radians
    static bool parseAngle(ByteSpan &s, double& value, SVGAngleUnits& units)
    {
        static charset chrNotAlpha = ~chrAlphaChars;

        s = chunk_ltrim(s, chrWspChars);
        
        if (s.empty())
            return false;

        if (!readNumber(s, value))
            return false;
        
        // After readNumber, s points to the suffix 
        // (could be unit, whitespace, comma, ')', etc.)
        s = chunk_ltrim(s, chrWspChars);

        // Capture unit identifier (deg, rad, grad, turn) if present
        ByteSpan unitSpan = chunk_token(s, chrNotAlpha);

        InternedKey ukey = unitSpan ? waavs::svgunits::internUnit(unitSpan) : InternedKey{};
        units = parseAngleUnits(ukey);


        switch (units)
        {
            // If degrees or unspecified, convert degress to radians
            case SVG_ANGLETYPE_UNSPECIFIED:
            case SVG_ANGLETYPE_DEG:
                value = value * (3.14159265358979323846 / 180.0);
            break;
            
            // If radians, do nothing, already in radians
            case SVG_ANGLETYPE_RAD:
                // already radians
            break;
            
            // If gradians specified, convert to radians
            case SVG_ANGLETYPE_GRAD:
                value = value * (3.14159265358979323846 / 200.0);
            break;
            
            case SVG_ANGLETYPE_TURN:
                value = value * (2.0 * 3.14159265358979323846);
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
    ///*
    struct SVGDimension 
    {
        double fValue{ 0.0 };
        uint32_t fUnits{ SVG_LENGTHTYPE_NUMBER };
        bool fHasValue{ false };

        SVGDimension(const SVGDimension& other) = default;
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
            ByteSpan s = chunk_ltrim(inChunk, chrWspChars);
            
            // don't change the state of 'hasValue'
            // if we previously parsed something, and now
            // we're being asked to parse again, just leave
            // the old state if there's nothing new
            if (s.empty())
                return false;

            
            if (!readNumber(s, fValue))
                return false;
            

            fHasValue = parseDimensionUnits(s, fUnits);
            
            return fHasValue;
        }
    };
    //*/

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
    //   (SVG 1.1)
    //     px, pt, pc, cm, mm, in, em, ex, 
    //   (SVG 2 CSS <length>)
    //     ch, rem, vw, vh, vmin, vmax
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
        SVGVariableSize(const SVGVariableSize& other) = delete;

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
            fSpanValue = chunk_trim(inChunk, chrWspChars);

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
        // Turn the style element into attributes of an XmlElement, 
        // then, the caller can use that to more easily parse whatever they're
        // looking for.
        ByteSpan styleChunk = chunk_ltrim(inChunk, chrWspChars);

        if (styleChunk.empty())
            return false;


        ByteSpan name{};
        ByteSpan value{};
        while (readNextCSSKeyValue(styleChunk, name, value))
        {
            styleAttributes.addValueBySpan(name, value);
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
        // value starts with: 'data:imaxsge/png;base64,<base64 encoded image>
        //
        ByteSpan data = chunk_token(value, ":");
        auto mime = chunk_token(value, ";");
        auto encoding = chunk_token(value, ",");


        if (value) {
            if (encoding == "base64" && mime == "base64")
            {
                unsigned int outBuffSize = base64::getDecodeOutputSize(value.size());
                MemBuff outBuff(outBuffSize);
                
                auto decodedSize = base64::decode(value.data(), value.size(), outBuff.data(), outBuffSize);

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
                else {
                    printf("parseFont() - Error base64 decoding font data\n");
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
    // parse a number of numeric arguments from a chunk.  The numbers
    // are delimited by whitspace, and/or ',' characters.
    // Parameters
    //   'args' is an array where the values will be stored
    //   'maxNa' is the maximum number of arguments
    //   'na' - returns the number of arguments actually retrieved
    static ByteSpan parseTransformArgs(const ByteSpan& inChunk, double* args, int maxNa, int& na)
    {
        // Now we're ready to parse the individual numbers
        //static const charset numDelims = xmlwsp + ',';
        
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
        //ByteSpan numChunk{};

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


namespace waavs {
    // State that represents the stroke-dasharray and stroke-dashoffset
    struct StrokeDashState
    {
        bool fHasArray{ false };
        bool fHasOffset{ false };

        // Raw as-authored values (preserve units)
        std::vector<SVGLengthValue> fArray{};   // each entry is a <length> or <percentage>
        SVGLengthValue fOffset{};               // <length> or <percentage>

        void clearArray() noexcept
        {
            fArray.clear();
            fHasArray = false;
        }

        void clearOffset() noexcept
        {
            fOffset = SVGLengthValue{};
            fHasOffset = false;
        }
    };

    static bool parseStrokeDashArray(const ByteSpan& inChunk,
        std::vector<SVGLengthValue>& outArray,
        bool& outIsNone) noexcept
    {
        outArray.clear();
        outIsNone = false;

        ByteSpan s = chunk_trim(inChunk, chrWspChars);
        if (!s) {
            // empty attribute -> treat as "none" (no dash array set)
            outIsNone = true;
            return true;
        }

        // Keyword "none"
        if (s == "none") {
            outIsNone = true;
            return true;
        }

        SVGTokenListView view(s);

        ByteSpan tok{};
        while (view.nextLengthToken(tok))
        {
            SVGLengthValue dim{};
            //if (!dim.loadFromChunk(tok))
            //    return false;

            // SVG disallows negative dash lengths
            if (dim.value() < 0.0)
                return false;

            outArray.push_back(dim);
        }

        // If we got no tokens, treat as none-ish
        if (outArray.empty()) {
            outIsNone = true;
            return true;
        }

        // If there is trailing junk, you can choose strict or permissive:
        // Strict: return false if non-separator remains
        // Permissive: ignore remaining garbage if any progress made
        ByteSpan rem = view.remaining();
        rem.skipWhile(SVGTokenListView::sepChars());
        if (rem) {
            // permissive: ignore; strict: return false;
            // return false;
        }

        return true;
    }

    static bool parseStrokeDashOffset(const ByteSpan& inChunk, SVGLengthValue& outOffset) noexcept
    {
        ByteSpan s = chunk_trim(inChunk, chrWspChars);
        if (!s) {
            // empty -> treat as not set
            outOffset = SVGLengthValue{};
            return false;
        }

        SVGLengthValue dim{};
        if (!parseLengthValue(s, dim))
            return false;

        // dashoffset may be negative; keep as-is.
        outOffset = dim;
        return true;
    }


}

// More helpers
namespace waavs
{
    // Parse helper for attributes into SVGLengthValue (permissive single-token)
    static INLINE SVGLengthValue parseLengthAttr(const ByteSpan& attr) noexcept
    {
        SVGLengthValue out{};
        ByteSpan s = chunk_trim(attr, chrWspChars);
        if (!s) return out;

        (void)parseLengthValue(s, out);
        return out;
    }

    // Resolve helper: resolve L against given ref and origin in USER space
    static INLINE bool resolveIfSet(const SVGLengthValue& L,
        double& ioValue,
        double ref,
        double origin,
        double dpi,
        const BLFont* font) noexcept
    {
        if (!L.isSet())
            return false;

        LengthResolveCtx ctx = makeLengthCtxUser(ref, origin, dpi, font);
        ioValue = resolveLengthUserUnits(L, ctx);
        return true;
    }
}