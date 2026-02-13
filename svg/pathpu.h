#pragma once

#include <vector>
#include <cmath>

#include "definitions.h"
#include "waavsgraph.h"     // PathSegment


//=================================
// BBox helpers
//=================================
namespace waavs
{
    static INLINE void bboxInit(double &minX, double &minY, 
        double &maxX, double &maxY, 
        double x, double y) noexcept
    {
        minX = maxX = x;
        minY = maxY = y;

        //minX = minY = std::numeric_limits<double>::infinity();
        //maxX = maxY = -std::numeric_limits<double>::infinity();
    }

    static INLINE void bboxExpand(double &minX, double &minY, 
        double &maxX, double &maxY, 
        double x, double y) noexcept
    {
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }

    static INLINE void quadBounds(
        double x0, double y0,
        double x1, double y1,
        double x2, double y2,
        double& minX, double& minY,
        double& maxX, double& maxY)
    {
        bboxExpand(minX, minY, maxX, maxY, x2, y2);

        // X extrema
        double dx = x0 - 2.0 * x1 + x2;
        if (dx != 0.0) {
            double t = (x0 - x1) / dx;
            if (t > 0.0 && t < 1.0) {
                double mt = 1.0 - t;
                double x = mt * mt * x0 + 2 * mt * t * x1 + t * t * x2;
                double y = mt * mt * y0 + 2 * mt * t * y1 + t * t * y2;
                bboxExpand(minX, minY, maxX, maxY, x, y);
            }
        }

        // Y extrema
        double dy = y0 - 2.0 * y1 + y2;
        if (dy != 0.0) {
            double t = (y0 - y1) / dy;
            if (t > 0.0 && t < 1.0) {
                double mt = 1.0 - t;
                double x = mt * mt * x0 + 2 * mt * t * x1 + t * t * x2;
                double y = mt * mt * y0 + 2 * mt * t * y1 + t * t * y2;
                bboxExpand(minX, minY, maxX, maxY, x, y);
            }
        }
    }

    static inline void cubicBounds(
        double x0, double y0,
        double x1, double y1,
        double x2, double y2,
        double x3, double y3,
        double& minX, double& minY,
        double& maxX, double& maxY)
    {
        bboxExpand(minX, minY, maxX, maxY, x3, y3);

        auto solve = [&](double p0, double p1, double p2, double p3,
            bool isX)
            {
                double a = -p0 + 3 * p1 - 3 * p2 + p3;
                double b = 2 * (p0 - 2 * p1 + p2);
                double c = -p0 + p1;

                if (std::abs(a) < 1e-12) {
                    if (std::abs(b) < 1e-12) return;
                    double t = -c / b;
                    if (t > 0.0 && t < 1.0) {
                        double mt = 1.0 - t;
                        double x = mt * mt * mt * x0 + 3 * mt * mt * t * x1 + 3 * mt * t * t * x2 + t * t * t * x3;
                        double y = mt * mt * mt * y0 + 3 * mt * mt * t * y1 + 3 * mt * t * t * y2 + t * t * t * y3;
                        bboxExpand(minX, minY, maxX, maxY, x, y);
                    }
                    return;
                }

                double disc = b * b - 4 * a * c;
                if (disc < 0.0) return;

                double s = std::sqrt(disc);
                double t1 = (-b + s) / (2 * a);
                double t2 = (-b - s) / (2 * a);

                auto eval = [&](double t) {
                    if (t > 0.0 && t < 1.0) {
                        double mt = 1.0 - t;
                        double x = mt * mt * mt * x0 + 3 * mt * mt * t * x1 + 3 * mt * t * t * x2 + t * t * t * x3;
                        double y = mt * mt * mt * y0 + 3 * mt * mt * t * y1 + 3 * mt * t * t * y2 + t * t * t * y3;
                        bboxExpand(minX, minY, maxX, maxY, x, y);
                    }
                    };

                eval(t1);
                eval(t2);
            };

        solve(x0, x1, x2, x3, true);
        solve(y0, y1, y2, y3, false);
    }




}

// Machinery for a PathProgram
// That represents a sequence of path operations
namespace waavs
{
	// These ops represent normalized commands.  That means...
	//	- no relative commands exist
    //	- no implicit lineto after moveto
	//	- arcs are already endpoint-form
	//	- smooth curves are expanded

	enum PathOp : uint8_t {
		OP_END = 0,		// Reprsesent the end of the path program (not Z / close
		OP_MOVETO,
		OP_LINETO,
		OP_CUBICTO,
		OP_QUADTO,
		OP_ARCTO,
		OP_CLOSE
	};

	// The container for a path program
    //	canonical
    //  comparable
	//  cacheable
	// 
	struct PathProgram
	{
		std::vector<uint8_t> ops;
		std::vector<float> args;

        void clear() noexcept
        {
            ops.clear();
            args.clear();
        }
	};

