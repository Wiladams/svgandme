#pragma once


#include <cstdint>		// uint8_t, etc
#include <cstddef>		// nullptr_t, ptrdiff_t, size_t


#include <blend2d/blend2d.h>

#include "xml_scan.h"
#include "xml_attribute_collection.h"

#include "lang_memory.h"
#include "core_base64.h"
#include "core_geometry.h"
#include "wg_matrix3x3.h"
#include "converters.h"
#include "svgenums.h"
#include "maths.h"
#include "svgatoms.h"
#include "svgunits.h"



//
// Parsing routines for the core SVG data types
// Higher level parsing routines will use these lower level
// routines to construct visual properties and structural components
// These routines are meant to be fairly low level, independent, and fast
//
// Data types:
//   SVGLength
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

namespace waavs
{
    // mapRectAABB
    // 
    // Given what is a typical Current Transformation Matrix (CTM) and a rectangle, 
    // compute the axis-aligned bounding box of the transformed rectangle.
    // This is typically done when you want to figure out the physical
    // pixel rectangle that corresponds to a user-space rectangle after applying the CTM.
    static INLINE WGRectD mapRectAABB(const WGMatrix3x3 & m, const WGRectD& r) noexcept
    {
        // Treat empty/degenerate as-is (or return empty).
        if (!(r.w > 0.0) || !(r.h > 0.0))
            return WGRectD{ r.x, r.y, r.w, r.h };

        const double x0 = r.x;
        const double y0 = r.y;
        const double x1 = r.x + r.w;
        const double y1 = r.y + r.h;

        // Transform 4 corners.
        WGPointD p0 = m.mapPoint(x0, y0);
        WGPointD p1 = m.mapPoint(x1, y0);
        WGPointD p2 = m.mapPoint(x1, y1);
        WGPointD p3 = m.mapPoint(x0, y1);

        double minX = p0.x, maxX = p0.x;
        double minY = p0.y, maxY = p0.y;

        auto expand = [&](const WGPointD& p) noexcept {
            if (p.x < minX) minX = p.x;
            if (p.x > maxX) maxX = p.x;
            if (p.y < minY) minY = p.y;
            if (p.y > maxY) maxY = p.y;
            };

        expand(p1);
        expand(p2);
        expand(p3);

        return WGRectD{ minX, minY, maxX - minX, maxY - minY };
    }

}


namespace waavs
{
    // Turn a units indicator into an enum
    // Instead of using WSEnum for this, we just do a direct comparison
    static INLINE uint32_t lengthUnit_to_enum(InternedKey u) noexcept
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


    static bool dimensionUnits_parse(const ByteSpan& inChunk, uint32_t& units) noexcept
    {
        if (inChunk.empty())
        {
            units = SVG_LENGTHTYPE_NUMBER;
            return true;
        }

        InternedKey ukey = waavs::svgunits::internUnit(inChunk);
        uint32_t parsed = lengthUnit_to_enum(ukey);

        if (parsed == SVG_LENGTHTYPE_UNKNOWN)
            return false;

        units = parsed;
        return true;
    }
}


namespace waavs
{
    // ==============================================================================
    // SVGNumberOrPercent
    // Representation of a number or percentage value.
    //   Reference:  https://svgwg.org/svg2-draft/types.html#InterfaceSVGNumber
    // ==============================================================================
    struct SVGNumberOrPercent
    {
        double fValue{ 0 };
        bool fIsPercent{ false };
        bool fIsSet{ false };

        bool isSet() const noexcept { return fIsSet; }
        bool isPercent() const noexcept { return fIsPercent; }
        double value() const noexcept { return fValue; }

        void reset() noexcept
        {
            fValue = 0.0;
            fIsPercent = false;
            fIsSet = false;
        }

        double normalizedValue() const noexcept
        {
            if (fIsPercent)
                return fValue / 100.0;
            else
                return fValue;
        }
    };


    static INLINE bool numberOrPercent_read(ByteSpan& s, SVGNumberOrPercent& out) noexcept
    {
        ByteSpan src = bspan_ltrim_spaces(s);
        if (!src)
            return false;

        double v = 0.0;
        if (!number_read(src, v))
            return false;

        bool isPct = false;
        if (!src.empty() && *src == '%') {
            isPct = true;
            ++src;
        }

        out = SVGNumberOrPercent{ v, isPct, true };
        s = src;
        return true;
    }



