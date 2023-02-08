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

		AABB& merge(const AABB& other) {
			using std::min;
			using std::max;

			ldf = glm::vec3(min(ldf.x, other.ldf.x), min(ldf.y, other.ldf.y), min(ldf.z, other.ldf.z));
			rub = glm::vec3(max(rub.x, other.rub.x), max(rub.y, other.rub.y), max(rub.z, other.rub.z));

			return *this;
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

	struct SphereBB {
		glm::vec3 pos;
		float radius;

		// produces new sphere that tightly bounds the original 2
		SphereBB& merge(const SphereBB& other) {
			auto deltaRad = std::abs(radius - other.radius);
			auto centerDist = glm::distance(pos, other.pos);

			if (centerDist < deltaRad) { // edge-case where my algorithm fails
				if (radius > other.radius) {
					return *this;
				}
				
				radius = other.radius;
				pos = other.pos;
			}
			else {
				// pretty proud of this even if it fails when one sphere contains the entirety of the other
				auto dir = glm::normalize(pos - other.pos);
				pos = ((pos - other.radius * dir) + (other.pos + radius * dir)) / 2.0f;
				radius = (centerDist + radius + other.radius) / 2.0;
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
		float distance(glm::vec3 p) const {
			glm::length(pos - p) - radius;
		}

		[[nodiscard]] // distance <= 0.0f when intersecting. The negative value doesn't make sense in the context of SDFs
		float distance(const SphereBB& other) const {
			glm::distance(pos, other.pos) - radius - other.radius;
		}

		[[nodiscard]]
		float distance(const AABB& aabb) const {
			return aabb.distance(pos) - radius;
		}

		[[nodiscard]]
		bool contains(glm::vec3 p) const {
			return distance(p) <= 0.0f;
		}

		[[nodiscard]]
		bool intersects(const SphereBB& other) const {
			return distance(other) <= 0.0f;
		}

		[[nodiscard]]
		bool intersects(const AABB& aabb) const {
			return distance(aabb) <= 0.0f;
		}
		
	};

	// THIS WILL INCREASE THE VOLUME OF THE BB
	AABB toAABB(const SphereBB& bb) {
		AABB out{};
		out.ldf = bb.pos - bb.radius;
		out.rub = bb.pos + bb.radius;
		return out;
	}

	// THIS WILL INCREASE THE VOLUME OF THE BB
	SphereBB toSphere(const AABB& bb) {
		SphereBB out{};
		out.pos = bb.center();
		out.radius = glm::distance(out.pos, bb.rub);
		return out;
	}
}