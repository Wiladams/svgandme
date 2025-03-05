#pragma once

#include <iterator>	// for std::input_iterator_tag
#include <cstddef>	// for std::ptrdiff_t
#include <algorithm>	// for std::ranges::filter_view
#include <ranges>
#include <vector>

namespace waavs {
	// C++11-compatible iterator
	template <typename T>
	struct MemIter {
		using iterator_category = std::random_access_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using reference = T&;

		const T* fPtr;

		MemIter() noexcept : fPtr(nullptr) {}
		explicit MemIter(const T* ptr) noexcept : fPtr(ptr) {}

		// Dereferencing operators
		T operator*() const noexcept { return *fPtr; }
		const T* operator->() const noexcept { return fPtr; }

		// Increment & Decrement
		MemIter& operator++() noexcept { ++fPtr; return *this; }
		MemIter operator++(int) noexcept { MemIter tmp(*this); ++fPtr; return tmp; }
		MemIter& operator--() noexcept { --fPtr; return *this; }
		MemIter operator--(int) noexcept { MemIter tmp(*this); --fPtr; return tmp; }

		// Arithmetic operators
		MemIter& operator+=(difference_type n) noexcept { fPtr += n; return *this; }
		MemIter& operator-=(difference_type n) noexcept { fPtr -= n; return *this; }
		MemIter operator+(difference_type n) const noexcept { return MemIter(fPtr + n); }
		MemIter operator-(difference_type n) const noexcept { return MemIter(fPtr - n); }
		difference_type operator-(const MemIter& b) const noexcept { return fPtr - b.fPtr; }

		// Comparison operators
		bool operator==(const MemIter& b) const noexcept { return fPtr == b.fPtr; }
		bool operator!=(const MemIter& b) const noexcept { return fPtr != b.fPtr; }
		bool operator<(const MemIter& b) const noexcept { return fPtr < b.fPtr; }
		bool operator<=(const MemIter& b) const noexcept { return fPtr <= b.fPtr; }
		bool operator>(const MemIter& b) const noexcept { return fPtr > b.fPtr; }
		bool operator>=(const MemIter& b) const noexcept { return fPtr >= b.fPtr; }
	};


}

namespace waavs {

	template <typename T>
	struct MemContainer {
		using iter = MemIter<T>;
		using const_iter = MemIter<const T>;

	private:
		const T* fData{ nullptr };
		std::size_t fSize{ 0 };

	public:
		MemContainer() noexcept = default;

		explicit MemContainer(const T* data, std::size_t size) noexcept
			: fData(data), fSize(size) {
		}

		template <std::size_t N>
		explicit MemContainer(const T(&arr)[N]) noexcept
			: fData(arr), fSize(N) {
		}

		explicit MemContainer(const std::vector<T>& vec) noexcept
			: fData(vec.data()), fSize(vec.size()) {
		}

		// Iterator functions
		iter begin() const noexcept { return iter(fData); }
		iter end() const noexcept { return iter(fData + fSize); }

		const_iter cbegin() const noexcept { return const_iter(fData); }
		const_iter cend() const noexcept { return const_iter(fData + fSize); }

		// Access functions
		const T& operator[](std::size_t index) const noexcept { return fData[index]; }
		const T* data() const noexcept { return fData; }
		std::size_t size() const noexcept { return fSize; }
		bool empty() const noexcept { return fSize == 0; }
	};

}