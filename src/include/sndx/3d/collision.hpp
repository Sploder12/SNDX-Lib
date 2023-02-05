#pragma once

#include <glm/glm.hpp>

#include <algorithm>

namespace sndx {

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
		constexpr double area() const {
			float w = rub.x - ldf.x;
			float l = rub.z - ldf.z;
			float h = rub.y - ldf.y;

			return l * w * h;
		}

		[[nodiscard]]
		constexpr double surfaceArea() const {
			float w = rub.x - ldf.x;
			float l = rub.z - ldf.z;
			float h = rub.y - ldf.y;

			return 2.0 * (w * l + h * l + w * h);
		}
	};
}