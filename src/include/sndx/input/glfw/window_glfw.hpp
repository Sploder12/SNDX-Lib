#pragma once

#include "../window_backend.hpp"

#define NOMINMAX
#include <GLFW/glfw3.h>

#include "../../render/viewport.hpp"

#include <type_traits>
#include <string>
#include <string_view>
#include <stdexcept>
#include <optional>
#include <unordered_map>


namespace sndx::input {
	template <template <typename, glm::qualifier> class ViewportT, typename InternalT = float, glm::qualifier Qualifier = glm::qualifier::defaultp>
		requires (std::is_base_of_v<sndx::render::Viewport<InternalT, Qualifier>, ViewportT<InternalT, Qualifier>>)
	class WindowGLFW : Window<WindowGLFW<ViewportT, InternalT, Qualifier>> {
	protected:
		friend Window<WindowGLFW<ViewportT, InternalT, Qualifier>>;

		ViewportT<InternalT, Qualifier> m_viewport;
		GLFWwindow* m_window = nullptr;

	public:
		using Viewport = decltype(m_viewport);
		using Vec = Viewport::Vec;

		template <class... Args>
		WindowGLFW(GLFWwindow* window, int width, int height, Args&&... args) :
			m_viewport{ Vec{width, height}, std::forward<Args>(args)... }, m_window{ window } {

			if (!m_window)
				throw std::runtime_error("Could not create glfwWindow");

			int fwidth, fheight;
			glfwGetFramebufferSize(m_window, &fwidth, &fheight);
			setViewportSize(Vec{ fwidth, fheight });

			glfwSetWindowUserPointer(m_window, this);
		}

		template <class... Args>
		WindowGLFW(std::nullptr_t, int, int, Args&&...) = delete;

	public:
		template <class... Args>
		WindowGLFW(const char* title, int width, int height, GLFWmonitor* monitor, GLFWwindow* share, Args&&... args) :
			Window(glfwCreateWindow(width, height, title, monitor, share), width, height, std::forward<Args>(args)...) {}

		template <class... Args>
		WindowGLFW(std::nullptr_t, int, int, GLFWmonitor*, GLFWwindow*, Args&&...) = delete;

		WindowGLFW(WindowGLFW&& other) noexcept :
			m_window(std::exchange(other.m_window, nullptr)),
			m_viewport(other.m_viewport) {

			glfwSetWindowUserPointer(m_window, this);
		}

		WindowGLFW& operator=(WindowGLFW&& other) noexcept {
			std::swap(m_viewport, other.m_viewport);
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

	protected:
		void bind() const noexcept {
			glfwMakeContextCurrent(m_window);
		}

		template <class ViewportCallback>
		void updateViewport(ViewportCallback&& callback) const noexcept {
			callback(*this, m_viewport.getOffset(), m_viewport.getDimensions());
		}

		void setViewportOffset(const Vec& offset) noexcept {
			m_viewport.setOffset(offset);
		}

		void setViewportSize(const Vec& newDims) {
			m_viewport.resize(newDims);
		}

		void setPosition(const Vec& pos) noexcept {
			glfwSetWindowPos(m_window, int(pos.x), int(pos.y));
		}

		template <class ViewportCallback>
		void resize(const Vec& dims, ViewportCallback&& callback) {
			glfwSetWindowSize(m_window, int(dims.x), int(dims.y));

			int fwidth, fheight;
			glfwGetFramebufferSize(m_window, &fwidth, &fheight);

			setViewportSize(Vec{ fwidth, fheight });
			updateViewport(std::forward<ViewportCallback>(callback));
		}

		[[nodiscard]]
		glm::ivec2 getPosition() const noexcept {
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

	class WindowHintsGLFW final: public WindowHints<WindowHintsGLFW> {
	protected:
		friend WindowHints<WindowHintsGLFW>;

		std::unordered_map<int, int> m_hints{};

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
	protected:
		friend WindowBuilder<WindowBuilderGLFW>;

		GLFWmonitor* m_monitor = nullptr;
		GLFWwindow* m_share = nullptr;
		GLFWcursor* m_cursor = nullptr;

	public:
		WindowBuilderGLFW() noexcept = default;

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
		auto build(const WindowHintsGLFW& hints, Args&&... args) const {
			hints.apply();
			return build(hints.getHintOr(GLFW_VISIBLE, GLFW_TRUE) == GLFW_TRUE, std::forward<Args>(args)...);
		}
	};
}