#pragma once

#include "gcdl_program.h"


namespace waavs
{
    struct GCDLNormalizedNode
    {
        InternedKey id = nullptr;
        InternedKey op = nullptr;

        std::vector<GeoRef> refs;
        std::vector<double> nums;

        GCDLNodeMeta meta{};
    };

    struct GCDLNormalizedProgram
    {
        std::vector<GCDLParam> params;
        std::vector<GCDLNormalizedNode> nodes;
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

        bool addNode(const GCDLNormalizedNode& node)
        {
            if (!node.id || !node.op)
                return false;

            nodes.push_back(node);
            return true;
        }

        bool addOutput(
            InternedKey id,
            InternedKey ref,
            GCDLDesignRole designRole,
            GCDLFabricationRole fabRole)
        {
            if (!id || !ref)
                return false;

            outputs.push_back({ id, ref, designRole, fabRole });
            return true;
        }
    };

}

