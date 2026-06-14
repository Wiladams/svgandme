// svgcolors.h
#pragma once

#include <map>
#include <unordered_map>
#include <string>

#include <blend2d/blend2d.h>
#include "bspan.h"
#include "coloring.h"
#include "svgscan.h"


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
    // Returns a color, based on a name (case insensitive).  If the 
    // name is not found in the database, it returns a default color (gray).
    // Note: The table stores a BLRgba32, which is a 32-bit ARGB pixel in sRGB space.  
    // The returned value is a ColorSRGB struct, which has float components 
    // in the range [0..1].  
    // The conversion is done by the caller, so that the caller can decide how 
    // to handle the conversion (e.g. whether to premultiply or not).
    static WGResult get_color_by_name_as_srgb(const ByteSpan &colorName, ColorSRGB &csrgb) noexcept
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

        //BLRgba32 c = BLRgba32(128, 128, 128, 255);
        uint8_t r=0, g=0, b=0, a = 255;

        auto it = svgcolors.find(colorName);
        if (it != svgcolors.end())
        {
            r = it->second.r();
            g = it->second.g();
            b = it->second.b();
            a = it->second.a();

            csrgb = colorSRGB_from_straight_components0_255(r, g, b, a);
            return WG_SUCCESS;
        }
        
        // If we don't find the color, return an error code.  
        // The caller can decide how to handle this 
        // (e.g. use a default color).
        return WGErrorCode::WG_ERROR_Invalid_Argument;
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
    static int parseHexToColor(const ByteSpan& inSpan, ColorSRGB& outValue) noexcept
    {
        outValue = {};


        if (inSpan.size() == 0)
            return WGErrorCode::WG_ERROR_Invalid_Argument;

        if (inSpan[0] != '#')
            return WG_ERROR_Invalid_Argument;

        uint8_t r{};
        uint8_t g{};
        uint8_t b{};
        uint8_t a{ 0xffu };

        if (inSpan.size() == 4) {
            uint8_t rNib, gNib, bNib;

            // get the three nibbles, and validate that they are hex digits
            if  (hex_nibble(inSpan[1], rNib) != WG_SUCCESS ||
                hex_nibble(inSpan[2], gNib) != WG_SUCCESS ||
                hex_nibble(inSpan[3], bNib) != WG_SUCCESS)
            {
                return WG_ERROR_Invalid_Argument;
            }

            // #RGB
            r = rNib << 4 | rNib;
            g = gNib << 4 | gNib;
            b = bNib << 4 | bNib;
        }
        else if (inSpan.size() == 5) {
            // #RGBA
            // double up each nibble to form bytes
            uint8_t rNib, gNib, bNib, aNib;
            if (hex_nibble(inSpan[1], rNib) != WG_SUCCESS ||
                hex_nibble(inSpan[2], gNib) != WG_SUCCESS ||
                hex_nibble(inSpan[3], bNib) != WG_SUCCESS ||
                hex_nibble(inSpan[4], aNib) != WG_SUCCESS)
            {
                return WG_ERROR_Invalid_Argument;
            }

            r = rNib << 4 | rNib;
            g = gNib << 4 | gNib;
            b = bNib << 4 | bNib;
            a = aNib << 4 | aNib;
        }
        else if (inSpan.size() == 7) {
            // #RRGGBB
            if (hex_byte(inSpan[1], inSpan[2], r) != WG_SUCCESS ||
                hex_byte(inSpan[3], inSpan[4], g) != WG_SUCCESS ||
                hex_byte(inSpan[5], inSpan[6], b) != WG_SUCCESS)
            {
                return WG_ERROR_Invalid_Argument;
            }
        }
        else if (inSpan.size() == 9) {
            // #RRGGBBAA
            if (hex_byte(inSpan[1], inSpan[2], r) != WG_SUCCESS ||
                hex_byte(inSpan[3], inSpan[4], g) != WG_SUCCESS ||
                hex_byte(inSpan[5], inSpan[6], b) != WG_SUCCESS ||
                hex_byte(inSpan[7], inSpan[8], a) != WG_SUCCESS)
            {
                return WG_ERROR_Invalid_Argument;
            }
        }
        else {
            return WG_ERROR_Invalid_Argument;
        }

        outValue = colorSRGB_from_straight_components0_255(r, g, b, a);

        return WG_SUCCESS;
    }

    // Turn a 3 or 6 digit hex string into a BLRgba32 value
    // if there's an error in the conversion, a transparent color is returned
    // BUGBUG - maybe a more obvious color should be returned
    static WGResult parse_colorsrgb_from_hex(const ByteSpan& chunk, ColorSRGB & srgb) noexcept
    {
        WGResult res = parseHexToColor(chunk, srgb);
        if (res != WG_SUCCESS)
            return res;

        return WG_SUCCESS;
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

        outA = clamp01(a.calculatedValue());

        return true;
    }

    // Numeric format can be:
    // rgb(255, 0, 0)
    // rgb(100%, 0%, 0%)
    // rgb(255 0 0 / 0.5)
    // 
    static WGResult parse_colorsrgb_from_func(const ByteSpan& inChunk, ColorSRGB& cSRGB)
    {
        // set a default color in case of parse failure.  
        // we'll return transparent black
        // we need to check the spec to see what this 
        // should really be
        colorsrgb_reset(cSRGB, 0, 0, 0, 0);

        // skip past the leading "rgb("
        ByteSpan s = inChunk;
        auto leading = chunk_token_char(s, '(');

        // s should now point to the first number
        // and 'leading' should contain 'rgb'
        // BUGBUG - we can check 'leading' to see if it's actually 'rgb'
        // but we'll just assume it is for now

        // get the numbers by separating at the ')'
        auto nums = chunk_token_char(s, ')');

        // So, now nums contains the individual numeric values, 
		// separated by ',' or spaces or '/' (for alpha).  

        // The individual numeric values are 'number | percent' (50, 50%)
        // 50 - an absolute value in the range [0..255]
        // 50% - a percentage of 255 (for the color components)
		// for alpha, 50% is a percentage of 1.0 (not 255)

        uint8_t rgba[4]{};
        rgba[3] = 255;  // default alpha value is 255 (fully opaque)



        // Get the rgb values
        charset delims = chrWspChars + ',';
        for (int i = 0; i < 3; i++)
        {
            bspan_skip_spaces(nums);
            // get a component, separated by space or ','
            auto num = chunk_token(nums, delims);

            if (!num)
                return WG_ERROR_Invalid_Argument;

            if (!readCSSRGBChannel(num, rgba[i]))
                return WG_ERROR_Invalid_Argument;
        }

        // assuming we've read the three color components
        // let's see if there's an alpha remaining.
        // it can be delimited by the previous delimiters
        // or the '/' character
        delims += '/';
        bspan_skip_while(nums, delims);

        if (nums)
        {
            bspan_skip_spaces(nums);
            double a = 1.0;
            if (!readCSSAlphaValue(nums, a))
                return WG_ERROR_Invalid_Argument;
            rgba[3] = quantize0_255(a);
        }

        cSRGB = colorSRGB_from_straight_components0_255(rgba[0], rgba[1], rgba[2], rgba[3]);

        return WG_SUCCESS;
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
        else if (bspan_starts_with(cSpan, rgbStr) ||
            bspan_starts_with(cSpan, rgbaStr) ||
            bspan_starts_with(cSpan, rgbaStrCaps) ||
            bspan_starts_with(cSpan, rgbStrCaps))
        {
            return parse_colorsrgb_from_func(cSpan, cSRGB);
        }
        else if (bspan_starts_with(cSpan, hslStr) ||
            bspan_starts_with(cSpan, hslaStr))
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
}

