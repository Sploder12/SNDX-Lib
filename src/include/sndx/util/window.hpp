#pragma once

#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <stdexcept>

namespace sndx {

	template <float aspectRatio = 16.0f / 9.0f>
	struct Window {

		GLFWwindow* window;
		glm::vec2 dims;
		glm::vec2 offset;

		[[nodiscard]]
		constexpr float getAspectRatio() const {
			return aspectRatio;
		}
		
		[[nodiscard]]
		constexpr glm::vec2 pixToNDC(glm::vec2 in) const {
			glm::vec2 realDims = glm::vec2(dims.x + offset.x * 2, dims.y + offset.y * 2);
			return (glm::vec2(in.x, realDims.y - in.y) / realDims) * 2.0f - glm::vec2(1.0f);
		}

		void resetViewport() const {
			glfwMakeContextCurrent(window);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(GLint(offset.x), GLint(offset.y), GLint(dims.x), GLint(dims.y));
		}

		constexpr void resize(int width, int height) {
			int asWidth = int(height * aspectRatio);

			if (asWidth == width) [[unlikely]] {
				dims = glm::vec2(width, height);
				return;
			}

				if (width > asWidth) {
					int padding = (width - asWidth) / 2;
					dims = glm::vec2(asWidth, height);
					offset = glm::vec2(padding, 0);
					return;
				}

			int asHeight = int(width / aspectRatio);
			int padding = (height - asHeight) / 2;
			dims = glm::vec2(width, asHeight);
			offset = glm::vec2(0, padding);
		}
	};


	template <float aspectRatio = 16.0f / 9.0f> [[nodiscard]]
	Window<aspectRatio> createWindow(int width, int height, const char* name, GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr) {
		GLFWwindow* win = glfwCreateWindow(width, height, name, monitor, share);
		if (win == nullptr) throw std::runtime_error("Creating window resulted in nullptr.");

		Window out{ win, glm::vec2(0.0f), glm::vec2(0.0f) };
		out.resize(width, height);
		return out;
	}
}