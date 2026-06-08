#pragma once

#include "./imcommon.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> 

namespace ImGui {
	template <glm::length_t L, class T, glm::qualifier Q>
	auto InputVec(const char* label, glm::vec<L, T, Q>& vec, const T* p_step = nullptr, const T* p_step_fast = nullptr, const char* fmt = nullptr, ImGuiInputFlags flags = ImGuiInputFlags_None) {
		return ImGui::InputScalarN(label, datatype<T>(), glm::value_ptr(vec), L, p_step, p_step_fast, fmt, flags);
	}

	template <glm::length_t L, class T, glm::qualifier Q>
	auto DragVec(const char* label, glm::vec<L, T, Q>& vec, float v_speed = 1.0f, const T* p_min = nullptr, const T* p_max = nullptr, const char* fmt = nullptr, ImGuiInputFlags flags = ImGuiInputFlags_None) {
		return ImGui::DragScalarN(label, datatype<T>(), glm::value_ptr(vec), L, v_speed, p_min, p_max, fmt, flags);
	}

	template <glm::length_t L, class T, glm::qualifier Q>
	auto SliderVec(const char* label, glm::vec<L, T, Q>& vec, const T& v_min, const T& v_max, const char* fmt = nullptr, ImGuiInputFlags flags = ImGuiInputFlags_None) {
		return ImGui::SliderScalarN(label, datatype<T>(), glm::value_ptr(vec), L, &v_min, &v_max, fmt, flags);
	}

	template <class T, glm::qualifier Q>
	auto InputEulerDegrees(const char* label, glm::qua<T, Q>& quat, const T* p_step = nullptr, const T* p_step_fast = nullptr, const char* fmt = nullptr, ImGuiInputFlags flags = ImGuiInputFlags_None) {
		auto eulers = glm::degrees(glm::eulerAngles(quat));
		if (InputVec(label, eulers, p_step, p_step_fast, fmt, flags)) {
			quat = glm::quat{ glm::radians(eulers) };
			return true;
		}
		return false;
	}

	template <class T, glm::qualifier Q>
	auto DragEulerDegrees(const char* label, glm::qua<T, Q>& quat, float v_speed = 1.0f, const T* p_min = nullptr, const T* p_max = nullptr, const char* fmt = nullptr, ImGuiInputFlags flags = ImGuiInputFlags_None) {
		auto eulers = glm::degrees(glm::eulerAngles(quat));
		if (DragVec(label, eulers, v_speed, p_min, p_max, fmt, flags)) {
			quat = glm::quat{ glm::radians(eulers) };
			return true;
		}
		return false;
	}

	template <class T, glm::qualifier Q>
	auto SliderEulerDegrees(const char* label, glm::qua<T, Q>& quat, const T& v_min, const T& v_max, const char* fmt = nullptr, ImGuiInputFlags flags = ImGuiInputFlags_None) {
		auto eulers = glm::degrees(glm::eulerAngles(quat));
		if (SliderVec(label, eulers, v_min, v_max, fmt, flags)) {
			quat = glm::quat{ glm::radians(eulers) };
			return true;
		}
		return false;
	}
}
