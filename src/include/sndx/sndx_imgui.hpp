#pragma once

#define IMGUI_STRUCTWIDGET_FRIEND() \
	template <class T> \
	friend bool ImGui::StructWidget(T&)

#ifndef SNDX_NO_IMGUI
#include "imgui/imcommon.hpp"
#include "imgui/imcollision.hpp"
#include "imgui/imglm.hpp"
#endif