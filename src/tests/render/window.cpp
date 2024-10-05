#include "render/window.hpp"

#include <gtest/gtest.h>

using namespace sndx::render;

TEST(Window, BuilderCopy) {
	WindowBuilder builder{};

	builder.setTitle("Title").setX(0).setY().setWidth(-10000).setHeight(50);

	ASSERT_EQ(builder.getTitle(), "Title");
	ASSERT_EQ(builder.getX(), 0);
	ASSERT_FALSE(builder.getY());
	ASSERT_EQ(builder.getWidth(), 1);
	ASSERT_EQ(builder.getHeight(), 50);
	ASSERT_EQ(builder.getMonitor(), nullptr);
	ASSERT_EQ(builder.getShare(), nullptr);
	ASSERT_EQ(builder.getCursor(), nullptr);

	WindowBuilder copy = builder;
	copy.setX();

	EXPECT_EQ(builder.getX(), 0);

	EXPECT_EQ(copy.getTitle(), "Title");
	EXPECT_FALSE(copy.getX());
	EXPECT_FALSE(copy.getY());
	EXPECT_EQ(copy.getWidth(), 1);
	EXPECT_EQ(copy.getHeight(), 50);
	EXPECT_EQ(copy.getMonitor(), nullptr);
	EXPECT_EQ(copy.getShare(), nullptr);
	EXPECT_EQ(copy.getCursor(), nullptr);
}