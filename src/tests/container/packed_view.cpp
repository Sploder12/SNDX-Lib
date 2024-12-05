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

	EXPECT_EQ(view.size(), arr.size());
	EXPECT_EQ(arr.data(), (std::uint8_t*)(view.data()));

	for (size_t i = 0; i < view.size(); ++i) {
		EXPECT_EQ(view[i].to_ullong(), i + 1);
	}

	view = view.subview(3);

	EXPECT_EQ(view.size(), arr.size() - 3);
	EXPECT_EQ(arr.data() + 3, (std::uint8_t*)(view.data()));

	for (size_t i = 0; i < view.size(); ++i) {
		EXPECT_EQ(view[i].to_ullong(), i + 1 + 3);
	}

	view = view.subview(0, 1);
	EXPECT_EQ(view.size(), 1);
	EXPECT_EQ(view.at(0), 4);

	decltype(view)::out_type t{}; // needed because of [[nodiscard]]
	EXPECT_THROW(t = view.at(1), std::out_of_range);
}

TEST(PackedView, Norma16Bit) {
	std::array<std::uint16_t, 10> arr{};

	std::iota(arr.begin(), arr.end(), 1337);

	const PackedView<sizeof(std::uint16_t) * 8> view((std::byte*)(arr.data()), arr.size());

	EXPECT_EQ(view.size(), arr.size());
	EXPECT_EQ(arr.data(), (std::uint16_t*)(view.data()));

	for (size_t i = 0; i < view.size(); ++i) {
		EXPECT_EQ(view[i].to_ullong(), arr[i]);
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

	EXPECT_EQ(view.size(), arr.size() - 1);
	EXPECT_EQ(arr.data(), (std::uint8_t*)(view.data()));

	for (size_t i = 0; i < view.size(); ++i) {
		EXPECT_EQ(view[i].to_ullong(), i + 1);
	}

	view = view.subview(1);

	EXPECT_EQ(view.size(), arr.size() - 2);

	for (size_t i = 0; i < view.size(); ++i) {
		EXPECT_EQ(view[i].to_ullong(), i + 2);
	}
}

TEST(PackedView, Bits3Big) {
	std::array<std::uint8_t, 3> arr{
		0b00000101,
		0b00111001,
		0b01110111,
	};

	const PackedView<3, std::endian::big> view((std::byte*)(arr.data()), 7);

	EXPECT_EQ(view.size(), 7);
	EXPECT_EQ(arr.data(), (std::uint8_t*)(view.data()));

	for (size_t i = 0; i < view.size(); ++i) {
		EXPECT_EQ(view[i].to_ullong(), i);
	}

	std::array<std::uint8_t, 4> arrOff1{
		0b00000010,
		0b10011100,
		0b10111011,
		0b10000000
	};

	const PackedView<3, std::endian::big> viewOff1((std::byte*)(arrOff1.data()), 7, 1);

	EXPECT_EQ(viewOff1.size(), 7);
	EXPECT_EQ(arrOff1.data(), (std::uint8_t*)(viewOff1.data()));

	size_t i = 0;
	for (auto v : viewOff1) {
		EXPECT_EQ(v.to_ullong(), i);
		++i;
	}
}

TEST(PackedView, Bits3Little) {
	std::array<std::uint8_t, 3> arr{
		0b10001000,
		0b11000110,
		0b11111010,
	};

	const PackedView<3, std::endian::little> view((std::byte*)(arr.data()), 7);

	EXPECT_EQ(view.size(), 7);
	EXPECT_EQ(arr.data(), (std::uint8_t*)(view.data()));

	for (size_t i = 0; i < view.size(); ++i) {
		EXPECT_EQ(view[i].to_ullong(), i);
	}
}

TEST(PackedView, SubviewThrowsWhenOutOfRange) {
	std::byte data{0xff};
	const PackedView<sizeof(data) * 8> view(&data, sizeof(data));

	const auto emptyView = view.subview(view.size());
	EXPECT_EQ(emptyView.size(), 0);

	EXPECT_THROW((std::ignore = view.subview(view.size() + 1)), std::out_of_range);
}