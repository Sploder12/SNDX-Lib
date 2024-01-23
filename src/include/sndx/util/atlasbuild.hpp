#pragma once

#include "../render/atlas.hpp"

namespace sndx {
	template <class IdT = std::string>
	struct AtlasBuilder {

		template <class idT = IdT>
		struct Entry {
			idT id;
			ImageData data;

			bool operator>(const Entry& other) const noexcept {
				return data.height > other.data.height;
			}
		};

		// sorted by height in descending order
		std::multiset<Entry<IdT>, std::greater<Entry<IdT>>> entries{};

		auto insert(const IdT& id, const ImageData& data) {
			return entries.emplace(id, data);
		}

		template <size_t padding = 2>
		Atlas<IdT> buildAtlas(float widthBias = 4.0f) const {
			// uses a modified Next-Fit Decreasing Height algorithm.
			struct Shelf {
				int height, width;
				int occupiedWidth;

				std::vector<const Entry<IdT>*> entries{};

				Shelf(int height, int width) :
					height(height), width(width), occupiedWidth(0) {}

				void addEntry(const Entry<IdT>& entry) {
					occupiedWidth += entry.data.width + padding;
					entries.emplace_back(&entry);
				}

				bool canAddEntry(const Entry<IdT>& entry) const {
					return (occupiedWidth + entry.data.width < width);
				}
			};

			int maxWidth = 0;
			int maxChannels = 0;
			for (const auto& entry : entries) {
				maxWidth = std::max(entry.data.width, maxWidth);
				maxChannels = std::max(entry.data.channels, maxChannels);
			}

			int shelfWidth = (int)(maxWidth * widthBias + padding * widthBias);
			int shelfHeight = entries.cbegin()->data.height;

			int maxShelfWidth = 0;

			int totalHeight = 0;

			std::vector<Shelf> shelves{};
			Shelf currentShelf{ shelfHeight, shelfWidth };

			for (const auto& entry : entries) {
				bool added = false;
				for (auto& prevShelf : shelves) {
					if (prevShelf.canAddEntry(entry)) {
						prevShelf.addEntry(entry);
						added = true;
					}
				}

				if (!added) {
					if (currentShelf.canAddEntry(entry))
						currentShelf.addEntry(entry);
					else {
						maxShelfWidth = std::max(maxShelfWidth, currentShelf.occupiedWidth);
						totalHeight += currentShelf.height + padding;
						shelves.emplace_back(std::move(currentShelf));
						shelfHeight = entry.data.height;
						currentShelf = Shelf{ shelfHeight, shelfWidth };
						currentShelf.addEntry(entry);
					}
				}
			}

			if (!currentShelf.entries.empty()) {
				maxShelfWidth = std::max(maxShelfWidth, currentShelf.occupiedWidth);
				totalHeight += currentShelf.height + padding;
				shelves.emplace_back(std::move(currentShelf));
			}

			ImageData outImg{};
			outImg.height = totalHeight + padding;
			outImg.width = maxShelfWidth + padding;
			outImg.channels = maxChannels;

			outImg.data.resize(outImg.width * outImg.height * outImg.channels, 0);

			Atlas<IdT> outAtlas{};

			int x = padding;
			int y = padding;

			for (const auto& shelf : shelves) {
				for (const auto& entry : shelf.entries) {
					glm::vec2 xy = { (float)(x) / (float)(outImg.width) , (float)(y) / (float)(outImg.height - 1) };
					glm::vec2 dims = { (float)(entry->data.width) / (float)(outImg.width), (float)(entry->data.height) / (float)(outImg.height - 1) };
					outAtlas.entries.emplace(entry->id, std::pair<glm::vec2, glm::vec2>{ xy, dims });

					for (int i = 0; i < entry->data.height; ++i) {
						int outY = (y + (entry->data.height - i)) * outImg.width * outImg.channels;
						int inY = i * entry->data.width * entry->data.channels;

						for (int j = 0; j < entry->data.width; ++j) {

							int idxOut = (x + j) * outImg.channels + outY;
							int idxIn = j * entry->data.channels + inY;

							if (idxOut + outImg.channels > outImg.data.size() || idxIn + entry->data.channels > entry->data.data.size()) {
								break;
							}

							for (int c = 0; c < entry->data.channels; ++c) {
								outImg.data[idxOut + c] = entry->data.data[idxIn + c];
							}

							for (int c = entry->data.channels; c < outImg.channels; ++c) {
								outImg.data[idxOut + c] = 255;
							}
						}
					}

					x += entry->data.width + padding;
				}
				y += shelf.height + padding;
				x = padding;
			}

			auto format = formatFromChannels(outImg.channels);
			outAtlas.tex = textureFromImage(std::move(outImg), format, GL_LINEAR);

			return outAtlas;
		}
	};
}