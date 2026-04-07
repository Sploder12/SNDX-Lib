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
			auto ab = m_p2 - m_p1;
			auto ac = m_p3 - m_p1;
			auto ap = point - m_p1;

			auto d1 = glm::dot(ab, ap);
			auto d2 = glm::dot(ac, ap);

			// Vertex region A
			if (d1 <= Precision(0.0) && d2 <= Precision(0.0))
				return m_p1;

			auto bp = point - m_p2;
			auto d3 = glm::dot(ab, bp);
			auto d4 = glm::dot(ac, bp);

			// Vertex region B
			if (d3 >= Precision(0.0) && d4 <= d3)
				return m_p2;

			// Edge region AB
			auto vc = d1 * d4 - d3 * d2;
			if (vc <= Precision(0.0) && d1 >= Precision(0.0) && d3 <= Precision(0.0)) {
				float v = d1 / (d1 - d3);
				return m_p1 + ab * v;
			}

			auto cp = point - m_p2;
			auto d5 = glm::dot(ab, cp);
			auto d6 = glm::dot(ac, cp);

			// Vertex region C
			if (d6 >= Precision(0.0) && d5 <= d6)
				return m_p3;

			// Edge region AC
			auto vb = d5 * d2 - d1 * d6;
			if (vb <= Precision(0.0) && d2 >= Precision(0.0) && d6 <= Precision(0.0)) {
				auto w = d2 / (d2 - d6);
				return m_p1 + ac * w;
			}

			// Edge region BC
			auto va = d3 * d6 - d5 * d4;
			if (va <= Precision(0.0) && (d4 - d3) >= Precision(0.0) && (d5 - d6) >= Precision(0.0)) {
				auto bc = m_p3 - m_p2;
				auto w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
				return m_p2 + bc * w;
			}

			// Inside face region
			auto denom = Precision(1.0) / (va + vb + vc);
			auto v = vb * denom;
			auto w = vc * denom;

			return m_p1 + ab * v + ac * w;
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