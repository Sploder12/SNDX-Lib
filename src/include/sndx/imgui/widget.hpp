#pragma once

#include <imgui.h>
#include <algorithm>

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace sndx {
	template <class T>
	struct Widget {
		static auto Begin(const char* name, bool* p_open = (bool*)0, ImGuiWindowFlags flags = 0) {
			return ImGui::Begin(name, p_open, flags);
		}

		template <class... Args>
		auto WidgetView(const Args&&... args) {
			return static_cast<T*>(this)->View(std::forward<Args>(args)...);
		}

		static void End() {
			ImGui::End();
		}
	};

	inline void ImGuiTerminate(ImGuiContext* context = (ImGuiContext*)0) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext(context);
	}

	inline void ImGuiNewFrame() {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	// this also renders the frame using GetDrawData
	inline void ImGuiEndFrame() {
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		ImGui::EndFrame();
	}
}