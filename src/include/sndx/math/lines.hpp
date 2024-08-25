#pragma once

#include <numeric>
#include <concepts>
#include <functional>

#include <glm/glm.hpp>

#include "./math.hpp"

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

	// note: ccw winding, return is not a normalized vector
	template <class I = float, glm::qualifier Q = glm::qualifier::defaultp> [[nodiscard]]
	constexpr glm::vec3 surfaceNormal(const glm::vec<3, I, Q>& p, const glm::vec<3, I, Q>& a, const glm::vec<3, I, Q>& b) {
		auto u = p - a;
		auto v = b - a;

		return glm::cross(u, v);
	}

	template <class I = float, I threshold = std::numeric_limits<I>::epsilon(), glm::qualifier Q = glm::qualifier::defaultp> [[nodiscard]]
	constexpr bool areColinear(const glm::vec<3, I, Q>& p, const glm::vec<3, I, Q>& a, const glm::vec<3, I, Q>& b) noexcept {
		return glm::length(surfaceNormal(p, a, b)) <= threshold;
	}

	template <class I = float, I threshold = std::numeric_limits<I>::epsilon(), glm::qualifier Q = glm::qualifier::defaultp> [[nodiscard]]
	constexpr bool areColinear(const glm::vec<2, I, Q>& p, const glm::vec<2, I, Q>& a, const glm::vec<2, I, Q>& b) noexcept {
		// this is functionally equivalent to the 3d version
		auto pv = p - a;
		auto bv = b - a;

		return glm::abs(pv.x * bv.y - bv.x * pv.y) <= threshold;
	}

	template <std::floating_point T, class... Ts>
	constexpr auto bezier(T t, const Ts&&... ts) noexcept {
		
		constexpr size_t n = sizeof...(ts);
		static_assert(n > 1);

		constexpr auto coefs = binomialCoefficients<n - 1, T>();

		using O = std::common_type_t<Ts...>;
		O out = O(0.0);

		constexpr bool isObj = requires (O o) { O::value_type; };

		T tI = T(1.0);

		size_t i = 0;
		for (const auto& p : { ts... }) {
			auto coef = tI * coefs[i];
			if constexpr (isObj) {
				out += p * (O::value_type)(std::pow(T(1.0) - t, n - 1 - i) * coef);
			}
			else {
				out += p * T(std::pow(T(1.0) - t, n - 1 - i) * coef);
			}

			tI *= t;
			++i;
		}

		return out;
	}
}