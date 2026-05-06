// coloring.h
#pragma once

#ifndef COLORING_H_INCLUDED
#define COLORING_H_INCLUDED

#include "pixeling.h"

namespace waavs
{
    // Generic all-purpose color structure
    // There is no presumption of color space or premultiplication; 
    // this is just a convenient way to hold four float components together
    struct Color4f
    {
        float r, g, b, a;
    };

    // This structure should be used when you're parsing a value from SVG or CSS
    // It is in gamma-encoded sRGB space, with straight (unpremultiplied) alpha
    // You should convert to linear space and premultiply before doing any rendering
    // gamma-encoded sRGB, straight alpha (not premultiplied)
    struct ColorSRGB
    {
        float r, g, b, a; // 0..1
    } ;

    // This structure should be used when doing rendering or compositing
    // It is in linear RGB space
    // The alpha is still just meta information (straight), so 
    // the component values are NOT pre-multiplied
    // This type is mostly an intermediary.
    // ColorPRGBA should be used for most operations
    // 
    // linear RGB, straight alpha (unpremultiplied) 
    struct ColorLinear
    {
        float r, g, b, a; // 0..1 
    } ;

    // ColorPRGBA
    // This structure is in the linear address space, and pre-multiplies
    // the alpha against the color components.  This is what is compatible
    // with blend2d PRGBA pixels.
    // 
    // linear RGB premultiplied by alpha (r,g,b <= a) 
    struct ColorPRGBA
    {
        float r, g, b, a; // 0..1 
    } ;
}

namespace waavs
{
    // Forward function declarations
    static INLINE void colorsrgb_reset(ColorSRGB  &c) noexcept;

    static INLINE float coloring_srgb_component_to_linear(float c);
    static INLINE float coloring_linear_component_to_srgb(float c);

    static INLINE float coloring_relative_luminance_linear(const ColorLinear* c);
    static INLINE ColorLinear coloring_srgb_to_linear(ColorSRGB c);
    static INLINE ColorSRGB coloring_linear_to_srgb(ColorLinear c);
    static INLINE ColorPRGBA coloring_linear_premultiply(ColorLinear c);
    static INLINE ColorLinear coloring_linear_unpremultiply(ColorPRGBA p);
    static INLINE ColorPRGBA coloring_prgba_from_srgb(ColorSRGB c);
    static INLINE ColorSRGB coloring_srgb_from_prgba(ColorPRGBA p);
    static INLINE float coloring_linear_color_distance(ColorLinear a, ColorLinear b);
    static INLINE float coloring_linear_luminance_diff(ColorLinear a, ColorLinear b);
    static INLINE ColorPRGBA coloring_prgba_lerp(const ColorPRGBA a, const ColorPRGBA b, float t);
    static INLINE ColorPRGBA coloring_prgba_over(const ColorPRGBA src, const ColorPRGBA dst);
    static INLINE Pixel_ARGB32 coloring_prgba_to_ARGB32(const ColorPRGBA& p) noexcept;

    static INLINE constexpr ColorSRGB SRGB8(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static INLINE constexpr ColorSRGB SRGB8_ARGB(uint32_t argb);

}


namespace waavs
{
    static INLINE void colorsrgb_reset(ColorSRGB& c) noexcept
    {
        c.r = c.g = c.b = c.a = 0.0f;
    }
    
    static INLINE void colorsrgb_reset(ColorSRGB& c, const float r, const float g, const float b, const float a) noexcept
    {
        c.r = r;
        c.g = g;
        c.b = b;
        c.a = a;
    }

    static INLINE ColorSRGB colorSRGB_from_straight_components0_255(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a) noexcept
    {
        // need to dequantize the components from 0..255 to 0..1
        return ColorSRGB{
            dequantize0_255(r),
            dequantize0_255(g),
            dequantize0_255(b),
            dequantize0_255(a)
        };
    }

    // colorsrgb_from_premultiplied_Pixel_ARGB32
    //
    // convert a premultiplied pixel into a 
    // straight SRGB value
	static INLINE ColorSRGB colorsrgb_from_premultiplied_Pixel_ARGB32(const Pixel_ARGB32& px) noexcept
	{
		// need to dequantize the components from 0..255 to 0..1
		// and unpremultiply the color components by the alpha
		float a = dequantize0_255((px >> 24) & 0xFFu);
        const float invA = (a > 1e-8f) ? (1.0f / a) : 0.0f;

		if (invA  <= 0.0f) { // treat near zero alpha as transparent
			return ColorSRGB{ 0.0f, 0.0f, 0.0f, 0.0f };
		}
		else {
			float r = clamp01f(dequantize0_255((px >> 16) & 0xFFu) *invA);
			float g = clamp01f(dequantize0_255((px >> 8) & 0xFFu) * invA);
			float b = clamp01f(dequantize0_255((px >> 0) & 0xFFu) * invA);

			return ColorSRGB{ r, g, b, a };
		}
	}

