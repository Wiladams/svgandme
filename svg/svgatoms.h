#pragma once


#include <functional>

#include "nametable.h"



// These are interned keys for various things within SVG.
// Mostly it's the attributres names, but also some keywords
// The element names are taken care of within the registration
// of those factory methods.  They can be placed here as well though
// so there's no ambiguity.


namespace waavs::svgattr
{
    // ============================================================
    // Actively used / modern SVG attributes
    // ============================================================

    // Core identity & structure
    inline InternedKey id() { static InternedKey k = PSNameTable::INTERN("id");            return k; }
    inline InternedKey klass() { static InternedKey k = PSNameTable::INTERN("class");         return k; }
    inline InternedKey style() { static InternedKey k = PSNameTable::INTERN("style");         return k; }
    inline InternedKey display() { static InternedKey k = PSNameTable::INTERN("display");       return k; }
    inline InternedKey visibility() { static InternedKey k = PSNameTable::INTERN("visibility");    return k; }
    inline InternedKey opacity() { static InternedKey k = PSNameTable::INTERN("opacity");       return k; }

    // Geometry / positioning
    inline InternedKey x() { static InternedKey k = PSNameTable::INTERN("x");             return k; }
    inline InternedKey y() { static InternedKey k = PSNameTable::INTERN("y");             return k; }
    inline InternedKey x1() { static InternedKey k = PSNameTable::INTERN("x1");            return k; }
    inline InternedKey y1() { static InternedKey k = PSNameTable::INTERN("y1");            return k; }
    inline InternedKey x2() { static InternedKey k = PSNameTable::INTERN("x2");            return k; }
    inline InternedKey y2() { static InternedKey k = PSNameTable::INTERN("y2");            return k; }
    inline InternedKey cx() { static InternedKey k = PSNameTable::INTERN("cx");            return k; }
    inline InternedKey cy() { static InternedKey k = PSNameTable::INTERN("cy");            return k; }
    inline InternedKey r() { static InternedKey k = PSNameTable::INTERN("r");             return k; }
    inline InternedKey rx() { static InternedKey k = PSNameTable::INTERN("rx");            return k; }
    inline InternedKey ry() { static InternedKey k = PSNameTable::INTERN("ry");            return k; }
    inline InternedKey width() { static InternedKey k = PSNameTable::INTERN("width");         return k; }
    inline InternedKey height() { static InternedKey k = PSNameTable::INTERN("height");        return k; }

    // Paths & shapes
    inline InternedKey d() { static InternedKey k = PSNameTable::INTERN("d");             return k; }
    inline InternedKey points() { static InternedKey k = PSNameTable::INTERN("points");        return k; }

    // Viewport & aspect
    inline InternedKey viewBox() { static InternedKey k = PSNameTable::INTERN("viewBox");       return k; }
    inline InternedKey preserveAspectRatio()
    {
        static InternedKey k = PSNameTable::INTERN("preserveAspectRatio"); return k;
    }

    // Transforms
    inline InternedKey transform() { static InternedKey k = PSNameTable::INTERN("transform");     return k; }

    // Paint & stroke
    inline InternedKey fill() { static InternedKey k = PSNameTable::INTERN("fill");          return k; }
    inline InternedKey fill_opacity() { static InternedKey k = PSNameTable::INTERN("fill-opacity");  return k; }
    inline InternedKey fill_rule() { static InternedKey k = PSNameTable::INTERN("fill-rule");     return k; }

    inline InternedKey stroke() { static InternedKey k = PSNameTable::INTERN("stroke");        return k; }
    inline InternedKey stroke_width() { static InternedKey k = PSNameTable::INTERN("stroke-width");  return k; }
    inline InternedKey stroke_opacity() { static InternedKey k = PSNameTable::INTERN("stroke-opacity"); return k; }
    inline InternedKey stroke_linecap() { static InternedKey k = PSNameTable::INTERN("stroke-linecap"); return k; }
    inline InternedKey stroke_linejoin()
    {
        static InternedKey k = PSNameTable::INTERN("stroke-linejoin"); return k;
    }
    inline InternedKey stroke_miterlimit()
    {
        static InternedKey k = PSNameTable::INTERN("stroke-miterlimit"); return k;
    }
    inline InternedKey stroke_dasharray()
    {
        static InternedKey k = PSNameTable::INTERN("stroke-dasharray"); return k;
    }
    inline InternedKey stroke_dashoffset()
    {
        static InternedKey k = PSNameTable::INTERN("stroke-dashoffset"); return k;
    }

