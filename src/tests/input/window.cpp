#include "fake_window_backend.hpp"

using namespace ::testing;

#include "render/viewport.hpp"

TEST(Window, TestBuilder) {
	FakeWindowBuilder builder{};

	builder.setTitle("Title").setFakeAttrib(37).setX(12).setY().setVisible(true).setWidth(-10000).setHeight(50);

	EXPECT_EQ(builder.getTitle(), "Title");
	EXPECT_EQ(builder.getX(), 12);
	EXPECT_FALSE(builder.getY());
	ASSERT_EQ(builder.getWidth(), 1);
	ASSERT_EQ(builder.getHeight(), 50);

	FakeWindow window = builder.build();

	EXPECT_EQ(window.getTitle(), "Title");
	EXPECT_TRUE(window.m_visible);
	EXPECT_EQ(window.hint, 0x1337);
	EXPECT_EQ(window.m_fake, 37);
	EXPECT_EQ(window.m_size, (glm::vec2{ 1, 50 }));
}

TEST(Window, TestWindow) {
	FakeWindow fakeWindow{ "test", 10, 20, 30, true };

	fakeWindow.bind();
	fakeWindow.setPosition(glm::vec2{ 13.0f, 37.0f });

	EXPECT_EQ(fakeWindow.getTitle(), "test");
	EXPECT_EQ(fakeWindow.getPosition(), (glm::vec2{ 13.0f, 37.0f }));
	EXPECT_EQ(fakeWindow.getSize(), (glm::vec2{10, 20}));

	fakeWindow.resize(glm::vec2{ 30.0, 40.0 });
	EXPECT_EQ(fakeWindow.getSize(), (glm::vec2{ 30.0, 40.0 }));

	EXPECT_THROW(fakeWindow.resize(glm::vec2{ 0, 20 }), std::invalid_argument);
	EXPECT_THROW(fakeWindow.resize(glm::vec2{ -1, 20 }), std::invalid_argument);
	EXPECT_THROW(fakeWindow.resize(glm::vec2{ 20, 0 }), std::invalid_argument);
	EXPECT_THROW(fakeWindow.resize(glm::vec2{ 20, -1 }), std::invalid_argument);
}