    static INLINE bool numberOrPercent_parse(const ByteSpan& inChunk, SVGNumberOrPercent& out) noexcept
    {
        SVGNumberOrPercent tmp{};
        ByteSpan s = inChunk;

        if (!numberOrPercent_read(s, tmp))
            return false;

        bspan_skip_spaces(s);
        if (!s.empty())
            return false; // trailing garbage

        out = tmp;
        return true;
    }

    // resolveNumberOrPercent
    // Given a number or percent, and a range, return the resolved value.
    // If the number is a percent, it is multiplied by the range.
    // If the number is a number, it is returned as-is.
    // If the number is not set, the fallback value is returned.
    //
    static INLINE double numberOrPercent_resolve(const SVGNumberOrPercent& norp, const double range, const double fallback) noexcept
    {
        if (norp.isSet())
        {
            if (norp.isPercent())
                return norp.normalizedValue() * range;

            return norp.normalizedValue();
        }

        return fallback;
    }


    static INLINE double numberOrPercent_resolveRange(const SVGNumberOrPercent& v,
        double range,
        double fallback = 0.0) noexcept
    {
        return numberOrPercent_resolve(v, range, fallback);
    }

    static INLINE double numberOrPercent_resolvePosition(const SVGNumberOrPercent& v,
        double origin,
        double range,
        double fallback = 0.0) noexcept
    {
        return origin + numberOrPercent_resolve(v, range, fallback);
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
        SVGLengthValue(double value, uint32_t uType) noexcept : fValue(value), fUnitType(uType) {}
        SVGLengthValue(double value, uint32_t uType, bool setIt) noexcept 
            : fValue(value), fUnitType(uType), fIsSet(setIt) {}

        double value() const noexcept { return fValue; }
        uint32_t unitType() const noexcept { return fUnitType; }

        bool isSet() const noexcept { return fIsSet; }
        bool isPercentage() const noexcept { return fUnitType == SVG_LENGTHTYPE_PERCENTAGE; }
    };

    static constexpr charset chrNotAlpha = ~chrAlphaChars;

    //==============================================================================
    // lengthValue_read()
    //
    // Reads a single <length> or <percentage> from the current cursor.
    //
    // On success:
    //   - advances 's' to the byte immediately following the token
    //   - fills 'out'
    //   - returns true
    //
    // On failure:
    //   - leaves 'out' unchanged
    //   - returns false
    //==============================================================================

    static INLINE bool lengthValue_read(ByteSpan& s, SVGLengthValue& out) noexcept
    {
        SVGLengthValue tmp = {};

        ByteSpan cur = s;
        bspan_ltrim_spaces(cur);
        if (!cur)
            return false;

        double value = 0.0;
        if (!number_read(cur, value))
            return false;

        uint32_t units = SVG_LENGTHTYPE_NUMBER;

        if (cur)
        {
            if (*cur == '%')
            {
                units = SVG_LENGTHTYPE_PERCENTAGE;
                ++cur;
            }
            else if (chrAlphaChars(*cur))
            {
                ByteSpan unitTok = bspan_read_while(cur, chrAlphaChars);

                if (!dimensionUnits_parse(unitTok, units))
                    return false;
            }
        }

        tmp.fValue = value;
        tmp.fUnitType = units;
        tmp.fIsSet = true;

        out = tmp;
        s = cur;

        return true;
    }

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

    //==============================================================================
    // lengthValue_parse()
    //
    // Parses an entire SVG length value.
    // Leading/trailing whitespace is permitted.
    // Any trailing non-whitespace causes failure.
    //==============================================================================
    static INLINE bool lengthValue_parse(const ByteSpan& inChunk, SVGLengthValue& out) noexcept
    {
        SVGLengthValue tmp = {};
        ByteSpan s = inChunk;

        if (!lengthValue_read(s, tmp))
            return false;

        bspan_ltrim_spaces(s);

        if (!s.empty())
            return false;

        out = tmp;
        return true;
    }


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
    //   Recommend: treat em/ex as unresolved -> return raw number, or use a default em.
    //
    static double resolveLengthUserUnits(const SVGLengthValue& L, const LengthResolveCtx& ctx) noexcept = delete;

    static double lengthValue_resolve(const SVGLengthValue& L, const LengthResolveCtx& ctx) noexcept
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
            const double ex = fm.x_height;         // best available approximation
            return ctx.origin + (v * ex);
        }
        }
    }

    // resolveLengthOr
    // 
    // Resolve length if set; otherwise return fallback value.
    static INLINE double resolveLengthOr(const SVGLengthValue& L, const LengthResolveCtx& ctx, double fallback) noexcept
    {
        return L.isSet() ? lengthValue_resolve(L, ctx) : fallback;
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
        ioValue = lengthValue_resolve(L, ctx);
        return true;
    }


}

