#pragma once

#include <filesystem>
#include <fstream>
#include <variant>
#include <unordered_map>
#include <sstream>
#include <functional>

#include "stringmanip.hpp"

namespace sndx {
	template <typename CharT = char>
	struct TreeFileLayout {
		const CharT* dataDelim;
		const CharT* endDataDelim;
		const CharT* beginDirDelim;
		const CharT* endDirDelim;
		const CharT* strip;
		const CharT* idStrip;

		//used for saving only
		const CharT* spacer;
		const CharT* depthSpacer;
		const CharT* decorator;
		bool dataEndOnLast;
		bool dirOnFirst;
	};

	// note this does NOT support JSON arrays (because they're stupid)
	template <typename CharT = char>
	constexpr TreeFileLayout<CharT> LayoutJSON{ ":", ",", "{", "}", " \t\n\"", " {}\n\t\"'", "\n", "\t", "\"", false, true };

	// note this does NOT support sections (because they're stupid)
	template <typename CharT = char>
	constexpr TreeFileLayout<CharT> LayoutINI{ "=", "\n", "[", "]", " \t\n\"", " []\n\t\"'", "", "", "", false, false };

	template <typename CharT = char>
	constexpr TreeFileLayout<CharT> LayoutSNDX{ "=", ",", "{", "}", " {}\t\n\"", " {}\n\t\"'", "\n", "\t", "", false, false };

	template <class dataT = std::string>
	struct DataNode {

		dataT data;

		[[nodiscard]] dataT& get() { return data; }
		[[nodiscard]] const dataT& get() const { return data; }

		template <typename CharT = char>
		void save(std::basic_ostream<CharT>& ostream, const TreeFileLayout<CharT>& layout, int depth = 0) const {
			using StrT = std::basic_string<CharT>;
			ostream << StrT(layout.decorator) << StrT(data) << StrT(layout.decorator);
		}
	};

	template <class dataT = std::string, typename CharT = char>
	struct DirectoryNode {
		using Node = std::variant<DataNode<dataT>, DirectoryNode<dataT, CharT>>;
		using StrT = std::basic_string<CharT>;

		std::unordered_map<StrT, Node> data;

		[[nodiscard]]
		Node* get(const StrT& id) {
			auto it = data.find(id);
			if (it == data.end()) {
				return nullptr;
			}

			return &it->second;
		}

		[[nodiscard]]
		Node* get(sv<CharT> idStr, CharT delim, sv<CharT> strips = " \t") {

			auto [path, rest] = splitFirst(idStr, delim, strips);

			Node* got = get(StrT(path));
			if (got == nullptr) return nullptr;
			
			if (rest == "") {
				return got;
			}

			if (!std::holds_alternative<DirectoryNode<dataT>>(*got)) {
				return nullptr;
			}
			else {
				return std::get<DirectoryNode<dataT>>(*got).get(rest, delim, strips);
			}
		}

		DataNode<dataT>* add(const dataT& val, sv<CharT> idStr, CharT delim, sv<CharT> strips = " \t") {
			auto [path, rest] = splitFirst(idStr, delim, strips);

			Node* got = get(StrT(path));
			if (got == nullptr) {
				if (rest == "") {
					auto it = data.emplace(StrT(path), DataNode{val});
					if (it.second) {
						return &std::get<DataNode<dataT>>(it.first->second);
					}

					return nullptr; //insertion failed!
				}
				
				auto it = data.emplace(StrT(path), DirectoryNode<dataT, CharT>{});
				if (it.second) {
					return std::get<DirectoryNode<dataT, CharT>>(it.first->second).add(val, rest, delim, strips);
				}

				return nullptr; //insertion failed!
			}
			
			if (rest == "") {
				if (!std::holds_alternative<DataNode<dataT>>(*got)) [[unlikely]] {
					return nullptr;
				}

				std::get<DataNode<dataT>>(*got).data = val;
				return &std::get<DataNode<dataT>>(*got);
			}

			if (!std::holds_alternative<DirectoryNode<dataT, CharT>>(*got)) [[unlikely]] {
				return nullptr;
			}

			return std::get<DirectoryNode<dataT, CharT>>(*got).add(val, rest, delim, strips);
		}

