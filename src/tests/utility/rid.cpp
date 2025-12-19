#include "utility/rid.hpp"

#include <gtest/gtest.h>

using namespace sndx::utility;

#include <unordered_map>

namespace {
	constexpr size_t test_category = 12951745512389773876;
}

TEST(Rids, ridsAreUnique) {
	auto id1 = generateRID<test_category>();
	auto id2 = generateRID<test_category>();

	EXPECT_NE(id1, id2);
	EXPECT_NE(id1, nullRID<test_category>());
	EXPECT_NE(id2, nullRID<test_category>());
}

TEST(Rids, ridsAreHashable) {
	auto id1 = generateRID<test_category + 1>();
	auto id2 = generateRID<test_category + 1>();

	std::unordered_map<RID<test_category + 1>, int> map{};
	map[id1] = 2;
	map[id2] = 5;

	EXPECT_EQ(map[id1], 2);
	EXPECT_EQ(map[id2], 5);
}

TEST(Rids, nullRidsAreEqual) {
	auto null1 = RID<test_category>(nullptr);
	auto null2 = RID<test_category>(nullptr);

	EXPECT_EQ(null1, null2);
}