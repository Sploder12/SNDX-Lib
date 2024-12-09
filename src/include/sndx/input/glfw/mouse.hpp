#pragma once

#include "./glfw.hpp"
#include "../../render/image/imagedata.hpp"

namespace sndx::input::glfw {
	class Cursor {
	private:
		GLFWcursor* m_cursor{};

	public:
		[[nodiscard]]
		static bool supportsRawMouseMotion() const noexcept {
			return glfwRawMouseMotionSupported() == GLFW_TRUE;
		}

		operator GLFWcursor* () const noexcept {
			return m_cursor;
		}

		operator bool() const noexcept {
			return m_cursor;
		}

		constexpr explicit Cursor(int shape) noexcept :
			m_cursor(glfwCreateStandardCursor(shape)) {}

		constexpr Cursor(const render::ImageData& img, int xhot, int yhot) noexcept {
			if (img.channels() == 4) {
				GLFWimage gimg{
					img.width(), img.height(), (unsigned char*)(img.data())
				};

				m_cursor = glfwCreateCursor(&gimg, xhot, yhot);
			}
		}

		Cursor(const Cursor&) = delete;
		Cursor(Cursor&& other) noexcept :
			m_cursor(std::exchange(other.m_cursor, nullptr)) {
		}

		Cursor& operator=(const Cursor&) = delete;
		Cursor& operator=(Cursor&& other) noexcept {
			std::swap(m_cursor, other.m_cursor);
			return *this;
		}

		~Cursor() noexcept {
			glfwDestroyCursor(std::exchange(m_cursor, nullptr));
		}
	};
}