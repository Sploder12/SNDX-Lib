#pragma once

#include <string_view>
#include <optional>
#include <sstream>
#include <iomanip>

#include <istream>
#include <fstream>
#include <filesystem>

#include "datatree.hpp"

#include "../util/stringmanip.hpp"

namespace sndx {

	[[nodiscard]]
	inline Primitive parsePrimitive(std::string_view in) {
		if (in.empty()) [[unlikely]] {
			return "";
		}

		if (in.front() == '"' && in.back() == '"') {
			if (in.size() == 1) [[unlikely]]
				return "\"";

			std::stringstream ss;
			ss << in;
			std::string out;
			ss >> std::quoted(out);
			return std::move(out);
		}

		if (in == "true") {
			return true;
		}
		else if (in == "false") {
			return false;
		}

		std::string inStr{ in };

		try {
			if (in.find('.') != std::string_view::npos) {
				return std::stold(inStr);
			}

			return std::stoll(inStr);
		}
		catch (...) {
			return std::move(inStr);
		}
	}

	template <GenericEncodingScheme scheme>
	class GenericDecoder {
	protected:
		static constexpr char strips_cstr[4] = { ' ', scheme.spacer, scheme.depthSpacer, '\0'};
		static constexpr auto strips = std::string_view(strips_cstr);

		[[nodiscard]] // precondition, str.front() is the start of the quoted string
		static size_t find_quote_end(std::string_view str) {
			if (str.empty()) [[unlikely]]
				return -1;
			
			if (str.front() != '"') [[unlikely]]
				return 0;

			bool escaped = false;

			for (size_t i = 1; i < str.size(); ++i) {
				char cur = str[i];

				if (cur == '\\') {
					escaped = !escaped;
				}
				else if (cur == '"' && !escaped) {
					return i;
				}
			}

			return -1;
		}

		[[nodiscard]] // precondition, str.front() is the start of the dict
		static size_t find_dict_end(std::string_view str) {
			if (str.empty()) [[unlikely]]
				return -1;

			if (str.front() != scheme.beginDir) [[unlikely]]
				return 0;

			size_t count = 1;

			for (size_t i = 1; i < str.size(); ++i) {
				char cur = str[i];

				if (cur == '"') {
					auto q = find_quote_end(str.substr(i));
					
					if (q == -1) [[unlikely]]
						return -1;

					i += q;

					continue;
				}

				if (cur == scheme.beginDir) {
					++count;
				}
				else if (cur == scheme.endDir) {
					--count;

					if (count == 0) {
						return i;
					}
				}
			}

			return -1;
		}

		[[nodiscard]] // precondition, str.front() is the start of the array
		static size_t find_arr_end(std::string_view str) {
			if (str.empty()) [[unlikely]]
				return -1;

			if (str.front() != scheme.beginArr) [[unlikely]]
				return 0;

			size_t count = 1;

			for (size_t i = 1; i < str.size(); ++i) {
				char cur = str[i];

				if (cur == '"') {
					auto q = find_quote_end(str.substr(i));

					if (q == -1) [[unlikely]]
						return -1;

						i += q;
				}

				if (cur == scheme.beginArr) {
					++count;
				}
				else if (cur == scheme.endArr) {
					--count;

					if (count == 0) {
						return i;
					}
				}
			}

			return -1;
		}
		
	public:
		[[nodiscard]]
		static DataArray parseArr(std::string_view arr) {
			DataArray out{};

			size_t ebegin = 0;

			for (size_t i = 0; i < arr.size(); ++i) {
				char cur = arr[i];

				if (cur == scheme.beginDir) {
					size_t e = find_dict_end(arr.substr(i));
					if (e == -1) [[unlikely]]
						return out;

					out.emplace_back(parseDict(arr.substr(i + 1, e - 2)));
					i += e;
					ebegin = -1;
				}
				else if (cur == scheme.beginArr) {
					size_t e = find_arr_end(arr.substr(i));
					if (e == -1) [[unlikely]]
						return out;

					out.emplace_back(parseArr(arr.substr(i + 1, e - 2)));
					i += e;
					ebegin = -1;
				}
				else if (cur == scheme.primDelim) {
					if (ebegin != -1)
						out.emplace_back(parsePrimitive(strip(arr.substr(ebegin, i - ebegin), strips)));
					
					ebegin = i + 1;
				}
			}

			if (ebegin != -1) {
				out.emplace_back(parsePrimitive(strip(arr.substr(ebegin), strips)));
			}

			return out;
		
			return out;
		}

