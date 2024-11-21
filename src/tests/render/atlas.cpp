#include "render/atlas.hpp"
#include "render/gl/texture.hpp"

#include "render/image/stbimage.hpp"
#include "../common.hpp"

#include <GLFW/glfw3.h>

using namespace sndx::render;

const std::filesystem::path test_data_path{ u8"test_data/visual/rgbbw_test_img☃.png" };

class ImageAtlasTest : public ::testing::Test {
public:
	void SetUp() override {
		set_test_weight(TestWeight::BasicIntegration);
	}
};

TEST_F(ImageAtlasTest, canBuildSingleImage) {
	AtlasBuilder<std::string> builder{};

	auto img = loadImageFile(test_data_path, 4, STBimageLoader{false});
	ASSERT_TRUE(img.has_value());

	builder.add("single", *img);

	auto atlas = builder.build(img->width(), 0);
	const auto& outImg = atlas.getImage();

	EXPECT_EQ(outImg.width(), 225);
	EXPECT_EQ(outImg.height(), 100);
	EXPECT_EQ(outImg.channels(), 4);

	EXPECT_EQ(atlas.getEntry("single").pos, (glm::vec<2, size_t>{0, 0}));
	EXPECT_EQ(atlas.getEntry("single").dims, (glm::vec<2, size_t>{225, 100}));
}

TEST_F(ImageAtlasTest, canBuildMultiImage) {
	AtlasBuilder<int> builder{};

	auto imgRGB = loadImageFile(test_data_path, 3, STBimageLoader{ true });
	ASSERT_TRUE(imgRGB.has_value());

	auto imgR = loadImageFile(test_data_path, 1, STBimageLoader{ false });
	ASSERT_TRUE(imgR.has_value());

	builder.add(0, *imgRGB);
	builder.add(1, *imgR);
	builder.add(2, *imgR);
	builder.add(3, *imgR);

	auto atlas = builder.build(imgR->width() * 2 + 1, 1);
	const auto& outImg = atlas.getImage();

	EXPECT_EQ(outImg.channels(), 3);
	EXPECT_EQ(outImg.width(), 225 * 2 + 2);
	EXPECT_EQ(outImg.height(), 100 * 2 + 2);

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			if (i == j) continue;

			EXPECT_NE(atlas.getEntry(int(i)).pos, atlas.getEntry(int(j)).pos);
		}
	}

	EXPECT_EQ(atlas.getEntry(0).dims, (glm::vec<2, size_t>{225, 100}));
	EXPECT_EQ(atlas.getEntry(1).dims, (glm::vec<2, size_t>{225, 100}));
	EXPECT_EQ(atlas.getEntry(2).dims, (glm::vec<2, size_t>{225, 100}));
	EXPECT_EQ(atlas.getEntry(3).dims, (glm::vec<2, size_t>{225, 100}));
}

class TextureAtlasTest : public ::testing::Test {
public:
	GLFWwindow* window = nullptr;

	void SetUp() override {
		set_test_weight(TestWeight::Integration)
		else {
			if (glfwInit() != GLFW_TRUE) {
				const char* what = "";
				glfwGetError(&what);
				GTEST_SKIP() << "Failed to initialize GLFW: " << what;
			}

			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
			if (window = glfwCreateWindow(640, 480, "Texture Testing", nullptr, nullptr); !window) {
				const char* what = "";
				glfwGetError(&what);
				GTEST_SKIP() << "Failed to create GLFW window: " << what;
			}

			glfwMakeContextCurrent(window);

			if (auto err = glewInit(); err != GLEW_OK) {
				GTEST_SKIP() << "Could not init glew " << glewGetErrorString(err);
			}

			// discard latest error to prevent polution
			while (glGetError() != GL_NO_ERROR) {}
			}
	}

	void TearDown() override {
		glfwDestroyWindow(window);
		window = nullptr;

		glfwTerminate();
	}
};

TEST_F(TextureAtlasTest, BasicTextureAtlas) {
	AtlasBuilder<std::string> builder{};

	auto img = loadImageFile(test_data_path, 4, STBimageLoader{ false });
	ASSERT_TRUE(img.has_value());

	builder.add("single", *img);

	auto atlas = builder.buildTexture<Texture2D>(img->width(), 0);

}