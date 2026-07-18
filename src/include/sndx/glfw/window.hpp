#pragma once

#include "./glfw.hpp"

#include "../render/window.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <type_traits>
#include <string>
#include <string_view>
#include <stdexcept>
#include <optional>
#include <unordered_map>
#include <utility>


namespace sndx::glfw {
	class Window final : public sndx::Window {
		friend class WindowBuilder;

		GLFWwindow* m_window = nullptr;

		explicit Window(std::nullptr_t) = delete;
		explicit Window(GLFWwindow* window) :
			m_window{ window } {

			if (!m_window)
				throw std::runtime_error("Could not create glfwWindow");

			glfwSetWindowUserPointer(m_window, this);
		}

	public:
		Window(const char* title, int width, int height, GLFWmonitor* monitor = nullptr, GLFWwindow* share = nullptr) :
			Window(glfwCreateWindow(width, height, title ? title : "", monitor, share)) {}

		Window(std::nullptr_t, int, int, GLFWmonitor* = nullptr, GLFWwindow* = nullptr) = delete;

		Window(Window&& other) noexcept :
			m_window(std::exchange(other.m_window, nullptr)) {

			glfwSetWindowUserPointer(m_window, this);
		}

		Window& operator=(Window&& other) noexcept {
			std::swap(m_window, other.m_window);

			glfwSetWindowUserPointer(other.m_window, nullptr);
			glfwSetWindowUserPointer(m_window, this);
			return *this;
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

		void setCursor(GLFWcursor* cursor = nullptr) noexcept {
			glfwSetCursor(m_window, cursor);
		}

		[[nodiscard]]
		bool shouldClose() const noexcept override {
			return glfwWindowShouldClose(m_window);
		}

		void unbind() noexcept override {
			glfwMakeContextCurrent(nullptr);
		}

		void bind() noexcept override {
			glfwMakeContextCurrent(m_window);
		}

		void swapBuffers() noexcept override {
			glfwSwapBuffers(m_window);
		}

		void setPosition(const glm::ivec2& pos) noexcept override {
			glfwSetWindowPos(m_window, pos.x, pos.y);
		}

		void resize(const glm::ivec2& dims) override {
			glfwSetWindowSize(m_window, dims.x, dims.y);
		}

		void captureMouse(bool capture) override {
			if (capture) {
				glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			else {
				glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}

		void forceFocus() override {
			glfwFocusWindow(m_window);
		}

		void requestAttention() override {
			glfwRequestWindowAttention(m_window);
		}

		void setVisibility(bool visible) override {
			if (visible) glfwShowWindow(m_window);
			else glfwHideWindow(m_window);
		}

		void setTitle(const std::string& newTitle) override {
			glfwSetWindowTitle(m_window, newTitle.c_str());
		}

		void signalClose() override {
			glfwSetWindowShouldClose(m_window, GLFW_TRUE);
		}

		[[nodiscard]]
		glm::ivec2 getPosition() const noexcept override {
			glm::ivec2 out{};
			glfwGetWindowPos(m_window, &out.x, &out.y);
			return out;
		}

		[[nodiscard]]
		glm::ivec2 getSize() const noexcept override {
			glm::ivec2 out{};
			glfwGetWindowSize(m_window, &out.x, &out.y);
			return out;
		}

		[[nodiscard]]
		glm::ivec2 getFramebufferSize() const noexcept override {
			glm::ivec2 out{};
			glfwGetFramebufferSize(m_window, &out.x, &out.y);
			return out;
		}

		[[nodiscard]]
		glm::vec2 getContentScale() const noexcept override {
			glm::vec2 out{};
			glfwGetWindowContentScale(m_window, &out.x, &out.y);
			return out;
		}

		[[nodiscard]]
		bool isMouseCaptured() const noexcept override {
			return glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
		}

		[[nodiscard]]
		bool isVisible() const noexcept override {
			return glfwGetWindowAttrib(m_window, GLFW_VISIBLE) == GLFW_TRUE;
		}

		[[nodiscard]]
		bool isFocused() const noexcept override {
			return glfwGetWindowAttrib(m_window, GLFW_FOCUSED) == GLFW_TRUE;
		}

		[[nodiscard]]
		std::string_view getTitle() const noexcept {
			return glfwGetWindowTitle(m_window);
		}
	};

	class WindowHints {
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
		std::string m_title{};

		int m_width = 1;
		int m_height = 1;

		std::optional<int> m_xpos{}, m_ypos{};

		GLFWmonitor* m_monitor = nullptr;
		GLFWwindow* m_share = nullptr;
		GLFWcursor* m_cursor = nullptr;

		WindowHints* m_hints = nullptr;

	public:
		WindowBuilder& copySettings(const WindowBuilder& other) {
			m_title = other.m_title;
			m_width = other.m_width;
			m_height = other.m_height;
			m_xpos = other.m_xpos;
			m_ypos = other.m_ypos;

			m_monitor = other.m_monitor;
			m_share = other.m_share;
			m_cursor = other.m_cursor;
			m_hints = other.m_hints;

			return *this;
		}

		WindowBuilder& setTitle(std::string_view title) noexcept {
			m_title = title;
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

		WindowBuilder& setHints(WindowHints* hints = nullptr) noexcept {
			m_hints = hints;
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

		[[nodiscard]]
		auto getHints() const noexcept {
			return m_hints;
		}

		[[nodiscard]]
		auto build() const {
			bool visible = !m_hints || m_hints->getHintOr(GLFW_VISIBLE, GLFW_TRUE) == GLFW_TRUE;

			if (m_hints) {
				m_hints->apply();
			}

			if (!visible || this->m_xpos || this->m_ypos) {
				glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
			}
			else {
				glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
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

			return sndx::glfw::Window{ window };
		}
	};
}