	// Arity table, how many arguments each op takes
	static constexpr uint8_t kPathOpArity[] = {
		0,	// OP_END
		2,	// OP_MOVETO: x y
		2,	// OP_LINETO: x y
		6,	// OP_CUBICTO: x1 y1 x2 y2 x y
		4,	// OP_QUADTO: x1 y1 x y
		7,	// OP_ARCTO: rx ry x-axis-rotation large-arc-flag sweep-flag x y
		0	// OP_CLOSE
	};

    // Ensure the ops size and arity table size match
	static_assert(OP_CLOSE + 1 == std::size(kPathOpArity), "PathOp arity table size mismatch");



    struct PathProgramBuilder
    {
        // Public program under construction.
        PathProgram prog;

        // Builder state.
        bool   fHasCurrentPoint{ false }; // any point set yet?
        bool   fSubpathOpen{ false }; // a subpath was started and not closed
        float  fCurX{ 0.0f };
        float  fCurY{ 0.0f };
        float  fStartX{ 0.0f }; // start point of current subpath
        float  fStartY{ 0.0f };

        // If true, builder refuses to emit further ops after end().
        bool   fEnded{ false };

        // ---- lifecycle ----

        void reset() noexcept
        {
            prog.ops.clear();
            prog.args.clear();

            fHasCurrentPoint = false;
            fSubpathOpen = false;
            fCurX = fCurY = 0.0f;
            fStartX = fStartY = 0.0f;
            fEnded = false;
        }

        // Reserve to avoid reallocations in hot parsing paths.
        void reserve(size_t opCount, size_t argCount) noexcept
        {
            prog.ops.reserve(opCount);
            prog.args.reserve(argCount);
        }

        // Finalize: ensures OP_END exists exactly once at end.
        // Returns false if builder is in an invalid state (e.g. already ended with extra ops).
        bool finalize() noexcept
        {
            if (fEnded) {
                // Already ended: ensure OP_END is last.
                if (prog.ops.empty()) return false;
                return prog.ops.back() == OP_END;
            }
            emitOp_(OP_END);
            fEnded = true;
            return true;
        }

        // Explicit end, same as finalize() but named like an opcode.
        bool end() noexcept { return finalize(); }

        // ---- query state ----
        bool hasCurrentPoint() const noexcept { return fHasCurrentPoint; }
        bool subpathOpen() const noexcept { return fSubpathOpen; }
        float curX() const noexcept { return fCurX; }
        float curY() const noexcept { return fCurY; }
        float subpathStartX() const noexcept { return fStartX; }
        float subpathStartY() const noexcept { return fStartY; }

        // ---- path operations ----
        //
        // These are absolute, normalized ops. If you parse SVG, do relative->absolute and
        // shorthand expansion before calling these, or do it outside.

        bool moveTo(float x, float y) noexcept
        {
            if (!ensureNotEnded_()) return false;

            emitOp_(OP_MOVETO);
            emit2_(x, y);

            fCurX = x; fCurY = y;
            fStartX = x; fStartY = y;
            fHasCurrentPoint = true;
            fSubpathOpen = true;
            return true;
        }

        bool lineTo(float x, float y) noexcept
        {
            if (!ensureNotEnded_()) return false;
            if (!fHasCurrentPoint) return fail_(); // must have moveto first

            emitOp_(OP_LINETO);
            emit2_(x, y);

            fCurX = x; fCurY = y;
            fSubpathOpen = true;
            return true;
        }

        bool quadTo(float x1, float y1, float x, float y) noexcept
        {
            if (!ensureNotEnded_()) return false;
            if (!fHasCurrentPoint) return fail_();

            emitOp_(OP_QUADTO);
            emit4_(x1, y1, x, y);

            fCurX = x; fCurY = y;
            fSubpathOpen = true;
            return true;
        }

        bool cubicTo(float x1, float y1, float x2, float y2, float x, float y) noexcept
        {
            if (!ensureNotEnded_()) return false;
            if (!fHasCurrentPoint) return fail_();

            emitOp_(OP_CUBICTO);
            emit6_(x1, y1, x2, y2, x, y);

            fCurX = x; fCurY = y;
            fSubpathOpen = true;
            return true;
        }

        // SVG endpoint-parameter arc form:
        // rx ry xAxisRotation largeArcFlag sweepFlag x y
        //
        // Here flags are expected as 0 or 1 (caller supplies normalized values).
        // You could also accept bool and store as 0/1 floats.
        bool arcTo(float rx, float ry, float xAxisRotation,
            float largeArcFlag, float sweepFlag,
            float x, float y) noexcept
        {
            if (!ensureNotEnded_()) return false;
            if (!fHasCurrentPoint) return fail_();

            emitOp_(OP_ARCTO);
            emit7_(rx, ry, xAxisRotation, largeArcFlag, sweepFlag, x, y);

            fCurX = x; fCurY = y;
            fSubpathOpen = true;
            return true;
        }