    // Text
    inline InternedKey font_family() { static InternedKey k = PSNameTable::INTERN("font-family");   return k; }
    inline InternedKey font_size() { static InternedKey k = PSNameTable::INTERN("font-size");     return k; }
    inline InternedKey font_weight() { static InternedKey k = PSNameTable::INTERN("font-weight");   return k; }
    inline InternedKey font_style() { static InternedKey k = PSNameTable::INTERN("font-style");    return k; }
    inline InternedKey text_anchor() { static InternedKey k = PSNameTable::INTERN("text-anchor");   return k; }
    inline InternedKey dominant_baseline()
    {
        static InternedKey k = PSNameTable::INTERN("dominant-baseline"); return k;
    }
    inline InternedKey alignment_baseline()
    {
        static InternedKey k = PSNameTable::INTERN("alignment-baseline"); return k;
    }

    // Linking / reuse
    inline InternedKey href() { static InternedKey k = PSNameTable::INTERN("href");          return k; }
    inline InternedKey xlink_href() { static InternedKey k = PSNameTable::INTERN("xlink:href");    return k; }

    // Clipping / masking
    inline InternedKey clip_path() { static InternedKey k = PSNameTable::INTERN("clip-path");     return k; }
    inline InternedKey mask() { static InternedKey k = PSNameTable::INTERN("mask");          return k; }

    // Filters / effects
    inline InternedKey filter() { static InternedKey k = PSNameTable::INTERN("filter");        return k; }

    // Gradients / patterns
    inline InternedKey gradientUnits() { static InternedKey k = PSNameTable::INTERN("gradientUnits"); return k; }
    inline InternedKey gradientTransform()
    {
        static InternedKey k = PSNameTable::INTERN("gradientTransform"); return k;
    }
    inline InternedKey spreadMethod() { static InternedKey k = PSNameTable::INTERN("spreadMethod");  return k; }

    inline InternedKey patternUnits() { static InternedKey k = PSNameTable::INTERN("patternUnits");  return k; }
    inline InternedKey patternTransform()
    {
        static InternedKey k = PSNameTable::INTERN("patternTransform"); return k;
    }

    // Markers
    inline InternedKey marker_start() { static InternedKey k = PSNameTable::INTERN("marker-start");  return k; }
    inline InternedKey marker_mid() { static InternedKey k = PSNameTable::INTERN("marker-mid");    return k; }
    inline InternedKey marker_end() { static InternedKey k = PSNameTable::INTERN("marker-end");    return k; }

    // ------------------------------------------------------------
    // SVG2 / modern additions you may see
    // ------------------------------------------------------------
    inline InternedKey vector_effect() { static InternedKey k = PSNameTable::INTERN("vector-effect"); return k; }
    inline InternedKey paint_order() { static InternedKey k = PSNameTable::INTERN("paint-order");   return k; }
    inline InternedKey shape_rendering()
    {
        static InternedKey k = PSNameTable::INTERN("shape-rendering"); return k;
    }
    inline InternedKey text_rendering()
    {
        static InternedKey k = PSNameTable::INTERN("text-rendering"); return k;
    }
    inline InternedKey image_rendering()
    {
        static InternedKey k = PSNameTable::INTERN("image-rendering"); return k;
    }

    // ------------------------------------------------------------
    // Event / interactivity attributes (still appear)
    // ------------------------------------------------------------
    inline InternedKey onclick() { static InternedKey k = PSNameTable::INTERN("onclick");      return k; }
    inline InternedKey onmouseover() { static InternedKey k = PSNameTable::INTERN("onmouseover");  return k; }
    inline InternedKey onmouseout() { static InternedKey k = PSNameTable::INTERN("onmouseout");   return k; }
}



namespace waavs::svgval
{
    // ============================================================
    // Actively used / modern SVG keyword values
    // ============================================================

    // Generic / global
    inline InternedKey none() { static InternedKey k = PSNameTable::INTERN("none");            return k; }
    inline InternedKey inherit() { static InternedKey k = PSNameTable::INTERN("inherit");         return k; }
    inline InternedKey initial() { static InternedKey k = PSNameTable::INTERN("initial");         return k; }
    inline InternedKey unset() { static InternedKey k = PSNameTable::INTERN("unset");           return k; }

    // Paint keywords
    inline InternedKey currentColor() { static InternedKey k = PSNameTable::INTERN("currentColor");    return k; }
    inline InternedKey context_fill() { static InternedKey k = PSNameTable::INTERN("context-fill");    return k; }
    inline InternedKey context_stroke() { static InternedKey k = PSNameTable::INTERN("context-stroke");  return k; }

