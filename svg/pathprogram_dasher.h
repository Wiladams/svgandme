#pragma once

#include <vector>
#include <cstddef>

#include "definitions.h"
#include "maths.h"
#include "wggeometry.h"
#include "pathprogram.h"
#include "pathprogram_builder.h"

namespace waavs
{
    struct PathDashPattern
    {
        std::vector<double> values;

        void clear() noexcept
        {
            values.clear();
        }

        bool empty() const noexcept
        {
            return values.empty();
        }

        size_t size() const noexcept
        {
            return values.size();
        }

        double period() const noexcept
        {
            double s = 0.0;
            for (double v : values)
                s += v;
            return s;
        }

        bool set(const std::vector<float>& dashArray) noexcept
        {
            values.clear();

            if (dashArray.empty())
                return false;

            const size_t dashCount = dashArray.size();
            double sum = 0.0;

            for (size_t i = 0; i < dashCount; ++i)
            {
                const double v = (double)dashArray[i];

                if (!isfinite((float)v))
                    return false;
                if (v < 0.0)
                    return false;

                values.push_back(v);
                sum += v;
            }

            if (values.empty() || sum <= dbl_eps)
                return false;

            // SVG semantics: odd-length patterns repeat to even length.
            if ((values.size() & 1u) != 0u)
            {
                const size_t n = values.size();
                values.reserve(n * 2u);
                for (size_t i = 0; i < n; ++i)
                    values.push_back(values[i]);
            }

            return period() > dbl_eps;
        }
    };

    struct PathDashState
    {
        const PathDashPattern* pattern{ nullptr };
        size_t index{ 0 };
        double remaining{ 0.0 };
        bool draw{ true };

        void reset(const PathDashPattern& pat, double dashOffset = 0.0) noexcept
        {
            pattern = &pat;
            index = 0;
            remaining = 0.0;
            draw = true;

            if (!pattern || pattern->empty())
                return;

            prime_();

            const double per = pattern->period();
            if (per <= dbl_eps)
                return;

            double off = std::fmod(dashOffset, per);
            if (off < 0.0)
                off += per;

            consume(off);
        }

        void consume(double dist) noexcept
        {
            if (!pattern || pattern->empty())
                return;

            double left = dist;

            while (left > dbl_eps)
            {
                ensureReady();

                const double step = min(left, remaining);
                if (step <= dbl_eps)
                    return;

                remaining -= step;
                left -= step;
            }
        }

        void ensureReady() noexcept
        {
            if (!pattern || pattern->empty())
                return;

            if (remaining > dbl_eps)
                return;

            advance_();
        }

    private:
        void prime_() noexcept
        {
            if (!pattern || pattern->empty())
                return;

            index = 0;
            draw = true;
            remaining = pattern->values[index];

            while (remaining <= dbl_eps)
                advance_();
        }

        void advance_() noexcept
        {
            if (!pattern || pattern->empty())
                return;

            do
            {
                index = (index + 1u) % pattern->size();
                draw = ((index & 1u) == 0u);
                remaining = pattern->values[index];
            } while (remaining <= dbl_eps);
        }
    };

    struct PathDashOptions
    {
        double dashOffset{ 0.0 };
        bool restartEachSubpath{ true };
    };

    template <class Sink>
    struct PathDasher
    {
        Sink& out;
        PathDashPattern pattern;
        PathDashOptions opt{};

        bool fHasCurrentPoint{ false };
        bool fSubpathOpen{ false };
        WGPointD fCur{ 0.0, 0.0 };
        WGPointD fStart{ 0.0, 0.0 };

        bool fPenDown{ false };
        WGPointD fPen{ 0.0, 0.0 };

        PathDashState fDash{};

        explicit PathDasher(Sink& sink) noexcept
            : out(sink)
        {
        }

        bool setDashArray(const std::vector<float>& dashArray) noexcept
        {
            return pattern.set(dashArray);
        }

        void resetState() noexcept
        {
            fHasCurrentPoint = false;
            fSubpathOpen = false;
            fCur = WGPointD(0.0, 0.0);
            fStart = WGPointD(0.0, 0.0);
            fPenDown = false;
            fPen = WGPointD(0.0, 0.0);
            fDash = PathDashState{};
        }

        bool onMoveTo(float x, float y) noexcept
        {
            const WGPointD p((double)x, (double)y);

            fCur = p;
            fStart = p;
            fHasCurrentPoint = true;
            fSubpathOpen = true;

            fPenDown = false;
            fPen = p;

            if (opt.restartEachSubpath || fDash.pattern == nullptr)
                fDash.reset(pattern, opt.dashOffset);

            return true;
        }

