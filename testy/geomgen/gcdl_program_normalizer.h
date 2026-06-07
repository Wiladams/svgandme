#pragma once

#include "gcdl_program_authored.h"
#include "gcdl_program_normalized.h"
#include "gcdl_parse_xml.h"

#include <unordered_map>
#include <cstdio>

namespace waavs
{
    using GCDLDefinitionMap =
        std::unordered_map<InternedKey, const GCDLAuthoredNode*,
        InternedKeyHash, InternedKeyEquivalent>;

    using GCDLIdRemap =
        std::unordered_map<InternedKey, InternedKey,
        InternedKeyHash, InternedKeyEquivalent>;

    enum struct GCDLMotifAnchor
    {
        Origin,
        Center
    };

    struct GCDLMotifDef
    {
        const GCDLAuthoredNode* node = nullptr;

        double width = 0.0;
        double height = 0.0;
        GCDLMotifAnchor anchor = GCDLMotifAnchor::Origin;
    };

    using GCDLMotifMap =
        std::unordered_map<InternedKey, GCDLMotifDef,
        InternedKeyHash, InternedKeyEquivalent>;


    struct GCDLDefinitionTable
    {
        GCDLMotifMap motifs{};

        void clear() noexcept
        {
            motifs.clear();
        }
    };

    struct GCDLNormalizeContext
    {
        const GCDLAuthoredProgram* authored = nullptr;
        const GCDLDefinitionTable* defs = nullptr;
        GCDLNormalizedProgram* out = nullptr;
    };

    struct GCDLNormalizeState
    {
        InternedKey currentStep = nullptr;
        InternedKey currentMotif = nullptr;
        GCDLNodeMeta meta{};
    };





    struct GCDLNormalizedCopyContext
    {
        GCDLIdRemap ids{};

        double tx = 0.0;
        double ty = 0.0;

        bool translate = false;
    };

    static bool gcdl_normalize_authored_node(
        const GCDLAuthoredNode& src,
        GCDLNormalizeState st,
        GCDLNormalizeContext& cx) noexcept;

    static bool gcdl_get_attr(
        const GCDLAuthoredNode& node,
        const char* name,
        ByteSpan& value) noexcept
    {
        return node.attrs.getValueBySpan(ByteSpan(name), value);
    }

    static bool gcdl_get_ref_attr(
        const GCDLAuthoredNode& node,
        const char* name,
        GeoRef& ref) noexcept
    {
        ByteSpan span{};
        if (!gcdl_get_attr(node, name, span))
            return false;

        return parseGeoRef(span, ref);
    }

    static bool gcdl_get_double_attr(
        const GCDLAuthoredNode& node,
        const char* name,
        double& value) noexcept
    {
        ByteSpan span{};
        if (!gcdl_get_attr(node, name, span))
            return false;

        return parseNumber(span, value);
    }

    static bool gcdl_get_u32_attr(
        const GCDLAuthoredNode& node,
        const char* name,
        uint32_t& value) noexcept
    {
        ByteSpan span{};
        if (!gcdl_get_attr(node, name, span))
            return false;

        uint64_t tmp = 0;
        if (!parse64u(span, tmp))
            return false;

        value = (uint32_t)tmp;
        return true;
    }

    static bool gcdl_get_bool_attr_default(
        const GCDLAuthoredNode& node,
        const char* name,
        bool defaultValue,
        bool& value) noexcept
    {
        ByteSpan span{};
        if (!gcdl_get_attr(node, name, span)) {
            value = defaultValue;
            return true;
        }

        if (span == "true" || span == "1") {
            value = true;
            return true;
        }

        if (span == "false" || span == "0") {
            value = false;
            return true;
        }

        return false;
    }

