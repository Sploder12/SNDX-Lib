#pragma once

#include <glm/glm.hpp>

namespace sndx::collision {
	template <class VectorT = glm::vec2>
	struct Volume {

		using Vec = VectorT;
		using Precision = Vec::value_type;

		// Number of dimensions of the volume
		static constexpr size_t s_dimensionality = Vec::length();

		constexpr virtual ~Volume() noexcept = default;


		/* Info Methods */

		[[nodiscard]]
		static constexpr size_t getDimensionality() noexcept {
			return s_dimensionality;
		}

		// get the dimensions of the volume
		[[nodiscard]]
		constexpr virtual Vec getSize() const noexcept = 0;

		// get the center point of the volume
		[[nodiscard]]
		constexpr virtual Vec getCenter() const noexcept = 0;


		/* Collision Related Methods */

		// get the signed distance from a point
		[[nodiscard]]
		constexpr virtual Precision getDistance(const Vec& point) const noexcept = 0;

		// returns true if the point is contained by the volume
		[[nodiscard]]
		constexpr virtual bool contains(const Vec& point) const noexcept {
			return this->getDistance(point) <= Precision(0.0);
		}
	};
}