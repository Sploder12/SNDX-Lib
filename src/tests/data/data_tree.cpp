#include "data/data_tree.hpp"

#include "../common.hpp"

class DataTreeTest : public ::testing::Test {

};

using namespace sndx;

TEST_F(DataTreeTest, NumberCanBeInt) {
	data::Number number{ 1 };

	EXPECT_EQ(number, 1);

	EXPECT_TRUE(number.isInt());
	EXPECT_FALSE(number.isFloat());

	EXPECT_NE(number.getInt(), nullptr);
	EXPECT_EQ(number.getFloat(), nullptr);

	EXPECT_EQ(number.getIntOr(0), 1);
	EXPECT_EQ(number.getFloatOr(0.0), 0.0);

	EXPECT_EQ(number.getAsInt(), 1);
}

TEST_F(DataTreeTest, NumberCanBeFloat) {
	data::Number number{ 1.5 };

	EXPECT_EQ(number, 1.5);

	EXPECT_TRUE(number.isFloat());
	EXPECT_FALSE(number.isInt());

	EXPECT_NE(number.getFloat(), nullptr);
	EXPECT_EQ(number.getInt(), nullptr);

	EXPECT_EQ(number.getFloatOr(0.0), 1.5);
	EXPECT_EQ(number.getIntOr(0), 0);

	EXPECT_EQ(number.getAsFloat(), 1.5);
}

TEST_F(DataTreeTest, NumberCanBeConverted) {
	data::Number number{ 1.0 };

	EXPECT_EQ(number, 1);
	EXPECT_EQ(number.getFloatOr(0), 1.0);

	number = 5;

	EXPECT_EQ(number, 5.0);
	EXPECT_EQ(number.getIntOr(0), 5);
}


TEST_F(DataTreeTest, ValueCanBeNumber) {
	data::Value inumber{ 2 };
	const data::Value fnumber{ 4.0 };

	EXPECT_TRUE(inumber.isNumber());
	EXPECT_TRUE(fnumber.isNumber());

	EXPECT_NE(inumber.getNumber(), nullptr);
	EXPECT_NE(fnumber.getNumber(), nullptr);

	EXPECT_EQ(inumber.getNumberOr(4.0), 2);
	EXPECT_EQ(fnumber.getNumberOr(2), 4.0);
	EXPECT_EQ(inumber, 2.0);
	EXPECT_EQ(fnumber, 4);

	inumber = 4;
	EXPECT_EQ(inumber, 4);
	EXPECT_EQ(inumber, fnumber);
}

TEST_F(DataTreeTest, ValueCanBeString) {
	data::Value str{ "hi" };

	EXPECT_TRUE(str.isString());

	EXPECT_NE(str.getString(), nullptr);
	EXPECT_EQ(str.getStringOr("bye"), "hi");
	EXPECT_EQ(str, "hi");

	str = std::string_view("hello");
	EXPECT_EQ(str, "hello");
}

TEST_F(DataTreeTest, ValueCanBeBool) {
	data::Value b{ false };

	EXPECT_TRUE(b.isBool());

	EXPECT_NE(b.getBool(), nullptr);
	EXPECT_EQ(b.getBoolOr(true), false);
	EXPECT_EQ(b, false);

	b = true;
	EXPECT_EQ(b, true);
}

TEST_F(DataTreeTest, ValueCanConvertToNumber) {
	const data::Value s{ "10.5" };
	EXPECT_EQ(s.getAsNumber(), 10.5);

	const data::Value i{ 1337 };
	EXPECT_EQ(i.getAsNumber(), 1337);

	data::Value f{ false };
	EXPECT_EQ(f.getAsNumber(), 0);

	const data::Value t{ true };
	EXPECT_NE(t.getAsNumber(), 0);
}

TEST_F(DataTreeTest, ValueCanConvertToString) {
	const data::Value s{ "hello" };
	EXPECT_EQ(s.getAsString(), "hello");

	const data::Value i{ 1337 };
	EXPECT_EQ(i.getAsString(), "1337");

	data::Value f{ false };
	EXPECT_EQ(f.getAsString(), "false");
	
	const data::Value t{ true };
	EXPECT_EQ(t.getAsString(), "true");
}

TEST_F(DataTreeTest, ValueCanConvertToBool) {
	const data::Value tr{ "true" };
	EXPECT_EQ(tr.getAsBool(), true);

	const data::Value Tr{ "TruE" };
	EXPECT_EQ(Tr.getAsBool(), true);

	const data::Value TR{ "TRUE" };
	EXPECT_EQ(TR.getAsBool(), true);

	const data::Value F{ "true " };
	EXPECT_EQ(F.getAsBool(), false);

	const data::Value i{ 1337 };
	EXPECT_EQ(i.getAsBool(), true);

	const data::Value z{ 0 };
	EXPECT_EQ(z.getAsBool(), false);

	data::Value f{ false };
	EXPECT_EQ(f.getAsBool(), false);

	const data::Value t{ true };
	EXPECT_EQ(t.getAsBool(), true);
}

