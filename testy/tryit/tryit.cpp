// test_ws_namemap.cpp

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "core_nametable.h"
#include "core_table.h"
#include "benchmark_ws_namemap.h"

namespace waavs::tests
{
    static constexpr uint32_t kDefaultSeed = 0x5A17C0DEu;
    static constexpr size_t kNameCount = 512;
    static constexpr size_t kOperationCount = 250000;


    //=========================================================================
    // fail()
    //
    // Location:
    //   Test support code.
    //
    // Uses abort() instead of assert(), so tests remain enabled when NDEBUG
    // is defined.
    //=========================================================================

    [[noreturn]]
    static void fail(
        const char* message,
        size_t operationIndex,
        const std::string& key = {})
    {
        std::fprintf(
            stderr,
            "\nWSNameMap correctness failure\n"
            "  operation: %zu\n"
            "  message:   %s\n",
            operationIndex,
            message);

        if (!key.empty()) {
            std::fprintf(
                stderr,
                "  key:       \"%s\"\n",
                key.c_str());
        }

        std::abort();
    }


    //=========================================================================
    // makeTestNames()
    //
    // Location:
    //   Test data generation.
    //
    // Generates unique text names. These strings are retained for the whole
    // test and are also used as keys in the reference unordered_map.
    //=========================================================================

    static std::vector<std::string> makeTestNames(size_t count)
    {
        std::vector<std::string> result;
        result.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            const uint32_t suffix =
                static_cast<uint32_t>(i) * 2654435761u;

            result.emplace_back(
                "test-name-" +
                std::to_string(i) +
                "-suffix-" +
                std::to_string(suffix & 0xffffu));
        }

