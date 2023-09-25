#pragma once

#include <glm/glm.hpp>

#include <array>

namespace sndx {

	template <size_t n> [[nodiscard]]
	consteval std::array<size_t, n+1> factorials() {
		std::array<size_t, n + 1> out;
		out[0] = 1;

		for (int i = 1; i <= n; ++i) {
			out[i] = out[i - 1] * i;
		}

		return out;
	}

	[[nodiscard]]
	constexpr size_t factorial(size_t n) {
		if (n == 0 || n == -1) return 1;

		size_t out = n;
		for (int i = 2; i < n; ++i) {
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

	template <class Container> [[nodiscard]]
	constexpr auto bezier(float t, const Container& container) {

		using T = typename Container::value_type;

		auto factN = factorial(container.size() - 1);

		T out = T(0.0);

		size_t i = 0;
		for (auto point : container) {
			size_t binomCoef = factN / (factorial(i) * factorial(container.size() - 1 - i));

			out += point * pow(1.0f - t, container.size() - 1 - i) * pow(t, i) * (float)(binomCoef);

			++i;
		}

		return out;
	}

	template <size_t n, class Container> [[nodiscard]]
	constexpr auto bezier(float t, const Container& container) {
		assert(n == container.size());

		using T = typename Container::value_type;

		static constexpr auto facts = factorials<n>();

		T out = T(0.0);

		size_t i = 0;
		for (auto point : container) {
			size_t binomCoef = facts[n - 1] / (facts[i] * facts[container.size() - 1 - i]);

			out += point * pow(1.0f - t, container.size() - 1 - i) * pow(t, i) * (float)(binomCoef);

			++i;
		}

		return out;
	}

	template <typename T, typename... Ts> [[nodiscard]]
	constexpr auto bezier(float t, T a, Ts... points) {
		return bezier<sizeof...(Ts) + 1>(t, { a, points... });
	}

	template <float threshold, class T> [[nodiscard]]
	inline auto vecEqual(const T& a, const T& b) {
		for (int i = 0; i < a.length(); ++i) {
			if (abs(a[i] - b[i]) > threshold) return false;
		}
		return true;
	}

	template <float threshold = 0.00001f> [[nodiscard]]
	inline bool areColinear(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b) {
		auto pv = p - a;
		auto bv = b - a;

		if (glm::length(glm::cross(pv, bv)) <= threshold) return true;

		return false;
	}

	[[nodiscard]]
	inline glm::vec3 surfaceNormal(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2) {
		auto u = p1 - p0;
		auto v = p2 - p0;

		return glm::cross(u, v);
	}
}