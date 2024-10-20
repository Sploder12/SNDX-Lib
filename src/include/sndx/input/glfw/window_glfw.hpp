#pragma once

#include "../window_backend.hpp"

#define NOMINMAX
#include <GLFW/glfw3.h>

#include "../../render/viewport.hpp"

#include <type_traits>
#include <string>
#include <stdexcept>
#include <optional>
#include <unordered_map>

namespace sndx::input {
	class WindowGLFW : public Window<WindowGLFW> {
		friend Window;
		friend class WindowBuilderGLFW;

		std::string m_title{};
		GLFWwindow* m_window = nullptr;

		explicit WindowGLFW(std::nullptr_t, const char* = "") = delete;
		explicit WindowGLFW(GLFWwindow* window, const char* title = "") :
			m_title{title}, m_window{ window } {

			if (!m_window)
				throw std::runtime_error("Could not create glfwWindow");

			glfwSetWindowUserPointer(m_window, this);
		}

	public:
		WindowGLFW(const char* title, int width, int height, GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr) :
			WindowGLFW(glfwCreateWindow(width, height, title ? title : "", monitor, share), title ? title : "") {}

		WindowGLFW(std::nullptr_t, int, int, GLFWmonitor* = nullptr, GLFWwindow* = nullptr) = delete;

		WindowGLFW(WindowGLFW&& other) noexcept :
			m_window(std::exchange(other.m_window, nullptr)) {

			glfwSetWindowUserPointer(m_window, this);
		}

		WindowGLFW& operator=(WindowGLFW&& other) noexcept {
			std::swap(m_window, other.m_window);

			glfwSetWindowUserPointer(m_window, this);
			glfwSetWindowUserPointer(other.m_window, nullptr);
			return *this;
		}

		~WindowGLFW() noexcept {
			if (m_window) {
				glfwDestroyWindow(m_window);
				m_window = nullptr;
			}
		}

		operator GLFWwindow* () const {
			return m_window;
		}

	private:
		void bindImpl() const noexcept {
			glfwMakeContextCurrent(m_window);
		}

		void setPositionImpl(const glm::ivec2& pos) noexcept {
			glfwSetWindowPos(m_window, pos.x, pos.y);
		}

		void resizeImpl(const glm::ivec2& dims) {
			glfwSetWindowSize(m_window, dims.x, dims.y);
		}

		void focusWindowImpl() {
			glfwFocusWindow(m_window);
		}

		void requestAttentionImpl() {
			glfwRequestWindowAttention(m_window);
		}

		void setVisibilityImpl(bool visible) {
			if (visible) glfwShowWindow(m_window);
			else glfwHideWindow(m_window);
		}

		decltype(auto) setTitleImpl(const std::string& newTitle) {
			glfwSetWindowTitle(m_window, newTitle.c_str());
			return std::exchange(m_title, newTitle);
		}

		void tryCloseImpl() {
			glfwSetWindowShouldClose(m_window, GLFW_TRUE);
		}

		[[nodiscard]]
		glm::ivec2 getPositionImpl() const noexcept {
			glm::ivec2 out{};
			glfwGetWindowPos(m_window, &out.x, &out.y);
			return out;
		}

		[[nodiscard]]
		glm::ivec2 getSizeImpl() const noexcept {
			glm::ivec2 out{};
			glfwGetWindowSize(m_window, &out.x, &out.y);
			return out;
		}

		[[nodiscard]]
		bool isVisibleImpl() const noexcept {
			return glfwGetWindowAttrib(m_window, GLFW_VISIBLE) == GLFW_TRUE;
		}

		[[nodiscard]]
		const std::string& getTitleImpl() const noexcept {
			return m_title;
		}
	};

	class WindowHintsGLFW {
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

	class WindowBuilderGLFW final: public WindowBuilder<WindowBuilderGLFW> {
		friend WindowBuilder;

		GLFWmonitor* m_monitor = nullptr;
		GLFWwindow* m_share = nullptr;
		GLFWcursor* m_cursor = nullptr;

		WindowHintsGLFW* m_hints = nullptr;

	public:
		WindowBuilderGLFW& copySettings(const WindowBuilderGLFW& other) {
			m_monitor = other.m_monitor;
			m_share = other.m_share;
			m_cursor = other.m_cursor;
			m_hints = other.m_hints;

			return copyBaseSettings(other);
		}

		WindowBuilderGLFW& setMonitor(GLFWmonitor* monitor = nullptr) noexcept {
			m_monitor = monitor;
			return *this;
		}

		WindowBuilderGLFW& setShare(GLFWwindow* share = nullptr) noexcept {
			m_share = share;
			return *this;
		}

		WindowBuilderGLFW& setCursor(GLFWcursor* cursor = nullptr) noexcept {
			m_cursor = cursor;
			return *this;
		}

		WindowBuilderGLFW& setHints(WindowHintsGLFW* hints = nullptr) noexcept {
			m_hints = hints;
			return *this;
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

		[[nodiscard]]
		auto getHints() const noexcept {
			return m_hints;
		}

	private:
		[[nodiscard]]
		auto buildImpl() const {
			bool visible = !m_hints || m_hints->getHintOr(GLFW_VISIBLE, GLFW_TRUE) == GLFW_TRUE;
			if (!visible || this->m_xpos || this->m_ypos) {
				glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
			}

			auto window = glfwCreateWindow(this->m_width, this->m_height, this->m_title.c_str(), m_monitor, m_share);
			if (!window)
				throw std::runtime_error("Could not create glfwWindow");

			if (m_cursor)
				glfwSetCursor(window, m_cursor);

			if (this->m_xpos || this->m_ypos) {
				int sx, sy;
				glfwGetWindowPos(window, &sx, &sy);
				glfwSetWindowPos(window, this->m_xpos.value_or(sx), this->m_ypos.value_or(sy));

				if (visible) {
					glfwShowWindow(window);
				}
			}

			return WindowGLFW{ window, m_title.c_str() };
		}
	};
}