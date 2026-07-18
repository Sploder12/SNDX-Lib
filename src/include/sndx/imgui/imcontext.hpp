#pragma once

#include <imgui.h>

#include <utility>

namespace sndx::imgui {
	template <class Backend>
	class Context {
	private:
		ImGuiContext* context{};

	public:
		[[nodiscard]]
		ImGuiContext* get() const noexcept {
			return context;
		}

		operator ImGuiContext* () const noexcept {
			return context;
		}

		static void unbind() {
			ImGui::SetCurrentContext(nullptr);
		}

		ImGuiContext* bind() const {
			auto old = ImGui::GetCurrentContext();
			ImGui::SetCurrentContext(context);
			return old;
		}

		template <class Fn>
		void doBind(Fn&& fn) const {
			auto old = bind();
			std::forward<Fn>(fn)();
			ImGui::SetCurrentContext(old);
		}

		template <class Fn, class... Args>
		Context(Fn&& configFn, ImFontAtlas* sharedFontAtlas = nullptr, Args&&... args):
			context(ImGui::CreateContext(sharedFontAtlas)) {
		
			std::forward<Fn>(configFn)(context);
			Backend::init(std::forward<Args>(args)...);
		}

		Context(Context&& other) noexcept:
			context(std::exchange(other.context, nullptr)) {}

		Context(const Context&) = delete;

		Context& operator=(Context&& other) noexcept {
			std::swap(context, other.context);
			return *this;
		}

		Context& operator=(const Context&) = delete;

		~Context() noexcept {
			if (context) {
				auto old = bind();
				Backend::destroy();
				ImGui::DestroyContext(context);
				ImGui::SetCurrentContext(context == old ? nullptr : old);
				context = nullptr;
			}
		}

		// recommended to call bind right before this
		static void newFrame() {
			Backend::newFrame();
			ImGui::NewFrame();
		}

		// recommended to call bind right before this
		static void endFrame() {
			ImGui::Render();
			Backend::endFrame();
		}
	};
}