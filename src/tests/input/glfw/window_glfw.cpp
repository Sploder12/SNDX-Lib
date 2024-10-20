#include "input/glfw/window_glfw.hpp"

#include <gtest/gtest.h>

using namespace sndx::input;
using namespace sndx::collision;

class GLFWwindowTest : public ::testing::Test {
public:
    void SetUp() override {
        if (glfwInit() != GLFW_TRUE)
            GTEST_SKIP() << "Failed to initialize GLFW, can't test on this platform.";

        m_testBuilder.setTitle("test").setX(0).setY(1).setWidth(320).setHeight(240);
    }

    static void verifyBuiltWindow(const WindowGLFW& window, const WindowBuilderGLFW& builder) {
        if (auto x = builder.getX(); x.has_value())
            EXPECT_EQ(*x, window.getPosition().x);

        if (auto y = builder.getY(); y.has_value())
            EXPECT_EQ(*y, window.getPosition().y);

        EXPECT_EQ(builder.getWidth(), window.getSize().x);
        EXPECT_EQ(builder.getHeight(), window.getSize().y);

        auto hints = builder.getHints();
        bool visible = !hints || hints->getHintOr(GLFW_VISIBLE, GLFW_TRUE) == GLFW_TRUE;
        EXPECT_EQ(window.isVisible(), visible);

        EXPECT_EQ(builder.getTitle(), window.getTitle());
    }

    void TearDown() override {
        glfwTerminate();
    }

    [[nodiscard]]
    const WindowBuilderGLFW& getBuilder() const {
        return m_testBuilder;
    }
private:
    WindowBuilderGLFW m_testBuilder{};
};

TEST_F(GLFWwindowTest, BadSizeWindowThrows) {
    EXPECT_THROW((WindowGLFW{"", 0, 0}), std::runtime_error);
    EXPECT_THROW((WindowGLFW{"", 1, 0}), std::runtime_error);
    EXPECT_THROW((WindowGLFW{"", 0, 1}), std::runtime_error);
    EXPECT_NO_THROW((WindowGLFW{"", 1, 1}));
}

TEST_F(GLFWwindowTest, BuilderBuilds) {
    WindowBuilderGLFW builder{getBuilder()};

    WindowHintsGLFW hints{};
    hints.setHint(GLFW_VISIBLE, GLFW_FALSE);

    builder.setHints(&hints);

    auto window = builder.build();

    verifyBuiltWindow(window, builder);
}