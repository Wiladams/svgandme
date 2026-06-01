#pragma once

#include "xmlscan.h"
#include "converters.h"

#include "gcdl_program.h"

namespace waavs
{
    static constexpr charset chrIdentStartChars(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_");

    static constexpr charset chrIdentChars(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-");

    // ---------------------------------------------------------
    // Attribute helpers
    // ---------------------------------------------------------

    static bool gcdl_attr(
        const XmlElement& elem,
        const char* name,
        ByteSpan& value)
    {
        return elem.getElementAttribute(ByteSpan(name), value);
    }

    static bool parseInternedAttr(
        const XmlElement& elem,
        const char* name,
        InternedKey& value)
    {
        ByteSpan span;
        if (!gcdl_attr(elem, name, span))
            return false;

        value = PSNameTable::INTERN(span);
        return value != nullptr;
    }

    static bool parseDoubleAttr(
        const XmlElement& elem,
        const char* name,
        double& value)
    {
        ByteSpan span;
        if (!gcdl_attr(elem, name, span))
            return false;

        return parseNumber(span, value);
    }

    static bool parseUInt32Attr(
        const XmlElement& elem,
        const char* name,
        uint32_t& value)
    {
        ByteSpan span;
        if (!gcdl_attr(elem, name, span))
            return false;

        uint64_t tmp = 0;
        if (!parse64u(span, tmp))
            return false;

        value = (uint32_t)tmp;
        return true;
    }

    static bool parseBoolAttrDefault(
        const XmlElement& elem,
        const char* name,
        bool defaultValue,
        bool& value)
    {
        ByteSpan span;
        if (!gcdl_attr(elem, name, span)) {
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

    // ---------------------------------------------------------
    // GeoRef
    // ---------------------------------------------------------

    static bool parseGeoRef(ByteSpan src, GeoRef& out)
    {
        bspan_skip_spaces(src);

        if (!src || !chrIdentStartChars(*src))
            return false;

        ByteSpan name = bspan_read_prefix(src, chrIdentChars);
        if (!name)
            return false;

        out.id = PSNameTable::INTERN(name);
        out.slot = 0;

        bspan_skip_spaces(src);

        if (src && *src == '[')
        {
            ByteSpan bracketed =
                chunk_read_bracketed(src, '[', ']');

            if (!bracketed)
                return false;

            uint64_t slot = 0;
            if (!parse64u(bracketed, slot))
                return false;

            out.slot = (uint32_t)slot;
        }

        bspan_skip_spaces(src);

        return !src;
    }

    static bool parseGeoRefAttr(
        const XmlElement& elem,
        const char* name,
        GeoRef& ref)
    {
        ByteSpan span;
        if (!gcdl_attr(elem, name, span))
            return false;

        return parseGeoRef(span, ref);
    }

    // ---------------------------------------------------------
    // Element matching
    // ---------------------------------------------------------

    static bool gcdl_name_equals(
        const XmlElement& elem,
        const char* name)
    {
        return elem.nameAtom() ==
            PSNameTable::INTERN(name);
    }

    static bool gcdl_is_node(
        const XmlElement& elem,
        const char* name)
    {
        return (elem.isStart() || elem.isSelfClosing()) &&
            gcdl_name_equals(elem, name);
    }

    static bool gcdl_is_end(
        const XmlElement& elem,
        const char* name)
    {
        return elem.isEnd() &&
            gcdl_name_equals(elem, name);
    }

    // ---------------------------------------------------------
    // Node parsers
    // ---------------------------------------------------------

    static bool parsePointNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = PSNameTable::INTERN("point.xy");

        double x = 0.0;
        double y = 0.0;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseDoubleAttr(elem, "x", x))
            return false;

        if (!parseDoubleAttr(elem, "y", y))
            return false;

        n.nums.push_back(x);
        n.nums.push_back(y);

        return prog.addNode(n);
    }

    static bool parsePointPickMode(ByteSpan span, double& out)
    {
        if (span == "first") {
            out = (double)GCDLPointPickMode::First;
            return true;
        }

        if (span == "upper") {
            out = (double)GCDLPointPickMode::Upper;
            return true;
        }

        if (span == "lower") {
            out = (double)GCDLPointPickMode::Lower;
            return true;
        }

        if (span == "left") {
            out = (double)GCDLPointPickMode::Left;
            return true;
        }

        if (span == "right") {
            out = (double)GCDLPointPickMode::Right;
            return true;
        }

        return false;
    }

    static bool parsePointPickNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = PSNameTable::INTERN("point.pick");

        GeoRef from;
        ByteSpan choose;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "from", from))
            return false;

        if (!gcdl_attr(elem, "choose", choose))
            return false;

