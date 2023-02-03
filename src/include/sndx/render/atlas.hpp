#pragma once

#include "texture.hpp"

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

	struct Atlas {

	};
}