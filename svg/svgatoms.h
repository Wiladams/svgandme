#pragma once


#include <functional>

#include "core_nametable.h"



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
    inline InternedKey id() { static InternedKey k = WSNameSet::INTERN("id");            return k; }
    inline InternedKey klass() { static InternedKey k = WSNameSet::INTERN("class");         return k; }
    inline InternedKey style() { static InternedKey k = WSNameSet::INTERN("style");         return k; }
    inline InternedKey display() { static InternedKey k = WSNameSet::INTERN("display");       return k; }
    inline InternedKey visibility() { static InternedKey k = WSNameSet::INTERN("visibility");    return k; }
    inline InternedKey systemLanguage() { static InternedKey k = WSNameSet::INTERN("systemLanguage"); return k; }
    inline InternedKey opacity() { static InternedKey k = WSNameSet::INTERN("opacity");       return k; }

    // Geometry / positioning
    inline InternedKey x() { static InternedKey k = WSNameSet::INTERN("x");             return k; }
    inline InternedKey y() { static InternedKey k = WSNameSet::INTERN("y");             return k; }
    inline InternedKey x1() { static InternedKey k = WSNameSet::INTERN("x1");            return k; }
    inline InternedKey y1() { static InternedKey k = WSNameSet::INTERN("y1");            return k; }
    inline InternedKey x2() { static InternedKey k = WSNameSet::INTERN("x2");            return k; }
    inline InternedKey y2() { static InternedKey k = WSNameSet::INTERN("y2");            return k; }
    inline InternedKey cx() { static InternedKey k = WSNameSet::INTERN("cx");            return k; }
    inline InternedKey cy() { static InternedKey k = WSNameSet::INTERN("cy");            return k; }
    inline InternedKey r() { static InternedKey k = WSNameSet::INTERN("r");             return k; }
    inline InternedKey rx() { static InternedKey k = WSNameSet::INTERN("rx");            return k; }
    inline InternedKey ry() { static InternedKey k = WSNameSet::INTERN("ry");            return k; }
    inline InternedKey width() { static InternedKey k = WSNameSet::INTERN("width");         return k; }
    inline InternedKey height() { static InternedKey k = WSNameSet::INTERN("height");        return k; }
    
    // Text positioning
    inline InternedKey dx() { static InternedKey k = WSNameSet::INTERN("dx"); return k; }
    inline InternedKey dy() { static InternedKey k = WSNameSet::INTERN("dy"); return k; }
    inline InternedKey rotate() { static InternedKey k = WSNameSet::INTERN("rotate"); return k; }

    // Paths & shapes
    inline InternedKey d() { static InternedKey k = WSNameSet::INTERN("d");             return k; }
    inline InternedKey points() { static InternedKey k = WSNameSet::INTERN("points");        return k; }

    // Viewport & aspect
    inline InternedKey viewBox() { static InternedKey k = WSNameSet::INTERN("viewBox");       return k; }
    inline InternedKey preserveAspectRatio()
    {
        static InternedKey k = WSNameSet::INTERN("preserveAspectRatio"); return k;
    }

    // Transforms
    inline InternedKey transform() { static InternedKey k = WSNameSet::INTERN("transform");     return k; }

    // Paint & stroke
    inline InternedKey color() { static InternedKey k = WSNameSet::INTERN("color");    return k; }
    inline InternedKey solid_color() { static InternedKey k = WSNameSet::INTERN("solid-color");    return k; }
    inline InternedKey solid_opacity() { static InternedKey k = WSNameSet::INTERN("solid-opacity");    return k; }

    inline InternedKey fill() { static InternedKey k = WSNameSet::INTERN("fill");          return k; }
    inline InternedKey fill_opacity() { static InternedKey k = WSNameSet::INTERN("fill-opacity");  return k; }
    inline InternedKey fill_rule() { static InternedKey k = WSNameSet::INTERN("fill-rule");     return k; }

    inline InternedKey stroke() { static InternedKey k = WSNameSet::INTERN("stroke");        return k; }
    inline InternedKey stroke_opacity() { static InternedKey k = WSNameSet::INTERN("stroke-opacity"); return k; }
    inline InternedKey stroke_width() { static InternedKey k = WSNameSet::INTERN("stroke-width");  return k; }
    inline InternedKey stroke_linecap() { static InternedKey k = WSNameSet::INTERN("stroke-linecap"); return k; }
    inline InternedKey stroke_linecap_start() { static InternedKey k = WSNameSet::INTERN("stroke-linecap-start"); return k; }
    inline InternedKey stroke_linecap_end() { static InternedKey k = WSNameSet::INTERN("stroke-linecap-end"); return k; }
    inline InternedKey stroke_linejoin(){static InternedKey k = WSNameSet::INTERN("stroke-linejoin"); return k;}
    inline InternedKey stroke_miterlimit(){static InternedKey k = WSNameSet::INTERN("stroke-miterlimit"); return k;}

    // Stroke dashing
    inline InternedKey stroke_dasharray(){static InternedKey k = WSNameSet::INTERN("stroke-dasharray"); return k;}
    inline InternedKey stroke_dashoffset(){static InternedKey k = WSNameSet::INTERN("stroke-dashoffset"); return k;}

    // flowRoot
    inline InternedKey line_height() { static InternedKey k = WSNameSet::INTERN("line-height"); return k; }

    // Text
    inline InternedKey font_family() { static InternedKey k = WSNameSet::INTERN("font-family");   return k; }
    inline InternedKey font_size() { static InternedKey k = WSNameSet::INTERN("font-size");     return k; }
    inline InternedKey font_weight() { static InternedKey k = WSNameSet::INTERN("font-weight");   return k; }
    inline InternedKey font_stretch() { static InternedKey k = WSNameSet::INTERN("font_stretch"); return k; }
    inline InternedKey font_style() { static InternedKey k = WSNameSet::INTERN("font-style");    return k; }
    inline InternedKey text_anchor() { static InternedKey k = WSNameSet::INTERN("text-anchor");   return k; }
    inline InternedKey text_align() { static InternedKey k = WSNameSet::INTERN("text-align");   return k; }
    inline InternedKey dominant_baseline()
    {
        static InternedKey k = WSNameSet::INTERN("dominant-baseline"); return k;
    }
    inline InternedKey alignment_baseline()
    {
        static InternedKey k = WSNameSet::INTERN("alignment-baseline"); return k;
    }

    // Linking / reuse
    inline InternedKey href() { static InternedKey k = WSNameSet::INTERN("href");          return k; }
    inline InternedKey xlink_href() { static InternedKey k = WSNameSet::INTERN("xlink:href");    return k; }

    // Clipping / masking
    inline InternedKey clip_path() { static InternedKey k = WSNameSet::INTERN("clip-path");     return k; }
    inline InternedKey clipPathUnits() { static InternedKey k = WSNameSet::INTERN("clipPathUnits");     return k; }

    // Masking
    INLINE InternedKey mask_units() { static InternedKey k = WSNameSet::INTERN("maskUnits");     return k; }
    INLINE InternedKey mask_content_units() { static InternedKey k = WSNameSet::INTERN("maskContentUnits");     return k; }
    INLINE InternedKey mask_type() { static InternedKey k = WSNameSet::INTERN("mask-type");     return k; }
    inline InternedKey mask() { static InternedKey k = WSNameSet::INTERN("mask");          return k; }

    // Filters / effects
    inline InternedKey filter() { static InternedKey k = WSNameSet::INTERN("filter");        return k; }

    // Gradients / patterns
    inline InternedKey stop_color() { static InternedKey k = WSNameSet::INTERN("stop-color");    return k; }
    inline InternedKey stop_opacity() { static InternedKey k = WSNameSet::INTERN("stop-opacity");  return k; }
    inline InternedKey offset() { static InternedKey k = WSNameSet::INTERN("offset");         return k; }

    inline InternedKey gradientUnits() { static InternedKey k = WSNameSet::INTERN("gradientUnits"); return k; }
    inline InternedKey gradientTransform(){static InternedKey k = WSNameSet::INTERN("gradientTransform"); return k;}
    inline InternedKey spreadMethod() { static InternedKey k = WSNameSet::INTERN("spreadMethod");  return k; }
    inline InternedKey extendMode() { static InternedKey k = WSNameSet::INTERN("extendMode");    return k; }
    inline InternedKey angle() { static InternedKey k = WSNameSet::INTERN("angle");  return k; }
    inline InternedKey repeat() { static InternedKey k = WSNameSet::INTERN("repeat");  return k; }
    inline InternedKey fx() { static InternedKey k = WSNameSet::INTERN("fx");  return k; }
    inline InternedKey fy() { static InternedKey k = WSNameSet::INTERN("fy");  return k; }
    inline InternedKey fr() { static InternedKey k = WSNameSet::INTERN("fr");  return k; }

    // Pattern attributes
    inline InternedKey patternUnits() { static InternedKey k = WSNameSet::INTERN("patternUnits");  return k; }
    inline InternedKey patternContentUnits() { static InternedKey k = WSNameSet::INTERN("patternContentUnits");  return k; }
    inline InternedKey patternTransform(){static InternedKey k = WSNameSet::INTERN("patternTransform"); return k;}

    // Markers
    inline InternedKey marker() { static InternedKey k = WSNameSet::INTERN("marker");  return k; }
    inline InternedKey marker_start() { static InternedKey k = WSNameSet::INTERN("marker-start");  return k; }
    inline InternedKey marker_mid() { static InternedKey k = WSNameSet::INTERN("marker-mid");    return k; }
    inline InternedKey marker_end() { static InternedKey k = WSNameSet::INTERN("marker-end");    return k; }
    inline InternedKey markerUnits() { static InternedKey k = WSNameSet::INTERN("markerUnits");  return k; }
    inline InternedKey markerWidth() { static InternedKey k = WSNameSet::INTERN("markerWidth");  return k; }
    inline InternedKey markerHeight() { static InternedKey k = WSNameSet::INTERN("markerHeight");  return k; }
    inline InternedKey refX() { static InternedKey k = WSNameSet::INTERN("refX");  return k; }
    inline InternedKey refY() { static InternedKey k = WSNameSet::INTERN("refY");  return k; }
    inline InternedKey orient() { static InternedKey k = WSNameSet::INTERN("orient");  return k; }

    // ------------------------------------------------------------
    // SVG2 / modern additions you may see
    // ------------------------------------------------------------
    inline InternedKey vector_effect() { static InternedKey k = WSNameSet::INTERN("vector-effect"); return k; }
    inline InternedKey paint_order() { static InternedKey k = WSNameSet::INTERN("paint-order");   return k; }
    inline InternedKey shape_rendering() { static InternedKey k = WSNameSet::INTERN("shape-rendering"); return k; }
    inline InternedKey text_rendering() { static InternedKey k = WSNameSet::INTERN("text-rendering"); return k; }
    inline InternedKey image_rendering() {static InternedKey k = WSNameSet::INTERN("image-rendering"); return k; }

    // ------------------------------------------------------------
    // Filters & effects
    // -------------------------------------------------------------
    inline InternedKey filterUnits() { static InternedKey k = WSNameSet::INTERN("filterUnits"); return k; }
    inline InternedKey primitiveUnits() { static InternedKey k = WSNameSet::INTERN("primitiveUnits"); return k; }


    

    // -------------------------------------------
    // Screen Capture
    // -------------------------------------------
    inline InternedKey src() { static InternedKey k = WSNameSet::INTERN("src"); return k; }
    inline InternedKey frame_rate() { static InternedKey k = WSNameSet::INTERN("frame-rate"); return k; }

    inline InternedKey cropX() { static InternedKey k = WSNameSet::INTERN("cropX"); return k; }
    inline InternedKey cropY() { static InternedKey k = WSNameSet::INTERN("cropY"); return k; }
    inline InternedKey cropWidth() { static InternedKey k = WSNameSet::INTERN("cropWidth"); return k; }
    inline InternedKey cropHeight() { static InternedKey k = WSNameSet::INTERN("cropHeight"); return k; }

    inline InternedKey capX() { static InternedKey k = WSNameSet::INTERN("capX"); return k; }
    inline InternedKey capY() { static InternedKey k = WSNameSet::INTERN("capY"); return k; }
    inline InternedKey capWidth() { static InternedKey k = WSNameSet::INTERN("capWidth"); return k; }
    inline InternedKey capHeight() { static InternedKey k = WSNameSet::INTERN("capHeight"); return k; }

    inline InternedKey displayUnits() { static InternedKey k = WSNameSet::INTERN("displayUnits"); return k; }

    // ------------------------------------------------------------
    // Event / interactivity attributes (still appear)
    // ------------------------------------------------------------
    inline InternedKey onclick() { static InternedKey k = WSNameSet::INTERN("onclick");      return k; }
    inline InternedKey onmouseover() { static InternedKey k = WSNameSet::INTERN("onmouseover");  return k; }
    inline InternedKey onmouseout() { static InternedKey k = WSNameSet::INTERN("onmouseout");   return k; }

}



