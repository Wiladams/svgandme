// gcdl_resolver.h - validation and lookup support for GCDL
#pragma once

#include "gcdl_program.h"

#include <unordered_set>

namespace waavs
{
    struct GCDLResolveResult {
        bool ok = true;
        const GCDLNode* failedNode = nullptr;
        InternedKey missingRef = nullptr;
        InternedKey duplicateId = nullptr;
    };

    static inline bool gcdl_has_id(
        const std::unordered_set<InternedKey>& ids,
        InternedKey id)
    {
        return id && ids.find(id) != ids.end();
    }

    static inline GCDLResolveResult resolveGCDLProgram(const GCDLProgram& prog)
    {
        GCDLResolveResult result;

        std::unordered_set<InternedKey> ids;

        for (size_t i = 0; i < prog.params.size(); i++) {
            InternedKey id = prog.params[i].id;
            if (!id)
                continue;

            if (ids.find(id) != ids.end()) {
                result.ok = false;
                result.duplicateId = id;
                return result;
            }

            ids.insert(id);
        }

        for (size_t i = 0; i < prog.nodes.size(); i++) {
            const GCDLNode& node = prog.nodes[i];

            if (!node.id || !node.op) {
                result.ok = false;
                result.failedNode = &node;
                return result;
            }

            if (ids.find(node.id) != ids.end()) {
                result.ok = false;
                result.failedNode = &node;
                result.duplicateId = node.id;
                return result;
            }

            for (size_t r = 0; r < node.refs.size(); r++) {
                const GeoRef& ref = node.refs[r];

                if (!gcdl_has_id(ids, ref.id)) {
                    result.ok = false;
                    result.failedNode = &node;
                    result.missingRef = ref.id;
                    return result;
                }
            }

            ids.insert(node.id);
        }

        for (size_t i = 0; i < prog.outputs.size(); i++) {
            const GCDLOutput& out = prog.outputs[i];

            if (!gcdl_has_id(ids, out.ref)) {
                result.ok = false;
                result.missingRef = out.ref;
                return result;
            }
        }

        return result;
    }
}

