#pragma once

// Full C++ implementation of arc flattening and integration with G-code emission
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm> // for std::clamp
#include <memory>

#include "pathsegmenter.h"
#include "parametric.h"
#include "waavsgraph.h"


namespace waavs
{
    struct IParametricCurve: public IParametricSource<Point>
    {
        virtual ~IParametricCurve() = default;

        virtual Point evalTangent(double t) const = 0;

        virtual Point evalNormal(double t) const {
            Point tangent = evalTangent(t);
            tangent.normalize();
            return { -tangent.y, tangent.x };
        }

        // Calculate the length of the curve
        virtual double computeLength(int steps = 50) const {
            double length = 0.0;
            Point prev = eval(0.0);
            for (int i = 1; i <= steps; ++i) {
                double t = static_cast<double>(i) / steps;
                Point curr = eval(t);
                double dx = curr.x - prev.x;
                double dy = curr.y - prev.y;
                length += std::sqrt(dx * dx + dy * dy);
                prev = curr;
            }
            return length;
        }

        double approximateArcLength(double t0, double t1, int steps = 10) const {
            double length = 0.0;
            Point prev = eval(t0);
            for (int i = 1; i <= steps; ++i) {
                double t = t0 + (t1 - t0) * i / steps;
                Point curr = eval(t);
                double dx = curr.x - prev.x;
                double dy = curr.y - prev.y;
                length += std::sqrt(dx * dx + dy * dy);
                prev = curr;
            }
            return length;
        }

        virtual double findTAtLength(double targetLength, int maxIterations = 20) const
        {
            if (targetLength <= 0.0)
                return 0.0;

            double totalLen = computeLength();
            if (targetLength >= totalLen)
                return 1.0;

            double lo = 0.0;
            double hi = 1.0;
            double tMid = 0.5;

            for (int i = 0; i < maxIterations; ++i)
            {
                tMid = 0.5 * (lo + hi);
                double len = approximateArcLength(0.0, tMid, 10); // coarse but consistent
                if (std::abs(len - targetLength) < 1e-4)
                    break;

                if (len < targetLength)
                    lo = tMid;
                else
                    hi = tMid;
            }

            return clamp(tMid, 0.0, 1.0);
        }
    };
}

namespace waavs {
	// SubCurve
	// This class is a wrapper around an existing parametric curve
	// It allows you to create a new curve that is a subset of the original
    //
    struct SubCurve : public IParametricCurve 
    {
    private:
        const IParametricCurve& fBase;
		double fT0, fT1;

    public:
        SubCurve(const IParametricCurve& base, double t0, double t1)
            : fBase(base)
            , fT0(t0)
            , fT1(t1) {}

        Point eval(double t) const override {
            double mapped = fT0 + t * (fT1 - fT0);
            return fBase.eval(mapped);
        }

        Point evalTangent(double t) const override {
            double mapped = fT0 + t * (fT1 - fT0);
            return fBase.evalTangent(mapped);
        }

        double computeLength(int steps = 50) const override {
            double len = 0.0;
            Point prev = eval(0.0);
            for (int i = 1; i <= steps; ++i) {
                double t = static_cast<double>(i) / steps;
                Point curr = eval(t);
                len += std::hypot(curr.x - prev.x, curr.y - prev.y);
                prev = curr;
            }
            return len;
        }

        double findTAtLength(double length, int steps = 50) const override {
            double total = computeLength(steps);
            if (length <= 0.0) return 0.0;
            if (length >= total) return 1.0;

            double acc = 0.0;
            Point prev = eval(0.0);
            for (int i = 1; i <= steps; ++i) {
                double t = static_cast<double>(i) / steps;
                Point curr = eval(t);
                double segLen = std::hypot(curr.x - prev.x, curr.y - prev.y);
                if (acc + segLen >= length) {
                    double frac = (length - acc) / segLen;
                    return static_cast<double>(i - 1) / steps + frac / steps;
                }
                acc += segLen;
                prev = curr;
            }
            return 1.0;
        }

    };

}


namespace waavs {

	// A simple parametric curve that represents a single line segment
    class LineCurve : public IParametricCurve 
    {
    public:
        LineCurve(Point a, Point b) : p0(a), p1(b) {}

        Point eval(double t) const override {
            double u = 1.0 - t;
            return { u * p0.x + t * p1.x, u * p0.y + t * p1.y };
        }

        Point evalTangent(double) const override {
            return { p1.x - p0.x, p1.y - p0.y }; // constant direction
        }