    static bool gcdl_parse_design_role(
        ByteSpan span,
        GCDLDesignRole& out) noexcept
    {
        if (span == "construction") { out = GCDLDesignRole::Construction; return true; }
        if (span == "finished") { out = GCDLDesignRole::Finished; return true; }
        if (span == "void-boundary") { out = GCDLDesignRole::VoidBoundary; return true; }
        if (span == "solid-boundary") { out = GCDLDesignRole::SolidBoundary; return true; }
        if (span == "decoration") { out = GCDLDesignRole::Decoration; return true; }
        if (span == "dimension") { out = GCDLDesignRole::Dimension; return true; }
        if (span == "label") { out = GCDLDesignRole::Label; return true; }
        if (span == "debug") { out = GCDLDesignRole::Debug; return true; }

        return false;
    }

    static bool gcdl_parse_fab_role(
        ByteSpan span,
        GCDLFabricationRole& out) noexcept
    {
        if (span == "cut") { out = GCDLFabricationRole::Cut; return true; }
        if (span == "score") { out = GCDLFabricationRole::Score; return true; }
        if (span == "engrave") { out = GCDLFabricationRole::Engrave; return true; }
        if (span == "drill") { out = GCDLFabricationRole::Drill; return true; }
        if (span == "pocket") { out = GCDLFabricationRole::Pocket; return true; }

        return false;
    }

    static bool gcdl_parse_motif_anchor( ByteSpan span, GCDLMotifAnchor& out) noexcept
    {
        if (span == "origin") {
            out = GCDLMotifAnchor::Origin;
            return true;
        }

        if (span == "center") {
            out = GCDLMotifAnchor::Center;
            return true;
        }

        return false;
    }

    static void gcdl_apply_node_meta_attrs(
        const GCDLAuthoredNode& src,
        GCDLNodeMeta& meta) noexcept
    {
        ByteSpan roleSpan{};
        if (gcdl_get_attr(src, "design-role", roleSpan))
            gcdl_parse_design_role(roleSpan, meta.designRole);

        ByteSpan fabSpan{};
        if (gcdl_get_attr(src, "fab-role", fabSpan))
            gcdl_parse_fab_role(fabSpan, meta.fabricationRole);
    }

    static GCDLNodeMeta gcdl_make_node_meta(
        const GCDLAuthoredNode& src,
        const GCDLNormalizeState& st) noexcept
    {
        GCDLNodeMeta meta = st.meta;

        meta.sourceStep = st.currentStep;
        meta.sourceMotif = st.currentMotif;
        meta.sourceNode = src.id;

        gcdl_apply_node_meta_attrs(src, meta);
        return meta;
    }

    static bool gcdl_collect_definitions( const GCDLAuthoredNode& node, GCDLDefinitionTable& defs) noexcept
    {
        if (node.elem == GCDLElem_motif()) {
            if (!node.id)
                return false;

            GCDLMotifDef def;
            def.node = &node;

            gcdl_get_double_attr(node, "width", def.width);
            gcdl_get_double_attr(node, "height", def.height);

            ByteSpan anchorSpan{};
            if (gcdl_get_attr(node, "anchor", anchorSpan))
                gcdl_parse_motif_anchor(anchorSpan, def.anchor);

            defs.motifs[node.id] = def;
        }

        for (const auto& child : node.children) {
            if (!gcdl_collect_definitions(child, defs))
                return false;
        }

        return true;
    }

    static const GCDLMotifDef* gcdl_find_motif( const GCDLDefinitionTable& defs, InternedKey id) noexcept
    {
        if (!id)
            return nullptr;

        auto it = defs.motifs.find(id);
        if (it == defs.motifs.end())
            return nullptr;

        return &it->second;
    }

    static InternedKey gcdl_make_child_id(
        InternedKey parent,
        const char* suffix) noexcept
    {
        char buf[256];

        std::snprintf(
            buf,
            sizeof(buf),
            "%s_%s",
            parent ? parent : "node",
            suffix ? suffix : "child");

        return PSNameTable::INTERN(buf);
    }

