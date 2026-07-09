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
		virtual ~IPipelineSink() {};

		virtual void setInput(std::function<bool(InType&)> input) = 0;
	};

	template <typename InType, typename OutType>
	struct IPipelineFilter
		: public IPipelineSource<OutType>, public IPipelineSink<InType>
	{
	};
}

namespace waavs {
    // IParametricSource
    // A parametric source produces values of type T for a parameter t in [0..1]
    // Call the eval(t) method to get the value at t
    // the value of 't is typically in the range [0..1], but can be outside that range

	template <typename T>
	struct IParametricSource {
	public:
		virtual ~IParametricSource() = default;

		virtual T eval(double t) = 0;

		// Makes it callable like a function
        // This might be good syntactically, but might be confusing
        // particularly if the class is inherited from
		T operator()(double t) { return eval(t); }
	};
}
