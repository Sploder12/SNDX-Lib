#pragma once

#define NOMINMAX
#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <stdexcept>

#include "../render/viewport.hpp"

namespace sndx {

	struct Window : public Viewport {

		GLFWwindow* window;

		explicit Window() :
			Viewport(glm::ivec2(0)), window(nullptr) {}

		Window(GLFWwindow* window, glm::ivec2 size, std::optional<float> aspectRatio = std::nullopt):
			Viewport(size, glm::ivec2(0), aspectRatio), window(window) {}

		operator GLFWwindow* () const {
			return window;
		}

		void setViewport() const {
			glfwMakeContextCurrent(window);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			Viewport::setViewport();
		}
	};

	[[nodiscard]]
	inline Window createWindow(int width, int height, const char* name, float aspectRatio = 16.0f / 9.0f, GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr) {
		Window out{ nullptr, glm::ivec2(width, height), aspectRatio };

		GLFWwindow* win = glfwCreateWindow(out.dims.x + out.offset.x * 2, out.dims.y + out.offset.y * 2, name, monitor, share);
		if (win == nullptr) [[unlikely]] throw std::runtime_error("Creating window resulted in nullptr.");

		out.window = win;

		return out;
	}
}