        bool onLineTo(float x, float y) noexcept
        {
            if (!fHasCurrentPoint)
                return fail_();

            const WGPointD p1((double)x, (double)y);

            if (!dashLine_(fCur, p1))
                return false;

            fCur = p1;
            return true;
        }

        bool onClose() noexcept
        {
            if (!fHasCurrentPoint)
                return fail_();
            if (!fSubpathOpen)
                return fail_();

            if (!dashLine_(fCur, fStart))
                return false;

            fCur = fStart;
            fSubpathOpen = false;
            fPenDown = false;
            fPen = fCur;

            // Do not forward onClose() downstream. Dashed output is emitted
            // as explicit move/line segments, not as one closed contour.
            return true;
        }

        bool onEnd() noexcept
        {
            return out.onEnd();
        }

        bool onQuadTo(float, float, float, float) noexcept
        {
            WAAVS_ASSERT(false && "PathDasher requires flattened input");
            return false;
        }

        bool onCubicTo(float, float, float, float, float, float) noexcept
        {
            WAAVS_ASSERT(false && "PathDasher requires flattened input");
            return false;
        }

        bool onArcTo(float, float, float, float, float, float, float) noexcept
        {
            WAAVS_ASSERT(false && "PathDasher requires flattened input");
            return false;
        }

    private:
        bool fail_() noexcept
        {
            WAAVS_ASSERT(false && "PathDasher: invalid path state");
            return false;
        }

        bool movePenToVisibleStart_(const WGPointD& p) noexcept
        {
            if (!fPenDown)
            {
                if (!out.onMoveTo((float)p.x, (float)p.y))
                    return false;
                fPenDown = true;
            }

            fPen = p;
            return true;
        }

        bool linePenTo_(const WGPointD& p) noexcept
        {
            if (!out.onLineTo((float)p.x, (float)p.y))
                return false;

            fPen = p;
            return true;
        }

        void breakStrokeAt_(const WGPointD& p) noexcept
        {
            fPenDown = false;
            fPen = p;
        }

        bool emitSolidLine_(const WGPointD& a, const WGPointD& b) noexcept
        {
            if (!movePenToVisibleStart_(a))
                return false;
            if (!linePenTo_(b))
                return false;
            return true;
        }

        bool dashLine_(const WGPointD& a, const WGPointD& b) noexcept
        {
            const WGPointD d = b - a;
            const double len = std::sqrt(d.x * d.x + d.y * d.y);

            if (len <= dbl_eps)
            {
                breakStrokeAt_(b);
                return true;
            }

            // Defensive fallback. If dash state was never initialized,
            // emit as a solid line rather than entering a bad state.
            if (fDash.pattern == nullptr || fDash.pattern->empty())
            {
                return emitSolidLine_(a, b);
            }

            double pos = 0.0;

            while (pos < len - dbl_eps)
            {
                fDash.ensureReady();

                const double step = min(len - pos, fDash.remaining);
                if (step <= dbl_eps)
                {
                    WAAVS_ASSERT(false && "PathDasher: zero dash step");
                    breakStrokeAt_(b);
                    return true;
                }

                const double t0 = pos / len;
                const double t1 = (pos + step) / len;

                const WGPointD p0(
                    lerp(a.x, b.x, t0),
                    lerp(a.y, b.y, t0));

                const WGPointD p1(
                    lerp(a.x, b.x, t1),
                    lerp(a.y, b.y, t1));

                if (fDash.draw)
                {
                    if (!movePenToVisibleStart_(p0))
                        return false;
                    if (!linePenTo_(p1))
                        return false;
                }
                else
                {
                    breakStrokeAt_(p1);
                }

                fDash.remaining -= step;
                pos += step;
            }

            return true;
        }
    };

    template <class Sink>
    static INLINE bool initPathDasher(
        PathDasher<Sink>& dasher,
        const std::vector<float>& dashArray,
        double dashOffset = 0.0,
        bool restartEachSubpath = true) noexcept
    {
        if (!dasher.setDashArray(dashArray))
            return false;

        dasher.opt.dashOffset = dashOffset;
        dasher.opt.restartEachSubpath = restartEachSubpath;
        dasher.resetState();

        return true;
    }

    static INLINE bool dashPathProgram(
        const PathProgram& srcFlat,
        PathProgram& dst,
        const std::vector<float>& dashArray,
        double dashOffset = 0.0,
        bool restartEachSubpath = true) noexcept
    {
        PathProgramBuilder builder;
        builder.reserve(srcFlat.ops.size() * 2u, srcFlat.args.size() * 2u);

        PathDasher<PathProgramBuilder> dasher(builder);

        if (!initPathDasher(dasher, dashArray, dashOffset, restartEachSubpath))
            return false;

        if (!pathprogram_dispatch(srcFlat, dasher))
            return false;

        dst = std::move(builder.prog);
        return true;
    }
}