    // Display / visibility
    inline InternedKey inline_() { static InternedKey k = PSNameTable::INTERN("inline");          return k; }
    inline InternedKey block() { static InternedKey k = PSNameTable::INTERN("block");           return k; }
    inline InternedKey visible() { static InternedKey k = PSNameTable::INTERN("visible");         return k; }
    inline InternedKey hidden() { static InternedKey k = PSNameTable::INTERN("hidden");          return k; }
    inline InternedKey collapse() { static InternedKey k = PSNameTable::INTERN("collapse");        return k; }

    // Fill / stroke rules
    inline InternedKey nonzero() { static InternedKey k = PSNameTable::INTERN("nonzero");         return k; }
    inline InternedKey evenodd() { static InternedKey k = PSNameTable::INTERN("evenodd");         return k; }

    // Stroke linecap
    inline InternedKey butt() { static InternedKey k = PSNameTable::INTERN("butt");            return k; }
    inline InternedKey round() { static InternedKey k = PSNameTable::INTERN("round");           return k; }
    inline InternedKey square() { static InternedKey k = PSNameTable::INTERN("square");          return k; }

    // Stroke linejoin
    inline InternedKey miter() { static InternedKey k = PSNameTable::INTERN("miter");           return k; }
    inline InternedKey bevel() { static InternedKey k = PSNameTable::INTERN("bevel");           return k; }

    // Marker placement
    inline InternedKey auto_() { static InternedKey k = PSNameTable::INTERN("auto");            return k; }

    // Units
    inline InternedKey userSpaceOnUse() { static InternedKey k = PSNameTable::INTERN("userSpaceOnUse");  return k; }
    inline InternedKey objectBoundingBox()
    {
        static InternedKey k = PSNameTable::INTERN("objectBoundingBox"); return k;
    }

    // PreserveAspectRatio
    inline InternedKey meet() { static InternedKey k = PSNameTable::INTERN("meet");            return k; }
    inline InternedKey slice() { static InternedKey k = PSNameTable::INTERN("slice");           return k; }

    inline InternedKey xMinYMin() { static InternedKey k = PSNameTable::INTERN("xMinYMin");        return k; }
    inline InternedKey xMidYMin() { static InternedKey k = PSNameTable::INTERN("xMidYMin");        return k; }
    inline InternedKey xMaxYMin() { static InternedKey k = PSNameTable::INTERN("xMaxYMin");        return k; }

    inline InternedKey xMinYMid() { static InternedKey k = PSNameTable::INTERN("xMinYMid");        return k; }
    inline InternedKey xMidYMid() { static InternedKey k = PSNameTable::INTERN("xMidYMid");        return k; }
    inline InternedKey xMaxYMid() { static InternedKey k = PSNameTable::INTERN("xMaxYMid");        return k; }

    inline InternedKey xMinYMax() { static InternedKey k = PSNameTable::INTERN("xMinYMax");        return k; }
    inline InternedKey xMidYMax() { static InternedKey k = PSNameTable::INTERN("xMidYMax");        return k; }
    inline InternedKey xMaxYMax() { static InternedKey k = PSNameTable::INTERN("xMaxYMax");        return k; }

    // Vector effects
    inline InternedKey non_scaling_stroke() { static InternedKey k = PSNameTable::INTERN("non-scaling-stroke"); return k;}


    // Text positioning
    inline InternedKey dx() { static InternedKey k = PSNameTable::INTERN("dx"); return k; }
    inline InternedKey dy() { static InternedKey k = PSNameTable::INTERN("dy"); return k; }

    // Text anchor
    inline InternedKey start() { static InternedKey k = PSNameTable::INTERN("start");           return k; }
    inline InternedKey middle() { static InternedKey k = PSNameTable::INTERN("middle");          return k; }
    inline InternedKey end() { static InternedKey k = PSNameTable::INTERN("end");             return k; }

    // Font weight
    inline InternedKey normal() { static InternedKey k = PSNameTable::INTERN("normal");          return k; }
    inline InternedKey bold() { static InternedKey k = PSNameTable::INTERN("bold");            return k; }
    inline InternedKey bolder() { static InternedKey k = PSNameTable::INTERN("bolder");          return k; }
    inline InternedKey lighter() { static InternedKey k = PSNameTable::INTERN("lighter");         return k; }

    // Font style
    inline InternedKey italic() { static InternedKey k = PSNameTable::INTERN("italic");          return k; }
    inline InternedKey oblique() { static InternedKey k = PSNameTable::INTERN("oblique");         return k; }

    // Shape rendering
    inline InternedKey auto_rendering() { static InternedKey k = PSNameTable::INTERN("auto");             return k; }
    inline InternedKey optimizeSpeed() { static InternedKey k = PSNameTable::INTERN("optimizeSpeed");   return k; }
    inline InternedKey crispEdges() { static InternedKey k = PSNameTable::INTERN("crispEdges");      return k; }
    inline InternedKey geometricPrecision()
    {
        static InternedKey k = PSNameTable::INTERN("geometricPrecision"); return k;
    }