namespace waavs::svgval
{
    // ============================================================
    // Actively used / modern SVG keyword values
    // ============================================================

    // Generic / global
    inline InternedKey none() { static InternedKey k = WSNameSet::INTERN("none");            return k; }
    inline InternedKey inherit() { static InternedKey k = WSNameSet::INTERN("inherit");         return k; }
    inline InternedKey initial() { static InternedKey k = WSNameSet::INTERN("initial");         return k; }
    inline InternedKey unset() { static InternedKey k = WSNameSet::INTERN("unset");           return k; }

    // Paint keywords
    inline InternedKey currentColor() { static InternedKey k = WSNameSet::INTERN("currentColor");    return k; }
    inline InternedKey context_fill() { static InternedKey k = WSNameSet::INTERN("context-fill");    return k; }
    inline InternedKey context_stroke() { static InternedKey k = WSNameSet::INTERN("context-stroke");  return k; }

    // Display / visibility
    inline InternedKey inline_() { static InternedKey k = WSNameSet::INTERN("inline");          return k; }
    inline InternedKey block() { static InternedKey k = WSNameSet::INTERN("block");           return k; }
    inline InternedKey visible() { static InternedKey k = WSNameSet::INTERN("visible");         return k; }
    inline InternedKey hidden() { static InternedKey k = WSNameSet::INTERN("hidden");          return k; }
    inline InternedKey collapse() { static InternedKey k = WSNameSet::INTERN("collapse");        return k; }

