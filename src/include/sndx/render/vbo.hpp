#pragma once

#include <gl/glew.h>

#include <glm/glm.hpp>

namespace sndx {

	template <typename T>
	struct GLnormalized {
		using value_type = T;
		using normalized = std::bool_constant<true>;
		T value;
	};

	template <typename T>
	using is_GLnormalized = std::bool_constant <
		requires (T) {
		std::same_as<T::normalized, std::bool_constant<true>>;
	} >;

	template <typename T>
	using is_vec = std::bool_constant <
		requires (T v) {
		T::length(); T::x;
		v[0];
	} >;

	template <typename T>
	using is_mat = std::bool_constant <
		requires (T m) {
		T::length(); typename T::col_type;
		m[0][0];
	} >;

	template <class T> [[nodiscard]]
	constexpr GLenum typeToGLenum() {
		static_assert(!std::is_pointer_v<T>);
		static_assert(!std::is_reference_v<T>);

		if constexpr (is_GLnormalized<T>::value) { //normalized types
			return typeToGLenum<T::value_type>();
		}
		else if constexpr (is_mat<T>::value) { //mat types
			return typeToGLenum<typename T::value_type>();
		}
		else if constexpr (is_vec<T>::value) { //vec types
			return typeToGLenum<typename T::value_type>();
		}
		else if constexpr (std::is_same_v<T, GLbyte>) {
			return GL_BYTE;
		}
		else if constexpr (std::is_same_v<T, GLubyte>) {
			return GL_UNSIGNED_BYTE;
		}
		else if constexpr (std::is_same_v<T, GLshort>) {
			return GL_SHORT;
		}
		else if constexpr (std::is_same_v<T, GLushort>) {
			return GL_UNSIGNED_SHORT;
		}
		else if constexpr (std::is_same_v<T, GLint>) {
			return GL_INT;
		}
		else if constexpr (std::is_same_v<T, GLuint>) {
			return GL_UNSIGNED_INT;
		}
		else if constexpr (std::is_same_v<T, GLfloat>) {
			return GL_FLOAT;
		}
		else if constexpr (std::is_same_v<T, GLdouble>) {
			return GL_DOUBLE;
		}
		else {
			return -1;
		}
	}

	template <class... Layout>
	class VboLayout {
	protected:
		template <class Cur, class... Rest>
		static GLuint vertexAttribPointer(GLuint index, GLuint divisor, size_t pointer) {
			static constexpr GLenum type = typeToGLenum<Cur>();
			static_assert(type != -1);

			static constexpr GLboolean normalized = is_GLnormalized<Cur>::value;

			if constexpr (is_mat<Cur>::value) {
				using value_type = Cur::col_type;

				for (int i = 0; i < Cur::length(); ++i) {
					glEnableVertexAttribArray(index + i);

					if constexpr (!normalized && std::is_integral_v<value_type>) {
						glVertexAttribIPointer(index + i, value_type::length(), type, stride(), (void*)(pointer + i * sizeof(value_type)));
					}
					else {
						glVertexAttribPointer(index + i, value_type::length(), type, normalized, stride(), (void*)(pointer + i * sizeof(value_type)));
					}

					glVertexAttribDivisor(index + i, divisor);
				}
				index += Cur::length();
			}
			else if constexpr (is_vec<Cur>::value) {
				using value_type = std::remove_cvref_t<decltype(Cur::x)>;

				glEnableVertexAttribArray(index);

				if constexpr (!normalized && std::is_integral_v<value_type>) {
					glVertexAttribIPointer(index, Cur::length(), type, stride(), (void*)(pointer));
				}
				else {
					glVertexAttribPointer(index, Cur::length(), type, normalized, stride(), (void*)(pointer));
				}

				glVertexAttribDivisor(index, divisor);
				++index;
			}
			else {
				glEnableVertexAttribArray(index);

				if constexpr (!normalized && std::is_integral_v<Cur>) {
					glVertexAttribIPointer(index, 1, type, stride(), (void*)(pointer));
				}
				else {
					glVertexAttribPointer(index, 1, type, normalized, stride(), (void*)(pointer));
				}

				glVertexAttribDivisor(index, divisor);
				++index;
			}

			if constexpr (sizeof...(Rest) > 0) {
				return vertexAttribPointer<Rest...>(index, divisor, pointer + sizeof(Cur));
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

	template <class... Layout>
	class VBO {
	protected:
		template <class Cur, class... LinearContainers> [[nodiscard]]
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
	
		using DataT = VboLayout<Layout...>::DataT;

		GLuint id;
		GLenum target;

		constexpr operator GLuint() const {
			return id;
		}

		constexpr VBO() :
			id(0), target(GL_ARRAY_BUFFER) {}

		constexpr explicit VBO(GLenum target) :
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

		void setDataSize(size_t count) {
			bind();
			glBufferData(target, count * sizeof(DataT), nullptr, GL_DYNAMIC_DRAW);
		}

		template <class LinearContainer>
		void setSubData(const LinearContainer& data, size_t offset) {
			using vtype = LinearContainer::value_type;
			static_assert(sizeof(vtype) == sizeof(DataT));

			bind();
			glBufferSubData(target, sizeof(vtype) * offset, sizeof(vtype) * data.size(), data.data());
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