#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>

#include <algorithm>
#include <array>
#include <optional>
#include <functional>
#include <numbers>

namespace sndx {

	template <size_t n, class DimensionType = glm::vec3>
	struct Intersections {

		using Dimension_T = DimensionType;

		std::array<std::optional<Dimension_T>, n> hits;

		[[nodiscard]]
		int hitCount() const {
			int count = 0;

			for (const auto& hit : hits) {
				count += int(hit.has_value());
			}

			return count;
		}
		
		[[nodiscard]]
		std::vector<Dimension_T> asVector() const {
			std::vector<Dimension_T> out{};
			out.reserve(n);

			for (const auto& hit : hits) {
				if (hit.has_value()) {
					out.emplace_back(hit.value());
				}
			}

			return out;
		}
	};

	template <class DimensionType = glm::vec3>
	struct AABB {

		using Dimension_T = DimensionType;

		Dimension_T ldf;
		Dimension_T rub;

		AABB& merge(const AABB& other) {
			ldf = glm::min(ldf, other.ldf);
			rub = glm::max(rub, other.rub);

			return *this;
		}

		[[nodiscard]]
		constexpr bool intersects(const AABB<Dimension_T>& other) const {
			auto comparisons = glm::lessThanEqual(ldf, other.rub) && glm::greaterThanEqual(rub, other.ldf);

			return glm::all(comparisons);
		}

		[[nodiscard]]
		constexpr bool contains(const Dimension_T& p) const {
			if (p.x <= rub.x && p.x >= ldf.x) {
				if (p.y <= rub.y && p.y >= ldf.y) {
					return (p.z <= rub.z && p.z >= ldf.z);
				}
			}
			return false;
		}

		[[nodiscard]]
		constexpr Dimension_T dims() const {
			return rub - ldf;
		}

		[[nodiscard]]
		constexpr double area() const {
			return glm::compMul(dims());
		}

		[[nodiscard]]
		constexpr double surfaceArea() const {
			auto dim = dims();

			double area = 0.0;

			for (size_t i = 0; i < dim.length(); ++i) {
				for (size_t j = 0; j < dim.length(); ++j) {
					if (i == j) continue;

					area += dim[j] * dim[i];
				}
			}

			return area;
		}

		[[nodiscard]]
		constexpr Dimension_T center() const {
			return (ldf + rub) * 0.5f;
		}

		// adapted from https://iquilezles.org/articles/distfunctions/
		[[nodiscard]]
		float distance(const Dimension_T& p) const {
			auto b = rub - center();

			// same as sdf article from here
			auto q = glm::abs(p) - b;
			return glm::length(glm::max(q, 0.0f)) + glm::min(glm::compMax(q), 0.0f);
		}

		// Liang-Barsky algorithm
		template <float epsilon = 0.00001f> [[nodiscard]]
		Intersections<2, Dimension_T> lineIntersections(const Dimension_T& p0, const Dimension_T& p1) const {

			auto d = p1 - p0;

			auto upBound = rub - p0;
			auto lowBound = ldf - p0;

			float intersect0 = -INFINITY;
			float intersect1 = INFINITY;

			for (int i = 0; i < p0.length(); ++i) {
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

	template <class DimensionType = glm::vec3>
	struct SphereBB {

		using Dimension_T = DimensionType;

		Dimension_T pos;
		float radius;

		// produces new sphere that tightly bounds the original 2
		SphereBB& merge(const SphereBB& other) {
			auto deltaRad = std::abs(radius - other.radius);
			auto centerDist = glm::distance(pos, other.pos);

			if (centerDist < deltaRad || centerDist == 0.0f) { // edge-case where my algorithm fails
				if (radius > other.radius) {
					return *this;
				}
				
				radius = other.radius;
				pos = other.pos;
			}
			else {
				// pretty proud of this even if it fails when one sphere contains the entirety of the other
				auto dir = (pos - other.pos) / centerDist; // normalized direction
				pos = ((pos + radius * dir) + (other.pos - other.radius * dir)) / 2.0f;
				radius = (centerDist + radius + other.radius) / 2.0f;
			}

			return *this;
		}

		[[nodiscard]] // these area functions actually describe volume, i just like the word area more
		constexpr double area() const {
			return 4.0 / 3.0 * std::numbers::pi * radius * radius * radius;
		}

		[[nodiscard]]
		constexpr double surfaceArea() const {
			// everyone loves 4 pi r^2
			return 4.0 * std::numbers::pi * radius * radius;
		}

		// adapted from https://iquilezles.org/articles/distfunctions/
		[[nodiscard]]
		float distance(const Dimension_T& p) const {
			return glm::length(pos - p) - radius;
		}

		[[nodiscard]] // distance <= 0.0f when intersecting. The negative value doesn't make sense in the context of SDFs
		float distance(const SphereBB<Dimension_T>& other) const {
			return glm::distance(pos, other.pos) - radius - other.radius;
		}

		[[nodiscard]]
		float distance(const AABB<Dimension_T>& aabb) const {
			return aabb.distance(pos) - radius;
		}

		[[nodiscard]]
		bool contains(const Dimension_T& p) const {
			return distance(p) <= 0.0f;
		}

		[[nodiscard]]
		bool intersects(const SphereBB<Dimension_T>& other) const {
			return distance(other) <= 0.0f;
		}

		[[nodiscard]]
		bool intersects(const AABB<Dimension_T>& aabb) const {
			return distance(aabb) <= 0.0f;
		}
		
	};

	// THIS WILL INCREASE THE VOLUME OF THE BB
	template <class Dimension_T = glm::vec3> [[nodiscard]]
	constexpr AABB<Dimension_T> toAABB(const SphereBB<Dimension_T>& bb) {
		AABB<Dimension_T> out{};
		out.ldf = bb.pos - bb.radius;
		out.rub = bb.pos + bb.radius;
		return out;
	}

	// THIS WILL INCREASE THE VOLUME OF THE BB
	template <class Dimension_T = glm::vec3> [[nodiscard]]
	inline SphereBB<Dimension_T> toSphereBB(const AABB<Dimension_T>& bb) {
		SphereBB<Dimension_T> out{};
		out.pos = bb.center();
		out.radius = glm::distance(out.pos, bb.rub);
		return out;
	}

	// technically 2d but useful in 3d scenarios
	[[nodiscard]]
	constexpr bool pointInTri(glm::vec2 p, glm::vec2 a, glm::vec2 b, glm::vec2 c) {
		float ab = (p.y - a.y) * (b.x - a.x) - (p.x - a.x) * (b.y - a.y);
		float ca = (p.y - c.y) * (a.x - c.x) - (p.x - c.x) * (a.y - c.y);
		float bc = (p.y - b.y) * (c.x - b.x) - (p.x - b.x) * (c.y - b.y);

		return (ab * bc >= 0.0f && bc * ca >= 0.0f);
	}
}