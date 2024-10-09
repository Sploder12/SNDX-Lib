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
	class Window {
	protected:
		ViewportT<InternalT, Qualifier> m_viewport;
		GLFWwindow* m_window = nullptr;

		using Viewport = decltype(m_viewport);
		using Vec = Viewport::Vec;

		template <class... Args>
		Window(GLFWwindow* window, int width, int height, Args&&... args) :
			m_viewport{ Vec{width, height}, std::forward<Args>(args)... }, m_window{ window } {

			if (!m_window)
				throw std::runtime_error("Could not create glfwWindow");

			int fwidth, fheight;
			glfwGetFramebufferSize(m_window, &fwidth, &fheight);
			setViewportSize(Vec{ fwidth, fheight });

			glfwSetWindowUserPointer(m_window, this);
		}

		template <class... Args>
		Window(std::nullptr_t, int, int, Args&&...) = delete;

		friend class WindowBuilder;
	public:

		template <class... Args>
		Window(const char* title, int width, int height, GLFWmonitor* monitor, GLFWwindow* share, Args&&... args) :
			Window(glfwCreateWindow(width, height, title, monitor, share), width, height, std::forward<Args>(args)...) {}

		template <class... Args>
		Window(std::nullptr_t, int, int, GLFWmonitor*, GLFWwindow*, Args&&...) = delete;

		Window(Window&& other) noexcept :
			m_window(std::exchange(other.m_window, nullptr)),
			m_viewport(other.m_viewport) {

			glfwSetWindowUserPointer(m_window, this);
		}

		Window& operator=(Window&& other) noexcept {
			std::swap(m_viewport, other.m_viewport);
			std::swap(m_window, other.m_window);

			glfwSetWindowUserPointer(m_window, this);
			glfwSetWindowUserPointer(other.m_window, nullptr);
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

			const auto& offset = m_viewport.getOffset();
			const auto& dims = m_viewport.getDimensions();

			glViewport(GLint(offset.x), GLint(offset.y), GLsizei(dims.x), GLsizei(dims.y));
		}

		void setViewportOffset(Vec offset) noexcept {
			m_viewport.setOffset(offset);
			setViewport();
		}

		void setViewportSize(Vec newDims) {
			m_viewport.resize(newDims);
			setViewport();
		}

		void setPosition(int x, int y) noexcept {
			glfwSetWindowPos(m_window, x, y);
		}

		void resize(int width, int height) {
			if (width <= 0 || height <= 0)
				throw std::invalid_argument("Window dims cannot be less than 1");

			glfwSetWindowSize(m_window, width, height);

			int fwidth, fheight;
			glfwGetFramebufferSize(m_window, &fwidth, &fheight);

			resizeViewport(Vec{ fwidth, fheight });
		}

		[[nodiscard]]
		glm::ivec2 getPos() const noexcept {
			glm::ivec2 out{};
			glfwGetWindowPos(m_window, &out.x, &out.y);
			return out;
		}

		[[nodiscard]]
		glm::ivec2 getSize() const noexcept {
			glm::ivec2 out{};
			glfwGetWindowSize(m_window, &out.x, &out.y);
			return out;
		}

		[[nodiscard]]
		const auto& getViewport() const noexcept {
			return m_viewport;
		}

		[[nodiscard]]
		Vec pixToNDC(Vec in) const noexcept {
			return m_viewport.pixToNDC(in);
		}

		[[nodiscard]]
		Vec NDCtoPix(const Vec& ndc) const noexcept {
			return m_viewport.NDCtoPix(ndc);
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

		int m_width = 1;
		int m_height = 1;

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
		auto build(bool visible = true, Args&&... args) const {
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

				if (visible)
					glfwShowWindow(window);
			}

			return Window<ViewportT, InternalT, Qualifier>{window, m_width, m_height, std::forward<Args>(args)...};
		} 

		template <template <typename, glm::qualifier> class ViewportT, typename InternalT = float, glm::qualifier Qualifier = glm::qualifier::defaultp, class... Args> [[nodiscard]]
		auto build(const WindowHints& hints, Args&&... args) const {
			hints.apply();
			return build(hints.getHintOr(GLFW_VISIBLE, GLFW_TRUE) == GLFW_TRUE, std::forward<Args>(args)...);
		} 
	};
}