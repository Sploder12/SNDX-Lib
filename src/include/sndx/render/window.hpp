#pragma once

#define NOMINMAX
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "./viewport.hpp"

#include <type_traits>
#include <string>
#include <string_view>
#include <stdexcept>
#include <optional>
#include <unordered_map>

namespace sndx::render {
	// this class does not own the window!
	template <template <typename, glm::qualifier> class ViewportT, typename InternalT = float, glm::qualifier Qualifier = glm::qualifier::defaultp>
		requires (std::is_base_of_v<Viewport<InternalT, Qualifier>, ViewportT<InternalT, Qualifier>>)
	class Window : public ViewportT<InternalT, Qualifier> {
	protected:
		GLFWwindow* m_window = nullptr;

		using Viewport = ViewportT<InternalT, Qualifier>;
		using Vec = Viewport::Vec;

		template <class... Args>
		Window(GLFWwindow* window, Args&&... args) :
			Viewport{ std::forward<Args>(args)... }, m_window{ window } {}

		template <class... Args>
		Window(std::nullptr_t, Args&&...) = delete;

		friend class WindowBuilder;
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

		~Window() noexcept {
			if (m_window) {
				glfwDestroyWindow(m_window);
				m_window = nullptr;
			}
		}

		operator GLFWwindow* () const {
			return m_window;
		}

		void bind() const noexcept {
			glfwMakeContextCurrent(m_window);
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

		[[nodiscard]]
		glm::ivec2 getPos() const noexcept {
			int x, y;
			glfwGetWindowPos(m_window, &x, &y);
			return glm::ivec2{ x, y };
		}
	};

	class WindowHints {
	protected:
		std::unordered_map<int, int> m_hints{};

	public:
		static void restoreDefaults() noexcept {
			glfwDefaultWindowHints();
		}

		[[nodiscard]]
		std::optional<int> getHint(int hint) const noexcept {
			if (auto it = m_hints.find(hint); it != m_hints.end()) {
				return it->second;
			}

			return std::nullopt;
		}

		[[nodiscard]]
		int getHintOr(int hint, int other) const noexcept {
			return getHint(hint).value_or(other);
		}

		void setHint(int hint, int value) noexcept {
			m_hints[hint] = value;
		}

		bool removeHint(int hint) noexcept {
			return m_hints.erase(hint) > 0;
		}

		void apply() const noexcept {
			for (auto [hint, val] : m_hints) {
				glfwWindowHint(hint, val);
			}
		}
	};

	class WindowBuilder {
	protected:
		std::string m_title{};

		int m_width = 0;
		int m_height = 0;

		std::optional<int> m_xpos{}, m_ypos{};

		GLFWmonitor* m_monitor = nullptr;
		GLFWwindow* m_share = nullptr;
		GLFWcursor* m_cursor = nullptr;

	public:
		WindowBuilder() noexcept = default;

		WindowBuilder& setTitle(std::string_view title) noexcept {
			m_title = title;
			return *this;
		}

		WindowBuilder& setMonitor(GLFWmonitor* monitor = nullptr) noexcept {
			m_monitor = monitor;
			return *this;
		}

		WindowBuilder& setShare(GLFWwindow* share = nullptr) noexcept {
			m_share = share;
			return *this;
		}

		WindowBuilder& setCursor(GLFWcursor* cursor = nullptr) noexcept {
			m_cursor = cursor;
			return *this;
		}

		WindowBuilder& setX(std::optional<int> x = std::nullopt) noexcept {
			m_xpos = x;
			return *this;
		}

		WindowBuilder& setY(std::optional<int> y = std::nullopt) noexcept {
			m_ypos = y;
			return *this;
		}

		WindowBuilder& setWidth(int width) noexcept {
			m_width = std::max(width, 1);
			return *this;
		}

		WindowBuilder& setHeight(int height) noexcept {
			m_height = std::max(height, 1);
			return *this;
		}


		[[nodiscard]]
		std::string_view getTitle() const noexcept {
			return m_title;
		}

		[[nodiscard]]
		auto getX() const noexcept {
			return m_xpos;
		}

		[[nodiscard]]
		auto getY() const noexcept {
			return m_ypos;
		}

		[[nodiscard]]
		auto getWidth() const noexcept {
			return m_width;
		}

		[[nodiscard]]
		auto getHeight() const noexcept {
			return m_height;
		}

		[[nodiscard]]
		auto getMonitor() const noexcept {
			return m_monitor;
		}

		[[nodiscard]]
		auto getShare() const noexcept {
			return m_share;
		}

		[[nodiscard]]
		auto getCursor() const noexcept {
			return m_cursor;
		}

		template <template <typename, glm::qualifier> class ViewportT, typename InternalT = float, glm::qualifier Qualifier = glm::qualifier::defaultp, class... Args> [[nodiscard]]
		auto build(const WindowHints& hints, Args&&... args) const {
			hints.apply();

			if (m_xpos || m_ypos) {
				glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
			}

			auto window = glfwCreateWindow(m_width, m_height, m_title.c_str(), m_monitor, m_share);
			if (!window)
				throw std::runtime_error("Could not create glfwWindow");

			if (m_cursor)
				glfwSetCursor(window, m_cursor);

			if (m_xpos || m_ypos) {
				int sx, sy;
				glfwGetWindowPos(window, &sx, &sy);
				glfwSetWindowPos(window, m_xpos.value_or(sx), m_ypos.value_or(sy));

				if (hints.getHintOr(GLFW_VISIBLE, GLFW_TRUE) == GLFW_TRUE)
					glfwShowWindow(window);
			}

			return Window<ViewportT, InternalT, Qualifier>{window, std::forward<Args>(args)...};
		} 
	};
}