#pragma once

#include "pathprogram_builder.h"
#include "pathsegment.h"

namespace waavs
{
    // ------------------------------------------------------------
    // Normalizer: consumes SVG PathSegments and emits normalized ops
    // ------------------------------------------------------------

struct PathSegmentNormalizer
{
    PathProgramBuilder& b;

    // Current point + subpath start (in float for program space).
    float cx{ 0.0f }, cy{ 0.0f };
    float sx{ 0.0f }, sy{ 0.0f };
    bool  hasCP{ false };
    bool  subpathOpen{ false };

    // For smooth commands reflection
    bool  hasLastCubicCtrl{ false };
    float lastCubicCtrlX{ 0.0f }, lastCubicCtrlY{ 0.0f };

    bool  hasLastQuadCtrl{ false };
    float lastQuadCtrlX{ 0.0f }, lastQuadCtrlY{ 0.0f };

    // Track previous command kind (for smooth semantics)
    SVGPathCommand prevCmd{ SVGPathCommand::M };

    // If we want to mimic the BLPath behavior where after Z a non-M
    // needs an injected moveTo before drawing, keep this flag for the executor.
    // The program itself doesn't need it; the executor can track it.
    bool justClosed{ false };

    explicit PathSegmentNormalizer(PathProgramBuilder& builder) noexcept
        : b(builder) {
    }

    void resetState() noexcept
    {
        cx = cy = 0.0f;
        sx = sy = 0.0f;
        hasCP = false;
        subpathOpen = false;

        hasLastCubicCtrl = false;
        lastCubicCtrlX = lastCubicCtrlY = 0.0f;

        hasLastQuadCtrl = false;
        lastQuadCtrlX = lastQuadCtrlY = 0.0f;

        prevCmd = SVGPathCommand::M;
        justClosed = false;
    }

