#pragma once

#include <utility>
#include <cassert>
#include <array>
#include <numeric>
#include <cmath>

namespace sndx::math {

	[[nodiscard]]
	inline constexpr size_t factorial(size_t n) noexcept {
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
	inline constexpr size_t fibonacci(size_t n) noexcept {
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
}