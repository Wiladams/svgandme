// gcdl_program.h - program container for GCDL
#pragma once

#include "gcdl_types.h"

#include <vector>

namespace waavs
{
    enum struct GCDLDesignRole
    {
        None,
        Construction,
        Finished,
        VoidBoundary,
        SolidBoundary,
        Decoration,
        Dimension,
        Label,
        Debug,
    };

    enum struct GCDLFabricationRole 
    {
        None,
        Cut,
        Score,
        Engrave,
        Drill,
        Pocket
    };

    struct GCDLNodeMeta
    {
        InternedKey sourceStep = nullptr;
        InternedKey sourceMotif = nullptr;
        InternedKey sourceNode = nullptr;

        GCDLDesignRole designRole = GCDLDesignRole::None;
        GCDLFabricationRole fabricationRole = GCDLFabricationRole::None;
    };

    struct GCDLParam {
        InternedKey id = nullptr;
        double value = 0.0;
    };

    struct GCDLOutput {
        InternedKey id = nullptr;
        InternedKey ref = nullptr;

        GCDLDesignRole designRole = GCDLDesignRole::Finished;
        GCDLFabricationRole fabRole = GCDLFabricationRole::None;
    };

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
