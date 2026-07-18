#pragma once

#include <imgui.h>

#ifdef GL_VERSION
#include <imgui_impl_opengl3.h>
#endif

#ifdef GLFW_VERSION_MAJOR
#include <imgui_impl_glfw.h>
#endif

namespace sndx::imgui {
	#ifdef GLFW_VERSION_MAJOR
	template <class RenderBackend>
	struct glfwBackend {
		template <class... Args>
		static void init(GLFWwindow* window, bool setupCallbacks, Args&&... args) {
			RenderBackend::initGLFW(window, setupCallbacks, std::forward<Args>(args)...);
		}

		static void destroy() {
			RenderBackend::destroy();
			ImGui_ImplGlfw_Shutdown();
		}

		static void newFrame() {
			RenderBackend::newFrame();
			ImGui_ImplGlfw_NewFrame();
		}

		static void endFrame() {
			RenderBackend::endFrame();
		}
	};
	#endif

	#ifdef GL_VERSION
	struct glBackend {
		static void init(const char* glslVersion) {
			ImGui_ImplOpenGL3_Init(glslVersion);
		}

		static void destroy() {
			ImGui_ImplOpenGL3_Shutdown();
		}

		#ifdef GLFW_VERSION_MAJOR
		template <class... Args>
		static void initGLFW(GLFWwindow* window, bool setupCallbacks, const char* glslVersion) {
			ImGui_ImplGlfw_InitForOpenGL(window, setupCallbacks);
			init(glslVersion);
		}
		#endif

		static void newFrame() {
			ImGui_ImplOpenGL3_NewFrame();
		}

		static void endFrame() {
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}
	};
	#endif

#ifdef GLFW_VERSION_MAJOR 
#ifdef GL_VERSION
	using glfw_gl_Backend = glfwBackend<glBackend>;
#endif
#endif
}