    static InternedKey gcdl_make_grid_id(
        InternedKey prefix,
        int col,
        int row,
        InternedKey local) noexcept
    {
        char buf[256];

        std::snprintf(
            buf,
            sizeof(buf),
            "%s_%d_%d_%s",
            prefix ? prefix : "grid",
            col,
            row,
            local ? local : "node");

        return PSNameTable::INTERN(buf);
    }

    static InternedKey gcdl_remap_id(
        const GCDLNormalizedCopyContext& cx,
        InternedKey id) noexcept
    {
        if (!id)
            return nullptr;

        auto it = cx.ids.find(id);
        if (it == cx.ids.end())
            return id;

        return it->second;
    }

    static bool gcdl_copy_normalized_node(
        const GCDLNormalizedNode& src,
        const GCDLNormalizedCopyContext& cx,
        GCDLNormalizedNode& dst) noexcept
    {
        dst = src;

        dst.id = gcdl_remap_id(cx, src.id);
        if (!dst.id)
            return false;

        for (auto& ref : dst.refs)
            ref.id = gcdl_remap_id(cx, ref.id);

        if (cx.translate && dst.op == GCDLOp_point_xy()) {
            if (dst.nums.size() < 2)
                return false;

            dst.nums[0] += cx.tx;
            dst.nums[1] += cx.ty;
        }

        return true;
    }

    static bool gcdl_copy_normalized_program(
        const GCDLNormalizedProgram& src,
        const GCDLNormalizedCopyContext& cx,
        GCDLNormalizedProgram& out) noexcept
    {
        for (const auto& n : src.nodes) {
            GCDLNormalizedNode copy;

            if (!gcdl_copy_normalized_node(n, cx, copy))
                return false;

            if (!out.addNode(copy))
                return false;
        }

        return true;
    }

    static bool gcdl_emit_point_xy(
        GCDLNormalizedProgram& out,
        InternedKey id,
        double x,
        double y,
        const GCDLNodeMeta& meta) noexcept
    {
        GCDLNormalizedNode n;

        n.id = id;
        n.op = GCDLOp_point_xy();
        n.nums.push_back(x);
        n.nums.push_back(y);
        n.meta = meta;

        return out.addNode(n);
    }

    static bool gcdl_emit_rect_path(
        GCDLNormalizedProgram& out,
        InternedKey id,
        double x,
        double y,
        double w,
        double h,
        const GCDLNodeMeta& meta) noexcept
    {
        if (!id)
            return false;

        if (!(w > 0.0) || !(h > 0.0))
            return false;

        InternedKey p0 = gcdl_make_child_id(id, "p0");
        InternedKey p1 = gcdl_make_child_id(id, "p1");
        InternedKey p2 = gcdl_make_child_id(id, "p2");
        InternedKey p3 = gcdl_make_child_id(id, "p3");

        if (!gcdl_emit_point_xy(out, p0, x, y, meta))
            return false;

        if (!gcdl_emit_point_xy(out, p1, x + w, y, meta))
            return false;

        if (!gcdl_emit_point_xy(out, p2, x + w, y + h, meta))
            return false;

        if (!gcdl_emit_point_xy(out, p3, x, y + h, meta))
            return false;

        GCDLNormalizedNode path;

        path.id = id;
        path.op = GCDLOp_path_from_refs();
        path.refs.push_back({ p0, 0 });
        path.refs.push_back({ p1, 0 });
        path.refs.push_back({ p2, 0 });
        path.refs.push_back({ p3, 0 });
        path.nums.push_back(1.0);
        path.meta = meta;

        return out.addNode(path);
    }

    static bool gcdl_normalize_point(
        const GCDLAuthoredNode& src,
        GCDLNormalizeState st,
        GCDLNormalizeContext& cx) noexcept
    {
        if (!cx.out)
            return false;

        GCDLNormalizedNode n;

        n.id = src.id;
        n.op = GCDLOp_point_xy();

        if (!n.id)
            return false;

        double x = 0.0;
        double y = 0.0;

        if (!gcdl_get_double_attr(src, "x", x))
            return false;

        if (!gcdl_get_double_attr(src, "y", y))
            return false;

        n.nums.push_back(x);
        n.nums.push_back(y);
        n.meta = gcdl_make_node_meta(src, st);

        return cx.out->addNode(n);
    }