        bool close() noexcept
        {
            if (!ensureNotEnded_()) return false;
            if (!fHasCurrentPoint) return fail_();
            if (!fSubpathOpen)     return fail_(); // nothing to close

            emitOp_(OP_CLOSE);
            // no args

            // Per SVG: closepath draws a segment back to start and sets current point to start.
            fCurX = fStartX; fCurY = fStartY;
            fSubpathOpen = false;
            return true;
        }

        // Convenience: "start a new subpath" in a strict way.
        // If a subpath is open, this begins a new one without implicitly closing (SVG allows this).
        // If you want "auto-close", do that at a higher layer.
        bool beginSubpath(float x, float y) noexcept { return moveTo(x, y); }

        // ---- internal helpers ----
    private:
        bool ensureNotEnded_() noexcept
        {
            if (!fEnded) return true;
            return fail_();
        }

        bool fail_() noexcept
        {
            // In debug, crash early; in release, return false.
            WAAVS_ASSERT(false && "PathProgramBuilder: invalid operation sequence");
            return false;
        }

        void emitOp_(uint8_t op) noexcept
        {
            prog.ops.push_back(op);
        }

        void emit2_(float a, float b) noexcept
        {
            prog.args.push_back(a);
            prog.args.push_back(b);
        }

        void emit4_(float a, float b, float c, float d) noexcept
        {
            prog.args.push_back(a);
            prog.args.push_back(b);
            prog.args.push_back(c);
            prog.args.push_back(d);
        }

        void emit6_(float a, float b, float c, float d, float e, float f) noexcept
        {
            prog.args.push_back(a);
            prog.args.push_back(b);
            prog.args.push_back(c);
            prog.args.push_back(d);
            prog.args.push_back(e);
            prog.args.push_back(f);
        }

        void emit7_(float a, float b, float c, float d, float e, float f, float g) noexcept
        {
            prog.args.push_back(a);
            prog.args.push_back(b);
            prog.args.push_back(c);
            prog.args.push_back(d);
            prog.args.push_back(e);
            prog.args.push_back(f);
            prog.args.push_back(g);
        }
    };

    // ------------------------------------------------------------
    // Normalizer: consumes SVG PathSegments and emits normalized ops
    // ------------------------------------------------------------

    struct PathProgramFromSegments 
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

        // If you want to mimic your current BLPath behavior where after Z a non-M
        // needs an injected moveTo before drawing, keep this flag for the executor.
        // The program itself doesn't need it; the executor can track it.
        bool justClosed{ false };

        explicit PathProgramFromSegments(PathProgramBuilder& builder) noexcept
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

	template <class Exec>
	static void runPathProgram(const PathProgram& p, Exec& exec) noexcept
	{
		size_t ip = 0;		// instruction pointer
		size_t ap = 0;		// argument pointer

		while (ip < p.ops.size()) {
			const uint8_t op = p.ops[ip++];
			if (op == OP_END)
				break;

			const uint8_t n = kPathOpArity[op];
			exec.execute(op, p.args.data() + ap);
			ap += n;
		}
	}
}

namespace waavs 
{

    static INLINE bool getBoundingBox(const PathProgram& prog,
        double& x, double& y,
        double& width, double& height)
    {
        if (prog.ops.empty())
            return false;

        double minX = 0, minY = 0, maxX = 0, maxY = 0;
        bool   hasBBox = false;

        double cx = 0, cy = 0;
        double sx = 0, sy = 0;
        bool   hasPoint = false;

        size_t ip = 0;
        size_t ap = 0;

        while (ip < prog.ops.size()) {
            uint8_t op = prog.ops[ip++];
            if (op == OP_END)
                break;

            const float* a = prog.args.data() + ap;
            ap += kPathOpArity[op];

            auto include = [&](double px, double py) {
                if (!hasBBox) {
                    bboxInit(minX, minY, maxX, maxY, px, py);
                    hasBBox = true;
                }
                else {
                    bboxExpand(minX, minY, maxX, maxY, px, py);
                }
                };

            switch (op) {
            case OP_MOVETO:
                cx = a[0]; cy = a[1];
                sx = cx; sy = cy;
                hasPoint = true;
                include(cx, cy);
                break;

            case OP_LINETO:
                cx = a[0]; cy = a[1];
                include(cx, cy);
                break;

            case OP_QUADTO:
                quadBounds(cx, cy,
                    a[0], a[1],
                    a[2], a[3],
                    minX, minY, maxX, maxY);
                cx = a[2]; cy = a[3];
                break;

            case OP_CUBICTO:
                cubicBounds(cx, cy,
                    a[0], a[1],
                    a[2], a[3],
                    a[4], a[5],
                    minX, minY, maxX, maxY);
                cx = a[4]; cy = a[5];
                break;

            case OP_CLOSE:
                cx = sx; cy = sy;
                include(cx, cy);
                break;

            case OP_ARCTO:
                // For now: include endpoints (safe fallback)
                // You can replace this with full arc math next.
                cx = a[5]; cy = a[6];
                include(cx, cy);
                break;
            }
        }

        if (!hasBBox)
            return false;

        x = minX;
        y = minY;
        width = maxX - minX;
        height = maxY - minY;
        return true;
    }
}