// gcdl_eval_ops.h
#pragma once


#include "gcdl_eval_context.h"
#include "gcdl_maths.h"

#include <cmath>

// Some useful routines for evaluating GCDL nodes. 
// Add more as needed.
//
namespace waavs
{
    // -------------------------------------------
    //

    static inline bool eval_point_xy(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.nums.size() < 2)
            return false;

        return ctx.putPoint(n.id, { n.nums[0], n.nums[1] });
    }

    static inline bool eval_point_pick(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 1 || n.nums.size() < 1)
            return false;

        auto it = ctx.symbols.find(n.refs[0].id);
        if (it == ctx.symbols.end())
            return false;

        const GCDLGeom& g = it->second;
        if (g.kind != GCDLGeomKind::Point &&
            g.kind != GCDLGeomKind::PointSet)
            return false;

        if (g.count < 1)
            return false;

        const int mode = (int)n.nums[0];

        uint32_t best = 0;
        const GCDLPoint* pts = &ctx.points[g.first];

        for (uint32_t i = 1; i < g.count; i++) {
            const GCDLPoint& a = pts[best];
            const GCDLPoint& b = pts[i];

            switch ((GCDLPointPickMode)mode) {
            case GCDLPointPickMode::Upper:
                if (b.y > a.y)
                    best = i;
                break;

            case GCDLPointPickMode::Lower:
                if (b.y < a.y)
                    best = i;
                break;

            case GCDLPointPickMode::Left:
                if (b.x < a.x)
                    best = i;
                break;

            case GCDLPointPickMode::Right:
                if (b.x > a.x)
                    best = i;
                break;

            case GCDLPointPickMode::First:
            default:
                break;
            }
        }

        return ctx.putPoint(n.id, pts[best]);
    }

    static inline bool eval_line_from_points(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 2)
            return false;

        GCDLPoint a;
        GCDLPoint b;

        if (!ctx.getPoint(n.refs[0], a))
            return false;

        if (!ctx.getPoint(n.refs[1], b))
            return false;

        return ctx.putLine(n.id, { a, b });
    }

    static inline bool eval_circle_center_point(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 2)
            return false;

        GCDLPoint c;
        GCDLPoint p;

        if (!ctx.getPoint(n.refs[0], c))
            return false;

        if (!ctx.getPoint(n.refs[1], p))
            return false;

        GCDLCircle circle;
        circle.c = c;
        circle.r = gcdl_distance(c, p);

        return ctx.putCircle(n.id, circle);
    }

    static inline bool eval_circle_center_radius(
        const GCDLPrimitiveNode& n,
        GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 1 || n.nums.size() < 1)
            return false;

        GCDLPoint center;
        if (!ctx.getPoint(n.refs[0], center))
            return false;

        const double radius = n.nums[0];
        if (!(radius > 0.0))
            return false;

        GCDLCircle circle;
        circle.c = center;
        circle.r = radius;

        return ctx.putCircle(n.id, circle);
    }

    static inline bool eval_divide_line(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 1 || n.nums.size() < 1)
            return false;

        GCDLLine l;
        if (!ctx.getLine(n.refs[0], l))
            return false;

        const int parts = (int)n.nums[0];
        if (parts <= 0)
            return false;

        GCDLGeom g;
        g.kind = GCDLGeomKind::PointSet;
        g.first = (uint32_t)ctx.points.size();
        g.count = (uint32_t)(parts + 1);

        for (int i = 0; i <= parts; i++) {
            const double t = (double)i / (double)parts;

            GCDLPoint p;
            p.x = l.a.x + (l.b.x - l.a.x) * t;
            p.y = l.a.y + (l.b.y - l.a.y) * t;

            ctx.points.push_back(p);
        }

        ctx.symbols[n.id] = g;
        return true;
    }


    static inline bool eval_intersect_circle_circle(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 2)
            return false;

        GCDLCircle c0;
        GCDLCircle c1;

        if (!ctx.getCircle(n.refs[0], c0))
            return false;

        if (!ctx.getCircle(n.refs[1], c1))
            return false;

        const double x0 = c0.c.x;
        const double y0 = c0.c.y;
        const double x1 = c1.c.x;
        const double y1 = c1.c.y;

        const double dx = x1 - x0;
        const double dy = y1 - y0;
        const double d = std::sqrt(dx * dx + dy * dy);

        if (d <= 0.0)
            return false;

        if (d > c0.r + c1.r)
            return false;

        if (d < std::fabs(c0.r - c1.r))
            return false;

        const double a =
            ((c0.r * c0.r) - (c1.r * c1.r) + (d * d)) / (2.0 * d);

        const double h2 = (c0.r * c0.r) - (a * a);

        if (h2 < 0.0)
            return false;

        const double h = std::sqrt(h2);

        const double xm = x0 + a * dx / d;
        const double ym = y0 + a * dy / d;

        const double rx = -dy * h / d;
        const double ry = dx * h / d;

        GCDLPoint p0;
        p0.x = xm + rx;
        p0.y = ym + ry;

        GCDLPoint p1;
        p1.x = xm - rx;
        p1.y = ym - ry;

        // Stable Cartesian ordering:
        // slot 0 = upper point
        // slot 1 = lower point
        if (p1.y > p0.y) {
            GCDLPoint tmp = p0;
            p0 = p1;
            p1 = tmp;
        }

        GCDLGeom g;
        g.kind = GCDLGeomKind::PointSet;
        g.first = (uint32_t)ctx.points.size();

        ctx.points.push_back(p0);

        if (h > 0.0) {
            ctx.points.push_back(p1);
            g.count = 2;
        }
        else {
            g.count = 1;
        }

        ctx.symbols[n.id] = g;
        return true;
    }

    static inline bool eval_intersect_line_circle(
        const GCDLPrimitiveNode& n,
        GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 2)
            return false;

        GCDLLine line;
        GCDLCircle circle;

        if (!ctx.getLine(n.refs[0], line))
            return false;

        if (!ctx.getCircle(n.refs[1], circle))
            return false;

        const double ax = line.a.x;
        const double ay = line.a.y;
        const double bx = line.b.x;
        const double by = line.b.y;

        const double dx = bx - ax;
        const double dy = by - ay;

        const double fx = ax - circle.c.x;
        const double fy = ay - circle.c.y;

        const double aa = dx * dx + dy * dy;
        if (aa <= 0.0)
            return false;

        const double bb = 2.0 * (fx * dx + fy * dy);
        const double cc = (fx * fx + fy * fy) - circle.r * circle.r;

        const double disc = bb * bb - 4.0 * aa * cc;

        if (disc < 0.0)
            return false;

        const double root = std::sqrt(disc);

        double t0 = (-bb - root) / (2.0 * aa);
        double t1 = (-bb + root) / (2.0 * aa);

        if (t1 < t0) {
            double tmp = t0;
            t0 = t1;
            t1 = tmp;
        }

        GCDLGeom g;
        g.kind = GCDLGeomKind::PointSet;
        g.first = (uint32_t)ctx.points.size();

        GCDLPoint p0;
        p0.x = ax + dx * t0;
        p0.y = ay + dy * t0;

        ctx.points.push_back(p0);

        if (root > 0.0) {
            GCDLPoint p1;
            p1.x = ax + dx * t1;
            p1.y = ay + dy * t1;

            ctx.points.push_back(p1);
            g.count = 2;
        }
        else {
            g.count = 1;
        }

        ctx.symbols[n.id] = g;
        return true;
    }

    static inline bool eval_arc_center_angles( const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 1 || n.nums.size() < 3)
            return false;

        GCDLPoint center;
        if (!ctx.getPoint(n.refs[0], center))
            return false;

        const double radius = n.nums[0];
        if (!(radius > 0.0))
            return false;

        GCDLArc arc;
        arc.c = center;
        arc.r = radius;
        arc.a0 = gcdl_deg_to_rad(n.nums[1]);
        arc.a1 = gcdl_deg_to_rad(n.nums[2]);
        arc.ccw = true;

        if (n.nums.size() >= 4)
            arc.ccw = n.nums[3] != 0.0;

        return ctx.putArc(n.id, arc);
    }

    static inline bool eval_arc_circle_from_to(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 3)
            return false;

        GCDLCircle c;
        GCDLPoint from;
        GCDLPoint to;

        if (!ctx.getCircle(n.refs[0], c))
            return false;

        if (!ctx.getPoint(n.refs[1], from))
            return false;

        if (!ctx.getPoint(n.refs[2], to))
            return false;

        GCDLArc a;
        a.c = c.c;
        a.r = c.r;
        a.a0 = gcdl_angle_of(c.c, from);
        a.a1 = gcdl_angle_of(c.c, to);
        a.ccw = true;

        if (n.nums.size() >= 1)
            a.ccw = n.nums[0] != 0.0;

        return ctx.putArc(n.id, a);
    }

    static inline bool eval_intersect_line_line(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 2)
            return false;

        GCDLLine l0;
        GCDLLine l1;

        if (!ctx.getLine(n.refs[0], l0))
            return false;

        if (!ctx.getLine(n.refs[1], l1))
            return false;

        const double x1 = l0.a.x;
        const double y1 = l0.a.y;
        const double x2 = l0.b.x;
        const double y2 = l0.b.y;

        const double x3 = l1.a.x;
        const double y3 = l1.a.y;
        const double x4 = l1.b.x;
        const double y4 = l1.b.y;

        const double den =
            (x1 - x2) * (y3 - y4) -
            (y1 - y2) * (x3 - x4);

        if (std::fabs(den) <= 1.0e-12)
            return false;

        const double a = x1 * y2 - y1 * x2;
        const double b = x3 * y4 - y3 * x4;

        GCDLPoint p;
        p.x = (a * (x3 - x4) - (x1 - x2) * b) / den;
        p.y = (a * (y3 - y4) - (y1 - y2) * b) / den;

        return ctx.putPoint(n.id, p);
    }

    static inline GCDLPoint gcdl_mirror_point_across_line(
        const GCDLPoint& p,
        const GCDLLine& axis)
    {
        const double ax = axis.a.x;
        const double ay = axis.a.y;
        const double bx = axis.b.x;
        const double by = axis.b.y;

        const double dx = bx - ax;
        const double dy = by - ay;

        const double len2 = dx * dx + dy * dy;

        if (len2 <= 0.0)
            return p;

        const double t =
            ((p.x - ax) * dx + (p.y - ay) * dy) / len2;

        GCDLPoint foot;
        foot.x = ax + t * dx;
        foot.y = ay + t * dy;

        GCDLPoint out;
        out.x = 2.0 * foot.x - p.x;
        out.y = 2.0 * foot.y - p.y;

        return out;
    }

    static inline bool eval_mirror_point(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 2)
            return false;

        GCDLPoint p;
        GCDLLine axis;

        if (!ctx.getPoint(n.refs[0], p))
            return false;

        if (!ctx.getLine(n.refs[1], axis))
            return false;

        return ctx.putPoint(n.id, gcdl_mirror_point_across_line(p, axis));
    }

    static inline bool eval_mirror_arc(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 2)
            return false;

        GCDLArc src;
        GCDLLine axis;

        if (!ctx.getArc(n.refs[0], src))
            return false;

        if (!ctx.getLine(n.refs[1], axis))
            return false;

        GCDLPoint p0;
        p0.x = src.c.x + std::cos(src.a0) * src.r;
        p0.y = src.c.y + std::sin(src.a0) * src.r;

        GCDLPoint p1;
        p1.x = src.c.x + std::cos(src.a1) * src.r;
        p1.y = src.c.y + std::sin(src.a1) * src.r;

        GCDLPoint mc = gcdl_mirror_point_across_line(src.c, axis);
        GCDLPoint mp0 = gcdl_mirror_point_across_line(p0, axis);
        GCDLPoint mp1 = gcdl_mirror_point_across_line(p1, axis);

        GCDLArc out;
        out.c = mc;
        out.r = src.r;
        out.a0 = gcdl_angle_of(mc, mp0);
        out.a1 = gcdl_angle_of(mc, mp1);

        // Reflection reverses orientation.
        out.ccw = !src.ccw;

        return ctx.putArc(n.id, out);
    }

    static inline bool eval_point_polar(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 1 || n.nums.size() < 2)
            return false;

        GCDLPoint c;
        if (!ctx.getPoint(n.refs[0], c))
            return false;

        const double r = n.nums[0];
        const double angle = gcdl_deg_to_rad(n.nums[1]);

        GCDLPoint p;
        p.x = c.x + std::cos(angle) * r;
        p.y = c.y + std::sin(angle) * r;

        return ctx.putPoint(n.id, p);
    }

    static inline bool eval_point_along_line( const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 1 || n.nums.size() < 1)
            return false;

        GCDLLine line;
        if (!ctx.getLine(n.refs[0], line))
            return false;

        const double t = n.nums[0];

        GCDLPoint p;
        p.x = line.a.x + (line.b.x - line.a.x) * t;
        p.y = line.a.y + (line.b.y - line.a.y) * t;

        return ctx.putPoint(n.id, p);
    }

    static inline bool eval_line_from_point_angle(
        const GCDLPrimitiveNode& n,
        GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 1 || n.nums.size() < 1)
            return false;

        GCDLPoint a;
        if (!ctx.getPoint(n.refs[0], a))
            return false;

        const double angle = gcdl_deg_to_rad(n.nums[0]);
        const double length = n.nums.size() >= 2 ? n.nums[1] : 1000.0;

        GCDLPoint b;
        b.x = a.x + std::cos(angle) * length;
        b.y = a.y + std::sin(angle) * length;

        return ctx.putLine(n.id, { a, b });
    }

    static inline bool eval_line_parallel(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 2)
            return false;

        GCDLLine src;
        GCDLPoint p;

        if (!ctx.getLine(n.refs[0], src))
            return false;

        if (!ctx.getPoint(n.refs[1], p))
            return false;

        const double dx = src.b.x - src.a.x;
        const double dy = src.b.y - src.a.y;

        if (dx == 0.0 && dy == 0.0)
            return false;

        GCDLLine out;
        out.a = p;
        out.b.x = p.x + dx;
        out.b.y = p.y + dy;

        return ctx.putLine(n.id, out);
    }

    static inline bool eval_line_perpendicular(
        const GCDLPrimitiveNode& n,
        GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 2)
            return false;

        GCDLLine src;
        GCDLPoint p;

        if (!ctx.getLine(n.refs[0], src))
            return false;

        if (!ctx.getPoint(n.refs[1], p))
            return false;

        const double dx = src.b.x - src.a.x;
        const double dy = src.b.y - src.a.y;

        if (dx == 0.0 && dy == 0.0)
            return false;

        GCDLLine out;

        out.a = p;

        out.b.x = p.x - dy;
        out.b.y = p.y + dx;

        return ctx.putLine(n.id, out);
    }

    static inline bool eval_point_project(
        const GCDLPrimitiveNode& n,
        GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 2)
            return false;

        GCDLPoint p;
        GCDLLine line;

        if (!ctx.getPoint(n.refs[0], p))
            return false;

        if (!ctx.getLine(n.refs[1], line))
            return false;

        const double ax = line.a.x;
        const double ay = line.a.y;
        const double bx = line.b.x;
        const double by = line.b.y;

        const double dx = bx - ax;
        const double dy = by - ay;

        const double len2 = dx * dx + dy * dy;
        if (len2 <= 0.0)
            return false;

        const double t =
            ((p.x - ax) * dx + (p.y - ay) * dy) / len2;

        GCDLPoint out;
        out.x = ax + t * dx;
        out.y = ay + t * dy;

        return ctx.putPoint(n.id, out);
    }

    static inline bool eval_point_rotate( const GCDLPrimitiveNode& n,  GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 2 || n.nums.size() < 1)
            return false;

        GCDLPoint point;
        GCDLPoint center;

        if (!ctx.getPoint(n.refs[0], point))
            return false;

        if (!ctx.getPoint(n.refs[1], center))
            return false;

        const GCDLPoint out =
            gcdl_rotate_point(point, center, n.nums[0]);

        return ctx.putPoint(n.id, out);
    }



    static inline bool eval_line_rotate(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 2 || n.nums.size() < 1)
            return false;

        GCDLLine line;
        GCDLPoint center;

        if (!ctx.getLine(n.refs[0], line))
            return false;

        if (!ctx.getPoint(n.refs[1], center))
            return false;

        GCDLLine out;
        out.a = gcdl_rotate_point(line.a, center, n.nums[0]);
        out.b = gcdl_rotate_point(line.b, center, n.nums[0]);

        return ctx.putLine(n.id, out);
    }

    static inline bool eval_polygon_regular(
        const GCDLPrimitiveNode& n,
        GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 1 || n.nums.size() < 2)
            return false;

        GCDLPoint center;
        if (!ctx.getPoint(n.refs[0], center))
            return false;

        const double radius = n.nums[0];
        const uint32_t sides = (uint32_t)n.nums[1];
        const double startAngle =
            (n.nums.size() >= 3) ? n.nums[2] : 0.0;

        if (sides < 3)
            return false;

        GCDLGeom g;
        g.kind = GCDLGeomKind::PointSet;
        g.first = (uint32_t)ctx.points.size();
        g.count = sides;

        const double step = 360.0 / (double)sides;

        for (uint32_t i = 0; i < sides; i++) {
            const double angle =
                gcdl_deg_to_rad(startAngle + step * i);

            GCDLPoint p;
            p.x = center.x + radius * std::cos(angle);
            p.y = center.y + radius * std::sin(angle);

            ctx.points.push_back(p);
        }

        ctx.symbols[n.id] = g;
        return true;
    }

    static inline bool eval_path_from_points(const GCDLPrimitiveNode& n, GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 1)
            return false;

        auto it = ctx.symbols.find(n.refs[0].id);
        if (it == ctx.symbols.end())
            return false;

        const GCDLGeom& g = it->second;

        if (g.kind != GCDLGeomKind::PointSet &&
            g.kind != GCDLGeomKind::Point)
            return false;

        if (g.count < 1)
            return false;

        GCDLPath p;
        p.firstPoint = g.first + n.refs[0].slot;
        p.pointCount = g.count - n.refs[0].slot;
        p.closed = true;

        if (n.nums.size() >= 1)
            p.closed = n.nums[0] != 0.0;

        return ctx.putPath(n.id, p);
    }

    static inline bool eval_path_from_refs(
        const GCDLPrimitiveNode& n,
        GCDLEvalContext& ctx)
    {
        if (n.refs.empty())
            return false;

        const uint32_t first = (uint32_t)ctx.points.size();

        for (size_t i = 0; i < n.refs.size(); ++i) {
            GCDLPoint p;
            if (!ctx.getPoint(n.refs[i], p))
                return false;

            ctx.points.push_back(p);
        }

        GCDLPath path;
        path.firstPoint = first;
        path.pointCount = (uint32_t)n.refs.size();
        path.closed = true;

        if (!n.nums.empty())
            path.closed = n.nums[0] != 0.0;

        return ctx.putPath(n.id, path);
    }


    static inline bool eval_polygon_star(
        const GCDLPrimitiveNode& n,
        GCDLEvalContext& ctx)
    {
        if (n.refs.size() < 1 || n.nums.size() < 3)
            return false;

        GCDLPoint center;
        if (!ctx.getPoint(n.refs[0], center))
            return false;

        const double radius = n.nums[0];
        const uint32_t points = (uint32_t)n.nums[1];
        const uint32_t skip = (uint32_t)n.nums[2];
        const double startAngle = (n.nums.size() >= 4) ? n.nums[3] : 0.0;

        if (points < 3)
            return false;

        if (skip < 1 || skip >= points)
            return false;

        if (gcdl_gcd_u32(points, skip) != 1)
            return false;

        GCDLGeom g;
        g.kind = GCDLGeomKind::PointSet;
        g.first = (uint32_t)ctx.points.size();
        g.count = points;

        const double step = 360.0 / (double)points;

        uint32_t idx = 0;

        for (uint32_t i = 0; i < points; i++) {
            const double angle = gcdl_deg_to_rad(startAngle + step * (double)idx);

            GCDLPoint p;
            p.x = center.x + radius * std::cos(angle);
            p.y = center.y + radius * std::sin(angle);

            ctx.points.push_back(p);

            idx = (idx + skip) % points;
        }

        ctx.symbols[n.id] = g;
        return true;
    }
}

