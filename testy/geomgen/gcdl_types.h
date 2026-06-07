// gcdl_types.h - types for GCDL (Geometric Construction Description Language)
#pragma once

#include "definitions.h"
#include "nametable.h"
#include "gcdl_program.h"

#include <vector>

// GCDL - Geometric Construction Description Language
//
// A language for describing geometric constructions,
// such as those found in islamic geometric art, or in geometry textbooks.
//

namespace waavs
{
    enum struct GCDLGeomKind {
        None,
        Point,
        Line,
        Circle,
        Arc,
        Path,
        PointSet,
    };

    enum struct GCDLPointPickMode {
        First = 0,
        Upper = 1,
        Lower = 2,
        Left = 3,
        Right = 4
    };

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
        GCDLFabricationRole fabricationRole = GCDLFabricationRole::None;
    };


    struct GeoRef {
        InternedKey id=nullptr;
        uint32_t slot = 0;
    };

    struct GCDLPoint {
        double x=0.0;
        double y=0.0;
    };

    struct GCDLLine {
        GCDLPoint a;
        GCDLPoint b;
    };

    struct GCDLCircle {
        GCDLPoint c;
        double r=0.0;
    };

    struct GCDLArc {
        GCDLPoint c;
        double r = 0.0;
        double a0 = 0.0;
        double a1 = 0.0;
        bool ccw = true;
    };

    // Representation of a path as a sequence of points.
    // The points are interpreted as connected by straight lines.
    struct GCDLPath {
        uint32_t firstPoint = 0;
        uint32_t pointCount = 0;
        bool closed = true;
    };

    struct GCDLPrimitiveNode {
        InternedKey id = nullptr;
        InternedKey op = nullptr;
        std::vector<GeoRef> refs;
        std::vector<double> nums;
        GCDLNodeMeta meta{};

    };

    struct GCDLProgramNode
    {
        InternedKey id = nullptr;
        InternedKey op = nullptr;

        std::vector<GeoRef> refs;
        std::vector<double> nums;

        GCDLNodeMeta meta{};
    };
}

// Declaring names that will be used in GCDL programs. 
namespace waavs
{
#define GCDL_ELEMENT_KEYS(x) \
    x(arc, "arc") \
    x(arc_center_angles, "arc-center-angles") \
    x(arc_rotate, "arc-rotate") \
    x(circle, "circle") \
    x(circle_center_radius, "circle-center-radius") \
    x(circle_rotate, "circle-rotate") \
    x(copy_rotate, "copy-rotate") \
    x(copy_translate, "copy-translate") \
    x(copy_mirror, "copy-mirror") \
    x(divide, "divide") \
    x(intersect, "intersect") \
    x(line, "line") \
    x(line_parallel, "line-parallel") \
    x(line_perpendicular, "line-perpendicular") \
    x(line_rotate, "line-rotate") \
    x(line_from_point_angle, "line-from-point-angle") \
    x(mirror_point, "mirror-point") \
    x(mirror_arc, "mirror-arc") \
    x(motif, "motif") \
    x(path_from_refs, "path-from-refs") \
    x(path_offset, "path-offset") \
    x(path_union, "path-union") \
    x(path_intersect, "path-intersect") \
    x(path_difference, "path-difference") \
    x(clip_to_panel, "clip-to-panel") \
    x(point, "point") \
    x(point_polar, "point-polar") \
    x(point_pick, "point-pick") \
    x(point_along_line, "point-along-line") \
    x(point_midpoint, "point-midpoint") \
    x(point_rotate, "point-rotate") \
    x(point_translate, "point-translate") \
    x(point_project, "point-project") \
    x(path_from_points, "path-from-points") \
    x(path_close, "path-close") \
    x(polygon_regular, "polygon-regular") \
    x(polygon_star, "polygon-star")

#define GCDL_DECLARE_KEY(name, text) \
    static InternedKey GCDLElem_##name() \
    { \
        static InternedKey key = PSNameTable::INTERN(text); \
        return key; \
    }

    GCDL_ELEMENT_KEYS(GCDL_DECLARE_KEY)

#undef GCDL_DECLARE_KEY

    // ----------------------------------------
    // Names for operations (node ops)
    //
#define GCDL_OP_KEYS(x) \
    x(arc_center_angles, "arc.centerAngles") \
    x(arc_circle_from_to, "arc.circleFromTo") \
    x(arc_rotate, "arc.rotate") \
    x(circle_center_point, "circle.centerPoint") \
    x(circle_center_radius, "circle.centerRadius") \
    x(circle_rotate, "circle.rotate") \
    x(copy_rotate, "copy.rotate") \
    x(copy_translate, "copy.translate") \
    x(copy_mirror, "copy.mirror") \
    x(intersect_circle_circle, "intersect.circleCircle") \
    x(intersect_line_line, "intersect.lineLine") \
    x(intersect_line_circle, "intersect.lineCircle") \
    x(line_divide, "line.divide") \
    x(line_from_points, "line.fromPoints") \
    x(line_from_point_angle, "line.fromPointAngle") \
    x(line_parallel, "line.parallel") \
    x(line_perpendicular, "line.perpendicular") \
    x(line_rotate, "line.rotate") \
    x(line_offset, "line.offset") \
    x(mirror_point, "mirror.point") \
    x(mirror_arc, "mirror.arc") \
    x(path_from_points, "path.fromPoints") \
    x(path_from_refs, "path.fromRefs") \
    x(path_close, "path.close") \
    x(path_offset, "path.offset") \
    x(path_union, "path.union") \
    x(path_intersect, "path.intersect") \
    x(path_difference, "path.difference") \
    x(clip_to_panel, "clip.toPanel") \
    x(point_xy, "point.xy") \
    x(point_polar, "point.polar") \
    x(point_pick, "point.pick") \
    x(point_along_line, "point.alongLine") \
    x(point_midpoint, "point.midpoint") \
    x(point_rotate, "point.rotate") \
    x(point_translate, "point.translate") \
    x(point_project, "point.project") \
    x(polygon_regular, "polygon.regular") \
    x(polygon_star, "polygon.star")

#define GCDL_DECLARE_OP_KEY(name, text) \
    static InternedKey GCDLOp_##name() \
    { \
        static InternedKey key = PSNameTable::INTERN(text); \
        return key; \
    }

        GCDL_OP_KEYS(GCDL_DECLARE_OP_KEY)

#undef GCDL_DECLARE_OP_KEY


}