		// returns removed data if the removed was a dataNode, returns empty otherwise
		std::optional<dataT> remove(sv<CharT> idStr, CharT delim, sv<CharT> strips = " \t") {
			auto [path, rest] = splitFirst(idStr, delim, strips);

			Node* got = get(StrT(path));
			if (got == nullptr) return {};

			if (rest == "") {
				if (std::holds_alternative<DataNode<dataT>>(*got)) {
					auto out = std::move(std::get<DataNode<dataT>>(*got).data);
					data.erase(StrT(path));
					return out;
				}
				else {
					data.erase(StrT(path));
					return {};
				}
			}

			if (std::holds_alternative<DirectoryNode<dataT, CharT>>(*got)) {
				return std::get<DirectoryNode<dataT, CharT>>(*got).remove(rest, delim, strips);
			}

			return {};
		}

		template <typename T = char>
		void save(std::basic_ostream<CharT>& ostream, const TreeFileLayout<CharT>& layout, int depth = 0) const {
			if (layout.dirOnFirst || depth != 0) ostream << StrT(layout.beginDirDelim) << StrT(layout.spacer);
			
			int i = 0;
			for (const auto& [id, node] : data) {

				for (int d = int(!layout.dirOnFirst); d <= depth; ++d) {
					ostream << StrT(layout.depthSpacer);
				}

				ostream << StrT(layout.decorator) << id << StrT(layout.decorator) << StrT(layout.dataDelim);

				std::visit([&ostream, &layout, depth](auto&& cur) {
					cur.save<CharT>(ostream, layout, depth + 1);
				}, node);

				if (layout.dataEndOnLast || i != data.size() - 1) [[likely]] {
					ostream << StrT(layout.endDataDelim);
				}
				ostream << StrT(layout.spacer);
				
				++i;
			}

			for (int d = int(!layout.dirOnFirst); d < depth; ++d) {
				ostream << StrT(layout.depthSpacer);
			}

			if (layout.dirOnFirst || depth != 0) ostream << StrT(layout.endDirDelim);
		}
	};

	template <class dataT = std::string, typename CharT = char>
	struct DataTree {
		using DataNodeT = DataNode<dataT>;
		using DirNodeT = DirectoryNode<dataT, CharT>;
		using Node = std::variant<DataNodeT, DirNodeT>;

		Node root;

		[[nodiscard]]
		Node* get(sv<CharT> idStr, CharT delim, sv<CharT> strips = " \t") {
			if (idStr == "") [[unlikely]] return &root;

			if (!std::holds_alternative<DirNodeT>(root)) [[unlikely]] {
				return nullptr;
			}

			return std::get<DirNodeT>(root).get(idStr, delim, strips);
		}

		[[nodiscard]]
		dataT* getData(sv<CharT> idStr, CharT delim, sv<CharT> strip = " \t") {
			auto dataNode = get(idStr, delim, strip);
			if (dataNode == nullptr) return nullptr;

			if (!std::holds_alternative<DataNodeT>(*dataNode)) {
				return nullptr;
			}

			return &std::get<DataNodeT>(*dataNode).data;
		}

		[[nodiscard]]
		dataT getOrElse(sv<CharT> idStr, CharT delim, const dataT& onElse, sv<CharT> strip = " \t") {
			auto out = getData(idStr, delim, strip);
			if (out == nullptr) {
				return onElse;
			}

			return *out;
		}

		template <class T> [[nodiscard]]
		T getOrElse(sv<CharT> idStr, CharT delim, const T& onElse, std::function<T(const dataT&)> conversion, sv<CharT> strip = " \t") {
			auto out = getData(idStr, delim, strip);
			if (out == nullptr) {
				return onElse;
			}

			return conversion(*out);
		}

		// adds a dataNode to the structure along with all directories needed to support it
		// returns nullptr on error
		DataNodeT* add(const dataT& data, sv<CharT> idStr, CharT delim, sv<CharT> strips = " \t") {
			if (!std::holds_alternative<DirNodeT>(root)) [[unlikely]] {
				return nullptr;
			}

			return std::get<DirNodeT>(root).add(data, idStr, delim, strips);
		}

		// returns removed data if the removed was a dataNode, returns empty otherwise
		std::optional<dataT> remove(sv<CharT> idStr, CharT delim, sv<CharT> strips = " \t") {
			if (idStr == "") {
				root = DirNodeT{};
				return {};
			}

			if (!std::holds_alternative<DirNodeT>(root)) [[unlikely]] {
				return {};
			}

			return std::get<DirNodeT>(root).remove(idStr, delim, strips);
		}

		void save(std::basic_ostream<CharT>& ostream, const TreeFileLayout<CharT>& layout) const {
			std::visit([&ostream, &layout](auto&& cur) {
				cur.save<CharT>(ostream, layout);
			}, root);

			if (layout.dataEndOnLast) {
				ostream << std::basic_string<CharT>(layout.endDataDelim);
			}
		}

