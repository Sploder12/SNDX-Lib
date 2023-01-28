#pragma once

namespace sndx {

	template <size_t n> [[nodiscard]]
	constexpr size_t factorial() {
		size_t out = 1;
		for (int i = 2; i <= n; ++i) {
			out *= i;
		}
		return out;
	}

	[[nodiscard]]
	constexpr size_t factorial(size_t n) {
		size_t out = 1;
		for (int i = 2; i <= n; ++i) {
			out *= i;
		}
		return out;
	}

	template <typename T> [[nodiscard]]
	constexpr auto pow(T val, size_t n) {
		T out = 1;

		while (n > 0) {
			if (n % 2 == 1) out *= val;
			
			n /= 2;

			if (n > 0) val *= val;
		}

		return out;
	}

	template <typename T> [[nodiscard]]
	constexpr auto clamp(T a, T min, T max) {
		if (a < min) return min;
		if (a > max) return max;
		return a;
	}

	template <typename A, typename B> [[nodiscard]]
	constexpr auto lerp(float t, A a, B b) {
		return a * (1.0f - t) + b * t;
	}

	template <typename T> [[nodiscard]]
	constexpr auto step(T edge, T val) {
		return T(val < edge);
	}

	template <typename T> [[nodiscard]]
	constexpr auto smoothStep(T edge0, T edge1, T val) {
		auto t = clamp((val - edge0) / (edge1 - edge0), T(0.0), T(1.0));
		return t * t * (T(3.0) - 2.0 * t);
	}

	// I have 0 clue if it works beyond cubic
	template <class Container> [[nodiscard]]
		constexpr auto bezier(float t, const Container& container) {

		using T = typename Container::value_type;

		auto factN = factorial(container.size() - 1);

		T out = T(0.0);

		size_t i = 0;
		for (auto point : container) {
			size_t binomCoef = factN / (factorial(i) * factorial(container.size() - 1 - i));

			out += point * pow(1.0f - t, container.size() - 1 - i) * pow(t, i) * T(binomCoef);

			++i;
		}

		return out;
	}

	template <typename T, typename... Ts> [[nodiscard]]
	constexpr auto bezier(float t, T a, Ts... points) {

		static constexpr auto factN = factorial<sizeof...(Ts)>();

		auto loci = { points... };

		T out = a * pow(1.0f - t, sizeof...(Ts));

		size_t i = 1;
		for (auto point : loci) {
			size_t binomCoef = factN / (factorial(i) * factorial(sizeof...(Ts) - i));

			out += point * pow(1.0f - t, sizeof...(Ts) - i) * pow(t, i) * T(binomCoef);

			++i;
		}

		return out;
	}
}