#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <array>
#include <cassert>

namespace sndx::string {
	template <typename CharT = char>
	using sv = std::basic_string_view<CharT>;

	template <typename CharT = char>
	using Str = std::basic_string<CharT>;

	template <typename CharT = char>
	static constexpr sv<CharT> defaultStrip = sv<CharT>{" \t\r"};

	
	template <class Chr = char> [[nodiscard]]
	constexpr auto strip(sv<Chr> str, sv<Chr> strips = defaultStrip<Chr>) noexcept {
		auto first = str.find_first_not_of(strips);
		auto last = str.find_last_not_of(strips);

		if (first == sv<char>::npos) [[unlikely]] {
			return str.substr(0, 0);
		}

		return str.substr(first, last - first + 1);
	}

	template <class CharT = char> [[nodiscard]]
	constexpr sv<CharT> strip(const Str<CharT>& str, sv<CharT> strips = defaultStrip<CharT>) {
		return strip(sv<CharT>(str), strips);
	}

	constexpr sv<char> strip(std::nullptr_t, sv<char> = defaultStrip<char>) = delete;

	[[nodiscard]]
	constexpr sv<char> strip(const char* str, sv<char> strips = defaultStrip<char>) {
		return strip(sv<char>(str), strips);
	}


	template <class Chr = char> [[nodiscard]]
	constexpr std::pair<sv<Chr>, sv<Chr>> splitFirst(sv<Chr> str, Chr delim, sv<Chr> strips = defaultStrip<Chr>) noexcept {
		auto end = str.find_first_of(delim);
		sv<Chr> first, second;

		if (end == sv<Chr>::npos) {
			first = str;
			second = "";
		}
		else {
			first = str.substr(0, end);
			second = str.substr(end + 1);
		}

		return { strip(first, strips), strip(second, strips) };
	}

	template <class CharT = char> [[nodiscard]]
	constexpr sv<CharT> splitFirst(const Str<CharT>& str, CharT delim, sv<CharT> strips = defaultStrip<char>) {
		return stripFirst(sv<CharT>(str), delim, strips);
	}

	constexpr std::pair<sv<char>, sv<char>> splitFirst(nullptr_t, char, sv<char> = defaultStrip<char>) = delete;

	[[nodiscard]]
	constexpr std::pair<sv<char>, sv<char>> splitFirst(const char* str, char delim, sv<char> strips = defaultStrip<char>) noexcept {
		return splitFirst(sv<char>{str}, delim, strips);
	}


	template <class Chr = char> [[nodiscard]]
	constexpr std::vector<sv<Chr>> splitStrip(sv<Chr> str, Chr delim, sv<Chr> strips = sv<Chr>{ " \t\r" }) {
		std::vector<sv<Chr>> out{};
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

	template <class Chr = char> [[nodiscard]]
	constexpr auto splitStrip(Str<Chr> str, Chr delim, sv<Chr> strips = sv<Chr>{ " \t\r" }) {
		return splitStrip(sv<Chr>{str}, delim, strips);
	}

	inline constexpr std::vector<sv<char>> splitStrip(nullptr_t, char, sv<char> = defaultStrip<char>) = delete;

	[[nodiscard]]
	constexpr auto splitStrip(const char* str, char delim, sv<char> strips = defaultStrip<char>) noexcept {
		return splitStrip(sv<char>{str}, delim, strips);
	}
	

	template <typename CharT = char> [[nodiscard]]
	constexpr Str<CharT> parseEscaped(sv<CharT> str) {
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
					[[fallthrough]];
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

	template <typename CharT = char> [[nodiscard]]
	constexpr Str<CharT> parseEscaped(const Str<CharT>& str) {
		return parseEscaped(sv<CharT>{str});
	}

	constexpr std::string parseEscaped(nullptr_t) = delete;

	[[nodiscard]]
	constexpr auto parseEscaped(const char* str) noexcept {
		return parseEscaped(sv<char>{str});
	}


	using Codepoint = uint32_t;

	[[nodiscard]]
	inline constexpr Str<Codepoint> operator ""_codepoint(const char32_t* str, size_t size) {
		return Str<Codepoint>{(const Codepoint*)(str), size};
	}

	[[nodiscard]]
	inline std::optional<Str<Codepoint>> decodeUTF8(std::string_view str) {
		Str<Codepoint> out{};
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
				out.push_back(Codepoint(uint8_t(chr)));
				continue;
			}

			cur <<= 6;

			if (i + len - 1 >= str.size()) {
				// invalid, assume extended ascii (ISO-8859-1)
				out.push_back(Codepoint(uint8_t(chr)));
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
	inline std::optional<std::string> encodeUTF8(sv<Codepoint> str) {
		std::string out{};
		out.reserve(str.size() * 2);

		for (auto codepoint : str) {
			if (codepoint > 0x10FFFF) return std::nullopt;

			if (codepoint <= 0x007F) { // 1 byte
				out.push_back(static_cast<char>(codepoint));
				continue;
			}

			std::array<char, 3> endBytes{};
			
			// gets 6 bits then appends the UTF-8 extend bits
			static constexpr auto getLast = [](Codepoint codepoint) {
				return char((codepoint & 0b00111111) | 0b10000000);
			};

			auto tmpPoint = codepoint;
			for (auto& b : endBytes) {
				b = getLast(tmpPoint);
				tmpPoint >>= 6;
			}

			size_t size = 1;
			if (codepoint <= 0x07FF) { // 2 byte
				out.push_back(static_cast<char>((codepoint >> 6) | 0b11000000));
				size = 1;
			}
			else if (codepoint <= 0xFFFF) { // 3 byte
				out.push_back(static_cast<char>((codepoint >> 12) | 0b11100000));
				size = 2;
			}
			else { // 4 byte
				out.push_back(static_cast<char>((codepoint >> 18) | 0b11110000));
				size = 3;
			}

			assert(size <= endBytes.size());
			for (size_t i = 0; i < size; ++i) {
				out.push_back(endBytes[size - 1 - i]);
			}
		}

		return out;
	}
}
