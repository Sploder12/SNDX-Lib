#pragma once

#include "./capsule.hpp"
#include "./circle.hpp"
#include "./rect.hpp"
#include "./orirect.hpp"
#include "./triangle.hpp"

#include "./gjk.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <optional>

namespace sndx::collision {
	template <Vector VectorT = glm::vec3>
	struct Collision {
		using Vec = VectorT;
		using Precision = typename Vec::value_type;

		// shape a translated by -normal * depth won't collide anymore
		// shape b translated by normal * depth ^
		VectorT normal{};
		Precision depth{};

		[[nodiscard]]
		constexpr Collision swapped() const noexcept {
			Collision out = *this;
			//std::swap(a, b);
			out.normal *= Precision(-1.0);
			return out;
		}
	};

	using Collision3D = Collision<glm::vec3>;

	template <Vector VectorT> [[nodiscard]]
	consteval VectorT getFallbackNormal() {
		VectorT f{};
		f[0] = typename Collision<VectorT>::Precision(1.0);
		return f;
	}

	// no shear, only uniform scale.
	struct Transform {
		glm::vec3 pos{};
		glm::quat rot{};
		float scale = 1.0f;

		[[nodiscard]]
		glm::mat4 asMatrix() const {
			glm::mat4 out = glm::mat4_cast(rot);
			out[0] *= scale;
			out[1] *= scale;
			out[2] *= scale;
			out[3] = glm::vec4{ pos, 1.0f };
			return out;
		}

		[[nodiscard]]
		Transform inverse() const {
			Transform out{};
			out.rot = glm::inverse(rot);
			out.scale = 1.0f / scale;
			out.pos = out.rot * (-pos * out.scale);
			return out;
		}

		explicit Transform(glm::vec3 pos = {}, const glm::quat& rot = {}, float scale = 1.0f):
			pos(pos), rot(rot), scale(scale) {}

		explicit Transform(const glm::mat4 matrix):
			pos(matrix[3]) {
			
			// no perspective
			assert(matrix[0][3] == 0.0f);
			assert(matrix[1][3] == 0.0f);
			assert(matrix[2][3] == 0.0f);
			assert(matrix[3][3] == 1.0f);

			glm::mat3 basis{ matrix };

			scale = glm::length(basis[0]);

			// avoid non-uniform scale
			assert(std::abs(scale - glm::length(basis[1])) < 0.0001f && std::abs(scale - glm::length(basis[2])) < 0.0001f);

			rot = glm::quat_cast(basis / scale);

			// avoid skew
			assert(std::abs(glm::dot(basis[0] / scale, basis[1] / scale)) < 0.0001f);
			assert(std::abs(glm::dot(basis[1] / scale, basis[2] / scale)) < 0.0001f);
			assert(std::abs(glm::dot(basis[2] / scale, basis[0] / scale)) < 0.0001f);
		}
	};

	template <class ShapeT>
	auto transform(const ShapeT& shape, const Transform& tform) {
		if constexpr (std::is_same_v<ShapeT, sndx::collision::Rect3D>) {
			return sndx::collision::OriRect3D{
				tform.pos + tform.rot * (shape.getCenter() * tform.scale),
				shape.getSize() * 0.5f * tform.scale,
				tform.rot 
			};
		}
		else if constexpr (std::is_same_v<ShapeT, sndx::collision::OriRect3D>) {
			return transform(shape, tform.asMatrix());
		}
		else if constexpr (std::is_same_v<ShapeT, sndx::collision::Circle3D>) {
			ShapeT out = shape;
			out.setRadius(shape.getRadius() * tform.scale);
			out.setPosition(tform.pos + tform.rot * (shape.getCenter() * tform.scale));
			return out;
		}
		else if constexpr (std::is_same_v<ShapeT, sndx::collision::Capsule3D>) {
			ShapeT out = shape;
			auto matform = tform.asMatrix();
			out.setPointA(matform * glm::vec4(shape.getPointA(), 1.0f));
			out.setPointB(matform * glm::vec4(shape.getPointB(), 1.0f));
			out.setRadius(shape.getRadius() * tform.scale);
			return out;
		}
		else if constexpr (std::is_same_v<ShapeT, sndx::collision::Tri3D>) {
			auto rs = glm::mat3_cast(tform.rot) * tform.scale;
			glm::vec3 p1 = tform.pos + rs * shape.getP1();
			glm::vec3 p2 = tform.pos + rs * shape.getP2();
			glm::vec3 p3 = tform.pos + rs * shape.getP3();
			return ShapeT{ p1, p2, p3 };
		}
	}