    // Image rendering
    inline InternedKey pixelated() { static InternedKey k = PSNameTable::INTERN("pixelated");       return k; }

    // Spread methods (gradients)
    inline InternedKey pad() { static InternedKey k = PSNameTable::INTERN("pad");             return k; }
    inline InternedKey reflect() { static InternedKey k = PSNameTable::INTERN("reflect");         return k; }
    inline InternedKey repeat() { static InternedKey k = PSNameTable::INTERN("repeat");          return k; }

    // Mask / clip
    inline InternedKey luminance() { static InternedKey k = PSNameTable::INTERN("luminance");       return k; }
    inline InternedKey alpha() { static InternedKey k = PSNameTable::INTERN("alpha");           return k; }

    // Pointer events (still emitted)
    inline InternedKey visiblePainted() { static InternedKey k = PSNameTable::INTERN("visiblePainted");  return k; }
    inline InternedKey visibleFill() { static InternedKey k = PSNameTable::INTERN("visibleFill");     return k; }
    inline InternedKey visibleStroke() { static InternedKey k = PSNameTable::INTERN("visibleStroke");   return k; }
    inline InternedKey visibleAll() { static InternedKey k = PSNameTable::INTERN("visible");         return k; }

    inline InternedKey painted() { static InternedKey k = PSNameTable::INTERN("painted");         return k; }
    inline InternedKey fill_kw() { static InternedKey k = PSNameTable::INTERN("fill");             return k; }
    inline InternedKey stroke_kw() { static InternedKey k = PSNameTable::INTERN("stroke");           return k; }
    inline InternedKey all() { static InternedKey k = PSNameTable::INTERN("all");              return k; }
    inline InternedKey none_events() { static InternedKey k = PSNameTable::INTERN("none");             return k; }

    // ------------------------------------------------------------
    // SVG2 additions commonly seen
    // ------------------------------------------------------------
    inline InternedKey context_value() { static InternedKey k = PSNameTable::INTERN("context-value");    return k; }
}





namespace waavs::svgval_legacy
{
    // ============================================================
    // Deprecated / legacy paint & text keywords
    // ============================================================

    inline InternedKey freeze() { static InternedKey k = PSNameTable::INTERN("freeze");          return k; }
    inline InternedKey remove() { static InternedKey k = PSNameTable::INTERN("remove");          return k; }

    // Text layout (SMIL / old text-flow)
    inline InternedKey spacing() { static InternedKey k = PSNameTable::INTERN("spacing");         return k; }
    inline InternedKey spacingAndGlyphs()
    {
        static InternedKey k = PSNameTable::INTERN("spacingAndGlyphs"); return k;
    }

    // Alignment baseline legacy values
    inline InternedKey auto_baseline() { static InternedKey k = PSNameTable::INTERN("auto");             return k; }
    inline InternedKey baseline() { static InternedKey k = PSNameTable::INTERN("baseline");        return k; }
    inline InternedKey before_edge() { static InternedKey k = PSNameTable::INTERN("before-edge");     return k; }
    inline InternedKey text_before_edge()
    {
        static InternedKey k = PSNameTable::INTERN("text-before-edge"); return k;
    }
    inline InternedKey middle_baseline() { static InternedKey k = PSNameTable::INTERN("middle");          return k; }
    inline InternedKey central() { static InternedKey k = PSNameTable::INTERN("central");         return k; }
    inline InternedKey after_edge() { static InternedKey k = PSNameTable::INTERN("after-edge");      return k; }
    inline InternedKey text_after_edge()
    {
        static InternedKey k = PSNameTable::INTERN("text-after-edge"); return k;
    }
    inline InternedKey ideographic_baseline()
    {
        static InternedKey k = PSNameTable::INTERN("ideographic");     return k;
    }
    inline InternedKey alphabetic_baseline()
    {
        static InternedKey k = PSNameTable::INTERN("alphabetic");      return k;
    }
    inline InternedKey hanging_baseline()
    {
        static InternedKey k = PSNameTable::INTERN("hanging");         return k;
    }
    inline InternedKey mathematical_baseline()
    {
        static InternedKey k = PSNameTable::INTERN("mathematical");    return k;
    }

    // Writing mode legacy
    inline InternedKey lr_tb() { static InternedKey k = PSNameTable::INTERN("lr-tb");            return k; }
    inline InternedKey rl_tb() { static InternedKey k = PSNameTable::INTERN("rl-tb");            return k; }
    inline InternedKey tb_rl() { static InternedKey k = PSNameTable::INTERN("tb-rl");            return k; }

