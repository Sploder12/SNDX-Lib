#include "container/prefix_trie.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cctype>
#include <cstdint>

using namespace sndx::container;

namespace {
	[[nodiscard]]
	constexpr uint8_t caseInsensitiveIndexer(char chr) {
		if (chr >= 'A' && chr <= 'Z') {
			chr += 'a' - 'A';
		}

		if (chr >= 'a' && chr <= 'z') {
			return chr - 'a';
		}
		return -1;
	}

	[[nodiscard]]
	constexpr char inverseCaseInsensitiveIndexer(size_t idx) {
		return char(idx + 'a');
	}

	template <class T = void>
	using InsensitiveTrie = PrefixTrie<caseInsensitiveIndexer, 'z' - 'a' + 1, T>;
}

TEST(PrefixTrie, ValidityIsChecked) {
	InsensitiveTrie trie{};

	EXPECT_TRUE(trie.isValidString(""));
	EXPECT_TRUE(trie.isValidString("apples"));
	EXPECT_TRUE(trie.isValidString("Zombies"));
	EXPECT_TRUE(trie.isValidString("zoo"));
	EXPECT_TRUE(trie.isValidString("Aardvark"));

	EXPECT_FALSE(trie.isValidString("the quick brown fox jumped over the lazy dog"));
	EXPECT_FALSE(trie.isValidString("?\?!"));
	EXPECT_FALSE(trie.isValidString(" </3"));
	EXPECT_FALSE(trie.isValidString("and;"));
	EXPECT_FALSE(trie.isValidString("true^false"));
}

TEST(PrefixTrie, InsertionWorks) {
	InsensitiveTrie trie{};

	EXPECT_TRUE(trie.insert(""));
	EXPECT_TRUE(trie.insert("apples"));

	EXPECT_FALSE(trie.get("bananas"));
	EXPECT_TRUE(trie.get(""));
	EXPECT_TRUE(trie.get("apples"));
}

TEST(PrefixTrie, BasicEraseWorks) {
	InsensitiveTrie trie{};

	EXPECT_TRUE(trie.insert(""));
	EXPECT_TRUE(trie.insert("app"));
	EXPECT_TRUE(trie.insert("apples"));
	ASSERT_TRUE(trie.get("app"));

	EXPECT_TRUE(trie.erase("app"));

	EXPECT_FALSE(trie.get("app"));
	EXPECT_TRUE(trie.get(""));
	EXPECT_TRUE(trie.get("apples"));
}

TEST(PrefixTrie, PruningEraseWorks) {
	InsensitiveTrie trie{};

	EXPECT_TRUE(trie.insert(""));
	EXPECT_TRUE(trie.insert("app"));
	EXPECT_TRUE(trie.insert("apples"));
	ASSERT_TRUE(trie.get("apples"));

	EXPECT_TRUE(trie.erase("apples"));

	EXPECT_TRUE(trie.get("app"));
	EXPECT_TRUE(trie.get(""));
	EXPECT_FALSE(trie.get("apples"));

	EXPECT_TRUE(trie.erase(""));

	EXPECT_FALSE(trie.get(""));
	EXPECT_TRUE(trie.get("app"));

	EXPECT_TRUE(trie.erase("app"));
	EXPECT_FALSE(trie.get("app"));
}

TEST(PrefixTrie, PrefixEraseWorks) {
	InsensitiveTrie trie{};

	EXPECT_TRUE(trie.insert(""));
	EXPECT_TRUE(trie.insert("app"));
	EXPECT_TRUE(trie.insert("apples"));

	EXPECT_TRUE(trie.prefixErase("app"));

	EXPECT_FALSE(trie.get("app"));
	EXPECT_TRUE(trie.get(""));
	EXPECT_FALSE(trie.get("apples"));
}

TEST(PrefixTrie, CollectWorks) {
	InsensitiveTrie trie{};

	EXPECT_TRUE(trie.insert(""));
	EXPECT_TRUE(trie.insert("app"));
	EXPECT_TRUE(trie.insert("Bananas"));
	EXPECT_TRUE(trie.insert("apples"));

	auto aps = trie.collect<inverseCaseInsensitiveIndexer>("ap");
	EXPECT_THAT(aps, ::testing::UnorderedElementsAre("app", "apples"));
}

TEST(PrefixTrie, CanContainValues) {
	InsensitiveTrie<int> trie{};

	EXPECT_TRUE(trie.insert("", 257));
	EXPECT_TRUE(trie.insert("bananas", -37));

	EXPECT_FALSE(trie.get("apples"));

	auto n = trie.get("");
	ASSERT_TRUE(n);
	EXPECT_EQ(*n, 257);

	auto b = trie.get("bananas");
	ASSERT_TRUE(b);
	EXPECT_EQ(*b, -37);
}

TEST(PrefixTrie, PreventsBadInsertion) {
	InsensitiveTrie trie{};

	EXPECT_FALSE(trie.insert(" "));
	EXPECT_FALSE(trie.insert("apples!"));

	EXPECT_FALSE(trie.get(""));
	EXPECT_FALSE(trie.get(" "));
	EXPECT_FALSE(trie.get("apples!"));
}