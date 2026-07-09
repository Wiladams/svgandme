#pragma once

#include <functional>
#include <memory>

namespace waavs
{
	// Producer: generates data of type T into an output reference
	// Returns true if a value was produced, false if no value is produced
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

// Class based interfaces for producers, consumers, and transformers
// pipeline construction
namespace waavs 
{

	template <typename OutType>
	struct IProduce
	{
        using OutputType = OutType; // For type inference in helpers

		virtual ~IProduce() = default;
		virtual bool next(OutType& out) { return false; }

		// Don't do the following, as when you pass an object
		// to a function that expects a function, it will make
		// a copy of this object!
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

		//void setInput(ProducerFn<InType>)

	};

	template <typename InType, typename OutType>
	struct ITransform : public IProduce<OutType>
	{
		// So we can infer template types in helpers
		using InputType = InType;
		using OutputType = OutType;

		ProducerFn<InType> fProducer;

		void setInput(ProducerFn<InType> src)
		{
			// Set the input producer function
			// This will be called to get the next input value
			// and then transform it into an output value
			fProducer = std::move(src);
		}

		virtual bool transform(const InType& in, OutType& out) = 0;

		// Implement next(), which will read one item from the
		// input source, transform it, and return it.
		bool next(OutType& outValue) override
		{
			InType inValue;
			if (!fProducer(inValue))
				return false;
			return transform(inValue, outValue);
		}
	};
}

/*
// Some helpers that make chaining better
namespace waavs 
{
	template <typename T, typename ProducerT>
	std::function<bool(T&)> PLProducer(ProducerT& producer)
	{
		return [&](T& out) -> bool {
			return producer.next(out);
			};
	}



	template <typename T, typename ConsumerT>
	std::function<void(const T&)> PLConsumer(ConsumerT& consumer)
	{
		return [&](const T& in) {
			consumer.consume(in);
			};
	}



	// Reference-based: caller owns the transformer
	template <typename TransformT>
	auto PLTransform(TransformT& filter)
	{
		using OutType = typename TransformT::OutputType;
		return std::function<bool(OutType&)>(
			[&filter](OutType& out) -> bool {
				return filter.next(out);
			}
		);
	}

	// Construct a transformer of type TransformT, bind it to input, 
	// and return a ProducerFn<OutType>
	// Construct and wrap a transformer, owning it via shared_ptr
	template <typename TransformT, typename InputFn>
	auto PLConstructTransform(InputFn inputFn)
	{
		using InType = typename TransformT::InputType;
		using OutType = typename TransformT::OutputType;

		std::function<bool(InType&)> input = inputFn;

		auto filter = std::make_shared<TransformT>(std::move(input));

		return std::function<bool(OutType&)>(
			[filter](OutType& out) -> bool {
				return filter->next(out);
			}
		);
	}
	
	// Use this one when you don't know the lifetime of 
	// the input.
	template <typename TransformT, typename InputFn>
	auto PLTransformAuto(InputFn inputFn)
	{
		using InType = typename TransformT::InputType;
		std::function<bool(InType&)> fn = inputFn;
		return PLConstructTransform<TransformT>(fn);
	}
}


namespace waavs {
	//template <typename TransformT, typename InputFn>
	//auto MakeTransform(InputFn inputFn)
	//{
	//	using InType = typename TransformT::InputType;
	//	std::function<bool(InType&)> fn = inputFn;
	//	return PLConstructTransform<TransformT>(fn);
	//}

	template <typename TransformT>
	auto MakeTransform()
	{
		return [](auto inputFn) {
			return PLConstructTransform<TransformT>(inputFn);
			};
	}

	template <typename InType, typename TransformFactory>
	auto operator|(waavs::ProducerFn<InType> input, TransformFactory&& factory)
	{
		return factory(std::move(input));
	}

	template <typename T>
	void operator|(waavs::ProducerFn<T> producer, waavs::ConsumerFn<T> consumer)
	{
		T val;
		while (producer(val)) {
			consumer(val);
		}
	}
}

*/

