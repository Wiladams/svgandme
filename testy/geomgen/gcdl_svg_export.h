#pragma once

#include "gcdl_eval_context.h"

#include <cstdio>
#include <cmath>

namespace waavs
{
    static inline double gcdl_arc_start_x(const GCDLArc& a)
    {
        return a.c.x + std::cos(a.a0) * a.r;
    }

    static inline double gcdl_arc_start_y(const GCDLArc& a)
    {
        return a.c.y + std::sin(a.a0) * a.r;
    }

    static inline double gcdl_arc_end_x(const GCDLArc& a)
    {
        return a.c.x + std::cos(a.a1) * a.r;
    }

    static inline double gcdl_arc_end_y(const GCDLArc& a)
    {
        return a.c.y + std::sin(a.a1) * a.r;
    }

    static inline void writeGCDLSVGPathFromPoints(
        FILE* fp,
        const GCDLEvalContext& ctx,
        const GCDLPath& path)
    {
        if (!fp || path.pointCount < 1)
            return;

        const GCDLPoint& p0 = ctx.points[path.firstPoint];

        std::fprintf(fp,
            "<path d=\"M %.6f %.6f ",
            p0.x, -p0.y);

        for (uint32_t i = 1; i < path.pointCount; i++) {
            const GCDLPoint& p = ctx.points[path.firstPoint + i];

            std::fprintf(fp,
                "L %.6f %.6f ",
                p.x, -p.y);
        }

        if (path.closed)
            std::fprintf(fp, "Z");

        std::fprintf(fp, "\"/>\n");
    }

    static inline bool writeGCDLSVG(
        const char* filename,
        const GCDLEvalContext& ctx)
    {
        FILE* fp = std::fopen(filename, "wb");
        if (!fp)
            return false;

        std::fprintf(fp,
            "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"-700 -700 1400 1400\">\n");

        std::fprintf(fp,
            "<g id=\"construction\" fill=\"none\" stroke=\"#999\" stroke-width=\"1\">\n");

        for (size_t i = 0; i < ctx.lines.size(); i++) {
            const GCDLLine& l = ctx.lines[i];

            std::fprintf(fp,
                "<line x1=\"%.6f\" y1=\"%.6f\" x2=\"%.6f\" y2=\"%.6f\"/>\n",
                l.a.x, -l.a.y,
                l.b.x, -l.b.y);
        }

        for (size_t i = 0; i < ctx.circles.size(); i++) {
            const GCDLCircle& c = ctx.circles[i];

            std::fprintf(fp,
                "<circle cx=\"%.6f\" cy=\"%.6f\" r=\"%.6f\"/>\n",
                c.c.x, -c.c.y, c.r);
        }

        std::fprintf(fp, "</g>\n");

        std::fprintf(fp,
            "<g id=\"points\" fill=\"#000\" stroke=\"none\">\n");

        for (size_t i = 0; i < ctx.points.size(); i++) {
            const GCDLPoint& p = ctx.points[i];

            std::fprintf(fp,
                "<circle cx=\"%.6f\" cy=\"%.6f\" r=\"4\"/>\n",
                p.x, -p.y);
        }

        std::fprintf(fp, "</g>\n");

        std::fprintf(fp,
            "<g id=\"finished\" fill=\"none\" stroke=\"#000\" stroke-width=\"6\">\n");

        for (size_t i = 0; i < ctx.paths.size(); i++) {
            writeGCDLSVGPathFromPoints(fp, ctx, ctx.paths[i]);
        }

        for (size_t i = 0; i < ctx.arcs.size(); i++) {
            const GCDLArc& a = ctx.arcs[i];

            const double x0 = gcdl_arc_start_x(a);
            const double y0 = gcdl_arc_start_y(a);
            const double x1 = gcdl_arc_end_x(a);
            const double y1 = gcdl_arc_end_y(a);

            const int largeArc = 0;
            const int sweep = a.ccw ? 0 : 1;

            std::fprintf(fp,
                "<path d=\"M %.6f %.6f A %.6f %.6f 0 %d %d %.6f %.6f\"/>\n",
                x0, -y0,
                a.r, a.r,
                largeArc,
                sweep,
                x1, -y1);
        }

        std::fprintf(fp, "</g>\n");
        std::fprintf(fp, "</svg>\n");

        std::fclose(fp);
        return true;
    }
}
