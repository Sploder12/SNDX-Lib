#pragma once

#include "./volume.hpp"
#include "./rect.hpp"

#include "../math/lines.hpp"

namespace sndx::collision {
	// A Triangle as described by three points.
	template <Vector VectorT = glm::vec3>
	class Tri {
	public:
		using Vec = VectorT;
		using Precision = Vec::value_type;

		static constexpr size_t dimensionality() noexcept {
			return Vec::length();
		}

	protected:
		Vec m_p1{}, m_p2{}, m_p3{};

	public:
		constexpr Tri(const Vec& p1, const Vec& p2, const Vec& p3) noexcept :
			m_p1(p1), m_p2(p2), m_p3(p3) {}

		constexpr Tri& translate(const Vec& vec) noexcept {
			m_p1 += vec;
			m_p2 += vec;
			m_p3 += vec;

			return *this;
		}

		constexpr Tri& flipNormal() noexcept {
			std::swap(m_p2, m_p3);
			return *this;
		}

		[[nodiscard]] // output is not normalized.
		constexpr Vec normal() const noexcept {
			return sndx::math::surfaceNormal(m_p1, m_p2, m_p3);
		}

		[[nodiscard]] // will return < 0.0 when outside triangle
		constexpr glm::vec3 uvw(Vec p) const noexcept {
			auto ab = m_p2 - m_p1;
			auto ac = m_p3 - m_p1;
			auto ap = p - m_p1;

			// project p
			auto n = glm::normalize(glm::cross(ab, ac));
			p = p - glm::dot(p - m_p1, n) * n;
			ap = p - m_p1;

			// solve for uvw
			auto abSqr = glm::dot(ab, ab);
			auto abc = glm::dot(ab, ac);
			auto acSqr = glm::dot(ac, ac);
			auto abp = glm::dot(ap, ab);
			auto acp = glm::dot(ap, ac);

			auto denom = abSqr * acSqr - abc * abc;

			if (std::abs(denom) < 0.00001f) [[unlikely]] {
				return glm::vec3{ 0.0f };
			}

			auto v = (acSqr * abp - abc * acp) / denom;
			auto w = (abSqr * acp - abc * abp) / denom;

			return glm::vec3{ 1.0f - v - w, v, w };
		}

		[[nodiscard]]
		constexpr Vec fromUVW(const glm::vec3& uvw) const noexcept {
			return uvw.x * m_p1 + uvw.y * m_p2 + uvw.z * m_p3;
		}

		[[nodiscard]]
		constexpr const Vec& getP1() const noexcept {
			return m_p1;
		}

		[[nodiscard]]
		constexpr const Vec& getP2() const noexcept {
			return m_p2;
		}

		[[nodiscard]]
		constexpr const Vec& getP3() const noexcept {
			return m_p3;
		}

		[[nodiscard]]
		constexpr bool isDegenerate() const noexcept {
			return sndx::math::areColinear(getP1(), getP2(), getP3());
		}

		[[nodiscard]]
		constexpr Rect<Vec> getBounds() const noexcept {
			auto minpt = glm::min(glm::min(getP1(), getP2()), getP3());
			auto maxpt = glm::max(glm::max(getP1(), getP2()), getP3());

			return Rect<Vec>{minpt, maxpt};
		}

		// get the dimensions of the Tri
		[[nodiscard]]
		constexpr Vec getSize() const noexcept {
			return getBounds().getSize();
		}

		// get the Volume/Area of the Tri
		[[nodiscard]]
		constexpr Precision getArea() const noexcept {
			auto ab = getP2() - getP1();
			auto ac = getP3() - getP1();
			return Precision(0.5) * glm::length(glm::cross(ab, ac));
		}

		// get the center point of the Tri
		[[nodiscard]]
		constexpr Vec getCenter() const noexcept {
			return (getP1() + getP2() + getP3()) / Precision(3.0);
		}

		[[nodiscard]]
		constexpr Precision getCenter(uint8_t axis) const noexcept {
			return (getP1()[axis] + getP2()[axis] + getP3()[axis]) / Precision(3.0);
		}

