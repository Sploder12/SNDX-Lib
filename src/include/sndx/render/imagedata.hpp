#pragma once

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <vector>
#include <filesystem>

namespace sndx {

	struct ImageData {
		int width, height;
		int channels;
		std::vector<unsigned char> data;

		void save(const char* path, int quality = 100) {

			stbi_flip_vertically_on_write(true);
			stbi_write_jpg(path, width, height, channels , data.data(), quality);
		}
	};

	[[nodiscard]]
	ImageData imageFromFile(const char* path, int wantedChannels = (int)STBI_rgb_alpha) {
		stbi_set_flip_vertically_on_load(true);

		ImageData out;
		unsigned char* data = stbi_load(path, &out.width, &out.height, &out.channels, wantedChannels);
		if (!data || out.width <= 0 || out.height <= 0) [[unlikely]] {
			throw std::runtime_error(std::string("Failed to load image: ") + path);
		}

		out.data.resize(out.width * out.height * out.channels);
		for (int i = 0; i < out.data.size(); ++i) {
			out.data[i] = data[i];
		}

		stbi_image_free(data);

		return out;
	}
}