		[[nodiscard]]
		static DataDict parseDict(std::string_view dict) {
			DataDict out{};

			std::string key = "";
			bool hasKey = false;
			size_t ebegin = 0;

			for (size_t i = 0; i < dict.size(); ++i) {
				char cur = dict[i];

				if (cur == scheme.beginDir && hasKey) {
					size_t e = find_dict_end(dict.substr(i));
					if (e == -1) [[unlikely]]
						return out;

					out.emplace(std::move(key), parseDict(dict.substr(i + 1, e - 2)));
					hasKey = false;
					i += e;
					ebegin = -1;
				}
				else if (cur == scheme.beginArr && hasKey) {
					size_t e = find_arr_end(dict.substr(i));
					if (e == -1) [[unlikely]]
						return out;

					out.emplace(std::move(key), parseArr(dict.substr(i + 1, e - 2)));
					hasKey = false;
					i += e;
					ebegin = -1;
				}
				else if (cur == '"' && !hasKey) {
					size_t e = find_quote_end(dict.substr(i));
					if (e == -1) [[unlikely]]
						return out;

					auto in = dict.substr(i + 1, e - 1);
					std::stringstream ss;
					ss << in;
					ss >> std::quoted(key);
					hasKey = true;

					i += e;
					ebegin = -1;
				}
				else if (cur == scheme.keyDelim) {
					if (!hasKey) [[unlikely]]
						return out;

					ebegin = i + 1;
				}
				else if (cur == scheme.primDelim) {
					if (!hasKey || ebegin == -1)
						continue;

					out.emplace(std::move(key), parsePrimitive(strip(dict.substr(ebegin, i - ebegin), strips)));
					ebegin = -1;
					hasKey = false;
				}
			}

			if (hasKey && ebegin != -1) {
				out.emplace(std::move(key), parsePrimitive(strip(dict.substr(ebegin), strips)));
			}

			return out;
		}

		[[nodiscard]]
		static std::optional<Data> decode(std::string_view in) {
			if (in.empty()) [[unlikely]] {
				return std::nullopt;
			}

			auto stripped = strip(in, strips);
			if (stripped.empty()) [[unlikely]] {
				return std::nullopt;
			}

			// if only std::regex didn't suck

			if (stripped.front() == scheme.beginArr && stripped.back() == scheme.endArr && stripped.size() > 1) {
				auto data = strip(stripped.substr(1, stripped.size() - 2), strips);
				return parseArr(data);
			}
			else if (stripped.front() == scheme.beginDir && stripped.back() == scheme.endDir) {
				auto data = strip(stripped.substr(1, stripped.size() - 2), strips);
				return parseDict(data);
			}
			else {
				// assume it's a dict
				return parseDict(stripped);
			}
		}
	
	};

	using SNDXdecoder = GenericDecoder<GenericEncodingScheme{ '=' }>;
	using JSONdecoder = GenericDecoder<GenericEncodingScheme{ ':' }>;

	template <class Dec> [[nodiscard]]
	std::optional<Data> decodeData(std::basic_istream<char>& in) {
		std::basic_stringstream<char> strm;
		strm << in.rdbuf();
		auto str = strm.str();

		return Dec::decode(str);
	}

	template <class Dec> [[nodiscard]]
	std::optional<Data> decodeData(const std::filesystem::path& path) {
		std::basic_ifstream<char> ifile{ path };

		if (ifile.is_open()) {
			return decodeData<Dec>(ifile);
		}

		return std::nullopt;
	}
}