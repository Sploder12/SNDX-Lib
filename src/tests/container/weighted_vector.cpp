#include "container/weighted_vector.hpp"

#include <gtest/gtest.h>

using namespace sndx::container;

TEST(WeightedVector, BadInsertion) {
	WeightedVector<int, int> vec{};

	EXPECT_THROW(vec.emplace_back(-1337, 0x1337), std::invalid_argument);
	EXPECT_THROW(vec.emplace_back(-1, 0xdead), std::invalid_argument);
	EXPECT_THROW(vec.push_back(0, 0xbeef), std::invalid_argument);
	
	EXPECT_EQ(vec.size(), 0);
	EXPECT_EQ(vec.count(), 0);
	EXPECT_TRUE(vec.empty());
}

TEST(WeightedVector, LinearPush) {
	WeightedVector<int, int> vec{};

	EXPECT_EQ(vec.at(0), nullptr);

	for (int i = 0; i <= 10000; ++i) {
		vec.emplace_back(1, i * 2);
	}

	EXPECT_EQ(*vec.at(-10000), 0);
	EXPECT_EQ(*vec.at(-1), 0);
	EXPECT_EQ(*vec.at(0), 0);
	EXPECT_EQ(*vec.at(1), 2);
	EXPECT_EQ(*vec.at(500), 1000);
	EXPECT_EQ(*vec.at(10000), 20000);
	EXPECT_EQ(*vec.at(10000), vec.back().data);
	EXPECT_EQ(vec.at(10001), nullptr);

	EXPECT_EQ(vec.count(), vec.size());
	EXPECT_EQ(vec.size(), 10001);
}

TEST(WeightedVector, FloatWeight) {
	WeightedVector<int, float> vec{};

	for (int i = 0; i <= 10000; ++i) {
		vec.emplace_back(0.25f, i);
	}

	EXPECT_EQ(*vec.at(-10000.0f), 0);
	EXPECT_EQ(*vec.at(-1.0f), 0);
	EXPECT_EQ(*vec.at(0.0f), 0);
	EXPECT_EQ(*vec.at(0.25f), 1);
	EXPECT_EQ(*vec.at(0.44f), 1);
	EXPECT_EQ(*vec.at(250.0f), 1000);
	EXPECT_EQ(*vec.at(2500.0f), 10000);
	EXPECT_EQ(*vec.at(2500.2f), vec.back().data);
	EXPECT_EQ(vec.at(2500.25f), nullptr);

	EXPECT_EQ(vec.count(), 10001);
	EXPECT_EQ(vec.size(), 2500.25);
}

TEST(WeightedVector, PushPop) {
	WeightedVector<int> vec{};

	vec.pop_back();
	EXPECT_EQ(vec.size(), 0);

	for (int i = 1; i <= 10000; ++i) {
		vec.push_back(i, i);
		vec.emplace_back(i + 1, i + 1);

		EXPECT_EQ(vec.size(), i * 2 + 1);
		EXPECT_EQ(vec.back().data, i + 1);

		vec.pop_back();

		EXPECT_EQ(vec.size(), i);
		EXPECT_EQ(vec.back().data, i);

		vec.pop_back();

		EXPECT_EQ(vec.size(), 0);
		EXPECT_TRUE(vec.empty());
	}
}