    void consume(const PathSegment& seg)
    {
        const SVGPathCommand cmd = seg.fSegmentKind;
        const float* a = seg.args();

        // Helpers
        auto ensureInitialRelMoveBase_ = [&]() noexcept {
            // SVG allows initial relative moveto. If no current point yet, base is (0,0).
            if (!hasCP) { cx = 0.0f; cy = 0.0f; hasCP = true; }
            };

        auto clearSmoothState_ = [&]() noexcept {
            // Spec: after any non-cubic command, cubic reflection becomes invalid, etc.
            hasLastCubicCtrl = false;
            hasLastQuadCtrl = false;
            };

        auto setCurrent_ = [&](float x, float y) noexcept {
            cx = x; cy = y; hasCP = true;
            };

        // MOVETO / m (with iteration rule)
        if (cmd == SVGPathCommand::M) {
            const float x = a[0], y = a[1];
            if (seg.iteration() == 0) {
                b.moveTo(x, y);
                sx = x; sy = y;
                setCurrent_(x, y);
                subpathOpen = true;
            }
            else {
                b.lineTo(x, y);
                setCurrent_(x, y);
            }
            clearSmoothState_();
            prevCmd = cmd;
            justClosed = false;
            return;
        }

        if (cmd == SVGPathCommand::m) {
            ensureInitialRelMoveBase_();
            const float x = cx + a[0];
            const float y = cy + a[1];

            if (seg.iteration() == 0) {
                b.moveTo(x, y);
                sx = x; sy = y;
                setCurrent_(x, y);
                subpathOpen = true;
            }
            else {
                b.lineTo(x, y);
                setCurrent_(x, y);
            }
            clearSmoothState_();
            prevCmd = cmd;
            justClosed = false;
            return;
        }

        // If we get here, we require a current point (except spec-allowed initial 'm' handled above).
        if (!hasCP) {
            // You can choose to assert or ignore.
            WAAVS_ASSERT(false && "PathProgramFromSegments: command before initial moveto");
            return;
        }

        // LINETO / l
        if (cmd == SVGPathCommand::L) {
            const float x = a[0], y = a[1];
            b.lineTo(x, y);
            setCurrent_(x, y);
            clearSmoothState_();
            prevCmd = cmd;
            justClosed = false;
            return;
        }
        if (cmd == SVGPathCommand::l) {
            const float x = cx + a[0], y = cy + a[1];
            b.lineTo(x, y);
            setCurrent_(x, y);
            clearSmoothState_();
            prevCmd = cmd;
            justClosed = false;
            return;
        }

        // H/V expand to lineTo
        if (cmd == SVGPathCommand::H) {
            const float x = a[0];
            b.lineTo(x, cy);
            setCurrent_(x, cy);
            clearSmoothState_();
            prevCmd = cmd;
            justClosed = false;
            return;
        }
        if (cmd == SVGPathCommand::h) {
            const float x = cx + a[0];
            b.lineTo(x, cy);
            setCurrent_(x, cy);
            clearSmoothState_();
            prevCmd = cmd;
            justClosed = false;
            return;
        }
        if (cmd == SVGPathCommand::V) {
            const float y = a[0];
            b.lineTo(cx, y);
            setCurrent_(cx, y);
            clearSmoothState_();
            prevCmd = cmd;
            justClosed = false;
            return;
        }
        if (cmd == SVGPathCommand::v) {
            const float y = cy + a[0];
            b.lineTo(cx, y);
            setCurrent_(cx, y);
            clearSmoothState_();
            prevCmd = cmd;
            justClosed = false;
            return;
        }

        // C/c
        if (cmd == SVGPathCommand::C) {
            const float x1 = a[0], y1 = a[1], x2 = a[2], y2 = a[3], x = a[4], y = a[5];
            b.cubicTo(x1, y1, x2, y2, x, y);
            setCurrent_(x, y);

            hasLastCubicCtrl = true;
            lastCubicCtrlX = x2; lastCubicCtrlY = y2;
            hasLastQuadCtrl = false;
            prevCmd = cmd;
            justClosed = false;
            return;
        }
        if (cmd == SVGPathCommand::c) {
            const float x1 = cx + a[0], y1 = cy + a[1], x2 = cx + a[2], y2 = cy + a[3], x = cx + a[4], y = cy + a[5];
            b.cubicTo(x1, y1, x2, y2, x, y);
            setCurrent_(x, y);

            hasLastCubicCtrl = true;
            lastCubicCtrlX = x2; lastCubicCtrlY = y2;
            hasLastQuadCtrl = false;
            prevCmd = cmd;
            justClosed = false;
            return;
        }

        // S/s expand to cubic using reflection of previous cubic control point
        if (cmd == SVGPathCommand::S || cmd == SVGPathCommand::s) {
            float x2, y2, x, y;

            if (cmd == SVGPathCommand::S) {
                x2 = a[0]; y2 = a[1]; x = a[2]; y = a[3];
            }
            else {
                x2 = cx + a[0]; y2 = cy + a[1]; x = cx + a[2]; y = cy + a[3];
            }

            float x1 = cx;
            float y1 = cy;

            // Reflection only valid if previous command was C/c/S/s
            if (hasLastCubicCtrl &&
                (prevCmd == SVGPathCommand::C || prevCmd == SVGPathCommand::c ||
                    prevCmd == SVGPathCommand::S || prevCmd == SVGPathCommand::s))
            {
                x1 = 2.0f * cx - lastCubicCtrlX;
                y1 = 2.0f * cy - lastCubicCtrlY;
            }

            b.cubicTo(x1, y1, x2, y2, x, y);
            setCurrent_(x, y);

            hasLastCubicCtrl = true;
            lastCubicCtrlX = x2; lastCubicCtrlY = y2;
            hasLastQuadCtrl = false;
            prevCmd = cmd;
            justClosed = false;
            return;
        }

        // Q/q
        if (cmd == SVGPathCommand::Q) {
            const float x1 = a[0], y1 = a[1], x = a[2], y = a[3];
            b.quadTo(x1, y1, x, y);
            setCurrent_(x, y);

            hasLastQuadCtrl = true;
            lastQuadCtrlX = x1; lastQuadCtrlY = y1;
            hasLastCubicCtrl = false;
            prevCmd = cmd;
            justClosed = false;
            return;
        }
        if (cmd == SVGPathCommand::q) {
            const float x1 = cx + a[0], y1 = cy + a[1], x = cx + a[2], y = cy + a[3];
            b.quadTo(x1, y1, x, y);
            setCurrent_(x, y);

            hasLastQuadCtrl = true;
            lastQuadCtrlX = x1; lastQuadCtrlY = y1;
            hasLastCubicCtrl = false;
            prevCmd = cmd;
            justClosed = false;
            return;
        }

        // T/t expand to quad using reflection of previous quad control point
        if (cmd == SVGPathCommand::T || cmd == SVGPathCommand::t) {
            float x, y;
            if (cmd == SVGPathCommand::T) {
                x = a[0]; y = a[1];
            }
            else {
                x = cx + a[0]; y = cy + a[1];
            }

            float x1 = cx;
            float y1 = cy;

            // Reflection only valid if previous command was Q/q/T/t
            if (hasLastQuadCtrl &&
                (prevCmd == SVGPathCommand::Q || prevCmd == SVGPathCommand::q ||
                    prevCmd == SVGPathCommand::T || prevCmd == SVGPathCommand::t))
            {
                x1 = 2.0f * cx - lastQuadCtrlX;
                y1 = 2.0f * cy - lastQuadCtrlY;
            }

            b.quadTo(x1, y1, x, y);
            setCurrent_(x, y);

            hasLastQuadCtrl = true;
            lastQuadCtrlX = x1; lastQuadCtrlY = y1;
            hasLastCubicCtrl = false;
            prevCmd = cmd;
            justClosed = false;
            return;
        }

        // A/a (keep endpoint form; normalize relative)
        if (cmd == SVGPathCommand::A || cmd == SVGPathCommand::a) {
            float rx = a[0];
            float ry = a[1];
            float xrot = a[2];
            float largeArc = a[3];
            float sweep = a[4];
            float x, y;

            if (cmd == SVGPathCommand::A) {
                x = a[5]; y = a[6];
            }
            else {
                x = cx + a[5]; y = cy + a[6];
            }

            b.arcTo(rx, ry, xrot, largeArc, sweep, x, y);
            setCurrent_(x, y);

            clearSmoothState_();
            prevCmd = cmd;
            justClosed = false;
            return;
        }

        // Z/z
        if (cmd == SVGPathCommand::Z || cmd == SVGPathCommand::z) {
            b.close();

            // SVG: current point becomes subpath start after close
            setCurrent_(sx, sy);
            subpathOpen = false;

            clearSmoothState_();
            prevCmd = cmd;
            justClosed = true;
            return;
        }

        // Unknown
        WAAVS_ASSERT(false && "PathProgramFromSegments: unknown command");
    }
};
}