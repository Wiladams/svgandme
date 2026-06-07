// gcdl_program.h - program container for GCDL
#pragma once

#include "gcdl_types.h"

#include <vector>

namespace waavs
{


    // A 'program' is a recipe for constructing a geometric figure. It consists of:
    struct GCDLProgram {
        std::vector<GCDLParam> params;
        std::vector<GCDLPrimitiveNode> nodes;
        std::vector<GCDLOutput> outputs;

        void clear()
        {
            params.clear();
            nodes.clear();
            outputs.clear();
        }

        bool addParam(InternedKey id, double value)
        {
            if (!id)
                return false;

            params.push_back({ id, value });
            return true;
        }

        bool addNode(const GCDLPrimitiveNode& node)
        {
            if (!node.id || !node.op)
                return false;

            nodes.push_back(node);
            return true;
        }

        bool addOutput(InternedKey id, InternedKey ref, GCDLDesignRole designRole, GCDLFabricationRole fabRole)
        {
            if (!id || !ref)
                return false;

            outputs.push_back({ id, ref, designRole, fabRole });
            return true;
        }
    };
}
