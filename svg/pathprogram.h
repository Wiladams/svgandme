#pragma once

#include "definitions.h"
#include <vector>

#include "wggeometry.h"

// Machinery for a PathProgram
// That represents a sequence of path segment operations
namespace waavs
{
    // These ops represent normalized commands.  That means...
    //  - no relative commands exist
    //  - no implicit lineto after moveto
    //  - arcs are already endpoint-form
    //  - smooth curves are expanded

    enum PathOp : uint8_t {
        OP_END = 0,		// Reprsesent the end of the path program (not Z / close)
        OP_MOVETO,
        OP_LINETO,
        OP_CUBICTO,
        OP_QUADTO,
        OP_ARCTO,
        OP_CLOSE
    };

    // Arity table, how many arguments each op takes
    static constexpr uint8_t kPathOpArity[] = {
        0,  // OP_END
        2,  // OP_MOVETO:   x y
        2,  // OP_LINETO:   x y
        6,  // OP_CUBICTO:  x1 y1 x2 y2 x y
        4,  // OP_QUADTO:   x1 y1 x y
        7,  // OP_ARCTO:    rx ry x-axis-rotation large-arc-flag sweep-flag x y
        0   // OP_CLOSE
    };
    
    // Ensure the ops size and arity table size match
    static_assert(OP_CLOSE + 1 == std::size(kPathOpArity), "PathOp arity table size mismatch");


    // The container for a path program
    //	canonical
    //  comparable
    //  cacheable
    // 
    struct PathProgram
    {
        std::vector<uint8_t> ops;
        std::vector<float> args;

        void clear() noexcept
        {
            ops.clear();
            args.clear();
        }
    };
}


//
// Don't want this file to fill up with utilities, but
// since the bounds calculation has no other dependencies
// it's safe enough to put this here for now.
//
namespace waavs
{
    // getBoundingBox
    // 
    // Get the bounding rect of a path program.  
    // Returns false if the program is empty or invalid.
    static INLINE bool pathprogram_get_bounds(const PathProgram& prog,
        double& x, double& y,
        double& width, double& height)
    {
        if (prog.ops.empty())
            return false;

        double minX = 0, minY = 0, maxX = 0, maxY = 0;
        bool   hasBBox = false;

        double cx = 0, cy = 0;
        double sx = 0, sy = 0;
        //bool   hasPoint = false;

        size_t ip = 0;
        size_t ap = 0;

        while (ip < prog.ops.size()) {
            uint8_t op = prog.ops[ip++];
            if (op == OP_END)
                break;

            const float* a = prog.args.data() + ap;
            ap += kPathOpArity[op];

            auto include = [&](double px, double py) {
                if (!hasBBox) {
                    bboxInit(minX, minY, maxX, maxY, px, py);
                    hasBBox = true;
                }
                else {
                    bboxExpand(minX, minY, maxX, maxY, px, py);
                }
                };

            switch (op) {
            case OP_MOVETO:
                cx = a[0]; cy = a[1];
                sx = cx; sy = cy;
                //hasPoint = true;
                include(cx, cy);
                break;

            case OP_LINETO:
                cx = a[0]; cy = a[1];
                include(cx, cy);
                break;

            case OP_QUADTO:
                quadBounds(cx, cy,
                    a[0], a[1],
                    a[2], a[3],
                    minX, minY, maxX, maxY);
                cx = a[2]; cy = a[3];
                break;

            case OP_CUBICTO:
                cubicBounds(cx, cy,
                    a[0], a[1],
                    a[2], a[3],
                    a[4], a[5],
                    minX, minY, maxX, maxY);
                cx = a[4]; cy = a[5];
                break;

            case OP_CLOSE:
                cx = sx; cy = sy;
                include(cx, cy);
                break;

            case OP_ARCTO:
                // For now: include endpoints (safe fallback)
                // You can replace this with full arc math next.
                cx = a[5]; cy = a[6];
                include(cx, cy);
                break;
            }
        }

        if (!hasBBox)
            return false;

        x = minX;
        y = minY;
        width = maxX - minX;
        height = maxY - minY;
        return true;
    }
}

namespace waavs
{
    // Only accepts a normalized, flattened PathProgram.
    // Expected ops:
    //   OP_MOVETO
    //   OP_LINETO
    //   OP_CLOSE
    //   OP_END
    //
    // Curves and arcs should be flattened before calling this.
    static INLINE bool pathprogram_get_flat_length(
        const PathProgram& prog,
        double& outLength) noexcept
    {
        outLength = 0.0;

        if (prog.ops.empty())
            return false;

        double cx = 0.0;
        double cy = 0.0;
        double sx = 0.0;
        double sy = 0.0;

        bool hasCurrentPoint = false;
        bool hasLength = false;

        size_t ip = 0;
        size_t ap = 0;

        while (ip < prog.ops.size())
        {
            const uint8_t op = prog.ops[ip++];

            if (op == OP_END)
                break;

            if (op > OP_CLOSE)
                return false;

            const uint8_t arity = kPathOpArity[op];

            if (ap + arity > prog.args.size())
                return false;

            const float* a = prog.args.data() + ap;
            ap += arity;

            switch (op)
            {
            case OP_MOVETO:
                cx = a[0];
                cy = a[1];
                sx = cx;
                sy = cy;
                hasCurrentPoint = true;
                break;

            case OP_LINETO:
            {
                if (!hasCurrentPoint)
                    return false;

                const double x = a[0];
                const double y = a[1];

                const double dx = x - cx;
                const double dy = y - cy;

                outLength += std::sqrt(dx * dx + dy * dy);

                cx = x;
                cy = y;
                hasLength = true;
            } break;

            case OP_CLOSE:
            {
                if (!hasCurrentPoint)
                    return false;

                const double dx = sx - cx;
                const double dy = sy - cy;

                outLength += std::sqrt(dx * dx + dy * dy);

                cx = sx;
                cy = sy;
                hasLength = true;
            } break;

            case OP_QUADTO:
            case OP_CUBICTO:
            case OP_ARCTO:
                // This routine intentionally only accepts flattened paths.
                return false;

            default:
                return false;
            }
        }

        return hasLength;
    }
}