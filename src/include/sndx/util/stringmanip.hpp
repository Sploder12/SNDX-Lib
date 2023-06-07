#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace sndx {
	template <typename CharT = char>
	using sv = std::basic_string_view<CharT>;

	template <typename CharT = char>
	using Str = std::basic_string<CharT>;

	template <typename CharT = char> [[nodiscard]]
	constexpr sv<CharT> strip(sv<CharT> str, sv<CharT> strips = " \t") {
		auto first = str.find_first_not_of(strips);
		auto last = str.find_last_not_of(strips);

		if (first == sv<CharT>::npos) [[unlikely]] {
			return str.substr(0, 0);
		}

		return str.substr(first, last - first + 1);
	}

	template <typename CharT = char> [[nodiscard]]
	inline std::vector<sv<CharT>> splitStrip(sv<CharT> str, CharT delim, sv<CharT> strips = " \t") {
		std::vector<sv<CharT>> out{};
		if (str == "") return out;

		out.reserve(str.size() / 2);

		auto in = strip(str, strips);

		size_t split = 0;
		while (split != sv<CharT>::npos) {
			auto next = in.find_first_of(delim, split);

			if (next != sv::npos) {
				out.emplace_back(strip(in.substr(split, next - split), strips));
			}
			else {
				out.emplace_back(strip(in.substr(split), strips));
				break;
			}
			split = next + 1;
		}

		return out;
	}

	template <typename CharT = char> [[nodiscard]]
	constexpr std::pair<sv<CharT>, sv<CharT>> splitFirst(sv<CharT> str, CharT delim, sv<CharT> strips = " \t") {
		auto end = str.find_first_of(delim);
		sv<CharT> first;
		sv<CharT> second;
		if (end == sv<CharT>::npos) {
			first = str;
			second = "";
		}
		else {
			first = str.substr(0, end);
			second = str.substr(end + 1);
		}
		first = strip(first, strips);
		second = strip(second, strips);

		return { first, second };
	}

	template <typename CharT = char> [[nodiscard]]
	inline Str<CharT> parseEscaped(sv<CharT> str) {
		Str<CharT> out{};
		out.reserve(str.size());

		for (int i = 0; i < str.size(); ++i) {
			CharT cur = str[i];
			if (cur == '\\') {
				++i;
				if (i >= str.size()) {
					out += cur;
					break;
				}

				CharT esc = str[i];
				switch (esc) {
				default:
					out += cur;
					// fallthrough
				case '\'':
				case '"':
				case '?':
				case '\\':
					out += esc;
					break;
				case 'a':
					out += '\a';
					break;
				case 'b':
					out += '\b';
					break;
				case 'f':
					out += '\f';
					break;
				case 'n':
					out += '\n';
					break;
				case 'r':
					out += '\r';
					break;
				case 't':
					out += '\t';
					break;
				case 'v':
					out += '\v';
					break;
				}
			}
			else {
				out += cur;
			}
		}
		return out;
	}
}
