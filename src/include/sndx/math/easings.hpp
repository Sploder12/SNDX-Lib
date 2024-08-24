#pragma once

#include <functional>
#include <concepts>

namespace sndx::math {

	// taken from https://easings.net/

	
	// preconditions: argument is [0.0, 1.0]
	// postconditions: f(0.0) == 0.0, f(1.0) == 1.0
	template <std::floating_point T>
	using EasingFunc = std::function<T(T)>;

	
	template <std::floating_point T, T(*Func)(T)> [[nodiscard]]
	inline constexpr auto easeOut(T a) noexcept {
		return T(1.0) - Func(T(1.0) - a);
	}

	template <std::floating_point T, class Func> [[nodiscard]]
	inline constexpr auto easeOut(Func func, T a) noexcept {
		return T(1.0) - func(T(1.0) - a);
	}

	template <std::floating_point T> [[nodiscard]]
	inline constexpr auto easeLinear(T a) noexcept {
			return a;
	}

	template <std::floating_point T> [[nodiscard]]
	inline constexpr auto easeInQuadratic(T a) noexcept {
		return a * a;
	}

	template <std::floating_point T> [[nodiscard]]
	inline constexpr auto easeOutQuadratic(T a) noexcept {
		return easeOut<T, easeInQuadratic>(a);
	}

	template <std::floating_point T> [[nodiscard]]
	inline constexpr auto easeInCubic(T a) noexcept {
		return a * a * a;
	}

	template <std::floating_point T> [[nodiscard]]
	inline constexpr auto easeOutCubic(T a) noexcept {
		return easeOut<T, easeInCubic>(a);
	}
}