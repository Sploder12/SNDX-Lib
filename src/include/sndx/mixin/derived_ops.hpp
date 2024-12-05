#pragma once

#include <concepts>

namespace sndx::mixin {

	// is_noexcept is needed since T is not complete during CRTP.
	// it is not possible to evaluate noexcept(T::...) due to this.

	// generates all comparison operators given operator<=>(T, O) exists
	template <class T, class O = T, bool is_noexcept = false>
	struct Comparisons {

		[[nodiscard]]
		constexpr bool operator==(const O& other) const noexcept(is_noexcept) {
			return (static_cast<const T&>(*this) <=> other) == 0;
		}

		[[nodiscard]]
		constexpr bool operator!=(const O& other) const noexcept(is_noexcept) {
			return !(static_cast<const T&>(*this) == other);
		}

		[[nodiscard]]
		constexpr bool operator<(const O& other) const noexcept(is_noexcept) {
			return (static_cast<const T&>(*this) <=> other) < 0;
		};

		[[nodiscard]]
		constexpr bool operator>=(const O& other) const noexcept(is_noexcept) {
			return !(static_cast<const T&>(*this) < other);
		}

		[[nodiscard]]
		constexpr bool operator>(const O& other) const noexcept(is_noexcept) {
			return (static_cast<const T&>(*this) <=> other) > 0;
		}

		[[nodiscard]]
		constexpr bool operator<=(const O& other) const noexcept(is_noexcept) {
			return !(static_cast<const T&>(*this) > other);
		}
	};

	// generates all + related operations given operator+=(T, O) exists
	template <class T, std::integral O, bool is_noexcept = false>
	struct AddInc {

		[[nodiscard]]
		constexpr T operator+(O n) const noexcept(is_noexcept) {
			T out = static_cast<const T&>(*this);
			return out += n;
		}

		[[nodiscard]]
		friend constexpr T operator+(O n, const T& t) noexcept(is_noexcept) {
			return t + n;
		}

		constexpr T& operator++() noexcept(is_noexcept) {
			return static_cast<T&>(*this) += O(1);
		}

		constexpr T operator++(int) noexcept(is_noexcept) {
			T tmp = static_cast<const T&>(*this);
			++static_cast<T&>(*this);
			return tmp;
		}
	};

	// generates all - related operations given operator+=(T, O) exists
	template <class T, std::integral O, bool is_noexcept = false>
	struct SubDec {

		[[nodiscard]]
		constexpr T operator-(O n) const noexcept(is_noexcept) {
			T out = static_cast<const T&>(*this);
			return out -= n;
		}

		[[nodiscard]]
		friend constexpr T operator-(O n, const T& t) noexcept(is_noexcept) {
			return t - n;
		}

		constexpr T& operator--() noexcept(is_noexcept) {
			return static_cast<T&>(*this) -= O(1);
		}

		constexpr T operator--(int) noexcept(is_noexcept) {
			T tmp = static_cast<const T&>(*this);
			--static_cast<T&>(*this);
			return tmp;
		}
	};

	// equivalent to : AddInc<T, O>, SubDec<T, O>
	template <class T, std::integral O, bool is_noexcept = false>
	struct AddSub : public AddInc<T, O, is_noexcept>, public SubDec<T, O, is_noexcept> {};
}