#pragma once

#include "./volume.hpp"

#include <array>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>

namespace sndx::collision {
	namespace detail {
		template <class SFnA, class SFnB> [[nodiscard]]
		glm::vec3 gjkMinkowski(SFnA&& supportA, SFnB&& supportB, glm::vec3 direction) {
			assert(std::abs(glm::length(direction) - 1.0f) <= 0.0001f && "direction must be normalized!");
			return std::forward<SFnA>(supportA)(direction) - std::forward<SFnB>(supportB)(-direction);
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

		std::array<Vec, 3 + 1> points{};
		uint8_t size = 0;

		void push_front(Vec point) {
			for (size_t i = points.size() - 1; i > 0; --i) {
				points[i] = points[i - 1];
			}
			points[0] = point;
			size = std::min(uint8_t(size + 1u), uint8_t(points.size()));
		}

		bool lineOrigin(Vec& newDirection) {
			auto ab = points[1] - points[0];
			auto ao = -points[0];

			if (detail::similarDir(ab, ao)) { // we want to point towards the origin
				newDirection = glm::normalize(detail::tripleProduct(ab, ao));
			}
			else {
				// remove b from simplex
				size = 1;
				newDirection = glm::normalize(ao);
			}

			return false;
		}

		bool triangleOrigin(Vec& newDirection) {
			auto ab = points[1] - points[0];
			auto ac = points[2] - points[0];
			auto ao = -points[0];

			auto cross = glm::cross(ab, ac);

			if (detail::similarDir(glm::cross(cross, ac), ao)) {
				// reduce back to line
				if (!detail::similarDir(ac, ao)) {
					// keep a and b
					size = 2;
					return lineOrigin(newDirection);
					
				}
				else {
					// keep a and c
					points[1] = points[2];
					size = 2;
					newDirection = glm::normalize(detail::tripleProduct(ac, ao));
				}
			}
			else {
				if (detail::similarDir(glm::cross(ab, cross), ao)) {
					// keep a and b
					size = 2;
					return lineOrigin(newDirection);
				}

				// check direction for tetrahedon
				if (detail::similarDir(cross, ao)) {
					newDirection = glm::normalize(cross);
				}
				else {
					// flip b and c
					std::swap(points[1], points[2]);
					newDirection = glm::normalize(-cross);
				}
			}
			return false;
		}

		bool tetrahedronOrigin(Vec& newDirection) {
			auto ab = points[1] - points[0];
			auto ac = points[2] - points[0];
			auto ad = points[3] - points[0];
			auto ao = -points[0];

			if (detail::similarDir(glm::cross(ab, ac), ao)) {
				// remove d
				size = 3;
				return triangleOrigin(newDirection);
			}

			if (detail::similarDir(glm::cross(ac, ad), ao)) {
				// remove b
				points[1] = points[2];
				points[2] = points[3];
				size = 3;
				return triangleOrigin(newDirection);
			}

			if (detail::similarDir(glm::cross(ad, ab), ao)) {
				// remove c and flip b, d
				points[2] = points[1];
				points[1] = points[3];
				return triangleOrigin(newDirection);
			}

			return true;
		}

		[[nodiscard]]
		bool gjkOrigin(Vec& newDirection) {
			switch (size) {
			case 2: return lineOrigin(newDirection);
			case 3: return triangleOrigin(newDirection);
			case 4: return tetrahedronOrigin(newDirection);
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

		auto dir = glm::normalize(-support);
		while (true) {
			support = detail::gjkMinkowski(supportA, supportB, dir);
			if (!detail::similarDir(support, dir)) {
				return std::nullopt;
			}

			simplex.push_front(support);
			if (simplex.gjkOrigin(dir)) {
				return simplex;
			}
		}
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
}