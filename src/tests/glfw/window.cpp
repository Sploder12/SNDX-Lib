#include "glfw/window.hpp"

#include "../common.hpp"

using namespace sndx::glfw;

class GLFWwindowTest : public ::testing::Test {
public:
    void SetUp() override {
        set_test_weight(TestWeight::Integration)
        else {
            // it's important to check if we even can test,
            // headless platforms can't test GLFW
            if (glfwInit() != GLFW_TRUE) {
                auto [_, what] = GLFW::getLastError();
                GTEST_SKIP() << "Failed to initialize GLFW: " << what;
            }

            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
            if (auto win = glfwCreateWindow(640, 480, "Verify Window", nullptr, nullptr); !win) {
                auto [_, what] = GLFW::getLastError();
                GTEST_SKIP() << "Failed to create GLFW window: " << what;
            }
            else {
                glfwDestroyWindow(win);
            }
            WindowHints::restoreDefaults();

            m_testBuilder.setTitle("test").setX(0).setY(1).setWidth(320).setHeight(240);
        }
    }

    void TearDown() override {
        glfwTerminate();
    }

    static void verifyBuiltWindow(const Window& window, const WindowBuilder& builder) {
        if (auto x = builder.getX())
            EXPECT_EQ(*x, window.getPosition().x);

        if (auto y = builder.getY())
            EXPECT_EQ(*y, window.getPosition().y);

        EXPECT_EQ(builder.getWidth(), window.getSize().x);
        EXPECT_EQ(builder.getHeight(), window.getSize().y);

        auto hints = builder.getHints();
        bool visible = !hints || hints->getHintOr(GLFW_VISIBLE, GLFW_TRUE) == GLFW_TRUE;
        EXPECT_EQ(window.isVisible(), visible);

        EXPECT_EQ(builder.getTitle(), window.getTitle());
    }

    [[nodiscard]]
    const WindowBuilder& getBuilder() const {
        return m_testBuilder;
    }
private:
    WindowBuilder m_testBuilder{};
};

TEST_F(GLFWwindowTest, BadSizeWindowThrows) {
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    EXPECT_THROW((Window{"", 0, 0}), std::runtime_error);
    EXPECT_THROW((Window{"", 1, 0}), std::runtime_error);
    EXPECT_THROW((Window{"", 0, 1}), std::runtime_error);
    EXPECT_NO_THROW((Window{"", 1, 1}));
}

TEST_F(GLFWwindowTest, BuilderBuilds) {
    WindowBuilder builder{getBuilder()};

    WindowHints::restoreDefaults();
    WindowHints hints{};
    hints.setHint(GLFW_VISIBLE, GLFW_FALSE);

    builder.setHints(&hints);

    auto window = builder.build();

    verifyBuiltWindow(window, builder);
}

TEST_F(GLFWwindowTest, CanTransform) {
    WindowHints::restoreDefaults();
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    Window window{"", 400, 300};

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

    EXPECT_FALSE(window.shouldClose());
    window.tryClose();
    EXPECT_TRUE(window.shouldClose());
}