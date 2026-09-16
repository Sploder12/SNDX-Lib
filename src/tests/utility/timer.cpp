#include "utility/timer.hpp"

#include <gtest/gtest.h>

using namespace sndx::utility;

TEST(IntervalTimer, badStartEndSwapped) {
	IntervalTimer t{ 2.0, 0.5, 1.0 };

	EXPECT_EQ(t.time, 2.0);
	EXPECT_EQ(t.start, 0.5);
	EXPECT_EQ(t.end, 2.0);
	EXPECT_EQ(t.speed, -1.0);
	EXPECT_EQ(t.getT(), 1.0);
	EXPECT_TRUE(t.atStart());
}

TEST(IntervalTimer, stopsAtEnd) {
	IntervalTimer t{ 0.0, 2.0 };

	t.advance(500000.0);
	EXPECT_TRUE(t.atEnd());
	EXPECT_EQ(t.getT(), 1.0);
	EXPECT_EQ(t, 2.0);
}

TEST(IntervalTimer, stopsAtEndReversed) {
	IntervalTimer t{ 0.0, 2.0, -1.0 };
	ASSERT_EQ(t.time, 2.0);

	t.advance(500000.0);
	EXPECT_TRUE(t.atEnd());
	EXPECT_EQ(t.getT(), 0.0);
	EXPECT_EQ(t.time, 0.0);
}

TEST(IntervalTimer, loopsCorrectly) {
	IntervalTimer t{ 1.0, 2.0 };

	t.advance(5.5, true);
	EXPECT_EQ(t.time, 1.5);
	EXPECT_EQ(t.getT(), 0.5);
}

TEST(IntervalTimer, loopsCorrectlyReversed) {
	IntervalTimer t{ 2.0, 1.0 };

	t.advance(5.25, true);
	EXPECT_EQ(t.time, 1.75);
	EXPECT_EQ(t.getT(), 0.75);
}