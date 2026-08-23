#pragma once

// benchmark_ws_namemap.cpp

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <random>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "core_nametable.h"
#include "core_table.h"


namespace waavs::bench
{
    using Clock = std::chrono::steady_clock;

    // Prevents the compiler from discarding benchmark results.
    static volatile uint64_t gBenchmarkSink = 0;

    struct BenchmarkResult
    {
        double nanosecondsPerOperation{};
        uint64_t checksum{};
    };


    //=========================================================================
    // makeNames()
    //
    // Location:
    //   Benchmark data setup.
    //
    // Creates distinct names and interns them before timing begins. Interning
    // time is intentionally excluded so this benchmark measures only the maps.
    //=========================================================================

    static std::vector<InternedKey> makeNames(
        const char* prefix,
        size_t count)
    {
        std::vector<InternedKey> keys;
        keys.reserve(count);

        for (size_t i = 0; i < count; ++i)
        {
            std::string text =
                std::string(prefix) +
                "-" +
                std::to_string(i) +
                "-suffix-" +
                std::to_string(
                    static_cast<uint32_t>(i * 2654435761u));

            keys.push_back(
                WSNameSet::INTERN(text.c_str()));
        }

        return keys;
    }


    //=========================================================================
    // shuffledIndices()
    //
    // Location:
    //   Benchmark data setup.
    //
    // Avoids testing only sequential insertion and lookup orders.
    //=========================================================================

    static std::vector<size_t> shuffledIndices(
        size_t count,
        uint32_t seed)
    {
        std::vector<size_t> indices(count);
        std::iota(indices.begin(), indices.end(), size_t{ 0 });

        std::mt19937 random(seed);
        std::shuffle(indices.begin(), indices.end(), random);

        return indices;
    }


    //=========================================================================
    // measure()
    //
    // Location:
    //   Benchmark timing support.
    //
    // The callback returns a checksum so observable work leaves the timed
    // section and cannot easily be optimized away.
    //=========================================================================

    template<typename Fn>
    static BenchmarkResult measure(
        size_t operationCount,
        Fn&& fn)
    {
        const auto start = Clock::now();
        const uint64_t checksum = fn();
        const auto finish = Clock::now();

        const auto elapsed =
            std::chrono::duration_cast<
            std::chrono::nanoseconds>(
                finish - start)
            .count();

        gBenchmarkSink ^= checksum;

        return {
            operationCount
                ? static_cast<double>(elapsed) /
                    static_cast<double>(operationCount)
                : 0.0,
            checksum
        };
    }


    //=========================================================================
    // WSNameMapAdapter
    //
    // Locations exercised:
    //   WSNameMap::reserve()
    //   WSNameMap::put()
    //   WSNameMap::getRef()
    //   WSNameMap::remove()
    //   WSNameMap::forEach()
    //=========================================================================

    template<typename ValueT>
    class WSNameMapAdapter
    {
        WSNameMap<ValueT> fMap;

    public:
        bool reserve(size_t count)
        {
            return fMap.reserve(count);
        }

        bool put(InternedKey key, const ValueT& value)
        {
            return fMap.put(key, value);
        }

        const ValueT* find(InternedKey key) const
        {
            return fMap.getRef(key);
        }

        bool remove(InternedKey key)
        {
            return fMap.remove(key);
        }

        template<typename Fn>
        void forEach(Fn&& fn) const
        {
            fMap.forEach(std::forward<Fn>(fn));
        }

        size_t size() const
        {
            return fMap.size();
        }
    };


    //=========================================================================
    // StdInternedMapAdapter
    //
    // Locations exercised:
    //   std::unordered_map::reserve()
    //   std::unordered_map::insert_or_assign()
    //   std::unordered_map::find()
    //   std::unordered_map::erase()
    //
    // Uses the same InternedKey pointer identity as WSNameMap.
    //=========================================================================

    template<typename ValueT>
    class StdInternedMapAdapter
    {
        using Map = std::unordered_map<
            InternedKey,
            ValueT,
            InternedKeyHash,
            InternedKeyEquivalent>;

        Map fMap;

    public:
        bool reserve(size_t count)
        {
            fMap.reserve(count);
            return true;
        }

