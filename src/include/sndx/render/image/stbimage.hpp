#include <optional>

#define STBI_WINDOWS_UTF8
#include <stb_image.h>

#include "imagedata.hpp"

namespace sndx::render {

	class STBimageLoader {
	private:
		bool m_flip{};

	public:
		constexpr STBimageLoader(bool flip = true) noexcept :
			m_flip{ flip } {}

		[[nodiscard]]
		std::optional<ImageData> loadFromFile(const std::filesystem::path& path, uint8_t channels) const noexcept {
			stbi_set_flip_vertically_on_load_thread(m_flip);

			std::byte* bdata = nullptr;
			int width, height, chan;

			if constexpr (std::is_same_v<std::decay_t<decltype(path.native())>, std::wstring>) {
				const auto& filename = path.wstring();
				std::string buf{};
				buf.resize(stbi_convert_wchar_to_utf8(nullptr, 0, filename.c_str()));

				stbi_convert_wchar_to_utf8(buf.data(), buf.size(), filename.c_str());
				auto data = stbi_load(buf.c_str(), &width, &height, &chan, channels);
				bdata = reinterpret_cast<std::byte*>(data);
			}
			else {
				auto data = stbi_load(path.string().c_str(), &width, &height, &chan, channels);
				bdata = reinterpret_cast<std::byte*>(data);
			}

			if (!bdata)
				return std::nullopt;

			std::vector<std::byte> dat{};
			dat.reserve(width * height * channels);

			std::copy(bdata, bdata + width * height * channels, std::back_inserter(dat));
			stbi_image_free(bdata);

			return ImageData{ size_t(width), size_t(height), channels, std::move(dat) };
		}
	};
}