#pragma once

#include "./volume.hpp"

#include "./circle.hpp"
#include "./rect.hpp"

namespace sndx::collision {
	// A Capsule as described by two points and radius.
	template <Vector VectorT = glm::vec2>
	class Capsule {
	public:
		using Vec = VectorT;
		using Precision = typename Vec::value_type;

		static constexpr size_t dimensionality() noexcept {
			return Vec::length();
		}

	private:
		Vec m_a, m_b;
		Precision m_radius;

		/* Unsafe Construction Methods */

		constexpr Capsule& setRawRadius(Precision radius) noexcept {
			m_radius = radius;

			return *this;
		}

		// constructs a capsule from 2 points and radius, does not verify preconditions
		explicit constexpr Capsule(const Vec& posA, const Vec& posB, Precision radius, std::nullptr_t) noexcept:
			m_a(posA), m_b(posB), m_radius(radius) {
		}

	public:
		/* Safe Construction Methods */

		constexpr Capsule(const Capsule&) = default;

		// constructs from positions and radius
		constexpr Capsule(const Vec& posA, const Vec& posB, Precision radius):
			m_a{ posA }, m_b{posB}, m_radius{} {

			setRadius(radius);
		}

		/* Transformation Methods */

		// set the position
		constexpr Capsule& setPositionA(const Vec& pos) noexcept {
			m_a = pos;

			return *this;
		}

		constexpr Capsule& setPositionB(const Vec& pos) noexcept {
			m_b = pos;

			return *this;
		}

		// safely set the radius
		constexpr Capsule& setRadius(Precision radius) {
			if (radius < Precision(0.0)) {
				throw std::invalid_argument("Radius cannot be negative");
			}

			m_radius = radius;
			return *this;
		}

		// moves both points by vec
		constexpr Capsule& translate(const Vec& vec) noexcept {
			m_a += vec;
			m_b += vec;

			return *this;
		}

		/* Info Methods */

		// get positions
		[[nodiscard]]
		constexpr const Vec& getPointA() const noexcept {
			return m_a;
		}

		[[nodiscard]]
		constexpr const Vec& getPointB() const noexcept {
			return m_b;
		}

		// get the radius
		[[nodiscard]]
		constexpr Precision getRadius() const noexcept {
			return m_radius;
		}

		// get the dimensions of the Capsule
		[[nodiscard]]
		constexpr Vec getSize() const noexcept {
			Rect<Vec> boundingRect{ getPointA(), getPointB() };
			return boundingRect.getSize() + m_radius * Precision(2.0);
		}

		// get the Volume/Area of the Circle
		[[nodiscard]]
		constexpr Precision getArea() const noexcept {
			const auto& r = getRadius();

			if constexpr (dimensionality() == 1) {
				return glm::distance(m_a, m_b) + r * Precision(2.0);
			}
			else {
				auto cylinder = CircleND<dimensionality() - 1, Precision>{ r };
				auto cap = Circle<Vec>{ r };

				return cylinder.getArea() * glm::distance(m_a, m_b) + cap.getArea();
			}
		}

		// get the center point of the Circle
		[[nodiscard]]
		constexpr Vec getCenter() const noexcept {
			return (m_a + m_b) / Precision(2.0);
		}

		[[nodiscard]]
		constexpr Precision getCenter(uint8_t axis) const noexcept {
			return getCenter()[axis];
		}


		/* Collision Related Methods */

		// get the signed distance from a point
		[[nodiscard]]
		constexpr Precision distance(const Vec& point) const noexcept {
			auto pa = point - m_a;
			auto ba = m_b - m_a;
			auto h = glm::clamp(glm::dot(pa, ba) / glm::dot(ba, ba), Precision(0.0), Precision(1.0));
			return glm::length(pa - ba * h) - getRadius();
		}

		[[nodiscard]]
		constexpr bool contains(const Vec& point) const noexcept {
			return distance(point) <= Precision(0.0);
		}

		struct RaycastResult {
			const Capsule<VectorT>* capsule = nullptr;
			Precision near = std::numeric_limits<Precision>::min();
			VectorT norm;

			[[nodiscard]]
			constexpr bool hit() const noexcept {
				return capsule != nullptr;
			}

			[[nodiscard]]
			constexpr Precision distance() const noexcept {
				return near;
			}

			[[nodiscard]]
			constexpr VectorT normal() const noexcept {
				return norm;
			}

			void setDistance(Precision dist) noexcept {
				near = dist;
			}

			void setNormal(VectorT norm) noexcept {
				this->norm = norm;
			}
		};
		using result_type = RaycastResult;

		[[nodiscard]] // cull is ignored
		constexpr RaycastResult raycast(const Vec& from, const Vec& dir, bool = false) const noexcept {
			RaycastResult out{};

			auto axis = m_b - m_a;
			auto rel = from - m_a;

			auto relLen = glm::dot(rel, rel);
			auto axisLen = glm::dot(axis, axis);
			auto dirDif = glm::dot(axis, dir);
			auto relDif = glm::dot(axis, rel);
			auto relDir = glm::dot(dir, rel);

			auto a = axisLen - dirDif * dirDif;
			auto b = axisLen * relDir - relDif * dirDif;
			auto c = axisLen * relLen - relDif * relDif - m_radius * m_radius * axisLen;

			// cylinder test
			if (a != Precision(0.0)) {
				if (auto discriminant = b * b - a * c; discriminant >= Precision(0.0)) {
					discriminant = std::sqrt(discriminant);

					auto dist = (-b - discriminant) / a;
					if (dist >= Precision(0.0)) {
						auto axisPos = relDif + dist * dirDif;
						if (axisPos > Precision(0.0) && axisPos < axisLen) {
							out.near = dist;
							out.capsule = this;

							auto hitPnt = from + dir * dist;
							auto onAxis = m_a + axis * (axisPos / axisLen);

							out.norm = glm::normalize(hitPnt - onAxis);
						}
						
					}
				}
			}

			auto cap = Circle<VectorT>{ m_a, m_radius };
			if (auto tmpRes = cap.raycast(from, dir); tmpRes.hit()) {
				if (tmpRes.distance() >= Precision(0.0) && tmpRes.distance() < out.near) {
					out.capsule = this;
					out.near = tmpRes.distance();
					out.norm = tmpRes.normal();
				}
			}

			cap.setPosition(m_b);
			if (auto tmpRes = cap.raycast(from, dir); tmpRes.hit()) {
				if (tmpRes.distance() >= Precision(0.0) && tmpRes.distance() < out.near) {
					out.capsule = this;
					out.near = tmpRes.distance();
					out.norm = tmpRes.normal();
				}
			}

			return out;
		}
	};

	template <size_t Dimensionality, typename InternalT = float, glm::qualifier Qualifier = glm::qualifier::defaultp>
	using CapsuleND = Capsule<glm::vec<Dimensionality, InternalT, Qualifier>>;

	using Capsule1D = CapsuleND<1>;
	using Capsule2D = CapsuleND<2>;
	using Capsule3D = CapsuleND<3>;
	using Capsule4D = CapsuleND<4>;

	static_assert(VolumeN<Capsule1D, 1>);
	static_assert(VolumeN<Capsule2D, 2>);
	static_assert(VolumeN<Capsule3D, 3>);
	static_assert(VolumeN<Capsule4D, 4>);
}