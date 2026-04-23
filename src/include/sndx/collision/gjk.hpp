#pragma once

#include "./triangle.hpp"

#include <array>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

// for distance2
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace sndx::collision {
	// this is pretty much what is written in https://winter.dev/articles/gjk-algorithm and their epa article
	namespace detail {
		struct MinkowskiDiff {
			glm::vec3 a, b, out;
		};

		template <class SFnA, class SFnB> [[nodiscard]]
		MinkowskiDiff gjkMinkowski(SFnA&& supportA, SFnB&& supportB, glm::vec3 direction) {
			auto a = std::forward<SFnA>(supportA)(direction);
			auto b = std::forward<SFnB>(supportB)(-direction);
			return MinkowskiDiff{ a, b, a - b };
		}

		[[nodiscard]]
		inline glm::vec3 tripleProduct(glm::vec3 a, glm::vec3 b) {
			return glm::cross(glm::cross(a, b), a);
		}

		template <class T> [[nodiscard]]
		bool similarDir(T a, T b) {
			return glm::dot(a, b) > 0.0;
		}
	}

	struct SimplexGJK {
		using Vec = glm::vec3;
		
		[[nodiscard]]
		static constexpr size_t dimensionality() noexcept {
			return Vec::length();
		}

		std::array<detail::MinkowskiDiff, 3 + 1> points{};
		uint8_t size = 0;

		void push_front(detail::MinkowskiDiff point) {
			for (size_t i = points.size() - 1; i > 0; --i) {
				points[i] = points[i - 1];
			}
			points[0] = point;
			size = std::min(uint8_t(size + 1u), uint8_t(points.size()));
		}

		auto pointClosest() const {
			return points[0].out;
		}

		auto lineClosest() const {
			auto ab = points[1].out - points[0].out;
			auto abSqr = glm::dot(ab, ab);
			if (abSqr < 0.0000001f) { // degenerate case
				return points[0].out;
			}

			auto t = glm::dot(-points[0].out, ab) / abSqr;
			return points[0].out + glm::clamp(t, 0.0f, 1.0f) * points[1].out;
		}

		auto triangleClosest() const {
			// degenerate case
			if (sndx::math::areColinear(points[0].out, points[1].out, points[2].out)) {
				SimplexGJK lineBC{};
				lineBC.size = 2;
				lineBC.points[0] = points[1];
				lineBC.points[1] = points[2];

				SimplexGJK lineAC{};
				lineBC.size = 2;
				lineBC.points[0] = points[0];
				lineBC.points[1] = points[2];

				auto ab = lineClosest();
				auto bc = lineBC.lineClosest();
				auto ac = lineAC.lineClosest();

				auto abl = glm::length2(ab);
				auto bcl = glm::length2(bc);
				auto acl = glm::length2(ac);

				if (abl < bcl) {
					return abl < acl ? ac : ab;
				}
				return bcl < acl ? ac : bc;
			}

			sndx::collision::Tri<Vec> tri{ points[0].out, points[1].out, points[2].out };
			return tri.closestPoint(glm::vec3{ 0.0f });
		}

		[[nodiscard]]
		Vec gjkClosest() const {
			switch (size) {
			case 1: return pointClosest();
			case 2: return lineClosest();
			case 3: return triangleClosest();
			default:
				throw std::logic_error("GJK had weird number of points in simplex");
			}
		}

		[[nodiscard]]
		glm::vec3 getUVW() const {
			switch (size) {
			case 1:
				return glm::vec3(1.0f, 0.0f, 0.0f);
			case 2: {
				auto ab = points[1].out - points[0].out;
				auto abSqr = glm::dot(ab, ab);
				if (abSqr < 0.0000001f) { // degenerate case
					return glm::vec3(1.0f, 0.0f, 0.0f);
				}

				auto t = glm::dot(-points[0].out, ab) / abSqr;
				return glm::vec3(1.0f - t, t, 0.0f);
			}
			case 3: {
				sndx::collision::Tri<Vec> tri{ points[0].out, points[1].out, points[2].out };
				assert(!tri.isDegenerate());
				return tri.uvw(glm::vec3{ 0.0f });
			}
			default:
				throw std::logic_error("can't get UV for strange simplex");
			}
		}

		// returns true on improvement
		bool updateForDist(const detail::MinkowskiDiff& p) {
			switch (size) {
			case 0: // empty -> point
				size = 1;
				points[0] = p;
				return true;
			case 1: // point -> line
				if (glm::distance2(points[0].out, p.out) <= 0.000001f) {
					// degenerate case
					if (glm::length2(p.out) < glm::length2(points[0].out)) {
						points[0] = p;
						return true;
					}
					return false;
				}
				size = 2;
				points[1] = p;
				return true;
			case 2: { // line -> triangle
				if (sndx::math::areColinear(points[0].out, points[1].out, p.out)) {
					// degenerate case, pick best line segment
					SimplexGJK lineBC{};
					lineBC.size = 2;
					lineBC.points[0] = points[1];
					lineBC.points[1] = p;

					SimplexGJK lineAC{};
					lineAC.size = 2;
					lineAC.points[0] = points[0];
					lineAC.points[1] = p;

					auto ab = lineClosest();
					auto bc = lineBC.lineClosest();
					auto ac = lineAC.lineClosest();

					auto abl = glm::length2(ab);
					auto bcl = glm::length2(bc);
					auto acl = glm::length2(ac);

					if (abl < bcl) {
						if (acl < abl) {
							*this = lineAC;
							return true;
						}
					}
					else if (bcl < acl) {
						*this = lineBC;
						return true;
					}

					return false;
				}

				size = 3;
				points[2] = p;
				return true;
			}
			case 3: { // triangle -> better triangle
				auto newAB = *this;
				newAB.points[2] = p;

				auto newAC = *this;
				newAC.points[1] = p;

				auto newBC = *this;
				newBC.points[0] = p;

				auto cur = gjkClosest();
				auto pAB = newAB.gjkClosest();
				auto pAC = newAC.gjkClosest();
				auto pBC = newBC.gjkClosest();

				auto curLen = glm::length2(cur);
				auto abLen = glm::length2(pAB);
				auto acLen = glm::length2(pAC);
				auto bcLen = glm::length2(pBC);

				if (curLen <= abLen && curLen <= acLen && curLen <= bcLen) {
					return false;
				}

				if (abLen < acLen) {
					if (abLen < bcLen) {
						*this = newAB;
					}
					else {
						*this = newBC;
					}
				}
				else {
					if (acLen < bcLen) {
						*this = newAC;
					}
					else {
						*this = newBC;
					}
				}

				// prevent degenerate triangles
				size = 2;
				updateForDist(points[2]);
				return true;
			}
			default:
				throw std::logic_error(":(");
			}
		}

		bool lineOrigin(Vec& newDirection) {
			auto ab = points[1].out - points[0].out;
			auto ao = -points[0].out;

			if (detail::similarDir(ab, ao)) { // we want to point towards the origin
				auto tp = detail::tripleProduct(ab, ao);
				auto len = glm::length(tp);

				// beware being near colinear with the origin
				if (len < 0.00001f) {
					auto mag = glm::abs(ab);
					if (mag.x < mag.y && mag.x < mag.z) {
						newDirection = glm::vec3(1.0f, 0.0f, 0.0f);
					}
					else if (mag.y < mag.z) {
						newDirection = glm::vec3(0.0f, 1.0f, 0.0f);
					}
					else {
						newDirection = glm::vec3(0.0f, 0.0f, 1.0f);
					}
					newDirection = glm::cross(ab, newDirection);
					return false;
				}

				newDirection = tp / len;
				return true;
			}

			// remove b from simplex
			size = 1;
			newDirection = ao;
			return false;
		}

		bool triangleOrigin(Vec& newDirection) {
			auto ab = points[1].out - points[0].out;
			auto ac = points[2].out - points[0].out;
			auto ao = -points[0].out;

			auto cross = glm::cross(ab, ac);

			if (detail::similarDir(glm::cross(cross, ac), ao)) {
				// reduce back to line
				if (!detail::similarDir(ac, ao)) {
					// keep a and b
					size = 2;
					lineOrigin(newDirection);
					return false;
				}
				
				// keep a and c
				points[1] = points[2];
				size = 2;
				newDirection = detail::tripleProduct(ac, ao);
				return false;
			}
			
			if (detail::similarDir(glm::cross(ab, cross), ao)) {
				// keep a and b
				size = 2;
				lineOrigin(newDirection);
				return false;
			}

			// check direction for tetrahedon
			if (detail::similarDir(cross, ao)) {
				newDirection = cross;
			}
			else {
				// flip b and c
				std::swap(points[1], points[2]);
				newDirection = -cross;
			}

			return true;
		}

		bool tetrahedronOrigin(Vec& newDirection) {
			auto ab = points[1].out - points[0].out;
			auto ac = points[2].out - points[0].out;
			auto ad = points[3].out - points[0].out;
			auto ao = -points[0].out;

			if (detail::similarDir(glm::cross(ab, ac), ao)) {
				// remove d
				size = 3;
				triangleOrigin(newDirection);
				return false;
			}

			if (detail::similarDir(glm::cross(ac, ad), ao)) {
				// remove b
				points[1] = points[2];
				points[2] = points[3];
				size = 3;
				triangleOrigin(newDirection);
				return false;
			}

			if (detail::similarDir(glm::cross(ad, ab), ao)) {
				// remove c and flip b, d
				points[2] = points[1];
				points[1] = points[3];
				size = 3;
				triangleOrigin(newDirection);
				return false;
			}

			return true;
		}

		[[nodiscard]]
		bool gjkOrigin(Vec& newDirection) {
			// checks need to be done after since size can be modified
			switch (size) {
			case 2: return lineOrigin(newDirection) && size == dimensionality() + 1;
			case 3: return triangleOrigin(newDirection) && size == dimensionality() + 1;
			case 4: return tetrahedronOrigin(newDirection) && size == dimensionality() + 1;
			default:
				throw std::logic_error("GJK had weird number of points in simplex");
			}
		}
	};

	template <class SFnA, class SFnB> [[nodiscard]]
	std::optional<SimplexGJK> gjk(const SFnA& supportA, const SFnB& supportB) {
		auto support = detail::gjkMinkowski(supportA, supportB, glm::vec3(1.0, 0.0, 0.0));

		SimplexGJK simplex{};
		simplex.push_front(support);

		size_t iterations = 0;

		auto dir = glm::normalize(-support.out);
		while (true) {
			support = detail::gjkMinkowski(supportA, supportB, dir);
			if (!detail::similarDir(support.out, dir)) {
				return std::nullopt;
			}

			simplex.push_front(support);
			if (simplex.gjkOrigin(dir)) {
				return simplex;
			}

			++iterations;
		}
		return std::nullopt;
	}

	struct ResDistGJK {
		bool hit = false; // hit IS NOT TO BE TRUSTED
		glm::vec3 a{}, b{};

		[[nodiscard]]
		auto distance() const noexcept {
			return glm::distance(a, b);
		}
	};

	// if the two shapes ARE colliding this has a high chance of failure.
	// use gjk to ensure no collision before using this.
	template <class SFnA, class SFnB> [[nodiscard]]
	ResDistGJK gjkDist(const SFnA& supportA, const SFnB& supportB) {
		auto support = detail::gjkMinkowski(supportA, supportB, glm::vec3(1.0, 0.0, 0.0));

		SimplexGJK simplex{};
		simplex.updateForDist(support);

		size_t iterations = 0;
		while (iterations < 1024) {
			auto dir = -simplex.gjkClosest();
			auto dirMag = glm::length2(dir);
			if (dirMag <= 0.0f) { // we hit the origin!
				return ResDistGJK{ true };
			}

			support = detail::gjkMinkowski(supportA, supportB, dir);

			auto alignment = glm::dot(support.out, dir);
			auto old = glm::dot(simplex.points[0].out, dir);

			// check making progress
			if (alignment - old < 0.000001f) {
				break;
			}

			if (!simplex.updateForDist(support)) {
				break;
			}

			++iterations;
		}

		ResDistGJK out{ false };

		sndx::collision::Tri3D triA{ simplex.points[0].a, simplex.points[1].a, simplex.points[2].a };
		sndx::collision::Tri3D triB{ simplex.points[0].b, simplex.points[1].b, simplex.points[2].b };

		auto uvw = simplex.getUVW();
		out.a = triA.fromUVW(uvw);
		out.b = triB.fromUVW(uvw);

		return out;
	}

	struct EpaResult {
		glm::vec3 a, b;
		glm::vec3 normal;
		float depth;
	};

	[[nodiscard]]
	inline std::pair<std::vector<glm::vec4>, size_t> GetFaceNormals(const std::vector<detail::MinkowskiDiff>& polytope, const std::vector<size_t>& faces) {
		std::vector<glm::vec4> normals{};
		size_t minTriangle = 0;
		float  minDistance = FLT_MAX;

		for (size_t i = 0; i < faces.size(); i += 3) {
			glm::vec3 a = polytope[faces[i]].out;
			glm::vec3 b = polytope[faces[i + 1]].out;
			glm::vec3 c = polytope[faces[i + 2]].out;

			glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
			float distance = dot(normal, a);

			if (distance < 0) {
				normal *= -1;
				distance *= -1;
			}

			normals.emplace_back(normal, distance);

			if (distance < minDistance) {
				minTriangle = i / 3;
				minDistance = distance;
			}
		}

		return { normals, minTriangle };
	}

	inline void AddIfUniqueEdge(std::vector<std::pair<size_t, size_t>>& edges, const std::vector<size_t>& faces, size_t a, size_t b) {
		auto reverse = std::find(
			edges.begin(),
			edges.end(),
			std::make_pair(faces[b], faces[a])
		);

		if (reverse != edges.end()) {
			edges.erase(reverse);
		}
		else {
			edges.emplace_back(faces[a], faces[b]);
		}
	}

	template <class SFnA, class SFnB> [[nodiscard]]
	EpaResult epa(const SimplexGJK& simplex, const SFnA& supportA, const SFnB& supportB) {
		std::vector<detail::MinkowskiDiff> polytope{ simplex.points[0], simplex.points[1], simplex.points[2], simplex.points[3] };
		std::vector<size_t> faces{
			0, 1, 2,
			0, 3, 1,
			0, 2, 3,
			1, 3, 2,
		};

		auto [normals, minFace] = GetFaceNormals(polytope, faces);

		glm::vec3 minNormal{};
		float minDistance = FLT_MAX;
		size_t i = 0;

		while (minDistance == FLT_MAX) {
			++i;
			minNormal = glm::vec3(normals[minFace]);
			minDistance = normals[minFace].w;

			if (i >= 1024) {
				break;
			}

			auto support = detail::gjkMinkowski(supportA, supportB, minNormal);
			float sDistance = glm::dot(minNormal, support.out);

			if (abs(sDistance - minDistance) > 0.0001f) {
				minDistance = FLT_MAX;

				std::vector<std::pair<size_t, size_t>> uniqueEdges;

				for (size_t i = 0; i < normals.size(); i++) {
					if (detail::similarDir(glm::vec3(normals[i]), support.out - polytope[faces[i * 3]].out)) {
						size_t f = i * 3;

						AddIfUniqueEdge(uniqueEdges, faces, f, f + 1);
						AddIfUniqueEdge(uniqueEdges, faces, f + 1, f + 2);
						AddIfUniqueEdge(uniqueEdges, faces, f + 2, f);

						faces[f + 2] = faces.back(); faces.pop_back();
						faces[f + 1] = faces.back(); faces.pop_back();
						faces[f] = faces.back(); faces.pop_back();

						normals[i] = normals.back(); // pop-erase
						normals.pop_back();

						i--;
					}
				}

				std::vector<size_t> newFaces;
				for (auto [edgeIndex1, edgeIndex2] : uniqueEdges) {
					newFaces.emplace_back(edgeIndex1);
					newFaces.emplace_back(edgeIndex2);
					newFaces.emplace_back(polytope.size());
				}

				polytope.emplace_back(support);

				auto [newNormals, newMinFace] = GetFaceNormals(polytope, newFaces);

				float oldMinDistance = FLT_MAX;
				for (size_t i = 0; i < normals.size(); i++) {
					if (normals[i].w < oldMinDistance) {
						oldMinDistance = normals[i].w;
						minFace = i;
					}
				}

				if (newNormals[newMinFace].w < oldMinDistance) {
					minFace = newMinFace + normals.size();
				}

				faces.insert(faces.end(), newFaces.begin(), newFaces.end());
				normals.insert(normals.end(), newNormals.begin(), newNormals.end());
			}
		}

		EpaResult result{};

		sndx::collision::Tri3D tri{ polytope[faces[minFace * 3]].out, polytope[faces[minFace * 3 + 1]].out, polytope[faces[minFace * 3 + 2]].out };
		sndx::collision::Tri3D triA{ polytope[faces[minFace * 3]].a, polytope[faces[minFace * 3 + 1]].a, polytope[faces[minFace * 3 + 2]].a };
		sndx::collision::Tri3D triB{ polytope[faces[minFace * 3]].b, polytope[faces[minFace * 3 + 1]].b, polytope[faces[minFace * 3 + 2]].b };
		auto uvw = tri.uvw(glm::vec3(0.0f));

		result.a = triA.fromUVW(uvw);
		result.b = triB.fromUVW(uvw);
		result.normal = minNormal;
		result.depth = minDistance + 0.00001f;

		return result;
	}

	template <class Fn> [[nodiscard]]
	auto transformSupportFn(Fn&& fnc, const glm::mat4& t, const glm::mat4& invT) {
		return [f = std::forward<Fn>(fnc), t, iT = glm::mat3{ invT }](glm::vec3 dir) {
			auto dirLocal = iT * dir;
			return glm::vec3(t * glm::vec4(f(dirLocal), 1.0f));
		};
	}

	template <VolumeN<3> T> [[nodiscard]]
	auto getSupportFn(const T& volume) {
		return [&](glm::vec3 dir) { return volume.supportPoint(dir); };
	}

	template <size_t extent> [[nodiscard]]
	auto getSupportFn(std::span<const glm::vec3, extent> points) {
		return [points](glm::vec3 dir) {
			auto best = std::numeric_limits<float>::min();
			glm::vec3 out{};
			for (const auto& point : points) {
				auto d = glm::dot(point, dir);
				if (d > best) {
					best = d;
					out = point;
				}
			}
			return out;
		};
	}

	template <class T> [[nodiscard]]
	auto getSupportFn(T&& shape, const glm::mat4& t, const glm::mat4& invT) {
		return transformSupportFn(getSupportFn(std::forward<T>(shape)), t, invT);
	}
}