    // Fill / stroke rules
    inline InternedKey nonzero() { static InternedKey k = WSNameSet::INTERN("nonzero");         return k; }
    inline InternedKey evenodd() { static InternedKey k = WSNameSet::INTERN("evenodd");         return k; }

    // Stroke linecap
    inline InternedKey butt() { static InternedKey k = WSNameSet::INTERN("butt");            return k; }
    inline InternedKey round() { static InternedKey k = WSNameSet::INTERN("round");           return k; }
    inline InternedKey square() { static InternedKey k = WSNameSet::INTERN("square");          return k; }

    // Stroke linejoin
    inline InternedKey miter() { static InternedKey k = WSNameSet::INTERN("miter");           return k; }
    inline InternedKey bevel() { static InternedKey k = WSNameSet::INTERN("bevel");           return k; }

    // Marker placement
    inline InternedKey auto_() { static InternedKey k = WSNameSet::INTERN("auto");            return k; }

    // Units
    inline InternedKey userSpaceOnUse() { static InternedKey k = WSNameSet::INTERN("userSpaceOnUse");  return k; }
    inline InternedKey objectBoundingBox()
    {
        static InternedKey k = WSNameSet::INTERN("objectBoundingBox"); return k;
    }

    // PreserveAspectRatio
    inline InternedKey meet() { static InternedKey k = WSNameSet::INTERN("meet");            return k; }
    inline InternedKey slice() { static InternedKey k = WSNameSet::INTERN("slice");           return k; }

    inline InternedKey xMinYMin() { static InternedKey k = WSNameSet::INTERN("xMinYMin");        return k; }
    inline InternedKey xMidYMin() { static InternedKey k = WSNameSet::INTERN("xMidYMin");        return k; }
    inline InternedKey xMaxYMin() { static InternedKey k = WSNameSet::INTERN("xMaxYMin");        return k; }

