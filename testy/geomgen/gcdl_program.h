// gcdl_program.h - program container for GCDL
#pragma once

#include "gcdl_types.h"

#include <vector>

namespace waavs
{
    enum struct GCDLOutputRole {
        Construction,
        Finished,
        Dimension,
        Label,
        Debug,
    };

    struct GCDLParam {
        InternedKey id = nullptr;
        double value = 0.0;
    };

    struct GCDLOutput {
        InternedKey id = nullptr;
        InternedKey ref = nullptr;
        GCDLOutputRole role = GCDLOutputRole::Finished;
    };

    // A 'program' is a recipe for constructing a geometric figure. It consists of:
    struct GCDLProgram {
        std::vector<GCDLParam> params;
        std::vector<GCDLNode> nodes;
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

        bool addNode(const GCDLNode& node)
        {
            if (!node.id || !node.op)
                return false;

            nodes.push_back(node);
            return true;
        }

        bool addOutput(InternedKey id, InternedKey ref, GCDLOutputRole role)
        {
            if (!id || !ref)
                return false;

            outputs.push_back({ id, ref, role });
            return true;
        }
    };
}