    // Glyph orientation legacy
    inline InternedKey auto_glyph() { static InternedKey k = PSNameTable::INTERN("auto");             return k; }
    inline InternedKey deg0() { static InternedKey k = PSNameTable::INTERN("0deg");             return k; }
    inline InternedKey deg90() { static InternedKey k = PSNameTable::INTERN("90deg");            return k; }
    inline InternedKey deg180() { static InternedKey k = PSNameTable::INTERN("180deg");           return k; }
    inline InternedKey deg270() { static InternedKey k = PSNameTable::INTERN("270deg");           return k; }

    // Legacy filter units
    inline InternedKey userSpace() { static InternedKey k = PSNameTable::INTERN("userSpace");        return k; }
    inline InternedKey objectBBox() { static InternedKey k = PSNameTable::INTERN("objectBoundingBox"); return k; }

    // Legacy cursor keywords
    inline InternedKey crosshair() { static InternedKey k = PSNameTable::INTERN("crosshair");        return k; }
    inline InternedKey pointer() { static InternedKey k = PSNameTable::INTERN("pointer");          return k; }
    inline InternedKey move() { static InternedKey k = PSNameTable::INTERN("move");             return k; }
    inline InternedKey e_resize() { static InternedKey k = PSNameTable::INTERN("e-resize");         return k; }
    inline InternedKey ne_resize() { static InternedKey k = PSNameTable::INTERN("ne-resize");        return k; }
    inline InternedKey nw_resize() { static InternedKey k = PSNameTable::INTERN("nw-resize");        return k; }
    inline InternedKey n_resize() { static InternedKey k = PSNameTable::INTERN("n-resize");         return k; }
    inline InternedKey se_resize() { static InternedKey k = PSNameTable::INTERN("se-resize");        return k; }
    inline InternedKey sw_resize() { static InternedKey k = PSNameTable::INTERN("sw-resize");        return k; }
    inline InternedKey s_resize() { static InternedKey k = PSNameTable::INTERN("s-resize");         return k; }
    inline InternedKey w_resize() { static InternedKey k = PSNameTable::INTERN("w-resize");         return k; }
}






//============================================================
// SVG element tag names
//============================================================
namespace waavs::svgtag
{
    // Document / container / structure
    inline InternedKey tag_svg() { static InternedKey k = PSNameTable::INTERN("svg");          return k; }
    inline InternedKey tag_g() { static InternedKey k = PSNameTable::INTERN("g");            return k; }
    inline InternedKey tag_defs() { static InternedKey k = PSNameTable::INTERN("defs");         return k; }
    inline InternedKey tag_symbol() { static InternedKey k = PSNameTable::INTERN("symbol");       return k; }
    inline InternedKey tag_use() { static InternedKey k = PSNameTable::INTERN("use");          return k; }
    inline InternedKey tag_switch_() { static InternedKey k = PSNameTable::INTERN("switch");       return k; }
    inline InternedKey tag_view() { static InternedKey k = PSNameTable::INTERN("view");         return k; }

    // Linking / scripting
    inline InternedKey tag_a() { static InternedKey k = PSNameTable::INTERN("a");            return k; }
    inline InternedKey tag_script() { static InternedKey k = PSNameTable::INTERN("script");       return k; }

    // Descriptive / metadata
    inline InternedKey tag_title() { static InternedKey k = PSNameTable::INTERN("title");        return k; }
    inline InternedKey tag_desc() { static InternedKey k = PSNameTable::INTERN("desc");         return k; }
    inline InternedKey tag_metadata() { static InternedKey k = PSNameTable::INTERN("metadata");     return k; }
    inline InternedKey tag_style() { static InternedKey k = PSNameTable::INTERN("style");        return k; }

    // Basic shapes / graphics
    inline InternedKey tag_circle() { static InternedKey k = PSNameTable::INTERN("circle");       return k; }
    inline InternedKey tag_ellipse() { static InternedKey k = PSNameTable::INTERN("ellipse");      return k; }
    inline InternedKey tag_line() { static InternedKey k = PSNameTable::INTERN("line");         return k; }
    inline InternedKey tag_rect() { static InternedKey k = PSNameTable::INTERN("rect");         return k; }
    inline InternedKey tag_path() { static InternedKey k = PSNameTable::INTERN("path");         return k; }
    inline InternedKey tag_polygon() { static InternedKey k = PSNameTable::INTERN("polygon");      return k; }
    inline InternedKey tag_polyline() { static InternedKey k = PSNameTable::INTERN("polyline");     return k; }
    inline InternedKey tag_image() { static InternedKey k = PSNameTable::INTERN("image");        return k; }
    inline InternedKey tag_foreignObject() { static InternedKey k = PSNameTable::INTERN("foreignObject"); return k; }