    inline InternedKey xMinYMid() { static InternedKey k = WSNameSet::INTERN("xMinYMid");        return k; }
    inline InternedKey xMidYMid() { static InternedKey k = WSNameSet::INTERN("xMidYMid");        return k; }
    inline InternedKey xMaxYMid() { static InternedKey k = WSNameSet::INTERN("xMaxYMid");        return k; }

    inline InternedKey xMinYMax() { static InternedKey k = WSNameSet::INTERN("xMinYMax");        return k; }
    inline InternedKey xMidYMax() { static InternedKey k = WSNameSet::INTERN("xMidYMax");        return k; }
    inline InternedKey xMaxYMax() { static InternedKey k = WSNameSet::INTERN("xMaxYMax");        return k; }

    // Vector effects
    inline InternedKey non_scaling_stroke() { static InternedKey k = WSNameSet::INTERN("non-scaling-stroke"); return k;}




    // Text align
    inline InternedKey center() { static InternedKey k = WSNameSet::INTERN("center");         return k; }
    inline InternedKey right() { static InternedKey k = WSNameSet::INTERN("right");          return k; }
    inline InternedKey left() { static InternedKey k = WSNameSet::INTERN("left");           return k; }

    // Text anchor
    inline InternedKey start() { static InternedKey k = WSNameSet::INTERN("start");           return k; }
    inline InternedKey middle() { static InternedKey k = WSNameSet::INTERN("middle");          return k; }
    inline InternedKey end() { static InternedKey k = WSNameSet::INTERN("end");             return k; }

    // Font weight
    inline InternedKey normal() { static InternedKey k = WSNameSet::INTERN("normal");          return k; }
    inline InternedKey bold() { static InternedKey k = WSNameSet::INTERN("bold");            return k; }
    inline InternedKey bolder() { static InternedKey k = WSNameSet::INTERN("bolder");          return k; }
    inline InternedKey lighter() { static InternedKey k = WSNameSet::INTERN("lighter");         return k; }

    // Font style
    inline InternedKey italic() { static InternedKey k = WSNameSet::INTERN("italic");          return k; }
    inline InternedKey oblique() { static InternedKey k = WSNameSet::INTERN("oblique");         return k; }

    // Shape rendering
    inline InternedKey auto_rendering() { static InternedKey k = WSNameSet::INTERN("auto");             return k; }
    inline InternedKey optimizeSpeed() { static InternedKey k = WSNameSet::INTERN("optimizeSpeed");   return k; }
    inline InternedKey crispEdges() { static InternedKey k = WSNameSet::INTERN("crispEdges");      return k; }
    inline InternedKey geometricPrecision()
    {
        static InternedKey k = WSNameSet::INTERN("geometricPrecision"); return k;
    }

    // Image rendering
    inline InternedKey pixelated() { static InternedKey k = WSNameSet::INTERN("pixelated");       return k; }

    // Spread methods (gradients)
    inline InternedKey pad() { static InternedKey k = WSNameSet::INTERN("pad");             return k; }
    inline InternedKey reflect() { static InternedKey k = WSNameSet::INTERN("reflect");         return k; }
    inline InternedKey repeat() { static InternedKey k = WSNameSet::INTERN("repeat");          return k; }

    // Mask / clip
    inline InternedKey luminance() { static InternedKey k = WSNameSet::INTERN("luminance");       return k; }
    inline InternedKey alpha() { static InternedKey k = WSNameSet::INTERN("alpha");           return k; }

    // Pointer events (still emitted)
    inline InternedKey visiblePainted() { static InternedKey k = WSNameSet::INTERN("visiblePainted");  return k; }
    inline InternedKey visibleFill() { static InternedKey k = WSNameSet::INTERN("visibleFill");     return k; }
    inline InternedKey visibleStroke() { static InternedKey k = WSNameSet::INTERN("visibleStroke");   return k; }
    inline InternedKey visibleAll() { static InternedKey k = WSNameSet::INTERN("visible");         return k; }

    inline InternedKey painted() { static InternedKey k = WSNameSet::INTERN("painted");         return k; }
    inline InternedKey fill_kw() { static InternedKey k = WSNameSet::INTERN("fill");             return k; }
    inline InternedKey stroke_kw() { static InternedKey k = WSNameSet::INTERN("stroke");           return k; }
    inline InternedKey all() { static InternedKey k = WSNameSet::INTERN("all");              return k; }
    inline InternedKey none_events() { static InternedKey k = WSNameSet::INTERN("none");             return k; }

    // ------------------------------------------------------------
    // SVG2 additions commonly seen
    // ------------------------------------------------------------
    inline InternedKey context_value() { static InternedKey k = WSNameSet::INTERN("context-value");    return k; }
}





namespace waavs::svgval_legacy
{
    // ============================================================
    // Deprecated / legacy paint & text keywords
    // ============================================================

    inline InternedKey freeze() { static InternedKey k = WSNameSet::INTERN("freeze");          return k; }
    inline InternedKey remove() { static InternedKey k = WSNameSet::INTERN("remove");          return k; }

    // Text layout (SMIL / old text-flow)
    inline InternedKey spacing() { static InternedKey k = WSNameSet::INTERN("spacing");         return k; }
    inline InternedKey spacingAndGlyphs() { static InternedKey k = WSNameSet::INTERN("spacingAndGlyphs"); return k; }

