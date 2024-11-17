#include "input/glfw/window_glfw.hpp"

#include "../../common.hpp"

using namespace sndx::input;
using namespace sndx::collision;

class GLFWwindowTest : public ::testing::Test {
public:
    void SetUp() override {
        set_test_weight<TestWeight::Integration>();

        // it's important to check if we even can test,
        // headless platforms like GitHub runners can't test GLFW
        if (glfwInit() != GLFW_TRUE) {
            const char* what = "";
            glfwGetError(&what);
            GTEST_SKIP() << "Failed to initialize GLFW: " << what;
        }

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        if (auto win = glfwCreateWindow(640, 480, "Verify Window", nullptr, nullptr); !win) {
            const char* what = "";
            glfwGetError(&what);
            GTEST_SKIP() << "Failed to create GLFW window: " << what;
        }
        else {
            glfwDestroyWindow(win);
        }
        WindowHintsGLFW::restoreDefaults();

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
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    EXPECT_THROW((WindowGLFW{"", 0, 0}), std::runtime_error);
    EXPECT_THROW((WindowGLFW{"", 1, 0}), std::runtime_error);
    EXPECT_THROW((WindowGLFW{"", 0, 1}), std::runtime_error);
    EXPECT_NO_THROW((WindowGLFW{"", 1, 1}));
}

TEST_F(GLFWwindowTest, BuilderBuilds) {
    WindowBuilderGLFW builder{getBuilder()};

    WindowHintsGLFW::restoreDefaults();
    WindowHintsGLFW hints{};
    hints.setHint(GLFW_VISIBLE, GLFW_FALSE);

    builder.setHints(&hints);

    auto window = builder.build();

    verifyBuiltWindow(window, builder);
}

TEST_F(GLFWwindowTest, CanTransform) {
    WindowHintsGLFW::restoreDefaults();
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    WindowGLFW window{"", 400, 300};

    window.setVisibility(true);
    EXPECT_TRUE(window.isVisible());

    window.setVisibility(false);
    EXPECT_FALSE(window.isVisible());

    window.setTitle("test");
    EXPECT_EQ(window.getTitle(), "test");

    window.setPosition(glm::ivec2(10, 20));
    EXPECT_EQ(window.getPosition(), glm::ivec2(10, 20));

    window.resize(glm::ivec2(450, 290));
    EXPECT_EQ(window.getSize(), glm::ivec2(450, 290));

    EXPECT_FALSE(glfwWindowShouldClose(window));
    window.tryClose();
    EXPECT_TRUE(glfwWindowShouldClose(window));
}