#include "../common.hpp"

#include "container/recency_map.hpp"

#include <gmock/gmock.h>
using ::testing::Return;

using namespace sndx;

namespace {
	using KeyT = char;
	using ItemT = int;

	using TimeT = size_t;

	struct MockTimeProvider {
		MOCK_METHOD(TimeT, call, ());

		TimeT operator()() {
			return call();
		}

		std::function<TimeT()> asFunc() {
			return [this]() {
				return (*this)();
			};
		}
	};

	using BasicT = RecencyMap<KeyT, ItemT>;
	using TimeAwareT = TimeAwareRecencyMap<KeyT, ItemT, TimeT>;

	template <class T>
	struct RecencyMapTestFixture: public ::testing::Test {
		T map{};
	};

	template <>
	struct RecencyMapTestFixture<TimeAwareT>: public ::testing::Test {
		MockTimeProvider timeProvider{};

		TimeAwareT map{ timeProvider.asFunc() };
	};

	using RecencyMapTypes = ::testing::Types<
		BasicT,
		TimeAwareT
	>;
}

TYPED_TEST_SUITE(RecencyMapTestFixture, RecencyMapTypes);

TYPED_TEST(RecencyMapTestFixture, recency_map_tracks_insertion_recency) {
	auto& map = this->map;

	if constexpr (std::is_same_v<TypeParam, TimeAwareT>) {
		EXPECT_CALL(this->timeProvider, call()).Times(4).WillRepeatedly(Return(0));
	}

	map.insert_or_assign('b', 0);
	map.insert_or_assign('a', 1);
	map.insert_or_assign('o', -1);
	map.insert_or_assign('g', 42);

	EXPECT_EQ(map.size(), 4);
	EXPECT_TRUE(map.contains('b'));
	EXPECT_TRUE(map.contains('a'));
	EXPECT_TRUE(map.contains('o'));
	EXPECT_TRUE(map.contains('g'));

	map.pop_least_recent();

	EXPECT_EQ(map.size(), 3);
	EXPECT_FALSE(map.contains('b'));

	map.pop_most_recent();

	EXPECT_EQ(map.size(), 2);
	EXPECT_FALSE(map.contains('g'));

	EXPECT_TRUE(map.contains('a'));
	EXPECT_TRUE(map.contains('o'));
}

TYPED_TEST(RecencyMapTestFixture, recency_map_tracks_get_recency) {
	auto& map = this->map;

	if constexpr (std::is_same_v<TypeParam, TimeAwareT>) {
		EXPECT_CALL(this->timeProvider, call()).Times(5).WillRepeatedly(Return(0));
	}

	map.insert_or_assign('b', 0);
	map.insert_or_assign('a', 1);
	map.insert_or_assign('o', -1);
	map.insert_or_assign('g', 42);

	EXPECT_EQ(map.size(), 4);

	auto b = map.get('b');
	ASSERT_TRUE(b);
	EXPECT_EQ(*b, 0);

	map.pop_least_recent();

	EXPECT_EQ(map.size(), 3);

	EXPECT_TRUE(map.contains('b'));
	EXPECT_TRUE(map.contains('o'));
	EXPECT_TRUE(map.contains('g'));
	EXPECT_FALSE(map.contains('a'));
}

TYPED_TEST(RecencyMapTestFixture, recency_map_tracks_poke_recency) {
	auto& map = this->map;

	if constexpr (std::is_same_v<TypeParam, TimeAwareT>) {
		EXPECT_CALL(this->timeProvider, call()).Times(5).WillRepeatedly(Return(0));
	}

	map.insert_or_assign('b', 0);
	map.insert_or_assign('a', 1);
	map.insert_or_assign('o', -1);
	map.insert_or_assign('g', 42);

	EXPECT_EQ(map.size(), 4);

	EXPECT_TRUE(map.poke('b'));

	map.pop_least_recent();

	EXPECT_EQ(map.size(), 3);

	EXPECT_TRUE(map.contains('b'));
	EXPECT_TRUE(map.contains('o'));
	EXPECT_TRUE(map.contains('g'));
	EXPECT_FALSE(map.contains('a'));
}

TYPED_TEST(RecencyMapTestFixture, recency_map_iterates_in_recency_order) {
	auto& map = this->map;

	if constexpr (std::is_same_v<TypeParam, TimeAwareT>) {
		EXPECT_CALL(this->timeProvider, call()).Times(11)
			.WillOnce(Return(0)).WillOnce(Return(0))
			.WillOnce(Return(0)).WillOnce(Return(0))
			.WillOnce(Return(0)).WillOnce(Return(0))
			.WillOnce(Return(0)) // 'd' does not get time 1
			.WillRepeatedly(Return(1));
	}

	map.insert_or_assign('d', 0);
	map.insert_or_assign('b', 1);
	map.insert_or_assign('c', -1);
	map.insert_or_assign('a', 42);
	map.insert_or_assign('f', 4);
	map.insert_or_assign('e', 2);

	EXPECT_THAT(map, ::testing::ElementsAre(
		::testing::Key(::testing::Pointee('e')),
		::testing::Key(::testing::Pointee('f')),
		::testing::Key(::testing::Pointee('a')),
		::testing::Key(::testing::Pointee('c')),
		::testing::Key(::testing::Pointee('b')),
		::testing::Key(::testing::Pointee('d'))
	));

	map.poke('d');
	map.poke('c');
	map.poke('b');
	map.poke('a');

	EXPECT_THAT(map, ::testing::ElementsAre(
		::testing::Key(::testing::Pointee('a')),
		::testing::Key(::testing::Pointee('b')),
		::testing::Key(::testing::Pointee('c')),
		::testing::Key(::testing::Pointee('d')),
		::testing::Key(::testing::Pointee('e')),
		::testing::Key(::testing::Pointee('f'))
	));

	if constexpr (std::is_same_v<TypeParam, TimeAwareT>) {
		EXPECT_EQ(map.erase_older_than(1), 3);
		EXPECT_EQ(map.size(), 3);
		EXPECT_TRUE(map.contains('a'));
		EXPECT_TRUE(map.contains('b'));
		EXPECT_TRUE(map.contains('c'));
	}
}
