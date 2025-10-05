#include "data/serialize.hpp"

#include <gtest/gtest.h>

using namespace sndx;

TEST(Serialize, serializeByteWorks)
{
	auto out = serialize(uint8_t(123));

	ASSERT_EQ(out.size(), 1);
	EXPECT_EQ(out[0], 123);
}

TEST(Serialize, serializeu32Works)
{
	auto out = serialize(uint32_t(0xff112233));

	ASSERT_EQ(out.size(), 4);
	EXPECT_EQ(out[0], 0x33);
	EXPECT_EQ(out[1], 0x22);
	EXPECT_EQ(out[2], 0x11);
	EXPECT_EQ(out[3], 0xff);
}

TEST(Serialize, serializeArrayWorks)
{
	std::array<int16_t, 4> buf{
		0x1122, 0x3344, 0x5566, 0x7788
	};

	auto out = serialize(buf);

	ASSERT_EQ(out.size(), 4 * 2);
	EXPECT_EQ(out[0], 0x22);
	EXPECT_EQ(out[1], 0x11);
	EXPECT_EQ(out[2], 0x44);
	EXPECT_EQ(out[3], 0x33);
	EXPECT_EQ(out[4], 0x66);
	EXPECT_EQ(out[5], 0x55);
	EXPECT_EQ(out[6], 0x88);
	EXPECT_EQ(out[7], 0x77);
}

TEST(Deserialize, deserializeByteWorks)
{
	std::vector<uint8_t> in{123};

	uint8_t out;
	deserialize(out, in);

	EXPECT_EQ(out, 123);
}

TEST(Deserialize, deserializeu32Works)
{
	std::vector<uint8_t> in{ 0x33, 0x22, 0x11, 0xff };

	uint32_t out;
	deserialize(out, in);

	EXPECT_EQ(out, 0xff112233);
}

TEST(Deserialize, deserializeArrayWorks)
{
	std::vector<uint8_t> in{ 
		0x22, 0x11, 0x44, 0x33, 0x66, 0x55, 0x88, 0x77
	};


	std::array<int16_t, 4> buf;

	deserialize(buf, in);

	EXPECT_EQ(buf[0], 0x1122);
	EXPECT_EQ(buf[1], 0x3344);
	EXPECT_EQ(buf[2], 0x5566);
	EXPECT_EQ(buf[3], 0x7788);
}