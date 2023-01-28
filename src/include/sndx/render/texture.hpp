#pragma once

#include <gl/glew.h>

#include <glm/glm.hpp>

#include "imagedata.hpp"

namespace sndx {

	constexpr GLenum formatFromChannels(int channels) {
		switch (channels) {
		case 1:
			return GL_R;
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

	constexpr int channelsFromFormat(GLenum format) {
		switch (format) {
		case GL_R:
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
			width(0), height(0), channels(0), id(0) {}

		Texture(size_t width, size_t height, GLenum iformat, GLenum filter) :
			width(width), height(height), channels(channelsFromFormat(iformat)) {

			if (width <= 0 || height <= 0) [[unlikely]] {
				throw std::runtime_error("Texture has dimension of 0");
			}

			glGenTextures(1, &id);
			glBindTexture(GL_TEXTURE_2D, id);
			glTexImage2D(GL_TEXTURE_2D, 0, iformat, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		Texture(size_t width, size_t height, const void* data, GLenum format, GLenum filter) :
			width(width), height(height), channels(channelsFromFormat(format)) {

			if (width <= 0 || height <= 0) [[unlikely]] {
				throw std::runtime_error("Texture has dimension of 0");
			}

			glGenTextures(1, &id);
			glBindTexture(GL_TEXTURE_2D, id);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, format, GL_UNSIGNED_BYTE, data);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		void bind(size_t tex = 0) const {
			assert(tex <= 31);
			glActiveTexture(GLenum(GL_TEXTURE0 + tex));
			glBindTexture(GL_TEXTURE_2D, id);
		}

		void destroy() {
			glDeleteTextures(1, &id);
			id = 0;
			width = 0;
			height = 0;
		}

		void save(const char* path, int quality = 100) {
			bind();

			GLsizei size = GLsizei(width * height * sizeof(unsigned char) * 4u);
			std::vector<unsigned char> data;
			data.resize(size);

			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

			stbi_flip_vertically_on_write(true);
			stbi_write_jpg(path, (int)width, (int)height, channels, data.data(), quality);
		}
	};

	[[nodiscard]]
	Texture textureFromImage(const ImageData& image, GLenum filter = GL_NEAREST) {
		return Texture(image.width, image.height, image.data.data(), formatFromChannels(image.channels), filter);
	}

	[[nodiscard]]
	Texture textureFromFile(const char* path, int wantedChannels = (int)STBI_rgb_alpha, GLenum filter = GL_NEAREST) {
		return textureFromImage(imageFromFile(path, wantedChannels), filter);
	}
}

