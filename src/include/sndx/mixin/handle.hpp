#pragma once

#include <functional>

namespace sndx::mixin {

	template <class T>
	class Handle {
	private:
		std::reference_wrapper<T> m_obj;

	public:
		constexpr Handle(T& obj) noexcept :
			m_obj(obj) {}

		T* operator->() const noexcept {
			return std::addressof(m_obj.get());
		}

		T& operator*() const noexcept {
			return m_obj;
		}

		[[nodiscard]]
		decltype(auto) get() const {
			return m_obj.get();
		}
	};
}