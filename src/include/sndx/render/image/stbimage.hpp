#include <optional>

#define STBI_WINDOWS_UTF8
#include <stb_image.h>

#define STBIW_WINDOWS_UTF8
#include <stb_image_write.h>

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

			std::u8string strPath{ path.u8string() };
			auto cstr = reinterpret_cast<const char*>(strPath.c_str());

			int width, height, chan;
			auto data = stbi_load(cstr, &width, &height, &chan, channels);
			auto bdata = reinterpret_cast<std::byte*>(data);

			if (!bdata)
				return std::nullopt;

			std::vector<std::byte> dat{};
			dat.reserve(width * height * channels);

			std::copy_n(bdata, width * height * channels, std::back_inserter(dat));
			stbi_image_free(bdata);

			return ImageData{ size_t(width), size_t(height), channels, std::move(dat) };
		}
	};

	class STBimageSaver {
	private:
		int8_t m_quality{};
		bool m_flip{};

	public:
		STBimageSaver(bool flip = false, int8_t quality = 100) noexcept:
			m_quality(quality), m_flip(flip) {}

		bool save(const std::filesystem::path& path, const ImageData& image) const {
			stbi_flip_vertically_on_write(m_flip);

			std::u8string strPath{path.u8string()};
			auto cstr = reinterpret_cast<const char*>(strPath.c_str());

			if (path.extension() == ".jpg" || path.extension() == ".jpeg")
				return stbi_write_jpg(cstr, image.width(), image.height(), image.channels(), image.data(), m_quality);
			
			if (path.extension() == ".bmp")
				return stbi_write_bmp(cstr, image.width(), image.height(), image.channels(), image.data());
			
			return stbi_write_png(cstr, image.width(), image.height(), image.channels(), image.data(), 0);
		}
	};
}