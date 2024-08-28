#include "container/packed_view.hpp"

#include <gtest/gtest.h>

using namespace sndx::container;

#include <array>
#include <numeric>

#include "math/math.hpp"

TEST(PackedView, NormalBytes) {
	std::array<std::uint8_t, 10> arr{};

	std::iota(arr.begin(), arr.end(), 1);

	PackedView<sizeof(std::uint8_t) * 8> view((std::byte*)(arr.data()), arr.size());

	ASSERT_EQ(view.size(), arr.size());
	ASSERT_EQ(arr.data(), (std::uint8_t*)(view.data()));

	for (size_t i = 0; i < view.size(); ++i) {
		ASSERT_EQ(view[i].to_ullong(), i + 1);
	}

	view = view.subview(3);

	ASSERT_EQ(view.size(), arr.size() - 3);
	ASSERT_EQ(arr.data() + 3, (std::uint8_t*)(view.data()));

	for (size_t i = 0; i < view.size(); ++i) {
		ASSERT_EQ(view[i].to_ullong(), i + 1 + 3);
	}

	view = view.subview(0, 1);
	ASSERT_EQ(view.size(), 1);
	ASSERT_EQ(view.at(0), 4);

	decltype(view)::out_type t{}; // needed because of [[nodiscard]]
	ASSERT_THROW(t = view.at(1), std::out_of_range);
}

TEST(PackedView, Norma16Bit) {
	std::array<std::uint16_t, 10> arr{};

	std::iota(arr.begin(), arr.end(), 1337);

	PackedView<sizeof(std::uint16_t) * 8> view((std::byte*)(arr.data()), arr.size());

	ASSERT_EQ(view.size(), arr.size());
	ASSERT_EQ(arr.data(), (std::uint16_t*)(view.data()));

	for (size_t i = 0; i < view.size(); ++i) {
		ASSERT_EQ(view[i].to_ullong(), arr[i]);
	}
}

TEST(PackedView, Offset8Bit) {
	std::array<std::uint8_t, 9> arr{
		0b00000000, 
		0b00100000,
		0b01000000,
		0b01100000,
		0b10000000,
		0b10100000,
		0b11000000,
		0b11100001,
		0b00000000,
	};

	PackedView<sizeof(std::uint8_t) * 8, std::endian::big> view((std::byte*)(arr.data()), arr.size() - 1, 3);

	ASSERT_EQ(view.size(), arr.size() - 1);
	ASSERT_EQ(arr.data(), (std::uint8_t*)(view.data()));

	for (size_t i = 0; i < view.size(); ++i) {
		ASSERT_EQ(view[i].to_ullong(), i + 1);
	}

	view = view.subview(1);

	ASSERT_EQ(view.size(), arr.size() - 2);

	for (size_t i = 0; i < view.size(); ++i) {
		ASSERT_EQ(view[i].to_ullong(), i + 2);
	}
}

TEST(PackedView, Bits3Big) {
	std::array<std::uint8_t, 3> arr{
		0b00000101,
		0b00111001,
		0b01110111,
	};

	PackedView<3, std::endian::big> view((std::byte*)(arr.data()), 7);

	ASSERT_EQ(view.size(), 7);
	ASSERT_EQ(arr.data(), (std::uint8_t*)(view.data()));

	for (size_t i = 0; i < view.size(); ++i) {
		ASSERT_EQ(view[i].to_ullong(), i);
	}
}

TEST(PackedView, Bits3Little) {
	std::array<std::uint8_t, 3> arr{
		0b10001000,
		0b11000110,
		0b11111010,
	};

	PackedView<3, std::endian::little> view((std::byte*)(arr.data()), 7);

	ASSERT_EQ(view.size(), 7);
	ASSERT_EQ(arr.data(), (std::uint8_t*)(view.data()));

	for (size_t i = 0; i < view.size(); ++i) {
		ASSERT_EQ(view[i].to_ullong(), i);
	}
}