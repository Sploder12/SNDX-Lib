#pragma once

#include "texture.hpp"

#include <unordered_map>
#include <set>

namespace sndx {

	// A texture atlas that consists of equally sized and spaced elements
	struct RegularAtlas {
		Texture tex;

		//width and height of the texture in CELLS!! (not pixels)
		size_t width, height;

		// precomputed offsets to save some time
		std::vector<glm::vec2> offsets;

		explicit RegularAtlas() :
			tex(), width(0), height(0), offsets{} {}

		// width and height are in cells! not pixels!!
		RegularAtlas(Texture data, size_t width, size_t height):
			tex(data), width(width), height(height), offsets{} {

			resize(width, height);
		}

		void resize(size_t newWidth, size_t newHeight) {
			if (newWidth <= 0 || newHeight <= 0) [[unlikely]] {
				throw std::runtime_error("Atlas has dimension of 0");
			}

			width = newWidth;
			height = newHeight;

			updateOffsets();
		}

		void updateOffsets() {
			offsets.resize(width * height);

			auto dims = cellDims();

			for (int x = 0; x < width; ++x) {
				for (int y = 0; y < height; ++y) {
					float xoff = dims.x * float(x) + dims.x * 0.5f / float(tex.width);
					float yoff = dims.y * float(height - 1 - y) + dims.y * 0.5 / float(tex.height);

					offsets[x + y * width] = glm::vec2(xoff, yoff);
				}
			}
		}

		inline void bind(size_t id = 0) const {
			tex.bind(id);
		}

		glm::vec2 cellDims() const {
			return { 1.0f / float(width), 1.0f / float(height) };
		}

		// half pixel corrected dimensions
		glm::vec2 cellDimsAdj() const {
			auto base = cellDims();
			glm::vec2 texAdj{ float(tex.width), float(tex.height) };
			base.x -= base.x / texAdj.x;
			base.y -= base.y / texAdj.y;
			return base;
		}

		glm::vec2 getOffset(size_t x, size_t y) const {
			return getOffset(x + y * width);
		}

		glm::vec2 getOffset(size_t id) const {
			if (id >= offsets.size()) [[unlikely]] return { -1.0, -1.0 };
			return offsets[id];
		}

		void destroy() {
			tex.destroy();
			width = 0;
			height = 0;
			offsets.clear();
		}
	};
	
	template <class IdT = std::string>
	struct Atlas {
		Texture tex{};

		std::unordered_map<IdT, std::pair<glm::vec2, glm::vec2>> entries{};


		auto& getCell(const IdT& id) {
			return entries.at(id);
		}

		const auto& getCell(const IdT& id) const {
			return entries.at(id);
		}

		inline void bind(size_t id = 0) const {
			tex.bind(id);
		}

		inline void destroy() {
			tex.destroy();
			entries.clear();
		}
	};


	template <class IdT = std::string>
	struct AtlasBuilder {

		template <class idT = IdT>
		struct Entry {
			idT id;
			ImageData data;

			bool operator>(const Entry& other) const {
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
			int maxHeight = shelfHeight;

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
						totalHeight += currentShelf.height + padding;
						shelves.emplace_back(std::move(currentShelf));
						shelfHeight = entry.data.height;
						currentShelf = Shelf{ shelfHeight, shelfWidth };
						currentShelf.addEntry(entry);
					}
				}
			}

			if (!currentShelf.entries.empty()) {
				totalHeight += currentShelf.height + padding;
				shelves.emplace_back(std::move(currentShelf));
			}

			ImageData outImg{};
			outImg.height = totalHeight + padding;
			outImg.width = shelfWidth + padding;
			outImg.channels = maxChannels;

			outImg.data.resize(outImg.width * outImg.height * outImg.channels, 0);
			
			Atlas<IdT> outAtlas{};
			
			int x = padding;
			int y = padding;

			for (const auto& shelf : shelves) {
				for (const auto& entry : shelf.entries) {
					glm::vec2 xy = {(float)(x) / (float)(outImg.width) , (float)(y) / (float)(outImg.height - 1)};
					glm::vec2 dims = { (float)(entry->data.width) / (float)(outImg.width), (float)(entry->data.height) / (float)(outImg.height - 1) };
					outAtlas.entries.emplace(entry->id, std::pair<glm::vec2, glm::vec2>{ xy, dims });

					for (int i = 0; i < entry->data.height; ++i) {
						int outY = (y + (entry->data.height - i)) * outImg.width * outImg.channels;
						int inY = i * entry->data.width * entry->data.channels;

						for (int j = 0; j < entry->data.width; ++j) {

							int idxOut = (x + j) * outImg.channels + outY;
							int idxIn = j * entry->data.channels + inY;

							if (idxOut >= outImg.data.size() || idxIn >= entry->data.data.size()) {
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
			
			outAtlas.tex = textureFromImage(outImg, formatFromChannels(outImg.channels), GL_LINEAR);
			
			outImg.save("testsdf.jpg");

			return outAtlas;
		}
	};
}