#include "string/stringmanip.hpp"

#include <gtest/gtest.h>

using namespace sndx::string;

using str = sv<char>;

TEST(StringManip, Strip) {
	EXPECT_EQ(strip(""), "");
	EXPECT_EQ(strip(" "), "");
	EXPECT_EQ(strip(std::string("  ")), "");
	EXPECT_EQ(strip(" \t "), "");
	EXPECT_EQ(strip(" \r\t\r \t  \r"), "");

	EXPECT_EQ(strip("apple"), "apple");
	EXPECT_EQ(strip(" apple\r"), "apple");
	EXPECT_EQ(strip("app le\r"), "app le");
	EXPECT_EQ(strip("\t a p\r ple\t "), "a p\r ple");

	EXPECT_EQ(strip("apple", "apple"), "");
	EXPECT_EQ(strip("apple", "ale"), "pp");
}

TEST(StringManip, SplitFirst) {
	using pair = std::pair<str, str>;

	EXPECT_EQ(splitFirst("", ','), pair("", ""));
	EXPECT_EQ(splitFirst(",", ','), pair("", ""));
	EXPECT_EQ(splitFirst(" , ", ','), pair("", ""));
	EXPECT_EQ(splitFirst("a, ", ','), pair("a", ""));
	EXPECT_EQ(splitFirst("a , b ", ','), pair("a", "b"));
	EXPECT_EQ(splitFirst("a a = b b ", '='), pair("a a", "b b"));
	EXPECT_EQ(splitFirst("a a = b b = c c\r", '='), pair("a a", "b b = c c"));

	EXPECT_EQ(splitFirst(" apple ", 'w'), pair("apple", ""));
}

TEST(StringManip, SplitStrip) {

	auto t = splitStrip(", ,", ',');

	EXPECT_EQ(t.size(), 3);
	EXPECT_EQ(t[0], "");
	EXPECT_EQ(t[1], "");
	EXPECT_EQ(t[2], "");

	t = splitStrip("apple, banana, orange", ',');

	EXPECT_EQ(t.size(), 3);
	EXPECT_EQ(t[0], "apple");
	EXPECT_EQ(t[1], "banana");
	EXPECT_EQ(t[2], "orange");

	t = splitStrip(" ", ',');

	EXPECT_TRUE(t.empty());

	t = splitStrip("apple", ',');

	EXPECT_EQ(t.size(), 1);
	EXPECT_EQ(t[0], "apple");
}

TEST(StringManip, ParseEscaped) {

	EXPECT_EQ(parseEscaped(""), "");
	EXPECT_EQ(parseEscaped("apple"), "apple");

	EXPECT_EQ(parseEscaped("\tap\vple\r"), "\tap\vple\r");

	EXPECT_EQ(parseEscaped("\\tap\\vple\\r"), "\tap\vple\r");

	EXPECT_EQ(parseEscaped("a\\pple"), "a\\pple");

	EXPECT_EQ(parseEscaped("\\\"apple\\\""), "\"apple\"");
	EXPECT_EQ(parseEscaped("\\\\\"apple\\\\\""), "\\\"apple\\\"");
	EXPECT_EQ(parseEscaped("\\\\\\\"apple\\\\\\\""), "\\\"apple\\\"");
}

TEST(StringManip, DecodeUTF8) {
	auto t = decodeUTF8("");

	EXPECT_TRUE(t.has_value());
	EXPECT_EQ(*t, U""_codepoint);

	t = decodeUTF8("banana");

	EXPECT_TRUE(t.has_value());
	EXPECT_EQ(*t, U"banana"_codepoint);

	// 0b11011111
	char invalid[] = { '\xdf', '\xff', '\0' };
	t = decodeUTF8(invalid);
	
	EXPECT_FALSE(t.has_value());

	char extendedAscii[] = { '\xe4', '\0' };
	t = decodeUTF8(extendedAscii);

	EXPECT_TRUE(t.has_value());
	EXPECT_EQ(*t, U"\xe4"_codepoint);

	t = decodeUTF8("😎 1337 ツ σ");

	EXPECT_TRUE(t.has_value());
	EXPECT_EQ(*t, U"😎 1337 ツ σ"_codepoint);
}

TEST(StringManip, EncodeUTF8) {
	auto t = encodeUTF8(U""_codepoint);

	EXPECT_TRUE(t.has_value());
	EXPECT_EQ(*t, "");

	t = encodeUTF8(U"banana"_codepoint);

	EXPECT_TRUE(t.has_value());
	EXPECT_EQ(*t, "banana");

	// 0b11011111
	Codepoint invalid[] = { 0x110000, '\0' };
	t = encodeUTF8(invalid);

	EXPECT_FALSE(t.has_value());

	t = encodeUTF8(U"😎 1337 ツ σ"_codepoint);

	EXPECT_TRUE(t.has_value());
	EXPECT_EQ(*t, "😎 1337 ツ σ");
}