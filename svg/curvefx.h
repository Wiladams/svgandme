#pragma once

#include "curves.h"

namespace waavs {

	struct CurveSegment //: public IParametricSegment 
	{
		std::shared_ptr<IParametricCurve> curve;
		double t0 = 0.0;
		double t1 = 1.0;

		// Optional per-segment metadata
		bool visible = true;
		Point start;
		Point end;
	};
}

//
// Components for chainging components together
//
namespace waavs {
	struct ISegmentSource {
		virtual ~ISegmentSource() = default;
		virtual bool nextSegment(CurveSegment& out) = 0;

	};

	struct ISegmentFilter : public ISegmentSource {
		virtual void setInput(std::shared_ptr<ISegmentSource> input) = 0;
	};

	struct ISegmentSink {
		virtual ~ISegmentSink() = default;
		virtual void consume(const CurveSegment& seg) = 0;
	};

}

// Here are some concrete components
namespace waavs
{
	// A simple segment source that emits a single curve
	struct PVXCurveSource : public ISegmentSource
	{
		std::shared_ptr<IParametricCurve> fCurve;
		bool fEmitted = false;

		PVXCurveSource(std::shared_ptr<IParametricCurve> c)
			: fCurve(std::move(c)) {
		}

		bool nextSegment(CurveSegment& out) override
		{
			// We return the whole curve as a single segment
			// So, if we've already emitted it, we're done
			if (fEmitted)
				return false;

			out.curve = fCurve;
			out.t0 = 0.0;
			out.t1 = 1.0;
			fEmitted = true;

			return true;
		}

		//static std::shared_ptr<ISegmentSource> makeShared(std::shared_ptr<IParametricCurve> curve)
		//{
		//	return std::make_shared<CurveSource>(curve);
		//}

	};
}


namespace waavs 
{
	// Take in a segment source, and generate a dashed version of it
	class PVXDashFilter : public ISegmentFilter 
	{
	private:
		std::shared_ptr<ISegmentSource> fInput;
		std::vector<double> fPattern;
		size_t fPatternIndex = 0;
		bool fDraw = true;
		bool fReturnVisibleOnly = true;
		int fArcSteps = 100;

		CurveSegment fBaseSeg;
		const IParametricCurve* fCurve = nullptr;

		double fArcLen0 = 0.0;
		double fRemaining = 0.0;
		double fTotalLength = 0.0;

	public:
		PVXDashFilter(std::shared_ptr<ISegmentSource> input,
			std::initializer_list<double> pattern,
			int arcSteps = 100)
			: PVXDashFilter(input, std::vector<double>(pattern), arcSteps)
		{
		}

		PVXDashFilter(std::shared_ptr<ISegmentSource> input, std::vector<double> pattern, int arcSteps = 100)
			: fInput(input)
			, fPattern(std::move(pattern))
			, fArcSteps(arcSteps)
		{
			if (fPattern.empty()) fPattern = { 1.0 };
		}

		void setInput(std::shared_ptr<ISegmentSource> input) override {
			fInput = std::move(input);
			fCurve = nullptr;
		}

		bool nextSegment(CurveSegment& out) override {
			while (true) {
				if (!fCurve) {
					if (!fInput || !fInput->nextSegment(fBaseSeg))
						return false;

					fCurve = fBaseSeg.curve.get();
					fTotalLength = fCurve->computeLength(fArcSteps);
					fArcLen0 = 0.0;
					fPatternIndex = 0;
					fDraw = true;

					// Apply any offset (currently fixed at 0.0, can be extended later)
					double adjustedOffset = 0.0;
					while (adjustedOffset >= fPattern[fPatternIndex]) {
						adjustedOffset -= fPattern[fPatternIndex];
						advancePattern();
					}

					fRemaining = fPattern[fPatternIndex] - adjustedOffset;
				}

				if (fArcLen0 < fTotalLength) {
					double arcLen1 = std::min(fArcLen0 + fRemaining, fTotalLength);
					double t0 = fCurve->findTAtLength(fArcLen0, fArcSteps);
					double t1 = fCurve->findTAtLength(arcLen1, fArcSteps);

					fArcLen0 = arcLen1;
					advancePattern();
					fRemaining = fPattern[fPatternIndex];

					out.start = fCurve->eval(t0);
					out.end = fCurve->eval(t1);
					out.t0 = t0;
					out.t1 = t1;
					out.visible = fDraw;
					out.curve = nullptr; // Optional: skip creating subcurve

					if (!fReturnVisibleOnly || fDraw)
						return true;

					continue; // skip invisible segments
				}

				fCurve = nullptr; // finished this base segment
			}
		}