    static INLINE Pixel_ARGB32 argb32_pack_premultiplied_srgb(float a, float r, float g, float b)
    {
        return argb32_pack_u8(
            quantize0_255(clamp01f(a)),
            quantize0_255(clamp01f(r)),
            quantize0_255(clamp01f(g)),
            quantize0_255(clamp01f(b)));
    }

    static INLINE Pixel_ARGB32 Pixel_ARGB32_from_ColorSRGB(const ColorSRGB& c) noexcept
    {
        const float a = clamp01f(c.a);
        const float r = clamp01f(c.r);
        const float g = clamp01f(c.g);
        const float b = clamp01f(c.b);

        return argb32_pack_u8(
            quantize0_255(a),
            quantize0_255(r),
            quantize0_255(g),
            quantize0_255(b));
    }

    static INLINE Pixel_ARGB32 Pixel_ARGB32_premultiplied_from_ColorSRGB(const ColorSRGB& c) noexcept
    {
        const float a = clamp01f(c.a);
        const float r = clamp01f(c.r) * a;
        const float g = clamp01f(c.g) * a;
        const float b = clamp01f(c.b) * a;

		return argb32_pack_u8(
			quantize0_255(a),
			quantize0_255(r),
			quantize0_255(g),
			quantize0_255(b));
    }

    // sRGB <-> linear transfer functions (float in [0,1]) 
    // convert a single component
    static INLINE float coloring_srgb_component_to_linear(float c)
    {
        c = clamp01f(c);
        return (c <= 0.04045f) ? (c / 12.92f)
            : powf((c + 0.055f) / 1.055f, 2.4f);
    }

    // convert a single component
    static INLINE float coloring_linear_component_to_srgb(float c)
    {
        c = clamp01f(c);
        return (c <= 0.0031308f) ? (12.92f * c)
            : (1.055f * powf(c, 1.0f / 2.4f) - 0.055f);
    }

    static INLINE Pixel_ARGB32 argb32_from_premultiplied_linear(
        float a, float pr, float pg, float pb) noexcept
    {
        ColorPRGBA p{};
        p.a = clamp01f(a);
        p.r = clamp01f(pr);
        p.g = clamp01f(pg);
        p.b = clamp01f(pb);

        return coloring_prgba_to_ARGB32(p);
    }

    // Relative luminance from linear RGB (Rec.709 primaries)
    static INLINE float coloring_relative_luminance_linear(const ColorLinear* c)
    {
        return 0.2126f * c->r + 0.7152f * c->g + 0.0722f * c->b;
    }


    // Conversions
    // convert sRGB color to linear color, 
    // clamp the alpha to [0..1], but don't modify it otherwise
    // and don't premultiply
    static INLINE ColorLinear coloring_srgb_to_linear(ColorSRGB c)
    {
        ColorLinear out;
        out.r = coloring_srgb_component_to_linear(c.r);
        out.g = coloring_srgb_component_to_linear(c.g);
        out.b = coloring_srgb_component_to_linear(c.b);
        out.a = clamp01f(c.a);
        return out;
    }

    // convert linear color to sRGB color,
    static INLINE ColorSRGB coloring_linear_to_srgb(ColorLinear c)
    {
        ColorSRGB out;
        out.r = coloring_linear_component_to_srgb(c.r);
        out.g = coloring_linear_component_to_srgb(c.g);
        out.b = coloring_linear_component_to_srgb(c.b);
        out.a = clamp01f(c.a);
        return out;
    }


    // linear straight -> premultiplied linear 
    // Alpha is clamped to [0..1] and retained as meta information
    static INLINE ColorPRGBA coloring_linear_premultiply(ColorLinear c)
    {
        ColorPRGBA p;
        float A = clamp01f(c.a);
        p.r = c.r * A;
        p.g = c.g * A;
        p.b = c.b * A;
        p.a = A;
        return p;
    }

    // premultiplied linear -> linear straight
    // Assuming the original alpha information used to perform a premultiply
    // is still available, this function will reverse the premultiply operation
    static INLINE ColorLinear coloring_linear_unpremultiply(ColorPRGBA p)
    {
        ColorLinear c;
        if (p.a <= 1.0f / 255.0f) { // treat sub-1 LSB alpha as transparent
            c.r = c.g = c.b = 0.0f; c.a = 0.0f;
        }
        else {
            float ia = 1.0f / p.a;
            c.r = p.r * ia;
            c.g = p.g * ia;
            c.b = p.b * ia;
            c.a = p.a;
        }
        return c;
    }


