#pragma once

#include <GL/glew.h>

#include <glm/glm.hpp>

#include "../../mixin/handle.hpp"

namespace sndx::render {
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
	} > ;

	template <typename T>
	using is_mat = std::bool_constant <
		requires (T m) {
		T::length(); typename T::col_type;
		m[0][0];
	} > ;

	template <typename T>
	using is_vec = std::bool_constant <
		requires (T v) {
		T::length(); T::x;
		v[0];
	} && !is_mat<T>::value > ;

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

	class VBO {
	protected:
		GLuint m_id = 0;

		void destroy() {
			if (m_id != 0) {
				glDeleteBuffers(1, &m_id);
				m_id = 0;
			}
		}

		void gen() {
			if (m_id == 0)
				glGenBuffers(1, &m_id);
		}

	public:
		explicit VBO() {
			gen();
		}

		explicit constexpr VBO(GLuint id) noexcept :
			m_id(id) {}

		explicit constexpr VBO(VBO&& other) noexcept :
			m_id(std::exchange(other.m_id, 0)) {}

		VBO& operator=(VBO&& other) noexcept {
			std::swap(m_id, other.m_id);
			return *this;
		}

		VBO& operator=(GLuint id) noexcept {
			destroy();
			m_id = id;
			return *this;
		}

		constexpr operator GLuint() const noexcept {
			return m_id;
		}

		~VBO() noexcept {
			destroy();
		}

		void bind(GLenum target) {
			if (m_id == 0) gen();

			glBindBuffer(target, m_id);
		}
	};

	static_assert(sizeof(VBO) == sizeof(GLuint));

	template <class... Layout>
	class TypedVBO : public VBO {
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
		void subData(GLenum target, long long offset, const Cur& data, const LinearContainers&... others) const {
			using vtype = Cur::value_type;

			static_assert(sizeof(vtype) == sizeof(DataT));

			glBufferSubData(target, sizeof(vtype) * offset, data.size() * sizeof(vtype), data.data());

			if constexpr (sizeof...(LinearContainers) > 0) {
				subData(offset + sizeof(vtype) * data.size(), others...);
			}
		}

	public:
		using DataT = VboLayout<Layout...>::DataT;

		explicit TypedVBO() :
			VBO{} {}

		explicit TypedVBO(VBO&& other) :
			m_id(std::exchange(other.m_id, 0)) {}

		template <class... LinearContainers>
		void setData(GLenum target = GL_ARRAY_BUFFER, const LinearContainers&... datas) {
			static_assert(sizeof...(LinearContainers) > 1);

			size_t totalSize = size(datas...);

			if (totalSize <= 0) [[unlikely]] return;

			bind(target);
			glBufferData(target, totalSize * sizeof(DataT), nullptr, GL_DYNAMIC_DRAW);

			subData<LinearContainers...>(target, 0, datas...);
		}

		// for containers that contain containers (ex. std::list<std::vector<...>>)
		// useful for batching
		template <std::forward_iterator It>
		void setData(GLenum target, It begin, It end) {
			using vtype = typename std::iterator_traits<It>::value_type::value_type;
			static_assert(sizeof(vtype) == sizeof(DataT));

			size_t totalSize = 0;
			for (It sizeIt = begin; sizeIt != end; ++sizeIt) {
				totalSize += sizeIt->size();
			}

			if (totalSize <= 0) [[unlikely]] return;

			bind(target);
			glBufferData(target, totalSize * sizeof(DataT), nullptr, GL_DYNAMIC_DRAW);

			long long offset = 0;
			for (It dataIt = begin; dataIt != end; ++dataIt) {
				glBufferSubData(target, sizeof(vtype) * offset, dataIt->size() * sizeof(vtype), dataIt->data());
				offset += dataIt->size();
			}
		}

		template <class LinearContainer>
		void setData(GLenum target, const LinearContainer& data) {
			using vtype = LinearContainer::value_type;
			static_assert(sizeof(vtype) == sizeof(DataT));

			bind(target);
			glBufferData(target, data.size() * sizeof(vtype), data.data(), GL_STATIC_DRAW);
		}

		void setDataSize(GLenum target, size_t count) {
			bind(target);
			glBufferData(target, count * sizeof(DataT), nullptr, GL_DYNAMIC_DRAW);
		}

		template <class LinearContainer>
		void setSubData(GLenum target, const LinearContainer& data, size_t offset) {
			using vtype = LinearContainer::value_type;
			static_assert(sizeof(vtype) == sizeof(DataT));

			bind(target);
			glBufferSubData(target, sizeof(vtype) * offset, sizeof(vtype) * data.size(), data.data());
		}
	};

	template <class... Layout>
	class TypedVBOhandle : public sndx::mixin::Handle<TypedVBO<Layout...>> {
	protected:
		GLenum m_target;

	public:
		using VBOt = TypedVBO<Layout...>;
		using HandleT = sndx::mixin::Handle<VBOt>;

		constexpr operator GLuint() const {
			return this->get();
		}

		constexpr explicit TypedVBOhandle(VBOt& vbo, GLenum target = GL_ARRAY_BUFFER) :
			HandleT(vbo), m_target(target) {}

		GLenum setTarget(GLenum target) noexcept {
			return std::exchange(m_target, target);
		}

		[[nodiscard]]
		GLenum getTarget() noexcept {
			return m_target;
		}

		void bind() {
			this->get().bind(m_target);
		}

		template <class... Args>
		void setData(Args&&... args) {
			this->get().setData(m_target, std::forward<Args>(args)...);
		}

		void setDataSize(size_t count) {
			this->get().setDataSize(m_target, count);
		}

		template <class LinearContainer>
		void setSubData(const LinearContainer& data, size_t offset) {
			this->get().setSubData(m_target, data, offset);
		}
	};
}