    // Text
    inline InternedKey tag_text() { static InternedKey k = PSNameTable::INTERN("text");         return k; }
    inline InternedKey tag_tspan() { static InternedKey k = PSNameTable::INTERN("tspan");        return k; }
    inline InternedKey tag_textPath() { static InternedKey k = PSNameTable::INTERN("textPath");     return k; }

    // Clipping / masking
    inline InternedKey tag_clipPath() { static InternedKey k = PSNameTable::INTERN("clipPath");     return k; }
    inline InternedKey tag_mask() { static InternedKey k = PSNameTable::INTERN("mask");         return k; }

    // Gradients / paint servers
    inline InternedKey tag_linearGradient() { static InternedKey k = PSNameTable::INTERN("linearGradient"); return k; }
    inline InternedKey tag_radialGradient() { static InternedKey k = PSNameTable::INTERN("radialGradient"); return k; }
    inline InternedKey tag_stop() { static InternedKey k = PSNameTable::INTERN("stop");         return k; }
    inline InternedKey tag_pattern() { static InternedKey k = PSNameTable::INTERN("pattern");      return k; }
    inline InternedKey tag_marker() { static InternedKey k = PSNameTable::INTERN("marker");       return k; }

    // Filters
    inline InternedKey tag_filter() { static InternedKey k = PSNameTable::INTERN("filter");       return k; }
    inline InternedKey tag_feBlend() { static InternedKey k = PSNameTable::INTERN("feBlend");      return k; }
    inline InternedKey tag_feColorMatrix() { static InternedKey k = PSNameTable::INTERN("feColorMatrix"); return k; }
    inline InternedKey tag_feComponentTransfer() { static InternedKey k = PSNameTable::INTERN("feComponentTransfer"); return k; }
    inline InternedKey tag_feComposite() { static InternedKey k = PSNameTable::INTERN("feComposite");  return k; }
    inline InternedKey tag_feConvolveMatrix() { static InternedKey k = PSNameTable::INTERN("feConvolveMatrix"); return k; }
    inline InternedKey tag_feDiffuseLighting() { static InternedKey k = PSNameTable::INTERN("feDiffuseLighting"); return k; }
    inline InternedKey tag_feDisplacementMap() { static InternedKey k = PSNameTable::INTERN("feDisplacementMap"); return k; }
    inline InternedKey tag_feDistantLight() { static InternedKey k = PSNameTable::INTERN("feDistantLight"); return k; }
    inline InternedKey tag_feDropShadow() { static InternedKey k = PSNameTable::INTERN("feDropShadow");  return k; }
    inline InternedKey tag_feFlood() { static InternedKey k = PSNameTable::INTERN("feFlood");      return k; }
    inline InternedKey tag_feFuncA() { static InternedKey k = PSNameTable::INTERN("feFuncA");      return k; }
    inline InternedKey tag_feFuncB() { static InternedKey k = PSNameTable::INTERN("feFuncB");      return k; }
    inline InternedKey tag_feFuncG() { static InternedKey k = PSNameTable::INTERN("feFuncG");      return k; }
    inline InternedKey tag_feFuncR() { static InternedKey k = PSNameTable::INTERN("feFuncR");      return k; }
    inline InternedKey tag_feGaussianBlur() { static InternedKey k = PSNameTable::INTERN("feGaussianBlur"); return k; }
    inline InternedKey tag_feImage() { static InternedKey k = PSNameTable::INTERN("feImage");      return k; }
    inline InternedKey tag_feMerge() { static InternedKey k = PSNameTable::INTERN("feMerge");      return k; }
    inline InternedKey tag_feMergeNode() { static InternedKey k = PSNameTable::INTERN("feMergeNode");  return k; }
    inline InternedKey tag_feMorphology() { static InternedKey k = PSNameTable::INTERN("feMorphology"); return k; }
    inline InternedKey tag_feOffset() { static InternedKey k = PSNameTable::INTERN("feOffset");     return k; }
    inline InternedKey tag_fePointLight() { static InternedKey k = PSNameTable::INTERN("fePointLight"); return k; }
    inline InternedKey tag_feSpecularLighting() { static InternedKey k = PSNameTable::INTERN("feSpecularLighting"); return k; }
    inline InternedKey tag_feSpotLight() { static InternedKey k = PSNameTable::INTERN("feSpotLight");  return k; }
    inline InternedKey tag_feTile() { static InternedKey k = PSNameTable::INTERN("feTile");       return k; }
    inline InternedKey tag_feTurbulence() { static InternedKey k = PSNameTable::INTERN("feTurbulence"); return k; }

