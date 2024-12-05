#include "utility/stream.hpp"

#include <gtest/gtest.h>

#include <array>

using namespace sndx::utility;

std::array<char, 40> readArr = {
	'l','o','r','e','m',
	'i','p','s','u','m',
	'd','o','l','o','r',
	's','e','t','a','m',
	'e','t','c','o','n',
	's','e','c','t','o',
	'r','a','d','i','p',
	's','t','i','n','g'
};

TEST(MemStream, Read) {
	std::array<char, 40> outArr{ 0 };

	MemoryStream buf(reinterpret_cast<uint8_t*>(readArr.data()), readArr.size());

	buf.read(outArr.data(), outArr.size());

	ASSERT_EQ(readArr, outArr);

	buf.seekg(5);
	buf.read(outArr.data(), outArr.size() - 5);

	for (size_t i = 0; i < outArr.size() - 5; ++i) {
		EXPECT_EQ(outArr[i], readArr[i + 5]);
	}

	buf.seekg(0);
	buf.read(outArr.data(), outArr.size() / 2);

	for (size_t i = 0; i < outArr.size() / 2; ++i) {
		EXPECT_EQ(outArr[i], readArr[i]);
	}

	buf.read(outArr.data(), outArr.size() / 2 + 1);
	EXPECT_TRUE(buf.eof());

	for (size_t i = 0; i < outArr.size() / 2; ++i) {
		EXPECT_EQ(outArr[i], readArr[i + outArr.size() / 2]);
	}
}

TEST(MemStream, Write) {
	std::array<char, 40> outArr{ 0 };

	MemoryStream buf(reinterpret_cast<uint8_t*>(outArr.data()), outArr.size());

	buf.write(readArr.data(), readArr.size());

	ASSERT_EQ(outArr, readArr);

	buf.seekp(0);
	outArr = { 0 };
	buf.write(readArr.data(), readArr.size());

	ASSERT_EQ(outArr, readArr);

	buf.write(readArr.data(), 1);
	ASSERT_TRUE(buf.fail());
	buf.clear();

	buf.seekp(5);
	buf.write(readArr.data(), 5);

	for (size_t i = 0; i < 5; ++i) {
		EXPECT_EQ(outArr[i + 5], readArr[i]);
	}

	buf.seekp(5, std::ios::cur);
	buf.write(readArr.data(), 5);

	for (size_t i = 0; i < 5; ++i) {
		EXPECT_EQ(outArr[i + 15], readArr[i]);
	}
}

TEST(MemStream, ReadWrite) {
	std::array<char, 40> arr{ 0 };

	MemoryStream buf(reinterpret_cast<uint8_t*>(arr.data()), arr.size());

	buf.write(readArr.data(), readArr.size() / 2);

	for (size_t i = 0; i < arr.size() / 2; ++i) {
		EXPECT_EQ(arr[i], readArr[i]);
	}

	buf.read(arr.data() + 1, arr.size() / 2);

	for (size_t i = 0; i < arr.size() / 2; ++i) {
		EXPECT_EQ(arr[i + 1], readArr[i]);
	}

	buf.write(readArr.data(), readArr.size() / 2);
	for (size_t i = 0; i < arr.size() / 2; ++i) {
		EXPECT_EQ(arr[i + arr.size() / 2], readArr[i]);
	}
}