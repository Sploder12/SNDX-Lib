#include "container/entity_vector.hpp"

#include <gtest/gtest.h>

using namespace sndx::container;

#include <string>

TEST(EntityVector, emplaceWorks) {
	EntityVector<std::string> es{};

	auto h = es.emplace("test");
	EXPECT_TRUE(es.isAlive(h));

	auto ptr = es.get(h);
	ASSERT_NE(ptr, nullptr);
	EXPECT_EQ(*ptr, "test");
}

TEST(EntityVector, eraseWorks) {
	EntityVector<std::string> es{};

	auto h = es.emplace("test");
	EXPECT_TRUE(es.isAlive(h));
	EXPECT_NE(es.get(h), nullptr);

	EXPECT_TRUE(es.erase(h));
	EXPECT_FALSE(es.isAlive(h));
	EXPECT_EQ(es.get(h), nullptr);
}

TEST(EntityVector, repeatedUsageWorks) {
	EntityVector<uint32_t> es{};

	auto h = es.emplace(0);
	for (uint32_t i = 1; i < 100; ++i) {
		auto t = es.emplace(i);
		if (i % 3 == 0) {
			es.erase(t);
		}
		else if (i > 10) {
			EXPECT_LE(t.id, i);
		}
	}

	EXPECT_TRUE(es.isAlive(h));
	auto p = es.get(h);
	ASSERT_NE(p, nullptr);
	EXPECT_EQ(*p, 0);
	EXPECT_TRUE(es.erase(h));

	uint32_t count = 0;
	es.foreach([&count](uint32_t v) {
		++count;
		EXPECT_TRUE(v % 3 != 0);
	});

	EXPECT_EQ(count, 67 - 1);
}