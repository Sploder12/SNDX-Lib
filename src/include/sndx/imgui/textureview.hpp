#pragma once

#include "widget.hpp"

#include "../render/texture.hpp"
#include "../render/atlas.hpp"

namespace sndx {

	namespace widget {
		struct TextureView : Widget<TextureView> {

			const Texture* target;

			TextureView(const Texture& texture) :
				target(&texture) {}

			// note this vertically flips by default
			void View(const ImVec2& uv0 = ImVec2(0, 1), const ImVec2& uv1 = ImVec2(1, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1), const ImVec4& border_col = ImVec4(0, 0, 0, 0)) const {
				ImGui::Text("Texture ID: %u", target->id);
				ImGui::Text("Size: %u x %u", target->width, target->height);
				
				long long id = target->id;
				ImGui::Image((void*)id, ImVec2(float(target->width), float(target->height)), uv0, uv1, tint_col, border_col);
			}
		};

		template <typename IdT = std::string>
		struct AtlasView : Widget<AtlasView<IdT>> {
			const Atlas<IdT>* target;

			using iterator_t = decltype(target->entries.cbegin());

			AtlasView(const Atlas<IdT>& atlas) :
				target(&atlas) {}

			void View(iterator_t& cur) const {

				auto width = ImGui::GetWindowContentRegionWidth();
				long long id = target->tex.id;

				if (ImGui::BeginChild("Atlas Texture", ImVec2(width * 0.8f, 0), false, ImGuiWindowFlags_HorizontalScrollbar)) {

					ImGui::Image((void*)id, ImVec2(float(target->tex.width), float(target->tex.height)), ImVec2(0, 1), ImVec2(1, 0));

					if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
						const auto& pos = ImGui::GetMousePos();
						const auto& offset = ImGui::GetWindowPos();

						auto scrollX = ImGui::GetScrollX();
						auto scrollY = ImGui::GetScrollY();

						ImVec2 clickPos(pos.x - offset.x + scrollX, pos.y - offset.y + scrollY);

						auto it = target->entries.cbegin();
						for (; it != target->entries.cend(); ++it) {
							const auto& entry = it->second;
							glm::vec2 adjp1(entry.first.x * float(target->tex.width), (1.0f - entry.first.y) * float(target->tex.height));
							auto adjp2 = adjp1 + glm::vec2(entry.second.x * float(target->tex.width), -entry.second.y * float(target->tex.height));

							if (clickPos.x >= adjp1.x && clickPos.x <= adjp2.x && clickPos.y <= adjp1.y && clickPos.y >= adjp2.y) {
								cur = it;
								break;
							}
						}
					}
				}
				ImGui::EndChild();

				
				ImGui::SameLine();
				if (ImGui::BeginChild("Atlas Entries", ImVec2(width * 0.2f, 0))) {

					ImGui::Text("Texture ID: %u", target->tex.id);
					ImGui::Text("Size: %u x %u", target->tex.width, target->tex.height);

					if (ImGui::Button("<")) {
						if (cur == target->entries.cbegin()) {
							cur = target->entries.cend();
						}

						cur--;
					}

					ImGui::SameLine();
					if (ImGui::Button(">")) {
						cur++;

						if (cur == target->entries.cend()) {
							cur = target->entries.cbegin();
						}
					}

					ImGui::SameLine();
					if constexpr (std::is_same_v<std::string, IdT>) {
						ImGui::Text("Entry: %s", cur->first.c_str());
					}
					else {
						ImGui::Text("Entry: %d", cur->first);
					}

					const auto& pos = cur->second.first;
					const auto& dims = cur->second.second;

					ImVec2 size(dims.x * target->tex.width, dims.y * target->tex.height);

					ImGui::Text("Pos: %f, %f", pos.x, pos.y);
					ImGui::Text("Dims: %f, %f", dims.x, dims.y);
					ImGui::Text("Size: %d x %d", int(size.x), int(size.y));

					size.x *= 2.0f;
					size.y *= 2.0f;

					ImGui::Image((void*)id, size, ImVec2(pos.x, pos.y + dims.y), ImVec2(pos.x + dims.x, pos.y));
				}

				ImGui::EndChild();
			}
		};
	}

	inline void TextureView(const Texture& texture, const char* name, ImGuiWindowFlags flags = 0) {
		widget::TextureView view(texture);
		if (view.Begin(name, (bool*)0, flags)) {
			view.View();
		}
		view.End();
	}

	template <typename IdT = std::string>
	inline void AtlasView(const Atlas<IdT>& atlas, const char* name, typename widget::AtlasView<IdT>::iterator_t& it, ImGuiWindowFlags flags = 0) {
		widget::AtlasView<IdT> view(atlas);
		if (view.Begin(name, (bool*)0, flags)) {
			view.View(it);
		}
		view.End();
	}
}