#pragma once

#include "widget.hpp"

#include "../data/encoder.hpp"

#include <type_traits>

namespace sndx {

	namespace widget {

		template <class Enc = SNDXencoder>
		struct DataPrimitiveView : Widget<DataPrimitiveView<Enc>> {
			const Primitive* target;

			DataPrimitiveView(const Primitive& data) :
				target(&data) {}

			void View() const {
				auto encoding = Encoder<Enc>::encode(*target);
				ImGui::Text("%s", encoding.c_str());
			}

			void View(const char* id) const {
				auto encoding = Encoder<Enc>::encode(*target);
				ImGui::Text("%s %c %s", id, Enc::encoderScheme.keyDelim, encoding.c_str());
			}
		};

		template <class Enc>
		struct DataArrView;

		template <class Enc = SNDXencoder>
		struct DataDictView : Widget<DataDictView<Enc>> {

			const DataDict* target;

			DataDictView(const DataDict& dict):
				target(&dict) {}

			void View(const char* tid = "-") const {

				ImGui::SetNextItemOpen(true, ImGuiTreeNodeFlags_DefaultOpen);
				if (ImGui::TreeNode(tid)) {

					for (const auto& [id, val] : *target) {
						const auto& node = val.data;
						if (std::holds_alternative<Primitive>(node)) {
							DataPrimitiveView<Enc> data(std::get<Primitive>(node));
							data.View(id.c_str());
						}
						else if (std::holds_alternative<DataDict>(node)) {
							DataDictView<Enc> dir(std::get<DataDict>(node));
							dir.View(id.c_str());
						}
						else {
							DataArrView<Enc> arr(std::get<DataArray>(node));
							arr.View(id.c_str());
						}
					}

					ImGui::TreePop();
				}
			}
		};

		template <class Enc = SNDXencoder>
		struct DataArrView : Widget<DataArrView<Enc>> {

			const DataArray* target;

			DataArrView(const DataArray& dict) :
				target(&dict) {}

			void View(const char* tid = "-") const {

				ImGui::SetNextItemOpen(true, ImGuiTreeNodeFlags_DefaultOpen);
				if (ImGui::TreeNode(tid)) {

					for (size_t i = 0; i < target->size(); ++i) {
						std::string idx = std::to_string(i);
						const auto& node = (*target)[i].data;

						if (std::holds_alternative<Primitive>(node)) {
							DataPrimitiveView<Enc> data(std::get<Primitive>(node));
							data.View(idx.c_str());
						}
						else if (std::holds_alternative<DataDict>(node)) {
							DataDictView<Enc> dir(std::get<DataDict>(node));
							dir.View(idx.c_str());
						}
						else {
							DataArrView<Enc> arr(std::get<DataArray>(node));
							arr.View(idx.c_str());
						}
					}

					ImGui::TreePop();
				}
			}
		};

		template <class Enc>
		struct DataTreeView : Widget<DataTreeView<Enc>> {

			const Data* target;

			DataTreeView(const Data& data) :
				target(&data) {}

			void View(const char* id = "root") const {
				const auto& node = target->data;
				if (std::holds_alternative<Primitive>(node)) {
					DataPrimitiveView<Enc> data(std::get<Primitive>(node));
					data.View(id);
				}
				else if (std::holds_alternative<DataDict>(node)) {
					DataDictView<Enc> dir(std::get<DataDict>(node));
					dir.View(id);
				}
				else {
					DataArrView<Enc> arr(std::get<DataArray>(node));
					arr.View(id);
				}
			}
		};
	}

	template <class Enc = SNDXencoder>
	void DataTreeView(const Data& tree, const char* name, ImGuiWindowFlags flags = 0) {
		widget::DataTreeView<Enc> view(tree);
		if (view.Begin(name, (bool*)0, flags)) {
			view.View();
		}
		view.End();
	}
}