#pragma once

#include "render/image/imagedata.hpp"

namespace sndx::render {

	inline const std::filesystem::path test_image_path{ u8"test_data/visual/rgbbw_test_img☃.png" };

	template <size_t channels = 3> [[nodiscard]]
	ImageData createSolidImage(size_t width, size_t height, glm::vec<channels, std::byte> color = {}) {
		std::vector<std::byte> buffer{};
		buffer.resize(width * height * channels);

		using l = decltype(color)::length_type;

		for (size_t y = 0; y < height; ++y) {
			for (size_t x = 0; x < width; ++x) {
				for (size_t c = 0; c < channels; ++c) {
					buffer[y * width * channels + x * channels + c] = color[c];
				}
			}
		}

		return ImageData{ width, height, channels, std::move(buffer) };
	}

	template <size_t channels = 3> [[nodiscard]]
	ImageData createCheckeredImage(size_t width, size_t height, glm::vec<channels, std::byte> c1, glm::vec<channels, std::byte> c2 = {}) {
		std::vector<std::byte> buffer{};
		buffer.resize(width * height * channels);

		using l = decltype(c1)::length_type;

		for (size_t y = 0; y < height; ++y) {
			for (size_t x = 0; x < width; ++x) {
				auto b = (y + x) % 2 == 0;

				for (l c = 0; c < channels; ++c) {
					buffer[y * width * channels + x * channels + c] = b ? c1[c] : c2[c];
				}
			}
		}

		return ImageData{ width, height, channels, std::move(buffer) };
	}

	[[nodiscard]]
	inline bool imageEqual(const ImageData& img1, const ImageData& img2) {
		if (img1.width() != img2.width()) return false;
		if (img1.height() != img2.height()) return false;
		if (img1.channels() != img2.channels()) return false;

		if (img1.pixels() != img2.pixels()) return false;
		if (img1.bytes() != img2.bytes()) return false;

		for (size_t i = 0; i < img1.bytes(); ++i) {
			if (img1.data()[i] != img2.data()[i]) return false;
		}

		return true;
	}
}