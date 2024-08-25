#pragma once

#include <memory>
#include <type_traits>
#include <concepts>

namespace sndx::mixin {

	template <class T>
	struct is_cloneable : public std::bool_constant <
		requires (T t) { *t.clone(); }
	> {};

	template <class T>
	constexpr bool is_cloneable_v = is_cloneable<T>::value;

	template <class Derived, class Base>
	struct Cloneable : public Base {
		[[nodiscard]]
		constexpr std::unique_ptr<Base> clone() const override {
			return std::make_unique<Derived>(static_cast<const Derived&>(*this));
		}
	};
}