namespace waavs {
    //
    // htt://www.w3.org/TR/SVG11/feature#PaintAttribute
    // color, 
    // fill, fill-opacity,
    // stroke, stroke-opacity,
    // stroke-width

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
        COLOR_SEMANTIC_UNKNOWN = 0x00,     // neither opacity, nor color
        // distinctly different than 'none'
        COLOR_SEMANTIC_NONE = 0x01,     // literal 'none'
        COLOR_SEMANTIC_OPACITY = 0x02,     // opacity specified
        COLOR_SEMANTIC_COLOR = 0x04,     // regular color
        COLOR_SEMANTIC_INHERIT = 0x08,     // literal 'inherit'
        COLOR_SEMANTIC_CURRENT = 0x10,     // literal 'currentColor'
        COLOR_SEMANTIC_CONTEXT_STROKE = 0x20,     // literal 'context-stroke'
        COLOR_SEMANTIC_CONTEXT_FILL = 0x40,     // literal 'context-fill'
        COLOR_SEMANTIC_REFERENCE = 0x80,     // starts-with 'url('
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




    struct SVGColor
    {
        ColorSRGB fValue{};
        ColorSemantics fSemantics{ COLOR_SEMANTIC_UNKNOWN };
        ByteSpan fColorSpan{};

        // Which attributes do we pull color and opacity from?
        InternedKey fColorField;
        InternedKey fOpacityField;


