#pragma once

#include <bit>
#include <concepts>

namespace sndx::utility {
	[[nodiscard]] // C++23's byteswap https://en.cppreference.com/w/cpp/numeric/byteswap
	constexpr auto byteswap(std::integral auto v) noexcept {
		static_assert(std::has_unique_object_representations_v<decltype(v)>);

		auto val = std::bit_cast<std::array<std::byte, sizeof(decltype(v))>>(v);
		std::reverse(val.begin(), val.end());
		return std::bit_cast<decltype(v)>(val);
	}

	template <std::endian endianess = std::endian::native> [[nodiscard]]
	constexpr auto fromEndianess(std::integral auto value) noexcept {
		static_assert(
			std::endian::native == std::endian::little || 
			std::endian::native == std::endian::big);

		if constexpr (endianess == std::endian::native) {
			return value;
		}
		else {
			return byteswap(value);
		}
	}
}