#pragma once

#include <gl/glew.h>

#include <glm/glm.hpp>

#include "imagedata.hpp"

namespace sndx {

	[[nodiscard]]
	constexpr GLenum formatFromChannels(int channels) {
		switch (channels) {
		case 1:
			return GL_RED;
		case 2:
			return GL_RG;
		case 3:
			return GL_RGB;
		case 4:
			return GL_RGBA;
		default:
			assert(false);
			return GL_RGBA;
		}
	}

	[[nodiscard]]
	constexpr int channelsFromFormat(GLenum format) {
		switch (format) {
		case GL_RED:
			return 1;
		case GL_RG:
			return 2;
		case GL_RGB:
			return 3;
		case GL_RGBA:
			return 4;
		default:
			return 0;
		}
	}

	struct Texture {
		GLuint id;
		size_t width, height;
		int channels;
		GLenum type = GL_TEXTURE_2D;

		constexpr explicit Texture() :
			id(0), width(0), height(0), channels(0) {}

		Texture(size_t width, size_t height, const void* data, GLenum iformat, GLenum format, GLenum filter) :
			width(width), height(height), channels(channelsFromFormat(iformat)) {

			if (width <= 0 || height <= 0) [[unlikely]] {
				throw std::runtime_error("Texture has dimension of 0");
			}

			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glGenTextures(1, &id);
			glBindTexture(type, id);
			glTexImage2D(type, 0, iformat, (GLsizei)width, (GLsizei)height, 0, format, GL_UNSIGNED_BYTE, data);
			glTexParameteri(type, GL_TEXTURE_MIN_FILTER, filter);
			glTexParameteri(type, GL_TEXTURE_MAG_FILTER, filter);
			glBindTexture(type, 0);
		}

		Texture(size_t width, size_t height, GLenum iformat, GLenum filter) :
			Texture(width, height, NULL, iformat, iformat, filter) {}

		void bind(size_t tex = 0) const {
			assert(tex <= 31);
			glActiveTexture(GLenum(GL_TEXTURE0 + tex));
			glBindTexture(GL_TEXTURE_2D, id);
		}

		void destroy() {
			if (id != 0) {
				glDeleteTextures(1, &id);
				id = 0;
			}

			width = 0;
			height = 0;
		}

		[[nodiscard]]
		ImageData asImage() const {
			ImageData out;
			bind();

			GLsizei size = GLsizei(width * height * sizeof(unsigned char) * channels);
			out.data.resize(size);

			glGetTexImage(type, 0, formatFromChannels(channels), GL_UNSIGNED_BYTE, out.data.data());

			out.width = int(width);
			out.height = int(height);
			out.channels = channels;
			return out;
		}

		void save(const char* path, int quality = 100) const {
			asImage().save(path, quality);
		}
	};

	[[nodiscard]]
	inline Texture textureFromImage(const ImageData& image, GLenum iformat = GL_RGBA, GLenum filter = GL_NEAREST) {
		return Texture(image.width, image.height, image.data.data(), iformat, formatFromChannels(image.channels), filter);
	}

	[[nodiscard]]
	inline std::optional<Texture> textureFromFile(const char* path, int wantedChannels = (int)STBI_rgb_alpha, GLenum iformat = GL_RGBA, GLenum filter = GL_NEAREST) {
		auto img = imageFromFile(path, wantedChannels);

		if (img.has_value()) {
			return textureFromImage(img.value(), iformat, filter);
		}

		return {};
	}
}

