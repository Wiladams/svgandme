#pragma once

#include "memiter.h"

namespace waavs {
    // Core `OcSpan` class with minimal functionality
    struct OcSpan {
        using iter = MemIter<uint8_t>;
        using const_iter = MemIter<const uint8_t>;

    private:
        const uint8_t* fData;
        std::size_t fSize;

    public:
        OcSpan() noexcept : fData(nullptr), fSize(0) {}

		explicit OcSpan(const char* str) noexcept
			: fData(reinterpret_cast<const uint8_t*>(str)), fSize(strlen(str)) {
		}

        explicit OcSpan(const uint8_t* data, std::size_t size) noexcept
            : fData(data), fSize(size) {
        }

        template <std::size_t N>
        explicit OcSpan(const uint8_t(&arr)[N]) noexcept
            : fData(arr), fSize(N) {
        }

        //explicit OcSpan(const std::vector<uint8_t>& vec) noexcept
        //    : fData(vec.data()), fSize(vec.size()) {
        //}

        // Iterator functions
        iter begin() const noexcept { return iter(fData); }
        iter end() const noexcept { return iter(fData + fSize); }

        const_iter cbegin() const noexcept { return const_iter(fData); }
        const_iter cend() const noexcept { return const_iter(fData + fSize); }

        // Access functions
        const uint8_t& operator[](std::size_t index) const noexcept { return fData[index]; }
        const uint8_t* data() const noexcept { return fData; }
        std::size_t size() const noexcept { return fSize; }
        bool empty() const noexcept { return fSize == 0; }
    };
}


