#include "data/data_io.hpp"

#include "../common.hpp"

#include <gmock/gmock.h>

class DataIOtest : public ::testing::Test {

};

using namespace sndx;


TEST_F(DataIOtest, EmptyDictIsEmpty) {
	data::DataDict dict{};

	EXPECT_EQ(data::encodeData(dict), "{}");
}

TEST_F(DataIOtest, NestedDictWorks) {
	data::DataDict dict = {
		{ "pizza", 1 },
		{ "sub", data::DataDict{
			{"b", "w"}
		}}
	};

	auto json = data::encodeData(dict);
	EXPECT_THAT(json, ::testing::AnyOf(
		::testing::Eq("{\"pizza\":1,\"sub\":{\"b\":\"w\"}}"),
		::testing::Eq("{\"sub\":{\"b\":\"w\"},\"pizza\":1}")));
}

TEST_F(DataIOtest, ArrayWorks) {
	data::DataDict dict = {
		{ "evil\"", nullptr },
		{ "arr", data::DataArray{
			"b", 1.5, "wa", true, false, 5.0, 0
		}}
	};

	auto json = data::encodeData(dict);
	EXPECT_THAT(json, ::testing::AnyOf(
		::testing::Eq("{\"evil\\\"\":null,\"arr\":[\"b\",1.5,\"wa\",true,false,5.0,0]}"),
		::testing::Eq("{\"arr\":[\"b\",1.5,\"wa\",true,false,5,0],\"evil\\\"\":null}")));
}

TEST_F(DataIOtest, prettyWorks) {
	data::DataDict dict = {
		{ "arr", data::DataArray{
			1.5, "wa", true, false, 5.0, data::DataDict{
				{"fish", "🐟"}
			},
			0
		}}
	};

	auto json = data::encodeData<data::prettyJSONencoder>(dict);
	EXPECT_EQ(json, "{\n\t\"arr\": [\n\t\t1.5,\n\t\t\"wa\",\n\t\ttrue,\n\t\tfalse,\n\t\t5.0,\n\t\t{\n\t\t\t\"fish\": \"🐟\"\n\t\t},\n\t\t0\n\t]\n}");
}