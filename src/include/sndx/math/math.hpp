#pragma once

#include <utility>
#include <cassert>
#include <array>
#include <numeric>
#include <cmath>
#include <limits>
#include <bit>

#include "./easings.hpp"

namespace sndx::math {

	template <class T, class InputIt> [[nodiscard]]
	constexpr T average(InputIt begin, InputIt end) {
		T count = 0;
		T avg = T(0);

		for (auto it = begin; it != end; ++it) {
			++count;
			avg += (*it - avg) / count;
		}

		return avg;
	}

	[[nodiscard]]
	constexpr size_t factorial(size_t n) noexcept {
		assert(n < 123);

		size_t result = 1;

		for (size_t i = 2; i <= n; ++i) {
			result *= i;
		}

		return result;
	}

	// returns the factorials up to (and including) factorial(n)
	template <size_t n> [[nodiscard]]
	constexpr std::array<size_t, n + 1> factorials() noexcept {
		static_assert(n < 123);

		if constexpr (n == 0) {
			return std::array<size_t, 1>{ 1 };
		}
		else {

			std::array<size_t, n + 1> out{ 1, 1 };

			if constexpr (out.size() > 2) {
				for (size_t i = 2; i < out.size(); ++i) {
					out[i] = i * out[i - 1];
				}
			}

			return out;
		}
	}

	// https://en.wikipedia.org/wiki/Binomial_coefficient
	template <std::floating_point T = float>
	constexpr size_t binomialCoefficient(size_t n, size_t k) noexcept {
		assert(k <= n);

		T out = T(1.0);

		auto limit = std::min(k, n - k);
		for (size_t i = 1; i <= limit; ++i) {
			out *= T(n + 1 - i) / T(i);
		}

		return size_t(std::round(out));
	}

	template <size_t n, std::floating_point T = float> [[nodiscard]]
	constexpr std::array<T, n + 1> binomialCoefficients() noexcept {
		constexpr auto facts = factorials<n>();
		constexpr T top = T(facts[n]);

		std::array<T, n + 1> out{};

		for (size_t k = 0; k < out.size(); ++k) {
			out[k] = top / T(facts[k] * facts[n - k]);
		}

		return out;
	}


	[[nodiscard]]
	constexpr size_t fibonacci(size_t n) noexcept {
		if (n == 0) 
			return 0;
		
		if (n <= 2)
			return 1;
		
		size_t cur = 2;
		size_t prev = 1;

		for (size_t i = 3; i < n; ++i) {
			prev = std::exchange(cur, cur + prev);
		}

		return cur;
	}

	// returns an array of fibonacci up to (and including) n
	template <size_t n> [[nodiscard]]
	constexpr std::array<size_t, n + 1> fibonacci() noexcept {
		static_assert(n > 0);

		// https://en.cppreference.com/w/cpp/algorithm/adjacent_difference
		std::array<size_t, n + 1> out{ 0, 1 };

		auto start = std::next(out.begin());

		std::adjacent_difference(
			start, std::prev(out.end()),
			std::next(start), std::plus<>{});

		return out;
	}

	// remaps value from old range to new range
	// performs a inverse lerp followed by a lerp.
	template <class T> [[nodiscard]]
	constexpr T remap(T value, T oldMin, T oldMax, T newMin, T newMax) noexcept {
		auto curPos = invLerp(oldMin, oldMax, value);
		return lerp(newMin, newMax, curPos);
	}

	// similar to remap, but mapping skewed based on centerpoint(s)
	// useful for audio
	// old centerpoint should NOT be the min/max value of that type
	template <std::floating_point T> [[nodiscard]]
	constexpr T remapBalanced(T value, T oldCenterpoint, T newCenterpoint, T oldMin, T oldMax, T newMin, T newMax) noexcept {
		if (value > oldCenterpoint)
			return remap(value, oldCenterpoint, oldMax, newCenterpoint, newMax);
		if (value < oldCenterpoint)
			return remap(value, oldMin, oldCenterpoint, newMin, newCenterpoint);

		return newCenterpoint;
	}

	// remaps value from its type to the output type.
	// since signed types are not centered on 0, beware that 0 may not map to 0
	// but generally, narrowing will preserve 0s.
	template <std::integral O, std::integral I> [[nodiscard]]
	constexpr O remap(I value) noexcept {
		static_assert(!std::is_same_v<I, O>);

		if constexpr (sizeof(I) == sizeof(O)) { // lossless 1-1 mapping
			if constexpr (
				(std::is_signed_v<I> && std::is_signed_v<O>) ||
				(std::is_unsigned_v<I> && std::is_unsigned_v<O>)) {

				// effectively same type
				return O(value);
			}
			else {
				// flip first bit to handle signed/unsigned differences
				I v = value ^ (1 << ((sizeof(I) * 8) - 1));
				return std::bit_cast<O>(v);
			}
		}
		else { // many-1 mappings
			constexpr auto oldMax = (long double)std::numeric_limits<I>::max();
			constexpr auto oldMin = (long double)std::numeric_limits<I>::lowest();

			constexpr auto newMax = (long double)std::numeric_limits<O>::max();
			constexpr auto newMin = (long double)std::numeric_limits<O>::lowest();

			return O(remap((long double)(value), oldMin, oldMax, newMin, newMax));
		}
	}

	// similar to remap, but mapping skewed based on centerpoint(s)
	// useful for audio
	// old centerpoint should NOT be the min/max value of that type
	template <std::integral O, std::integral I> [[nodiscard]]
	constexpr O remapBalanced(I value, I oldCenterpoint = 0, O newCenterpoint = 0) noexcept {
		constexpr auto oldMax = (long double)std::numeric_limits<I>::max();
		constexpr auto oldMin = (long double)std::numeric_limits<I>::lowest();

		constexpr auto newMax = (long double)std::numeric_limits<O>::max();
		constexpr auto newMin = (long double)std::numeric_limits<O>::lowest();

		return O(remapBalanced((long double)(value), (long double)(oldCenterpoint), (long double)(newCenterpoint), oldMin, oldMax, newMin, newMax));
	}

	// f stands for fast and floor
	// negative values and 0 return -1
	template <std::integral I> [[nodiscard]]
	constexpr I fLog10(I value) noexcept {
		if (value <= I(0)) {
			return static_cast<I>(-1);
		}

		using Unsigned = std::make_unsigned_t<I>;
		Unsigned v = static_cast<Unsigned>(value);

		auto log2 = (sizeof(Unsigned) * 8 - 1) - std::countl_zero(v);

		// rough approximation of the change of base formula
		Unsigned approx = (log2 * 1233) / 4096;

		constexpr auto poTs = [](){
			std::array<Unsigned, std::numeric_limits<Unsigned>::digits10 + 1> out{};
			out[0] = 1;
			for (size_t i = 1; i < out.size(); ++i) {
				out[i] = out[i - 1] * static_cast<Unsigned>(10);
			}
			return out;
		}();

		return static_cast<I>(approx + (v >= poTs[approx + 1]));
	}

	template <std::integral I> [[nodiscard]]
	constexpr I fPow10(I exp) noexcept {
		if constexpr (std::is_signed_v<I>) {
			if (exp < static_cast<I>(0)) {
				return static_cast<I>(0);
			}
		}

		constexpr auto poTs = []() {
			std::array<I, std::numeric_limits<I>::digits10 + 1> out{};
			out[0] = 1;
			for (size_t i = 1; i < out.size(); ++i) {
				out[i] = out[i - 1] * static_cast<I>(10);
			}
			return out;
		}();

		if (exp >= poTs.size()) {
			return poTs.back();
		}
		return poTs[exp];
	}
}