        double computeLength(int = 1) const override {
            double dx = p1.x - p0.x;
            double dy = p1.y - p0.y;
            return std::sqrt(dx * dx + dy * dy);
        }

        double findTAtLength(double length, int = 1) const override {
            double totalLen = computeLength();
            if (totalLen <= 1e-8) return 0.0;
            return clamp(length / totalLen, 0.0, 1.0);
        }

    private:
        Point p0, p1;
    };
}

namespace waavs 
{
    struct CubicCurve : public IParametricCurve {
        Point a, b, c, d;

        CubicCurve(const Point &p0, const Point &p1, const Point &p2, const Point &p3) 
        {
            d = p0;
            c = { 3 * (p1.x - p0.x),       3 * (p1.y - p0.y) };
            b = { 3 * (p2.x - 2 * p1.x + p0.x), 3 * (p2.y - 2 * p1.y + p0.y) };
            a = { p3.x - p0.x - c.x - b.x, p3.y - p0.y - c.y - b.y };
        }

        Point eval(double t) const override 
        {
            t = clamp(t, 0.0, 1.0);
            return ((a * t + b) * t + c) * t + d;
        }

        Point evalTangent(double t) const override 
        {
            double u = 1.0 - t;
            return c * (3 * u * u) + b * (6 * u * t) + a * (3 * t * t);
        }
    };
}



namespace waavs {
    struct QuadraticCurve : public IParametricCurve 
    {
        Point a, b, c;

        QuadraticCurve(Point p0, Point p1, Point p2) {
            c = p0;
            b = { 2 * (p1.x - p0.x),       2 * (p1.y - p0.y) };
            a = { p2.x - p0.x - b.x / 2,   p2.y - p0.y - b.y / 2 };
        }

        Point eval(double t) const override {
            t = clamp(t, 0.0, 1.0);
            return (a * t + b) * t + c;
        }

        Point evalTangent(double t) const override {
            return a * (2 * t) + b;
        }
    };
}


namespace waavs {

    struct ArcCurve : public IParametricCurve {
        Point center;
        double rx, ry, phi, theta1, deltaTheta;
        double cosPhi, sinPhi;

        ArcCurve(Point p0, Point p1, double rx_, double ry_, double xAxisRotationDeg, bool largeArc, bool sweep) {
            constexpr double pi = 3.14159265358979323846;
            rx = std::abs(rx_);
            ry = std::abs(ry_);
            phi = xAxisRotationDeg * pi / 180.0;
            cosPhi = std::cos(phi);
            sinPhi = std::sin(phi);

            if (p0.x == p1.x && p0.y == p1.y) {
                theta1 = deltaTheta = 0;
                center = p0;
                return;
            }

            double dx2 = (p0.x - p1.x) / 2.0;
            double dy2 = (p0.y - p1.y) / 2.0;
            double cos_phi = std::cos(-phi);
            double sin_phi = std::sin(-phi);
            double x1p = cos_phi * dx2 - sin_phi * dy2;
            double y1p = sin_phi * dx2 + cos_phi * dy2;

            double lambda = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry);
            if (lambda > 1) {
                double scale = std::sqrt(lambda);
                rx *= scale;
                ry *= scale;
            }

            double num = rx * rx * ry * ry - rx * rx * y1p * y1p - ry * ry * x1p * x1p;
            double denom = rx * rx * y1p * y1p + ry * ry * x1p * x1p;
            double coef = (largeArc != sweep ? 1 : -1) * std::sqrt(std::max(0.0, num / denom));
            double cxp = coef * rx * y1p / ry;
            double cyp = coef * -ry * x1p / rx;
            center = {
                cosPhi * cxp - sinPhi * cyp + (p0.x + p1.x) / 2.0,
                sinPhi * cxp + cosPhi * cyp + (p0.y + p1.y) / 2.0
            };

            Point start_v = { (x1p - cxp) / rx, (y1p - cyp) / ry };
            Point end_v = { (-x1p - cxp) / rx, (-y1p - cyp) / ry };

            auto angle_between = [](const Point& u, const Point& v) -> double {
                double dot = u.x * v.x + u.y * v.y;
                double len_u = std::sqrt(u.x * u.x + u.y * u.y);
                double len_v = std::sqrt(v.x * v.x + v.y * v.y);
                double ang = std::acos(clamp(dot / (len_u * len_v), -1.0, 1.0));
                if ((u.x * v.y - u.y * v.x) < 0) ang = -ang;
                return ang;
                };

