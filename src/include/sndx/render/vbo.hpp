#pragma once

#include <gl/glew.h>

#include <glm/glm.hpp>

namespace sndx {

	template <class T>
	constexpr GLenum typeToGLenum();

	template <>
	constexpr GLenum typeToGLenum<char>() { return GL_BYTE; }

	template <>
	constexpr GLenum typeToGLenum<unsigned char>() { return GL_UNSIGNED_BYTE; }

	template <>
	constexpr GLenum typeToGLenum<short>() { return GL_SHORT; }

	template <>
	constexpr GLenum typeToGLenum<unsigned short>() { return GL_UNSIGNED_SHORT; }

	template <>
	constexpr GLenum typeToGLenum<int>() { return GL_INT; }

	template <>
	constexpr GLenum typeToGLenum<unsigned int>() { return GL_UNSIGNED_INT; }

	template <>
	constexpr GLenum typeToGLenum<float>() { return GL_FLOAT; }

	template <>
	constexpr GLenum typeToGLenum<double>() { return GL_DOUBLE; }

	template <>
	constexpr GLenum typeToGLenum<glm::vec2>() { return GL_FLOAT; }
	template <>
	constexpr GLenum typeToGLenum<glm::vec3>() { return GL_FLOAT; }
	template <>
	constexpr GLenum typeToGLenum<glm::vec4>() { return GL_FLOAT; }

	template <>
	constexpr GLenum typeToGLenum<glm::mat2>() { return GL_FLOAT; }
	template <>
	constexpr GLenum typeToGLenum<glm::mat3>() { return GL_FLOAT; }
	template <>
	constexpr GLenum typeToGLenum<glm::mat4>() { return GL_FLOAT; }

	template <class... Layout>
	class VboLayout {
	protected:
		template <class Cur, class... Rest>
		static GLuint vertexAttribPointer(GLuint index, GLuint divisor, GLboolean normalized, size_t pointer) {
			static constexpr GLenum type = typeToGLenum<Cur>();
			static constexpr size_t size = sizeof(Cur);

			if constexpr (std::is_integral_v<Cur>) { // integral types
				glEnableVertexAttribArray(index);
				glVertexAttribIPointer(index, 1, type, stride(), (void*)(pointer));
				glVertexAttribDivisor(index, divisor);
				++index;
			}
			else { // floating types
				if constexpr (size > 16) { // mat types
					static constexpr size_t vecs = size / sizeof(float);
					static constexpr size_t packSize = vecs / sizeof(float);
					for (int i = 0; i < vecs; ++i) {
						glEnableVertexAttribArray(index  + i);
						glVertexAttribPointer(index + i, packSize, type, normalized, stride(), (void*)(pointer + i * packSize * sizeof(float)));
						glVertexAttribDivisor(index, divisor);
					}
					
					index += vecs;
				}
				else if constexpr (size > 4) { // vec types
					static constexpr size_t packSize = size / sizeof(float);
					glEnableVertexAttribArray(index);
					glVertexAttribPointer(index, packSize, type, normalized, stride(), (void*)(pointer));
					glVertexAttribDivisor(index, divisor);
					++index;
				}
				else {
					glEnableVertexAttribArray(index);
					glVertexAttribPointer(index, 1, type, normalized, stride(), (void*)(pointer));
					glVertexAttribDivisor(index, divisor);
					++index;
				}
			}

			if constexpr (sizeof...(Rest) > 0) {
				return vertexAttribPointer<Rest...>(index, divisor, normalized, pointer + size);
			}
			else {
				return index;
			}
		}
	public:
		static_assert(sizeof...(Layout) > 0);

		using DataT = std::tuple<Layout...>;

		static constexpr int stride() {
			// this layout doesn't actually hold any data, but a tuple would
			return sizeof(DataT);
		}

		static auto vertexAttribPointer(GLuint index = 0, GLuint divisor = 0, GLboolean normalized = GL_FALSE) {
			return vertexAttribPointer<Layout...>(index, divisor, normalized, 0);
		}
	};

	template <class Layout>
	struct VBO {
	
		using DataT = Layout::DataT;

		GLuint id;
		GLenum target;

		operator GLuint() const {
			return id;
		}

		VBO() :
			id(0), target(GL_ARRAY_BUFFER) {}

		explicit VBO(GLenum target) :
			id(0), target(target) {}

		void bind() const {
			glBindBuffer(target, id);
		}

		template <class LinearContainer>
		void setData(const LinearContainer& data) const {
			using vtype = LinearContainer::value_type;

			static_assert(sizeof(vtype) == sizeof(DataT));

			bind();
			glBufferData(target, data.size() * sizeof(vtype), data.data(), GL_STATIC_DRAW);
		}

		void gen() {
			if (id != 0) {
				destroy();
			}
			glGenBuffers(1, &id);
		}

		void destroy() {
			glDeleteBuffers(1, &id);
			id = 0;
		}
	};
}