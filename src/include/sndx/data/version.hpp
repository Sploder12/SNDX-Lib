#pragma once

#include <string>

#define SNDXLIB_MAJOR 0
#define SNDXLIB_MINOR 3
#define SNDXLIB_PATCH 1

#define SNDX_AS_STRING(macro) #macro

#define DETAIL_SNDXLIB_VERSION(major, minor, patch) (SNDX_AS_STRING(major) "." SNDX_AS_STRING(minor) "." SNDX_AS_STRING(patch))
#define SNDXLIB_VERSION DETAIL_SNDXLIB_VERSION(SNDXLIB_MAJOR, SNDXLIB_MINOR, SNDXLIB_PATCH)

namespace sndx {
	struct Version {
		int major{}, minor{}, patch{};

		[[nodiscard]]
		std::string asString(const std::string& seperator = ".") const {
			return std::to_string(major) + seperator + std::to_string(minor) + seperator + std::to_string(patch);
		}

		auto operator<=>(const Version& other) const = default;
	};

	static_assert(Version{ 3, 2, 1 } > Version{ 1, 2, 3 });
	static_assert(Version{ 1, 2, 0 } == Version{ 1, 2, 0 });
	static_assert(noexcept(Version{} < Version{}));

	constexpr Version SNDXlibVersion{ SNDXLIB_MAJOR, SNDXLIB_MINOR, SNDXLIB_PATCH };

	static_assert(SNDXlibVersion >= Version{ 0, 0, 0 });
}