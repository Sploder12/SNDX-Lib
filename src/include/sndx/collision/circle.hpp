#pragma once

#include "./volume.hpp"

#include <stdexcept>
#include <numbers>

namespace sndx::collision {
	// A Circle as described by a point and radius.
	template <class VectorT = glm::vec2>
	class Circle : public Volume<VectorT> {
	public:
		using Vec = Volume<VectorT>::Vec;
		using Precision = Volume<VectorT>::Precision;

	protected:
		Vec m_pos;
		Precision m_radius;

		/* Unsafe Construction Methods */

		constexpr Circle& setRawRadius(Precision radius) noexcept {
			m_radius = radius;

			return *this;
		}

		// constructs a circle from a point and radius, does not verify preconditions
		explicit constexpr Circle(const Vec& pos, Precision radius, nullptr_t) noexcept :
			m_pos(pos), m_radius(radius) {}

	public:

		/* Safe Construction Methods */

		constexpr Circle(const Circle&) = default;

		// constructs from position and radius
		constexpr Circle(const Vec& pos, Precision radius):
			m_pos{ pos }, m_radius{} {

			setRadius(radius);
		}

		// constructs from radius at 0.0
		explicit constexpr Circle(Precision radius) :
			Circle(Vec{}, radius) {}

		// constructs a Circle from another volume
		template <class Volume>
		explicit constexpr Circle(const Volume& volume) noexcept :
			m_pos(volume.getCenter()), 
			m_radius(glm::length(volume.getSize()) * Precision(0.5)) {}


		/* Transformation Methods */

		// set the position
		constexpr Circle& setPosition(const Vec& pos) noexcept {
			m_pos = pos;
			
			return *this;
		}

		// safely set the radius
		constexpr Circle& setRadius(Precision radius) {
			if (radius < Precision(0.0)) {
				throw std::invalid_argument("Radius cannot be negative");
			}

			m_radius = radius;
			return *this;
		}

		// moves both points by vec
		constexpr Circle& translate(const Vec& vec) noexcept {
			m_pos += vec;

			return *this;
		}

		// returns a Circle that contains both Circles.
		constexpr Circle combine(const Circle& other) const noexcept {
			const auto& radius = getRadius();
			const auto& pos = getPosition();

			const auto& otherRadius = other.getRadius();
			const auto& otherPos = other.getPosition();

			auto deltaRad = std::abs(radius - otherRadius);
			auto centerDist = glm::distance(pos, otherPos);

			if (centerDist < deltaRad || centerDist == Precision(0.0)) { // edge-case where my algorithm fails
				if (radius > otherRadius) {
					return *this;
				}
				
				return other;
			}

			// pretty proud of this even if it fails when one sphere contains the entirety of the other
			auto dir = (pos - otherPos) / centerDist; // normalized direction
			auto outPos = ((pos + radius * dir) + (otherPos - otherRadius * dir)) * Precision(0.5);
			auto outRadius = (centerDist + radius + other.radius) * Precision(0.5);

			return Circle{ outPos, outRadius, nullptr };
		}


		/* Info Methods */

		// get the position
		[[nodiscard]]
		constexpr const Vec& getPosition() const noexcept {
			return m_pos;
		}

		// get the radius
		[[nodiscard]]
		constexpr const Precision getRadius() const noexcept {
			return m_radius;
		}

		// get the dimensions of the Circle
		[[nodiscard]]
		constexpr Vec getSize() const noexcept override {
			return Vec{getRadius()};
		}

		// get the Volume/Area of the Circle
		[[nodiscard]]
		constexpr Precision getArea() const noexcept {
			const auto& r = getRadius();

			constexpr auto pi = std::numbers::pi_v<Precision>;
			constexpr auto d = this->getDimensionality();

			if constexpr (d == 0) {
				return Precision(1.0);
			}
			if constexpr (d == 1) {
				return Precision(2.0) * r;
			}
			else if constexpr (d == 2) {
				return pi * r * r;
			}
			else if constexpr (d == 3) {
				return Precision(4.0) / Precision(3.0) * pi * r * r * r;
			}
			else if constexpr (d == 4) {
				return Precision(0.5) * pi * pi * r * r * r * r;
			}
			// > 4D is not handled since glm::vec does not go beyond 4D
		}

		// get the center point of the Circle
		[[nodiscard]]
		constexpr Vec getCenter() const noexcept override {
			return getPosition();
		}


		/* Collision Related Methods */

		// returns true if there exists a point which is contained by both Volumes
		template <class Volume> [[nodiscard]]
		constexpr bool overlaps(const Volume& other) const noexcept {
			return this->getDistance(other) <= Precision(0.0);
		}

		// returns true if the other Circle is fully contained by this Circle
		[[nodiscard]]
		constexpr bool contains(const Circle& other) const noexcept {
			if (getRadius() < other.getRadius()) return false;

			auto deltaRad = getRadius() - other.getRadius();
			auto centerDist = glm::distance(getPosition(), other.getPosition());

			return centerDist <= deltaRad;
		}

		// get the signed distance from a point
		[[nodiscard]]
		constexpr Precision getDistance(const Vec& point) const noexcept override {
			return glm::distance(getPosition(), point) - getRadius();
		}

		// get the signed distance from another Volume,
		// negative distances are valid but strange
		template <class Volume> [[nodiscard]]
		constexpr Precision getDistance(const Volume& other) const noexcept {
			return other.getDistance(getPosition()) - getRadius();
		}
	};

	template <size_t Dimensionality, typename InternalT = float, glm::qualifier Qualifier = glm::qualifier::defaultp>
	using CircleND = Circle<glm::vec<Dimensionality, InternalT, Qualifier>>;

	using Circle1D = CircleND<1>;
	using Circle2D = CircleND<2>;
	using Circle3D = CircleND<3>;
	using Circle4D = CircleND<4>;
}