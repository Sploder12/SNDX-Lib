#include "math/binpack.hpp"

#include <gtest/gtest.h>

using namespace sndx::math;

TEST(Binpack, TrivialPacking) {
	BinPacker packer{};

	auto none_out = packer.pack(0);
	EXPECT_TRUE(none_out.empty());
	EXPECT_EQ(none_out.neededHeight, 0);
	EXPECT_EQ(none_out.neededWidth, 0);

	ASSERT_TRUE(packer.add("a", 10, 5));

 	auto trivial_out = packer.pack(10);
	
	EXPECT_EQ(trivial_out.neededHeight, 5);
	EXPECT_EQ(trivial_out.neededWidth, 10);
	ASSERT_TRUE(trivial_out.contains("a"));

	EXPECT_EQ(trivial_out.positions["a"].x, 0);
	EXPECT_EQ(trivial_out.positions["a"].y, 0);

	ASSERT_TRUE(packer.add("b", 1, 5));
	auto horizontal_out = packer.pack(11);

	EXPECT_EQ(horizontal_out.neededHeight, 5);
	EXPECT_EQ(horizontal_out.neededWidth, 11);

	EXPECT_TRUE(horizontal_out.contains("a"));
	ASSERT_TRUE(horizontal_out.contains("b"));

	EXPECT_EQ(horizontal_out.positions["a"].y, 0);
	EXPECT_EQ(horizontal_out.positions["b"].y, 0);

	auto aX = horizontal_out.positions["a"].x;
	auto bX = horizontal_out.positions["b"].x;

	EXPECT_NE(aX, bX);
	EXPECT_TRUE(aX == 0 || aX == 1);
	EXPECT_TRUE(bX == 0 || bX == 10);


	BinPacker<false> verticalPacker{};

	ASSERT_TRUE(verticalPacker.add("a", 5, 10));
	ASSERT_TRUE(verticalPacker.add("b", 5, 1));
	auto vertical_out = verticalPacker.pack(11);

	EXPECT_EQ(vertical_out.neededHeight, 11);
	EXPECT_EQ(vertical_out.neededWidth, 5);

	EXPECT_TRUE(vertical_out.contains("a"));
	ASSERT_TRUE(vertical_out.contains("b"));

	EXPECT_EQ(vertical_out.positions["a"].x, 0);
	EXPECT_EQ(vertical_out.positions["b"].x, 0);

	auto aY = vertical_out.positions["a"].y;
	auto bY = vertical_out.positions["b"].y;

	EXPECT_NE(aY, bY);
	EXPECT_TRUE(aY == 0 || aY == 1);
	EXPECT_TRUE(bY == 0 || bY == 10);
}

TEST(Binpack, PaddingPads) {
	BinPacker packer{};

	auto none_out = packer.pack(0, 20);
	EXPECT_TRUE(none_out.empty());
	EXPECT_EQ(none_out.neededHeight, 0);
	EXPECT_EQ(none_out.neededWidth, 0);

	ASSERT_TRUE(packer.add("a", 10, 5));

	auto trivial_out = packer.pack(10, 20);

	EXPECT_EQ(trivial_out.neededHeight, 5);
	EXPECT_EQ(trivial_out.neededWidth, 10);
	ASSERT_TRUE(trivial_out.contains("a"));

	EXPECT_EQ(trivial_out.positions["a"].x, 0);
	EXPECT_EQ(trivial_out.positions["a"].y, 0);

	ASSERT_TRUE(packer.add("b", 1, 5));
	auto horizontal_out = packer.pack(11 + 20, 20);

	EXPECT_EQ(horizontal_out.neededHeight, 5);
	EXPECT_EQ(horizontal_out.neededWidth, 11 + 20);

	EXPECT_TRUE(horizontal_out.contains("a"));
	ASSERT_TRUE(horizontal_out.contains("b"));

	EXPECT_EQ(horizontal_out.positions["a"].y, 0);
	EXPECT_EQ(horizontal_out.positions["b"].y, 0);

	auto aX = horizontal_out.positions["a"].x;
	auto bX = horizontal_out.positions["b"].x;

	EXPECT_NE(aX, bX);
	EXPECT_TRUE(aX == 0 || aX == 1 + 20);
	EXPECT_TRUE(bX == 0 || bX == 10 + 20);
}

TEST(Binpack, invalid_packing_throws) {
	BinPacker packer{};

	ASSERT_TRUE(packer.add("a", 10, 5));
	
	EXPECT_THROW(auto ign = packer.pack(0), std::invalid_argument);
	EXPECT_THROW(auto ign = packer.pack(1), std::invalid_argument);
}