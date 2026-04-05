#pragma once

#include "./triangle.hpp"

#include <array>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sndx::collision {
	// this is pretty much what is written in https://winter.dev/articles/gjk-algorithm and their epa article
	namespace detail {
		struct MinkowskiDiff {
			glm::vec3 a, b, out;
		};

		template <class SFnA, class SFnB> [[nodiscard]]
		MinkowskiDiff gjkMinkowski(SFnA&& supportA, SFnB&& supportB, glm::vec3 direction) {
			auto a = std::forward<SFnA>(supportA)(direction);
			auto b = std::forward<SFnB>(supportB)(-direction);
			return MinkowskiDiff{ a, b, a - b };
		}

		[[nodiscard]]
		inline glm::vec3 tripleProduct(glm::vec3 a, glm::vec3 b) {
			return glm::cross(glm::cross(a, b), a);
		}

		template <class T> [[nodiscard]]
		bool similarDir(T a, T b) {
			return glm::dot(a, b) > 0.0;
		}
	}

	struct SimplexGJK {
		using Vec = glm::vec3;
		
		[[nodiscard]]
		static constexpr size_t dimensionality() noexcept {
			return Vec::length();
		}

		std::array<detail::MinkowskiDiff, 3 + 1> points{};
		uint8_t size = 0;

		void push_front(detail::MinkowskiDiff point) {
			for (size_t i = points.size() - 1; i > 0; --i) {
				points[i] = points[i - 1];
			}
			points[0] = point;
			size = std::min(uint8_t(size + 1u), uint8_t(points.size()));
		}

		bool lineOrigin(Vec& newDirection) {
			auto ab = points[1].out - points[0].out;
			auto ao = -points[0].out;

			if (detail::similarDir(ab, ao)) { // we want to point towards the origin
				auto tp = detail::tripleProduct(ab, ao);
				auto len = glm::length(tp);

				// beware being near colinear with the origin
				if (len < 0.00001f) {
					auto mag = glm::abs(ab);
					if (mag.x < mag.y && mag.x < mag.z) {
						newDirection = glm::vec3(1.0f, 0.0f, 0.0f);
					}
					else if (mag.y < mag.z) {
						newDirection = glm::vec3(0.0f, 1.0f, 0.0f);
					}
					else {
						newDirection = glm::vec3(0.0f, 0.0f, 1.0f);
					}
					newDirection = glm::cross(ab, newDirection);
					return false;
				}

				newDirection = tp / len;
				return true;
			}

			// remove b from simplex
			size = 1;
			newDirection = ao;
			return false;
		}

		bool triangleOrigin(Vec& newDirection) {
			auto ab = points[1].out - points[0].out;
			auto ac = points[2].out - points[0].out;
			auto ao = -points[0].out;

			auto cross = glm::cross(ab, ac);

			if (detail::similarDir(glm::cross(cross, ac), ao)) {
				// reduce back to line
				if (!detail::similarDir(ac, ao)) {
					// keep a and b
					size = 2;
					lineOrigin(newDirection);
					return false;
				}
				
				// keep a and c
				points[1] = points[2];
				size = 2;
				newDirection = detail::tripleProduct(ac, ao);
				return false;
			}
			
			if (detail::similarDir(glm::cross(ab, cross), ao)) {
				// keep a and b
				size = 2;
				lineOrigin(newDirection);
				return false;
			}

			// check direction for tetrahedon
			if (detail::similarDir(cross, ao)) {
				newDirection = cross;
			}
			else {
				// flip b and c
				std::swap(points[1], points[2]);
				newDirection = -cross;
			}

			return true;
		}

		bool tetrahedronOrigin(Vec& newDirection) {
			auto ab = points[1].out - points[0].out;
			auto ac = points[2].out - points[0].out;
			auto ad = points[3].out - points[0].out;
			auto ao = -points[0].out;

			if (detail::similarDir(glm::cross(ab, ac), ao)) {
				// remove d
				size = 3;
				triangleOrigin(newDirection);
				return false;
			}

			if (detail::similarDir(glm::cross(ac, ad), ao)) {
				// remove b
				points[1] = points[2];
				points[2] = points[3];
				size = 3;
				triangleOrigin(newDirection);
				return false;
			}

			if (detail::similarDir(glm::cross(ad, ab), ao)) {
				// remove c and flip b, d
				points[2] = points[1];
				points[1] = points[3];
				size = 3;
				triangleOrigin(newDirection);
				return false;
			}

			return true;
		}

		[[nodiscard]]
		bool gjkOrigin(Vec& newDirection) {
			// checks need to be done after since size can be modified
			switch (size) {
			case 2: return lineOrigin(newDirection) && size == dimensionality() + 1;
			case 3: return triangleOrigin(newDirection) && size == dimensionality() + 1;
			case 4: return tetrahedronOrigin(newDirection) && size == dimensionality() + 1;
			default:
				throw std::logic_error("GJK had weird number of points in simplex");
			}
		}
	};

	template <class SFnA, class SFnB> [[nodiscard]]
	std::optional<SimplexGJK> gjk(const SFnA& supportA, const SFnB& supportB) {
		auto support = detail::gjkMinkowski(supportA, supportB, glm::vec3(1.0, 0.0, 0.0));

		SimplexGJK simplex{};
		simplex.push_front(support);

		size_t iterations = 0;

		auto dir = glm::normalize(-support.out);
		while (true) {
			support = detail::gjkMinkowski(supportA, supportB, dir);
			if (!detail::similarDir(support.out, dir)) {
				return std::nullopt;
			}

			simplex.push_front(support);
			if (simplex.gjkOrigin(dir)) {
				return simplex;
			}

			++iterations;
		}
		return std::nullopt;
	}

	struct EpaResult {
		glm::vec3 normal;
		float depth;

		[[nodiscard]]
		EpaResult invTransform(const glm::mat3& inverse) const {
			auto tInv = glm::transpose(inverse);
			auto tNormal = tInv * normal;
			auto len = glm::length(tNormal);

			return EpaResult{ tNormal / len, depth * len };
		}
	};

	template <float epsilon = 0.00001f, class SFnA, class SFnB> [[nodiscard]]
	EpaResult epa(const SimplexGJK& simplex, const SFnA& supportA, const SFnB& supportB) {
		throw std::runtime_error("not implemented");
	}

	template <class Fn> [[nodiscard]]
	auto transformSupportFn(Fn&& fnc, const glm::mat4& t, const glm::mat4& invT) {
		return [f = std::forward<Fn>(fnc), t, iT = glm::mat3{ invT }](glm::vec3 dir) {
			auto dirLocal = iT * dir;
			return glm::vec3(t * glm::vec4(f(dirLocal), 1.0f));
		};
	}

	template <VolumeN<3> T> [[nodiscard]]
	auto getSupportFn(const T& volume) {
		return [&](glm::vec3 dir) { return volume.supportPoint(dir); };
	}

	template <size_t extent> [[nodiscard]]
	auto getSupportFn(std::span<const glm::vec3, extent> points) {
		return [points](glm::vec3 dir) {
			auto best = std::numeric_limits<float>::min();
			glm::vec3 out{};
			for (const auto& point : points) {
				auto d = glm::dot(point, dir);
				if (d > best) {
					best = d;
					out = point;
				}
			}
			return out;
		};
	}

	template <class T> [[nodiscard]]
	auto getSupportFn(T&& shape, const glm::mat4& t, const glm::mat4& invT) {
		return transformSupportFn(getSupportFn(std::forward<T>(shape)), t, invT);
	}
}