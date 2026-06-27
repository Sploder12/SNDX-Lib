#pragma once

#include "../layout.hpp"

#include "buffer.hpp"

namespace sndx::render::gl {

	[[nodiscard]]
	constexpr GLenum toGl(sndx::render::Type type) {
		switch (type) {
		case sndx::render::Type::bytes:
			return GL_BYTE;
		case sndx::render::Type::ubytes:
			return GL_UNSIGNED_BYTE;
		case sndx::render::Type::shorts:
			return GL_SHORT;
		case sndx::render::Type::ushorts:
			return GL_UNSIGNED_SHORT;
		case sndx::render::Type::ints:
			return GL_INT;
		case sndx::render::Type::uints:
			return GL_UNSIGNED_INT;
		case sndx::render::Type::halfFloats:
			return GL_HALF_FLOAT;
		case sndx::render::Type::floats:
			return GL_FLOAT;
		case sndx::render::Type::doubles:
			return GL_DOUBLE;
		default:
			throw std::logic_error("Invalid vertex attrib typename");
		}
	}

	template <bool useDSA = SNDX_USE_DSA>
	struct VAO {
	private:
		GLuint curIdx = 0;
		GLuint curBufs = 0;

	public:
		GLuint id;

		explicit VAO() {
			if constexpr (useDSA) {
				glCreateVertexArrays(1, &id);
			}
			else {
				glGenVertexArrays(1, &id);
			}
		}

		VAO(VAO&& o) noexcept: id(
			std::exchange(o.id, 0)), curIdx(std::exchange(o.curIdx, 0)), curBufs(std::exchange(o.curBufs, 0)) {}
		VAO(const VAO&) = delete;
		VAO& operator=(VAO&& o) noexcept {
			std::swap(id, o.id);
			std::swap(curIdx, o.curIdx);
			std::swap(curBufs, o.curBufs);
			return *this;
		}
		VAO& operator=(const VAO&) = delete;
		~VAO() noexcept {
			if (id != 0) {
				glDeleteVertexArrays(1, &id);
				id = 0;
			}
		}

		void bind() const {
			glBindVertexArray(id);
		}

		void bindEBO(const Buffer<useDSA>& ebo) {
			assert(ebo.type == GL_ELEMENT_ARRAY_BUFFER);
			if constexpr (useDSA) {
				glVertexArrayElementBuffer(id, ebo.id);
			}
			else {
				bind();
				ebo.bind();
			}
		}

		template <class ArrT>
		void bindVBO(const Buffer<useDSA>& vbo, const ArrT& entries) requires useDSA {
			assert(vbo.type == GL_ARRAY_BUFFER);

			for (const auto& entry : entries) {
				GLenum type = toGl(entry.type);

				glVertexArrayVertexBuffer(id, curBufs, vbo.id, 0, entry.stride);
				glEnableVertexArrayAttrib(id, curIdx);
				glVertexArrayAttribFormat(id, curIdx, entry.count, type, entry.normalized, entry.offset);
				glVertexArrayAttribBinding(id, curIdx, curBufs);

				if (entry.instanced) {
					glVertexArrayBindingDivisor(id, curBufs, 1);
				}

				++curIdx;
			}
			++curBufs;
		}

		template <class ArrT>
		void bindVBO(const Buffer<useDSA>& vbo, const ArrT& entries) requires !useDSA{
			assert(vbo.type == GL_ARRAY_BUFFER);

			bind();
			vbo.bind();

			for (const auto& entry : entries) {
				GLenum type = toGl(entry.type);
				void* offset = (void*)static_cast<uintptr_t>(entry.offset);

				if (entry.normalized) {
					glVertexAttribPointer(curIdx, entry.count, type, entry.normalized, entry.stride, offset);
				}
				else {
					switch (entry.type) {
					case Type::halfFloats:
					case Type::floats:
						glVertexAttribPointer(curIdx, entry.count, type, entry.normalized, entry.stride, offset);
						break;
					case Type::doubles:
						glVertexAttribLPointer(curIdx, entry.count, type, entry.stride, offset);
						break;
					default:
						glVertexAttribIPointer(curIdx, entry.count, type, entry.stride, offset);
						break;
					}
				}

				if (entry.instanced) {
					glVertexAttribDivisor(curIdx, 1);
				}
				glEnableVertexAttribArray(curIdx);

				++curIdx;
			}
			++curBufs;
		}

		template <Layout layout, bool instanced = false>
		void bindVBO(const Buffer<useDSA>& vbo) {
			bindVBO(vbo, Layout::value<instanced>);
		}

		void resetLayout() {
			if constexpr (useDSA) {
				for (GLuint i = 0; i < curIdx; ++i) {
					glDisableVertexArrayAttrib(id, i);
				}
			}
			else {
				bind();
				for (GLuint i = 0; i < curIdx; ++i) {
					glDisableVertexAttribArray(i);
				}
			}
			curIdx = 0;
			curBufs = 0;
		}
	};
}