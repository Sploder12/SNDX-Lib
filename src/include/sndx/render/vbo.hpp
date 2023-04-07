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
	struct is_GLnormalized : std::bool_constant <
		requires (T) {
		std::same_as<T::normalized, std::bool_constant<true>>;
	} > {};

	template <typename T>
	struct is_vec : std::bool_constant <
		requires (T v) {
		T::length();
		T::x;
		v[0];
	} > {};

	template <typename T>
	struct is_mat : std::bool_constant <
		requires (T m) {
		T::length();
		T::x0;
		m[0][0];
	} > {};

	template <class T>
	constexpr GLenum typeToGLenum() {
		if constexpr (is_GLnormalized<T>::value) { //normalized types
			return typeToGLenum<T::value_type>();
		}
		else if constexpr (is_mat<T>::value) { //mat types
			return typeToGLenum<std::remove_cvref_t<decltype(T::x0)>>();
		}
		else if constexpr (is_vec<T>::value) { //vec types
			return typeToGLenum<std::remove_cvref_t<decltype(T::x)>>();
		}
		else if constexpr (std::is_same_v<T, char>) {
			return GL_BYTE;
		}
		else if constexpr (std::is_same_v<T, unsigned char>) {
			return GL_UNSIGNED_BYTE;
		}
		else if constexpr (std::is_same_v<T, short>) {
			return GL_SHORT;
		}
		else if constexpr (std::is_same_v<T, unsigned short>) {
			return GL_UNSIGNED_SHORT;
		}
		else if constexpr (std::is_same_v<T, int>) {
			return GL_INT;
		}
		else if constexpr (std::is_same_v<T, unsigned int>) {
			return GL_UNSIGNED_INT;
		}
		else if constexpr (std::is_same_v<T, float>) {
			return GL_FLOAT;
		}
		else if constexpr (std::is_same_v<T, double>) {
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
				using value_type = std::remove_cvref_t<decltype(Cur::x0)>;

				for (int i = 0; i < Cur::length(); ++i) {
					glEnableVertexAttribArray(index + i);
					glVertexAttribPointer(index + i, value_type::length(), type, normalized, stride(), (void*)(pointer + i * sizeof(value_type)));
					glVertexAttribDivisor(index, divisor);
				}
				index += Cur::length();
			}
			else if constexpr (is_vec<Cur>::value) {
				using value_type = std::remove_cvref_t<decltype(Cur::x)>;

				glEnableVertexAttribArray(index);
				glVertexAttribPointer(index, Cur::length(), type, normalized, stride(), (void*)(pointer));
				glVertexAttribDivisor(index, divisor);
				++index;
			}
			else {
				glEnableVertexAttribArray(index);
				glVertexAttribPointer(index, 1, type, normalized, stride(), (void*)(pointer));
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