    // Alignment baseline legacy values
    inline InternedKey auto_baseline() { static InternedKey k = WSNameSet::INTERN("auto");             return k; }
    inline InternedKey baseline() { static InternedKey k = WSNameSet::INTERN("baseline");        return k; }
    inline InternedKey before_edge() { static InternedKey k = WSNameSet::INTERN("before-edge");     return k; }
    inline InternedKey text_before_edge(){static InternedKey k = WSNameSet::INTERN("text-before-edge"); return k;}
    inline InternedKey middle_baseline() { static InternedKey k = WSNameSet::INTERN("middle");          return k; }
    inline InternedKey central() { static InternedKey k = WSNameSet::INTERN("central");         return k; }
    inline InternedKey after_edge() { static InternedKey k = WSNameSet::INTERN("after-edge");      return k; }
    inline InternedKey text_after_edge(){static InternedKey k = WSNameSet::INTERN("text-after-edge"); return k;}
    inline InternedKey ideographic_baseline(){static InternedKey k = WSNameSet::INTERN("ideographic");     return k;}
    inline InternedKey alphabetic_baseline(){static InternedKey k = WSNameSet::INTERN("alphabetic");      return k;}
    inline InternedKey hanging_baseline(){static InternedKey k = WSNameSet::INTERN("hanging");         return k;}
    inline InternedKey mathematical_baseline(){static InternedKey k = WSNameSet::INTERN("mathematical");    return k;}

    // Writing mode legacy
    inline InternedKey lr_tb() { static InternedKey k = WSNameSet::INTERN("lr-tb");            return k; }
    inline InternedKey rl_tb() { static InternedKey k = WSNameSet::INTERN("rl-tb");            return k; }
    inline InternedKey tb_rl() { static InternedKey k = WSNameSet::INTERN("tb-rl");            return k; }

    // Glyph orientation legacy
    inline InternedKey auto_glyph() { static InternedKey k = WSNameSet::INTERN("auto");             return k; }
    inline InternedKey deg0() { static InternedKey k = WSNameSet::INTERN("0deg");             return k; }
    inline InternedKey deg90() { static InternedKey k = WSNameSet::INTERN("90deg");            return k; }
    inline InternedKey deg180() { static InternedKey k = WSNameSet::INTERN("180deg");           return k; }
    inline InternedKey deg270() { static InternedKey k = WSNameSet::INTERN("270deg");           return k; }

    // Legacy filter units
    inline InternedKey userSpace() { static InternedKey k = WSNameSet::INTERN("userSpace");        return k; }
    inline InternedKey objectBBox() { static InternedKey k = WSNameSet::INTERN("objectBoundingBox"); return k; }

    // Legacy cursor keywords
    inline InternedKey crosshair() { static InternedKey k = WSNameSet::INTERN("crosshair");        return k; }
    inline InternedKey pointer() { static InternedKey k = WSNameSet::INTERN("pointer");          return k; }
    inline InternedKey move() { static InternedKey k = WSNameSet::INTERN("move");             return k; }
    inline InternedKey e_resize() { static InternedKey k = WSNameSet::INTERN("e-resize");         return k; }
    inline InternedKey ne_resize() { static InternedKey k = WSNameSet::INTERN("ne-resize");        return k; }
    inline InternedKey nw_resize() { static InternedKey k = WSNameSet::INTERN("nw-resize");        return k; }
    inline InternedKey n_resize() { static InternedKey k = WSNameSet::INTERN("n-resize");         return k; }
    inline InternedKey se_resize() { static InternedKey k = WSNameSet::INTERN("se-resize");        return k; }
    inline InternedKey sw_resize() { static InternedKey k = WSNameSet::INTERN("sw-resize");        return k; }
    inline InternedKey s_resize() { static InternedKey k = WSNameSet::INTERN("s-resize");         return k; }
    inline InternedKey w_resize() { static InternedKey k = WSNameSet::INTERN("w-resize");         return k; }
}






//============================================================
// SVG element tag names
//============================================================
namespace waavs::svgtag
{
    // Document / container / structure
    inline InternedKey tag_svg() { static InternedKey k = WSNameSet::INTERN("svg");          return k; }
    inline InternedKey tag_g() { static InternedKey k = WSNameSet::INTERN("g");            return k; }
    inline InternedKey tag_defs() { static InternedKey k = WSNameSet::INTERN("defs");         return k; }
    inline InternedKey tag_symbol() { static InternedKey k = WSNameSet::INTERN("symbol");       return k; }
    inline InternedKey tag_use() { static InternedKey k = WSNameSet::INTERN("use");          return k; }
    inline InternedKey tag_switch_() { static InternedKey k = WSNameSet::INTERN("switch");       return k; }
    inline InternedKey tag_view() { static InternedKey k = WSNameSet::INTERN("view");         return k; }

    // Linking / scripting
    inline InternedKey tag_a() { static InternedKey k = WSNameSet::INTERN("a");            return k; }
    inline InternedKey tag_script() { static InternedKey k = WSNameSet::INTERN("script");       return k; }

    // Descriptive / metadata
    inline InternedKey tag_title() { static InternedKey k = WSNameSet::INTERN("title");        return k; }
    inline InternedKey tag_desc() { static InternedKey k = WSNameSet::INTERN("desc");         return k; }
    inline InternedKey tag_metadata() { static InternedKey k = WSNameSet::INTERN("metadata");     return k; }
    inline InternedKey tag_style() { static InternedKey k = WSNameSet::INTERN("style");        return k; }