        bool put(InternedKey key, const ValueT& value)
        {
            fMap.insert_or_assign(key, value);
            return true;
        }

        const ValueT* find(InternedKey key) const
        {
            const auto found = fMap.find(key);

            return found == fMap.end()
                ? nullptr
                : &found->second;
        }

        bool remove(InternedKey key)
        {
            return fMap.erase(key) != 0;
        }

        template<typename Fn>
        void forEach(Fn&& fn) const
        {
            for (const auto& item : fMap)
                fn(item.first, item.second);
        }

        size_t size() const
        {
            return fMap.size();
        }
    };


    //=========================================================================
    // benchmarkInsert()
    //
    // Locations exercised:
    //   WSNameMap::reserve()
    //   WSNameMap::put()
    //
    // A new map is constructed for every repetition.
    //=========================================================================

    template<template<typename> class MapT>
    static BenchmarkResult benchmarkInsert(
        const std::vector<InternedKey>& keys,
        const std::vector<size_t>& insertionOrder,
        size_t repetitions,
        bool reserveFirst)
    {
        const size_t operationCount =
            keys.size() * repetitions;

        return measure(
            operationCount,
            [&]() -> uint64_t
            {
                uint64_t checksum = 0;

                for (size_t repetition = 0;
                    repetition < repetitions;
                    ++repetition)
                {
                    MapT<uint64_t> map;

                    if (reserveFirst)
                    {
                        if (!map.reserve(keys.size()))
                            std::abort();
                    }

                    for (size_t index : insertionOrder)
                    {
                        const uint64_t value =
                            static_cast<uint64_t>(index) +
                            repetition;

                        if (!map.put(keys[index], value))
                            std::abort();
                    }

                    checksum += map.size();
                }

                return checksum;
            });
    }


    //=========================================================================
    // benchmarkSuccessfulLookup()
    //
    // Locations exercised:
    //   WSNameMap::getRef()
    //
    // Every queried key is present.
    //=========================================================================

    template<template<typename> class MapT>
    static BenchmarkResult benchmarkSuccessfulLookup(
        const std::vector<InternedKey>& keys,
        const std::vector<size_t>& lookupOrder,
        size_t repetitions)
    {
        MapT<uint64_t> map;

        if (!map.reserve(keys.size()))
            std::abort();

        for (size_t i = 0; i < keys.size(); ++i)
        {
            if (!map.put(
                keys[i],
                static_cast<uint64_t>(i + 1)))
            {
                std::abort();
            }
        }

        const size_t operationCount =
            keys.size() * repetitions;

        return measure(
            operationCount,
            [&]() -> uint64_t
            {
                uint64_t checksum = 0;

                for (size_t repetition = 0;
                    repetition < repetitions;
                    ++repetition)
                {
                    for (size_t index : lookupOrder)
                    {
                        const uint64_t* value =
                            map.find(keys[index]);

                        if (!value)
                            std::abort();

                        checksum += *value;
                    }
                }

                return checksum;
            });
    }


    //=========================================================================
    // benchmarkFailedLookup()
    //
    // Locations exercised:
    //   WSNameMap::getRef()
    //
    // None of the queried keys are present.
    //=========================================================================

    template<template<typename> class MapT>
    static BenchmarkResult benchmarkFailedLookup(
        const std::vector<InternedKey>& presentKeys,
        const std::vector<InternedKey>& absentKeys,
        const std::vector<size_t>& lookupOrder,
        size_t repetitions)
    {
        MapT<uint64_t> map;

        if (!map.reserve(presentKeys.size()))
            std::abort();

        for (size_t i = 0; i < presentKeys.size(); ++i)
        {
            if (!map.put(
                presentKeys[i],
                static_cast<uint64_t>(i + 1)))
            {
                std::abort();
            }
        }

        const size_t operationCount =
            absentKeys.size() * repetitions;

        return measure(
            operationCount,
            [&]() -> uint64_t
            {
                uint64_t checksum = 0;

                for (size_t repetition = 0;
                    repetition < repetitions;
                    ++repetition)
                {
                    for (size_t index : lookupOrder)
                    {
                        const uint64_t* value =
                            map.find(absentKeys[index]);

                        checksum +=
                            static_cast<uint64_t>(value != nullptr);
                    }
                }

                return checksum;
            });
    }


