// gcdl_eval.h
#pragma once

#include "gcdl_eval_ops.h"
#include <unordered_map>

namespace waavs
{
    // Generic signature for node evaluation functions.
    using GCDLEvalFunc = bool (*)(const GCDLPrimitiveNode&, GCDLEvalContext&);

    // dictionary that maps node op keys to their evaluation functions
    using GCDLEvalMap =
        std::unordered_map<InternedKey, GCDLEvalFunc,
        InternedKeyHash, InternedKeyEquivalent>;

    static const GCDLEvalMap& gcdlEvalMap()
    {
        static const GCDLEvalMap map = {
            { GCDLOp_arc_circle_from_to(), eval_arc_circle_from_to },
            { GCDLOp_arc_center_angles(), eval_arc_center_angles },
            //{ GCDLOp_arc_rotate(), eval_arc_rotate },

            { GCDLOp_point_xy(), eval_point_xy },
            { GCDLOp_point_polar(), eval_point_polar },
            { GCDLOp_point_pick(), eval_point_pick },
            { GCDLOp_point_along_line(), eval_point_along_line },
            { GCDLOp_point_project(), eval_point_project },
            //{ GCDLOp_point_midpoint(), eval_point_midpoint },
            { GCDLOp_point_rotate(), eval_point_rotate },



            { GCDLOp_circle_center_point(), eval_circle_center_point },
            { GCDLOp_circle_center_radius(), eval_circle_center_radius},

            { GCDLOp_intersect_circle_circle(), eval_intersect_circle_circle },
            { GCDLOp_intersect_line_line(), eval_intersect_line_line },
            { GCDLOp_intersect_line_circle(), eval_intersect_line_circle},

            { GCDLOp_line_from_point_angle(), eval_line_from_point_angle },
            { GCDLOp_line_from_points(), eval_line_from_points },
            { GCDLOp_line_divide(), eval_divide_line },
            { GCDLOp_line_parallel(), eval_line_parallel },
            { GCDLOp_line_perpendicular(), eval_line_perpendicular },
            { GCDLOp_line_rotate(), eval_line_rotate },

            { GCDLOp_mirror_point(), eval_mirror_point },
            { GCDLOp_mirror_arc(), eval_mirror_arc },
            
            { GCDLOp_path_from_points(), eval_path_from_points },
            { GCDLOp_path_from_refs(), eval_path_from_refs },

            { GCDLOp_polygon_regular(), eval_polygon_regular },
            { GCDLOp_polygon_star(), eval_polygon_star },

        };

        return map;
    }

    static inline bool evalGCDLNode(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (!n.op)
            return false;

        const GCDLEvalMap& map = gcdlEvalMap();

        auto it = map.find(n.op);
        if (it == map.end())
            return false;

        return it->second(n, ctx);
    }



    static inline bool evalGCDLProgram(const GCDLProgram& prog, GCDLEvalContext& ctx)
    {
        ctx.clear();

        for (size_t i = 0; i < prog.nodes.size(); i++) {
            if (!evalGCDLNode(prog.nodes[i], ctx))
                return false;
        }

        return true;
    }
}

