#pragma once

#include "definitions.h"
#include "maths.h"
#include "wggeometry.h"
#include "pathprogram.h"
#include "pathprogram_builder.h"

namespace waavs
{
    struct PathFlattenOptions
    {
        double flatness{ 0.25 };
        int maxDepth{ 16 };
    };

    template <class Sink>
    struct PathFlattener
    {
        Sink& out;
        PathFlattenOptions opt{};

        bool fHasCurrentPoint{ false };
        bool fSubpathOpen{ false };
        WGPointD fCur{ 0.0, 0.0 };
        WGPointD fStart{ 0.0, 0.0 };

        explicit PathFlattener(Sink& sink) noexcept
            : out(sink)
        {
        }

        void resetState() noexcept
        {
            fHasCurrentPoint = false;
            fSubpathOpen = false;
            fCur = WGPointD(0.0, 0.0);
            fStart = WGPointD(0.0, 0.0);
        }

        bool onMoveTo(float x, float y) noexcept
        {
            fCur = WGPointD(x, y);
            fStart = fCur;
            fHasCurrentPoint = true;
            fSubpathOpen = true;
            return out.onMoveTo(x, y);
        }

        bool onLineTo(float x, float y) noexcept
        {
            if (!fHasCurrentPoint)
                return fail_();

            fCur = WGPointD(x, y);
            return out.onLineTo(x, y);
        }

        bool onQuadTo(float x1, float y1, float x, float y) noexcept
        {
            if (!fHasCurrentPoint)
                return fail_();

            return flattenQuadRecursive_(
                fCur,
                WGPointD(x1, y1),
                WGPointD(x, y),
                0);
        }

        bool onCubicTo(float x1, float y1, float x2, float y2, float x, float y) noexcept
        {
            if (!fHasCurrentPoint)
                return fail_();

            return flattenCubicRecursive_(
                fCur,
                WGPointD(x1, y1),
                WGPointD(x2, y2),
                WGPointD(x, y),
                0);
        }

        bool onArcTo(float rx, float ry, float xAxisRotation,
            float largeArcFlag, float sweepFlag,
            float x, float y) noexcept
        {
            if (!fHasCurrentPoint)
                return fail_();

            return flattenArc_(
                rx, ry, xAxisRotation,
                largeArcFlag, sweepFlag,
                x, y);
        }

        bool onClose() noexcept
        {
            if (!fHasCurrentPoint)
                return fail_();
            if (!fSubpathOpen)
                return fail_();

            fCur = fStart;
            fSubpathOpen = false;
            return out.onClose();
        }

        bool onEnd() noexcept
        {
            return out.onEnd();
        }

    private:
        bool fail_() noexcept
        {
            WAAVS_ASSERT(false && "PathFlattener: invalid path state");
            return false;
        }

        bool emitLineTo_(const WGPointD& p) noexcept
        {
            fCur = p;
            return out.onLineTo((float)p.x, (float)p.y);
        }

        bool flattenQuadRecursive_(
            const WGPointD& p0,
            const WGPointD& p1,
            const WGPointD& p2,
            int depth) noexcept
        {
            if (depth >= opt.maxDepth)
                return emitLineTo_(p2);

            const double d = distance_to_line(p1, p0, p2);
            if (d <= opt.flatness)
                return emitLineTo_(p2);

            const WGPointD p01 = p0.midpoint(p1);
            const WGPointD p12 = p1.midpoint(p2);
            const WGPointD p012 = p01.midpoint(p12);

            if (!flattenQuadRecursive_(p0, p01, p012, depth + 1))
                return false;

            return flattenQuadRecursive_(p012, p12, p2, depth + 1);
        }

        bool flattenCubicRecursive_(
            const WGPointD& p0,
            const WGPointD& p1,
            const WGPointD& p2,
            const WGPointD& p3,
            int depth) noexcept
        {
            if (depth >= opt.maxDepth)
                return emitLineTo_(p3);

            const double d1 = distance_to_line(p1, p0, p3);
            const double d2 = distance_to_line(p2, p0, p3);

            if (max(d1, d2) <= opt.flatness)
                return emitLineTo_(p3);

            const WGPointD p01 = p0.midpoint(p1);
            const WGPointD p12 = p1.midpoint(p2);
            const WGPointD p23 = p2.midpoint(p3);

            const WGPointD p012 = p01.midpoint(p12);
            const WGPointD p123 = p12.midpoint(p23);

            const WGPointD p0123 = p012.midpoint(p123);

            if (!flattenCubicRecursive_(p0, p01, p012, p0123, depth + 1))
                return false;

            return flattenCubicRecursive_(p0123, p123, p23, p3, depth + 1);
        }

