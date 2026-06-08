#pragma once

#include <imgui.h>

namespace ImGui {
	template <class T> [[nodiscard]]
	constexpr ImGuiDataType datatype() noexcept {
		if constexpr (std::is_same_v<double, T>) {
			return ImGuiDataType_Double;
		}
		else if constexpr (std::is_same_v<float, T>) {
			return ImGuiDataType_Float;
		}
		else if constexpr (std::is_same_v<uint64_t, T>) {
			return ImGuiDataType_U64;
		}
		else if constexpr (std::is_same_v<uint32_t, T>) {
			return ImGuiDataType_U32;
		}
		else if constexpr (std::is_same_v<uint16_t, T>) {
			return ImGuiDataType_U16;
		}
		else if constexpr (std::is_same_v<uint8_t, T>) {
			return ImGuiDataType_U8;
		}
		else if constexpr (std::is_same_v<int64_t, T>) {
			return ImGuiDataType_S64;
		}
		else if constexpr (std::is_same_v<int32_t, T>) {
			return ImGuiDataType_S32;
		}
		else if constexpr (std::is_same_v<int16_t, T>) {
			return ImGuiDataType_S16;
		}
		else if constexpr (std::is_same_v<int8_t, T>) {
			return ImGuiDataType_S8;
		}
		else {
			static_assert(false);
		}
	}
}