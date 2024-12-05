#pragma once

#include "vbo.hpp"

namespace sndx::render {

	class VAO {
	private:
		GLuint curIdx = 0;
		GLuint m_id = 0;

		void destroy() noexcept {
			glDeleteVertexArrays(&m_id);
			curIdx = 0;
			m_id = 0;
		}

	public:
		VAO() {
			gen();
		}

		explicit constexpr VAO(GLuint id) noexcept :
			m_id(id) {}

		VAO(VAO&& other) noexcept:
			curIdx(std::exchange(other.curIdx, 0)), m_id(std::exchange(other.m_id, 0)) {}

		VAO& operator=(VAO&& other) noexcept {
			std::swap(curIdx, other.curIdx);
			std::swap(m_id, other.m_id);
			return *this;
		}

		constexpr operator GLuint() const noexcept {
			return m_id;
		}

		~VAO() noexcept {
			destroy();
		}

		void bind() {
			if (m_id == 0) gen();
			glBindVertexArray(m_id);
		}

		template <class... Layout>
		void bindLayout(TypedVBO<Layout...>& vbo, GLuint divisor = 0) {
			bind();
			vbo.bind();
			curIdx += VboLayout<Layout...>::vertexAttribPointer(curIdx, divisor);
		}

		void resetLayout() {
			if (m_id != 0) {
				bind();
				for (size_t i = 0; i < curIdx; ++i) {
					glDisableVertexAttribArray(i);
				}
				curIdx = 0;
			}
		}

		void gen() {
			if (m_id != 0) {
				destroy();
			}
			glGenVertexArrays(1, &m_idx);
		}
	};
}