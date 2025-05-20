#pragma once

#include "pipeline.h"

//
// Components for chaining components together in a pull model pipeline
//
namespace waavs {
	template <typename OutType>
	struct IPipelineSource {
		virtual ~IPipelineSource() = default;

		virtual bool next(OutType& out) = 0;

		// Enables use as a callable/lambda
		bool operator()(OutType& out) { return next(out); }
	};

	template <typename InType>
	struct IPipelineSink
	{
		virtual ~IPipelineSink() = default;

		virtual void setInput(std::function<bool(InType&)> input) = 0;
	};

	template <typename InType, typename OutType>
	struct IPipelineFilter : public IPipelineSource<OutType>, public IPipelineSink<InType> {};

}

namespace waavs {
	template <typename T>
	class IParametricSource {
	public:
		virtual ~IParametricSource() = default;

		virtual T eval(double t) const = 0;

		// Makes it callable like a function
		T operator()(double t) const { return eval(t); }
	};
}