namespace waavs
{
    // Convert an SVGLengthValue into SVGNumberOrPercent.
    //
    // Rules:
    // - NUMBER stays NUMBER
    // - PERCENTAGE stays PERCENTAGE
    // - PX stays NUMBER
    // - absolute units (in, cm, mm, pt, pc) are converted to NUMBER using dpi
    // - em/ex are converted to NUMBER if font is available
    // - if em/ex are used and font is null:
    //     * fail if fontFallbackToRaw == false
    //     * otherwise treat raw value as NUMBER
    //
    // This is intended for compact storage in places where you only want
    // number-or-percent rather than carrying the larger burden.
    static INLINE bool lengthValueToNumberOrPercent(const SVGLengthValue& inVal,
        SVGNumberOrPercent& outVal,
        double dpi = 96.0,
        const BLFont* font = nullptr,
        bool fontFallbackToRaw = true) noexcept
    {
        if (!inVal.isSet())
            return false;

        SVGNumberOrPercent tmp{};
        tmp.fIsSet = true;

        switch (inVal.unitType())
        {
        case SVG_LENGTHTYPE_NUMBER:
            tmp.fValue = inVal.value();
            tmp.fIsPercent = false;
            break;

        case SVG_LENGTHTYPE_PERCENTAGE:
            tmp.fValue = inVal.value();
            tmp.fIsPercent = true;
            break;

        case SVG_LENGTHTYPE_PX:
            tmp.fValue = inVal.value();
            tmp.fIsPercent = false;
            break;

        case SVG_LENGTHTYPE_IN:
            tmp.fValue = inVal.value() * dpi;
            tmp.fIsPercent = false;
            break;

        case SVG_LENGTHTYPE_CM:
            tmp.fValue = inVal.value() * (dpi / 2.54);
            tmp.fIsPercent = false;
            break;

        case SVG_LENGTHTYPE_MM:
            tmp.fValue = inVal.value() * (dpi / 25.4);
            tmp.fIsPercent = false;
            break;

        case SVG_LENGTHTYPE_PT:
            tmp.fValue = inVal.value() * (dpi / 72.0);
            tmp.fIsPercent = false;
            break;

        case SVG_LENGTHTYPE_PC:
            tmp.fValue = inVal.value() * (dpi / 6.0);
            tmp.fIsPercent = false;
            break;

        case SVG_LENGTHTYPE_EMS:
            if (font != nullptr)
            {
                const auto& fm = font->metrics();
                tmp.fValue = inVal.value() * fm.size;
                tmp.fIsPercent = false;
                break;
            }

            if (!fontFallbackToRaw)
                return false;

            tmp.fValue = inVal.value();
            tmp.fIsPercent = false;
            break;

        case SVG_LENGTHTYPE_EXS:
            if (font != nullptr)
            {
                const auto& fm = font->metrics();
                tmp.fValue = inVal.value() * fm.x_height;
                tmp.fIsPercent = false;
                break;
            }

            if (!fontFallbackToRaw)
                return false;

            tmp.fValue = inVal.value();
            tmp.fIsPercent = false;
            break;

        default:
            return false;
        }

        outVal = tmp;
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


    static SVGAngleUnits angleUnits_parse(InternedKey u)
    {
        if (!u)
            return SVG_ANGLETYPE_UNSPECIFIED;


        if (u == waavs::svgunits::deg()) return SVG_ANGLETYPE_DEG;
        if (u == waavs::svgunits::rad()) return SVG_ANGLETYPE_RAD;
        if (u == waavs::svgunits::grad()) return SVG_ANGLETYPE_GRAD;
        if (u == waavs::svgunits::turn()) return SVG_ANGLETYPE_TURN;

        return SVG_ANGLETYPE_UNKNOWN;
    }

    // parseAngle()
// 
// returns in radians
    static bool parseAngle(ByteSpan& s, double& value, SVGAngleUnits& units)
    {
        static charset chrNotAlpha = ~chrAlphaChars;

        bspan_skip_spaces(s);

        if (s.empty())
            return false;

        if (!number_read(s, value))
            return false;

        // After readNumber, s points to the suffix 
        // (could be unit, whitespace, comma, ')', etc.)
        bspan_ltrim_spaces(s);

        // Capture unit identifier (deg, rad, grad, turn) if present
        ByteSpan unitSpan = bspan_read_until(s, chrNotAlpha);

        InternedKey ukey = unitSpan ? waavs::svgunits::internUnit(unitSpan) : InternedKey{};
        units = angleUnits_parse(ukey);

        // the angle value is returned as radians
        // we use the units to perform a conversion if necessary
        switch (units)
        {
            // If degrees or unspecified, convert degress to radians
        case SVG_ANGLETYPE_UNSPECIFIED:
        case SVG_ANGLETYPE_DEG:
            value = value * (kDegToRad);
            break;

            // If radians, do nothing, already in radians
        case SVG_ANGLETYPE_RAD:
            // already radians
            break;

            // If gradians specified, convert to radians
            // Gradians are a unit where 100 gradians = 90 degrees = pi/2 radians
        case SVG_ANGLETYPE_GRAD:
            value = value * (kPi / 200.0);
            break;

            // Turns are a percentage of a full rotation
        case SVG_ANGLETYPE_TURN:
            value = value * (Pi2);
            break;

        default:
            return false;
        }

        return true;
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
            bspan_ltrim(fCur, sepChars());
        }



        //
        // nextNumberToken()
        // 
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

            if (!number_read(fCur, dummy))
                return false;

            // fCur advanced to first byte after the number lexeme
            outTok = ByteSpan::fromPointers( start.begin(), fCur.begin());
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
            if (!number_read(fCur, dummy))
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
                    const uint8_t* p = fCur.begin();
                    const uint8_t* e = fCur.end();
                    while (p < e && chrAlphaChars(*p))
                        ++p;
                    fCur.resetStart(p);
                }
            }

            outTok = ByteSpan::fromPointers( start.begin(), fCur.begin());
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

            const uint8_t* start = fCur.begin();
            const uint8_t* end = fCur.end();
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
            fCur.resetStart(p);
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
            const uint8_t* before = fCur.begin();

            ByteSpan tok{};
            if (nextLengthToken(tok)) return true;
            if (nextIdentToken(tok))  return true;

            skipSeparators();
            if (fCur) ++fCur;

            return fCur.begin() != before;
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
            bspan_ltrim(tmp,sepChars());
            if (!tmp) return false;

            double dummy = 0.0;
            if (!number_read(tmp, dummy)) return false;

            bspan_ltrim(tmp, sepChars());
            if (!tmp) return false;

            // second number?
            ByteSpan t2 = tmp;
            return number_read(t2, dummy);
        }



        // Convenience operators to read specific data types
        bool readANumber(double &out) noexcept
        {
            ByteSpan tok;
            if (!nextNumberToken(tok))
                return false;
            return number_read(tok, out);
        }

    };

    // Listview helpers for reading numeric arguments from SVG lists.
    static INLINE void svgList_skipSeparators(ByteSpan& s) noexcept
    {
        static charset sep = chrWspChars + ",";
        bspan_ltrim(s, sep);
    }

    static INLINE bool svgList_readNumber(ByteSpan& s, double& out) noexcept
    {
        ByteSpan cur = s;
        svgList_skipSeparators(cur);

        if (!number_read(cur, out))
            return false;

        s = cur;
        return true;
    }

    /*
    static INLINE bool svgList_readLength(ByteSpan& s, SVGLengthValue& out) noexcept
    {
        ByteSpan cur = s;
        svgList_skipSeparators(cur);

        if (!lengthValue_read(cur, out))
            return false;

        s = cur;
        return true;
    }
    */

    /*
    static INLINE bool svgList_readFlag(ByteSpan& s, int& out) noexcept
    {
        ByteSpan cur = s;
        svgList_skipSeparators(cur);

        if (!readNextFlag(cur, out))
            return false;

        s = cur;
        return true;
    }
    */

    /*
    // Read a sequence of numeric arguments from a list, 
    // given a format string.
    static int readNumericArguments(ByteSpan& s, const char* argTypes, double* outArgs) noexcept
    {
        ByteSpan cur = s;

        int i = 0;
        for (; argTypes[i]; ++i)
        {
            switch (argTypes[i])
            {
            case 'c':
            case 'r':
            {
                if (!svgList_readNumber(cur, outArgs[i])) {
                    s = cur;
                    return i;
                }
            } break;

            case 'f':
            {
                int flag = 0;
                if (!svgList_readFlag(cur, flag)) {
                    s = cur;
                    return i;
                }

                outArgs[i] = double(flag);
            } break;

            default:
                s = cur;
                return 0;
            }
        }

        s = cur;
        return i;
    }
    */

}