TEST_F(DataTreeTest, DefaultDataIsNull) {
	data::Data data{};

	EXPECT_TRUE(data.isNull());
	EXPECT_FALSE(data);
	EXPECT_EQ(data, nullptr);
}

TEST_F(DataTreeTest, DataCanBeValue) {
	data::Data num(1);
	EXPECT_TRUE(num);
	EXPECT_TRUE(num.isValue());
	EXPECT_EQ(num, 1);

	data::Data str("hi");
	EXPECT_TRUE(str);
	EXPECT_TRUE(str.isValue());
	EXPECT_EQ(str, "hi");

	data::Data f(false);
	EXPECT_FALSE(f);
	EXPECT_TRUE(f.isValue());
	EXPECT_EQ(f, false);

	data::Data t(true);
	EXPECT_TRUE(t);
	EXPECT_TRUE(t.isValue());
	EXPECT_EQ(t, true);
}

TEST_F(DataTreeTest, ArrayCanBeListInitialized) {
	data::DataArray arr = { 1, 5, data::DataArray{"silly", 0.5}, true, nullptr };

	EXPECT_EQ(arr[0], 1);
	EXPECT_EQ(5, arr[1]);

	EXPECT_EQ(arr[2], data::DataArray("silly", 0.5));

	EXPECT_EQ(arr[3], true);
	EXPECT_EQ(arr[4], nullptr);
}

TEST_F(DataTreeTest, DictCanBeListInitialized) {
	data::DataDict dict = {
		{ "pizza", 1 },
		{ "sub", data::DataDict{
			{"b", "w"}
		}}
	};

	EXPECT_EQ(dict.size(), 2);

	if (auto pizz = dict.find("pizza"); pizz != dict.end()) {
		EXPECT_EQ(pizz->first, "pizza");
		EXPECT_EQ(pizz->second, 1);
	}
	else {
		ADD_FAILURE() << "'pizza' not found in dict!";
	}
	
	if (auto sub = dict.find("sub"); sub != dict.end()) {
		EXPECT_EQ(sub->first, "sub");
		EXPECT_EQ(sub->second, (data::DataDict{ { "b", "w" } }));
	}
	else {
		ADD_FAILURE() << "'sub' not found in dict!";
	}
}

TEST_F(DataTreeTest, DataCanSquareBracket) {
	data::Data dat{};

	dat[0]["banana"][1] = 5;

	EXPECT_EQ(dat[0]["banana"][1], 5);
	EXPECT_EQ(dat[0]["banana"][0], nullptr);

	dat[1]["banana"][0] = 5;

	EXPECT_EQ(dat[0]["banana"][1], 5);
	EXPECT_EQ(dat[0]["banana"][0], nullptr);
	EXPECT_EQ(dat[1]["banana"][0], 5);
	EXPECT_EQ(dat[1]["banana"][1], nullptr);

	dat[0]["fish"] = true;

	EXPECT_TRUE(dat[0]["fish"]);
}

TEST_F(DataTreeTest, DataThrowsOnBadIndexing) {
	data::Data val = 5;

	EXPECT_THROW(val[0], std::logic_error);
	EXPECT_THROW(val[":)"], std::logic_error);

	EXPECT_THROW(val.at(0), std::logic_error);
	EXPECT_THROW(val.at(":)"), std::logic_error);

	data::Data arr = data::DataArray{ 5, 10 };

	EXPECT_THROW(arr[":("], std::logic_error);

	EXPECT_THROW(arr.at(2), std::out_of_range);
	EXPECT_THROW(arr.at(":("), std::logic_error);

	data::Data dict = data::DataDict{{":(", 10}};

	EXPECT_THROW(dict[0], std::logic_error);

	EXPECT_THROW(dict.at(":)"), std::out_of_range);
	EXPECT_THROW(dict.at(0), std::logic_error);
}

TEST_F(DataTreeTest, DataCanBeAssigned) {
	data::Data a = 5;
	data::Data b = data::DataArray{ 5, 10 };
	data::Data c = data::DataDict{ {":(", 10} };

	a = c;
	c = b;
	b = 5;

	b = { 10, 20 };

	EXPECT_EQ(a, data::DataDict({":(", 10}));
	EXPECT_EQ(b, data::DataArray(10, 20));
	EXPECT_EQ(c, data::DataArray(5, 10));
}
