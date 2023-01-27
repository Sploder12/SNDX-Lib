#pragma once

namespace sndx {

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

	// this is not particularly efficient and I have 0 clue if it works beyond cubic
	template <typename A, typename B, typename... Ts> [[nodiscard]]
	constexpr auto bezier(float t, A a, B b, Ts... points) {

		if constexpr (sizeof...(Ts) == 0) {
			return lerp(t, a, b);
		}
		else {
			return bezier(t, lerp(t, a, b), bezier(t, b, points...));
		}
	}
}