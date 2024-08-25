#include "string/stringmanip.hpp"

#include <gtest/gtest.h>

using namespace sndx::string;

using str = sv<char>;

TEST(StringManip, Strip) {
	ASSERT_EQ(strip(""), "");
	ASSERT_EQ(strip(" "), "");
	ASSERT_EQ(strip(std::string("  ")), "");
	ASSERT_EQ(strip(" \t "), "");
	ASSERT_EQ(strip(" \r\t\r \t  \r"), "");

	ASSERT_EQ(strip("apple"), "apple");
	ASSERT_EQ(strip(" apple\r"), "apple");
	ASSERT_EQ(strip("app le\r"), "app le");
	ASSERT_EQ(strip("\t a p\r ple\t "), "a p\r ple");

	ASSERT_EQ(strip("apple", "apple"), "");
	ASSERT_EQ(strip("apple", "ale"), "pp");
}

TEST(StringManip, SplitFirst) {
	using pair = std::pair<str, str>;

	ASSERT_EQ(splitFirst("", ','), pair("", ""));
	ASSERT_EQ(splitFirst(",", ','), pair("", ""));
	ASSERT_EQ(splitFirst(" , ", ','), pair("", ""));
	ASSERT_EQ(splitFirst("a, ", ','), pair("a", ""));
	ASSERT_EQ(splitFirst("a , b ", ','), pair("a", "b"));
	ASSERT_EQ(splitFirst("a a = b b ", '='), pair("a a", "b b"));
	ASSERT_EQ(splitFirst("a a = b b = c c\r", '='), pair("a a", "b b = c c"));

	ASSERT_EQ(splitFirst(" apple ", 'w'), pair("apple", ""));
}

TEST(StringManip, SplitStrip) {

	auto t = splitStrip(", ,", ',');

	ASSERT_EQ(t.size(), 3);
	ASSERT_EQ(t[0], "");
	ASSERT_EQ(t[1], "");
	ASSERT_EQ(t[2], "");

	t = splitStrip("apple, banana, orange", ',');

	ASSERT_EQ(t.size(), 3);
	ASSERT_EQ(t[0], "apple");
	ASSERT_EQ(t[1], "banana");
	ASSERT_EQ(t[2], "orange");

	t = splitStrip(" ", ',');

	ASSERT_TRUE(t.empty());

	t = splitStrip("apple", ',');

	ASSERT_EQ(t.size(), 1);
	ASSERT_EQ(t[0], "apple");
}

TEST(StringManip, ParseEscaped) {

	ASSERT_EQ(parseEscaped(""), "");
	ASSERT_EQ(parseEscaped("apple"), "apple");

	ASSERT_EQ(parseEscaped("\tap\vple\r"), "\tap\vple\r");

	ASSERT_EQ(parseEscaped("\\tap\\vple\\r"), "\tap\vple\r");

	ASSERT_EQ(parseEscaped("a\\pple"), "a\\pple");

	ASSERT_EQ(parseEscaped("\\\"apple\\\""), "\"apple\"");
	ASSERT_EQ(parseEscaped("\\\\\"apple\\\\\""), "\\\"apple\\\"");
	ASSERT_EQ(parseEscaped("\\\\\\\"apple\\\\\\\""), "\\\"apple\\\"");
}

TEST(StringManip, DecodeUTF8) {
	auto t = decodeUTF8("");

	ASSERT_TRUE(t.has_value());
	ASSERT_EQ(*t, U""_codepoint);

	t = decodeUTF8("banana");

	ASSERT_TRUE(t.has_value());
	ASSERT_EQ(*t, U"banana"_codepoint);

	// 0b11011111
	char invalid[] = { '\xdf', '\xff', '\0' };
	t = decodeUTF8(invalid);
	
	ASSERT_FALSE(t.has_value());

	char extendedAscii[] = { '\xe4', '\0' };
	t = decodeUTF8(extendedAscii);

	ASSERT_TRUE(t.has_value());
	ASSERT_EQ(*t, U"\xe4"_codepoint);

	t = decodeUTF8("😎 1337 ツ σ");

	ASSERT_TRUE(t.has_value());
	ASSERT_EQ(*t, U"😎 1337 ツ σ"_codepoint);
}

TEST(StringManip, EncodeUTF8) {
	auto t = encodeUTF8(U""_codepoint);

	ASSERT_TRUE(t.has_value());
	ASSERT_EQ(*t, "");

	t = encodeUTF8(U"banana"_codepoint);

	ASSERT_TRUE(t.has_value());
	ASSERT_EQ(*t, "banana");

	// 0b11011111
	Codepoint invalid[] = { 0x110000, '\0' };
	t = encodeUTF8(invalid);

	ASSERT_FALSE(t.has_value());

	t = encodeUTF8(U"😎 1337 ツ σ"_codepoint);

	ASSERT_TRUE(t.has_value());
	ASSERT_EQ(*t, "😎 1337 ツ σ");
}