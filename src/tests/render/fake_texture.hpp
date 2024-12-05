#pragma once

#include <gmock/gmock.h>

#include "render/image/imagedata.hpp"

class FakeTexture {
private:
	sndx::render::ImageData m_data;
	int m_mipmaps;
	bool m_compressed;

public:

	FakeTexture(const sndx::render::ImageData& img, int mipmaps = 0, bool compress = true) :
		m_data(img), m_mipmaps(mipmaps), m_compressed(compress) {}

	[[nodiscard]]
	size_t width() const noexcept {
		return m_data.width();
	}

	[[nodiscard]]
	size_t height() const noexcept {
		return m_data.height();
	}

	[[nodiscard]]
	const sndx::render::ImageData& asImage(uint8_t, int = 0) const noexcept {
		return m_data;
	}

	[[nodiscard]]
	int mipmaps() const noexcept {
		return m_mipmaps;
	}

	[[nodiscard]]
	bool compressed() const noexcept {
		return m_compressed;
	}
};