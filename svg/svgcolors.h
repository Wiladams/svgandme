#pragma once

#include <map>
#include <unordered_map>
#include <string>

#include "blend2d.h"
#include "bspan.h"


namespace waavs
{
    // Database of SVG colors
    // BUGBUG - it might be better if these used float instead of byte values
    // Then they can be converted to various forms as needed.
    // Note:  Everything is in lowercase.  So, when looking up a key
    // the caller should ensure their key is lowercase first
    // https://www.w3.org/TR/SVG11/types.html#ColorKeywords
    //
    // 
    
    // getSVGColorByName
    //
    // Returns a color, based on a name (case insensitive).  If the name is not found in the
	// database, it returns a default color (gray).
    static BLRgba32 getSVGColorByName(const ByteSpan &colorName) noexcept
    {

        static std::unordered_map<ByteSpan, BLRgba32, ByteSpanInsensitiveHash, ByteSpanCaseInsensitive> svgcolors =
        {
            {("white"),  BLRgba32(255, 255, 255)},
            {("ivory"), BLRgba32(255, 255, 240)},
            {("lightyellow"), BLRgba32(255, 255, 224)},
            {("mintcream"), BLRgba32(245, 255, 250)},
            {("azure"), BLRgba32(240, 255, 255)},
            {("snow"), BLRgba32(255, 250, 250)},
            {("honeydew"), BLRgba32(240, 255, 240)},
            {("floralwhite"), BLRgba32(255, 250, 240)},
            {("ghostwhite"), BLRgba32(248, 248, 255)},
            {("lightcyan"), BLRgba32(224, 255, 255)},
            {("lemonchiffon"), BLRgba32(255, 250, 205)},
            {("cornsilk"), BLRgba32(255, 248, 220)},
            {("lightgoldenrodyellow"), BLRgba32(250, 250, 210)},
            {("aliceblue"), BLRgba32(240, 248, 255)},
            {("seashell"), BLRgba32(255, 245, 238)},
            {("oldlace"), BLRgba32(253, 245, 230)},
            {("whitesmoke"), BLRgba32(245, 245, 245)},
            {("lavenderblush"), BLRgba32(255, 240, 245)},
            {("beige"), BLRgba32(245, 245, 220)},
            {("linen"), BLRgba32(250, 240, 230)},
            {("papayawhip"), BLRgba32(255, 239, 213)},
            {("blanchedalmond"), BLRgba32(255, 235, 205)},
            {("antiquewhite"), BLRgba32(250, 235, 215)},
            {("yellow"), BLRgba32(255, 255, 0)},
            {("mistyrose"), BLRgba32(255, 228, 225)},
            {("lavender"), BLRgba32(230, 230, 250)},
            {("bisque"), BLRgba32(255, 228, 196)},
            {("moccasin"), BLRgba32(255, 228, 181)},
            {("palegoldenrod"), BLRgba32(238, 232, 170)},
            {("khaki"), BLRgba32(240, 230, 140)},
            {("navajowhite"), BLRgba32(255, 222, 173)},
            {("aquamarine"), BLRgba32(127, 255, 212)},
            {("paleturquoise"), BLRgba32(175, 238, 238)},
            {("wheat"), BLRgba32(245, 222, 179)},
            {("peachpuff"), BLRgba32(255, 218, 185)},
            {("palegreen"), BLRgba32(152, 251, 152)},
            {("greenyellow"), BLRgba32(173, 255, 47)},
            {("gainsboro"), BLRgba32(220, 220, 220)},
            {("powderblue"), BLRgba32(176, 224, 230)},
            {("lightgreen"), BLRgba32(144, 238, 144)},
            {("lightgray"), BLRgba32(211, 211, 211)},
            {("chartreuse"), BLRgba32(127, 255, 0)},
            {("gold"), BLRgba32(255, 215, 0)},
            {("lightblue"), BLRgba32(173, 216, 230)},
            {("lawngreen"), BLRgba32(124, 252, 0)},
            {("pink"), BLRgba32(255, 192, 203)},
            {("aqua"), BLRgba32(0, 255, 255)},
            {("cyan"), BLRgba32(0, 255, 255)},
            {("lightpink"), BLRgba32(255, 182, 193)},
            {("thistle"), BLRgba32(216, 191, 216)},
            {("lightskyblue"), BLRgba32(135, 206, 250)},
            {("lightsteelblue"), BLRgba32(176, 196, 222)},
            {("skyblue"), BLRgba32(135, 206, 235)},
            {("silver"), BLRgba32(192, 192, 192)},
            {("springgreen"), BLRgba32(0, 255, 127)},
            {("mediumspringgreen"), BLRgba32(0, 250, 154)},
            {("turquoise"), BLRgba32(64, 224, 208)},
            {("burlywood"), BLRgba32(222, 184, 135)},
            {("tan"), BLRgba32(210, 180, 140)},
            {("yellowgreen"), BLRgba32(154, 205, 50)},
            {("lime"), BLRgba32(0, 255, 0)},
            {("mediumaquamarine"), BLRgba32(102, 205, 170)},
            {("mediumturquoise"), BLRgba32(72, 209, 204)},
            {("darkkhaki"), BLRgba32(189, 183, 107)},
            {("lightsalmon"), BLRgba32(255, 160, 122)},
            {("plum"), BLRgba32(221, 160, 221)},
            {("sandybrown"), BLRgba32(244, 164, 96)},
            {("darkseagreen"), BLRgba32(143, 188, 143)},
            {("orange"), BLRgba32(255, 165, 0)},
            {("darkgray"), BLRgba32(169, 169, 169)},
            {("goldenrod"), BLRgba32(218, 165, 32)},
            {("darksalmon"), BLRgba32(233, 150, 122)},
            {("darkturquoise"), BLRgba32(0, 206, 209)},
            {("limegreen"), BLRgba32(50, 205, 50)},
            {("violet"), BLRgba32(238, 130, 238)},
            {("deepskyblue"), BLRgba32(0, 191, 255)},
            {("darkorange"), BLRgba32(255, 140, 0)},
            {("salmon"), BLRgba32(250, 128, 114)},
            {("rosybrown"), BLRgba32(188, 143, 143)},
            {("lightcoral"), BLRgba32(240, 128, 128)},
            {("coral"), BLRgba32(255, 127, 80)},
            {("mediumseagreen"), BLRgba32(60, 179, 113)},
            {("lightseagreen"), BLRgba32(32, 178, 170)},
            {("cornflowerblue"), BLRgba32(100, 149, 237)},
            {("cadetblue"), BLRgba32(95, 158, 160)},
            {("peru"), BLRgba32(205, 133, 63)},
            {("hotpink"), BLRgba32(255, 105, 180)},
            {("orchid"), BLRgba32(218, 112, 214)},
            {("palevioletred"), BLRgba32(219, 112, 147)},
            {("darkgoldenrod"), BLRgba32(184, 134, 11)},
            {("lightslategray"), BLRgba32(119, 136, 153)},
            {("tomato"), BLRgba32(255, 99, 71)},
            {("gray"), BLRgba32(128, 128, 128)},
            {("dodgerblue"), BLRgba32(30, 144, 255)},
            {("mediumpurple"), BLRgba32(147, 112, 219)},
            {("olivedrab"), BLRgba32(107, 142, 35)},
            {("slategray"), BLRgba32(112, 128, 144)},
            {("chocolate"), BLRgba32(210, 105, 30)},
            {("steelblue"), BLRgba32(70, 130, 180)},
            {("olive"), BLRgba32(128, 128, 0)},
            {("mediumslateblue"), BLRgba32(123, 104, 238)},
            {("indianred"), BLRgba32(205, 92, 92)},
            {("mediumorchid"), BLRgba32(186, 85, 211)},
            {("seagreen"), BLRgba32(46, 139, 87)},
            {("darkcyan"), BLRgba32(0, 139, 139)},
            {("forestgreen"), BLRgba32(34, 139, 34)},
            {("royalblue"), BLRgba32(65, 105, 225)},
            {("dimgray"), BLRgba32(105, 105, 105)},
            {("orangered"), BLRgba32(255, 69, 0)},
            {("slateblue"), BLRgba32(106, 90, 205)},
            {("teal"), BLRgba32(0, 128, 128)},
            {("darkolivegreen"), BLRgba32(85, 107, 47)},
            {("sienna"), BLRgba32(160, 82, 45)},
            {("green"), BLRgba32(0, 128, 0)},
            {("darkorchid"), BLRgba32(153, 50, 204)},
            {("saddlebrown"), BLRgba32(139, 69, 19)},
            {("deeppink"), BLRgba32(255, 20, 147)},
            {("blueviolet"), BLRgba32(138, 43, 226)},
            {("magenta"), BLRgba32(255, 0, 255)},
            {("fuchsia"), BLRgba32(255, 0, 255)},
            {("darkslategray"), BLRgba32(47, 79, 79)},
            {("darkgreen"), BLRgba32(0, 100, 0)},
            {("darkslateblue"), BLRgba32(72, 61, 139)},
            {("brown"), BLRgba32(165, 42, 42)},
            {("mediumvioletred"), BLRgba32(199, 21, 133)},
            {("crimson"), BLRgba32(220, 20, 60)},
            {("firebrick"), BLRgba32(178, 34, 34)},
            {("red"), BLRgba32(255, 0, 0)},
            {("darkviolet"), BLRgba32(148, 0, 211)},
            {("darkmagenta"), BLRgba32(139, 0, 139)},
            {("purple"), BLRgba32(128, 0, 128)},
            {("rebeccapurple"),BLRgba32(102,51,153)},
            {("midnightblue"), BLRgba32(25, 25, 112)},
            {("darkred"), BLRgba32(139, 0, 0)},
            {("maroon"), BLRgba32(128, 0, 0)},
            {("indigo"), BLRgba32(75, 0, 130)},
            {("blue"), BLRgba32(0, 0, 255)},
            {("mediumblue"), BLRgba32(0, 0, 205)},
            {("darkblue"), BLRgba32(0, 0, 139)},
            {("navy"), BLRgba32(0, 0, 128)},
            {("black"), BLRgba32(0, 0, 0)},
            {("transparent"), BLRgba32(0, 0, 0, 0) },

            // Deprecated system colors
            {"activeborder", BLRgba32(0xffb4b4b4)},
            {"activecaption", BLRgba32(0xff000080)},
            { "appworkspace", BLRgba32(0xffc0c0c0) },
            { "background", BLRgba32(0xff000000) },
            { "buttonface", BLRgba32(0xfff0f0f0) },
            { "buttonhighlight", BLRgba32(0xffffffff) },
            { "buttonshadow", BLRgba32(0xffa0a0a0) },
            { "buttontext", BLRgba32(0xff000000) },
            { "captiontext", BLRgba32(0xff000000) },
            { "graytext", BLRgba32(0xff808080) },
            { "highlight", BLRgba32(0xff3399ff) },
            { "highlighttext", BLRgba32(0xffffffff) },
            { "inactiveborder", BLRgba32(0xfff4f7fc) },
            { "inactivecaption", BLRgba32(0xff7a96df) },
            { "inactivecaptiontext", BLRgba32(0xffd2b4de) },
            { "infobackground", BLRgba32(0xffffffe1) },
            { "infotext", BLRgba32(0xff000000) },
            { "menu", BLRgba32(0xfff0f0f0) },
            { "menutext", BLRgba32(0xff000000) },
            { "scrollbar", BLRgba32(0xffd4d0c8) },
            { "threeddarkshadow", BLRgba32(0xff696969) },
            { "threedface", BLRgba32(0xffc0c0c0) },
            { "threedhighlight", BLRgba32(0xffffffff) },
            { "threedlightshadow", BLRgba32(0xffd3d3d3) },
            { "threedshadow", BLRgba32(0xffa0a0a0) },
            { "window", BLRgba32(0xffffffff) },
            { "windowframe", BLRgba32(0xff646464) },
            { "windowtext", BLRgba32(0xff000000) },
        };

        
        auto it = svgcolors.find(colorName);
		if (it != svgcolors.end())
		{
			return it->second;
		}
        //printf("UNKNOWN COLOR: ");
		//printChunk(colorName);
        
        return BLRgba32(128, 128, 128);
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
    // 
    // #RRGGBB
    // #RGB
    // 
    // Anything else is an error
    static bool parseHexToRgba32(const ByteSpan& inSpan, BLRgba32& outValue) noexcept
    {
        outValue.value = 0;

        if (inSpan.size() == 0)
            return false;

        if (inSpan[0] != '#')
            return false;

        uint8_t r{};
        uint8_t g{};
        uint8_t b{};
        uint8_t a{ 0xffu };

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

            outValue = BLRgba32(r * 17, g * 17, b * 17, a * 17);			// same effect as (r<<4|r), (g<<4|g), ..

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
        if (parseHexToRgba32(chunk, res))
            return res;

        return BLRgba32(0);
    }


    //
    // parse a color string
    // Return a BLRgba32 value

    // HSL to RGB conversion function based on the algorithm from the CSS specification:
    // Hue normalization to [0, 1).
    // This is NOT a clamp. It's modulo-1 wrapping, used for cyclic hue.
    static INLINE double normalize01(double x) noexcept
    {
        // Map x into (-1, 1) via fmod, then shift into [0,1).
        x = std::fmod(x, 1.0);
        if (x < 0.0)
            x += 1.0;
        return x;
    }

    // Normalize degrees to [0, 360).
    static INLINE double normalizeDegrees(double deg) noexcept
    {
        deg = std::fmod(deg, 360.0);
        if (deg < 0.0)
            deg += 360.0;
        return deg;
    }

    // Convert normalized [0..1) hue to [0..1) after accepting degrees input.
    // (Convenience helper; keeps callsites clean.)
    static INLINE double normalizeHue01FromDegrees(double deg) noexcept
    {
        return normalizeDegrees(deg) / 360.0;
    }

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

    // ------------------------------------------------------------
    // Updated parseColorHsl() that does NOT advance the caller's cursor:
    // (per your convention: parse takes const ByteSpan&)
    // ------------------------------------------------------------
    static BLRgba32 parseColorHsl(const ByteSpan& inChunk) noexcept
    {
        BLRgba32 defaultColor(0, 0, 0);

        // Work on local cursor only (parse => does not mutate caller input)
        ByteSpan str = inChunk;

        // Split at '('
        (void)chunk_token(str, "(");

        // Pull the substring up to ')'
        ByteSpan nums = chunk_token(str, ")");

        // h can be <number> (treated as degrees in hsl()).
        // s and l are typically percentages; CSS allows numbers too.
        // alpha (optional) is number|percent (0..1 or 0..100%).
        SVGNumberOrPercent hNP{};
        SVGNumberOrPercent sNP{};
        SVGNumberOrPercent lNP{};
        SVGNumberOrPercent aNP{};

        // h
        ByteSpan tok = chunk_token(nums, ",");
        {
            ByteSpan t = tok;
            if (!readSVGNumberOrPercent(t, hNP) || !hNP.isSet())
                return defaultColor;
        }

        // s
        tok = chunk_token(nums, ",");
        {
            ByteSpan t = tok;
            if (!readSVGNumberOrPercent(t, sNP) || !sNP.isSet())
                return defaultColor;
        }

        // l
        tok = chunk_token(nums, ",");
        {
            ByteSpan t = tok;
            if (!readSVGNumberOrPercent(t, lNP) || !lNP.isSet())
                return defaultColor;
        }

        // optional alpha
        double alpha01 = 1.0;
        tok = chunk_token(nums, ",");
        if (tok) {
            ByteSpan t = tok;
            if (readSVGNumberOrPercent(t, aNP) && aNP.isSet()) {
                alpha01 = aNP.isPercent() ? (aNP.value() / 100.0) : aNP.value();
                alpha01 = waavs::clamp(alpha01, 0.0, 1.0);
            }
        }

        // ---- Normalize / clamp per CSS/SVG behavior ----
        // Hue is cyclic: normalize modulo 360deg, then to [0,1).
        double h01 = normalizeHue01FromDegrees(hNP.value());

        // Saturation & lightness are linear factors. If authored as percent:
        //   0..100% => 0..1
        // If authored as number: treat as already 0..1 (then clamp).
        double s01 = sNP.isPercent() ? (sNP.value() / 100.0) : sNP.value();
        double l01 = lNP.isPercent() ? (lNP.value() / 100.0) : lNP.value();
        s01 = waavs::clamp(s01, 0.0, 1.0);
        l01 = waavs::clamp(l01, 0.0, 1.0);

        BLRgba32 res = hsl_to_rgb(h01, s01, l01);
        res.setA((uint32_t)std::lround(alpha01 * 255.0));
        return res;
    }

    /*


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
    */


    // Parse rgb color. The pointer 'str' must point at "rgb(" (4+ characters).
    // Default:
    //      This function returns gray (rgb(128, 128, 128) == '#808080') on parse errors
    //      for backwards compatibility. 
    // Note: some image viewers return black instead.
    static INLINE bool readCSSRGBChannel(ByteSpan& s, uint8_t& outC) noexcept
    {
        SVGNumberOrPercent c{};
        if (!readSVGNumberOrPercent(s, c))
            return false;

        double v = 0.0;
        if (c.isPercent()) {
            // 0..100% -> 0..255
            v = (waavs::clamp(c.value(), 0.0, 100.0) * 255.0) / 100.0;
        }
        else {
            // 0..255
            v = std::clamp(c.value(), 0.0, 255.0);
        }

        outC = (uint8_t)std::lround(v);
        return true;
    }

    static INLINE bool readCSSAlphaValue(ByteSpan& s, double& outA) noexcept
    {
        SVGNumberOrPercent a{};
        if (!readSVGNumberOrPercent(s, a))
            return false;

        double v = a.isPercent() ? (a.value() / 100.0) : a.value();
        outA = std::clamp(v, 0.0, 1.0);
        return true;
    }

    static bool parseColorRGB(const ByteSpan& inChunk, BLRgba32& aColor)
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
        // The individual numeric values are 'number | percent' (50, 50%)
        // 50 - an absolute value in the range [0..255]
        // 50% - a percentage of 255

        int i = 0;
        uint8_t rgba[4]{};

        // Get the first token, which is red
        // if it's not there, then return gray
        auto num = chunk_token_char(nums, ',');
        if (num.size() < 1)
        {
            //aColor.reset(128, 128, 128, 0);
            aColor.reset(255, 255, 0, 255);     // Use turquoise to indicate error
            return false;
        }

        while (num && i < 4)
        {
            if (i < 3) {
                if (!readCSSRGBChannel(num, rgba[i]))
                    return false;
            }
            else {
                double a = 1.0;
                if (!readCSSAlphaValue(num, a))
                    return false;
                rgba[i] = (uint8_t)std::lround(a * 255.0);
            }

            i++;
            num = chunk_token_char(nums, ',');
        }

        if (i == 4)
            aColor.reset(rgba[0], rgba[1], rgba[2], rgba[3]);
        else
            aColor.reset(rgba[0], rgba[1], rgba[2]);

        return true;
    }
}
