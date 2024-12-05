#include "math/easings.hpp"

#include <gtest/gtest.h>

using namespace sndx::math;

void testDomain(const EasingFunc<float>& func) {
	EXPECT_FLOAT_EQ(func(0.0f), 0.0f);
	EXPECT_FLOAT_EQ(func(1.0f), 1.0f);
}

TEST(Easings, Domain) {
	testDomain(easeLinear<float>);
	testDomain(easeInQuadratic<float>);
	testDomain(easeOutQuadratic<float>);
	testDomain(easeInCubic<float>);
	testDomain(easeOutCubic<float>);
}