namespace waavs
{



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
        SVGLengthValue fLength{};
        uint32_t fKeyword{ 0 };
        bool fHasValue{ false };

        uint32_t fUnits{ SVG_LENGTHTYPE_NUMBER };
        double fValue{ 0.0 };

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
                    LengthResolveCtx ctx = makeLengthCtxUser(length, orig, dpi, &font, units);
                    return lengthValue_resolve(fLength, ctx);

                }

                default:
                    return fValue;
            }

            return fValue;
        }

        bool loadFromChunk(const ByteSpan& inChunk)
        {
            fSpanValue = inChunk;
            bspan_trim(fSpanValue, chrWspChars);

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
                SVGLengthValue L{};
                if (!lengthValue_parse(fSpanValue, L))
                    return false;

                fKindOfSize = SVG_SIZE_KIND_LENGTH;
                fLength = L;
                fHasValue = true;

            }
            
            return true;
        }
    };
}



namespace waavs {
    // readNextCSSKeyValue()
    // 
    // Properties are separated by ';'
    // values are separated from the key with ':'
    // Ex: <tagname style="stroke:black;fill:white" />
    // Return
    //   true - if a valid key/value pair was found
    //      in this case, key, and value will be populated
    //   false - if no key/value pair was found, or end of string
    //      in this case, key, and value will be undefined
    //
    static INLINE bool readNextCSSKeyValue(
        ByteSpan& src,
        ByteSpan& key,
        ByteSpan& value) noexcept
    {
        return parameter_read_next(src, key, value, ';', ':');
    }

