#pragma once

#include <GL/glew.h>

#include <glm/glm.hpp>

#include "../image/imagedata.hpp"

namespace sndx::render {
	[[nodiscard]]
	constexpr GLenum formatFromChannels(int channels, bool compressed = false) {
		switch (channels) {
		case 1:
			return compressed ? GL_COMPRESSED_RED : GL_RED;
		case 2:
			return compressed ? GL_COMPRESSED_RG : GL_RG;
		case 3:
			return compressed ? GL_COMPRESSED_RGB : GL_RGB;
		case 4:
			return compressed ? GL_COMPRESSED_RGBA : GL_RGBA;
		default:
			throw std::invalid_argument("Channels must be between 1-4");
		}
	}

	[[nodiscard]]
	constexpr int channelsFromFormat(GLenum format) {
		switch (format) {
		case GL_ALPHA:
		case GL_RED:
		case GL_COMPRESSED_RED:
			return 1;
		case GL_RG:
		case GL_COMPRESSED_RG:
			return 2;
		case GL_RGB:
		case GL_COMPRESSED_RGB:
			return 3;
		case GL_RGBA:
		case GL_COMPRESSED_RGBA:
			return 4;
		default:
			throw std::invalid_argument("Unrecognized format");
		}
	}

	class Texture2D {
	private:
		size_t m_width{}, m_height{};
		GLuint m_id{};
		GLenum m_target{GL_TEXTURE_2D};
		
	public:

		explicit Texture2D() = default;

		Texture2D(GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* data, GLint mipmaps = 0) :
			m_width(width), m_height(height), m_target(target) {

			glGenTextures(1, &m_id);
			glBindTexture(m_target, m_id);
			glTexImage2D(m_target, mipmaps, internalFormat, GLsizei(m_width), GLsizei(m_height), 0, format, type, data);
		}

		Texture2D(const ImageData& image, GLint mipmaps = 0, bool compress = true) :
			Texture2D(GL_TEXTURE_2D, formatFromChannels(image.channels(), compress),
				GLsizei(image.width()), GLsizei(image.height()), formatFromChannels(image.channels(), false), GL_UNSIGNED_BYTE, image.data()) { }

		Texture2D(const Texture2D&) = delete;
		Texture2D(Texture2D&& other) noexcept :
			m_width(std::exchange(other.m_width, 0)), m_height(std::exchange(other.m_height, 0)),
			m_id(std::exchange(other.m_id, 0)), m_target(other.m_target) {}

		Texture2D& operator=(const Texture2D&) = delete;
		Texture2D& operator=(Texture2D&& other) noexcept {
			std::swap(m_width, other.m_width);
			std::swap(m_height, other.m_height);
			std::swap(m_id, other.m_id);
			std::swap(m_target, other.m_target);
			return *this;
		}

		~Texture2D() noexcept {
			if (m_id != 0)
				glDeleteTextures(1, &m_id);

			m_id = 0;
		}

		[[nodiscard]]
		auto width() const noexcept {
			return m_width;
		}

		[[nodiscard]]
		auto height() const noexcept {
			return m_height;
		}

		[[nodiscard]]
		auto target() const noexcept {
			return m_target;
		}

		void bind() const {
			glBindTexture(m_target, m_id);
		}

		void bind(size_t tex) const {
			if (tex >= 32)
				throw std::invalid_argument("Cannot bind beyond 32");

			glActiveTexture(GLenum(GL_TEXTURE0 + tex));
			bind();
		}

		[[nodiscard]]
		ImageData asImage(uint8_t channels, GLint mipmap = 0) const {
			auto format = formatFromChannels(channels, false);
			size_t size = size_t(m_width * m_height * channels * sizeof(std::byte));

			std::vector<std::byte> data{};
			data.resize(size);

			bind();
			glGetTexImage(m_target, mipmap, format, GL_UNSIGNED_BYTE, data.data());

			return ImageData{ m_width, m_height, channels, std::move(data) };
		}
	};

	template <class Loader> [[nodiscard]]
	auto loadTextureFile(const std::filesystem::path& path, uint8_t channels, const Loader& loader, GLint mipmaps = 0, bool compress = true) {
		if (auto img = loadImageFile(path, channels, loader)) {
			return std::optional<Texture2D>{Texture2D{ *img, mipmaps, compress }};
		}

		return std::optional<Texture2D>{};
	}

	template <class Saver> [[nodiscard]]
	bool saveTextureFile(const std::filesystem::path& path, const Texture2D& tex, uint8_t channels, const Saver& saver, GLint mipmap = 0) {
		return saver.save(path, tex.asImage(channels, mipmap));
	}
}