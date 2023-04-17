#pragma once

#include "widget.hpp"

#include "../util/datafile.hpp"

#include <type_traits>

namespace sndx {

	namespace widget {

		template <class DataT>
		struct DataNodeView : Widget<DataNodeView<DataT>> {
			const DataNode<DataT>* target;

			DataNodeView(const DataNode<DataT>& data) :
				target(&data) {}

			void View(const std::string& id) const {
				if constexpr (std::is_same_v<std::string, DataT>) {
					ImGui::Text("%s : \"%s\"", id.c_str(), target->data.c_str());
				}
				else if constexpr (std::is_integral_v<DataT>) {
					ImGui::Text("%s : %d", id.c_str(), target->data);
				}
				else {
					ImGui::Text("%s : %p", id.c_str(), &target->data);
				}
			}
		};

		template <class DataT, typename CharT>
		struct DirNodeView : Widget<DirNodeView<DataT, CharT>> {

			const DirectoryNode<DataT, CharT>* target;

			DirNodeView(const DirectoryNode<DataT, CharT>& dir):
				target(&dir) {}

			void View(const std::string& id) const {

				ImGui::SetNextItemOpen(true, ImGuiTreeNodeFlags_DefaultOpen);
				if (ImGui::TreeNode(id.c_str())) {

					for (const auto& [id, node] : target->data) {
						if (std::holds_alternative<DataNode<DataT>>(node)) {
							DataNodeView data(std::get<DataNode<DataT>>(node));
							data.View(id);
						}
						else {
							DirNodeView dir(std::get<DirectoryNode<DataT, CharT>>(node));
							dir.View(id);
						}
					}

					ImGui::TreePop();
				}
			}
		};

		template <class DataT, typename CharT>
		struct DataTreeView : Widget<DataTreeView<DataT, CharT>> {

			const DataTree<DataT, CharT>* target;

			DataTreeView(const DataTree<DataT, CharT>& data) :
				target(&data) {}

			void View() const {
				if (std::holds_alternative<DataNode<DataT>>(target->root)) {
					DataNodeView data(std::get<DataNode<DataT>>(target->root));
					data.View("Root");
				}
				else {
					DirNodeView dir(std::get<DirectoryNode<DataT, CharT>>(target->root));
					dir.View("Root");
				}
			}
		};
	}

	template <class DataT, typename CharT>
	void DataTreeView(const DataTree<DataT, CharT>& tree, const char* name, ImGuiWindowFlags flags = 0) {
		widget::DataTreeView<DataT, CharT> view(tree);
		if (view.Begin(name, (bool*)0, flags)) {
			view.View();
		}
		view.End();
	}
}