    /*
    static bool readNextCSSKeyValue(ByteSpan& src, ByteSpan& key, ByteSpan& value, const unsigned char fieldDelimeter = ';', const unsigned char keyValueSeparator = ':') noexcept
    {

        // Trim leading whitespace to begin
        bspan_skip_spaces(src);

        // If the string is now blank, return immediately
        if (!src)
            return false;

        // peel off a key/value pair by taking a token up to the fieldDelimeter
        // BUGBUG - should be able to use read_identifier here
        value = bspan_read_until(src, fieldDelimeter);

        // Now, separate the key from the value using the keyValueSeparator
        key = bspan_read_until(value, keyValueSeparator);

        // trim the key and value fields of whitespace
        bspan_trim(key, chrWspChars);
        bspan_trim(value, chrWspChars);

        return true;
    }
    */

    static bool parseStyleAttribute(const ByteSpan & inChunk, XmlAttributeCollection &styleAttributes) noexcept
    {   
        // Turn the style element into attributes of an XmlElement, 
        // then, the caller can use that to more easily parse whatever they're
        // looking for.
        ByteSpan styleChunk = inChunk;
        bspan_ltrim_spaces(styleChunk);

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


//
// These are various routines that help manipulate the WGRectD
// structure.  Finding corners, moving, query containment
// scaling, merging, expanding, and the like

namespace waavs {
    // State that represents the stroke-dasharray and stroke-dashoffset
    struct StrokeDashState
    {
        bool fHasArray{ false };
        bool fHasOffset{ false };

        // Raw as-authored values (preserve units)
        std::vector<float> fArray{};   // each entry is a <length> or <percentage>
        float fOffset{};               // <length> or <percentage>

        void clearArray() noexcept
        {
            fArray.clear();
            fHasArray = false;
        }

        void clearOffset() noexcept
        {
            fOffset = 0;
            fHasOffset = false;
        }

        void reset() noexcept 
        { 
            clearArray(); 
            clearOffset(); 
        }
    };

    static bool parseStrokeDashArray(const ByteSpan& inChunk, std::vector<float>& outArray, bool& outIsNone) noexcept
    {
        outArray.clear();
        outIsNone = false;


        ByteSpan s = inChunk;
        bspan_trim(s, chrWspChars);
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

        double dummy = 0.0f;
        while (number_list_read_next(s, dummy))
        {
            // SVG disallows negative dash lengths
            if (dummy < 0.0)
                return false;

            outArray.push_back(static_cast<float>(dummy));
        }

        // If we got no tokens, treat as none-ish
        if (outArray.empty()) {
            outIsNone = true;
            return true;
        }

        return true;
    }

    static bool parseStrokeDashOffset(const ByteSpan& inChunk, SVGLengthValue& outOffset) noexcept
    {
        ByteSpan s = inChunk;
        bspan_trim(s, chrWspChars);
        if (!s) {
            // empty -> treat as not set
            outOffset = SVGLengthValue{};
            return false;
        }

        SVGLengthValue dim{};
        if (!lengthValue_parse(s, dim))
            return false;

        // dashoffset may be negative; keep as-is.
        outOffset = dim;
        return true;
    }


}

