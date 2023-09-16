#pragma once

#define NOMINMAX
#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <stdexcept>

namespace sndx {

	struct Window {

		GLFWwindow* window;
		glm::vec2 dims;
		glm::vec2 offset;
		float aspectRatio;

		operator GLFWwindow* () const {
			return window;
		}

		[[nodiscard]]
		constexpr float getAspectRatio() const {
			return aspectRatio;
		}

		[[nodiscard]]
		constexpr glm::vec2 pixToNDC(glm::vec2 in) const {
			in -= offset;
			return (glm::vec2(in.x, dims.y - in.y) / dims) * 2.0f - glm::vec2(1.0f);
		}

		[[nodiscard]]
		constexpr glm::vec2 NDCtoPix(glm::vec2 ndc) const {
			auto tmp = (ndc + glm::vec2(1.0f)) / 2.0f;
			return dims * tmp;
		}

		void resetViewport() const {
			glfwMakeContextCurrent(window);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(GLint(offset.x), GLint(offset.y), GLint(dims.x), GLint(dims.y));
		}

		constexpr void resize(int width, int height) {
			int asWidth = int(height * aspectRatio);

			if (asWidth == width) [[unlikely]] {
				offset = glm::vec2(0.0f);
				dims = glm::vec2(width, height);
				return;
			}

			if (width == 0) {
				offset = glm::vec2(0.0f);
				dims = glm::vec2(asWidth, height);
				return;
			}

			int asHeight = int(width / aspectRatio);

			if (height == 0) {
				offset = glm::vec2(0.0f);
				dims = glm::vec2(width, asHeight);
				return;
			}

			if (width > asWidth) {
				int padding = (width - asWidth) / 2;
				dims = glm::vec2(asWidth, height);
				offset = glm::vec2(padding, 0);
				return;
			}

			int padding = (height - asHeight) / 2;
			dims = glm::vec2(width, asHeight);
			offset = glm::vec2(0, padding);
		}

		constexpr void setAspectRatio(float AR) {
			aspectRatio = AR;
			resize((int)dims.x, (int)dims.y);
		}
	};

	[[nodiscard]]
	inline Window createWindow(int width, int height, const char* name, float aspectRatio = 16.0f / 9.0f, GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr) {
		Window out{ nullptr, glm::vec2(0.0f), glm::vec2(0.0f), aspectRatio };
		out.resize(width, height);

		GLFWwindow* win = glfwCreateWindow(int(out.dims.x + out.offset.x * 2), int(out.dims.y + out.offset.y * 2), name, monitor, share);
		if (win == nullptr) [[unlikely]] throw std::runtime_error("Creating window resulted in nullptr.");

		out.window = win;

		return out;
	}
}