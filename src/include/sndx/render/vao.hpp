#pragma once

#include "vbo.hpp"

namespace sndx {
	class VAO {
	protected:
		GLuint curIdx = 0;

	public:
		GLuint id = 0;

		void bind() const {
			glBindVertexArray(id);
		}

		template <class Layout>
		void bindVBO(const VBO<Layout>& vbo, GLuint divisor = 0, GLboolean normalized = GL_FALSE) {
			if (id == 0) return;

			bind();
			vbo.bind();
			curIdx += Layout::vertexAttribPointer(curIdx, divisor, normalized);
		}

		void gen() {
			if (id != 0) {
				destroy();
			}
			glGenVertexArrays(1, &id);
		}

		void reset() {
			if (id != 0) {
				bind();
				for (size_t i = 0; i < curIdx; ++i) {
					glDisableVertexAttribArray(GLuint(i));
				}
				curIdx = 0;
			}
		}

		void destroy() {
			glDeleteVertexArrays(1, &id);
			id = 0;
		}
	};
}