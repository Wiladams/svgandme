#pragma once

#include <memory>
#include <deque>
#include <functional>

namespace waavs {
	/*
		Topic facilitates the publisher/subscriber pattern.
		When you want to know when something happens, you
		subscribe() to a topic.

		Whatever is responsible for signaling the event
		will call the notify() function of the topic, and the
		subscribed function will be called.

		The Topic does not incoroporate any threading model
		A single topic is not a whole pub/sub system
		Multiple topics are meant to be managed together to create
		a pub/sub system.

		Doing it this way allows for different forms of composition and
		general topic management.

		T - The event payload, this is the type of data that will be
		sent when a subscriber is notified.

		The subscriber is a functor, that is, anything that has the
		function signature.  It can be an object, or a function pointer,
		essentially anything that resolves as std::function<void>(const T&)

		This is a very nice pure type with no dependencies outside
		the standard template library, and <functional>
	*/
	template <typename T>
	class Topic
	{
	public:
		// This is the form of subscriber
		using Subscriber = std::function<void(const T& m)>;

	private:
		std::deque<Subscriber> fSubscribers{};

	public:
		constexpr Topic<T>() = default;

		Topic<T>(const Topic<T>& other) noexcept
		{
			fSubscribers = other.fSubscribers;
		}

		virtual ~Topic() = default;

		// Notify subscribers that an event has occured
		virtual void notify(const T& m) noexcept
		{
			for (Subscriber& it : fSubscribers) {
				//it(m, this);
				it(m);
			}
		}

		// Add a subscriber to the list of subscribers
		virtual void subscribe(Subscriber s) noexcept
		{
			fSubscribers.push_back(s);
		}

		// Remove a subscriber from the list of subscribers
		virtual void unsubscribe(Subscriber s) noexcept
		{
			// BUGBUG
			// find the subscriber that matches
		}
	};

}