    static bool gcdl_normalize_circle_center_radius(
        const GCDLAuthoredNode& src,
        GCDLNormalizeState st,
        GCDLNormalizeContext& cx) noexcept
    {
        if (!cx.out)
            return false;

        GCDLNormalizedNode n;

        n.id = src.id;
        n.op = GCDLOp_circle_center_radius();

        if (!n.id)
            return false;

        GeoRef center;
        double radius = 0.0;

        if (!gcdl_get_ref_attr(src, "center", center))
            return false;

        if (!gcdl_get_double_attr(src, "radius", radius))
            return false;

        n.refs.push_back(center);
        n.nums.push_back(radius);
        n.meta = gcdl_make_node_meta(src, st);

        return cx.out->addNode(n);
    }

    static bool gcdl_normalize_polygon_regular(
        const GCDLAuthoredNode& src,
        GCDLNormalizeState st,
        GCDLNormalizeContext& cx) noexcept
    {
        if (!cx.out)
            return false;

        GCDLNormalizedNode n;

        n.id = src.id;
        n.op = GCDLOp_polygon_regular();

        if (!n.id)
            return false;

        GeoRef center;
        double radius = 0.0;
        uint32_t sides = 0;
        double startAngle = 0.0;

        if (!gcdl_get_ref_attr(src, "center", center))
            return false;

        if (!gcdl_get_double_attr(src, "radius", radius))
            return false;

        if (!gcdl_get_u32_attr(src, "sides", sides))
            return false;

        gcdl_get_double_attr(src, "start-angle", startAngle);

        n.refs.push_back(center);
        n.nums.push_back(radius);
        n.nums.push_back((double)sides);
        n.nums.push_back(startAngle);
        n.meta = gcdl_make_node_meta(src, st);

        return cx.out->addNode(n);
    }

    static bool gcdl_normalize_polygon_star(
        const GCDLAuthoredNode& src,
        GCDLNormalizeState st,
        GCDLNormalizeContext& cx) noexcept
    {
        if (!cx.out)
            return false;

        GCDLNormalizedNode n;

        n.id = src.id;
        n.op = GCDLOp_polygon_star();

        if (!n.id)
            return false;

        GeoRef center;
        double radius = 0.0;
        uint32_t points = 0;
        uint32_t skip = 0;
        double startAngle = 0.0;

        if (!gcdl_get_ref_attr(src, "center", center))
            return false;

        if (!gcdl_get_double_attr(src, "radius", radius))
            return false;

        if (!gcdl_get_u32_attr(src, "points", points))
            return false;

        if (!gcdl_get_u32_attr(src, "skip", skip))
            return false;

        gcdl_get_double_attr(src, "start-angle", startAngle);

        n.refs.push_back(center);
        n.nums.push_back(radius);
        n.nums.push_back((double)points);
        n.nums.push_back((double)skip);
        n.nums.push_back(startAngle);
        n.meta = gcdl_make_node_meta(src, st);

        return cx.out->addNode(n);
    }

    static bool gcdl_normalize_path_from_points(
        const GCDLAuthoredNode& src,
        GCDLNormalizeState st,
        GCDLNormalizeContext& cx) noexcept
    {
        if (!cx.out)
            return false;

        GCDLNormalizedNode n;

        n.id = src.id;
        n.op = GCDLOp_path_from_points();

        if (!n.id)
            return false;

        GeoRef points;
        bool closed = true;

        if (!gcdl_get_ref_attr(src, "points", points))
            return false;

        if (!gcdl_get_bool_attr_default(src, "closed", true, closed))
            return false;

        n.refs.push_back(points);
        n.nums.push_back(closed ? 1.0 : 0.0);
        n.meta = gcdl_make_node_meta(src, st);

        return cx.out->addNode(n);
    }

