#include "math/easings.hpp"

#include <gtest/gtest.h>

using namespace sndx::math;

void testDomain(EasingFunc<float> func) {
	ASSERT_FLOAT_EQ(func(0.0f), 0.0f);
	ASSERT_FLOAT_EQ(func(1.0f), 1.0f);
}

TEST(Easings, Domain) {
	testDomain(easeLinear<float>);
	testDomain(easeInQuadratic<float>);
	testDomain(easeOutQuadratic<float>);
	testDomain(easeInCubic<float>);
	testDomain(easeOutCubic<float>);
}