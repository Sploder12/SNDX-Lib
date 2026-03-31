#pragma once

#include "./volume.hpp"

#include <array>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sndx::collision {
	// this is pretty much what is written in https://winter.dev/articles/gjk-algorithm and their epa article
	namespace detail {
		template <class SFnA, class SFnB> [[nodiscard]]
		glm::vec3 gjkMinkowski(SFnA&& supportA, SFnB&& supportB, glm::vec3 direction) {
			assert(std::abs(glm::length(direction) - 1.0f) <= 0.0001f && "direction must be normalized!");
			return std::forward<SFnA>(supportA)(direction) - std::forward<SFnB>(supportB)(-direction);
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

		std::array<Vec, 3 + 1> points{};
		uint8_t size = 0;

		void push_front(Vec point) {
			for (size_t i = points.size() - 1; i > 0; --i) {
				points[i] = points[i - 1];
			}
			points[0] = point;
			size = std::min(uint8_t(size + 1u), uint8_t(points.size()));
		}

		bool lineOrigin(Vec& newDirection) {
			auto ab = points[1] - points[0];
			auto ao = -points[0];

			if (detail::similarDir(ab, ao)) { // we want to point towards the origin
				newDirection = glm::normalize(detail::tripleProduct(ab, ao));
				return true;
			}

			// remove b from simplex
			size = 1;
			newDirection = glm::normalize(ao);
			return false;
		}

		bool triangleOrigin(Vec& newDirection) {
			auto ab = points[1] - points[0];
			auto ac = points[2] - points[0];
			auto ao = -points[0];

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
				newDirection = glm::normalize(detail::tripleProduct(ac, ao));
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
				newDirection = glm::normalize(cross);
			}
			else {
				// flip b and c
				std::swap(points[1], points[2]);
				newDirection = glm::normalize(-cross);
			}

			return true;
		}

		bool tetrahedronOrigin(Vec& newDirection) {
			auto ab = points[1] - points[0];
			auto ac = points[2] - points[0];
			auto ad = points[3] - points[0];
			auto ao = -points[0];

			if (detail::similarDir(glm::cross(ab, ac), ao)) {
				// remove d
				size = 3;
				return triangleOrigin(newDirection);
			}

			if (detail::similarDir(glm::cross(ac, ad), ao)) {
				// remove b
				points[1] = points[2];
				points[2] = points[3];
				size = 3;
				return triangleOrigin(newDirection);
			}

			if (detail::similarDir(glm::cross(ad, ab), ao)) {
				// remove c and flip b, d
				points[2] = points[1];
				points[1] = points[3];
				return triangleOrigin(newDirection);
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

		auto dir = glm::normalize(-support);
		while (true) {
			support = detail::gjkMinkowski(supportA, supportB, dir);
			if (!detail::similarDir(support, dir)) {
				return std::nullopt;
			}

			simplex.push_front(support);
			if (simplex.gjkOrigin(dir)) {
				return simplex;
			}
		}
	}

	struct EpaResult {
		glm::vec3 normal;
		float depth;
	};

	struct PolytopeEPA {
		using Vertices = std::vector<glm::vec3>;
		using Indices = std::vector<glm::vec<3, size_t>>;
		using Edges = std::vector<std::pair<size_t, size_t>>;

		Vertices verts{};
		Indices indices{};

		static void addUniqueEdge(Edges& out, const Indices& indices, size_t tri, size_t a, size_t b) {
			auto reversed = std::find(out.begin(), out.end(), std::make_pair(indices[tri][b], indices[tri][a]));

			if (reversed != out.end()) {
				out.erase(reversed);
			}
			else {
				out.emplace_back(indices[tri][a], indices[tri][b]);
			}
		}

		[[nodiscard]]
		static std::pair<std::vector<glm::vec4>, size_t> calcNormals(const Vertices& verts, const Indices& indices) {
			size_t mTri = 0;
			std::vector<glm::vec4> norms{};
			norms.reserve(indices.size());
			float minDist = std::numeric_limits<float>::max();
			for (size_t i = 0; i < indices.size(); ++i) {
				const auto& a = verts[indices[i].x];
				const auto& b = verts[indices[i].y];
				const auto& c = verts[indices[i].z];

				auto normal = glm::normalize(glm::cross(b - a, c - a));
				auto dist = glm::dot(normal, a);
				if (dist < 0) {
					normal *= -1.0f;
					dist *= -1.0f;
				}

				norms.emplace_back(normal, dist);
				if (dist < minDist) {
					mTri = i;
					minDist = dist;
				}
			}

			return { std::move(norms), mTri };
		}

		[[nodiscard]]
		auto calcNormals() const {
			return calcNormals(verts, indices);
		}

		PolytopeEPA(const SimplexGJK& simplex):
			verts(simplex.points.begin(), simplex.points.begin() + simplex.size),
			indices{ {0, 1, 2}, {0, 3, 1}, {0, 2, 3}, {1, 3, 2} } {

			assert(simplex.size == 4);
		}
	};

	template <float epsilon = 0.00001f, class SFnA, class SFnB> [[nodiscard]]
	EpaResult epa(const SimplexGJK& simplex, const SFnA& supportA, const SFnB& supportB) {
		PolytopeEPA polytope{ simplex };

		auto [normals, minTri] = polytope.calcNormals();

		glm::vec3 minNorm{};
		float minDist = std::numeric_limits<float>::max();

		while (minDist == std::numeric_limits<float>::max()) {
			minNorm = glm::vec3(normals[minTri]);
			minDist = normals[minTri].w;

			auto support = detail::gjkMinkowski(supportA, supportB, minNorm);
			auto dist = glm::dot(minNorm, support);

			if (std::abs(dist - minDist) > epsilon) {
				minDist = std::numeric_limits<float>::max();

				PolytopeEPA::Edges edges{};
				edges.reserve(polytope.verts.size());

				for (size_t i = 0; i < normals.size(); ++i) {
					if (detail::similarDir(glm::vec3(normals[i]), support - polytope.verts[polytope.indices[i].x])) {

						PolytopeEPA::addUniqueEdge(edges, polytope.indices, i, 0, 1);
						PolytopeEPA::addUniqueEdge(edges, polytope.indices, i, 1, 2);
						PolytopeEPA::addUniqueEdge(edges, polytope.indices, i, 2, 0);

						polytope.indices[i] = polytope.indices.back();
						polytope.indices.pop_back();

						normals[i] = normals.back();
						normals.pop_back();

						--i;
					}
				}

				PolytopeEPA::Indices newIndices{};
				newIndices.reserve(edges.size() / 3);
				for (auto [a, b] : edges) {
					newIndices.emplace_back(a, b, polytope.verts.size());
				}
				polytope.verts.emplace_back(support);

				auto [newNorms, newMinTri] = PolytopeEPA::calcNormals(polytope.verts, newIndices);

				float oldMin = std::numeric_limits<float>::max();
				for (size_t i = 0; i < normals.size(); ++i) {
					if (normals[i].w < oldMin) {
						oldMin = normals[i].w;
						minTri = i;
					}
				}

				if (newNorms[newMinTri].w < oldMin) {
					minTri = newMinTri + normals.size();
				}

				polytope.indices.insert(polytope.indices.begin(), newIndices.begin(), newIndices.end());
				normals.insert(normals.begin(), newNorms.begin(), newNorms.end());
			}
		}

		EpaResult out{};
		out.normal = minNorm;
		out.depth = minDist + epsilon;
		return out;
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
}