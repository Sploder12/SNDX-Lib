#include "render/viewport.hpp"

#include <gtest/gtest.h>

using namespace sndx::render;
using namespace sndx::collision;

void testPixConv(const Viewport<>& v, glm::vec2 pix, glm::vec2 ndc) {
	EXPECT_EQ(v.pixToNDC(pix), ndc);
	EXPECT_EQ(v.NDCtoPix(ndc), pix);
}

void testViewport(glm::vec2 offset, glm::vec2 scale) {
	Viewport v{ scale, offset };

	EXPECT_EQ(v.getOffset(), offset);
	EXPECT_EQ(v.getDimensions(), glm::vec2(scale));
	EXPECT_DOUBLE_EQ(v.getAspectRatio(), double(scale.x) / double(scale.y));

	testPixConv(v, glm::vec2(0.0) + offset, glm::vec2(-1.0));
	testPixConv(v, scale + offset, glm::vec2(1.0));
	testPixConv(v, scale * 0.5f + offset, glm::vec2(0.0));

	testPixConv(v, glm::vec2(0.0, scale.y) + offset, glm::vec2(-1.0, 1.0));
	testPixConv(v, glm::vec2(scale.x, 0.0) + offset, glm::vec2(1.0, -1.0));
}

TEST(Viewport, NormalViewport) {

	EXPECT_THROW(Viewport{ glm::vec2(0.0) }, std::invalid_argument);
	EXPECT_THROW(Viewport{ glm::vec2(-1.0) }, std::invalid_argument);

	for (float scaleX = 0.25f; scaleX <= 2.0f; scaleX += 0.25f) {
		for (float scaleY = 0.25f; scaleY <= 2.0f; scaleY += 0.25f) {
			for (float offsetX = -2.0f; offsetX <= 2.0f; offsetX += 0.25f) {
				for (float offsetY = -2.0f; offsetY <= 2.0f; offsetY += 0.25f) {
					auto scale = glm::vec2(scaleX, scaleY);
					auto offset = glm::vec2(offsetX, offsetY);

					testViewport(offset, scale);
				}
			}
		}
	}
}

TEST(Viewport, AspectRatioViewport) {
	EXPECT_THROW((AspectRatioViewport{ glm::vec2(0.0), 1.0f }), std::invalid_argument);
	EXPECT_THROW((AspectRatioViewport{ glm::vec2(-1.0), 1.0f }), std::invalid_argument);
	EXPECT_THROW((AspectRatioViewport{ glm::vec2(1.0), 0.0f }), std::invalid_argument);
	EXPECT_THROW((AspectRatioViewport{ glm::vec2(1.0), -1.0f }), std::invalid_argument);

	AspectRatioViewport v{ glm::vec2(2.0, 1.0), 1.0f };

	EXPECT_THROW(v.setAspectRatio(0.0), std::invalid_argument);
	EXPECT_THROW(v.setAspectRatio(-1.0), std::invalid_argument);

	EXPECT_THROW(v.setAlignment(glm::vec2(-0.1f)), std::invalid_argument);
	EXPECT_THROW(v.setAlignment(glm::vec2(1.1f)), std::invalid_argument);

	EXPECT_EQ(v.getAspectRatio(), 1.0);
	EXPECT_EQ(v.getOffset(), glm::vec2(0.5f, 0.0f));
	EXPECT_EQ(v.getDimensions(), glm::vec2(1.0f));

	v.resize(glm::vec2(3.0f, 4.0f));

	EXPECT_EQ(v.getAspectRatio(), 1.0);
	EXPECT_EQ(v.getOffset(), glm::vec2(0.0f, 0.5f));
	EXPECT_EQ(v.getDimensions(), glm::vec2(3.0f));

	[[maybe_unused]]
	const auto& r = static_cast<const Rect<glm::vec2>&>(v);
}

TEST(Viewport, ResizeThrowsOnBadSize) {
	constexpr auto size = glm::vec2(1.0f);
	constexpr auto badSize = glm::vec2(0.0f);
	Viewport v{ size };
	AspectRatioViewport arv{ size, 1.0f };

	EXPECT_THROW(v.resize(badSize), std::invalid_argument);
	EXPECT_THROW(arv.resize(badSize), std::invalid_argument);
}