        double mode = 0.0;
        if (!parsePointPickMode(choose, mode))
            return false;

        n.refs.push_back(from);
        n.nums.push_back(mode);

        return prog.addNode(n);
    }


    static bool parseLineNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = PSNameTable::INTERN("line.fromPoints");

        GeoRef a;
        GeoRef b;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "from", a))
            return false;

        if (!parseGeoRefAttr(elem, "to", b))
            return false;

        n.refs.push_back(a);
        n.refs.push_back(b);

        return prog.addNode(n);
    }

    static bool parseLinePerpendicularNode(const XmlElement& elem, GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = GCDLOp_line_perpendicular();

        GeoRef line;
        GeoRef point;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "line", line))
            return false;

        if (!parseGeoRefAttr(elem, "point", point))
            return false;

        n.refs.push_back(line);
        n.refs.push_back(point);

        return prog.addNode(n);
    }

    static bool parseDivideNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = PSNameTable::INTERN("line.divide");

        GeoRef line;
        uint32_t parts = 0;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "line", line))
            return false;

        if (!parseUInt32Attr(elem, "parts", parts))
            return false;

        n.refs.push_back(line);
        n.nums.push_back((double)parts);

        return prog.addNode(n);
    }

    static bool parseCircleNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = PSNameTable::INTERN("circle.centerPoint");

        GeoRef center;
        GeoRef through;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "center", center))
            return false;

        if (!parseGeoRefAttr(elem, "through", through))
            return false;

        n.refs.push_back(center);
        n.refs.push_back(through);

        return prog.addNode(n);
    }

    static bool parseCircleCenterRadiusNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = GCDLOp_circle_center_radius();

        GeoRef center;
        double radius = 0.0;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "center", center))
            return false;

        if (!parseDoubleAttr(elem, "radius", radius))
            return false;

        n.refs.push_back(center);
        n.nums.push_back(radius);

        return prog.addNode(n);
    }

    static bool parseIntersectNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        ByteSpan kind;
        if (!gcdl_attr(elem, "kind", kind))
            return false;

        GCDLNode n;

        if (kind == "circleCircle") {
            n.op = GCDLOp_intersect_circle_circle();

            GeoRef a;
            GeoRef b;

            if (!parseInternedAttr(elem, "id", n.id))
                return false;

            if (!parseGeoRefAttr(elem, "a", a))
                return false;

            if (!parseGeoRefAttr(elem, "b", b))
                return false;

            n.refs.push_back(a);
            n.refs.push_back(b);

            return prog.addNode(n);
        }

        if (kind == "lineLine") {
            n.op = GCDLOp_intersect_line_line();

            GeoRef a;
            GeoRef b;

            if (!parseInternedAttr(elem, "id", n.id))
                return false;

            if (!parseGeoRefAttr(elem, "a", a))
                return false;

            if (!parseGeoRefAttr(elem, "b", b))
                return false;

            n.refs.push_back(a);
            n.refs.push_back(b);

            return prog.addNode(n);
        }

        if (kind == "lineCircle") {
            n.op = GCDLOp_intersect_line_circle();

            GeoRef line;
            GeoRef circle;

            if (!parseInternedAttr(elem, "id", n.id))
                return false;

            if (!parseGeoRefAttr(elem, "line", line))
                return false;

            if (!parseGeoRefAttr(elem, "circle", circle))
                return false;

            n.refs.push_back(line);
            n.refs.push_back(circle);

            return prog.addNode(n);
        }

        return false;
    }

    static bool parseArcNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = PSNameTable::INTERN("arc.circleFromTo");

        GeoRef circle;
        GeoRef from;
        GeoRef to;

        bool ccw = true;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "circle", circle))
            return false;

        if (!parseGeoRefAttr(elem, "from", from))
            return false;

        if (!parseGeoRefAttr(elem, "to", to))
            return false;

        if (!parseBoolAttrDefault(elem, "ccw", true, ccw))
            return false;

        n.refs.push_back(circle);
        n.refs.push_back(from);
        n.refs.push_back(to);

        n.nums.push_back(ccw ? 1.0 : 0.0);

        return prog.addNode(n);
    }

    static bool parseArcCenterAnglesNode( const XmlElement& elem, GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = GCDLOp_arc_center_angles();

        GeoRef center;
        double radius = 0.0;
        double startAngle = 0.0;
        double endAngle = 0.0;
        bool ccw = true;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "center", center))
            return false;

        if (!parseDoubleAttr(elem, "radius", radius))
            return false;

        if (!parseDoubleAttr(elem, "start-angle", startAngle))
            return false;

        if (!parseDoubleAttr(elem, "end-angle", endAngle))
            return false;

        if (!parseBoolAttrDefault(elem, "ccw", true, ccw))
            return false;

        n.refs.push_back(center);
        n.nums.push_back(radius);
        n.nums.push_back(startAngle);
        n.nums.push_back(endAngle);
        n.nums.push_back(ccw ? 1.0 : 0.0);

        return prog.addNode(n);
    }

    static bool parseMirrorPointNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = PSNameTable::INTERN("mirror.point");

        GeoRef point;
        GeoRef axis;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "point", point))
            return false;

        if (!parseGeoRefAttr(elem, "axis", axis))
            return false;

        n.refs.push_back(point);
        n.refs.push_back(axis);

        return prog.addNode(n);
    }

    static bool parseMirrorArcNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = PSNameTable::INTERN("mirror.arc");

        GeoRef arc;
        GeoRef axis;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "arc", arc))
            return false;

        if (!parseGeoRefAttr(elem, "axis", axis))
            return false;

        n.refs.push_back(arc);
        n.refs.push_back(axis);

        return prog.addNode(n);
    }

    static bool parsePointPolarNode(const XmlElement& elem,GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = PSNameTable::INTERN("point.polar");

        GeoRef center;
        double radius = 0.0;
        double angle = 0.0;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "center", center))
            return false;

        if (!parseDoubleAttr(elem, "radius", radius))
            return false;

        if (!parseDoubleAttr(elem, "angle", angle))
            return false;

        n.refs.push_back(center);
        n.nums.push_back(radius);
        n.nums.push_back(angle);

        return prog.addNode(n);
    }

    static bool parsePointProjectNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = GCDLOp_point_project();

        GeoRef point;
        GeoRef line;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "point", point))
            return false;

        if (!parseGeoRefAttr(elem, "line", line))
            return false;

        n.refs.push_back(point);
        n.refs.push_back(line);

        return prog.addNode(n);
    }

    
    static bool parsePointAlongLineNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = GCDLOp_point_along_line();

        GeoRef line;
        double t = 0.0;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "line", line))
            return false;

        if (!parseDoubleAttr(elem, "t", t))
            return false;

        n.refs.push_back(line);
        n.nums.push_back(t);

        return prog.addNode(n);
    }
    
    static bool parseLineFromPointAngleNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = GCDLOp_line_from_point_angle();

        GeoRef point;
        double angle = 0.0;
        double length = 1000.0;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "point", point))
            return false;

        if (!parseDoubleAttr(elem, "angle", angle))
            return false;

        // Optional. Defaults to 1000 user units.
        parseDoubleAttr(elem, "length", length);

        n.refs.push_back(point);
        n.nums.push_back(angle);
        n.nums.push_back(length);

        return prog.addNode(n);
    }

    static bool parseLineParallelNode( const XmlElement& elem, GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = GCDLOp_line_parallel();

        GeoRef line;
        GeoRef point;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "line", line))
            return false;

        if (!parseGeoRefAttr(elem, "point", point))
            return false;

        n.refs.push_back(line);
        n.refs.push_back(point);

        return prog.addNode(n);
    }


    static bool parsePointRotateNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = GCDLOp_point_rotate();

        GeoRef point;
        GeoRef center;
        double angle = 0.0;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "point", point))
            return false;

        if (!parseGeoRefAttr(elem, "center", center))
            return false;

        if (!parseDoubleAttr(elem, "angle", angle))
            return false;

        n.refs.push_back(point);
        n.refs.push_back(center);
        n.nums.push_back(angle);

        return prog.addNode(n);
    }

    static bool parseLineRotateNode(const XmlElement& elem, GCDLProgram& prog)
    {
        GCDLNode n;
        n.op = GCDLOp_line_rotate();

        GeoRef line;
        GeoRef center;
        double angle = 0.0;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "line", line))
            return false;

        if (!parseGeoRefAttr(elem, "center", center))
            return false;

        if (!parseDoubleAttr(elem, "angle", angle))
            return false;

        n.refs.push_back(line);
        n.refs.push_back(center);
        n.nums.push_back(angle);

        return prog.addNode(n);
    }

    static bool parsePolygonRegularNode( const XmlElement& elem, GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = GCDLOp_polygon_regular();

        GeoRef center;
        double radius = 0.0;
        uint32_t sides = 0;
        double startAngle = 0.0;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "center", center))
            return false;

        if (!parseDoubleAttr(elem, "radius", radius))
            return false;

        if (!parseUInt32Attr(elem, "sides", sides))
            return false;

        parseDoubleAttr(elem, "start-angle", startAngle);

        n.refs.push_back(center);
        n.nums.push_back(radius);
        n.nums.push_back((double)sides);
        n.nums.push_back(startAngle);

        return prog.addNode(n);
    }

    static bool parsePolygonStarNode( const XmlElement& elem, GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = GCDLOp_polygon_star();

        GeoRef center;
        double radius = 0.0;
        uint32_t points = 0;
        uint32_t skip = 0;
        double startAngle = 0.0;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "center", center))
            return false;

        if (!parseDoubleAttr(elem, "radius", radius))
            return false;

        if (!parseUInt32Attr(elem, "points", points))
            return false;

        if (!parseUInt32Attr(elem, "skip", skip))
            return false;

        parseDoubleAttr(elem, "start-angle", startAngle);

        n.refs.push_back(center);
        n.nums.push_back(radius);
        n.nums.push_back((double)points);
        n.nums.push_back((double)skip);
        n.nums.push_back(startAngle);

        return prog.addNode(n);
    }


    static bool parsePathFromPointsNode(
        const XmlElement& elem,
        GCDLProgram& prog)
    {
        GCDLNode n;

        n.op = GCDLOp_path_from_points();

        GeoRef points;
        bool closed = true;

        if (!parseInternedAttr(elem, "id", n.id))
            return false;

        if (!parseGeoRefAttr(elem, "points", points))
            return false;

        if (!parseBoolAttrDefault(elem, "closed", true, closed))
            return false;

        n.refs.push_back(points);
        n.nums.push_back(closed ? 1.0 : 0.0);

        return prog.addNode(n);
    }



    // ---------------------------------------------------------
    // Dispatcher
    // ---------------------------------------------------------
    
    // Generic signature for node parsing functions. 
    // Returns true on success, false on failure.
    using GCDLNodeParserFunc = bool (*)(const XmlElement& elem, GCDLProgram& prog);
    using GCDLNodeParserMap =
        std::unordered_map<InternedKey, GCDLNodeParserFunc,
        InternedKeyHash, InternedKeyEquivalent>;
    
    static const GCDLNodeParserMap& gcdlNodeParserMap()
    {
        static const GCDLNodeParserMap map = {
            { GCDLElem_arc(), parseArcNode },
            { GCDLElem_circle(), parseCircleNode },
            { GCDLElem_circle_center_radius(), parseCircleCenterRadiusNode},
            { GCDLElem_divide(), parseDivideNode },
            { GCDLElem_intersect(), parseIntersectNode },
            { GCDLElem_line(), parseLineNode },
            { GCDLElem_point_along_line(), parsePointAlongLineNode },
            { GCDLElem_line_from_point_angle(), parseLineFromPointAngleNode },
            { GCDLElem_line_parallel(), parseLineParallelNode },
            { GCDLElem_line_perpendicular(), parseLinePerpendicularNode },
            { GCDLElem_line_rotate(), parseLineRotateNode },
            { GCDLElem_mirror_arc(), parseMirrorArcNode },
            { GCDLElem_mirror_point(), parseMirrorPointNode },
            { GCDLElem_path_from_points(), parsePathFromPointsNode },
            { GCDLElem_point(), parsePointNode },
            { GCDLElem_point_polar(), parsePointPolarNode },
            { GCDLElem_point_pick(), parsePointPickNode },
            { GCDLElem_point_project(), parsePointProjectNode},
            { GCDLElem_point_rotate(), parsePointRotateNode },
            { GCDLElem_polygon_regular(), parsePolygonRegularNode },
            { GCDLElem_polygon_star(), parsePolygonStarNode },

        };

        return map;
    }

    static bool parseGCDLNodeElement( const XmlElement& elem, GCDLProgram& prog)
    {
        if (!elem.isStart() && !elem.isSelfClosing())
            return true;

        InternedKey name = elem.nameAtom();
        if (!name)
            return true;

        const GCDLNodeParserMap& map = gcdlNodeParserMap();

        auto it = map.find(name);
        if (it == map.end())
            return true;

        return it->second(elem, prog);
    }



    // ---------------------------------------------------------
    // Document parser
    // ---------------------------------------------------------

    static bool parseGCDLDocument(
        ByteSpan src,
        GCDLProgram& prog)
    {
        prog.clear();

        XmlPull pull(src);

        bool inGCDL = false;
        bool inNodes = false;

        while (pull.next())
        {
            const XmlElement& elem = *pull;

            if (!inGCDL)
            {
                if (gcdl_is_node(elem, "gcdl"))
                    inGCDL = true;

                continue;
            }

            if (gcdl_is_end(elem, "gcdl"))
                return true;

            if (gcdl_is_node(elem, "nodes"))
            {
                inNodes = true;
                continue;
            }

            if (gcdl_is_end(elem, "nodes"))
            {
                inNodes = false;
                continue;
            }

            if (inNodes)
            {
                if (!parseGCDLNodeElement(elem, prog))
                    return false;
            }
        }

        return inGCDL;
    }
}
