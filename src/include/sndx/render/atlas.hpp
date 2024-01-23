#pragma once

#include "texture.hpp"

#include "../data.hpp"

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
					float yoff = dims.y * float(height - 1 - y) + dims.y * 0.5f / float(tex.height);

					offsets[x + y * width] = glm::vec2(xoff, yoff);
				}
			}
		}

		void bind(size_t id = 0) const {
			tex.bind(id);
		}

		[[nodiscard]]
		glm::vec2 cellDims() const {
			return { 1.0f / float(width), 1.0f / float(height) };
		}

		// half pixel corrected dimensions
		[[nodiscard]]
		glm::vec2 cellDimsAdj() const {
			auto base = cellDims();
			glm::vec2 texAdj{ float(tex.width), float(tex.height) };
			base.x -= base.x / texAdj.x;
			base.y -= base.y / texAdj.y;
			return base;
		}

		[[nodiscard]]
		glm::vec2 getOffset(size_t x, size_t y) const {
			return getOffset(x + y * width);
		}

		[[nodiscard]]
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

		[[nodiscard]]
		auto& getCell(const IdT& id) {
			return entries.at(id);
		}

		[[nodiscard]]
		const auto& getCell(const IdT& id) const {
			return entries.at(id);
		}

		[[nodiscard]]
		bool contains(const IdT& id) const {
			return entries.contains(id);
		}

		[[nodiscard]]
		decltype(auto) find(const IdT& id) const {
			return entries.find(id);
		}

		decltype(auto) begin() const {
			return entries.begin();
		}

		decltype(auto) end() const {
			return entries.end();
		}

		void bind(size_t id = 0) const {
			tex.bind(id);
		}

		// currently only supports string based atlases
		bool save(const std::string& atlasname) const {

			std::string imgPath = atlasname + ".jpg";

			Data root("img", imgPath);
			
			Data offsets(DataDict{});
			
			for (const auto& [id, entry] : entries) {
				Data data("x", entry.first.x);
				data.append("y", entry.first.y);
				data.append("wid", entry.second.x);
				data.append("hig", entry.second.y);

				offsets.append(std::to_string(id), std::move(data));
			}

			root.append("offsets", std::move(offsets));

			if (encodeData<JSONencoder>(atlasname + ".atlas", root)) {
				tex.save(imgPath.c_str());
				return true;
			}
			
			return false;
		}

		void destroy() {
			tex.destroy();
			entries.clear();
		}
	};
}