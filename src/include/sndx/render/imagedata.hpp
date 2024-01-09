#pragma once

#include <stb_image.h>
#include <stb_image_write.h>

/* Don't forget 
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
*/

#include <vector>
#include <filesystem>

namespace sndx {

	struct ImageData {
		int width = 0, height = 0;
		int channels = 0;
		std::vector<unsigned char> data{};

		void save(const char* path, int quality = 100) {

			stbi_flip_vertically_on_write(true);
			stbi_write_jpg(path, width, height, channels, data.data(), quality);
		}

		[[nodiscard]]
		ImageData asGrayScale() {
			ImageData out{};
			out.width = width;
			out.height = height;
			out.channels = 1;
			out.data.reserve(data.size() / channels);

			for (int i = 0; i < data.size() / channels; ++i) {
				int index = i * channels;
				
				int sum = 0;
				for (int c = 0; c < channels; ++c) {
					sum += data[index + c];
				}

				out.data.emplace_back(sum / channels);
			}

			return out;
		}
	};

	[[nodiscard]]
	inline std::optional<ImageData> imageFromFile(const char* path, int wantedChannels = (int)STBI_rgb_alpha, bool flip = true) {
		stbi_set_flip_vertically_on_load(flip);

		ImageData out;
		unsigned char* data = stbi_load(path, &out.width, &out.height, &out.channels, wantedChannels);
		if (!data || out.width <= 0 || out.height <= 0) [[unlikely]] {
			return {};
		}

		out.data.resize(out.width * out.height * out.channels);
		for (int i = 0; i < out.data.size(); ++i) {
			out.data[i] = data[i];
		}

		stbi_image_free(data);

		return out;
	}
}