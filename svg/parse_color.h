#ifndef PARSE_COLOR_H_INCLUDED
#define PARSE_COLOR_H_INCLUDED



#include "bspan.h"
#include "converters.h"
#include "coloring.h"
#include "svgdatatypes.h"


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





    // Scan a bytespan for a sequence of hex digits
    // without using scanf. return 'true' upon success
    // false for any error.
    // The format is either
    // 
    // #RRGGBB
    // #RGB
    // 
    // Anything else is an error
    static int parse_color_hexSpanToRgba32(const waavs::ByteSpan& inSpan, ColorSRGB& outValue) noexcept
    {
        outValue = SRGB8_ARGB(0);

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

            outValue = SRGB8(r * 17, g * 17, b * 17, 255);			// same effect as (r<<4|r), (g<<4|g), ..

            return 0;
        }
        else if (inSpan.size() == 5) {
            // #RGBA
            r = hexToDec(inSpan[1]);
            g = hexToDec(inSpan[2]);
            b = hexToDec(inSpan[3]);
            a = hexToDec(inSpan[4]);

            outValue = SRGB8(r * 17, g * 17, b * 17, a * 17);			// same effect as (r<<4|r), (g<<4|g), ..

            return 0;
        }
        else if (inSpan.size() == 7) {
            // #RRGGBB
            r = (hexToDec(inSpan[1]) << 4) | hexToDec(inSpan[2]);
            g = (hexToDec(inSpan[3]) << 4) | hexToDec(inSpan[4]);
            b = (hexToDec(inSpan[5]) << 4) | hexToDec(inSpan[6]);

            outValue = SRGB8(r, g, b, 255);
            return 0;
        }
        else if (inSpan.size() == 9) {
            // #RRGGBBAA
            r = (hexToDec(inSpan[1]) << 4) | hexToDec(inSpan[2]);
            g = (hexToDec(inSpan[3]) << 4) | hexToDec(inSpan[4]);
            b = (hexToDec(inSpan[5]) << 4) | hexToDec(inSpan[6]);
            a = (hexToDec(inSpan[7]) << 4) | hexToDec(inSpan[8]);
            outValue = SRGB8(r, g, b, a);

            return 0;
        }

        return 1;
    }

    // Turn a 3 or 6 digit hex string into a BLRgba32 value
    // if there's an error in the conversion, a transparent color is returned
    // BUGBUG - maybe a more obvious color should be returned
    static int parse_color_ColorHex(const waavs::ByteSpan& chunk, ColorSRGB &outValue) noexcept
    {
        if (parse_color_hexSpanToRgba32(chunk, outValue))
            return 0;

        outValue = SRGB8_ARGB(0);

        return 1;
    }


    //
    // parse a color string
    //
    static float parse_color_hue_to_rgb(float p, float q, float t) noexcept
    {
        if (t < 0) t += 1;
        if (t > 1) t -= 1;
        if (t < 1 / 6.0) return p + (q - p) * 6 * t;
        if (t < 1 / 2.0) return q;
        if (t < 2 / 3.0) return p + (q - p) * (2 / 3.0 - t) * 6;

        return p;
    }

    // Return a ColorRgba value
    static ColorSRGB parse_color_hsl(float h, float s, float l) noexcept
    {
        float r, g, b;

        if (s == 0) {
            r = g = b = l; // achromatic
        }
        else {
            float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
            float p = 2 * l - q;
            r = parse_color_hue_to_rgb(p, q, h + 1 / 3.0);
            g = parse_color_hue_to_rgb(p, q, h);
            b = parse_color_hue_to_rgb(p, q, h - 1 / 3.0);
        }

        return SRGB8(uint32_t(r * 255), uint32_t(g * 255), uint32_t(b * 255), 255);
    }

    static int parse_color_parseColorHsl(const waavs::ByteSpan& inChunk, ColorSRGB &res) noexcept
    {
        res = SRGB8(0, 0, 0, 255);
        waavs::ByteSpan str = inChunk;
        auto leading = chunk_token(str, "(");

        // h, s, l attributes can be numeric, or percent
        // get the numbers by separating at the ')'
        auto nums = chunk_token(str, ")");
        SVGDimension hd{};
        SVGDimension sd{};
        SVGDimension ld{};
        SVGDimension od{ 255,SVG_LENGTHTYPE_NUMBER };

        double h = 0, s = 0, l = 0;
        double o = 1.0;

        auto num = chunk_token(nums, ",");
        if (!hd.loadFromChunk(num))
            return 1;

        num = chunk_token(nums, ",");
        if (!sd.loadFromChunk(num))
            return 1;

        num = chunk_token(nums, ",");
        if (!ld.loadFromChunk(num))
            return 1;

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

        auto res = parse_color_hsl(h, s, l);
        res.a = 0;

        return 0;
    }

    // Parse rgb color. The pointer 'str' must point at "rgb(" (4+ characters).
    // Default:
    //      This function returns gray (rgb(128, 128, 128) == '#808080') on parse errors
    //      for backwards compatibility. 
    // Note: some image viewers return black instead.

    static int parse_color_ColorRGB(const waavs::ByteSpan& inChunk, ColorSRGB& aColor)
    {
        // skip past the leading "rgb("
        waavs::ByteSpan s = inChunk;
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

        uint8_t rgbi[4]{};

        // Get the first token, which is red
        // if it's not there, then return gray
        auto num = chunk_token_char(nums, ',');
        if (num.size() < 1)
        {
            //aColor.reset(128, 128, 128, 0);
            aColor = SRGB8(255, 255, 0, 255);     // Use turquoise to indicate error
            return 1;
        }

        int i = 0;
        while (num && i<4)
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
                    rgbi[i] = (uint8_t)waavs::clamp(cv.value(), 0.0, 255.0);
            }
            i++;
            num = chunk_token_char(nums, ',');
        }

        if (i == 4)
            aColor = SRGB8(rgbi[0], rgbi[1], rgbi[2], rgbi[3]);
        else
            aColor = SRGB8(rgbi[0], rgbi[1], rgbi[2], 255);

        return 0;
    }



#endif