    // Animation (SMIL)
    inline InternedKey tag_animate() { static InternedKey k = PSNameTable::INTERN("animate");      return k; }
    inline InternedKey tag_animateMotion() { static InternedKey k = PSNameTable::INTERN("animateMotion"); return k; }
    inline InternedKey tag_animateTransform() { static InternedKey k = PSNameTable::INTERN("animateTransform"); return k; }
    inline InternedKey tag_set() { static InternedKey k = PSNameTable::INTERN("set");          return k; }
    inline InternedKey tag_mpath() { static InternedKey k = PSNameTable::INTERN("mpath");        return k; }

    // SVG2 “specials” that can appear (rare, but show up in the SVG2 element index)
    inline InternedKey tag_discard() { static InternedKey k = PSNameTable::INTERN("discard");      return k; }
    inline InternedKey tag_unknown() { static InternedKey k = PSNameTable::INTERN("unknown");      return k; }
}


namespace waavs::svgtag_legacy
{
    // ------------------------------------------------------------
    // SVG Fonts (deprecated/removed in SVG2; common in older assets)
    // ------------------------------------------------------------
    inline InternedKey tag_font() { static InternedKey k = PSNameTable::INTERN("font");              return k; }
    inline InternedKey tag_font_face() { static InternedKey k = PSNameTable::INTERN("font-face");         return k; }
    inline InternedKey tag_font_face_src() { static InternedKey k = PSNameTable::INTERN("font-face-src");     return k; }
    inline InternedKey tag_font_face_uri() { static InternedKey k = PSNameTable::INTERN("font-face-uri");     return k; }
    inline InternedKey tag_font_face_name() { static InternedKey k = PSNameTable::INTERN("font-face-name");    return k; }
    inline InternedKey tag_font_face_format() { static InternedKey k = PSNameTable::INTERN("font-face-format");  return k; }

    inline InternedKey tag_glyph() { static InternedKey k = PSNameTable::INTERN("glyph");             return k; }
    inline InternedKey tag_missing_glyph() { static InternedKey k = PSNameTable::INTERN("missing-glyph");     return k; }
    inline InternedKey tag_hkern() { static InternedKey k = PSNameTable::INTERN("hkern");             return k; }
    inline InternedKey tag_vkern() { static InternedKey k = PSNameTable::INTERN("vkern");             return k; }

    // Less common SVG-font support elements seen in some generators
    inline InternedKey tag_definition_src() { static InternedKey k = PSNameTable::INTERN("definition-src");    return k; }

    // ------------------------------------------------------------
    // Alternate glyph mechanism (deprecated/removed)
    // ------------------------------------------------------------
    inline InternedKey tag_altGlyph() { static InternedKey k = PSNameTable::INTERN("altGlyph");          return k; }
    inline InternedKey tag_altGlyphDef() { static InternedKey k = PSNameTable::INTERN("altGlyphDef");       return k; }
    inline InternedKey tag_altGlyphItem() { static InternedKey k = PSNameTable::INTERN("altGlyphItem");      return k; }
    inline InternedKey tag_glyphRef() { static InternedKey k = PSNameTable::INTERN("glyphRef");          return k; }

    // ------------------------------------------------------------
    // Legacy text reference element (deprecated/removed)
    // ------------------------------------------------------------
    inline InternedKey tag_tref() { static InternedKey k = PSNameTable::INTERN("tref");              return k; }

    // ------------------------------------------------------------
    // Deprecated/removed metadata-ish elements
    // ------------------------------------------------------------
    inline InternedKey tag_color_profile() { static InternedKey k = PSNameTable::INTERN("color-profile");     return k; }
    inline InternedKey tag_cursor() { static InternedKey k = PSNameTable::INTERN("cursor");            return k; }

    // ------------------------------------------------------------
    // Rare legacy scripting/event element (SVG Tiny / older content)
    // ------------------------------------------------------------
    inline InternedKey tag_handler() { static InternedKey k = PSNameTable::INTERN("handler");           return k; }

    // ------------------------------------------------------------
    // SMIL-related deprecated animation variants you might see
    // (you already have animate/animateMotion/animateTransform/set/mpath
    //  in your modern list; this covers older extras)
    // ------------------------------------------------------------
    inline InternedKey tag_animateColor() { static InternedKey k = PSNameTable::INTERN("animateColor");      return k; }
}








