#pragma once

#include <filesystem>
#include <fstream>
#include <variant>
#include <unordered_map>
#include <sstream>

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
	constexpr TreeFileLayout<CharT> LayoutJSON{ ":", ",", "{", "}", " {}\t\n\"", " {}\n\t\"'", "\n", "\t", "\"", false, true };

	// note this does NOT support sections (because they're stupid)
	template <typename CharT = char>
	constexpr TreeFileLayout<CharT> LayoutINI{ "=", "\n", "[", "]", " \t\n\"", " []\n\t\"'", "", "", "", false, false };

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
		using Node = std::variant<DataNode<dataT>, DirectoryNode<dataT>>;
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
		Node* get(std::basic_string_view<CharT> idStr, CharT delim, std::basic_string_view<CharT> strip = " \t") {
			auto path = splitStrip(idStr, delim, strip);

			if (path.empty()) [[unlikely]] return nullptr;

			Node* cur = &root;

			for (const auto& id : path) {
				if (!std::holds_alternative<DirNodeT>(*cur)) {
					return nullptr;
				}
				else {
					auto dat = std::get<DirNodeT>(*cur).get(std::basic_string<CharT>(id));
					if (dat != nullptr) {
						cur = dat;
					}
					else {
						return nullptr;
					}
				}
			}

			return cur;
		}

		[[nodiscard]]
		dataT* getData(std::basic_string_view<CharT> idStr, CharT delim, std::basic_string_view<CharT> strip = " \t") {
			auto dataNode = get(idStr, delim, strip);
			if (dataNode == nullptr) return nullptr;

			if (!std::holds_alternative<DataNodeT>(*dataNode)) {
				return nullptr;
			}

			return &std::get<DataNodeT>(*dataNode).data;
		}

		[[nodiscard]]
		dataT getOrElse(std::basic_string_view<CharT> idStr, CharT delim, const dataT& onElse, std::basic_string_view<CharT> strip = " \t") {
			auto out = getData(idStr, delim, strip);
			if (out == nullptr) {
				return onElse;
			}
			else {
				return *out;
			}
		}

		void save(std::basic_ostream<CharT>& ostream, const TreeFileLayout<CharT>& layout) const {
			std::visit([&ostream, &layout](auto&& cur) {
				cur.save<CharT>(ostream, layout);
			}, root);

			if (layout.dataEndOnLast) {
				ostream << std::basic_string<CharT>(layout.endDataDelim);
			}
		}

		void save(std::filesystem::path path, const TreeFileLayout<CharT>& layout = LayoutJSON<CharT>) const {
			std::basic_ofstream<CharT> ofile(path);

			if (ofile.is_open()) {
				save(ofile, layout);
			}
			else {
				throw std::runtime_error("Could not open file " + path.filename().string());
			}
		}
	};

	template <class dataT = std::string, typename CharT = char> [[nodiscard]]
	DirectoryNode<dataT, CharT> parseBranch(std::basic_string_view<CharT> branch, TreeFileLayout<CharT> layout) {
		DirectoryNode<dataT, CharT> out{};

		using sv = std::basic_string_view<CharT>;
		auto npos = std::basic_string_view<CharT>::npos;

		size_t cur = 0;

		size_t next = 0;
		while (next != npos) {
			next = branch.find_first_of(layout.dataDelim, cur);
			auto id = strip(branch.substr(cur, next - cur), sv(layout.idStrip));

			cur = next;

			auto pDataEnd = branch.find_first_of(layout.endDataDelim, cur);
			auto pDirBegin = branch.find_first_of(layout.beginDirDelim, cur);

			if (pDataEnd != npos) {
				if (pDirBegin != npos && pDirBegin < pDataEnd) {

					auto pDirEnd = branch.find_last_of(layout.endDirDelim, pDirBegin);
					if (pDirEnd != npos) {
						out.data.emplace(id, parseBranch(branch.substr(pDirBegin + 1, pDirEnd - pDirBegin - 1), layout));
						cur = pDirEnd + 1;
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
	DataTree<dataT, CharT> loadDataTree(std::basic_istream<CharT>& in, const TreeFileLayout<CharT>& layout = LayoutJSON<CharT>) {
		DataTree<dataT, CharT> out{};
		out.root = DataNode<dataT>{};

		std::basic_stringstream<CharT> strm;
		strm << in.rdbuf();
		auto str = strm.str();

		auto inpt = std::basic_string_view<CharT>(str);

		auto firstData = inpt.find_first_of(layout.dataDelim);
		auto firstDir = inpt.find_first_of(layout.beginDirDelim);

		if (firstData == std::basic_string_view<CharT>::npos) [[unlikely]] return out;

		if (firstDir != std::basic_string_view<CharT>::npos && firstDir < firstData) {
			auto lastDir = inpt.find_last_of(layout.endDirDelim);

			if (lastDir == std::basic_string_view<CharT>::npos) {
				inpt = inpt.substr(firstDir + 1);
			}
			else {
				inpt = inpt.substr(firstDir + 1, lastDir - firstDir - 2);
			}
		}


		out.root = parseBranch(strip(inpt, std::basic_string_view<CharT>(layout.strip)), layout);

		return out;
	}

	template <class dataT = std::string, typename CharT = char> [[nodiscard]]
	DataTree<dataT, CharT> loadDataTree(std::filesystem::path path, const TreeFileLayout<CharT>& layout = LayoutJSON<CharT>) {
		std::basic_ifstream<CharT> ifile{ path };

		if (ifile.is_open()) {
			return loadDataTree<dataT, CharT>(ifile, layout);
		}
		else {
			throw std::runtime_error("Could not open file " + path.filename().string());
		}
	}
}