    // Basic shapes / graphics
    inline InternedKey tag_circle() { static InternedKey k = WSNameSet::INTERN("circle");       return k; }
    inline InternedKey tag_ellipse() { static InternedKey k = WSNameSet::INTERN("ellipse");      return k; }
    inline InternedKey tag_line() { static InternedKey k = WSNameSet::INTERN("line");         return k; }
    inline InternedKey tag_rect() { static InternedKey k = WSNameSet::INTERN("rect");         return k; }
    inline InternedKey tag_path() { static InternedKey k = WSNameSet::INTERN("path");         return k; }
    inline InternedKey tag_polygon() { static InternedKey k = WSNameSet::INTERN("polygon");      return k; }
    inline InternedKey tag_polyline() { static InternedKey k = WSNameSet::INTERN("polyline");     return k; }
    inline InternedKey tag_image() { static InternedKey k = WSNameSet::INTERN("image");        return k; }
    inline InternedKey tag_foreignObject() { static InternedKey k = WSNameSet::INTERN("foreignObject"); return k; }

    // Text
    inline InternedKey tag_text() { static InternedKey k = WSNameSet::INTERN("text");         return k; }
    inline InternedKey tag_tspan() { static InternedKey k = WSNameSet::INTERN("tspan");        return k; }
    inline InternedKey tag_textPath() { static InternedKey k = WSNameSet::INTERN("textPath");     return k; }

    // Clipping / masking
    inline InternedKey tag_clipPath() { static InternedKey k = WSNameSet::INTERN("clipPath");     return k; }
    inline InternedKey tag_mask() { static InternedKey k = WSNameSet::INTERN("mask");         return k; }

    // Gradients / paint servers
    inline InternedKey tag_linearGradient() { static InternedKey k = WSNameSet::INTERN("linearGradient"); return k; }
    inline InternedKey tag_radialGradient() { static InternedKey k = WSNameSet::INTERN("radialGradient"); return k; }
    inline InternedKey tag_stop() { static InternedKey k = WSNameSet::INTERN("stop");         return k; }
    inline InternedKey tag_pattern() { static InternedKey k = WSNameSet::INTERN("pattern");      return k; }
    inline InternedKey tag_marker() { static InternedKey k = WSNameSet::INTERN("marker");       return k; }
    inline InternedKey SVGSolidColorElement() { static InternedKey k = WSNameSet::INTERN("solidColor");       return k; }
    // Filters
    inline InternedKey tag_filter() { static InternedKey k = WSNameSet::INTERN("filter");       return k; }
    inline InternedKey tag_feBlend() { static InternedKey k = WSNameSet::INTERN("feBlend");      return k; }
    inline InternedKey tag_feColorMatrix() { static InternedKey k = WSNameSet::INTERN("feColorMatrix"); return k; }
    inline InternedKey tag_feComponentTransfer() { static InternedKey k = WSNameSet::INTERN("feComponentTransfer"); return k; }
    inline InternedKey tag_feComposite() { static InternedKey k = WSNameSet::INTERN("feComposite");  return k; }
    inline InternedKey tag_feConvolveMatrix() { static InternedKey k = WSNameSet::INTERN("feConvolveMatrix"); return k; }
    inline InternedKey tag_feDiffuseLighting() { static InternedKey k = WSNameSet::INTERN("feDiffuseLighting"); return k; }
    inline InternedKey tag_feDisplacementMap() { static InternedKey k = WSNameSet::INTERN("feDisplacementMap"); return k; }
    inline InternedKey tag_feDistantLight() { static InternedKey k = WSNameSet::INTERN("feDistantLight"); return k; }
    inline InternedKey tag_feDropShadow() { static InternedKey k = WSNameSet::INTERN("feDropShadow");  return k; }
    inline InternedKey tag_feFlood() { static InternedKey k = WSNameSet::INTERN("feFlood");      return k; }
    inline InternedKey tag_feFuncA() { static InternedKey k = WSNameSet::INTERN("feFuncA");      return k; }
    inline InternedKey tag_feFuncB() { static InternedKey k = WSNameSet::INTERN("feFuncB");      return k; }
    inline InternedKey tag_feFuncG() { static InternedKey k = WSNameSet::INTERN("feFuncG");      return k; }
    inline InternedKey tag_feFuncR() { static InternedKey k = WSNameSet::INTERN("feFuncR");      return k; }
    inline InternedKey tag_feGaussianBlur() { static InternedKey k = WSNameSet::INTERN("feGaussianBlur"); return k; }
    inline InternedKey tag_feImage() { static InternedKey k = WSNameSet::INTERN("feImage");      return k; }
    inline InternedKey tag_feMerge() { static InternedKey k = WSNameSet::INTERN("feMerge");      return k; }
    inline InternedKey tag_feMergeNode() { static InternedKey k = WSNameSet::INTERN("feMergeNode");  return k; }
    inline InternedKey tag_feMorphology() { static InternedKey k = WSNameSet::INTERN("feMorphology"); return k; }
    inline InternedKey tag_feOffset() { static InternedKey k = WSNameSet::INTERN("feOffset");     return k; }
    inline InternedKey tag_fePointLight() { static InternedKey k = WSNameSet::INTERN("fePointLight"); return k; }
    inline InternedKey tag_feSpecularLighting() { static InternedKey k = WSNameSet::INTERN("feSpecularLighting"); return k; }
    inline InternedKey tag_feSpotLight() { static InternedKey k = WSNameSet::INTERN("feSpotLight");  return k; }
    inline InternedKey tag_feTile() { static InternedKey k = WSNameSet::INTERN("feTile");       return k; }
    inline InternedKey tag_feTurbulence() { static InternedKey k = WSNameSet::INTERN("feTurbulence"); return k; }

