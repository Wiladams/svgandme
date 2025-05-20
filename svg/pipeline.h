#pragma once

#include <functional>

namespace waavs 
{
	// Producer: generates data of type T into an output reference
	// Returns true if a value was produced, false if stream is finished
	template<typename T>
	using ProducerFn = std::function<bool(T&)>;

	// Transformer: converts an input value into an output value
	// Returns true if transformation succeeded
	template<typename In, typename Out>
	using TransformerFn = std::function<bool(const In&, Out&)>;

	// Consumer: accepts a value for side effects (logging, output, rendering, etc.)
	template<typename T>
	using ConsumerFn = std::function<void(const T&)>;
}

namespace {
	template <typename OutType>
	struct IProduce
	{
		virtual ~IProduce() = default;
		virtual bool next(OutType& out) = 0;

		bool operator()(OutType& out)
		{
			return next(out);
		}
	};

	template <typename InType>
	struct IConsume
	{
		virtual ~IConsume() = default;
		virtual void consume(const InType& in) = 0;

		void operator()(const InType& in)
		{
			consume(in);
		}
	};
}