    static bool gcdl_normalize_panel(
        const GCDLAuthoredNode& src,
        GCDLNormalizeState st,
        GCDLNormalizeContext& cx) noexcept
    {
        if (!cx.out)
            return false;

        if (!src.id)
            return false;

        double w = 0.0;
        double h = 0.0;

        if (!gcdl_get_double_attr(src, "width", w))
            return false;

        if (!gcdl_get_double_attr(src, "height", h))
            return false;

        GCDLNodeMeta meta = gcdl_make_node_meta(src, st);

        if (meta.designRole == GCDLDesignRole::None)
            meta.designRole = GCDLDesignRole::Construction;

        return gcdl_emit_rect_path(*cx.out, src.id, 0.0, 0.0, w, h, meta);
    }

    static bool gcdl_normalize_inset_rect(
        const GCDLAuthoredNode& src,
        GCDLNormalizeState st,
        GCDLNormalizeContext& cx) noexcept
    {
        if (!cx.out || !cx.authored)
            return false;

        if (!src.id)
            return false;

        GeoRef from;
        double inset = 0.0;

        if (!gcdl_get_ref_attr(src, "from", from))
            return false;

        if (!gcdl_get_double_attr(src, "inset", inset))
            return false;

        const GCDLAuthoredNode* base = nullptr;

        if (from.id) {
            // For now, only support inset from an authored panel.
            // This should eventually resolve from normalized/evaluated bounds.
            const GCDLAuthoredNode& root = cx.authored->rootNode;

            // Local recursive search.
            struct Finder
            {
                static bool find(
                    const GCDLAuthoredNode& n,
                    InternedKey id,
                    const GCDLAuthoredNode*& out) noexcept
                {
                    if (n.id == id) {
                        out = &n;
                        return true;
                    }

                    for (const auto& c : n.children) {
                        if (find(c, id, out))
                            return true;
                    }

                    return false;
                }
            };

            if (!Finder::find(root, from.id, base))
                return false;
        }

        if (!base)
            return false;

        double w = 0.0;
        double h = 0.0;

        if (!gcdl_get_double_attr(*base, "width", w))
            return false;

        if (!gcdl_get_double_attr(*base, "height", h))
            return false;

        if (!(w > 0.0) || !(h > 0.0))
            return false;

        if (inset < 0.0)
            return false;

        if ((inset * 2.0) >= w || (inset * 2.0) >= h)
            return false;

        GCDLNodeMeta meta = gcdl_make_node_meta(src, st);

        if (meta.designRole == GCDLDesignRole::None)
            meta.designRole = GCDLDesignRole::Construction;

        return gcdl_emit_rect_path(
            *cx.out,
            src.id,
            inset,
            inset,
            w - inset * 2.0,
            h - inset * 2.0,
            meta);
    }

    static bool gcdl_normalize_motif_body(
        const GCDLAuthoredNode& motif,
        GCDLNormalizeState st,
        GCDLNormalizedProgram& dst,
        GCDLNormalizeContext& parentCx) noexcept
    {
        GCDLNormalizeContext localCx = parentCx;
        localCx.out = &dst;

        st.currentMotif = motif.id;

        for (const auto& child : motif.children) {
            if (!gcdl_normalize_authored_node(child, st, localCx))
                return false;
        }

        return true;
    }

