#pragma once

#include "gcdl_program.h"

#include <unordered_map>

namespace waavs
{
    struct GCDLGeom {
        GCDLGeomKind kind = GCDLGeomKind::None;
        uint32_t first = 0;
        uint32_t count = 0;
    };

    struct GCDLEvalContext {
        std::vector<GCDLPoint> points;
        std::vector<GCDLLine> lines;
        std::vector<GCDLCircle> circles;
        std::vector<GCDLArc> arcs;
        std::vector<GCDLPath> paths;

        std::unordered_map<InternedKey, GCDLGeom> symbols;

        void clear()
        {
            points.clear();
            lines.clear();
            circles.clear();
            arcs.clear();
            paths.clear();
            symbols.clear();
        }

        // ---------------------------------
        // Storing geometry by reference
        //
        bool putPoint(InternedKey id, const GCDLPoint& p)
        {
            if (!id)
                return false;

            GCDLGeom g;
            g.kind = GCDLGeomKind::Point;
            g.first = (uint32_t)points.size();
            g.count = 1;

            points.push_back(p);
            symbols[id] = g;
            return true;
        }

        bool putLine(InternedKey id, const GCDLLine& l)
        {
            if (!id)
                return false;

            GCDLGeom g;
            g.kind = GCDLGeomKind::Line;
            g.first = (uint32_t)lines.size();
            g.count = 1;

            lines.push_back(l);
            symbols[id] = g;
            return true;
        }

        bool putCircle(InternedKey id, const GCDLCircle& c)
        {
            if (!id)
                return false;

            GCDLGeom g;
            g.kind = GCDLGeomKind::Circle;
            g.first = (uint32_t)circles.size();
            g.count = 1;

            circles.push_back(c);
            symbols[id] = g;
            return true;
        }

        bool putArc(InternedKey id, const GCDLArc& a)
        {
            if (!id)
                return false;

            GCDLGeom g;
            g.kind = GCDLGeomKind::Arc;
            g.first = (uint32_t)arcs.size();
            g.count = 1;

            arcs.push_back(a);
            symbols[id] = g;
            return true;
        }

        bool putPath(InternedKey id, const GCDLPath& p)
        {
            if (!id)
                return false;

            GCDLGeom g;
            g.kind = GCDLGeomKind::Path;
            g.first = (uint32_t)paths.size();
            g.count = 1;

            paths.push_back(p);
            symbols[id] = g;
            return true;
        }


        // ---------------------------------
        // Retrieving geometry by reference

        bool getPoint(const GeoRef& ref, GCDLPoint& out) const
        {
            auto it = symbols.find(ref.id);
            if (it == symbols.end())
                return false;

            const GCDLGeom& g = it->second;
            if (g.kind != GCDLGeomKind::Point && g.kind != GCDLGeomKind::PointSet)
                return false;

            if (ref.slot >= g.count)
                return false;

            out = points[g.first + ref.slot];
            return true;
        }

        bool getLine(const GeoRef& ref, GCDLLine& out) const
        {
            auto it = symbols.find(ref.id);
            if (it == symbols.end())
                return false;

            const GCDLGeom& g = it->second;
            if (g.kind != GCDLGeomKind::Line || ref.slot >= g.count)
                return false;

            out = lines[g.first + ref.slot];
            return true;
        }

        bool getCircle(const GeoRef& ref, GCDLCircle& out) const
        {
            auto it = symbols.find(ref.id);
            if (it == symbols.end())
                return false;

            const GCDLGeom& g = it->second;
            if (g.kind != GCDLGeomKind::Circle || ref.slot >= g.count)
                return false;

            out = circles[g.first + ref.slot];
            return true;
        }

        bool getArc(const GeoRef& ref, GCDLArc& out) const
        {
            auto it = symbols.find(ref.id);
            if (it == symbols.end())
                return false;

            const GCDLGeom& g = it->second;
            if (g.kind != GCDLGeomKind::Arc || ref.slot >= g.count)
                return false;

            out = arcs[g.first + ref.slot];
            return true;
        }

        bool getPath(const GeoRef& ref, GCDLPath& out) const
        {
            auto it = symbols.find(ref.id);
            if (it == symbols.end())
                return false;

            const GCDLGeom& g = it->second;
            if (g.kind != GCDLGeomKind::Path || ref.slot >= g.count)
                return false;

            out = paths[g.first + ref.slot];
            return true;
        }
    };

}
