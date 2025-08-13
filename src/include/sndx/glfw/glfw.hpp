#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <string>
#include <string_view>

#include "../data/version.hpp"

namespace sndx::glfw {
	struct GLFW {
		GLFW() {
			if (!glfwInit()) {
				auto [ec, err_str] = getLastError();
				throw std::runtime_error("Cannot init GLFW: " + err_str);
			}
		}

		~GLFW() noexcept {
			glfwTerminate();
		}

		GLFW(GLFW&&) = delete;
		GLFW(const GLFW&) = delete;
		GLFW& operator=(GLFW&&) = delete;
		GLFW& operator=(const GLFW&) = delete;

		[[nodiscard]]
		static std::pair<int, std::string> getLastError() noexcept {
			const char* desc = nullptr;
			auto ec = glfwGetError(&desc);

			return { ec, ec != GLFW_NO_ERROR ? std::string(desc) : "" };
		}

		[[nodiscard]]
		static Version getVersion() noexcept {
			Version v{};
			glfwGetVersion(&v.major, &v.minor, &v.patch);
			return v;
		}

		[[nodiscard]]
		static std::string_view getStringVersion() noexcept {
			static std::string_view str = glfwGetVersionString();
			return str;
		}

		[[nodiscard]]
		std::string getKeyName(int key, int scancode) const noexcept {
			auto cptr = glfwGetKeyName(key, scancode);

			return cptr ? std::string(cptr) : "";
		}

		[[nodiscard]]
		int getKeyScancode(int key) const noexcept {
			return glfwGetKeyScancode(key);
		}
	};
}