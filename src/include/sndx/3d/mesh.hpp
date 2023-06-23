#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "collision.hpp"

#include "../render/texture.hpp"
#include "../render/vao.hpp"
#include "../render/vbo.hpp"

namespace sndx {

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 texCoords;

		explicit operator glm::vec3() const {
			return pos;
		}
	};

	typedef std::vector<std::pair<size_t, float>> Bone;

	// this would be nicer if we had reflection
	template <class VertexT, class... VertexLayout>
	class Mesh {
		// this does not fully protect against incorrect layouts
		static_assert(sizeof(VertexT) == VboLayout<VertexLayout...>::stride());

	protected:
		VAO vao{};
		VBO<VertexLayout...> vbo{};
		VBO<unsigned int> ebo{ GL_ELEMENT_ARRAY_BUFFER };
		

	public:
		std::vector<VertexT> vertices{};
		std::vector<unsigned int> indices{};
		std::vector<Texture> textures{};

		std::unordered_map<std::string, Bone> bones{};

		AABB getBounds() const {
			if (vertices.empty()) return AABB{ glm::vec3(0.0f), glm::vec3(0.0f) };

			AABB out = {glm::vec3(vertices.front()), glm::vec3(vertices.front())};

			for (const auto& vert : vertices) {
				const glm::vec3 p = glm::vec3(vert);

				out.ldf.x = std::min(out.ldf.x, p.x);
				out.ldf.y = std::min(out.ldf.y, p.y);
				out.ldf.z = std::min(out.ldf.z, p.z);

				out.rub.x = std::max(out.rub.x, p.x);
				out.rub.y = std::max(out.rub.y, p.y);
				out.rub.z = std::max(out.rub.z, p.z);
			}

			return out;
		}

		auto& updateBuffers() {
			vbo.setData(vertices);
			ebo.setData(indices);
			return *this;
		}

		auto& gen() {
			vao.bind();
			ebo.bind();
			vao.bindVBO(vbo);
			updateBuffers();
			return *this;
		}

		template <class Layout>
		auto& bindAuxiliaryVBO(VBO<Layout>& vbo, GLuint divisor = 0, GLboolean normalized = GL_FALSE) {
			vao.bindVBO(vbo, divisor, normalized);
			return *this;
		}

		auto& destroy() {
			vao.destroy();
			vbo.destroy();
			ebo.destroy();
			vertices.clear();
			indices.clear();
			textures.clear();
			return *this;
		}

		auto& bind() {
			for (int i = 0; i < textures.size(); ++i) {
				textures[i].bind(i);
			}

			vao.bind();
			return *this;
		}
	};
}