		void setReturnVisibleOnly(bool flag) {
			fReturnVisibleOnly = flag;
		}

	private:
		void advancePattern() {
			fPatternIndex = (fPatternIndex + 1) % fPattern.size();
			fDraw = !fDraw;
		}
	};

}

namespace waavs {

	class WidthOutlineFilter : public ISegmentFilter {
	private:
		std::shared_ptr<ISegmentSource> fInput;
		std::function<double(double)> fWidthFn;
		int fSteps;
		CurveSegment fCurrentSegment;
		int fIndex = 0;
		bool fReverse = false;
		bool fHaveSegment = false;
		Point fPrevPt;

	public:
		WidthOutlineFilter(std::function<double(double)> widthFn,
			int steps = 40)
			: fWidthFn(std::move(widthFn)), fSteps(steps) {
		}

		void setInput(std::shared_ptr<ISegmentSource> input) override {
			fInput = std::move(input);
			reset();
		}

		bool nextSegment(CurveSegment& out) override {
			while (true) {
				if (!fHaveSegment) {
					if (!fInput || !fInput->nextSegment(fCurrentSegment))
						return false;

					fIndex = 0;
					fReverse = false;
					fHaveSegment = true;
					fPrevPt = computeOffsetPoint(0.0, -1); // initialize left side
				}

				if (!fReverse && fIndex > fSteps) {
					// switch to right side
					fIndex = fSteps;
					fReverse = true;
					fPrevPt = computeOffsetPoint(1.0, +1);
				}

				if (fReverse && fIndex < 0) {
					fHaveSegment = false;
					continue;
				}

				double t = static_cast<double>(fIndex) / fSteps;
				double dir = fReverse ? +1.0 : -1.0;
				Point currPt = computeOffsetPoint(t, dir);

				auto seg = std::make_shared<LineCurve>(fPrevPt, currPt);
				out = { seg, t, t + dir / fSteps };

				fPrevPt = currPt;
				fIndex += (fReverse ? -1 : 1);
				return true;
			}
		}

	private:
		Point computeOffsetPoint(double t, double direction) const {
			double width = 0.5 * fWidthFn(t);
			Point base = fCurrentSegment.curve->eval(t);
			Point normal = fCurrentSegment.curve->evalNormal(t);
			return base + normal * (direction * width);
		}

		void reset() {
			fIndex = 0;
			fReverse = false;
			fHaveSegment = false;
		}
	};

	// Factory
	inline std::shared_ptr<ISegmentSource> brushVariableWidth(
		std::function<double(double)> widthFn,
		std::shared_ptr<ISegmentSource> input,
		int steps = 40)
	{
		auto filter = std::make_shared<WidthOutlineFilter>(std::move(widthFn), steps);
		filter->setInput(std::move(input));
		return filter;
	}

	// Overload for ParametricSource
	inline std::shared_ptr<ISegmentSource> brushVariableWidth(
		std::shared_ptr<IParametricSource<double>> widthMap,
		std::shared_ptr<ISegmentSource> input,
		int steps = 40)
	{
		return brushVariableWidth(
			[wm = std::move(widthMap)](double t) { return wm->eval(t); },
			std::move(input),
			steps
		);
	}

} // namespace waavs


