#pragma once

#include "gcdl_program.h"
#include "gcdl_program_normalized.h"

namespace waavs
{
    static bool gcdl_flatten_node(
        const GCDLNormalizedNode& src,
        GCDLPrimitiveNode& dst) noexcept
    {
        dst = {};

        if (!src.id || !src.op)
            return false;

        dst.id = src.id;
        dst.op = src.op;
        dst.refs = src.refs;
        dst.nums = src.nums;
        dst.meta = src.meta;

        return true;
    }

    static bool gcdl_flatten_program(
        const GCDLNormalizedProgram& src,
        GCDLProgram& dst) noexcept
    {
        dst.clear();

        for (const auto& p : src.params) {
            if (!dst.addParam(p.id, p.value))
                return false;
        }

        for (const auto& n : src.nodes) {
            GCDLPrimitiveNode outNode;

            if (!gcdl_flatten_node(n, outNode))
                return false;

            if (!dst.addNode(outNode))
                return false;
        }

        for (const auto& o : src.outputs) {
            if (!dst.addOutput(o.id, o.ref, o.designRole, o.fabricationRole))
                return false;
        }

        return true;
    }
}
