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

	// difference is the center of mass - the geometric center (or vice versa)
	// make sure the difference and tensor are in the same vector space
	[[nodiscard]]
	inline glm::mat3 parallelAxisTheorem(const glm::mat3& tensor, float mass, const glm::vec3& difference) {
		return tensor + mass * (glm::mat3{ glm::length2(difference) } - glm::outerProduct(difference, difference));
	}

	template <Vector VectorT = glm::vec3>
	struct Collision {
		using Vec = VectorT;
		using Precision = typename Vec::value_type;

		// shape a translated by -normal * depth won't collide anymore
		// shape b translated by normal * depth ^
		VectorT normal{};
		Precision depth{};

		// contact points
		VectorT a{}, b{};

		[[nodiscard]]
		constexpr Collision swapped() const noexcept {
			Collision out = *this;
			out.a = b;
			out.b = a;
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

	namespace detail {
		template <bool uniformScale = true>
		struct Transform {
			using ScaleT = std::conditional_t<uniformScale, float, glm::vec3>;

			glm::vec3 pos{};
			glm::quat rot{};
			ScaleT scale{ 1.0f };

			[[nodiscard]]
			bool isIsotropic() const noexcept {
				if constexpr (uniformScale) {
					return true;
				}
				else {
					return std::abs(scale.x - scale.y) < 0.0001f && std::abs(scale.y - scale.z) < 0.0001f;
				}
			}

			[[nodiscard]]
			glm::mat4 asMatrix() const {
				glm::mat4 out = glm::mat4_cast(rot);
				if constexpr (uniformScale) {
					out[0] *= scale;
					out[1] *= scale;
					out[2] *= scale;
				}
				else {
					out[0] *= scale.x;
					out[1] *= scale.y;
					out[2] *= scale.z;
				}
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

			explicit Transform(glm::vec3 pos = {}, const glm::quat& rot = {}, ScaleT scale = ScaleT{ 1.0f }):
				pos(pos), rot(rot), scale(scale) {
			}

			explicit Transform(const glm::mat4& matrix):
				pos(matrix[3]) {

				auto basis = glm::mat3{ matrix };
				if constexpr (uniformScale) {
					scale = glm::length(basis[0]);

					// avoid non-uniform scale
					assert(std::abs(scale - glm::length(basis[1])) < 0.0001f && std::abs(scale - glm::length(basis[2])) < 0.0001f);
					rot = glm::quat_cast(basis / scale);

					// avoid skew
					assert(std::abs(glm::dot(basis[0] / scale, basis[1] / scale)) < 0.0001f);
					assert(std::abs(glm::dot(basis[1] / scale, basis[2] / scale)) < 0.0001f);
					assert(std::abs(glm::dot(basis[2] / scale, basis[0] / scale)) < 0.0001f);
				}
				else {
					scale.x = glm::length(basis[0]);
					scale.y = glm::length(basis[1]);
					scale.z = glm::length(basis[2]);

					glm::mat3 rotMat{
						basis[0] / scale.x,
						basis[1] / scale.y,
						basis[2] / scale.z
					};
					rot = glm::quat_cast(rotMat);

					// avoid skew
					assert(std::abs(glm::dot(basis[0] / scale.x, basis[1] / scale.y)) < 0.0001f);
					assert(std::abs(glm::dot(basis[1] / scale.y, basis[2] / scale.z)) < 0.0001f);
					assert(std::abs(glm::dot(basis[2] / scale.z, basis[0] / scale.x)) < 0.0001f);
				}

				// no perspective
				assert(matrix[0][3] == 0.0f);
				assert(matrix[1][3] == 0.0f);
				assert(matrix[2][3] == 0.0f);
				assert(matrix[3][3] == 1.0f);
			}
		};
	}

	// no shear, only uniform scale.
	using TransformIsotropic = detail::Transform<true>;

	// no shear, non-uniform scale.
	using Transform = detail::Transform<false>;

	[[nodiscard]]
	inline auto transform(const sndx::collision::Rect3D& shape, const Transform& tform) {

		return sndx::collision::OriRect3D{
			tform.pos + tform.rot * (shape.getCenter() * tform.scale),
			shape.getSize() * 0.5f * glm::abs(tform.scale),
			tform.rot
		};
	}

	[[nodiscard]]
	inline auto transform(const sndx::collision::Tri3D& shape, const Transform& tform) {
		glm::vec3 p1{ tform.pos + tform.rot * (shape.getP1() * tform.scale) };
		glm::vec3 p2{ tform.pos + tform.rot * (shape.getP2() * tform.scale) };
		glm::vec3 p3{ tform.pos + tform.rot * (shape.getP3() * tform.scale) };
		return Tri3D{ p1, p2, p3 };
	}

	template <class ShapeT> [[nodiscard]]
	auto transform(const ShapeT& shape, const TransformIsotropic& tform) {
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
		else {
			static_assert(false);
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
		out.depth = a.getRadius() + b.getRadius() - l;
		out.a = a.getCenter() + out.normal * a.getRadius();
		out.b = b.getCenter() - out.normal * b.getRadius();
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
		out.a = a.getCenter() - out.normal * a.getRadius();
		out.b = closest;

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

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Circle<VectorT>& a, const Capsule<VectorT>& b) {
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

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Capsule<VectorT>& a, const Capsule<VectorT>& b) {
		auto [centerA, centerB] = sndx::math::closestPoints(a.getPointA(), a.getPointB(), b.getPointA(), b.getPointB());
		return getCollision(Circle<VectorT>(centerA, a.getRadius()), Circle<VectorT>(centerB, b.getRadius()));
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Capsule<VectorT>& a, const Circle<VectorT>& b) {
		using Precision = typename Circle<VectorT>::Precision;

		auto ab = a.getPointB() - a.getPointA();
		auto l2 = glm::length2(ab);

		if (l2 <= Precision(0.00001)) [[unlikely]] { // degenerate capsule
			return getCollision(Circle<VectorT>(ab, a.getRadius()), b);
		}

		auto t = glm::dot(b.getCenter() - a.getPointA(), ab) / l2;
		t = glm::clamp(t, 0.0f, 1.0f);

		auto closestOnLine = a.getPointA() + t * ab;
		auto delta = b.getCenter() - closestOnLine;

		auto dist = glm::length(delta);
		if (dist > a.getRadius() + b.getRadius()) {
			return std::nullopt;
		}

		Collision<VectorT> out{};
		out.normal = (dist > Precision(0.00001)) ? delta / dist : getFallbackNormal<VectorT>();
		out.depth = a.getRadius() + b.getRadius() - dist;
		out.a = closestOnLine + out.normal * a.getRadius();
		out.b = b.getCenter() - out.normal * b.getRadius();
		return out;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Capsule<VectorT>& a, const Rect<VectorT>& b) {
		using Precision = typename OriRect<VectorT>::Precision;

		auto ab = a.getPointB() - a.getPointA();
		auto l2 = glm::length2(ab);
		if (l2 < Precision(0.00001)) [[unlikely]] {
			// degenerate capsule
			Collision<VectorT> out{};
			out.a = a.getPointA();
			out.b = glm::clamp(out.a, b.getP1(), b.getP2());

			auto dist = glm::distance(out.a, out.b);
			if (dist > a.getRadius()) {
				return std::nullopt;
			}

			out.normal = (dist > Precision(0.00001)) ? (out.b - out.a) / dist : getFallbackNormal<VectorT>();
			out.depth = a.getRadius() - dist;
			return out;
		}

		// line segment vs aabb closest point
		auto halfExtent = b.getSize() * Precision(0.5);

		auto center = a.getCenter() - b.getCenter();
		auto halfDir = ab * Precision(0.5);

		auto best2 = std::numeric_limits<Precision>::max();
		VectorT bestSeg{}, bestBox{};

		for (auto t : {Precision(-1.0), Precision(1.0)}) {
			auto s = center + halfDir * t;
			auto b = glm::clamp(s, -halfExtent, halfExtent);
			auto dist2 = glm::distance2(s, b);
			if (dist2 < best2) {
				best2 = dist2;
				bestSeg = s;
				bestBox = b;
			}
		}

		for (uint16_t axis = 0; axis < a.dimensionality(); ++axis) {
			if (std::abs(halfDir[axis]) < Precision(0.00001)) {
				continue;
			}

			for (auto side : { Precision(-1.0), Precision(1.0) }) {
				auto t = (side * halfExtent[axis] - center[axis]) / halfDir[axis];
				if (t < Precision(-1.0) || t > Precision(1.0)) {
					continue;
				}

				auto s = center + halfDir * t;
				auto b = glm::clamp(s, -halfExtent, halfExtent);
				auto dist2 = glm::distance2(s, b);
				if (dist2 < best2) {
					best2 = dist2;
					bestSeg = s;
					bestBox = b;
				}
			}
		}

		if (best2 > a.getRadius() * a.getRadius()) {
			return std::nullopt;
		}
		auto dist = std::sqrt(best2);

		Collision<VectorT> out{};
		out.depth = a.getRadius() - dist;
		out.normal = out.depth <= Precision(0.00001) ? getFallbackNormal<VectorT>() : (bestBox - bestSeg) / dist;
		out.a = bestSeg + out.normal * a.getRadius();
		out.b = bestBox;
		return out;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Capsule<VectorT>& a, const OriRect<VectorT>& b) {
		using Precision = typename OriRect<VectorT>::Precision;

		auto invRot = glm::inverse(b.getRotation());

		auto localA = invRot * (a.getPointA() - b.getCenter());
		auto localB = invRot * (a.getPointB() - b.getCenter());

		auto localRect = Rect<VectorT>(-b.getHalfExtents(), b.getHalfExtents());
		
		auto localRes = getCollision(Capsule<VectorT>{localA, localB, a.getRadius()}, localRect);
		if (!localRes) {
			return std::nullopt;
		}

		Collision<VectorT> out{};
		out.normal = b.getRotation() * (localRes->normal * localRes->depth);
		out.depth = glm::length(out.normal);
		out.normal = out.depth <= Precision(0.00001) ? getFallbackNormal<VectorT>() : out.normal / out.depth;
		out.a = b.getCenter() + b.getRotation() * localRes->a;
		out.b = b.getCenter() + b.getRotation() * localRes->b;
		return out;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Capsule<VectorT>& a, const Tri<VectorT>& b) {
		using Precision = typename Tri<VectorT>::Precision;

		// based on Ericson's collision detection
		std::pair<VectorT, VectorT> best{};
		Precision minDist2 = std::numeric_limits<Precision>::max();

		// test edges and segment ends
		auto ab = sndx::math::closestPoints(a.getPointA(), a.getPointB(), b.getP1(), b.getP2());
		if (auto l2 = glm::distance2(ab.first, ab.second); l2 < minDist2) {
			best = ab;
			minDist2 = l2;
		}

		auto bc = sndx::math::closestPoints(a.getPointA(), a.getPointB(), b.getP2(), b.getP3());
		if (auto l2 = glm::distance2(bc.first, bc.second); l2 < minDist2) {
			best = bc;
			minDist2 = l2;
		}

		auto ac = sndx::math::closestPoints(a.getPointA(), a.getPointB(), b.getP1(), b.getP3());
		if (auto l2 = glm::distance2(ac.first, ac.second); l2 < minDist2) {
			best = ac;
			minDist2 = l2;
		}

		auto uvwA = b.uvw(a.getPointA());
		if (!glm::any(glm::lessThan(uvwA, VectorT(0.0)))) {
			auto p1 = a.getPointA();
			auto p2 = b.fromUVW(uvwA);
			if (auto l2 = glm::distance2(p1, p2); l2 < minDist2) {
				best = std::make_pair(p1, p2);
				minDist2 = l2;
			}
		}

		auto uvwB = b.uvw(a.getPointB());
		if (!glm::any(glm::lessThan(uvwB, VectorT(0.0)))) {
			auto p1 = a.getPointB();
			auto p2 = b.fromUVW(uvwB);
			if (auto l2 = glm::distance2(p1, p2); l2 < minDist2) {
				best = std::make_pair(p1, p2);
				minDist2 = l2;
			}
		}

		auto dist = std::sqrt(minDist2);
		if (dist > a.getRadius()) {
			return std::nullopt;
		}

		Collision<VectorT> out{};
		out.normal = (dist > Precision(0.00001)) ? (best.first - best.second) / dist : getFallbackNormal<VectorT>();
		out.depth = a.getRadius() - dist;
		out.a = best.second;
		out.b = best.first + out.normal * a.getRadius();
		return out;
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
		out.a = glm::clamp(a.getCenter(), maxMins, minMaxes);
		out.a[minAxis] = out.normal[minAxis] > Precision(0.0) ? a.getP2()[minAxis] : a.getP1()[minAxis];

		out.b = glm::clamp(b.getCenter(), maxMins, minMaxes);
		out.b[minAxis] = out.normal[minAxis] > Precision(0.0) ? b.getP2()[minAxis] : b.getP1()[minAxis];

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
	constexpr std::optional<Collision<VectorT>> getCollision(const Rect<VectorT>& a, const Capsule<VectorT>& b) {
		if (auto r = getCollision(b, a)) {
			return r->swapped();
		}
		return std::nullopt;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const Rect<VectorT>& a, const Tri<VectorT>& b) {
		return getCollision(sndx::collision::getSupportFn(a), sndx::collision::getSupportFn(b), 3 + 8);
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

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const OriRect<VectorT>& a, const Capsule<VectorT>& b) {
		if (auto r = getCollision(b, a)) {
			return r->swapped();
		}
		return std::nullopt;
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
	constexpr bool hasCollision(const OriRect<VectorT>& a, const OriRect<VectorT>& b) {
		using Precision = typename OriRect<VectorT>::Precision;

		glm::mat3 axesA = glm::mat3_cast(a.getRotation());
		glm::mat3 axesB = glm::mat3_cast(b.getRotation());

		Precision minOverlap = std::numeric_limits<Precision>::max();
		VectorT minAxis{};

		// test a faces
		for (uint8_t i = 0; i < axesA.length(); ++i) {
			if (!detail::testAxis(axesA[i], a, b, minOverlap, minAxis))
				return false;
		}

		// test b faces
		for (uint8_t i = 0; i < axesB.length(); ++i) {
			if (!detail::testAxis(axesB[i], a, b, minOverlap, minAxis))
				return false;
		}

		// test edges
		for (uint8_t i = 0; i < axesA.length(); ++i) {
			for (uint8_t j = 0; j < axesB.length(); ++j) {
				auto axis = glm::cross(axesA[i], axesB[j]);

				if (!detail::testAxis(axis, a, b, minOverlap, minAxis))
					return false;
			}
		}

		return true;
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

		// @TODO figure out why GJK-EPA MTV is wrong (but collision points are right)
		auto points = getCollision(getSupportFn(a), getSupportFn(b), 8 + 8);
		if (!points) {
			return std::nullopt;
		}
		out.a = points->a;
		out.b = points->b;
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

		out.a = world;
		out.b = b.getCenter() + out.normal * b.getRadius();
		return out;
	}

	template <Vector VectorT> [[nodiscard]]
	constexpr std::optional<Collision<VectorT>> getCollision(const OriRect<VectorT>& a, const Tri<VectorT>& b) {
		// GJK + EPA wins over SAT in this case
		return getCollision(sndx::collision::getSupportFn(a), sndx::collision::getSupportFn(b), 3 + 8);
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
	constexpr std::optional<Collision<VectorT>> getCollision(const Tri<VectorT>& a, const Capsule<VectorT>& b) {
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
	bool hasCollision(FnA&& sptA, FnB&& sptB, uint16_t maxIterations = 32) {
		return bool(gjk(sptA, sptB, maxIterations));
	}

	template <class T, class U> [[nodiscard]]
	bool hasCollision(const T& a, const U& b) {
		// slow fallback, avoid where possible
		return bool(getCollision(a, b));
	}

	template <std::invocable<glm::vec3> FnA, std::invocable<glm::vec3> FnB> [[nodiscard]]
	std::optional<Collision3D> getCollision(FnA&& sptA, FnB&& sptB, uint16_t maxIterations = 32) {
		if (auto simplex = gjk(sptA, sptB, maxIterations)) {
			EpaResult res = epa(*simplex, std::forward<FnA>(sptA), std::forward<FnB>(sptB));

			return Collision3D{
				.normal = -res.normal, .depth = res.depth,
				.a = res.a, .b = res.b,
			};
		}
		return std::nullopt;
	}
}