#pragma once

#include "pathprogram.h"
#include "bspan.h"

namespace waavs
{

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

        // sink-style aliases
        bool onMoveTo(float x, float y) noexcept { return moveTo(x, y); }
        bool onLineTo(float x, float y) noexcept { return lineTo(x, y); }
        bool onQuadTo(float x1, float y1, float x, float y) noexcept { return quadTo(x1, y1, x, y); }
        bool onCubicTo(float x1, float y1, float x2, float y2, float x, float y) noexcept { return cubicTo(x1, y1, x2, y2, x, y); }
        bool onArcTo(float rx, float ry, float xAxisRotation,
            float largeArcFlag, float sweepFlag,
            float x, float y) noexcept {
            return arcTo(rx, ry, xAxisRotation, largeArcFlag, sweepFlag, x, y);
        }
        bool onClose() noexcept { return close(); }
        bool onEnd() noexcept { return finalize(); }

    private:
        // ---- internal helpers ----
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
}

