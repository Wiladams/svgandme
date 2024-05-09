#pragma once

#include <cstdint>

namespace waavs {

    // SVG path commands
    // M - move       (M, m)
    // L - line       (L, l, H, h, V, v)
    // C - cubic      (C, c, S, s)
    // Q - quad       (Q, q, T, t)
    // A - ellipticArc  (A, a)
    // Z - close        (Z, z)
    enum class SegmentCommand : uint8_t
    {
        INVALID = 0
        , MoveTo = 'M'
        , MoveBy = 'm'
        , LineTo = 'L'
        , LineBy = 'l'
        , HLineTo = 'H'
        , HLineBy = 'h'
        , VLineTo = 'V'
        , VLineBy = 'v'
        , CubicTo = 'C'
        , CubicBy = 'c'
        , SCubicTo = 'S'
        , SCubicBy = 's'
        , QuadTo = 'Q'
        , QuadBy = 'q'
        , SQuadTo = 'T'
        , SQuadBy = 't'
        , ArcTo = 'A'
        , ArcBy = 'a'
        , CloseTo = 'Z'
        , CloseBy = 'z'
    };
}
