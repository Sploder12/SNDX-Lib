#pragma once

#define NOMINMAX
#include <GL/glew.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <optional>

namespace sndx {
	struct Viewport {
		glm::ivec2 offset{0};
		glm::ivec2 dims{1};

		std::optional<float> aspectRatio{};

		constexpr Viewport(glm::ivec2 dims, glm::ivec2 offset = glm::vec2(0), std::optional<float> aspectRatio = std::nullopt) :
			offset(offset), dims(dims), aspectRatio(aspectRatio) {
		
			if (aspectRatio) {
				resize(dims);
			}
		}

		[[nodiscard]]
		constexpr std::optional<float> getAspectRatio() const {
			return aspectRatio;
		}

		[[nodiscard]]
		constexpr glm::vec2 pixToNDC(glm::vec2 in) const {
			in -= offset;
			return (glm::vec2(in.x, dims.y - in.y) / glm::vec2(dims)) * 2.0f - glm::vec2(1.0f);
		}

		[[nodiscard]]
		constexpr glm::vec2 NDCtoPix(glm::vec2 ndc) const {
			auto tmp = (ndc + glm::vec2(1.0f)) / 2.0f;
			return glm::vec2(dims) * tmp;
		}

		void setViewport() const {
			glViewport(offset.x, offset.y, dims.x, dims.y);
		}

		constexpr void resetAspectRatio() {
			aspectRatio.reset();
		}

		constexpr void setAspectRatio(float value) {
			aspectRatio = value;
		}

		constexpr void resize(glm::ivec2 targetDims) {
			if (targetDims.x <= 0 && targetDims.y <= 0) [[unlikely]] return;

			int asWidth = std::max(int(targetDims.y * aspectRatio.value_or(1.0f)), 1);

			if (asWidth == targetDims.x) [[unlikely]] {
				offset = glm::ivec2(0);
				dims = targetDims;
				return;
			}

			if (targetDims.x == 0) {
				offset = glm::ivec2(0);
				dims = glm::ivec2(asWidth, targetDims.y);
				return;
			}

			int asHeight = std::max(int(targetDims.x / aspectRatio.value_or(1.0f)), 1);

			if (targetDims.y == 0) {
				offset = glm::ivec2(0);
				dims = glm::ivec2(targetDims.x, asHeight);
				return;
			}

			if (targetDims.x > asWidth) {
				int padding = (targetDims.x - asWidth) / 2;
				dims = glm::ivec2(asWidth, targetDims.y);
				offset = glm::ivec2(padding, 0);
				return;
			}

			int padding = (targetDims.y - asHeight) / 2;
			dims = glm::ivec2(targetDims.x, asHeight);
			offset = glm::ivec2(0, padding);
		}
	};
}