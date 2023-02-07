#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <optional>
#include <functional>

namespace sndx {

	template <size_t n>
	struct Intersections {
		std::array<std::optional<glm::vec3>, n> hits;

		[[nodiscard]]
		int hitCount() const {
			int count = 0;

			for (const auto& hit : hits) {
				count += int(hit.has_value());
			}

			return count;
		}
		
		[[nodiscard]]
		std::vector<glm::vec3> asVector() const {
			std::vector<glm::vec3> out{};
			out.reserve(n);

			for (const auto& hit : hits) {
				if (hit.has_value()) {
					out.emplace_back(hit.value());
				}
			}

			return out;
		}
	};

	struct AABB {

		glm::vec3 ldf;
		glm::vec3 rub;

		[[nodiscard]]
		constexpr AABB merge(const AABB& other) const {
			using std::min;
			using std::max;

			AABB newBB;

			newBB.ldf = glm::vec3(min(ldf.x, other.ldf.x), min(ldf.y, other.ldf.y), min(ldf.z, other.ldf.z));
			newBB.rub = glm::vec3(max(rub.x, other.rub.x), max(rub.y, other.rub.y), max(rub.z, other.rub.z));

			return newBB;
		}

		[[nodiscard]]
		constexpr bool intersects(const AABB& other) const {
			if (ldf.x <= other.rub.x && rub.x >= other.ldf.x) {
				if (ldf.y <= other.rub.y && rub.y >= other.ldf.y) {
					return (ldf.z <= other.rub.z && rub.z >= other.ldf.z);
				}
			}

			return false;
		}

		[[nodiscard]]
		constexpr bool contains(glm::vec3 p) const {
			if (p.x <= rub.x && p.x >= ldf.x) {
				if (p.y <= rub.y && p.y >= ldf.y) {
					return (p.z <= rub.z && p.z >= ldf.z);
				}
			}
			return false;
		}

		[[nodiscard]]
		constexpr glm::vec3 dims() const {
			return rub - ldf;
		}

		[[nodiscard]]
		constexpr double area() const {
			auto dim = dims();

			return dim.x * dim.y * dim.z;
		}

		[[nodiscard]]
		constexpr double surfaceArea() const {
			auto dim = dims();

			return 2.0 * (dim.x * dim.z + dim.y * dim.z + dim.x * dim.y);
		}

		[[nodiscard]]
		constexpr glm::vec3 center() const {
			return (ldf + rub) * 0.5f;
		}

		// adapted from https://iquilezles.org/articles/distfunctions/
		[[nodiscard]]
		float distance(glm::vec3 p) const {
			glm::vec3 b = rub - center();

			// same as sdf article from here
			glm::vec3 q = glm::abs(p) - b;
			return glm::length(glm::max(q, 0.0f)) + glm::min(std::max(q.x, std::max(q.y, q.z)), 0.0f);
		}

		
		// Liang-Barsky algorithm
		template <float epsilon = 0.00001f> [[nodiscard]]
		Intersections<2> lineIntersections(glm::vec3 p0, glm::vec3 p1) const {

			glm::vec3 d = p1 - p0;

			glm::vec3 upBound = rub - p0;
			glm::vec3 lowBound = ldf - p0;

			float intersect0 = -INFINITY;
			float intersect1 = INFINITY;

			for (int i = 0; i < 3; ++i) {
				if (std::abs(d[i]) <= epsilon) {
					if (upBound[i] < 0 || lowBound[i] > 0) return {};
				}
				else {
					float plow = lowBound[i] / d[i];
					float phigh = upBound[i] / d[i];

					if (d[i] < 0) {
						intersect0 = std::max(intersect0, phigh);
						intersect1 = std::min(intersect1, plow);
					}
					else {
						intersect0 = std::max(intersect0, plow);
						intersect1 = std::min(intersect1, phigh);
					}

					if (intersect1 < intersect0) return {};
				}
			}

			if (intersect0 == intersect1) return { {p0 + intersect0 * d, {}} };

			return { {p0 + intersect0 * d, p0 + intersect1 * d} };
		}
	};
}