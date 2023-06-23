#pragma once

#include "vbo.hpp"

namespace sndx {
	class VAO {
	protected:
		GLuint curIdx = 0;

	public:
		GLuint id = 0;

		void bind() {
			if (id == 0) gen();
			glBindVertexArray(id);
		}

		template <class... Layout>
		void bindVBO(VBO<Layout...>& vbo, GLuint divisor = 0) {
			if (id == 0) {
				gen();
				curIdx = 0;
			}

			bind();
			vbo.bind();
			curIdx += VboLayout<Layout...>::vertexAttribPointer(curIdx, divisor);
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