		bool save(const std::filesystem::path& path, const TreeFileLayout<CharT>& layout = LayoutSNDX<CharT>) const {
			std::basic_ofstream<CharT> ofile(path);

			if (ofile.is_open()) {
				save(ofile, layout);
				return true;
			}
			
			return false;
		}
	};

	template <class dataT = std::string, typename CharT = char> [[nodiscard]]
	DirectoryNode<dataT, CharT> parseBranch(sv<CharT> branch, TreeFileLayout<CharT> layout) {
		DirectoryNode<dataT, CharT> out{};

		constexpr auto npos = sv<CharT>::npos;

		size_t cur = 0;

		size_t next = 0;
		while (next != npos) {
			next = branch.find_first_of(layout.dataDelim, cur);

			if (next == npos) return out;

			auto id = strip(branch.substr(cur, next - cur), sv(layout.idStrip));

			cur = next;

			auto pDataEnd = branch.find_first_of(layout.endDataDelim, cur);
			auto pDirBegin = branch.find_first_of(layout.beginDirDelim, cur);

			if (pDataEnd != npos) {
				if (pDirBegin != npos && pDirBegin < pDataEnd) {

					int count = 1;
					auto dirEnd = pDirBegin;
					const auto endDirStr = std::basic_string<CharT>(layout.endDirDelim);
					while (count > 0) {
						dirEnd = branch.find_first_of(endDirStr + layout.beginDirDelim, dirEnd + 1);

						if (dirEnd == npos) break;

						if (std::find(endDirStr.begin(), endDirStr.end(), branch[dirEnd]) != endDirStr.end()) {
							--count;
						}
						else {
							++count;
						}
					}

					if (pDirBegin > dirEnd) return out;

					if (dirEnd != npos) {
						out.data.emplace(id, parseBranch(branch.substr(pDirBegin + 1, dirEnd - pDirBegin - 1), layout));
						cur = dirEnd + 2;
					}
					else {
						if (id != "") out.data.emplace(id, parseBranch(branch.substr(pDirBegin + 1), layout));
						break;
					}
				}
				else {
					out.data.emplace(id, DataNode<dataT>{dataT(strip(branch.substr(cur + 1, pDataEnd - cur - 1), sv(layout.strip)))});
					cur = pDataEnd + 1;
				}
			}
			else {
				if (pDirBegin != npos) {
					if (id != "") out.data.emplace(id, parseBranch(branch.substr(pDirBegin + 1), layout));
					break;
				}
				else {
					if (id != "") out.data.emplace(id, DataNode<dataT>{dataT(strip(branch.substr(cur + 1), sv(layout.strip)))});
					break;
				}
			}
		}

		return out;
	}

	template <class dataT = std::string, typename CharT = char> [[nodiscard]]
	DataTree<dataT, CharT> loadDataTree(std::basic_istream<CharT>& in, const TreeFileLayout<CharT>& layout = LayoutSNDX<CharT>) {
		DataTree<dataT, CharT> out{};
		out.root = DataNode<dataT>{};

		std::basic_stringstream<CharT> strm;
		strm << in.rdbuf();
		auto str = strm.str();

		auto inpt = sv<CharT>(str);

		auto firstData = inpt.find_first_of(layout.dataDelim);
		auto firstDir = inpt.find_first_of(layout.beginDirDelim);

		if (firstData == sv<CharT>::npos) [[unlikely]] return out;

		if (firstDir != sv<CharT>::npos && firstDir < firstData) {
			auto lastDir = inpt.find_last_of(layout.endDirDelim);

			if (lastDir == sv<CharT>::npos) {
				inpt = inpt.substr(firstDir + 1);
			}
			else {
				inpt = inpt.substr(firstDir + 1, lastDir - firstDir - 2);
			}
		}


		out.root = parseBranch(strip(inpt, sv<CharT>(layout.strip)), layout);

		return out;
	}

	template <class dataT = std::string, typename CharT = char> [[nodiscard]]
	std::optional<DataTree<dataT, CharT>> loadDataTree(const std::filesystem::path& path, const TreeFileLayout<CharT>& layout = LayoutSNDX<CharT>) {
		std::basic_ifstream<CharT> ifile{ path };

		if (ifile.is_open()) {
			return loadDataTree<dataT, CharT>(ifile, layout);
		}

		return {};
	}
}