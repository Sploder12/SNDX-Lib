#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "../render/texture.hpp"
#include "../render/vao.hpp"
#include "../render/vbo.hpp"

namespace sndx {

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 texCoords;
	};

	// this would be nicer if we had reflection
	template <class VertexT, class... VertexLayout>
	class Mesh {
		// this does not fully protect against incorrect layouts
		static_assert(sizeof(VertexT) == VboLayout<VertexLayout...>::stride());

	protected:
		VAO vao{};
		VBO<VboLayout<VertexLayout...>> vbo{};
		VBO<VboLayout<unsigned int>> ebo = VBO<VboLayout<unsigned int>>(GL_ELEMENT_ARRAY_BUFFER);
		

	public:
		std::vector<VertexT> vertices{};
		std::vector<unsigned int> indices{};
		std::vector<Texture> textures{};

		void updateBuffers() {
			vbo.setData(vertices);
			ebo.setData(indices);
		}

		void gen() {
			vao.bind();
			ebo.bind();
			vao.bindVBO(vbo);
			updateBuffers();
		}

		template <class Layout>
		void bindAuxiliaryVBO(VBO<Layout>& vbo, GLuint divisor = 0, GLboolean normalized = GL_FALSE) {
			vao.bindVBO(vbo, divisor, normalized);
		}

		void destroy() {
			vao.destroy();
			vbo.destroy();
			ebo.destroy();
			vertices.clear();
			indices.clear();
			textures.clear();
		}

		void bind() {
			for (int i = 0; i < textures.size(); ++i) {
				textures[i].bind(i);
			}

			vao.bind();
		}
	};
}