        return result;
    }


    //=========================================================================
    // makeInternedKeys()
    //
    // Exercises:
    //   WSNameSet::INTERN(const char*)
    //
    // Converts every test string into its canonical interned pointer.
    //=========================================================================

    static std::vector<InternedKey> makeInternedKeys(
        const std::vector<std::string>& names)
    {
        std::vector<InternedKey> result;
        result.reserve(names.size());

        for (const std::string& name : names) {
            InternedKey key =
                WSNameSet::INTERN(name.c_str());

            if (!key) {
                fail(
                    "WSNameSet::INTERN() returned nullptr",
                    result.size(),
                    name);
            }

            result.push_back(key);
        }

        return result;
    }


    //=========================================================================
    // validateOne()
    //
    // Exercises:
    //   WSNameMap::getRef(Key) const
    //   WSNameMap::contains(Key) const
    //
    // Compares one key against the std::unordered_map reference model.
    //=========================================================================

    static void validateOne(
        const WSNameMap<int>& table,
        const std::unordered_map<std::string, int>& reference,
        const std::string& text,
        InternedKey key,
        size_t operationIndex)
    {
        const auto referenceIt =
            reference.find(text);

        const int* actual =
            table.getRef(key);

        const bool actualContains =
            table.contains(key);

        const bool expectedContains =
            referenceIt != reference.end();

        if (actualContains != expectedContains) {
            fail(
                "contains() differs from reference",
                operationIndex,
                text);
        }

        if (!expectedContains) {
            if (actual != nullptr) {
                fail(
                    "getRef() returned a value for an absent key",
                    operationIndex,
                    text);
            }

            return;
        }

        if (actual == nullptr) {
            fail(
                "getRef() failed for a present key",
                operationIndex,
                text);
        }

        if (*actual != referenceIt->second) {
            std::fprintf(
                stderr,
                "  expected value: %d\n"
                "  actual value:   %d\n",
                referenceIt->second,
                *actual);

            fail(
                "getRef() returned the wrong value",
                operationIndex,
                text);
        }

        // Also exercise WSNameMap::get().
        int copiedValue = 0;

        if (!table.get(key, copiedValue)) {
            fail(
                "get() failed for a present key",
                operationIndex,
                text);
        }

        if (copiedValue != referenceIt->second) {
            fail(
                "get() copied the wrong value",
                operationIndex,
                text);
        }
    }


    //=========================================================================
    // validateAll()
    //
    // Exercises:
    //   WSNameMap::size()
    //   WSNameMap::getRef()
    //   WSNameMap::get()
    //   WSNameMap::contains()
    //
    // Validates all known keys, including keys absent from the table.
    //=========================================================================

    static void validateAll(
        const WSNameMap<int>& table,
        const std::unordered_map<std::string, int>& reference,
        const std::vector<std::string>& names,
        const std::vector<InternedKey>& keys,
        size_t operationIndex)
    {
        if (table.size() != reference.size()) {
            std::fprintf(
                stderr,
                "  expected size: %zu\n"
                "  actual size:   %zu\n",
                reference.size(),
                table.size());

            fail(
                "size() differs from reference",
                operationIndex);
        }

        for (size_t i = 0; i < names.size(); ++i) {
            validateOne(
                table,
                reference,
                names[i],
                keys[i],
                operationIndex);
        }
    }


    //=========================================================================
    // testInterning()
    //
    // Exercises:
    //   WSNameSet::INTERN()
    //   WSNameSet::FIND()
    //
    // Confirms that identical text produces identical pointer identities.
    //=========================================================================

    static void testInterning(
        const std::vector<std::string>& names,
        const std::vector<InternedKey>& keys)
    {
        for (size_t i = 0; i < names.size(); ++i) {
            InternedKey again =
                WSNameSet::INTERN(names[i].c_str());

            if (again != keys[i]) {
                fail(
                    "INTERN() did not return the same canonical pointer",
                    i,
                    names[i]);
            }

            InternedKey found =
                WSNameSet::FIND(names[i].c_str());

            if (found != keys[i]) {
                fail(
                    "FIND() did not return the canonical pointer",
                    i,
                    names[i]);
            }
        }

        const char* absentName =
            "name-that-was-never-interned-cc928dd2";

        if (WSNameSet::FIND(absentName) != nullptr) {
            fail(
                "FIND() returned a pointer for an absent name",
                0,
                absentName);
        }
    }


    //=========================================================================
    // testPutAndReplace()
    //
    // Exercises:
    //   WSNameMap::put(Key, const ValueT&)
    //   WSNameMap::getRef()
    //   WSNameMap::size()
    //
    // Confirms that put() inserts new keys and replaces existing values without
    // increasing the table size.
    //=========================================================================

    static void testPutAndReplace(
        const std::vector<std::string>& names,
        const std::vector<InternedKey>& keys)
    {
        WSNameMap<int> table;

        for (size_t i = 0; i < keys.size(); ++i) {
            const int value =
                static_cast<int>(i);

            if (!table.put(keys[i], value)) {
                fail(
                    "put() failed during initial insertion",
                    i,
                    names[i]);
            }
        }

        if (table.size() != keys.size()) {
            fail(
                "size() is wrong after initial insertion",
                keys.size());
        }

        for (size_t i = 0; i < keys.size(); ++i) {
            const int replacement =
                static_cast<int>(100000 + i);

            if (!table.put(keys[i], replacement)) {
                fail(
                    "put() failed while replacing a value",
                    i,
                    names[i]);
            }
        }

        if (table.size() != keys.size()) {
            fail(
                "replacing values changed the table size",
                keys.size());
        }

        for (size_t i = 0; i < keys.size(); ++i) {
            const int* actual =
                table.getRef(keys[i]);

            const int expected =
                static_cast<int>(100000 + i);

            if (!actual || *actual != expected) {
                fail(
                    "put() replacement stored the wrong value",
                    i,
                    names[i]);
            }
        }
    }


    //=========================================================================
    // testRvaluePut()
    //
    // Exercises:
    //   WSNameMap::put(Key, ValueT&&)
    //
    // Uses std::string so the rvalue overload is exercised with a nontrivial
    // movable type.
    //=========================================================================

    static void testRvaluePut(
        const std::vector<std::string>& names,
        const std::vector<InternedKey>& keys)
    {
        WSNameMap<std::string> table;

        const size_t count =
            keys.size() < 64 ? keys.size() : 64;

        for (size_t i = 0; i < count; ++i) {
            std::string value =
                "value-" + std::to_string(i);

            if (!table.put(keys[i], std::move(value))) {
                fail(
                    "rvalue put() failed",
                    i,
                    names[i]);
            }
        }

        for (size_t i = 0; i < count; ++i) {
            const std::string* actual =
                table.getRef(keys[i]);

            const std::string expected =
                "value-" + std::to_string(i);

            if (!actual || *actual != expected) {
                fail(
                    "rvalue put() stored the wrong value",
                    i,
                    names[i]);
            }
        }
    }


    //=========================================================================
    // testReserve()
    //
    // Exercises:
    //   WSNameMap::reserve()
    //   WSNameMap::put()
    //   WSNameMap::getRef()
    //
    // Confirms that reserve() preserves existing entries through rehashing.
    //=========================================================================

    static void testReserve(
        const std::vector<std::string>& names,
        const std::vector<InternedKey>& keys)
    {
        WSNameMap<int> table;

        if (!table.reserve(keys.size())) {
            fail(
                "initial reserve() failed",
                0);
        }

        for (size_t i = 0; i < keys.size(); ++i) {
            if (!table.put(
                keys[i],
                static_cast<int>(i)))
            {
                fail(
                    "put() failed after reserve()",
                    i,
                    names[i]);
            }
        }

        if (!table.reserve(keys.size() * 4)) {
            fail(
                "reserve() failed with existing entries",
                keys.size());
        }

        for (size_t i = 0; i < keys.size(); ++i) {
            const int* actual =
                table.getRef(keys[i]);

            if (!actual ||
                *actual != static_cast<int>(i))
            {
                fail(
                    "reserve() corrupted an existing entry",
                    i,
                    names[i]);
            }
        }
    }


    //=========================================================================
    // testClear()
    //
    // Exercises:
    //   WSNameMap::clear()
    //   WSNameMap::size()
    //   WSNameMap::empty()
    //   WSNameMap::contains()
    //   WSNameMap::getRef()
    //=========================================================================

    static void testClear(
        const std::vector<std::string>& names,
        const std::vector<InternedKey>& keys)
    {
        WSNameMap<int> table;

        for (size_t i = 0; i < keys.size(); ++i) {
            table.put(
                keys[i],
                static_cast<int>(i));
        }

        table.clear();

        if (table.size() != 0) {
            fail(
                "clear() did not reset size()",
                0);
        }

        if (!table.empty()) {
            fail(
                "empty() returned false after clear()",
                0);
        }

        for (size_t i = 0; i < keys.size(); ++i) {
            if (table.contains(keys[i])) {
                fail(
                    "contains() found a key after clear()",
                    i,
                    names[i]);
            }

            if (table.getRef(keys[i]) != nullptr) {
                fail(
                    "getRef() found a key after clear()",
                    i,
                    names[i]);
            }
        }
    }


    //=========================================================================
    // testRemoveForward()
    //
    // Exercises:
    //   WSNameMap::remove()
    //   WSNameMap::getRef()
    //   WSNameMap::size()
    //
    // Removes entries in insertion order and verifies all remaining entries
    // after each removal.
    //=========================================================================

    static void testRemoveForward(
        const std::vector<std::string>& names,
        const std::vector<InternedKey>& keys)
    {
        WSNameMap<int> table;

        for (size_t i = 0; i < keys.size(); ++i) {
            table.put(
                keys[i],
                static_cast<int>(i));
        }

        for (size_t i = 0; i < keys.size(); ++i) {
            if (!table.remove(keys[i])) {
                fail(
                    "remove() failed in forward order",
                    i,
                    names[i]);
            }

            if (table.getRef(keys[i]) != nullptr) {
                fail(
                    "removed key remained findable",
                    i,
                    names[i]);
            }

            const size_t expectedSize =
                keys.size() - i - 1;

            if (table.size() != expectedSize) {
                fail(
                    "size() is wrong after remove()",
                    i,
                    names[i]);
            }

            for (size_t j = i + 1; j < keys.size(); ++j) {
                const int* actual =
                    table.getRef(keys[j]);

                if (!actual ||
                    *actual != static_cast<int>(j))
                {
                    fail(
                        "remove() made another key unreachable",
                        i,
                        names[j]);
                }
            }
        }
    }


    //=========================================================================
    // testRemoveReverse()
    //
    // Exercises:
    //   WSNameMap::remove()
    //   WSNameMap::getRef()
    //
    // Removes entries in reverse insertion order.
    //=========================================================================

    static void testRemoveReverse(
        const std::vector<std::string>& names,
        const std::vector<InternedKey>& keys)
    {
        WSNameMap<int> table;

        for (size_t i = 0; i < keys.size(); ++i) {
            table.put(
                keys[i],
                static_cast<int>(i));
        }

        for (size_t i = keys.size(); i-- > 0;) {
            if (!table.remove(keys[i])) {
                fail(
                    "remove() failed in reverse order",
                    i,
                    names[i]);
            }

            for (size_t j = 0; j < i; ++j) {
                const int* actual =
                    table.getRef(keys[j]);

                if (!actual ||
                    *actual != static_cast<int>(j))
                {
                    fail(
                        "reverse remove made another key unreachable",
                        i,
                        names[j]);
                }
            }
        }

        if (!table.empty()) {
            fail(
                "table is not empty after reverse removal",
                keys.size());
        }
    }


    //=========================================================================
    // testRemoveAlternating()
    //
    // Exercises:
    //   WSNameMap::remove()
    //   WSNameMap::getRef()
    //
    // Removes even-indexed entries first, producing holes throughout clusters.
    //=========================================================================

    static void testRemoveAlternating(
        const std::vector<std::string>& names,
        const std::vector<InternedKey>& keys)
    {
        WSNameMap<int> table;

        for (size_t i = 0; i < keys.size(); ++i) {
            table.put(
                keys[i],
                static_cast<int>(i));
        }

        for (size_t i = 0; i < keys.size(); i += 2) {
            if (!table.remove(keys[i])) {
                fail(
                    "remove() failed for an even-indexed key",
                    i,
                    names[i]);
            }
        }

        for (size_t i = 1; i < keys.size(); i += 2) {
            const int* actual =
                table.getRef(keys[i]);

            if (!actual ||
                *actual != static_cast<int>(i))
            {
                fail(
                    "removing even entries damaged an odd entry",
                    i,
                    names[i]);
            }
        }

        for (size_t i = 0; i < keys.size(); i += 2) {
            if (table.remove(keys[i])) {
                fail(
                    "remove() returned true for an absent key",
                    i,
                    names[i]);
            }
        }

        for (size_t i = 1; i < keys.size(); i += 2) {
            if (!table.remove(keys[i])) {
                fail(
                    "remove() failed for an odd-indexed key",
                    i,
                    names[i]);
            }
        }

        if (!table.empty()) {
            fail(
                "alternating removal did not empty the table",
                keys.size());
        }
    }


    //=========================================================================
    // runRandomizedDifferentialTest()
    //
    // Exercises:
    //   WSNameMap::put()
    //   WSNameMap::get()
    //   WSNameMap::getRef()
    //   WSNameMap::contains()
    //   WSNameMap::remove()
    //   WSNameMap::reserve()
    //   WSNameMap::clear()
    //   WSNameMap::size()
    //
    // Runs the same operations against WSNameMap<int> and
    // std::unordered_map<std::string, int>.
    //=========================================================================

    static void runRandomizedDifferentialTest(
        const std::vector<std::string>& names,
        const std::vector<InternedKey>& keys,
        uint32_t seed,
        size_t operationCount)
    {
        WSNameMap<int> table;
        std::unordered_map<std::string, int> reference;

        std::mt19937 random(seed);

        std::uniform_int_distribution<size_t> nameDistribution(
            0,
            names.size() - 1);

        std::uniform_int_distribution<int> valueDistribution(
            -1000000,
            1000000);

        std::uniform_int_distribution<int> operationDistribution(
            0,
            99);

        std::uniform_int_distribution<size_t> reserveDistribution(
            0,
            names.size() * 4);

        for (size_t operationIndex = 0;
            operationIndex < operationCount;
            ++operationIndex)
        {
            const size_t nameIndex =
                nameDistribution(random);

            const std::string& text =
                names[nameIndex];

            const InternedKey key =
                keys[nameIndex];

            const int operation =
                operationDistribution(random);

            if (operation < 40) {
                // Location exercised:
                //   WSNameMap::put()

                const int value =
                    valueDistribution(random);

                if (!table.put(key, value)) {
                    fail(
                        "put() reported failure",
                        operationIndex,
                        text);
                }

                reference[text] = value;
            }
            else if (operation < 65) {
                // Locations exercised:
                //   WSNameMap::get()
                //   WSNameMap::getRef()
                //   WSNameMap::contains()

                validateOne(
                    table,
                    reference,
                    text,
                    key,
                    operationIndex);
            }
            else if (operation < 85) {
                // Location exercised:
                //   WSNameMap::remove()

                const bool actualRemoved =
                    table.remove(key);

                const bool expectedRemoved =
                    reference.erase(text) != 0;

                if (actualRemoved != expectedRemoved) {
                    fail(
                        "remove() result differs from reference",
                        operationIndex,
                        text);
                }

                validateOne(
                    table,
                    reference,
                    text,
                    key,
                    operationIndex);

                // Check unrelated keys immediately after a removal.
                for (size_t check = 0; check < 8; ++check) {
                    const size_t checkIndex =
                        nameDistribution(random);

                    validateOne(
                        table,
                        reference,
                        names[checkIndex],
                        keys[checkIndex],
                        operationIndex);
                }
            }
            else if (operation < 92) {
                // Location exercised:
                //   WSNameMap::reserve()

                const size_t requested =
                    reserveDistribution(random);

                if (!table.reserve(requested)) {
                    fail(
                        "reserve() reported failure",
                        operationIndex);
                }
            }
            else if (operation < 95) {
                // Location exercised:
                //   WSNameMap::clear()

                table.clear();
                reference.clear();

                if (!table.empty()) {
                    fail(
                        "clear() did not leave the table empty",
                        operationIndex);
                }
            }
            else {
                validateAll(
                    table,
                    reference,
                    names,
                    keys,
                    operationIndex);
            }

            // Full validation every 1024 operations.
            if ((operationIndex & 0x3ffu) == 0) {
                validateAll(
                    table,
                    reference,
                    names,
                    keys,
                    operationIndex);
            }
        }

        validateAll(
            table,
            reference,
            names,
            keys,
            operationCount);
    }


    //=========================================================================
    // runWSNameMapCorrectnessTests()
    //
    // Public entry point.
    //=========================================================================

    void runWSNameMapCorrectnessTests()
    {
        const std::vector<std::string> names =
            makeTestNames(kNameCount);

        const std::vector<InternedKey> keys =
            makeInternedKeys(names);

        testInterning(names, keys);
        testPutAndReplace(names, keys);
        testRvaluePut(names, keys);
        testReserve(names, keys);
        testClear(names, keys);
        testRemoveForward(names, keys);
        testRemoveReverse(names, keys);
        testRemoveAlternating(names, keys);

        runRandomizedDifferentialTest(
            names,
            keys,
            kDefaultSeed,
            kOperationCount);

        runRandomizedDifferentialTest(
            names,
            keys,
            0x12345678u,
            kOperationCount);

        runRandomizedDifferentialTest(
            names,
            keys,
            0xDEADBEEFu,
            kOperationCount);

        std::printf(
            "WSNameMap correctness tests passed.\n"
            "  names:               %zu\n"
            "  operations per seed: %zu\n"
            "  randomized seeds:    3\n",
            names.size(),
            kOperationCount);
    }
}


// Remove this main() when integrating the test into an existing test runner.

int main()
{
//    waavs::tests::runWSNameMapCorrectnessTests();
    waavs::bench::runWSNameMapPerformanceComparison();

    return 0;
}