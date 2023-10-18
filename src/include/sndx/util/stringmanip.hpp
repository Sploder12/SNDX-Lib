#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace sndx {
	template <typename CharT = char>
	using sv = std::basic_string_view<CharT>;

	template <typename CharT = char>
	using Str = std::basic_string<CharT>;

	[[nodiscard]]
	inline constexpr auto strip(sv<char> str, sv<char> strips = " \t") {
		auto first = str.find_first_not_of(strips);
		auto last = str.find_last_not_of(strips);

		if (first == sv<char>::npos) [[unlikely]] {
			return str.substr(0, 0);
		}

		return str.substr(first, last - first + 1);
	}

	[[nodiscard]]
	inline constexpr std::pair<sv<char>, sv<char>> splitFirst(sv<char> str, char delim, sv<char> strips = " \t") {
		auto end = str.find_first_of(delim);
		sv<char> first, second;

		if (end == sv<char>::npos) {
			first = str;
			second = "";
		}
		else {
			first = str.substr(0, end);
			second = str.substr(end + 1);
		}

		return { strip(first, strips), strip(second, strips) };
	}

	[[nodiscard]]
	inline std::vector<sv<char>> splitStrip(sv<char> str, char delim, sv<char> strips = " \t") {
		std::vector<sv<char>> out{};
		str = strip(str, strips);
		if (str == "") return out;

		out.reserve(str.size() / 2);

		auto [first, next] = splitFirst(str, delim, strips);
		out.emplace_back(std::move(first));

		while (next != "") {
			auto [cur, rest] = splitFirst(next, delim, strips);
			out.emplace_back(std::move(cur));
			next = std::move(rest);
		}

		if (str.back() == delim) {
			out.emplace_back("");
		}

		return out;
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
