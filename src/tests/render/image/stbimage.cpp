#include "render/image/stbimage.hpp"

#include <gtest/gtest.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace sndx::render;

std::filesystem::path test_data_path{ L"test_data/visual/rgbbw_test_img📷.png" };

class STBimageTest : public ::testing::Test {
public:
	void SetUp() override {
#ifdef SNDX_NO_INTEGRATION_TESTS
		GTEST_SKIP() << "Integration tests disabled.";
#else

#endif
	}
};

TEST_F(STBimageTest, noFileReturnsNullopt) {
	auto img = loadImageFile(".", 4, STBimageLoader{});
	EXPECT_FALSE(img.has_value());
}

TEST_F(STBimageTest, noChannelThrows) {
	EXPECT_THROW(auto ign = loadImageFile(test_data_path, 0, STBimageLoader{}), std::invalid_argument);
}

TEST_F(STBimageTest, manyChannelsThrows) {
	EXPECT_THROW(auto ign = loadImageFile(test_data_path, 5, STBimageLoader{}), std::invalid_argument);
}

TEST_F(STBimageTest, LoadsRGBpng) {
	auto img = STBimageLoader{false}.loadFromFile(test_data_path, 3);

	ASSERT_TRUE(img.has_value());

	EXPECT_EQ(img->width(), 225);
	EXPECT_EQ(img->height(), 100);
	EXPECT_EQ(img->channels(), 3);
	EXPECT_EQ(img->pixels(), 225 * 100);
	EXPECT_EQ(img->bytes(), 225 * 100 * 3);

	using Vec = glm::vec<3, std::byte>;

	EXPECT_EQ(img->at<3>(0, 0), (Vec{ 255, 0, 0 }));
	EXPECT_EQ(img->at<3>(50, 0), (Vec{ 0, 255, 0 }));
	EXPECT_EQ(img->at<3>(100, 0), (Vec{ 0, 0, 255 }));
	EXPECT_EQ(img->at<3>(150, 0), (Vec{ 0, 0, 0 }));
	EXPECT_EQ(img->at<3>(190, 0), (Vec{ 255, 255, 255 }));

	EXPECT_EQ(img->at<3>(0, 99), (Vec{ 255, 0, 0 }));
	EXPECT_EQ(img->at<3>(50, 99), (Vec{ 0, 255, 0 }));
	EXPECT_EQ(img->at<3>(100, 99), (Vec{ 0, 0, 255 }));
	EXPECT_EQ(img->at<3>(150, 99), (Vec{ 255, 255, 255 }));
	EXPECT_EQ(img->at<3>(190, 99), (Vec{ 0, 0, 0 }));
}

TEST_F(STBimageTest, LoadsRGBApng) {
	auto img = STBimageLoader{ false }.loadFromFile(test_data_path, 4);

	ASSERT_TRUE(img.has_value());

	EXPECT_EQ(img->width(), 225);
	EXPECT_EQ(img->height(), 100);
	EXPECT_EQ(img->channels(), 4);
	EXPECT_EQ(img->pixels(), 225 * 100);
	EXPECT_EQ(img->bytes(), 225 * 100 * 4);

	using Vec = glm::vec<4, std::byte>;

	EXPECT_EQ(img->at<4>(0, 0), (Vec{ 255, 0, 0, 255 }));
	EXPECT_EQ(img->at<4>(50, 0), (Vec{ 0, 255, 0, 255 }));
	EXPECT_EQ(img->at<4>(100, 0), (Vec{ 0, 0, 255, 255 }));
	EXPECT_EQ(img->at<4>(150, 0), (Vec{ 0, 0, 0, 255 }));
	EXPECT_EQ(img->at<4>(190, 0), (Vec{ 255, 255, 255, 255 }));

	EXPECT_EQ(img->at<4>(0, 99), (Vec{ 255, 0, 0, 255 }));
	EXPECT_EQ(img->at<4>(50, 99), (Vec{ 0, 255, 0, 255 }));
	EXPECT_EQ(img->at<4>(100, 99), (Vec{ 0, 0, 255, 255 }));
	EXPECT_EQ(img->at<4>(150, 99), (Vec{ 255, 255, 255, 255 }));
	EXPECT_EQ(img->at<4>(190, 99), (Vec{ 0, 0, 0, 255 }));
}

TEST_F(STBimageTest, LoadsRGBApngFlipped) {
	auto img = STBimageLoader{ true }.loadFromFile(test_data_path, 4);

	ASSERT_TRUE(img.has_value());

	EXPECT_EQ(img->width(), 225);
	EXPECT_EQ(img->height(), 100);
	EXPECT_EQ(img->channels(), 4);
	EXPECT_EQ(img->pixels(), 225 * 100);
	EXPECT_EQ(img->bytes(), 225 * 100 * 4);

	using Vec = glm::vec<4, std::byte>;

	EXPECT_EQ(img->at<4>(0, 0), (Vec{ 255, 0, 0, 255 }));
	EXPECT_EQ(img->at<4>(50, 0), (Vec{ 0, 255, 0, 255 }));
	EXPECT_EQ(img->at<4>(100, 0), (Vec{ 0, 0, 255, 255 }));
	EXPECT_EQ(img->at<4>(150, 0), (Vec{ 255, 255, 255, 255 }));
	EXPECT_EQ(img->at<4>(190, 0), (Vec{ 0, 0, 0, 255 }));

	EXPECT_EQ(img->at<4>(0, 99), (Vec{ 255, 0, 0, 255 }));
	EXPECT_EQ(img->at<4>(50, 99), (Vec{ 0, 255, 0, 255 }));
	EXPECT_EQ(img->at<4>(100, 99), (Vec{ 0, 0, 255, 255 }));
	EXPECT_EQ(img->at<4>(150, 99), (Vec{ 0, 0, 0, 255 }));
	EXPECT_EQ(img->at<4>(190, 99), (Vec{ 255, 255, 255, 255 }));
}