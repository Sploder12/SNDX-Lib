#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace sndx {
	template <typename CharT = char> [[nodiscard]]
	std::basic_string_view<CharT> strip(std::basic_string_view<CharT> str, std::basic_string_view<CharT> strips = " \t") {
		auto first = str.find_first_not_of(strips);
		auto last = str.find_last_not_of(strips);

		if (first == std::basic_string_view<CharT>::npos) [[unlikely]] {
			return str.substr(0, 0);
		}

		return str.substr(first, last - first + 1);
	}

	template <typename CharT = char> [[nodiscard]]
	std::vector<std::basic_string_view<CharT>> splitStrip(std::basic_string_view<CharT> str, CharT delim, std::basic_string_view<CharT> strips = " \t") {
		std::vector<std::basic_string_view<CharT>> out{};
		if (str == "") return out;

		out.reserve(str.size() / 2);

		auto in = strip(str, strips);

		size_t split = 0;
		while (split != std::basic_string_view<CharT>::npos) {
			auto next = in.find_first_of(delim, split);

			if (next != std::basic_string_view<CharT>::npos) {
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
}
