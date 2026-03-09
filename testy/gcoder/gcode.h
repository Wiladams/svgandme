#pragma once

#include "curves.h"
#include "pathsegmenter.h"

namespace waavs {
    void emitGcodeFromPath(ByteSpan d) {
        waavs::SVGSegmentParseParams params;
        waavs::SVGSegmentParseState state{ d };

        Point current{ 0, 0 };
        Point startOfSubpath{ 0, 0 };
        Point lastCtrlPoint{ 0, 0 };
        bool lastWasCurve = false;

        while (waavs::readNextSegmentCommand(params, state)) {
            char cmd = state.fSegmentKind;
            bool rel = (cmd >= 'a' && cmd <= 'z');

            switch (cmd) {
            case 'M': case 'm': {
                double x = state.args[0];
                double y = state.args[1];
                if (rel) { x += current.x; y += current.y; }
                current = startOfSubpath = { x, y };
                printf("G0 X%.3f Y%.3f\n", x, -y);
                lastWasCurve = false;
                break;
            }
            case 'L': case 'l': {
                double x = state.args[0];
                double y = state.args[1];
                if (rel) { x += current.x; y += current.y; }
                current = { x, y };
                printf("G1 X%.3f Y%.3f\n", x, -y);
                lastWasCurve = false;
                break;
            }
            case 'H': case 'h': {
                double x = state.args[0];
                if (rel) x += current.x;
                current.x = x;
                printf("G1 X%.3f Y%.3f\n", x, -current.y);
                lastWasCurve = false;
                break;
            }
            case 'V': case 'v': {
                double y = state.args[0];
                if (rel) y += current.y;
                current.y = y;
                printf("G1 X%.3f Y%.3f\n", current.x, -y);
                lastWasCurve = false;
                break;
            }
            case 'Z': case 'z': {
                printf("G1 X%.3f Y%.3f ; close path\n", startOfSubpath.x, -startOfSubpath.y);
                current = startOfSubpath;
                lastWasCurve = false;
                break;
            }
            case 'A': case 'a': {
                double rx = state.args[0];
                double ry = state.args[1];
                double angle = state.args[2];
                bool largeArc = state.args[3] != 0;
                bool sweep = state.args[4] != 0;
                double x = state.args[5];
                double y = state.args[6];
                if (rel) { x += current.x; y += current.y; }
                Point target = { x, y };
                ArcSegmentGenerator gen(current, target, rx, ry, angle, largeArc, sweep);
                Point pt; double t;
                while (gen.next(pt, t))
                    printf("G1 X%.3f Y%.3f ; t=%.2f\n", pt.x, -pt.y, t);
                current = target;
                lastWasCurve = false;
                break;
            }
            case 'C': case 'c': {
                Point p1 = { state.args[0], state.args[1] };
                Point p2 = { state.args[2], state.args[3] };
                Point p3 = { state.args[4], state.args[5] };
                if (rel) {
                    p1.x += current.x; p1.y += current.y;
                    p2.x += current.x; p2.y += current.y;
                    p3.x += current.x; p3.y += current.y;
                }
                CubicBezierSegmentGenerator gen(current, p1, p2, p3);
                Point pt; double t;
                while (gen.next(pt, t))
                    printf("G1 X%.3f Y%.3f ; t=%.2f\n", pt.x, -pt.y, t);
                lastCtrlPoint = p2;
                current = p3;
                lastWasCurve = true;
                break;
            }
            case 'S': case 's': {
                Point p2 = { state.args[0], state.args[1] };
                Point p3 = { state.args[2], state.args[3] };
                if (rel) {
                    p2.x += current.x; p2.y += current.y;
                    p3.x += current.x; p3.y += current.y;
                }
                Point p1 = lastWasCurve ? Point{ 2 * current.x - lastCtrlPoint.x, 2 * current.y - lastCtrlPoint.y } : current;
                CubicBezierSegmentGenerator gen(current, p1, p2, p3);
                Point pt; double t;
                while (gen.next(pt, t))
                    printf("G1 X%.3f Y%.3f ; t=%.2f\n", pt.x, -pt.y, t);
                lastCtrlPoint = p2;
                current = p3;
                lastWasCurve = true;
                break;
            }
            case 'Q': case 'q': {
                Point p1 = { state.args[0], state.args[1] };
                Point p2 = { state.args[2], state.args[3] };
                if (rel) {
                    p1.x += current.x; p1.y += current.y;
                    p2.x += current.x; p2.y += current.y;
                }
                QuadraticBezierSegmentGenerator gen(current, p1, p2);
                Point pt; double t;
                while (gen.next(pt, t))
                    printf("G1 X%.3f Y%.3f ; t=%.2f\n", pt.x, -pt.y, t);
                lastCtrlPoint = p1;
                current = p2;
                lastWasCurve = true;
                break;
            }
            case 'T': case 't': {
                Point p2 = { state.args[0], state.args[1] };
                if (rel) {
                    p2.x += current.x; p2.y += current.y;
                }
                Point p1 = lastWasCurve ? Point{ 2 * current.x - lastCtrlPoint.x, 2 * current.y - lastCtrlPoint.y } : current;
                QuadraticBezierSegmentGenerator gen(current, p1, p2);
                Point pt; double t;
                while (gen.next(pt, t))
                    printf("G1 X%.3f Y%.3f ; t=%.2f\n", pt.x, -pt.y, t);
                lastCtrlPoint = p1;
                current = p2;
                lastWasCurve = true;
                break;
            }
            default:
                printf("; Unsupported command: %c\n", cmd);
                lastWasCurve = false;
                break;
            }
        }
    }
}
