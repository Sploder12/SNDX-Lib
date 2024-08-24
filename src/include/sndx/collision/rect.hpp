#pragma once

#include "./volume.hpp"

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#undef GLM_ENABLE_EXPERIMENTAL
#else
#include <glm/gtx/component_wise.hpp>
#endif

#include <stdexcept>

namespace sndx::collision {


	// A Rectangle as described by two points.
	template <class VectorT = glm::vec2>
	class Rect : public Volume<VectorT> {
	public:
		using Volume<VectorT>::Vec;
		using Volume<VectorT>::Precision;
		
	protected:
		Vec m_p1, m_p2;

		/* Unsafe Construction Methods */

		constexpr Rect& setRawPoints(const Vec& p1, const Vec& p2) noexcept {
			m_p1 = p1;
			m_p2 = p2;

			return *this;
		}

		// constructs a Rect from two points, does not verify preconditions
		explicit constexpr Rect(const Vec& p1, const Vec& p2, nullptr_t) noexcept :
			m_p1{ p1 }, m_p2{ p2 } {}

		// constructs a Rect from dimensions, always positioned at 0.0, does not verify preconditions
		explicit constexpr Rect(const Vec& dims, nullptr_t) :
			m_p1{ Precision(0.0) }, m_p2{ dims } {}

	public:

		/* Safe Construction Methods */

		constexpr Rect(const Rect&) = default;

		// implicit Vec conversions are bad!
		template <class A, class B>
		Rect(const A&, const B&) = delete;

		// constructs a Rect from two points
		constexpr Rect(const Vec& p1, const Vec& p2) noexcept:
			m_p1{}, m_p2{} {

			setPoints(p1, p2);
		}

		// constructs a Rect from dimensions, always positioned at 0.0
		explicit constexpr Rect(const Vec& dims) :
			m_p1{ Precision(0.0) }, m_p2{ dims } {

			if (glm::compMin(dims) < Precision(0.0)) {
				throw std::invalid_argument("Dimensions of Rect must be >= 0.");
			}
		}

		// constructs a Rect from another volume
		template <class Volume>
		explicit constexpr Rect(const Volume& volume) noexcept :
			m_p1{}, m_p2{} {

			auto hsize = volume.getSize() * Precision(0.5);
			const auto& center = volume.getCenter();
			setRawPoints(center - hsize, center + hsize);
		}

		/* Transformation Methods */

		// safely sets the points defining the Rect
		constexpr Rect& setPoints(const Vec& p1, const Vec& p2) noexcept {
			m_p1 = glm::min(p1, p2);
			m_p2 = glm::max(p1, p2);

			return *this;
		}

		// sets the points of the Rect based off a position and dimensions
		constexpr Rect& setPosDims(const Vec& pos, const Vec& dims) noexcept {
			m_p1 = pos;
			m_p2 = m_p1 + dims;

			return *this;
		}

		// moves both points by vec
		constexpr Rect& translate(const Vec& vec) noexcept {
			m_p1 += vec;
			m_p2 += vec;

			return *this;
		}

		// returns a Rect that contains both Rects.
		constexpr Rect combine(const Rect& other) const noexcept {
			return Rect{ glm::min(getP1(), other.getP1()), glm::max(getP2(), other.getP2()), nullptr };
		}

		// returns a Rect that is contained by both Rects.
		// if such a Rect does not exist, the resulting Rect is still valid
		constexpr Rect reduce(const Rect& other) const noexcept {
			return Rect{ glm::max(getP1(), other.getP1()), glm::min(getP2(), other.getP2()) };
		}


		/* Info Methods */

		[[nodiscard]]
		constexpr const Vec& getP1() const noexcept {
			return m_p1;
		}

		[[nodiscard]]
		constexpr const Vec& getP2() const noexcept {
			return m_p2;
		}

		// get the "position" of the Rect
		[[nodiscard]]
		constexpr const Vec& getPosition() const noexcept {
			return getP1();
		}

		// get the dimensions of the Rect
		[[nodiscard]]
		constexpr Vec getSize() const noexcept override {
			return getP2() - getP1();
		}

		// get the Volume/Area of the Rect
		[[nodiscard]]
		constexpr Precision getArea() const noexcept {
			return glm::compMul(getSize());
		}

		// get the center point of the Rect
		[[nodiscard]]
		constexpr Vec getCenter() const noexcept override {
			return glm::mix(getP1(), getP2(), Precision(0.5));
		}

		
		/* Collision Related Methods */

		// returns true if there exists a point which is contained by both Rects
		[[nodiscard]]
		constexpr bool overlaps(const Rect& other) const noexcept {
			auto comparisons = glm::lessThanEqual(getP1(), other.getP2()) && glm::greaterThanEqual(getP2(), other.getP1());

			return glm::all(comparisons);
		}

		// returns true if the other Rect is fully contained by this Rect
		[[nodiscard]]
		constexpr bool contains(const Rect& other) const noexcept {
			auto comparisons = glm::lessThanEqual(getP1(), other.getP1()) && glm::greaterThanEqual(getP2(), other.getP2());

			return glm::all(comparisons);
		}

		// returns true if the point is contained by the Rect
		[[nodiscard]]
		constexpr bool contains(const Vec& point) const noexcept override {
			auto comparisons = glm::lessThanEqual(point, getP2()) && glm::greaterThanEqual(point, getP1());

			return glm::all(comparisons);
		}

		// returns true if the other volume is fully contained by this Rect
		template <class Volume> [[nodiscard]]
		constexpr bool contains(const Volume& other) const noexcept {
			return this->contains(Rect{ other });
		}

		// get the signed distance from a point
		// adapted from https://iquilezles.org/articles/distfunctions/
		[[nodiscard]]
		constexpr Precision getDistance(const Vec& point) const noexcept override {
			auto size = getSize() * Precision(0.5);
			auto tpoint = point - getPosition() - size;

			auto q = glm::abs(tpoint) - size;
			return glm::length(glm::max(q, Precision(0.0))) + glm::min(glm::compMax(q), Precision(0.0));
		}
	};

	template <size_t Dimensionality, typename InternalT = float, glm::qualifier Qualifier = glm::qualifier::defaultp>
	using RectND = Rect<glm::vec<Dimensionality, InternalT, Qualifier>>;

	using Rect1D = RectND<1>;
	using Rect2D = RectND<2>;
	using Rect3D = RectND<3>;
	using Rect4D = RectND<4>;
}