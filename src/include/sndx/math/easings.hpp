#pragma once

#include <functional>
#include <concepts>

#include <glm/glm.hpp>

namespace sndx::math {

	template <class T, class F>
	constexpr auto lerp(const T& x, const T& y, F a) noexcept {
		return glm::mix(x, y, a);
	}

	template <class T>
	constexpr auto invLerp(const T& x, const T& y, const T& v) noexcept {
		assert(y != x);

		return (v - x) / (y - x);
	}

	template <std::integral T>
	constexpr auto invLerp(const T& x, const T& y, const T& v) noexcept {
		assert(y != x);

		return (long double)(v - x) / (long double)(y - x);
	}

	// taken from https://easings.net/

	
	// preconditions: argument is [0.0, 1.0]
	// postconditions: f(0.0) == 0.0, f(1.0) == 1.0
	template <std::floating_point T>
	using EasingFunc = std::function<T(T)>;

	
	template <std::floating_point T, T(*Func)(T)> [[nodiscard]]
	constexpr auto easeOut(T a) noexcept {
		return T(1.0) - Func(T(1.0) - a);
	}

	template <std::floating_point T, class Func> [[nodiscard]]
	constexpr auto easeOut(Func func, T a) noexcept {
		return T(1.0) - func(T(1.0) - a);
	}

	template <std::floating_point T> [[nodiscard]]
	constexpr auto easeLinear(T a) noexcept {
			return a;
	}

	template <std::floating_point T> [[nodiscard]]
	constexpr auto easeInQuadratic(T a) noexcept {
		return a * a;
	}

	template <std::floating_point T> [[nodiscard]]
	constexpr auto easeOutQuadratic(T a) noexcept {
		return easeOut<T, easeInQuadratic>(a);
	}

	template <std::floating_point T> [[nodiscard]]
	constexpr auto easeInCubic(T a) noexcept {
		return a * a * a;
	}

	template <std::floating_point T> [[nodiscard]]
	constexpr auto easeOutCubic(T a) noexcept {
		return easeOut<T, easeInCubic>(a);
	}
}