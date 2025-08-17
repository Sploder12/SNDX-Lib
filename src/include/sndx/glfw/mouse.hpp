#pragma once

#include "./glfw.hpp"
#include "../render/image/imagedata.hpp"

namespace sndx::glfw {
	class Cursor {
	private:
		GLFWcursor* m_cursor{};

	public:
		[[nodiscard]]
		static bool supportsRawMouseMotion() noexcept {
			return glfwRawMouseMotionSupported() == GLFW_TRUE;
		}

		operator GLFWcursor* () const noexcept {
			return m_cursor;
		}

		operator bool() const noexcept {
			return m_cursor;
		}

		explicit Cursor(int shape) noexcept :
			m_cursor(glfwCreateStandardCursor(shape)) {}

		Cursor(const render::ImageData& img, int xhot, int yhot) {
			if (img.channels() == 4) {
				GLFWimage gimg{
					int(img.width()), int(img.height()), (unsigned char*)(img.data())
				};

				m_cursor = glfwCreateCursor(&gimg, xhot, yhot);
			}
			else [[unlikely]] {
				throw std::logic_error("Provided cursor image does not have 4 channels");
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