    // FlowRoot
    inline InternedKey tag_flowRoot() { static InternedKey k = WSNameSet::INTERN("flowRoot");      return k; }
    inline InternedKey tag_flowRegion() { static InternedKey k = WSNameSet::INTERN("flowRegion");    return k; }
    inline InternedKey tag_flowRegionBreak() { static InternedKey k = WSNameSet::INTERN("flowRegionBreak"); return k; }
    inline InternedKey tag_flowSpan() { static InternedKey k = WSNameSet::INTERN("flowSpan");      return k; }
    inline InternedKey tag_flowLine() { static InternedKey k = WSNameSet::INTERN("flowLine");      return k; }
    inline InternedKey tag_flowPara() { static InternedKey k = WSNameSet::INTERN("flowPara");      return k; }

    // Animation (SMIL)
    inline InternedKey tag_animate() { static InternedKey k = WSNameSet::INTERN("animate");      return k; }
    inline InternedKey tag_animateMotion() { static InternedKey k = WSNameSet::INTERN("animateMotion"); return k; }
    inline InternedKey tag_animateTransform() { static InternedKey k = WSNameSet::INTERN("animateTransform"); return k; }
    inline InternedKey tag_set() { static InternedKey k = WSNameSet::INTERN("set");          return k; }
    inline InternedKey tag_mpath() { static InternedKey k = WSNameSet::INTERN("mpath");        return k; }

    // SVG2 “specials” that can appear (rare, but show up in the SVG2 element index)
    inline InternedKey tag_discard() { static InternedKey k = WSNameSet::INTERN("discard");      return k; }
    inline InternedKey tag_unknown() { static InternedKey k = WSNameSet::INTERN("unknown");      return k; }
}


namespace waavs::svgtag_legacy
{
    // ------------------------------------------------------------
    // SVG Fonts (deprecated/removed in SVG2; common in older assets)
    // ------------------------------------------------------------
    inline InternedKey tag_font() { static InternedKey k = WSNameSet::INTERN("font");              return k; }
    inline InternedKey tag_font_face() { static InternedKey k = WSNameSet::INTERN("font-face");         return k; }
    inline InternedKey tag_font_face_src() { static InternedKey k = WSNameSet::INTERN("font-face-src");     return k; }
    inline InternedKey tag_font_face_uri() { static InternedKey k = WSNameSet::INTERN("font-face-uri");     return k; }
    inline InternedKey tag_font_face_name() { static InternedKey k = WSNameSet::INTERN("font-face-name");    return k; }
    inline InternedKey tag_font_face_format() { static InternedKey k = WSNameSet::INTERN("font-face-format");  return k; }

    inline InternedKey tag_glyph() { static InternedKey k = WSNameSet::INTERN("glyph");             return k; }
    inline InternedKey tag_missing_glyph() { static InternedKey k = WSNameSet::INTERN("missing-glyph");     return k; }
    inline InternedKey tag_hkern() { static InternedKey k = WSNameSet::INTERN("hkern");             return k; }
    inline InternedKey tag_vkern() { static InternedKey k = WSNameSet::INTERN("vkern");             return k; }

    // Less common SVG-font support elements seen in some generators
    inline InternedKey tag_definition_src() { static InternedKey k = WSNameSet::INTERN("definition-src");    return k; }

    // ------------------------------------------------------------
    // Alternate glyph mechanism (deprecated/removed)
    // ------------------------------------------------------------
    inline InternedKey tag_altGlyph() { static InternedKey k = WSNameSet::INTERN("altGlyph");          return k; }
    inline InternedKey tag_altGlyphDef() { static InternedKey k = WSNameSet::INTERN("altGlyphDef");       return k; }
    inline InternedKey tag_altGlyphItem() { static InternedKey k = WSNameSet::INTERN("altGlyphItem");      return k; }
    inline InternedKey tag_glyphRef() { static InternedKey k = WSNameSet::INTERN("glyphRef");          return k; }

    // ------------------------------------------------------------
    // Legacy text reference element (deprecated/removed)
    // ------------------------------------------------------------
    inline InternedKey tag_tref() { static InternedKey k = WSNameSet::INTERN("tref");              return k; }

    // ------------------------------------------------------------
    // Deprecated/removed metadata-ish elements
    // ------------------------------------------------------------
    inline InternedKey tag_color_profile() { static InternedKey k = WSNameSet::INTERN("color-profile");     return k; }
    inline InternedKey tag_cursor() { static InternedKey k = WSNameSet::INTERN("cursor");            return k; }

    // ------------------------------------------------------------
    // Rare legacy scripting/event element (SVG Tiny / older content)
    // ------------------------------------------------------------
    inline InternedKey tag_handler() { static InternedKey k = WSNameSet::INTERN("handler");           return k; }

    // ------------------------------------------------------------
    // SMIL-related deprecated animation variants you might see
    // (you already have animate/animateMotion/animateTransform/set/mpath
    //  in your modern list; this covers older extras)
    // ------------------------------------------------------------
    inline InternedKey tag_animateColor() { static InternedKey k = WSNameSet::INTERN("animateColor");      return k; }
}