    static bool gcdl_normalize_repeat_grid(
        const GCDLAuthoredNode& src,
        GCDLNormalizeState st,
        GCDLNormalizeContext& cx) noexcept
    {
        if (!cx.out || !cx.defs)
            return false;

        if (!src.id)
            return false;

        GeoRef motifRef;
        double dx = 0.0;
        double dy = 0.0;

        if (!gcdl_get_ref_attr(src, "motif", motifRef))
            return false;

        if (!gcdl_get_double_attr(src, "dx", dx))
            return false;

        if (!gcdl_get_double_attr(src, "dy", dy))
            return false;

        const GCDLMotifDef* motifDef =
            gcdl_find_motif(*cx.defs, motifRef.id);

        if (!motifDef || !motifDef->node)
            return false;

        const GCDLAuthoredNode* motif = motifDef->node;

        if (!motif)
            return false;

        const int cols = 4;
        const int rows = 6;
        //int cols = floor(fieldWidth / dx);
        //int rows = floor(fieldHeight / dy);

        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                GCDLNormalizedProgram cellProg;

                if (!gcdl_normalize_motif_body(
                    *motif,
                    st,
                    cellProg,
                    cx))
                    return false;


                GCDLNormalizedCopyContext copyCx;
                copyCx.translate = true;

                double anchorX = 0.0;
                double anchorY = 0.0;

                if (motifDef->anchor == GCDLMotifAnchor::Center) {
                    anchorX = motifDef->width * 0.5;
                    anchorY = motifDef->height * 0.5;
                }

                // Temporary until boundary="field" is resolved.
                const double fieldX = 40.0;
                const double fieldY = 40.0;

                copyCx.tx = fieldX + anchorX + col * dx;
                copyCx.ty = fieldY + anchorY + row * dy;

                for (const auto& n : cellProg.nodes) {
                    InternedKey newId =
                        gcdl_make_grid_id(src.id, col, row, n.id);

                    copyCx.ids[n.id] = newId;
                }

                if (!gcdl_copy_normalized_program(
                    cellProg,
                    copyCx,
                    *cx.out))
                    return false;
            }
        }

        return true;
    }

    static bool gcdl_normalize_authored_node(
        const GCDLAuthoredNode& src,
        GCDLNormalizeState st,
        GCDLNormalizeContext& cx) noexcept
    {
        if (!src.elem)
            return false;

        if (src.elem == PSNameTable::INTERN("step")) {
            if (src.id)
                st.currentStep = src.id;
        }
        else if (src.elem == PSNameTable::INTERN("motif")) {
            // Motifs are definitions. They are emitted only when used.
            return true;
        }
        else if (src.elem == GCDLElem_point()) {
            if (!gcdl_normalize_point(src, st, cx))
                return false;
        }
        else if (src.elem == GCDLElem_circle_center_radius()) {
            if (!gcdl_normalize_circle_center_radius(src, st, cx))
                return false;
        }
        else if (src.elem == GCDLElem_polygon_regular()) {
            if (!gcdl_normalize_polygon_regular(src, st, cx))
                return false;
        }
        else if (src.elem == GCDLElem_polygon_star()) {
            if (!gcdl_normalize_polygon_star(src, st, cx))
                return false;
        }
        else if (src.elem == GCDLElem_path_from_points()) {
            if (!gcdl_normalize_path_from_points(src, st, cx))
                return false;
        }
        else if (src.elem == PSNameTable::INTERN("panel")) {
            if (!gcdl_normalize_panel(src, st, cx))
                return false;
        }
        else if (src.elem == PSNameTable::INTERN("inset-rect")) {
            if (!gcdl_normalize_inset_rect(src, st, cx))
                return false;
        }
        else if (src.elem == PSNameTable::INTERN("repeat-grid")) {
            if (!gcdl_normalize_repeat_grid(src, st, cx))
                return false;
        }

        for (const auto& child : src.children) {
            if (!gcdl_normalize_authored_node(child, st, cx))
                return false;
        }

        return true;
    }

    static bool gcdl_normalize_program(
        const GCDLAuthoredProgram& src,
        GCDLNormalizedProgram& out) noexcept
    {
        out.clear();

        if (src.empty())
            return false;

        GCDLDefinitionTable defs;
        if (!gcdl_collect_definitions(src.rootNode, defs))
            return false;

        GCDLNormalizeContext cx;
        cx.authored = &src;
        cx.defs = &defs;
        cx.out = &out;

        GCDLNormalizeState st{};
        return gcdl_normalize_authored_node(src.rootNode, st, cx);
    }
}
