#include "data/json.hpp"

#include "../common.hpp"

#include <gmock/gmock.h>

class JsonTest : public ::testing::Test {

};

using namespace sndx;


TEST_F(JsonTest, EmptyDictIsEmpty) {
	data::DataDict dict{};

	EXPECT_EQ(data::toJSON(dict), "{}");
}

TEST_F(JsonTest, NestedDictWorks) {
	data::DataDict dict = {
		{ "pizza", 1 },
		{ "sub", data::DataDict{
			{"b", "w"}
		}}
	};

	auto json = data::toJSON(dict);
	EXPECT_THAT(json, ::testing::AnyOf(
		::testing::Eq("{\"pizza\":1,\"sub\":{\"b\":\"w\"}}"),
		::testing::Eq("{\"sub\":{\"b\":\"w\"},\"pizza\":1}")));
}

TEST_F(JsonTest, ArrayWorks) {
	data::DataDict dict = {
		{ "evil\"", nullptr },
		{ "arr", data::DataArray{
			"b", "wa", true, false, 5, 0
		}}
	};

	auto json = data::toJSON(dict);
	EXPECT_THAT(json, ::testing::AnyOf(
		::testing::Eq("{\"evil\\\"\":null,\"arr\":[\"b\",\"wa\",true,false,5,0]}"),
		::testing::Eq("{\"arr\":[\"b\",\"wa\",true,false,5,0],\"evil\\\"\":null}")));
}