namespace waavs::svgattr_legacy
{
    // ============================================================
    // SVG Fonts (deprecated / removed)
    // ============================================================
    inline InternedKey horiz_adv_x() { static InternedKey k = WSNameSet::INTERN("horiz-adv-x");      return k; }
    inline InternedKey horiz_origin_x() { static InternedKey k = WSNameSet::INTERN("horiz-origin-x");   return k; }
    inline InternedKey horiz_origin_y() { static InternedKey k = WSNameSet::INTERN("horiz-origin-y");   return k; }
    inline InternedKey vert_adv_y() { static InternedKey k = WSNameSet::INTERN("vert-adv-y");       return k; }
    inline InternedKey vert_origin_x() { static InternedKey k = WSNameSet::INTERN("vert-origin-x");    return k; }
    inline InternedKey vert_origin_y() { static InternedKey k = WSNameSet::INTERN("vert-origin-y");    return k; }

    inline InternedKey unicode() { static InternedKey k = WSNameSet::INTERN("unicode");          return k; }
    inline InternedKey glyph_name() { static InternedKey k = WSNameSet::INTERN("glyph-name");       return k; }
    inline InternedKey arabic_form() { static InternedKey k = WSNameSet::INTERN("arabic-form");      return k; }
    inline InternedKey lang() { static InternedKey k = WSNameSet::INTERN("lang");             return k; }
    inline InternedKey orientation() { static InternedKey k = WSNameSet::INTERN("orientation");      return k; }

    inline InternedKey panose_1() { static InternedKey k = WSNameSet::INTERN("panose-1");          return k; }
    inline InternedKey units_per_em() { static InternedKey k = WSNameSet::INTERN("units-per-em");      return k; }
    inline InternedKey ascent() { static InternedKey k = WSNameSet::INTERN("ascent");            return k; }
    inline InternedKey descent() { static InternedKey k = WSNameSet::INTERN("descent");           return k; }
    inline InternedKey alphabetic() { static InternedKey k = WSNameSet::INTERN("alphabetic");        return k; }
    inline InternedKey mathematical() { static InternedKey k = WSNameSet::INTERN("mathematical");      return k; }
    inline InternedKey ideographic() { static InternedKey k = WSNameSet::INTERN("ideographic");       return k; }
    inline InternedKey hanging() { static InternedKey k = WSNameSet::INTERN("hanging");           return k; }
    inline InternedKey v_ideographic() { static InternedKey k = WSNameSet::INTERN("v-ideographic");     return k; }

    inline InternedKey underline_position() { static InternedKey k = WSNameSet::INTERN("underline-position"); return k; }
    inline InternedKey underline_thickness()
    {
        static InternedKey k = WSNameSet::INTERN("underline-thickness"); return k;
    }
    inline InternedKey strikethrough_position()
    {
        static InternedKey k = WSNameSet::INTERN("strikethrough-position"); return k;
    }
    inline InternedKey strikethrough_thickness()
    {
        static InternedKey k = WSNameSet::INTERN("strikethrough-thickness"); return k;
    }

    // ------------------------------------------------------------
    // Alternate glyph system (deprecated / removed)
    // ------------------------------------------------------------
    inline InternedKey glyph_ref() { static InternedKey k = WSNameSet::INTERN("glyphRef");         return k; }
    inline InternedKey alt_glyph() { static InternedKey k = WSNameSet::INTERN("altGlyph");         return k; }
    inline InternedKey alt_glyph_def() { static InternedKey k = WSNameSet::INTERN("altGlyphDef");      return k; }
    inline InternedKey alt_glyph_item() { static InternedKey k = WSNameSet::INTERN("altGlyphItem");     return k; }

    // ------------------------------------------------------------
    // Deprecated text / linking
    // ------------------------------------------------------------
    inline InternedKey tref() { static InternedKey k = WSNameSet::INTERN("tref");              return k; }

    // ------------------------------------------------------------
    // Color profile / cursor (deprecated)
    // ------------------------------------------------------------
    inline InternedKey color_profile() { static InternedKey k = WSNameSet::INTERN("color-profile");     return k; }
    inline InternedKey cursor() { static InternedKey k = WSNameSet::INTERN("cursor");            return k; }

    // ------------------------------------------------------------
    // Rare / obsolete scripting / metadata
    // ------------------------------------------------------------
    inline InternedKey baseProfile() { static InternedKey k = WSNameSet::INTERN("baseProfile");       return k; }
    inline InternedKey version() { static InternedKey k = WSNameSet::INTERN("version");           return k; }

    // ------------------------------------------------------------
    // Deprecated SMIL animation variants
    // ------------------------------------------------------------
    inline InternedKey animateColor() { static InternedKey k = WSNameSet::INTERN("animateColor");      return k; }
}

namespace waavs::svgfunc
{
    // svg transform functions
    static INLINE InternedKey scale() { return WSNameSet::INTERN("scale"); }
    static INLINE InternedKey rotate() { return WSNameSet::INTERN("rotate"); }
    static INLINE InternedKey translate() { return WSNameSet::INTERN("translate"); }
    static INLINE InternedKey skewX() { return WSNameSet::INTERN("skewX"); }
    static INLINE InternedKey skewY() { return WSNameSet::INTERN("skewY"); }
    static INLINE InternedKey matrix() { return WSNameSet::INTERN("matrix"); }

    // svgcolor functions
    static INLINE InternedKey rgb() { return WSNameSet::INTERN("rgb"); }
    static INLINE InternedKey rgba() { return WSNameSet::INTERN("rgba"); }
    static INLINE InternedKey hsl() { return WSNameSet::INTERN("hsl"); }
    static INLINE InternedKey hsla() { return WSNameSet::INTERN("hsla"); }
    static INLINE InternedKey url() { return WSNameSet::INTERN("url"); }

}