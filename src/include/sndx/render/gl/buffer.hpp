#pragma once

#include <span>

#include <GL/glew.h>

#ifndef SNDX_USE_DSA
#ifndef SNDX_NO_DSA 
#define SNDX_USE_DSA true
#else
#define SNDX_USE_DSA false
#endif
#endif

namespace sndx::render::gl {
	template <bool useDSA = SNDX_USE_DSA>
	struct Buffer {
		GLuint id;
		GLenum type;

		explicit Buffer(GLenum type = GL_ARRAY_BUFFER):
			id(0), type(type) {
			if constexpr (useDSA) {
				glCreateBuffers(1, &id);
			}
			else {
				glGenBuffers(1, &id);
			}
		}

		Buffer(Buffer&& o) noexcept: id(std::exchange(o.id, 0)), type(o.type) {}
		Buffer(const Buffer&) = delete;
		Buffer& operator=(Buffer&& o) noexcept {
			std::swap(id, o.id);
			std::swap(type, o.type);
			return *this;
		}
		Buffer& operator=(const Buffer&) = delete;
		~Buffer() noexcept {
			if (id != 0) {
				glDeleteBuffers(1, &id);
				id = 0;
			}
		}

		void bind() const {
			glBindBuffer(type, id);
		}

		void setBinding(GLintptr offset, GLsizeiptr size, GLuint index) {
			assert(
				type == GL_ATOMIC_COUNTER_BUFFER ||
				type == GL_TRANSFORM_FEEDBACK_BUFFER ||
				type == GL_UNIFORM_BUFFER ||
				type == GL_SHADER_STORAGE_BUFFER
			);
			glBindBufferRange(type, index, id, offset, size);
		}

		void setBinding(GLuint index) {
			assert(
				type == GL_ATOMIC_COUNTER_BUFFER ||
				type == GL_TRANSFORM_FEEDBACK_BUFFER ||
				type == GL_UNIFORM_BUFFER ||
				type == GL_SHADER_STORAGE_BUFFER
			);
			glBindBufferBase(type, index, id);
		}

		void resize(GLsizeiptr size) {
			if constexpr (useDSA) {
				glNamedBufferData(id, size, nullptr, GL_DYNAMIC_DRAW);
			}
			else {
				bind();
				glBufferData(type, size, nullptr, GL_DYNAMIC_DRAW);
			}
		}

		void data(std::span<const uint8_t> data) {
			resize(data.size());
			if constexpr (useDSA) {
				subdata(0, data);
			} 
			else {
				// we don't call subdata because we don't want to rebind
				glBufferSubData(type, 0, data.size(), data.data());
			}
		}

		void subdata(GLintptr offset, std::span<const uint8_t> data) {
			if constexpr (useDSA) {
				glNamedBufferSubData(id, offset, data.size(), data.data());
			}
			else {
				bind();
				glBufferSubData(type, offset, data.size(), data.data());
			}
		}

		// BEWARE LIFETIMES AND AVOIDING .data CALLS WHEN USING MAP
		[[nodiscard]]
		void* map(GLenum access) {
			if constexpr (useDSA) {
				return glMapNamedBuffer(id, access);
			}
			else {
				bind();
				return glMapBuffer(type, access);
			}
		}

		[[nodiscard]]
		std::span<void*> map(GLintptr offset, GLsizeiptr length, GLenum access) {
			if constexpr (useDSA) {
				return std::span{ glMapNamedBufferRange(id, offset, length, access), length };
			}
			else {
				bind();
				return std::span{ glMapBufferRange(type, offset, length, access), length };
			}
		}

		// only useful if GL_MAP_FLUSH_EXPLICIT_BIT was set when mapping
		void flushMap(GLintptr offset, GLsizeiptr length) {
			if constexpr (useDSA) {
				glFlushMappedNamedBufferRange(id, offset, length);
			}
			else {
				bind();
				glFlushMappedBufferRange(type, offset, length);
			}
		}

		// it is not required to call unmap, but a mapped buffer cannot be used after dtor.
		GLboolean unmap() {
			if constexpr (useDSA) {
				return glUnmapNamedBuffer(id);
			}
			else {
				bind();
				return glUnmapBuffer(type);
			}
		}
	};
}