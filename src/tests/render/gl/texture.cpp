#include "render/gl/texture.hpp"

#include "render/image/stbimage.hpp"
#include "../../common.hpp"

#include "../image/image_helper.hpp"

#include <GLFW/glfw3.h>

using namespace sndx::render;

class TextureTest : public ::testing::Test {
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

TEST_F(TextureTest, CanCreateEmptyTexture) {
	Texture2D tex{ GL_TEXTURE_2D, GL_RED, 64, 64, GL_RED, GL_UNSIGNED_BYTE, nullptr };

	ASSERT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(TextureTest, BindWorks) {
	Texture2D tex{ GL_TEXTURE_2D, GL_RED, 64, 64, GL_RED, GL_UNSIGNED_BYTE, nullptr };
	ASSERT_EQ(glGetError(), GL_NO_ERROR);

	EXPECT_THROW(tex.bind(32), std::invalid_argument);
	ASSERT_EQ(glGetError(), GL_NO_ERROR);

	tex.bind();
	ASSERT_EQ(glGetError(), GL_NO_ERROR);

	tex.bind(0);
	ASSERT_EQ(glGetError(), GL_NO_ERROR);

	tex.bind(31);
	ASSERT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(TextureTest, CanLoadFromFile) {
	auto tex = loadTextureFile(test_image_path, 3, STBimageLoader{});
	ASSERT_TRUE(tex.has_value());
	ASSERT_EQ(glGetError(), GL_NO_ERROR);
}

TEST_F(TextureTest, CanConvertToImg) {
	auto originalImg = createCheckeredImage(16, 10, {0xff, 0xff, 0xff});
	auto tex = Texture2D(originalImg, 0, false);

	ASSERT_EQ(glGetError(), GL_NO_ERROR);

	auto img = tex.asImage(3);
	ASSERT_EQ(glGetError(), GL_NO_ERROR);

	EXPECT_TRUE(imageEqual(img, originalImg));
}
