#pragma once

#include <glm/glm.hpp>

#include <string>
#include <string_view>

namespace sndx {
	// @TODO cursors

	class Window {
	public:
		[[nodiscard]]
		virtual glm::ivec2 getPosition() const = 0;
		[[nodiscard]]
		virtual glm::ivec2 getSize() const = 0;

		[[nodiscard]]
		virtual glm::ivec2 getFramebufferSize() const = 0;

		[[nodiscard]]
		virtual glm::vec2 getContentScale() const = 0;

		[[nodiscard]]
		virtual bool isMouseCaptured() const = 0;

		[[nodiscard]]
		virtual bool isVisible() const = 0;

		[[nodiscard]]
		virtual bool isFocused() const = 0;

		[[nodiscard]] // valid until next call
		virtual std::string_view getTitle() const = 0;

		[[nodiscard]]
		virtual bool shouldClose() const = 0;
		virtual void signalClose() = 0;

		virtual void bind() = 0;
		virtual void swapBuffers() = 0;

		virtual void setPosition(const glm::ivec2& pos) = 0;
		virtual void resize(const glm::ivec2& dims) = 0;

		virtual void captureMouse(bool capture) = 0;

		virtual void forceFocus() = 0;
		virtual void requestAttention() = 0;

		virtual void setVisibility(bool visible) = 0;

		virtual void setTitle(const std::string& title) = 0;

		virtual ~Window() = default;
	};
}