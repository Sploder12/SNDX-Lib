#pragma once

#include <glm/glm.hpp>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#undef GLM_ENABLE_EXPERIMENTAL
#else
#include <glm/gtx/component_wise.hpp>
#endif

#include <stdexcept>
#include <optional>

namespace sndx::input {
	template <class Backend>
	class Window {
	protected:
		template <class ViewportCallback>
		void updateViewport(ViewportCallback&& callback) const {
			bind();
			static_cast<const Backend*>(this)->updateViewportImpl(std::forward<ViewportCallback>(callback));
		}
		
	public:
		using BackendType = Backend;

		void bind() const {
			static_cast<const Backend*>(this)->bindImpl();
		}

		template <class Vec, class ViewportCallback>
		void setViewportOffset(const Vec& offset, ViewportCallback&& callback) {
			static_cast<Backend*>(this)->setViewportOffsetImpl(offset);
			updateViewport(std::forward<ViewportCallback>(callback));
		}

		template <class Vec, class ViewportCallback>
		void setViewportSize(const Vec& newDims, ViewportCallback&& callback) {
			if (glm::compMin(newDims) < 1.0)
				throw std::invalid_argument("Viewport dims cannot be less than 1");

			static_cast<Backend*>(this)->setViewportSizeImpl(newDims);
			updateViewport(std::forward<ViewportCallback>(callback));
		}

		template <class Vec>
		void setPosition(const Vec& pos) {
			static_cast<Backend*>(this)->setPositionImpl(pos);
		}

		template <class Vec, class ViewportCallback>
		void resize(const Vec& dims, ViewportCallback&& callback) {
			if (glm::compMin(dims) < 1.0)
				throw std::invalid_argument("Window dims cannot be less than 1");

			static_cast<Backend*>(this)->resizeImpl(dims, std::forward<ViewportCallback>(callback));
		}

		[[nodiscard]]
		decltype(auto) getPosition() const noexcept {
			return static_cast<const Backend*>(this)->getPositionImpl();
		}

		[[nodiscard]]
		decltype(auto) getSize() const noexcept {
			return static_cast<const Backend*>(this)->getSizeImpl();
		}

		[[nodiscard]]
		decltype(auto) getViewport() const noexcept {
			return static_cast<const Backend*>(this)->getViewportImpl();
		}

		template <class Vec> [[nodiscard]]
		decltype(auto) pixToNDC(const Vec& in) const noexcept {
			return static_cast<const Backend*>(this)->pixToNDCImpl(in);
		}

		template <class Vec> [[nodiscard]]
		decltype(auto) NDCtoPix(const Vec& ndc) const noexcept {
			return static_cast<const Backend*>(this)->NDCtoPixImpl(ndc);
		}
	};

	template <class Backend>
	class WindowHints {
	public:
		static void restoreDefaults() noexcept {
			static_cast<Backend*>(this)->restoreDefaultsImpl();
		}

		template <class KeyT> [[nodiscard]]
		decltype(auto) getHint(const KeyT& id) const {
			return static_cast<const Backend*>(this)->getHintImpl(id);
		}

		template <class KeyT, class ValT> [[nodiscard]]
		decltype(auto) getHintOr(const KeyT& id, ValT&& alternative) const {
			return static_cast<const Backend*>(this)->getHintOrImpl(id, std::forward<ValT>(alternative));
		}

		template <class KeyT, class ValueT>
		decltype(auto) setHint(const KeyT& id, ValueT&& val) {
			return static_cast<Backend*>(this)->setHintImpl(id, std::forward<ValueT>(val));
		}

		template <class KeyT>
		bool removeHint(const KeyT& id) {
			return static_cast<Backend*>(this)->removeHintImpl(id);
		}

		void apply() const {
			static_cast<const Backend*>(this)->applyImpl();
		}
	};

	template <class Backend>
	class WindowBuilder {
	protected:
		std::string m_title{};

		int m_width = 1;
		int m_height = 1;

		std::optional<int> m_xpos{}, m_ypos{};

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

		template <class... Args> [[nodiscard]]
		decltype(auto) build(bool visible = true, Args&&... args) const {
			return static_cast<const Backend*>(this)->buildImpl(visible, std::forward<Args>(args)...);
		} 

		template <class WindowHintsT, class... Args> [[nodiscard]]
		decltype(auto) build(const WindowHintsT& hints, Args&&... args) const {
			return static_cast<const Backend*>(this)->buildImpl(hints, std::forward<Args>(args)...);
		} 
	};
}