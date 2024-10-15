#include "fake_window_backend.hpp"

using namespace ::testing;

#include "render/viewport.hpp"

// test that these classes can compile
#include "input/glfw/window_glfw.hpp"
template class WindowGLFW<sndx::render::Viewport>;
template class WindowGLFW<sndx::render::AspectRatioViewport>;


TEST(Window, TestBuilder) {
	FakeWindowBuilder builder{};

	builder.setTitle("Title").setFakeAttrib(37).setX(12).setY().setWidth(-10000).setHeight(50);

	EXPECT_EQ(builder.getTitle(), "Title");
	EXPECT_EQ(builder.getX(), 12);
	EXPECT_FALSE(builder.getY());
	ASSERT_EQ(builder.getWidth(), 1);
	ASSERT_EQ(builder.getHeight(), 50);

	FakeWindowHints hints{};
	hints.setHint("visible", int(true));
	
	ASSERT_EQ(hints.getHintOr("visible", 0), int(true));
	ASSERT_FALSE(hints.getHint("hint"));

	FakeWindow window = builder.build(hints);

	EXPECT_TRUE(window.m_visible);
	EXPECT_EQ(window.hint, 0xDEAD);
	EXPECT_EQ(window.m_fake, 37);
	EXPECT_EQ(window.m_size, (glm::vec2{ 1, 50 }));
}

TEST(Window, TestWindow) {
	FakeWindow fakeWindow{ 10, 20, 30, true };

	int callCounter = 0;
	auto fn = [&callCounter]() {
		++callCounter;
	};

	fakeWindow.bind();
	fakeWindow.setPosition(glm::vec2{ 13.0f, 37.0f });

	EXPECT_EQ(fakeWindow.getPosition(), (glm::vec2{ 13.0f, 37.0f }));
	EXPECT_EQ(fakeWindow.getSize(), (glm::vec2{10, 20}));

	fakeWindow.resize(glm::vec2{ 30.0, 40.0 }, fn);
	EXPECT_EQ(fakeWindow.tmpViewport, fakeWindow.viewport);

	EXPECT_EQ(callCounter, 1);
	EXPECT_EQ(fakeWindow.getSize(), (glm::vec2{ 30.0, 40.0 }));

	EXPECT_THROW(fakeWindow.resize(glm::vec2{ 0, 20 }, fn), std::invalid_argument);
	EXPECT_THROW(fakeWindow.resize(glm::vec2{ -1, 20 }, fn), std::invalid_argument);
	EXPECT_THROW(fakeWindow.resize(glm::vec2{ 20, 0 }, fn), std::invalid_argument);
	EXPECT_THROW(fakeWindow.resize(glm::vec2{ 20, -1 }, fn), std::invalid_argument);

	EXPECT_THROW(fakeWindow.setViewportSize(glm::vec2{ 0, 20 }, fn), std::invalid_argument);
	EXPECT_THROW(fakeWindow.setViewportSize(glm::vec2{ -1, 20 }, fn), std::invalid_argument);
	EXPECT_THROW(fakeWindow.setViewportSize(glm::vec2{ 20, 0 }, fn), std::invalid_argument);
	EXPECT_THROW(fakeWindow.setViewportSize(glm::vec2{ 20, -1 }, fn), std::invalid_argument);

	
	fakeWindow.setViewportOffset(glm::vec2{ -20, -30 }, fn);
	EXPECT_EQ(fakeWindow.tmpViewport, fakeWindow.viewport);

	EXPECT_EQ(callCounter, 2);
	EXPECT_EQ(fakeWindow.viewport.offset, (glm::vec2{ -20, -30 }));


	fakeWindow.setViewportSize(glm::vec2{ 1000, 2000 }, fn);
	EXPECT_EQ(fakeWindow.tmpViewport, fakeWindow.viewport);

	EXPECT_EQ(callCounter, 3);
	EXPECT_EQ(fakeWindow.viewport.size, (glm::vec2{ 1000, 2000 }));
}