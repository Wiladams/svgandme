#pragma once

#include <vector>
#include <algorithm>
#include <functional>

namespace waavs {
    template <typename T>
    class IParametricSource {
    public:
        virtual ~IParametricSource() = default;

        virtual T eval(double t) const = 0;

        // Makes it callable like a function
        T operator()(double t) const { return eval(t); }
    };

    //struct IParametricSegment 
    //{
    //    double t0 = 0.0;
    //    double t1 = 1.0;
    //};
}

namespace waavs 
{

    // ParametricStopMap
    // Define a fixed set of stops, with offsets between [0..1]
    // Associate values with each of the stops.
    // When eval(t) is called, a value is returned which is 
    // an interpolation between the nearest stops.
	// An easing function can be supplied to control the interpolation
    //
    // Note: Since this implements IParametricSource, it can be used
	// as a curve, and passed to other functions that expect a curve
    //
    template <typename T>
    struct ParametricStopMap : public IParametricSource<T> 
    {
        using EasingFunction = std::function<double(double localT, double globalT)>;

        ParametricStopMap()
            : fEasing([](double localT, double globalT) { return localT; }) // linear by default
        {
        }

        void addStop(double offset, const T& value) {
            //assert(offset >= 0.0 && offset <= 1.0);
            fStops.emplace_back(offset, value);
            fSorted = false;
        }

        void setEasingFunction(EasingFunction easing) {
            fEasing = std::move(easing);
        }

        T eval(double t) const override {
            if (fStops.empty()) return T{};
            if (fStops.size() == 1) return fStops.front().second;

            if (!fSorted)
                const_cast<ParametricStopMap*>(this)->sortStops();

            t = clamp(t, 0.0, 1.0);

            for (size_t i = 1; i < fStops.size(); ++i) {
                const auto& a = fStops[i - 1];
                const auto& b = fStops[i];

                if (t <= b.first) {
                    double range = b.first - a.first;
                    if (range <= 1e-8) return a.second;

                    double localT = (t - a.first) / range;
                    double easedT = fEasing(localT, t);
                    return interpolate(a.second, b.second, easedT);
                }
            }

            return fStops.back().second;
        }

    private:
        std::vector<std::pair<double, T>> fStops;
        bool fSorted = true;
        EasingFunction fEasing;

        static double clamp(double t, double lo, double hi) {
            return t < lo ? lo : (t > hi ? hi : t);
        }

        void sortStops() {
            std::sort(fStops.begin(), fStops.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
            fSorted = true;
        }

		// Interpolation requires the type T to support +,- and * operators
        // The type must support a scalar multiplication
        static T interpolate(const T& a, const T& b, double t) 
        {
            return a + (b - a) * t;
        }
    };
}