            theta1 = angle_between({ 1, 0 }, start_v);
            deltaTheta = angle_between(start_v, end_v);
            if (!sweep && deltaTheta > 0) deltaTheta -= 2 * pi;
            if (sweep && deltaTheta < 0) deltaTheta += 2 * pi;
        }

        Point eval(double t) const override {
            double theta = theta1 + deltaTheta * clamp(t, 0.0, 1.0);
            return {
                center.x + rx * std::cos(theta) * cosPhi - ry * std::sin(theta) * sinPhi,
                center.y + rx * std::cos(theta) * sinPhi + ry * std::sin(theta) * cosPhi
            };
        }

        Point evalTangent(double t) const override {
            double theta = theta1 + deltaTheta * clamp(t, 0.0, 1.0);
            double dx = -rx * std::sin(theta);
            double dy = ry * std::cos(theta);
            return {
                dx * cosPhi - dy * sinPhi,
                dx * sinPhi + dy * cosPhi
            };
        }
    };


}

namespace waavs {
    struct CurveParametricSegmentGenerator
    {
    public:
        CurveParametricSegmentGenerator(const IParametricCurve& curve, int steps)
            : fCurve(curve), fSteps(steps), fIndex(0) {
        }

        bool next(Point& out, double& t) {
            if (fIndex > fSteps) return false;
            t = static_cast<double>(fIndex++) / fSteps;
            out = fCurve.eval(t);
            return true;
        }

    private:
        const IParametricCurve& fCurve;
        int fSteps;
        int fIndex;
    };

    class CurveArcLengthSegmentGenerator {
    public:
        CurveArcLengthSegmentGenerator(const IParametricCurve& curve, int steps)
            : fCurve(curve), fSteps(steps), fIndex(0), fTotalLength(curve.computeLength()) {
        }

        bool next(Point& out, double& t) {
            if (fIndex > fSteps) return false;
            double arcLen = (fTotalLength * fIndex++) / fSteps;
            t = fCurve.findTAtLength(arcLen);
            out = fCurve.eval(t);
            return true;
        }

    private:
        const IParametricCurve& fCurve;
        int fSteps;
        int fIndex;
        double fTotalLength;
    };

}

// Here for safe keeping
// We'll revisit this adaptive method later
namespace waavs {
    // CubicBezierAdaptiveGenerator
    //
    // This generator uses an adaptive subdivision algorithm to generate 
    // points along a cubic Bézier curve.
    // It recursively subdivides the curve until the segments are flat enough.
    // This is in contrast to the SegmentGenerator, which uses a fixed number
    // of segments, no matter how flat the curve is.
    // The same generator interface is used : next() returns the next point
    struct CubicBezierAdaptiveGenerator
    {
    private:
        struct Segment {
            Point p0, p1, p2, p3;
            double t0, t1;
        };

        std::vector<Segment> fSegmentStack;
        bool first = false;
        double flatnessThreshold;


    public:
        CubicBezierAdaptiveGenerator(Point p0_, Point p1_, Point p2_, Point p3_, double flatness = 0.25)
            : flatnessThreshold(flatness)
        {
            fSegmentStack.push_back({ p0_, p1_, p2_, p3_ });
            first = true;
        }

        bool next(Point& out) {
            while (!fSegmentStack.empty()) {
                Segment seg = fSegmentStack.back();
                fSegmentStack.pop_back();

                if (isFlatEnough(seg)) {
                    if (first) {
                        out = seg.p0;
                        first = false;
                        fSegmentStack.push_back(seg); // put back to return p3 next
                        return true;
                    }
                    out = seg.p3;
                    return true;
                }
                else {
                    // Subdivide
                    Point p01 = seg.p0.midpoint(seg.p1);
                    Point p12 = seg.p1.midpoint(seg.p2);
                    Point p23 = seg.p2.midpoint(seg.p3);
                    Point p012 = p01.midpoint(p12);
                    Point p123 = p12.midpoint(p23);
                    Point p0123 = p012.midpoint(p123);

                    fSegmentStack.push_back({ p0123, p123, p23, seg.p3 });
                    fSegmentStack.push_back({ seg.p0, p01, p012, p0123 });
                }
            }
            return false;
        }

    private:

        bool isFlatEnough(const Segment& s) const {
            return distanceToLine(s.p1, s.p0, s.p3) < flatnessThreshold &&
                distanceToLine(s.p2, s.p0, s.p3) < flatnessThreshold;
        }

    };
}