    // Go from a sRGB (typically parsed) to a pre-multiplied linear color
    // Convenience: sRGB straight -> PRGBA linear 
    static INLINE ColorPRGBA coloring_prgba_from_srgb(ColorSRGB c)
    {
        return coloring_linear_premultiply(coloring_srgb_to_linear(c));
    }

    // Go from a pre-multiplied linear color to a sRGB straight color
    // Convenience: PRGBA linear -> sRGB straight 
    static INLINE ColorSRGB coloring_srgb_from_prgba(ColorPRGBA p)
    {
        return coloring_linear_to_srgb(coloring_linear_unpremultiply(p));
    }


    // Common operations operating in premultiplied linear space
    // 
    // Squared Euclidean distance in linear RGB (straight)
    // Calculate how close two colors are to each other
    static INLINE float coloring_linear_color_distance(ColorLinear a, ColorLinear b)
    {
        float dr = a.r - b.r;
        float dg = a.g - b.g;
        float db = a.b - b.b;
        return dr * dr + dg * dg + db * db;
    }

    // Luminance difference in linear RGB (straight) 
    static INLINE float coloring_linear_luminance_diff(ColorLinear a, ColorLinear b)
    {
        float ya = coloring_relative_luminance_linear(&a);
        float yb = coloring_relative_luminance_linear(&b);
        float d = ya - yb;

        return d < 0.0f ? -d : d;
    }


    // Lerp two premultiplied colors in linear space
    // fmaf - fuse, multiply - add
    static INLINE ColorPRGBA coloring_prgba_lerp(const ColorPRGBA a, const ColorPRGBA b, float t)
    {
        ColorPRGBA o;
        o.r = WAAVS_FMAF((b.r - a.r), t, a.r);
        o.g = WAAVS_FMAF((b.g - a.g), t, a.g);
        o.b = WAAVS_FMAF((b.b - a.b), t, a.b);
        o.a = WAAVS_FMAF((b.a - a.a), t, a.a);

        return o;
    }

    // Porter-Duff "over" (src over dst) in premultiplied linear space 
    // Uses source alpha multiplied into destination color
    static INLINE ColorPRGBA coloring_prgba_over(const ColorPRGBA src, const ColorPRGBA dst)
    {
        ColorPRGBA o;
        float oa = 1.0f - src.a;
        o.r = WAAVS_FMAF(oa, dst.r, src.r);
        o.g = WAAVS_FMAF(oa, dst.g, src.g);
        o.b = WAAVS_FMAF(oa, dst.b, src.b);
        o.a = WAAVS_FMAF(oa, dst.a, src.a);

        return o;
    }


    // Helpers to build ColorSRGB from 8-bit sRGB literals (straight alpha)
    // Use this in places where you would normallly be constructing components
    // with integer literals in the range 0..255
    static INLINE constexpr ColorSRGB SRGB8_set(float r, float g, float b, float a)
    {
        return ColorSRGB{ r , g , b , a };
    }

    static INLINE constexpr ColorSRGB SRGB8(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        return ColorSRGB{ r * (1.0f / 255.0f), g * (1.0f / 255.0f), b * (1.0f / 255.0f), a * (1.0f / 255.0f) };
    }

    static INLINE constexpr ColorSRGB SRGB_set_A(ColorSRGB& value, uint8_t a)
    {
        value.a = a * (1.0f / 255.0f);
        return value;
    }

    static INLINE constexpr ColorSRGB SRGB8_ARGB(uint32_t argb)
    {
        return SRGB8(uint8_t((argb >> 16) & 0xFF), uint8_t((argb >> 8) & 0xFF), uint8_t(argb & 0xFF), uint8_t((argb >> 24) & 0xFF));
    }
}

// Converting between pixel formats and color structs
namespace waavs
{
    static INLINE ColorPRGBA coloring_ARGB32_to_prgba(Pixel_ARGB32 px) noexcept
    {
        ColorSRGB srgb = colorsrgb_from_premultiplied_Pixel_ARGB32(px);
        ColorPRGBA out = coloring_prgba_from_srgb(srgb);

        return out;

    }


    static INLINE Pixel_ARGB32 coloring_prgba_to_ARGB32(const ColorPRGBA& p) noexcept
    {

        const ColorLinear lin = coloring_linear_unpremultiply(p);
        const ColorSRGB srgb = coloring_linear_to_srgb(lin);

        const float a = clamp01f(p.a);

        const uint8_t A = quantize0_255(a);
        const uint8_t R = quantize0_255(clamp01f(srgb.r) * a);
        const uint8_t G = quantize0_255(clamp01f(srgb.g) * a);
        const uint8_t B = quantize0_255(clamp01f(srgb.b) * a);
        
        return argb32_pack_u32(A, R, G, B);
    }

}

#endif // COLORING_H_INCLUDED