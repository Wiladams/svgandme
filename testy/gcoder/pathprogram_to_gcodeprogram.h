#pragma once

#include "pathprogram.h"
#include "pathprogram_flattener.h"
#include "toolpathpolicy.h"
#include "gcodeprogram_builder.h"

namespace waavs
{
    template <typename PolicyT>
    static INLINE bool pathprogram_to_gcodeprogram_flattened(
        const PathProgram& prog,
        PolicyT& policy) noexcept
    {
        size_t ip = 0;
        size_t ap = 0;

        bool inSubpath = false;

        if (!policy.beginProgram())
            return false;

        while (ip < prog.ops.size()) {
            const uint8_t op = prog.ops[ip++];

            if (op > OP_CLOSE)
                return false;

            if (op == OP_END)
                break;

            const size_t arity = kPathOpArity[op];
            if (ap + arity > prog.args.size())
                return false;

            const float* a = prog.args.data() + ap;
            ap += arity;

            switch ((PathOp)op) {
            case OP_MOVETO:
                if (inSubpath) {
                    if (!policy.endPath())
                        return false;
                }

                if (!policy.beginPath())
                    return false;

                if (!policy.moveTo((double)a[0], (double)a[1]))
                    return false;

                inSubpath = true;
                break;

            case OP_LINETO:
                if (!inSubpath)
                    return false;

                if (!policy.lineTo((double)a[0], (double)a[1]))
                    return false;
                break;

            case OP_CLOSE:
                if (!inSubpath)
                    return false;

                if (!policy.closePath())
                    return false;

                if (!policy.endPath())
                    return false;

                inSubpath = false;
                break;

            case OP_QUADTO:
            case OP_CUBICTO:
            case OP_ARCTO:
                // This function expects already-flattened input.
                return false;

            case OP_END:
            default:
                return false;
            }
        }

        if (inSubpath) {
            if (!policy.endPath())
                return false;
        }

        if (!policy.endProgram())
            return false;

        return true;
    }


    template <typename PolicyT>
    static INLINE bool pathprogram_to_gcodeprogram(
        const PathProgram& src,
        PolicyT& policy,
        double flatness = 0.25,
        int maxDepth = 16) noexcept
    {
        PathProgram flattened;
        if (!flattenPathProgram(src, flattened, flatness, maxDepth))
            return false;

        return pathprogram_to_gcodeprogram_flattened(flattened, policy);
    }



}