        bool flattenArc_(
            double rx, double ry, double xAxisRotation,
            double largeArcFlag, double sweepFlag,
            double x, double y) noexcept
        {
            const WGPointD p0 = fCur;
            const WGPointD p1(x, y);

            if (abs(rx) <= dbl_eps || abs(ry) <= dbl_eps)
                return emitLineTo_(p1);

            rx = abs(rx);
            ry = abs(ry);

            const double phi = xAxisRotation * DegToRad;
            const double cosPhi = cos(phi);
            const double sinPhi = sin(phi);

            const double dx2 = (p0.x - p1.x) * 0.5;
            const double dy2 = (p0.y - p1.y) * 0.5;

            const double x1p = cosPhi * dx2 + sinPhi * dy2;
            const double y1p = -sinPhi * dx2 + cosPhi * dy2;

            if (abs(x1p) <= dbl_eps && abs(y1p) <= dbl_eps)
                return emitLineTo_(p1);

            double lam = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
            if (lam > 1.0) {
                const double s = sqrt(lam);
                rx *= s;
                ry *= s;
            }

            const double rx2 = rx * rx;
            const double ry2 = ry * ry;
            const double x1p2 = x1p * x1p;
            const double y1p2 = y1p * y1p;

            const bool largeArc = (largeArcFlag != 0.0);
            const bool sweep = (sweepFlag != 0.0);

            double num = rx2 * ry2 - rx2 * y1p2 - ry2 * x1p2;
            double den = rx2 * y1p2 + ry2 * x1p2;

            if (den <= dbl_eps)
                return emitLineTo_(p1);

            if (num < 0.0)
                num = 0.0;

            double coef = sqrt(num / den);
            if (largeArc == sweep)
                coef = -coef;

            const double cxp = coef * ((rx * y1p) / ry);
            const double cyp = coef * (-(ry * x1p) / rx);

            const double cx = cosPhi * cxp - sinPhi * cyp + (p0.x + p1.x) * 0.5;
            const double cy = sinPhi * cxp + cosPhi * cyp + (p0.y + p1.y) * 0.5;

            auto angleBetween_ = [](double ux, double uy, double vx, double vy) noexcept -> double
                {
                    const double dotp = ux * vx + uy * vy;
                    const double det = ux * vy - uy * vx;
                    return atan2(det, dotp);
                };

            const double ux = (x1p - cxp) / rx;
            const double uy = (y1p - cyp) / ry;
            const double vx = (-x1p - cxp) / rx;
            const double vy = (-y1p - cyp) / ry;

            double theta1 = atan2(uy, ux);
            double delta = angleBetween_(ux, uy, vx, vy);

            if (!sweep && delta > 0.0)
                delta -= Pi2;
            else if (sweep && delta < 0.0)
                delta += Pi2;

            const double rmax = max(rx, ry);

            double maxStep = 0.0;
            if (rmax <= opt.flatness || rmax <= dbl_eps) {
                maxStep = abs(delta);
            }
            else {
                double c = 1.0 - (opt.flatness / rmax);
                c = clamp(c, -1.0, 1.0);
                maxStep = 2.0 * acos(c);
            }

            if (maxStep <= 1e-6 || !isfinite((float)maxStep))
                maxStep = 0.25;

            int steps = (int)std::ceil(abs(delta) / maxStep);
            if (steps < 1)
                steps = 1;

            for (int i = 1; i <= steps; ++i) {
                const double t = (double)i / (double)steps;
                const double a = theta1 + delta * t;

                const double ca = cos(a);
                const double sa = sin(a);

                const double px = cx + cosPhi * rx * ca - sinPhi * ry * sa;
                const double py = cy + sinPhi * rx * ca + cosPhi * ry * sa;

                if (!emitLineTo_(WGPointD(px, py)))
                    return false;
            }

            return true;
        }
    };

    static INLINE bool flattenPathProgram(
        const PathProgram& src,
        PathProgram& dst,
        double flatness = 0.25,
        int maxDepth = 16) noexcept
    {
        PathProgramBuilder builder;
        builder.reserve(src.ops.size() * 2u, src.args.size() * 4u);

        PathFlattener<PathProgramBuilder> flattener(builder);
        flattener.opt.flatness = flatness;
        flattener.opt.maxDepth = maxDepth;

        if (!pathprogram_dispatch(src, flattener))
            return false;

        dst = std::move(builder.prog);
        return true;
    }
}