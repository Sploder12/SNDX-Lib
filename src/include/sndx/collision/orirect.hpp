#pragma once

#include "./volume.hpp"
#include "./rect.hpp"

#include <glm/gtc/quaternion.hpp>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#undef GLM_ENABLE_EXPERIMENTAL
#else
#include <glm/gtx/component_wise.hpp>
#endif

#include <stdexcept>

namespace sndx::collision {

	// An oriented rectangle described by center, half extents, axes
	template <Vector VectorT = glm::vec2>
	class OriRect {
	public:
		using Vec = VectorT;
		using Precision = Vec::value_type;

		static constexpr size_t dimensionality() noexcept {
			return Vec::length();
		}

	protected:
		glm::quat rotation{};
		Vec center;
		Vec halfExtents;

	public:
		constexpr explicit OriRect(Vec center, Vec halfExtents, const glm::quat& rot):
			rotation(rot), center(center), halfExtents(halfExtents) {}

		constexpr explicit OriRect(const Rect<Vec>& other, const glm::quat& rot = glm::quat{}):
			OriRect(other.getCenter(), other.getSize() * Precision(0.5), rot) {}

		constexpr OriRect& setCenter(const Vec& vec) noexcept {
			center = vec;
			return *this;
		}

		constexpr OriRect& setExtents(const Vec& vec) noexcept {
			halfExtents = vec * Precision(0.5);
			return *this;
		}

		constexpr OriRect& setRotation(const glm::quat& rot) noexcept {
			rotation = rot;
			return *this;
		}

		constexpr OriRect& translate(const Vec& vec) noexcept {
			center += vec;
			return *this;
		}

		/* Info Methods */

		[[nodiscard]]
		constexpr Vec getCenter() const noexcept {
			return center;
		}

		[[nodiscard]]
		constexpr Vec getHalfExtents() const noexcept {
			return halfExtents;
		}

		[[nodiscard]]
		constexpr const glm::quat getRotation() const noexcept {
			return rotation;
		}

		[[nodiscard]] // does not account for orientation
		constexpr Vec getSize() const noexcept {
			return halfExtents * Precision(2.0);
		}

		[[nodiscard]]
		constexpr Precision getArea() const noexcept {
			return glm::compMul(getSize());
		}

		[[nodiscard]] // this one aint signed lol!
		constexpr Precision distance(const Vec& point) const noexcept {
			auto local = glm::inverse(getRotation()) * (point - getCenter());

			Vec clamped = glm::clamp(local, -getHalfExtents(), getHalfExtents());
			return glm::distance(local, clamped);
		}

		[[nodiscard]]
		constexpr Vec supportPoint(const Vec& direction) const noexcept {
			auto axes = glm::mat3_cast(getRotation());

			Vec out = getCenter();
			for (uint16_t axis = 0; axis < dimensionality(); ++axis) {
				out += axes[axis] * (glm::dot(direction, axes[axis]) <= Precision(0.0) ? -getHalfExtents()[axis] : getHalfExtents()[axis]);
			}
			return out;
		}

		[[nodiscard]]
		constexpr bool contains(const Vec& point) const noexcept {
			return distance(point) <= Precision(0.0);
		}


		struct RaycastResult {
			const OriRect<VectorT>* rect = nullptr;
			Precision near = std::numeric_limits<Precision>::min();
			Precision far = std::numeric_limits<Precision>::max();
			VectorT normalNear{}, normalFar{};

			[[nodiscard]]
			constexpr bool hit() const noexcept {
				return rect != nullptr;
			}

			[[nodiscard]]
			constexpr Precision distance() const noexcept {
				return near;
			}

			[[nodiscard]]
			constexpr VectorT normal() const noexcept {
				return normalNear;
			}

			void setDistance(Precision dist) noexcept {
				near = dist;
			}

			void setNormal(VectorT norm) noexcept {
				normalNear = norm;
			}
		};
		using result_type = RaycastResult;

		[[nodiscard]] // cull is ignored
		constexpr RaycastResult raycast(const Vec& from, const Vec& dir, bool = false) const noexcept {
			RaycastResult out{};

			auto tbasis = glm::transpose(glm::mat3_cast(getRotation()));
			auto p = from - getCenter();

			Rect<Vec> rect{-getHalfExtents(), getHalfExtents()};
			auto res = rect.raycast(tbasis * p, tbasis * dir);
			if (res.hit()) {
				out.rect = this;
				out.near = res.near;
				out.far = res.far;
				out.normalNear = res.normalNear;
				out.normalFar = res.normalFar;
			}
			return out;
		}
	};

	template <size_t n, typename InternalT = float, glm::qualifier Qualifier = glm::qualifier::defaultp>
	using OriRectND = OriRect<glm::vec<n, InternalT, Qualifier>>;

	using OriRect1D = OriRectND<1>;
	using OriRect2D = OriRectND<2>;
	using OriRect3D = OriRectND<3>;

	static_assert(VolumeN<OriRect1D, 1>);
	static_assert(VolumeN<OriRect2D, 2>);
	static_assert(VolumeN<OriRect3D, 3>);

	[[nodiscard]] // no shear allowed
	inline OriRect3D transform(const OriRect3D& in, const glm::mat4& tform) {
		glm::vec3 center{ tform * glm::vec4(in.getCenter(), 1.0f) };

		glm::mat3 rot = glm::mat3_cast(in.getRotation());
		glm::mat3 tform3 = glm::mat3(tform);

		glm::vec3 xAxis = tform3 * rot[0];
		glm::vec3 yAxis = tform3 * rot[1];
		glm::vec3 zAxis = tform3 * rot[2];

		glm::vec3 lens{ glm::length(xAxis), glm::length(yAxis), glm::length(zAxis) };
		
		glm::mat3 basis{};
		basis[0] = xAxis / lens[0];
		basis[1] = yAxis / lens[1];
		basis[2] = zAxis / lens[2];

		return OriRect3D{center, in.getHalfExtents() * lens, glm::quat_cast(basis) };
	}
}