    //=========================================================================
    // benchmarkReplacement()
    //
    // Locations exercised:
    //   WSNameMap::put()
    //
    // All keys already exist. This is useful for measuring the current
    // maybeGrow-before-probe behavior separately from insertion.
    //=========================================================================

    template<template<typename> class MapT>
    static BenchmarkResult benchmarkReplacement(
        const std::vector<InternedKey>& keys,
        const std::vector<size_t>& updateOrder,
        size_t repetitions)
    {
        MapT<uint64_t> map;

        if (!map.reserve(keys.size()))
            std::abort();

        for (size_t i = 0; i < keys.size(); ++i)
        {
            if (!map.put(
                keys[i],
                static_cast<uint64_t>(i)))
            {
                std::abort();
            }
        }

        const size_t operationCount =
            keys.size() * repetitions;

        return measure(
            operationCount,
            [&]() -> uint64_t
            {
                uint64_t checksum = 0;

                for (size_t repetition = 0;
                    repetition < repetitions;
                    ++repetition)
                {
                    for (size_t index : updateOrder)
                    {
                        const uint64_t value =
                            static_cast<uint64_t>(
                                repetition + index + 1);

                        if (!map.put(keys[index], value))
                            std::abort();

                        checksum += value;
                    }
                }

                return checksum;
            });
    }


    //=========================================================================
    // benchmarkRemove()
    //
    // Locations exercised:
    //   WSNameMap::remove()
    //
    // Rebuilds the map for each repetition, then times only the removal pass.
    // Map construction occurs outside the individually measured removal pass.
    //=========================================================================

    template<template<typename> class MapT>
    static BenchmarkResult benchmarkRemove(
        const std::vector<InternedKey>& keys,
        const std::vector<size_t>& removalOrder,
        size_t repetitions)
    {
        uint64_t checksum = 0;
        int64_t elapsedNanoseconds = 0;

        for (size_t repetition = 0;
            repetition < repetitions;
            ++repetition)
        {
            MapT<uint64_t> map;

            if (!map.reserve(keys.size()))
                std::abort();

            for (size_t i = 0; i < keys.size(); ++i)
            {
                if (!map.put(
                    keys[i],
                    static_cast<uint64_t>(i)))
                {
                    std::abort();
                }
            }

            const auto start = Clock::now();

            for (size_t index : removalOrder)
            {
                if (!map.remove(keys[index]))
                    std::abort();
            }

            const auto finish = Clock::now();

            elapsedNanoseconds +=
                std::chrono::duration_cast<
                std::chrono::nanoseconds>(
                    finish - start)
                .count();

            checksum += map.size();
        }

        gBenchmarkSink ^= checksum;

        return {
            static_cast<double>(elapsedNanoseconds) /
                static_cast<double>(
                    keys.size() * repetitions),
            checksum
        };
    }


    //=========================================================================
    // benchmarkIteration()
    //
    // Locations exercised:
    //   WSNameMap::forEach()
    //=========================================================================

    template<template<typename> class MapT>
    static BenchmarkResult benchmarkIteration(
        const std::vector<InternedKey>& keys,
        size_t repetitions)
    {
        MapT<uint64_t> map;

        if (!map.reserve(keys.size()))
            std::abort();

        for (size_t i = 0; i < keys.size(); ++i)
        {
            if (!map.put(
                keys[i],
                static_cast<uint64_t>(i + 1)))
            {
                std::abort();
            }
        }

        const size_t operationCount =
            keys.size() * repetitions;

        return measure(
            operationCount,
            [&]() -> uint64_t
            {
                uint64_t checksum = 0;

                for (size_t repetition = 0;
                    repetition < repetitions;
                    ++repetition)
                {
                    map.forEach(
                        [&](InternedKey, const uint64_t& value)
                        {
                            checksum += value;
                        });
                }

                return checksum;
            });
    }


