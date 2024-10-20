#include "math/lines.hpp"

#include <gtest/gtest.h>

#include <array>
#include <algorithm>

using namespace sndx::math;

template <bool expected, class T>
void testColinear(T a, T b, T c) {
	std::array<T*, 3> arr{ &a, &b, &c };
	std::sort(arr.begin(), arr.end());

	while (std::next_permutation(arr.begin(), arr.end())) {
		if constexpr (expected) {
			EXPECT_TRUE(areColinear(*arr[0], *arr[1], *arr[2]));
		}
		else {
			EXPECT_FALSE(areColinear(*arr[0], *arr[1], *arr[2]));
		}
	}
}

TEST(Lines, Colinear3D) {
	glm::vec3 o{};
	glm::vec3 a{ 1.0f, 0.0f, 0.0f };
	glm::vec3 b{ 0.0f, 1.0f, 0.0f };
	glm::vec3 c{ 0.0f, 0.0f, 1.0f };

	glm::vec3 d{ -1.0f, 0.0f, 0.0f };
	glm::vec3 e{ 0.0f, -1.0f, 0.0f };
	glm::vec3 f{ 0.0f, 0.0f, -1.0f };

	glm::vec3 p{ 1.0f };
	glm::vec3 q{ -1.0f };

	glm::vec3 w{ 2.0f, 0.0f, 0.0f };

	testColinear<true>(a, a, b);
	testColinear<true>(o, o, a);

	testColinear<true>(o, a, d);
	testColinear<true>(a, d, w);
	testColinear<true>(o, p, q);

	testColinear<false>(a, b, c);
	testColinear<false>(d, e, f);
}

TEST(Lines, Colinear2D) {
	glm::vec2 o{};
	glm::vec2 a{ 1.0f, 0.0f };
	glm::vec2 b{ 0.0f, 1.0f };
	glm::vec2 c{ 0.0f, 0.0f };

	glm::vec2 d{ -1.0f, 0.0f };
	glm::vec2 e{ 0.0f, -1.0f };
	glm::vec2 f{ 0.0f, 0.0f  };

	glm::vec2 p{ 1.0f };
	glm::vec2 q{ -1.0f };

	glm::vec2 w{ 2.0f, 0.0f };

	testColinear<true>(a, a, b);
	testColinear<true>(o, o, a);

	testColinear<true>(o, a, d);
	testColinear<true>(a, d, w);
	testColinear<true>(o, p, q);

	testColinear<false>(a, b, c);
	testColinear<false>(d, e, f);
}

TEST(Lines, LinearBezier) {
	EXPECT_DOUBLE_EQ(bezier(0.0, 1.0, 2.0), 1.0);
	EXPECT_DOUBLE_EQ(bezier(0.5, 1.0, 2.0), 1.5);
	EXPECT_DOUBLE_EQ(bezier(1.0, 1.0, 2.0), 2.0);

	EXPECT_EQ(bezier(0.0f, glm::vec3(1.0), glm::vec3(2.0)), glm::vec3(1.0));
	EXPECT_EQ(bezier(0.5f, glm::vec3(1.0), glm::vec3(2.0)), glm::vec3(1.5));
	EXPECT_EQ(bezier(1.0f, glm::vec3(1.0), glm::vec3(2.0)), glm::vec3(2.0));
}

TEST(Lines, QuadraticBezier) {
	EXPECT_EQ(bezier(0.0f, glm::vec2(1.0), glm::vec2(1.5), glm::vec2(2.0, 1.0)), glm::vec2(1.0));
	EXPECT_EQ(bezier(0.5f, glm::vec2(1.0), glm::vec2(1.5), glm::vec2(2.0, 1.0)), glm::vec2(1.5, 1.25));
	EXPECT_EQ(bezier(1.0f, glm::vec2(1.0), glm::vec2(1.5), glm::vec2(2.0, 1.0)), glm::vec2(2.0, 1.0));
}