namespace waavs::svgattr_legacy
{
    // ============================================================
    // SVG Fonts (deprecated / removed)
    // ============================================================
    inline InternedKey horiz_adv_x() { static InternedKey k = PSNameTable::INTERN("horiz-adv-x");      return k; }
    inline InternedKey horiz_origin_x() { static InternedKey k = PSNameTable::INTERN("horiz-origin-x");   return k; }
    inline InternedKey horiz_origin_y() { static InternedKey k = PSNameTable::INTERN("horiz-origin-y");   return k; }
    inline InternedKey vert_adv_y() { static InternedKey k = PSNameTable::INTERN("vert-adv-y");       return k; }
    inline InternedKey vert_origin_x() { static InternedKey k = PSNameTable::INTERN("vert-origin-x");    return k; }
    inline InternedKey vert_origin_y() { static InternedKey k = PSNameTable::INTERN("vert-origin-y");    return k; }

    inline InternedKey unicode() { static InternedKey k = PSNameTable::INTERN("unicode");          return k; }
    inline InternedKey glyph_name() { static InternedKey k = PSNameTable::INTERN("glyph-name");       return k; }
    inline InternedKey arabic_form() { static InternedKey k = PSNameTable::INTERN("arabic-form");      return k; }
    inline InternedKey lang() { static InternedKey k = PSNameTable::INTERN("lang");             return k; }
    inline InternedKey orientation() { static InternedKey k = PSNameTable::INTERN("orientation");      return k; }

    inline InternedKey panose_1() { static InternedKey k = PSNameTable::INTERN("panose-1");          return k; }
    inline InternedKey units_per_em() { static InternedKey k = PSNameTable::INTERN("units-per-em");      return k; }
    inline InternedKey ascent() { static InternedKey k = PSNameTable::INTERN("ascent");            return k; }
    inline InternedKey descent() { static InternedKey k = PSNameTable::INTERN("descent");           return k; }
    inline InternedKey alphabetic() { static InternedKey k = PSNameTable::INTERN("alphabetic");        return k; }
    inline InternedKey mathematical() { static InternedKey k = PSNameTable::INTERN("mathematical");      return k; }
    inline InternedKey ideographic() { static InternedKey k = PSNameTable::INTERN("ideographic");       return k; }
    inline InternedKey hanging() { static InternedKey k = PSNameTable::INTERN("hanging");           return k; }
    inline InternedKey v_ideographic() { static InternedKey k = PSNameTable::INTERN("v-ideographic");     return k; }

    inline InternedKey underline_position() { static InternedKey k = PSNameTable::INTERN("underline-position"); return k; }
    inline InternedKey underline_thickness()
    {
        static InternedKey k = PSNameTable::INTERN("underline-thickness"); return k;
    }
    inline InternedKey strikethrough_position()
    {
        static InternedKey k = PSNameTable::INTERN("strikethrough-position"); return k;
    }
    inline InternedKey strikethrough_thickness()
    {
        static InternedKey k = PSNameTable::INTERN("strikethrough-thickness"); return k;
    }

    // ------------------------------------------------------------
    // Alternate glyph system (deprecated / removed)
    // ------------------------------------------------------------
    inline InternedKey glyph_ref() { static InternedKey k = PSNameTable::INTERN("glyphRef");         return k; }
    inline InternedKey alt_glyph() { static InternedKey k = PSNameTable::INTERN("altGlyph");         return k; }
    inline InternedKey alt_glyph_def() { static InternedKey k = PSNameTable::INTERN("altGlyphDef");      return k; }
    inline InternedKey alt_glyph_item() { static InternedKey k = PSNameTable::INTERN("altGlyphItem");     return k; }

    // ------------------------------------------------------------
    // Deprecated text / linking
    // ------------------------------------------------------------
    inline InternedKey tref() { static InternedKey k = PSNameTable::INTERN("tref");              return k; }

    // ------------------------------------------------------------
    // Color profile / cursor (deprecated)
    // ------------------------------------------------------------
    inline InternedKey color_profile() { static InternedKey k = PSNameTable::INTERN("color-profile");     return k; }
    inline InternedKey cursor() { static InternedKey k = PSNameTable::INTERN("cursor");            return k; }

    // ------------------------------------------------------------
    // Rare / obsolete scripting / metadata
    // ------------------------------------------------------------
    inline InternedKey baseProfile() { static InternedKey k = PSNameTable::INTERN("baseProfile");       return k; }
    inline InternedKey version() { static InternedKey k = PSNameTable::INTERN("version");           return k; }

    // ------------------------------------------------------------
    // Deprecated SMIL animation variants
    // ------------------------------------------------------------
    inline InternedKey animateColor() { static InternedKey k = PSNameTable::INTERN("animateColor");      return k; }
}

