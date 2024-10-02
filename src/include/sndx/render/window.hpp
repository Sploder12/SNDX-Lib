#pragma once

#define NOMINMAX
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "./viewport.hpp"

#include <type_traits>
#include <string>
#include <stdexcept>

namespace sndx::render {
	// this class does not own the window!
	template <template <typename, glm::qualifier> class ViewportT, typename InternalT = float, glm::qualifier Qualifier = glm::qualifier::defaultp>
		requires (std::is_base_of_v<Viewport<InternalT, Qualifier>, ViewportT<InternalT, Qualifier>>)
	class Window : public ViewportT<InternalT, Qualifier> {
	protected:
		GLFWwindow* m_window = nullptr;

		using Viewport = ViewportT<InternalT, Qualifier>;
		using Vec = Viewport::Vec;

	public:
		template <class... Args>
		Window(const char* title, GLFWmonitor* monitor, GLFWwindow* share, Args&&... args) :
			Viewport{ std::forward<Args>(args)... }, m_window{ nullptr } {
		
			const auto& offset = this->getOffset();
			const auto& dims = this->getDimensions();

			m_window = glfwCreateWindow(int(dims.x + offset.x * 2), int(dims.y + offset.y * 2), title, monitor, share);
			if (!m_window)
				throw std::runtime_error("Could not create glfwWindow");
		}

		template <class... Args>
		Window(std::nullptr_t, GLFWmonitor*, GLFWwindow*, Args&&...) = delete;

		operator GLFWwindow* () const {
			return m_window;
		}
	
		void setViewport() const noexcept {
			glfwMakeContextCurrent(m_window);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			const auto& offset = this->getOffset();
			const auto& dims = this->getDimensions();
			
			glViewport(GLint(offset.x), GLint(offset.y), GLsizei(dims.x), GLsizei(dims.y));
		}

		void setOffset(Vec offset) noexcept override  {
			ViewportT<InternalT, Qualifier>::setOffset(offset);
			setViewport();
		}

		void resize(Vec newDims) noexcept override {
			ViewportT<InternalT, Qualifier>::resize(newDims);
			setViewport();
		}
	};
}