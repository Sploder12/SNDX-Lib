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
	constexpr GLenum typeToGLenum<glm::vec<2, unsigned char>>() { return GL_UNSIGNED_BYTE; }
	template <>
	constexpr GLenum typeToGLenum<glm::vec<3, unsigned char>>() { return GL_UNSIGNED_BYTE; }
	template <>
	constexpr GLenum typeToGLenum<glm::vec<4, unsigned char>>() { return GL_UNSIGNED_BYTE; }

	template <>
	constexpr GLenum typeToGLenum<glm::mat2>() { return GL_FLOAT; }
	template <>
	constexpr GLenum typeToGLenum<glm::mat3>() { return GL_FLOAT; }
	template <>
	constexpr GLenum typeToGLenum<glm::mat4>() { return GL_FLOAT; }

	template <typename T>
	struct GLnormalized {
		using value_type = T;
		using normalized = std::bool_constant<true>;

		static constexpr GLenum glType = typeToGLenum<T>();
		T value;
	};

	template <typename T>
	constexpr GLenum typeToGLenum<GLnormalized<T>>() { return GLnormalized<T>::glType; };

	template <typename T>
	struct is_GLnormalized : std::bool_constant<
		requires (T) {
			std::same_as<T::normalized, std::bool_constant<true>>;
	}> {};

	template <class... Layout>
	class VboLayout {
	protected:
		template <class Cur, class... Rest>
		static GLuint vertexAttribPointer(GLuint index, GLuint divisor, size_t pointer) {
			static constexpr GLenum type = typeToGLenum<Cur>();
			static constexpr size_t size = sizeof(Cur);
			static constexpr GLboolean normalized = is_GLnormalized<Cur>::value;

			if constexpr (std::is_integral_v<Cur>) { // integral types
				glEnableVertexAttribArray(index);
				glVertexAttribPointer(index, 1, type, normalized, stride(), (void*)(pointer));
				glVertexAttribDivisor(index, divisor);
				++index;
			}
			else if constexpr (type == GL_UNSIGNED_BYTE) { // multiple bytes (meant for color data)
				static constexpr size_t packSize = size / sizeof(unsigned char);
				static_assert(packSize >= 1 && packSize <= 4);

				glEnableVertexAttribArray(index);
				glVertexAttribPointer(index, packSize, type, normalized, stride(), (void*)(pointer));
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
				return vertexAttribPointer<Rest...>(index, divisor, pointer + size);
			}
			else {
				return index;
			}
		}
	public:
		static_assert(sizeof...(Layout) > 0);

		using DataT = std::tuple<Layout...>;

		[[nodiscard]]
		static constexpr int stride() {
			return sizeof(DataT);
		}

		static auto vertexAttribPointer(GLuint index = 0, GLuint divisor = 0) {
			return vertexAttribPointer<Layout...>(index, divisor, 0);
		}
	};

	template <class Layout>
	class VBO {
	protected:
		template <class Cur, class... LinearContainers>
		static size_t size(const Cur& data, const LinearContainers&... others) {
			if constexpr (sizeof...(LinearContainers) > 0) {
				return data.size() + size<LinearContainers...>(others...);
			}
			else {
				return data.size();
			}
		}

		template <class Cur, class... LinearContainers>
		void subData(long long offset, const Cur& data, const LinearContainers&... others) const {
			using vtype = Cur::value_type;

			static_assert(sizeof(vtype) == sizeof(DataT));

			glBufferSubData(target, sizeof(vtype) * offset, data.size() * sizeof(vtype), data.data());

			if constexpr (sizeof...(LinearContainers) > 0) {
				subData(offset + sizeof(vtype) * data.size(), others...);
			}
		}
	public:
	
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

		void bind() {
			if (id == 0) gen();

			glBindBuffer(target, id);
		}

		template <class... LinearContainers>
		void setData(const LinearContainers&... datas) {
			static_assert(sizeof...(LinearContainers) > 1);

			size_t totalSize = size(datas...);

			if (totalSize <= 0) [[unlikely]] return;

			bind();
			glBufferData(target, totalSize * sizeof(DataT), nullptr, GL_DYNAMIC_DRAW);

			subData<LinearContainers...>(0, datas...);
		}

		// for containers that contain containers (ex. std::list<std::vector<...>>)
		// useful for batching
		template <std::forward_iterator It>
		void setData(It begin, It end) {
			using vtype = typename std::iterator_traits<It>::value_type::value_type;
			static_assert(sizeof(vtype) == sizeof(DataT));

			size_t totalSize = 0;
			for (It sizeIt = begin; sizeIt != end; ++sizeIt) {
				totalSize += sizeIt->size();
			}

			if (totalSize <= 0) [[unlikely]] return;

			bind();
			glBufferData(target, totalSize * sizeof(DataT), nullptr, GL_DYNAMIC_DRAW);

			long long offset = 0;
			for (It dataIt = begin; dataIt != end; ++dataIt) {
				glBufferSubData(target, sizeof(vtype) * offset, dataIt->size() * sizeof(vtype), dataIt->data());
				offset += dataIt->size();
			}
		}

		template <class LinearContainer>
		void setData(const LinearContainer& data) {
			using vtype = LinearContainer::value_type;
			static_assert(sizeof(vtype) == sizeof(DataT));

			bind();
			glBufferData(target, data.size() * sizeof(vtype), data.data(), GL_STATIC_DRAW);
		}

		void gen() {
			if (id != 0) destroy();
			
			glGenBuffers(1, &id);
		}

		void destroy() {
			glDeleteBuffers(1, &id);
			id = 0;
		}
	};
}