	// =================
	// = Sphere VS ... =
	// =================

	[[nodiscard]]
	inline Rect3D getBounds(const Circle3D& a) {
		return Rect3D{glm::vec3(a.getCenter() - glm::vec3(a.getRadius())), glm::vec3(a.getCenter() + glm::vec3(a.getRadius())) };
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr bool hasCollision(const Circle<VectorT>& a, const Circle<VectorT>& b) {
		return a.overlaps(b);
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr bool hasCollision(const Circle<VectorT>& a, const Rect<VectorT>& b) {
		auto closest = glm::clamp(a.getCenter(), b.getP1(), b.getP2());
		auto delta = a.getCenter() - closest;

		return glm::length2(delta) <= a.getRadius() * a.getRadius();
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr bool hasCollision(const Circle<VectorT>& a, const OriRect<VectorT>& b) {
		auto center = a.getCenter() - b.getCenter();
		auto local = glm::inverse(b.getRotation()) * center;

		auto closest = glm::clamp(local, -b.getHalfExtents(), b.getHalfExtents());
		auto delta = local - closest;

		return glm::length2(delta) <= a.getRadius() * a.getRadius();
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Circle<VectorT>& a, const Circle<VectorT>& b) {
		using Precision = typename Circle<VectorT>::Precision;

		auto ab = b.getCenter() - a.getCenter();
		auto l = glm::length(ab);

		if (l > a.getRadius() + b.getRadius()) {
			return std::nullopt;
		}

		Collision<VectorT> out{};
		out.normal = (l > Precision(0.00001)) ? ab / l : getFallbackNormal<VectorT>();
		//out.a = a.getCenter() + out.normal * a.getRadius();
		//out.b = b.getCenter() - out.normal * b.getRadius();
		out.depth = a.getRadius() + b.getRadius() - l;
		return out;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Circle<VectorT>& a, const Tri<VectorT>& b) {
		auto closest = b.closestPoint(a.getCenter());
		auto delta = a.getCenter() - closest;
		auto len2 = glm::dot(delta, delta);

		if (len2 > a.getRadius() * a.getRadius())
			return std::nullopt;

		auto len = std::sqrt(len2);

		Collision<VectorT> out{};
		if (len < typename Circle<VectorT>::Precision(0.0000001)) {
			out.normal = glm::normalize(b.normal());
		}
		else {
			out.normal = delta / len;
		}
		out.depth = a.getRadius() - len;
		//out.a = a.getCenter() - out.normal * a.getRadius();
		//out.b = closest;

		assert(!std::isnan(out.depth));

		return out;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Circle<VectorT>& a, const Rect<VectorT>& b) {
		if (auto r = getCollision(b, a)) {
			return r->swapped();
		}
		return std::nullopt;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Circle<VectorT>& a, const OriRect<VectorT>& b) {
		if (auto r = getCollision(b, a)) {
			return r->swapped();
		}
		return std::nullopt;
	}

	// =================
	// = Capsule VS ... =
	// =================

	[[nodiscard]]
	inline Rect3D getBounds(const Capsule3D& a) {
		auto r = glm::vec3{ a.getRadius() };
		Rect3D t{ a.getPointA(), a.getPointB() };
		return Rect3D{ t.getP1() - r, t.getP2() + r };
	}

	// ===============
	// = AABB VS ... =
	// ===============

	[[nodiscard]]
	inline Rect3D getBounds(const Rect3D& a) {
		return a;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr bool hasCollision(const Rect<VectorT>& a, const Rect<VectorT>& b) {
		return a.overlaps(b);
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr bool hasCollision(const Rect<VectorT>& a, const Circle<VectorT>& b) {
		return hasCollision(b, a);
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Rect<VectorT>& a, const Rect<VectorT>& b) {
		using Precision = typename Rect<VectorT>::Precision;

		auto minMaxes = glm::min(a.getP2(), b.getP2());
		auto maxMins = glm::max(a.getP1(), b.getP1());
		VectorT overlap = minMaxes - maxMins;

		uint8_t minAxis = 0;
		for (uint8_t i = 0; i < a.dimensionality(); ++i) {
			if (overlap[i] <= Precision(0.0))
				return std::nullopt;

			if (overlap[i] < overlap[minAxis])
				minAxis = i;
		}

		Collision<VectorT> out{};
		out.normal[minAxis] = b.getCenter(minAxis) > a.getCenter(minAxis) ? Precision(1.0) : Precision(-1.0);
		out.depth = overlap[minAxis];
		//out.a = glm::clamp(a.getCenter(), maxMins, minMaxes);
		//out.a[minAxis] = out.normal[minAxis] > Precision(0.0) ? a.getP2()[minAxis] : a.getP1()[minAxis];

		//out.b = glm::clamp(b.getCenter(), maxMins, minMaxes);
		//out.b[minAxis] = out.normal[minAxis] > Precision(0.0) ? b.getP2()[minAxis] : b.getP1()[minAxis];

		return out;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Rect<VectorT>& a, const OriRect<VectorT>& b) {
		if (auto r = getCollision(b, a)) {
			return r->swapped();
		}
		return std::nullopt;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Rect<VectorT>& a, const Circle<VectorT>& b) {
		return getCollision(OriRect<VectorT>{a}, b);
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Rect<VectorT>& a, const Tri<VectorT>& b) {
		return getCollision(sndx::collision::getSupportFn(a), sndx::collision::getSupportFn(b));
	}

	// ===============
	// = OBB VS ... =
	// ===============

	[[nodiscard]]
	inline Rect3D getBounds(const OriRect3D& obb) {
		auto axes = glm::mat3_cast(obb.getRotation());
		glm::vec3 extent{ 0.0f };
		for (glm::length_t i = 0; i < 3; ++i) {
			extent[i] = glm::dot(glm::abs(axes[i]), obb.getHalfExtents());
		}

		return Rect3D{ obb.getCenter() - extent, obb.getCenter() + extent };
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr bool hasCollision(const OriRect<VectorT>& a, const Circle<VectorT>& b) {
		return hasCollision(b, a);
	}

	namespace detail {
		// helper for SAT
		template <Vector VectorT = glm::vec3> [[nodiscard]]
		bool testAxis(VectorT axis, const OriRect<VectorT>& a, const OriRect<VectorT>& b, typename OriRect<VectorT>::Precision& minOverlap, VectorT& bestAxis) {
			using Precision = typename OriRect<VectorT>::Precision;
			
			auto len2 = glm::length2(axis);
			if (len2 < Precision(0.00000001))
				return true;

			// normalize axis
			axis /= std::sqrt(len2);

			auto ab = b.getCenter() - a.getCenter();
			auto dist = std::abs(glm::dot(ab, axis));

			auto axesA = glm::mat3_cast(a.getRotation());
			auto axesB = glm::mat3_cast(b.getRotation());

			Precision overlapA{};
			Precision overlapB{};
			for (uint8_t i = 0; i < a.dimensionality(); ++i) {
				overlapA += a.getHalfExtents()[i] * std::abs(glm::dot(axesA[i], axis));
				overlapB += b.getHalfExtents()[i] * std::abs(glm::dot(axesB[i], axis));
			}

			auto overlap = overlapA + overlapB - dist;

			if (overlap < Precision(0))
				return false;

			if (overlap < minOverlap) {
				minOverlap = overlap;
				bestAxis = axis;
			}

			return true;
		}
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const OriRect<VectorT>& a, const OriRect<VectorT>& b) {
		using Precision = typename OriRect<VectorT>::Precision;

		glm::mat3 axesA = glm::mat3_cast(a.getRotation());
		glm::mat3 axesB = glm::mat3_cast(b.getRotation());

		Precision minOverlap = std::numeric_limits<Precision>::max();
		VectorT minAxis{};

		// test a faces
		for (uint8_t i = 0; i < axesA.length(); ++i) {
			if (!detail::testAxis(axesA[i], a, b, minOverlap, minAxis))
				return std::nullopt;
		}

		// test b faces
		for (uint8_t i = 0; i < axesB.length(); ++i) {
			if (!detail::testAxis(axesB[i], a, b, minOverlap, minAxis))
				return std::nullopt;
		}

		// test edges
		for (uint8_t i = 0; i < axesA.length(); ++i) {
			for (uint8_t j = 0; j < axesB.length(); ++j) {
				auto axis = glm::cross(axesA[i], axesB[j]);

				if (!detail::testAxis(axis, a, b, minOverlap, minAxis))
					return std::nullopt;
			}
		}

		auto ba = b.getCenter() - a.getCenter();
		if (glm::dot(ba, minAxis) < Precision(0.0)) {
			minAxis = -minAxis;
		}

		Collision<VectorT> out{};
		out.normal = minAxis;
		out.depth = minOverlap * 2.0f;
		return out;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const OriRect<VectorT>& a, const Rect<VectorT>& b) {
		return getCollision(a, OriRect<VectorT>(b));
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const OriRect<VectorT>& a, const Circle<VectorT>& b) {
		using Precision = typename Rect<VectorT>::Precision;

		auto basis = glm::mat3_cast(glm::normalize(a.getRotation()));

		auto ab = b.getCenter() - a.getCenter();

		VectorT local = glm::transpose(basis) * ab;

		auto closest = glm::clamp(local, -a.getHalfExtents(), a.getHalfExtents());
		auto world = a.getCenter() + basis * closest;

		auto delta = b.getCenter() - world;
		auto len2 = glm::length2(delta);

		if (len2 > b.getRadius() * b.getRadius())
			return std::nullopt;

		Collision<VectorT> out{};
		if (len2 < Precision(0.0000001)) {
			auto faceDists = a.getHalfExtents() - glm::abs(local);
			uint8_t smallest = 0;
			for (uint8_t i = 1; i < a.dimensionality(); ++i) {
				if (faceDists[i] < faceDists[smallest]) {
					smallest = i;
				}
			}

			out.normal = local[smallest] > Precision(0.0) ? basis[smallest] : -basis[smallest];
			out.depth = b.getRadius() + faceDists[smallest];
		}
		else {
			auto l = std::sqrt(len2);
			out.normal = delta / l;
			out.depth = b.getRadius() - l;
		}
		return out;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const OriRect<VectorT>& a, const Tri<VectorT>& b) {
		// GJK + EPA wins over SAT in this case
		return getCollision(sndx::collision::getSupportFn(a), sndx::collision::getSupportFn(b));
	}

	// ===================
	// = Triangle VS ... =
	// ===================

	[[nodiscard]]
	inline Rect3D getBounds(const Tri3D& tri) {
		auto a = Rect3D{ tri.getP1(), tri.getP2() };
		return a.combine(Rect3D{ tri.getP3(), tri.getP2() });
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Tri<VectorT>& a, const Circle<VectorT>& b) {
		if (auto r = getCollision(b, a)) {
			return r->swapped();
		}
		return std::nullopt;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Tri<VectorT>& a, const Rect<VectorT>& b) {
		if (auto r = getCollision(b, a)) {
			return r->swapped();
		}
		return std::nullopt;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Tri<VectorT>& a, const OriRect<VectorT>& b) {
		if (auto r = getCollision(b, a)) {
			return r->swapped();
		}
		return std::nullopt;
	}

	// ==========================
	// = Arbitrary Convex (gjk) =
	// ==========================
	template <std::invocable<glm::vec3> FnA, std::invocable<glm::vec3> FnB> [[nodiscard]]
	bool hasCollision(FnA&& sptA, FnB&& sptB) {
		return bool(gjk(sptA, sptB));
	}

	template <class T, class U> [[nodiscard]]
	bool hasCollision(const T& a, const U& b) {
		// slow fallback, avoid where possible
		return bool(getCollision(a, b));
	}

	template <std::invocable<glm::vec3> FnA, std::invocable<glm::vec3> FnB> [[nodiscard]]
	std::optional<Collision3D> getCollision(FnA&& sptA, FnB&& sptB) {
		if (auto simplex = gjk(sptA, sptB)) {
			EpaResult res = epa(*simplex, std::forward<FnA>(sptA), std::forward<FnB>(sptB));

			return Collision3D{
			//	.a = res.a, .b = res.b,
				.normal = -res.normal, .depth = res.depth
			};
		}
		return std::nullopt;
	}
}