    static void printComparison(
        const char* operation,
        BenchmarkResult wsResult,
        BenchmarkResult stdResult)
    {
        const double improvementPercent =
            stdResult.nanosecondsPerOperation > 0.0
            ? 100.0 *
            (stdResult.nanosecondsPerOperation -
                wsResult.nanosecondsPerOperation) /
            stdResult.nanosecondsPerOperation
            : 0.0;

        std::printf(
            "%-24s %12.3f %12.3f %11.1f%%\n",
            operation,
            wsResult.nanosecondsPerOperation,
            stdResult.nanosecondsPerOperation,
            improvementPercent);
    }


    //=========================================================================
    // runSizeBenchmark()
    //
    // Location:
    //   Benchmark orchestration for one map size.
    //=========================================================================

    static void runSizeBenchmark(size_t keyCount)
    {
        const std::vector<InternedKey> presentKeys =
            makeNames("present-key", keyCount);

        const std::vector<InternedKey> absentKeys =
            makeNames("absent-key", keyCount);

        const std::vector<size_t> order =
            shuffledIndices(
                keyCount,
                static_cast<uint32_t>(
                    0x41C64E6Du ^ keyCount));

        // Keep total operations reasonably similar across table sizes.
        const size_t lookupRepetitions =
            std::max<size_t>(
                10,
                4000000 / keyCount);

        const size_t constructionRepetitions =
            std::max<size_t>(
                5,
                500000 / keyCount);

        const size_t removalRepetitions =
            std::max<size_t>(
                5,
                300000 / keyCount);

        std::printf(
            "\nEntries: %zu\n"
            "%-24s %12s %12s %12s\n",
            keyCount,
            "Operation",
            "WS ns/op",
            "std ns/op",
            "Improvement");

        printComparison(
            "insert, reserved",
            benchmarkInsert<WSNameMapAdapter>(
                presentKeys,
                order,
                constructionRepetitions,
                true),
            benchmarkInsert<StdInternedMapAdapter>(
                presentKeys,
                order,
                constructionRepetitions,
                true));

        printComparison(
            "insert, no reserve",
            benchmarkInsert<WSNameMapAdapter>(
                presentKeys,
                order,
                constructionRepetitions,
                false),
            benchmarkInsert<StdInternedMapAdapter>(
                presentKeys,
                order,
                constructionRepetitions,
                false));

        printComparison(
            "successful lookup",
            benchmarkSuccessfulLookup<WSNameMapAdapter>(
                presentKeys,
                order,
                lookupRepetitions),
            benchmarkSuccessfulLookup<StdInternedMapAdapter>(
                presentKeys,
                order,
                lookupRepetitions));

        printComparison(
            "failed lookup",
            benchmarkFailedLookup<WSNameMapAdapter>(
                presentKeys,
                absentKeys,
                order,
                lookupRepetitions),
            benchmarkFailedLookup<StdInternedMapAdapter>(
                presentKeys,
                absentKeys,
                order,
                lookupRepetitions));

        printComparison(
            "replace existing",
            benchmarkReplacement<WSNameMapAdapter>(
                presentKeys,
                order,
                lookupRepetitions),
            benchmarkReplacement<StdInternedMapAdapter>(
                presentKeys,
                order,
                lookupRepetitions));

        printComparison(
            "remove all",
            benchmarkRemove<WSNameMapAdapter>(
                presentKeys,
                order,
                removalRepetitions),
            benchmarkRemove<StdInternedMapAdapter>(
                presentKeys,
                order,
                removalRepetitions));

        printComparison(
            "iterate",
            benchmarkIteration<WSNameMapAdapter>(
                presentKeys,
                lookupRepetitions),
            benchmarkIteration<StdInternedMapAdapter>(
                presentKeys,
                lookupRepetitions));
    }


    void runWSNameMapPerformanceComparison()
    {
        std::printf(
            "WSNameMap vs std::unordered_map<InternedKey, uint64_t>\n"
            "Times are nanoseconds per logical operation.\n"
            "WS/std below 1.0 means WSNameMap is faster.\n");

        constexpr size_t sizes[] = {
            4,
            8,
            16,
            32,
            64,
            128,
            256,
            1024,
            4096
        };

        for (size_t size : sizes)
            runSizeBenchmark(size);

        std::printf(
            "\nBenchmark sink: %llu\n",
            static_cast<unsigned long long>(
                gBenchmarkSink));
    }
}

