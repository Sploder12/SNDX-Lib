#pragma once

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#define UNDEF_MINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define UNDEF_LEANMEAN
#endif

#include <windows.h>

#include <string>
#include <string_view>
[[nodiscard]]
inline std::wstring toWinStr(std::string_view str) {
	auto len = MultiByteToWideChar(CP_UTF8, 0, str.data(), int(str.size()), NULL, 0);

	std::wstring out{};
	if (len == 0) {
		return out;
	}

	out.resize(len);
	MultiByteToWideChar(CP_UTF8, 0, str.data(), int(str.size()), out.data(), len);
	return out;
};

#ifdef UNDEF_MINMAX
#undef NOMINMAX
#undef UNDEF_MINMAX
#endif

#ifdef UNDEF_LEANMEAN
#undef WIN32_LEAN_AND_MEAN
#undef UNDEF_LEANMEAN
#endif

#endif