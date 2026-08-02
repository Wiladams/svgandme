#pragma once

#include "core_nametable.h"
#include "lang_span.h"


namespace waavs::svgunits
{
    // ============================================================
// Absolute length units
// Syntactic Numeric modifiers
// ============================================================
    inline InternedKey none() { static InternedKey k = WSNameSet::INTERN("none"); return k; }
    inline InternedKey px() { static InternedKey k = WSNameSet::INTERN("px");  return k; }  // CSS px (SVG default)
    inline InternedKey cm() { static InternedKey k = WSNameSet::INTERN("cm");  return k; }
    inline InternedKey mm() { static InternedKey k = WSNameSet::INTERN("mm");  return k; }
    inline InternedKey in_() { static InternedKey k = WSNameSet::INTERN("in");  return k; }
    inline InternedKey pt() { static InternedKey k = WSNameSet::INTERN("pt");  return k; }
    inline InternedKey pc() { static InternedKey k = WSNameSet::INTERN("pc");  return k; }

    // ============================================================
    // Relative length units
    // ============================================================

    inline InternedKey em() { static InternedKey k = WSNameSet::INTERN("em");  return k; }
    inline InternedKey ex() { static InternedKey k = WSNameSet::INTERN("ex");  return k; }
    inline InternedKey ch() { static InternedKey k = WSNameSet::INTERN("ch");  return k; }
    inline InternedKey rem() { static InternedKey k = WSNameSet::INTERN("rem"); return k; }

    // ============================================================
    // Viewport / container relative
    // ============================================================

    inline InternedKey vw() { static InternedKey k = WSNameSet::INTERN("vw");  return k; }
    inline InternedKey vh() { static InternedKey k = WSNameSet::INTERN("vh");  return k; }
    inline InternedKey vmin() { static InternedKey k = WSNameSet::INTERN("vmin"); return k; }
    inline InternedKey vmax() { static InternedKey k = WSNameSet::INTERN("vmax"); return k; }

    // ============================================================
    // Percent
    // ============================================================

    inline InternedKey pct() { static InternedKey k = WSNameSet::INTERN("%"); return k; }

    // ============================================================
    // Angles (for gradients, transforms, etc.)
    // ============================================================

    inline InternedKey deg() { static InternedKey k = WSNameSet::INTERN("deg"); return k; }
    inline InternedKey rad() { static InternedKey k = WSNameSet::INTERN("rad"); return k; }
    inline InternedKey grad() { static InternedKey k = WSNameSet::INTERN("grad"); return k; }
    inline InternedKey turn() { static InternedKey k = WSNameSet::INTERN("turn"); return k; }

    // ============================================================
    // Time units (animations)
    // ============================================================

    inline InternedKey s() { static InternedKey k = WSNameSet::INTERN("s");  return k; }
    inline InternedKey ms() { static InternedKey k = WSNameSet::INTERN("ms"); return k; }

    // ============================================================
    // Frequency (filters / audio-ish SVG extensions)
    // ============================================================

    inline InternedKey hz() { static InternedKey k = WSNameSet::INTERN("Hz");  return k; }
    inline InternedKey khz() { static InternedKey k = WSNameSet::INTERN("kHz"); return k; }

    // ============================================================
    // Resolution (filters, CSS images)
    // ============================================================

    inline InternedKey dpi() { static InternedKey k = WSNameSet::INTERN("dpi");  return k; }
    inline InternedKey dpcm() { static InternedKey k = WSNameSet::INTERN("dpcm"); return k; }
    inline InternedKey dppx() { static InternedKey k = WSNameSet::INTERN("dppx"); return k; }

    // ============================================================
    // Flex / grid (SVG2 / CSS compatibility)
    // ============================================================

    inline InternedKey fr() { static InternedKey k = WSNameSet::INTERN("fr"); return k; }


    // ============================================================
    // Unit Helpers
    // ============================================================
    inline InternedKey internUnit(const ByteSpan& s)
    {
        return s ? WSNameSet::INTERN(s) : InternedKey{};
    }

    // Is this a length unit?
    inline bool isLengthUnit(InternedKey u) noexcept
    {
        return (u == px() ||
                u == cm() ||
                u == mm() ||
                u == in_() ||
                u == pt() ||
                u == pc() ||
                u == em() ||
                u == ex() ||
                u == ch() ||
                u == rem() ||
                u == vw() ||
                u == vh() ||
                u == vmin() ||
                u == vmax() ||
                u == pct());
    }

    // Is this an angle unit?
    inline bool isAngleUnit(InternedKey u) noexcept
    {
        return (u == deg() ||
                u == rad() ||
                u == grad() ||
                u == turn());
    }

    // Is this a time unit?
    inline bool isTimeUnit(InternedKey u) noexcept
    {
        return (u == s() ||
            u == ms());
    }

    // Is this a resolution unit?
    inline bool isResolutionUnit(InternedKey u) noexcept
    {
        return (u == dpi() ||
            u == dpcm() ||
            u == dppx());
    }


}
