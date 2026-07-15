#pragma once

#include <numeric>
#include <concepts>
#include <functional>

#include <glm/glm.hpp>

// for length2
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

#include "./math.hpp"

namespace sndx::math {

	// note: ccw winding, return is not a normalized vector
	template <class I = float, glm::qualifier Q = glm::qualifier::defaultp> [[nodiscard]]
	constexpr auto surfaceNormal(const glm::vec<3, I, Q>& p, const glm::vec<3, I, Q>& a, const glm::vec<3, I, Q>& b) {
		auto u = p - a;
		auto v = b - a;

		return glm::cross(u, v);
	}

	// closest points of two lines segments as described by Ericson's realtime collision detection
	template <class VecT> [[nodiscard]]
	constexpr std::pair<VecT, VecT> closestPoints(const VecT& p1, const VecT& q1, const VecT& p2, const VecT& q2) {
		auto d1 = q1 - p1;
		auto d2 = q2 - p2;
		auto ab = p1 - p2;
		auto la2 = glm::length2(d1);
		auto lb2 = glm::length2(d2);
		auto f = glm::dot(d2, ab);

		if (la2 <= 0.00001f && lb2 <= 0.00001f) [[unlikely]] {
			// both are degenerate
			return std::make_pair(p1, p2);
		}

		float s = 0.0f;
		float t = 0.0f;
		if (la2 <= 0.00001f) [[unlikely]] {
			// a is degenerate
			t = glm::clamp(f / lb2, 0.0f, 1.0f);
		}
		else {
			float c = glm::dot(d1, ab);
			if (lb2 <= 0.00001f) {
				// b is degenerate
				s = glm::clamp(-c / la2, 0.0f, 1.0f);
			}
			else {
				auto b = glm::dot(d1, d2);
				auto denom = la2 * lb2 - b * b;
				if (denom != 0.0) {
					s = glm::clamp((b * f - c * lb2) / denom, 0.0f, 1.0f);
				}
				else {
					// parallel lines
					s = 0.0f;
				}

				t = (b * s + f) / lb2;
				if (t < 0.0f) {
					t = 0.0f;
					s = glm::clamp(-c / la2, 0.0f, 1.0f);
				}
				else if (t > 1.0f) {
					t = 1.0f;
					s = glm::clamp((b - lb2) / la2, 0.0f, 1.0f);
				}
			}
		}

		return std::make_pair(p1 + d1 * s, p2 + d2 * t);
	}

	template <class I = float, glm::qualifier Q = glm::qualifier::defaultp> [[nodiscard]]
	constexpr auto projectOnPlane(const glm::vec<3, I, Q>& a, const glm::vec<3, I, Q>& planeNormal) {
		return a - planeNormal * glm::dot(a, planeNormal);
	}


	template <class I = float, glm::qualifier Q = glm::qualifier::defaultp> [[nodiscard]]
	constexpr bool areColinear(const glm::vec<3, I, Q>& p, const glm::vec<3, I, Q>& a, const glm::vec<3, I, Q>& b) noexcept {
		return glm::length2(surfaceNormal(p, a, b)) <= I(0.00001f);
	}

	template <class I = float, glm::qualifier Q = glm::qualifier::defaultp> [[nodiscard]]
	constexpr bool areColinear(const glm::vec<2, I, Q>& p, const glm::vec<2, I, Q>& a, const glm::vec<2, I, Q>& b) noexcept {
		// this is functionally equivalent to the 3d version
		auto pv = p - a;
		auto bv = b - a;

		return glm::abs(pv.x * bv.y - bv.x * pv.y) <= std::numeric_limits<I>::epsilon();
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