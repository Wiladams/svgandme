#pragma once

#include <vector>
#include <cmath>

#include "definitions.h"
#include "maths.h"
#include "wggeometry.h"
#include "pathprogram.h"
#include "pathprogram_builder.h"
#include "pathprogram_flattener.h"

namespace waavs
{
    struct DragKnifeCompensateOptions
    {
        double bladeOffset{ 0.25 };
        double cornerAngleDegrees{ 30.0 };

        bool compensateOpenPaths{ true };
        bool compensateClosedPaths{ true };
        bool insertCornerBridge{ true };
        bool preserveClose{ true };
    };

    template <class Sink>
    struct DragKnifeCompensator
    {
        Sink& out;
        DragKnifeCompensateOptions opt{};

        bool fHasCurrentPoint{ false };
        bool fSubpathOpen{ false };
        bool fSubpathClosed{ false };

        WGPointD fCur{ 0.0, 0.0 };
        WGPointD fStart{ 0.0, 0.0 };

        std::vector<WGPointD> fPts;

        explicit DragKnifeCompensator(Sink& sink) noexcept
            : out(sink)
        {
        }

        void resetState() noexcept
        {
            fHasCurrentPoint = false;
            fSubpathOpen = false;
            fSubpathClosed = false;
            fCur = WGPointD(0.0, 0.0);
            fStart = WGPointD(0.0, 0.0);
            fPts.clear();
        }

        bool onMoveTo(float x, float y) noexcept
        {
            if (fSubpathOpen) {
                if (!flushSubpath_(false))
                    return false;
            }

            fCur = WGPointD(x, y);
            fStart = fCur;
            fHasCurrentPoint = true;
            fSubpathOpen = true;
            fSubpathClosed = false;
            fPts.clear();
            fPts.push_back(fCur);

            return true;
        }

        bool onLineTo(float x, float y) noexcept
        {
            if (!fHasCurrentPoint)
                return fail_();

            fCur = WGPointD(x, y);
            if (fPts.empty() || !samePoint_(fPts.back(), fCur))
                fPts.push_back(fCur);

            return true;
        }

        bool onQuadTo(float, float, float, float) noexcept
        {
            WAAVS_ASSERT(false && "DragKnifeCompensator expects flattened input");
            return false;
        }

        bool onCubicTo(float, float, float, float, float, float) noexcept
        {
            WAAVS_ASSERT(false && "DragKnifeCompensator expects flattened input");
            return false;
        }

        bool onArcTo(float, float, float, float, float, float, float) noexcept
        {
            WAAVS_ASSERT(false && "DragKnifeCompensator expects flattened input");
            return false;
        }

        bool onClose() noexcept
        {
            if (!fHasCurrentPoint)
                return fail_();
            if (!fSubpathOpen)
                return fail_();

            fCur = fStart;
            fSubpathClosed = true;

            if (fPts.empty() || !samePoint_(fPts.back(), fStart))
                fPts.push_back(fStart);

            if (!flushSubpath_(true))
                return false;

            fSubpathOpen = false;
            fSubpathClosed = false;
            fHasCurrentPoint = false;
            return true;
        }

        bool onEnd() noexcept
        {
            if (fSubpathOpen) {
                if (!flushSubpath_(fSubpathClosed))
                    return false;

                fSubpathOpen = false;
                fSubpathClosed = false;
                fHasCurrentPoint = false;
            }

            return out.onEnd();
        }

    private:
        bool fail_() noexcept
        {
            WAAVS_ASSERT(false && "DragKnifeCompensator: invalid path state");
            return false;
        }

        static bool nearlyEqual_(double a, double b) noexcept
        {
            const double eps = 1e-9;
            return std::abs(a - b) <= eps;
        }

        static bool samePoint_(const WGPointD& a, const WGPointD& b) noexcept
        {
            return nearlyEqual_(a.x, b.x) && nearlyEqual_(a.y, b.y);
        }

        static bool normalize_(const WGPointD& v, WGPointD& outv) noexcept
        {
            const double len = std::sqrt(v.x * v.x + v.y * v.y);
            if (len <= 1e-12)
                return false;

            outv.x = v.x / len;
            outv.y = v.y / len;
            return true;
        }

        static double angleDegrees_(const WGPointD& a, const WGPointD& b) noexcept
        {
            double dotp = a.x * b.x + a.y * b.y;
            if (dotp < -1.0)
                dotp = -1.0;
            else if (dotp > 1.0)
                dotp = 1.0;

            return std::acos(dotp) * (180.0 / 3.14159265358979323846);
        }

        void appendUnique_(std::vector<WGPointD>& pts, const WGPointD& p) noexcept
        {
            if (!pts.empty() && samePoint_(pts.back(), p))
                return;

            pts.push_back(p);
        }