        SVGColor(InternedKey colorField, InternedKey opacityField)
            : fColorField(colorField)
            , fOpacityField(opacityField)
        {
            //setName(colorField);
        }

        void setValue(float r, float g, float b, float a = 1.0f) noexcept
        {
            colorsrgb_reset(fValue, r, g, b, a);
            fSemantics = COLOR_SEMANTIC_COLOR;
        }

        ColorSRGB value() const { return fValue; }
        ByteSpan rawValue() const { return fColorSpan; }

        // semantics helpers
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
        bool loadFromAttributes(const XmlAttributeCollection& attrs)
        {
            static const char* rgbStr = "rgb(";
            static const char* rgbaStr = "rgba(";
            static const char* rgbStrCaps = "RGB(";
            static const char* rgbaStrCaps = "RGBA(";
            static const char* hslStr = "hsl(";
            static const char* hslaStr = "hsla(";

            //ByteSpan colorSpan{};
            ByteSpan opacitySpan{};

            // You can still have a valid color if only 
            // opacity is specified.
            attrs.getValue(fColorField, fColorSpan);
            attrs.getValue(fOpacityField, opacitySpan);

            // only return early if neither color nor opacity is specified
            if (!fColorSpan)
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
                }
            }

            // Quick success if we have a URL reference
            if (bspan_starts_with(fColorSpan, "url("))
            {
                fSemantics = COLOR_SEMANTIC_REFERENCE;

                return true;
            }




            // Finally, if we have a color value, we need to 
            // parse it.  
            if (fColorSpan)
            {
                // Before we try to parse the color, check 
                // to see if the value is 'currentColor'.
                // If it is, then we need to check if there 
                // is a 'color' attribute in the current set of attributes.
                if (fColorSpan == "currentColor")
                {
                    ByteSpan currentColorValue{};
                    if (attrs.getValue(svgattr::color(), currentColorValue))
                    {
                        fColorSpan = currentColorValue;
                    }
                    else {
                        fSemantics = COLOR_SEMANTIC_CURRENT;
                        return true;
                    }
                }


                ByteSpan cSpan = fColorSpan;
                size_t len = 0;
                len = cSpan.size();
                if ((len >= 1) && (*cSpan == '#'))
                {
                    if (parse_colorsrgb_from_hex(cSpan, cSRGB) == WG_SUCCESS)
                    {
                        fSemantics = COLOR_SEMANTIC_COLOR;
                    }
                    else {
                        return false;
                    }
                }
                else if (bspan_starts_with(cSpan, rgbStr) ||
                    bspan_starts_with(cSpan, rgbaStr) ||
                    bspan_starts_with(cSpan, rgbaStrCaps) ||
                    bspan_starts_with(cSpan, rgbStrCaps))
                {
                    if (parse_colorsrgb_from_func(cSpan, cSRGB) == WG_SUCCESS)
                    {
                        fSemantics = COLOR_SEMANTIC_COLOR;
                    }
                    else {
                        return false;
                    }
                }
                else if (bspan_starts_with(cSpan, hslStr) ||
                    bspan_starts_with(cSpan, hslaStr))
                {
                    // BUGBUG - need to modernize Hsl to fill
                    // in ColorSRGB
                    //c = parseColorHsl(str);

                    fSemantics = COLOR_SEMANTIC_COLOR;
                }
                else {
                    if (cSpan == svgval::none()) {
                        fSemantics = COLOR_SEMANTIC_NONE;
                    }
                    else if (cSpan == "context-stroke")
                    {
                        fSemantics = COLOR_SEMANTIC_CONTEXT_STROKE;
                    }
                    else if (cSpan == "context-fill")
                    {
                        fSemantics = COLOR_SEMANTIC_CONTEXT_FILL;
                    }
                    else if ((cSpan == "inherit"))
                    {
                        fSemantics = COLOR_SEMANTIC_INHERIT;
                    }
                    else if (cSpan == "currentColor")
                    {
                        // Take on whatever color was set in the 'color'
                        // attribute of this node, or whatever is the 
                        // currently Active color at the time of render
                        fSemantics = COLOR_SEMANTIC_CURRENT;
                    }
                    else {
                        if (get_color_by_name_as_srgb(cSpan, cSRGB) == WG_SUCCESS)
                        {
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

}
