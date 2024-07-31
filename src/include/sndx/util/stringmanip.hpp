#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <array>

namespace sndx {
	template <typename CharT = char>
	using sv = std::basic_string_view<CharT>;

	template <typename CharT = char>
	using Str = std::basic_string<CharT>;

	[[nodiscard]]
	inline constexpr auto strip(sv<char> str, sv<char> strips = " \t\r") {
		auto first = str.find_first_not_of(strips);
		auto last = str.find_last_not_of(strips);

		if (first == sv<char>::npos) [[unlikely]] {
			return str.substr(0, 0);
		}

		return str.substr(first, last - first + 1);
	}

	[[nodiscard]]
	inline constexpr std::pair<sv<char>, sv<char>> splitFirst(sv<char> str, char delim, sv<char> strips = " \t\r") {
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
	inline std::vector<sv<char>> splitStrip(sv<char> str, char delim, sv<char> strips = " \t\r") {
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

	using Codepoint = uint32_t;

	[[nodiscard]]
	inline std::optional<std::basic_string<Codepoint>> decodeUTF8(std::string_view str) {
		std::basic_string<Codepoint> out{};
		out.reserve(str.size());

		for (size_t i = 0; i < str.size(); ++i) {
			auto chr = str[i];

			if ((chr & 0b10000000) == 0) {
				out.push_back(Codepoint(chr));
				continue;
			}

			Codepoint cur = 0;

			auto len = 0;
			if ((chr & 0b11100000) == 0b11000000) {
				len = 2;
				cur = chr & 0b00011111;
			}
			else if ((chr & 0b11110000) == 0b11100000) {
				len = 3;
				cur = chr & 0b00001111;
			}
			else if ((chr & 0b11111000) == 0b11110000) {
				len = 4;
				cur = chr & 0b00000111;
			}
			else {
				// invalid, assume extended ascii (ISO-8859-1)
				out.push_back(Codepoint(chr));
				continue;
			}

			cur <<= 6;

			if (i + len - 1 >= str.size()) {
				// invalid, assume extended ascii (ISO-8859-1)
				out.push_back(Codepoint(chr));
				continue;
			}

			for (size_t j = 1; j < len; ++j) {
				auto b = str[i + j];

				if ((b & 0b11000000) != 0b10000000)
					return std::nullopt;

				cur |= b & 0b00111111;

				if (j != len - 1) {
					cur <<= 6;
				}
			}

			out.push_back(cur);
			i += len - 1;
		}

		return out;
	}

	[[nodiscard]]
	inline std::optional<std::string> encodeUTF8(std::basic_string_view<Codepoint> str) {
		std::string out{};
		out.reserve(str.size() * 4);

		for (auto codepoint : str) {
			if (codepoint > 0x10FFFF) return std::nullopt;

			if (codepoint <= 0x007F) { // 1 byte
				out.push_back(static_cast<char>(codepoint));
				continue;
			}

			std::array<char, 3> endBytes{};
			
			static constexpr auto getLast = [](Codepoint codepoint) {
				return (codepoint & 0b00111111) | 0b10000000;
			};

			auto tmpPoint = codepoint;
			for (auto& b : endBytes) {
				b = getLast(tmpPoint);
				tmpPoint >>= 6;
			}

			
			if (codepoint <= 0x07FF) { // 2 byte
				out.push_back(static_cast<char>((codepoint >> 6) | 0b11000000));
			}
			else if (codepoint <= 0xFFFF) { // 3 byte
				out.push_back(static_cast<char>((codepoint >> 12) | 0b11100000));
			}
			else { // 4 byte
				out.push_back(static_cast<char>((codepoint >> 18) | 0b11110000));
			}

			for (auto it = endBytes.rbegin(); it != endBytes.rend(); ++it) {
				if ((*it & 0b00111111) == 0) {
					break;
				}

				out.push_back(*it);
			}
		}

		return out;
	}
}