        bool emitPolyline_(const std::vector<WGPointD>& pts, bool closed) noexcept
        {
            if (pts.empty())
                return true;

            if (!out.onMoveTo((float)pts[0].x, (float)pts[0].y))
                return false;

            for (size_t i = 1; i < pts.size(); ++i) {
                if (!out.onLineTo((float)pts[i].x, (float)pts[i].y))
                    return false;
            }

            if (closed && opt.preserveClose)
                return out.onClose();

            return true;
        }

        bool compensateOpen_(const std::vector<WGPointD>& src,
            std::vector<WGPointD>& dst) noexcept
        {
            dst.clear();

            if (src.size() < 2)
                return false;

            if (!opt.compensateOpenPaths) {
                dst = src;
                return true;
            }

            WGPointD d0 = src[1] - src[0];
            WGPointD n0;
            if (!normalize_(d0, n0))
                return false;

            appendUnique_(dst, WGPointD(
                src[0].x - n0.x * opt.bladeOffset,
                src[0].y - n0.y * opt.bladeOffset));

            for (size_t i = 1; i + 1 < src.size(); ++i) {
                const WGPointD vin = src[i] - src[i - 1];
                const WGPointD vout = src[i + 1] - src[i];

                WGPointD nin;
                WGPointD nout;

                if (!normalize_(vin, nin) || !normalize_(vout, nout)) {
                    appendUnique_(dst, src[i]);
                    continue;
                }

                const double ang = angleDegrees_(nin, nout);

                if (opt.insertCornerBridge && ang >= opt.cornerAngleDegrees)
                    appendUnique_(dst, src[i]);

                appendUnique_(dst, WGPointD(
                    src[i].x + nout.x * opt.bladeOffset,
                    src[i].y + nout.y * opt.bladeOffset));
            }

            WGPointD d1 = src[src.size() - 1] - src[src.size() - 2];
            WGPointD n1;
            if (!normalize_(d1, n1))
                return false;

            appendUnique_(dst, WGPointD(
                src[src.size() - 1].x + n1.x * opt.bladeOffset,
                src[src.size() - 1].y + n1.y * opt.bladeOffset));

            return dst.size() >= 2;
        }

        bool compensateClosed_(const std::vector<WGPointD>& src,
            std::vector<WGPointD>& dst) noexcept
        {
            dst.clear();

            if (src.size() < 4)
                return false;

            if (!opt.compensateClosedPaths) {
                dst = src;
                return true;
            }

            const size_t n = src.size() - 1;

            if (n < 3)
                return false;

            appendUnique_(dst, src[0]);

            for (size_t i = 0; i < n; ++i) {
                const size_t ip = (i + n - 1) % n;
                const size_t in = (i + 1) % n;

                const WGPointD& prev = src[ip];
                const WGPointD& cur = src[i];
                const WGPointD& next = src[in];

                const WGPointD vin = cur - prev;
                const WGPointD vout = next - cur;

                WGPointD nin;
                WGPointD nout;

                if (!normalize_(vin, nin) || !normalize_(vout, nout))
                    continue;

                const double ang = angleDegrees_(nin, nout);

                if (opt.insertCornerBridge && ang >= opt.cornerAngleDegrees)
                    appendUnique_(dst, cur);

                appendUnique_(dst, WGPointD(
                    cur.x + nout.x * opt.bladeOffset,
                    cur.y + nout.y * opt.bladeOffset));
            }

            appendUnique_(dst, dst.front());
            return dst.size() >= 2;
        }

        bool flushSubpath_(bool closed) noexcept
        {
            if (fPts.empty())
                return true;

            std::vector<WGPointD> src = fPts;
            std::vector<WGPointD> dst;

            bool ok = false;
            if (closed)
                ok = compensateClosed_(src, dst);
            else
                ok = compensateOpen_(src, dst);

            if (!ok)
                dst = src;

            fPts.clear();
            return emitPolyline_(dst, closed);
        }
    };

}





namespace waavs
{
    static INLINE bool compensateDragKnifePathProgram(
        const PathProgram& src,
        PathProgram& dst,
        double bladeOffset = 0.25,
        double cornerAngleDegrees = 30.0) noexcept
    {
        PathProgramBuilder builder;
        builder.reserve(src.ops.size() * 2u, src.args.size() * 4u);

        DragKnifeCompensator<PathProgramBuilder> comp(builder);
        comp.opt.bladeOffset = bladeOffset;
        comp.opt.cornerAngleDegrees = cornerAngleDegrees;

        if (!pathprogram_dispatch(src, comp))
            return false;

        dst = std::move(builder.prog);
        return true;
    }

    static INLINE bool compensateDragKnifePathProgramFlattened(
        const PathProgram& src,
        PathProgram& dst,
        double bladeOffset = 0.25,
        double cornerAngleDegrees = 30.0,
        double flatness = 0.25,
        int maxDepth = 16) noexcept
    {
        PathProgram flattened;
        if (!flattenPathProgram(src, flattened, flatness, maxDepth))
            return false;

        return compensateDragKnifePathProgram(
            flattened,
            dst,
            bladeOffset,
            cornerAngleDegrees);
    }
}