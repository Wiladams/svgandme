#ifndef COLORING_H_INCLUDED
#define COLORING_H_INCLUDED


#include "definitions.h"

#include <stdint.h>
#include <math.h>

// Tiny helpers
// Clamp a float value to the range [0..1]
#ifndef WAAVS_CLAMP01
#define WAAVS_CLAMP01(v) ((v) < 0.0f ? 0.0f : ((v) > 1.0f ? 1.0f : (v)))
#endif

// sRGB <-> linear transfer functions (float in [0,1]) 
// convert a single component
static inline float coloring_srgbc_to_linear(float c)
{
    c = WAAVS_CLAMP01(c);
    return (c <= 0.04045f) ? (c / 12.92f)
        : powf((c + 0.055f) / 1.055f, 2.4f);
}

// convert a single component
static inline float coloring_linear_component_to_srgb(float c)
{
    c = WAAVS_CLAMP01(c);
    return (c <= 0.0031308f) ? (12.92f * c)
        : (1.055f * powf(c, 1.0f / 2.4f) - 0.055f);
}


// ----- representations -----

// This structure should be used when you're parsing a value from SVG or CSS
// It is in gamma-encoded sRGB space, with straight (unpremultiplied) alpha
// You should convert to linear space and premultiply before doing any rendering
typedef struct WAAVS_ALIGN16 {
    // gamma-encoded sRGB, straight alpha (unpremultiplied)
    float r, g, b, a; // 0..1
} ColorSRGB;

// This structure should be used when doing rendering or compositing
// It is in linear RGB space
// The alpha is still just meta information (straight), so 
// the component values are NOT pre-multiplied
// This type is mostly an intermediary.
// ColorPRGBA should be used for most operations
typedef struct WAAVS_ALIGN16 {
    // linear RGB, straight alpha (unpremultiplied) 
    float r, g, b, a; // 0..1 
} ColorLinear;

// This structure is in the linear address space, and pre-multiplies
// the alpha against the color components.
typedef struct WAAVS_ALIGN16 {
    // linear RGB premultiplied by alpha (r,g,b <= a) 
    float r, g, b, a; // 0..1 
} ColorPRGBA;


// Relative luminance from linear RGB (Rec.709 primaries)
static inline float coloring_relative_luminance_linear(const ColorLinear* c)
{
    return 0.2126f * c->r + 0.7152f * c->g + 0.0722f * c->b;
}


// Conversions
// convert sRGB color to linear color, 
// clamp the alpha to [0..1], but don't modify it otherwise
// and don't premultiply
static inline ColorLinear coloring_srgb_to_linear(ColorSRGB c)
{
    ColorLinear out;
    out.r = coloring_srgbc_to_linear(c.r);
    out.g = coloring_srgbc_to_linear(c.g);
    out.b = coloring_srgbc_to_linear(c.b);
    out.a = WAAVS_CLAMP01(c.a);
    return out;
}

// convert linear color to sRGB color,
static inline ColorSRGB coloring_linear_to_srgb(ColorLinear c)
{
    ColorSRGB out;
    out.r = coloring_linear_component_to_srgb(c.r);
    out.g = coloring_linear_component_to_srgb(c.g);
    out.b = coloring_linear_component_to_srgb(c.b);
    out.a = WAAVS_CLAMP01(c.a);
    return out;
}


// linear straight -> premultiplied linear 
// Alpha is clamped to [0..1] and retained as meta information
static inline ColorPRGBA coloring_linear_premultiply(ColorLinear c)
{
    ColorPRGBA p;
    float A = WAAVS_CLAMP01(c.a);
    p.r = c.r * A;
    p.g = c.g * A;
    p.b = c.b * A;
    p.a = A;
    return p;
}

// premultiplied linear -> linear straight
// Assuming the original alpha information used to perform a premultiply
// is still available, this function will reverse the premultiply operation
static inline ColorLinear coloring_linear_unpremultiply(ColorPRGBA p)
{
    ColorLinear c;
    if (p.a <= 1.0f/255.0f) { // treat sub-1 LSB alpha as transparent
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
static inline ColorPRGBA coloring_prgba_from_srgb(ColorSRGB c)
{
    return coloring_linear_premultiply(coloring_srgb_to_linear(c));
}

// Go from a pre-multiplied linear color to a sRGB straight color
// Convenience: PRGBA linear -> sRGB straight 
static inline ColorSRGB coloring_srgb_from_prgba(ColorPRGBA p)
{
    return coloring_linear_to_srgb(coloring_linear_unpremultiply(p));
}


// Common operations operating in premultiplied linear space
// 
// Squared Euclidean distance in linear RGB (straight)
// Calculate how close two colors are to each other
static inline float coloring_linear_color_distance(ColorLinear a, ColorLinear b)
{
    float dr = a.r - b.r;
    float dg = a.g - b.g;
    float db = a.b - b.b;
    return dr * dr + dg * dg + db * db;
}

// Luminance difference in linear RGB (straight) 
static inline float coloring_linear_luminance_diff(ColorLinear a, ColorLinear b)
{
    float ya = coloring_relative_luminance_linear(&a);
    float yb = coloring_relative_luminance_linear(&b);
    float d = ya - yb;
    return d < 0.0f ? -d : d;
}


// Lerp two premultiplied colors in linear space
// fmaf - fuse, multiply - add
static inline ColorPRGBA coloring_prgba_lerp(const ColorPRGBA a, const ColorPRGBA b, float t)
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
static inline ColorPRGBA coloring_prgba_over(const ColorPRGBA src, const ColorPRGBA dst)
{
    ColorPRGBA o;
    float oa = 1.0f - src.a;
    o.r = WAAVS_FMAF(oa, dst.r, src.r);
    o.g = WAAVS_FMAF(oa, dst.g, src.g);
    o.b = WAAVS_FMAF(oa, dst.b, src.b);
    o.a = WAAVS_FMAF(oa, dst.a, src.a);
    return o;
}





#endif // COLORING_H_INCLUDED