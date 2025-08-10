#pragma once

#include <glm/glm.hpp>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#undef GLM_ENABLE_EXPERIMENTAL
#else
#include <glm/gtx/component_wise.hpp>
#endif

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <optional>

namespace sndx::input {
	template <class Backend>
	class Window {
	protected:
		Window() = default;

	public:
		using BackendType = Backend;

		void bind() const {
			static_cast<const Backend*>(this)->bindImpl();
		}

		template <class Vec>
		void setPosition(const Vec& pos) {
			static_cast<Backend*>(this)->setPositionImpl(pos);
		}

		template <class Vec>
		void resize(const Vec& dims) {
			if (glm::compMin(dims) < 1.0)
				throw std::invalid_argument("Window dims cannot be less than 1");

			static_cast<Backend*>(this)->resizeImpl(dims);
		}

		void focusWindow() {
			static_cast<Backend*>(this)->focusWindowImpl();
		}

		void requestAttention() {
			static_cast<Backend*>(this)->requestAttentionImpl();
		}

		void setVisibility(bool visible) {
			static_cast<Backend*>(this)->setVisibilityImpl(visible);
		}

		decltype(auto) setTitle(const std::string& newTitle) {
			return static_cast<Backend*>(this)->setTitleImpl(newTitle);
		}

		void tryClose() {
			static_cast<Backend*>(this)->tryCloseImpl();
		}

		[[nodiscard]]
		decltype(auto) getPosition() const {
			return static_cast<const Backend*>(this)->getPositionImpl();
		}

		[[nodiscard]]
		decltype(auto) getSize() const {
			return static_cast<const Backend*>(this)->getSizeImpl();
		}

		[[nodiscard]]
		bool isVisible() const {
			return static_cast<const Backend*>(this)->isVisibleImpl();
		}

		[[nodiscard]]
		const std::string& getTitle() const {
			return static_cast<const Backend*>(this)->getTitleImpl();
		}
	};

	template <class Backend>
	class WindowBuilder {
	protected:
		std::string m_title{};

		int m_width = 1;
		int m_height = 1;

		std::optional<int> m_xpos{}, m_ypos{};

		template <class OtherBuilder>
		Backend& copyBaseSettings(const OtherBuilder& other) {
			m_title = other.m_title;
			m_width = other.m_width;
			m_height = other.m_height;
			m_xpos = other.m_xpos;
			m_ypos = other.m_ypos;
			return static_cast<Backend&>(*this);
		}

	public:
		Backend& setTitle(std::string_view title) noexcept {
			m_title = title;
			return *static_cast<Backend*>(this);
		}

		Backend& setX(std::optional<int> x = std::nullopt) noexcept {
			m_xpos = x;
			return *static_cast<Backend*>(this);
		}

		Backend& setY(std::optional<int> y = std::nullopt) noexcept {
			m_ypos = y;
			return *static_cast<Backend*>(this);
		}

		Backend& setWidth(int width) noexcept {
			m_width = std::max(width, 1);
			return *static_cast<Backend*>(this);
		}

		Backend& setHeight(int height) noexcept {
			m_height = std::max(height, 1);
			return *static_cast<Backend*>(this);
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
		auto build() const {
			return static_cast<const Backend*>(this)->buildImpl();
		}
	};
}