#pragma once

#include <array>
#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sndx::container {
	namespace detail {

		template <uint8_t IndexCount, class DataT = void>
		struct PrefixTrieNode {
			std::array<uint32_t, IndexCount> children{ 0 };
			std::optional<DataT> data{};

			[[nodiscard]]
			constexpr bool atData() const noexcept {
				return data.has_value();
			}

			constexpr bool erase() {
				auto had = data.has_value();
				data.reset();
				return had;
			}
		};

		template <uint8_t IndexCount>
		struct PrefixTrieNode<IndexCount, void> {
			std::array<uint32_t, IndexCount> children{ 0 };
			bool data{false};

			[[nodiscard]]
			constexpr bool atData() const noexcept {
				return data;
			}

			constexpr bool erase() {
				return std::exchange(data, false);
			}
		};
	}

	// IndexMapping should return -1 (or any value >= IndexCount) for unmapped characters.
	// A DataT of `void` only tracks containment and no other data.
	template <auto IndexMapping, uint8_t IndexCount, class DataT = void>
	class PrefixTrie {
	public:
		static_assert(std::invocable<decltype(IndexMapping), char>);

		using Node = detail::PrefixTrieNode<IndexCount, DataT>;

		[[nodiscard]]
		static constexpr bool isValidString(std::string_view str) {
			for (auto ch : str) {
				if (IndexMapping(ch) >= 255) {
					return false;
				}
			}
			return true;
		}

	private:
		// using a flat graph improves cache locality, memory usage (given DataT is small)
		std::vector<Node> nodes{ Node{} };
		std::vector<uint32_t> freelist{};

		[[nodiscard]]
		constexpr Node* root() {
			return std::addressof(nodes.front());
		}

		[[nodiscard]]
		constexpr const Node* root() const {
			return std::addressof(nodes.front());
		}

		[[nodiscard]]
		constexpr Node* getNode(std::string_view str) {
			Node* cur = root();
			for (auto chr : str) {
				auto idx = IndexMapping(chr);
				if (idx >= IndexCount) {
					return nullptr;
				}

				if (auto ptr = cur->children[idx]; ptr != 0) {
					cur = std::addressof(nodes[ptr]);
				}
				else {
					return nullptr;
				}
			}
			return cur;
		}

		[[nodiscard]]
		constexpr const Node* getNode(std::string_view str) const {
			const Node* cur = root();
			for (auto chr : str) {
				auto idx = IndexMapping(chr);
				if (idx >= IndexCount) {
					return nullptr;
				}

				if (auto ptr = cur->children[idx]; ptr != 0) {
					cur = std::addressof(nodes[ptr]);
				}
				else {
					return nullptr;
				}
			}
			return cur;
		}

		[[nodiscard]]
		constexpr static bool nodeHasChildren(const Node& node) {
			for (const auto& child : node.children) {
				if (child != 0) {
					return true;
				}
			}
			return false;
		}

		template <auto InverseIndexMap> [[nodiscard]]
		constexpr static void collectPrefix(const std::string& prefix, const std::vector<Node>& nodes, const Node& node, std::vector<std::string>& out) {
			if (node.atData()) {
				out.emplace_back(prefix);
			}

			for (uint16_t i = 0; i < node.children.size(); ++i) {
				if (auto child = node.children[i]; child != 0) {
					auto chr = InverseIndexMap(i);
					collectPrefix<InverseIndexMap>(prefix + chr, nodes, nodes[child], out);
				}
			}
		}

		// assumes `str` is present!
		constexpr void prune(std::string_view str) {
			std::vector<uint32_t> path{};
			path.reserve(str.size() + 1);

			uint32_t cur = 0;
			for (auto& chr : str) {
				path.emplace_back(cur);

				auto idx = IndexMapping(chr);
				cur = nodes[cur].children[idx];
			}
			path.emplace_back(cur);

			for (size_t i = 0; i < path.size() - 1; ++i) {
				auto reversed = path.size() - i - 1;
				const auto& node = nodes[path[reversed]];

				if (node.atData() || nodeHasChildren(node)) {
					return;
				}

				freelist.emplace_back(path[reversed]);

				auto& parent = nodes[path[reversed - 1]];
				auto idx = IndexMapping(str[reversed - 1]);

				parent.children[idx] = 0;
			}
		}

		constexpr void kill(Node& start) {
			start.erase();
			for (auto& idx : start.children) {
				if (idx != 0) {
					kill(nodes[idx]);
					freelist.emplace_back(std::exchange(idx, 0));
				}
			}
		}

	public:
		template <class... Args>
		constexpr bool insert(std::string_view str, Args&&... args) {
			static_assert(
				!std::is_same_v<void, DataT> ||
				(sizeof...(args) == 0 && std::is_same_v<void, DataT>)
			);

			uint32_t cur = 0;
			for (auto chr : str) {
				auto idx = IndexMapping(chr);
				if (idx >= IndexCount) {
					return false;
				}

				uint32_t next = nodes[cur].children[idx];
				if (next == 0) {
					if (freelist.empty()) {
						next = nodes.size();
						nodes.emplace_back();
					}
					else {
						next = freelist.back();
						freelist.pop_back();
					}
					nodes[cur].children[idx] = next;
				}
				cur = next;
			}

			if constexpr (std::is_same_v<DataT, void>) {
				nodes[cur].data = true;
			}
			else {
				nodes[cur].data.emplace(std::forward<Args>(args)...);
			}
			return true;
		}

		constexpr bool erase(std::string_view str) {
			if (str.empty()) {
				return nodes.front().erase();
			}
			if (!isValidString(str)) {
				return false;
			}

			if (auto node = getNode(str)) {
				if (!node->erase()) {
					// guarenteed not a terminal node, can exit early
					return false;
				}

				if (nodeHasChildren(*node)) {
					// can't prune
					return true;
				}

				prune(str);
				return true;
			}

			return false;
		}

		constexpr bool prefixErase(std::string_view str) {
			if (auto node = getNode(str)) {
				kill(*node);
				return true;
			}

			return false;
		}

		[[nodiscard]]
		constexpr auto get(std::string_view str) {
			if (auto nd = getNode(str)) {
				if constexpr (std::is_same_v<DataT, void>) {
					return nd->data;
				}
				else {
					if (nd->data) {
						return std::addressof(*nd->data);
					}
				}
			}

			if constexpr (std::is_same_v<DataT, void>) {
				return false;
			}
			else {
				return (DataT*)nullptr;
			}
		}

		[[nodiscard]]
		constexpr auto get(std::string_view str) const {
			if (auto nd = getNode(str)) {
				if constexpr (std::is_same_v<DataT, void>) {
					return nd->data;
				}
				else {
					if (nd->data) {
						return (const DataT*)std::addressof(nd->data);
					}
				}
			}

			if constexpr (std::is_same_v<DataT, void>) {
				return false;
			}
			else {
				return (const DataT*)nullptr;
			}
		}

		template <auto InverseIndexMap> [[nodiscard]]
		constexpr std::vector<std::string> collect(std::string_view prefix) const {
			std::vector<std::string> out{};
			if (auto node = getNode(prefix)) {
				collectPrefix<InverseIndexMap>(std::string(prefix), nodes, *node, out);
			}

			return out;
		}

		[[nodiscard]]
		constexpr bool contains(std::string_view str) const {
			return bool(get(str));
		}

		[[nodiscard]]
		constexpr bool hasPrefix(std::string_view str) const {
			return bool(getNode(str));
		}

		constexpr void reserve(uint32_t size) {
			nodes.reserve(size);
		}
	};
}