		[[nodiscard]] // from Ericson's Real Time Collision Detection
		constexpr Vec closestPoint(const Vec& point) const noexcept {
			auto ab = getP2() - getP1();
			auto ac = getP3() - getP1();
			auto bc = getP3() - getP2();

			auto snom = glm::dot(point - getP1(), ab);
			auto sdenom = glm::dot(point - getP2(), -ab);

			auto tnom = glm::dot(point - getP1(), ac);
			auto tdenom = glm::dot(point - getP3(), -ac);

			if (snom <= Precision(0.0) && tnom <= Precision(0.0))
				return getP1();

			auto unom = glm::dot(point - getP2(), bc);
			auto udenom = glm::dot(point - getP3(), -bc);

			if (sdenom <= Precision(0.0) && unom <= Precision(0.0))
				return getP2();

			if (tdenom <= Precision(0.0) && udenom <= Precision(0.0))
				return getP3();

			auto n = glm::cross(ab, ac);
			auto vc = glm::dot(n, glm::cross(getP1() - point, getP2() - point));
			if (vc <= Precision(0.0) && snom >= Precision(0.0) && sdenom >= Precision(0.0))
				return getP1() + snom / (snom + sdenom) * ab;

			auto va = glm::dot(n, glm::cross(getP2() - point, getP3() - point));
			if (va <= Precision(0.0) && unom >= Precision(0.0) && udenom >= Precision(0.0))
				return getP2() + unom / (unom + udenom) * bc;

			auto vb = glm::dot(n, glm::cross(getP3() - point, getP1() - point));
			if (vb <= Precision(0.0) && tnom > Precision(0.0) && tdenom >= Precision(0.0))
				return getP1() + tnom / (tnom + tdenom) * ac;

			auto u = va / (va + vb + vc);
			auto v = vb / (va + vb + vc);
			auto w = 1.0f - u - v;
			return fromUVW(glm::vec3(u, v, w));
		}

		[[nodiscard]]
		constexpr Precision distance(const Vec& point) const noexcept {
			return glm::distance(point, closestPoint(point));
		}

		[[nodiscard]]
		constexpr bool contains(const Vec& point) const noexcept {
			return distance(point) <= Precision(0.0);
		}

		[[nodiscard]]
		constexpr Vec supportPoint(const Vec& dir) const noexcept {
			auto dotA = glm::dot(getP1(), dir);
			auto dotB = glm::dot(getP2(), dir);
			auto dotC = glm::dot(getP3(), dir);

			if (dotA >= dotB) {
				return dotA >= dotC ? getP1() : getP3();
			}
			return dotB >= dotC ? getP2() : getP3();
		}

		struct RaycastResult {
			const Tri* tri = nullptr;
			Vec norm{};
			glm::vec2 uv{};
			float dist = std::numeric_limits<float>::max();

			[[nodiscard]]
			constexpr bool hit() const noexcept {
				return tri != nullptr;
			}

			[[nodiscard]]
			constexpr float distance() const noexcept {
				return dist;
			}

			[[nodiscard]]
			constexpr Vec normal() const noexcept {
				return norm;
			}

			constexpr void setDistance(float d) noexcept {
				dist = d;
			}

			constexpr void setNormal(Vec normal) noexcept {
				this->norm = normal;
			}
		};
		using result_type = RaycastResult;


		[[nodiscard]]
		constexpr RaycastResult raycast(Vec from, Vec dir, bool cull = false) const noexcept {
			constexpr Precision epsilon = Precision(0.000001);

			auto edge1 = m_p2 - m_p1;
			auto edge2 = m_p3 - m_p1;

			auto hyp = glm::cross(dir, edge2);
			auto r = glm::dot(edge1, hyp);

			RaycastResult out{};

			// handle parallel and backface culling
			if (r < epsilon) {
				if (cull || r > -epsilon) {
					return out;
				}
			}

			auto inv = Precision(1.0) / r;
			from -= m_p1;

			out.uv.x = inv * glm::dot(from, hyp);
			if (out.uv.x < Precision(0.0) || out.uv.x > Precision(1.0))
				return out;

			auto q = glm::cross(from, edge1);
			out.uv.y = inv * glm::dot(dir, q);
			if (out.uv.y < Precision(0.0) || (out.uv.x + out.uv.y) > Precision(1.0))
				return out;

			out.norm = glm::cross(edge1, edge2);

			out.dist = inv * glm::dot(edge2, q);
			if (out.dist > epsilon) {
				out.tri = this;
			}
			return out;
		}
	};

	template <size_t n, typename InternalT = float, glm::qualifier Qualifier = glm::qualifier::defaultp>
	using TriND = Tri<glm::vec<n, InternalT, Qualifier>>;

	using Tri2D = TriND<2>;
	using Tri3D = TriND<3>;

	static_assert(VolumeN<Tri2D, 2>);
	static_assert(VolumeN<Tri3D, 3>);
}